/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015-2016 Mattia Basaglia
 * \license
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "settings.hpp"

#include <cstdlib>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include "string/logger.hpp"
#include "string/json.hpp"
#include "config.hpp"

static std::unordered_map<std::string, settings::FileFormat> format_extension = {
    {".json", settings::FileFormat::JSON},
    {".info", settings::FileFormat::INFO},
    {".xml", settings::FileFormat::XML},
    {".ini", settings::FileFormat::INI},
};

Settings settings::global_settings;

Settings settings::initialize ( int argc, char** argv )
{
    // Global settings
    global_settings.put("website", PROJECT_WEBSITE);

    // By default exit code is 0 (success)
    global_settings.put("exit_code", 0);

    // Find system paths
    boost::system::error_code err;

    // Executable name and path
    boost::filesystem::path path = argv[0];
    global_settings.put("executable", path.filename().empty() ?
        std::string(PROJECT_SHORTNAME) : path.filename().string() );
    auto exe_dir = boost::filesystem::canonical(
        boost::filesystem::system_complete(path).parent_path(), err);
    if ( !err )
    {
        global_settings.put("path.executable", exe_dir.string());
    }

    // Library paths
    std::vector<std::string> library_path;
    boost::filesystem::path dir("/lib/melanobot");
    library_path.push_back("/usr"+dir.string());
    library_path.push_back("/usr/local"+dir.string());
    if ( !err )
    {
        library_path.push_back((exe_dir.parent_path()/dir).string());
    }
    global_settings.put("path.library", melanolib::string::implode(":", library_path));

    // Home
    path = std::getenv("HOME");
    std::string home_dir = boost::filesystem::canonical(path, err).string();
    if ( !err )
        global_settings.put("path.home", home_dir);

    // Load command line options
    string::FormatterAnsi fmt(true);
    auto bold = fmt.to_string(string::FormatFlags::BOLD);
    auto clear = fmt.to_string(string::ClearFormatting());
    namespace po = boost::program_options;
    po::options_description described_options(bold+"Options"+clear);
    described_options.add_options()
        ("help", "Print a description of the command-line options")
        ("config", po::value<std::string>(), "Configuration file path")
    ;
    po::options_description options;
    options.add(described_options);
    options.add_options()
        ("log.debug", po::value<int>(), "")
    ;
    po::parsed_options parsed =
        po::command_line_parser(argc, argv).options(options).allow_unregistered().run();
    po::variables_map vm;
    po::store(parsed, vm);
    po::notify(vm);

    // Show help and exit
    if ( vm.count("help") )
    {
        std::cout << bold << "Version" << clear << ":\n";
        std::cout << "  " PROJECT_NAME " " PROJECT_DEV_VERSION << "\n";
        std::cout << bold << "Usage" << clear << ":\n";
        std::cout << "  " << global_settings.get("executable", "") << " [option ...]\n";
        std::cout << described_options;
        std::cout << bold << "System" << clear << ":\n";
        std::cout << "  " SYSTEM_NAME " " SYSTEM_VERSION " " SYSTEM_PROCESSOR " " SYSTEM_COMPILER "\n";
        std::cout << bold << "Website" << clear << ":\n";
        std::cout << "  " PROJECT_WEBSITE "\n";
        std::cout << "\n";
        return {};
    }

    // Extract debug settings from command line because config might throw exceptions
    int debug = 0;
    if ( vm.count("log.debug") )
    {
        debug = vm["log.debug"].as<int>();
        global_settings.put("debug", debug);
    }

    // Get the right config file
    std::string settings_file;

    if ( vm.count("config") )
        settings_file = vm["config"].as<std::string>();
    else
        settings_file = find_config();

    global_settings.put("config", settings_file);

    if ( settings_file.empty() )
    {
        global_settings.put("exit_code", 1);
        ErrorLog("sys") << "Cannot start without a config file";
        return {};
    }

    // Load config
    Settings settings = load(settings_file);

    // Overwrite config options from the command line
    for ( const auto& opt : parsed.options )
    {
        if ( opt.unregistered && !opt.value.empty() )
        {
            if ( melanolib::string::starts_with(opt.string_key, "settings.") )
                global_settings.put(opt.string_key, opt.value.front());
            else
                settings.put(opt.string_key, opt.value.front());
        }
    }

    debug = settings.get("log.debug", debug);
    global_settings.put("debug", debug);

    return settings;
}

