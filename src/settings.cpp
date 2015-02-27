/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015 Mattia Basaglia
 * \section License
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

#include <unordered_map>
#include <cstdlib>

#include <boost/filesystem.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include "string/logger.hpp"
#include "config.hpp"

namespace settings {

static std::unordered_map<std::string,FileFormat> format_extension = {
    {".json",FileFormat::JSON},
    {".info",FileFormat::INFO},
    {".xml",FileFormat::XML},
    {".ini",FileFormat::INI},
};

static boost::filesystem::path exe_dir;
static boost::filesystem::path home_dir;

void initialize ( int argc, char** argv )
{
    boost::system::error_code err;

    boost::filesystem::path path = argv[0];
    exe_dir = boost::filesystem::canonical(path.parent_path(),err);
    if ( err )
        exe_dir.clear();

    path = std::getenv("HOME");
    home_dir = boost::filesystem::canonical(path,err);
    if ( err )
        home_dir.clear();
}

Settings load ( const std::string& file_name, FileFormat format )
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

    Settings settings;
    switch ( format )
    {
        case FileFormat::INFO:
            boost::property_tree::info_parser::read_info(file_name,settings);
            break;
        case FileFormat::INI:
            boost::property_tree::ini_parser::read_ini(file_name,settings);
            break;
        case FileFormat::JSON:
            boost::property_tree::json_parser::read_json(file_name,settings);
            break;
        case FileFormat::XML:
            boost::property_tree::xml_parser::read_xml(file_name,settings);
            break;
        case FileFormat::AUTO:
            CRITICAL_ERROR("Cannot detect file format for "+file_name);
    }

    return settings;
}

static boost::filesystem::path find_config ( const boost::filesystem::path& dir, FileFormat format)
{
    boost::system::error_code err;
    auto status = boost::filesystem::status(dir,err);
    if ( status.type() != boost::filesystem::directory_file || err )
        return {};


    for ( const auto& p : format_extension )
    {
        if ( format == FileFormat::AUTO || format == p.second )
        {
            boost::filesystem::path fp = dir.string() + ("/config"+p.first);
            if ( boost::filesystem::exists(fp) )
            {
                boost::system::error_code err;
                fp = boost::filesystem::canonical(fp,err).string();
                if ( !err )
                    return fp;
            }
        }
    }

    return {};
}
std::string find_config(FileFormat format)
{
    std::vector<boost::filesystem::path> paths;
    paths.push_back(".");
    if ( !exe_dir.empty() )
        paths.push_back(exe_dir);
    if ( !home_dir.empty() )
    {
        paths.push_back(home_dir.string() + "/.config/" + PROJECT_SHORTNAME);
        paths.push_back(home_dir.string() + ("/." PROJECT_SHORTNAME));
    }

    for ( const auto& path : paths )
    {
        auto file = find_config(path, format);
        if ( !file.empty() )
            return file.string();
    }

    return {};
}

} // namespace settings
