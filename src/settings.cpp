/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015 Mattia Basaglia
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

std::unordered_map<std::string,Settings::FileFormat> Settings::format_extension = {
    {".json",FileFormat::JSON},
    {".info",FileFormat::INFO},
    {".xml",FileFormat::XML},
    {".ini",FileFormat::INI},
};

Settings Settings::global_settings;

Settings Settings::initialize ( int argc, char** argv )
{
    // Global settings
    global_settings.put("website",PROJECT_WEBSITE);

    // By default exit code is 0 (success)
    global_settings.put("exit_code",0);

    // Find system paths
    boost::system::error_code err;
    // Executable name and path
    boost::filesystem::path path = argv[0];
    global_settings.put("executable", path.filename().empty() ?
        std::string(PROJECT_SHORTNAME) : path.filename().string() );
    std::string exe_dir = boost::filesystem::canonical(path.parent_path(),err).string();
    if ( !err )
        global_settings.put("path.executable",exe_dir);
    // Home
    path = std::getenv("HOME");
    std::string home_dir = boost::filesystem::canonical(path,err).string();
    if ( !err )
        global_settings.put("path.home",home_dir);

    // Load command line options
    namespace po = boost::program_options;
    po::options_description described_options("Options");
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
        std::cout << "Usage:\n";
        std::cout << "  " << global_settings.get("executable","") << " [option ...]\n";
        std::cout << described_options;
        return {};
    }

    // Extract debug settings from command line because config might throw exceptions
    int debug = 0;
    if ( vm.count("log.debug") )
    {
        debug = vm["log.debug"].as<int>();
        global_settings.put("debug",debug);
    }

    // Get the right config file
    std::string settings_file;

    if ( vm.count("config") )
        settings_file = vm["config"].as<std::string>();
    else
        settings_file = find_config();

    global_settings.put("config",settings_file);

    if ( settings_file.empty() )
    {
        ErrorLog("sys") << "Cannot start without a config file";
        return {};
    }

    // Load config
    Settings settings(settings_file);

    // Overwrite config options from the command line
    for ( const auto& opt : parsed.options )
    {
        if ( opt.unregistered && !opt.value.empty() )
            settings.put(opt.string_key,opt.value.front());
    }

    debug = settings.get("log.debug", debug);
    global_settings.put("debug",debug);

    return settings;
}

Settings::Settings ( const std::string& file_name, FileFormat format )
{
    boost::filesystem::path path = file_name;

    if ( format == FileFormat::AUTO )
    {
        auto it = format_extension.find(path.extension().string());
        if ( it != format_extension.end() )
            format = it->second;
    }

    boost::system::error_code err;
    auto status = boost::filesystem::status(file_name,err);
    if ( status.type() != boost::filesystem::regular_file || err )
        CRITICAL_ERROR("Cannot load config file: "+file_name);

    PTree& ptree = *this;
    switch ( format )
    {
        case FileFormat::INFO:
            boost::property_tree::info_parser::read_info(file_name,ptree);
            break;
        case FileFormat::INI:
            boost::property_tree::ini_parser::read_ini(file_name,ptree);
            break;
        case FileFormat::JSON:
        {
            JsonParser parser;
            ptree = parser.parse_file(file_name);
            break;
        }
        case FileFormat::XML:
            boost::property_tree::xml_parser::read_xml(file_name,ptree);
            break;
        case FileFormat::AUTO:
            CRITICAL_ERROR("Cannot detect file format for "+file_name);
    }
}

std::string Settings::find_config ( const std::string& dir, FileFormat format)
{
    boost::system::error_code err;
    auto status = boost::filesystem::status(dir,err);
    if ( status.type() != boost::filesystem::directory_file || err )
        return {};


    for ( const auto& p : format_extension )
    {
        if ( format == FileFormat::AUTO || format == p.second )
        {
            boost::filesystem::path fp = dir + "/config"+p.first;
            if ( boost::filesystem::exists(fp) )
            {
                boost::system::error_code err;
                fp = boost::filesystem::canonical(fp,err).string();
                if ( !err )
                    return fp.string();
            }
        }
    }

    return {};
}
std::string Settings::find_config(FileFormat format)
{
    std::vector<std::string> paths;
    paths.push_back(".");
    std::string exe_dir = global_settings.get("path.executable","");
    if ( !exe_dir.empty() )
        paths.push_back(exe_dir);
    std::string home_dir = global_settings.get("path.home","");
    if ( !home_dir.empty() )
    {
        paths.push_back(home_dir + "/.config/" + PROJECT_SHORTNAME);
        paths.push_back(home_dir + ("/." PROJECT_SHORTNAME));
    }

    for ( const auto& path : paths )
    {
        auto file = find_config(path, format);
        if ( !file.empty() )
            return file;
    }

    return {};
}
