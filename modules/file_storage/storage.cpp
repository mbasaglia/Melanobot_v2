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

#include "storage.hpp"

#include <fstream>

#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "string/string_functions.hpp"
#include "string/json.hpp"

void Storage::initialize (const Settings& settings)
{
    std::string formatstring = string::strtolower(settings.get("format","json"));
    if ( formatstring == "xml" )
        format = settings::FileFormat::XML;
    else if ( formatstring == "json" )
        format = settings::FileFormat::JSON;
    else
        throw ConfigurationError("Wrong storage format");

    filename = settings.get("file",
        settings::data_file("storage.json", settings::FileCheck::CREATE));
    if ( filename.empty() )
        throw ConfigurationError("Wrong storage file name");

    if ( filename[0] != '/' )
        filename = settings::data_file(filename);

    lazy_save = settings.get("lazy",lazy_save);

    cache_policy = cache::Policy(settings.get_child("cache",{}));
}

void Storage::start()
{
    ThreadedAsyncService::start();
    cache_policy.mark_initializing();
    maybe_load();
    cache_policy.mark_initialized();
}

void Storage::stop()
{
    ThreadedAsyncService::stop();
    cache_policy.mark_finalizing();
    maybe_save();
}

void Storage::maybe_load()
{
    if ( cache_policy.should_read() )
        load();
}

void Storage::load()
{
    data.clear();
    cache_policy.mark_clean();
    if ( std::ifstream file{filename} )
    {
        Log("sys",'!',4) << "Loading settings from " << filename;
        if ( format == settings::FileFormat::XML )
            boost::property_tree::read_xml(file, data);
        else
            data = JsonParser().parse_file(filename);
    }
}

void Storage::maybe_save()
{
    if ( cache_policy.should_write() )
        save();
}

void Storage::save()
{
    if ( std::ofstream file{filename} )
    {
        cache_policy.mark_clean();
        Log("sys",'!',4) << "Writing settings to " << filename;
        if ( format == settings::FileFormat::XML )
            boost::property_tree::write_xml(file, data);
        else
            boost::property_tree::write_json(file, data);
    }
    else
    {
        ErrorLog("sys") << "Cannot write settings to " << filename;
    }
}

std::string Storage::get(const std::string& key)
{
    maybe_load();
    if ( auto var = data.get_optional<std::string>(key) )
        return *var;
    throw Error("Not found");
}

std::string Storage::maybe_get(const std::string& key,
                               const std::string& default_value)
{
    maybe_load();
    if ( auto var = data.get_optional<std::string>(key) )
        return *var;
    return default_value;
}

std::string Storage::put(const std::string& key, const std::string& value)
{
    data.put(key, value);
    cache_policy.mark_dirty();
    maybe_save();
    return value;
}

std::string Storage::maybe_put(const std::string& key, const std::string& value)
{
    auto var = data.get_optional<std::string>(key);
    if ( !var )
        return put(key, value);
    return *var;
}

void Storage::erase(const std::string& key)
{
    if ( !data.erase(key) )
        throw Error("Not found");
    maybe_save();
}

network::Response Storage::query(const network::Request& request)
{
    network::Response resp;
    resp.resource = request.resource;

    try
    {
        if ( request.command == "get" )
        {
            resp.contents = get(request.resource);
        }
        else if ( request.command == "maybe_get" )
        {
            if ( request.parameters.empty() )
                throw Error("Missing argument");

            resp.contents = maybe_get(request.resource, request.parameters[0]);
        }
        else if ( request.command == "put" )
        {
            if ( request.parameters.empty() )
                throw Error("Missing argument");

            resp.contents = put(request.resource, request.parameters[0]);
        }
        else if ( request.command == "maybe_put" )
        {
            if ( request.parameters.empty() )
                throw Error("Missing argument");

            resp.contents = maybe_put(request.resource, request.parameters[0]);
        }
        else if ( request.command == "delete" )
        {
            erase(request.resource);
        }
    }
    catch ( const Error& err )
    {
        /// \todo Log error?
        resp.error_message = err.what();
    }
    catch ( const JsonError& err )
    {
        ErrorLog errlog("web","JSON Error");
        if ( settings::global_settings.get("debug",0) )
            errlog << err.file << ':' << err.line << ": ";
        errlog << err.what();

        resp.error_message = "Malformed file";
    }

    return resp;
}