Settings settings::load ( const std::string& file_name, FileFormat format )
{
    boost::filesystem::path path = file_name;

    if ( format == FileFormat::AUTO )
    {
        auto it = format_extension.find(path.extension().string());
        if ( it != format_extension.end() )
            format = it->second;
    }

    boost::system::error_code err;
    auto status = boost::filesystem::status(file_name, err);
    if ( status.type() != boost::filesystem::regular_file || err )
    {
        ErrorLog("sys") << "Cannot load config file: " << file_name;
        return {};
    }

    Settings ptree;
    switch ( format )
    {
        case FileFormat::INFO:
            boost::property_tree::info_parser::read_info(file_name, ptree);
            break;
        case FileFormat::INI:
            boost::property_tree::ini_parser::read_ini(file_name, ptree);
            break;
        case FileFormat::JSON:
        {
            JsonParser parser;
            ptree = parser.parse_file(file_name);
            break;
        }
        case FileFormat::XML:
            boost::property_tree::xml_parser::read_xml(file_name, ptree);
            break;
        case FileFormat::AUTO:
            ErrorLog("sys") << "Cannot detect file format for " << file_name;
            return {};
    }
    return ptree;
}

static std::string find_config ( const std::string& dir, settings::FileFormat format)
{
    boost::system::error_code err;
    auto status = boost::filesystem::status(dir, err);
    if ( status.type() != boost::filesystem::directory_file || err )
        return {};


    for ( const auto& p : format_extension )
    {
        if ( format == settings::FileFormat::AUTO || format == p.second )
        {
            boost::filesystem::path fp = dir + "/config"+p.first;
            if ( boost::filesystem::exists(fp) )
            {
                boost::system::error_code err;
                fp = boost::filesystem::canonical(fp, err).string();
                if ( !err )
                    return fp.string();
            }
        }
    }

    return {};
}
std::string settings::find_config(FileFormat format)
{
    std::vector<std::string> paths;
    paths.push_back(".");
    std::string home_dir = global_settings.get("path.home", "");
    if ( !home_dir.empty() )
    {
        paths.push_back(home_dir + "/.config/" + PROJECT_SHORTNAME);
        paths.push_back(home_dir + ("/." PROJECT_SHORTNAME));
    }
    std::string exe_dir = global_settings.get("path.executable", "");
    if ( !exe_dir.empty() )
        paths.push_back(exe_dir);

    for ( const auto& path : paths )
    {
        auto file = ::find_config(path, format);
        if ( !file.empty() )
            return file;
    }

    return {};
}

/**
 * \brief Recustively prints a ptree
 */
static std::ostream& recursive_print(std::ostream& stream,
                                     const boost::property_tree::ptree& tree,
                                     int depth = 0)
{
    for ( const auto& p : tree )
    {
        stream << std::string(depth*2, ' ') << p.first << ": " << p.second.data() << '\n';
        recursive_print(stream, p.second, depth+1);
    }
    return stream;
}


std::ostream& operator<< ( std::ostream& stream, const Settings& settings )
{
    return recursive_print(stream, settings);
}

/**
 * \brief Info about data paths
 */
struct DataPathInfo
{
    DataPathInfo()
    {
        // Search current directory
        paths.push_back(".");

        std::string home_dir = settings::global_settings.get("path.home", "");
        if ( !home_dir.empty() )
        {
            // Search ~/.config/melanobot
            paths.push_back(home_dir + "/.config/" + PROJECT_SHORTNAME);
            best_match = paths.back();
            // Search ~/.melanobot
            paths.push_back(home_dir + ("/." PROJECT_SHORTNAME));
        }

        // Search installation directory PREFIX/share/melanobot
        std::string exe_dir = settings::global_settings.get("path.executable", "");
        if ( !exe_dir.empty() )
            paths.push_back(exe_dir+"/../share/"+PROJECT_SHORTNAME);
    }

    std::vector<std::string> paths; ///< All search paths
    std::string best_match=".";     ///< Preferred path
};

std::string settings::data_file(const std::string& rel_path, FileCheck check)
{
    using namespace boost::filesystem;

    static DataPathInfo paths;

    if ( check == FileCheck::NO_CHECK )
        return absolute(paths.best_match+"/"+rel_path).string();

    for ( const auto& dir : paths.paths )
    {
        std::string full = dir+"/"+rel_path;
        if ( exists(full) )
            return canonical(full).string();
    }

    if ( check == FileCheck::CREATE )
    {
        path file = absolute(paths.best_match + "/" + rel_path);
        create_directory(file.parent_path());
        std::ofstream{file.string()};
        return file.string();
    }

    return {};
}

PropertyTree properties_to_tree(const Properties& properties)
{
    PropertyTree ptree;
    for ( const auto& prop : properties )
        ptree.put(prop.first, prop.second);
    return ptree;
}

void settings::merge ( Settings& target, const Settings& source, bool overwrite )
{
    if ( overwrite )
        target.data() = source.data();

    for ( const auto& prop : source )
    {
        auto child = target.get_child_optional(prop.first);
        if ( !child )
            target.put_child(prop.first, prop.second);
        else
            merge(*child, prop.second, overwrite);
    }
}
