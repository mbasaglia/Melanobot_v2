/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015-2016 Mattia Basaglia
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

#include "file_storage.hpp"

#include <fstream>

#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/info_parser.hpp>

#include "melanolib/string/stringutils.hpp"
#include "string/json.hpp"
#include "string/logger.hpp"

namespace storage {
namespace file {

Storage::sequence Storage::node_to_sequence(const PropertyTree& node)
{
    sequence ret;
    ret.reserve(node.size());
    for ( const auto& pair : node )
        ret.push_back(pair.second.data());
    return ret;
}
Storage::table Storage::node_to_map(const PropertyTree& node)
{
    table ret;
    for ( const auto& pair : node )
        ret[pair.first] = pair.second.data();
    return ret;
}
PropertyTree Storage::node_from_sequence(const sequence& value)
{
    PropertyTree pt;
    for ( unsigned i = 0; i < value.size(); i++ )
        pt.put(std::to_string(i), value[i]);
    return pt;
}
PropertyTree Storage::node_from_map(const table& value)
{
    PropertyTree pt;
    for ( const auto& pair : value )
        pt.push_back({pair.first, PropertyTree(pair.second)});
    return pt;
}

Storage::Storage(const Settings& settings)
    : cache_policy{melanobot::CachePolicy::Read::ONCE, melanobot::CachePolicy::Write::DYNAMIC}
{
    std::string formatstring = melanolib::string::strtolower(settings.get("format", "info"));
    if ( formatstring == "xml" )
        format = settings::FileFormat::XML;
    else if ( formatstring == "json" )
        format = settings::FileFormat::JSON;
    else if ( formatstring == "info" )
        format = settings::FileFormat::INFO;
    else
        throw melanobot::ConfigurationError("Wrong storage format");

    filename = settings.get("file",
        settings::data_file("storage."+formatstring, settings::FileCheck::CREATE));
    if ( filename.empty() )
        throw melanobot::ConfigurationError("Wrong storage file name");

    if ( filename[0] != '/' )
        filename = settings::data_file(filename);

    cache_policy.load_settings(settings);

    cache_policy.mark_initializing();
    maybe_load();
    cache_policy.mark_initialized();
}

Storage::~Storage()
{
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
        Log("sys", '!', 4) << "Loading settings from " << filename;
        if ( format ==  settings::FileFormat::INFO )
            boost::property_tree::read_info(file, data);
        else if ( format == settings::FileFormat::XML )
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
        Log("sys", '!', 4) << "Writing settings to " << filename;

        if ( format == settings::FileFormat::INFO )
            boost::property_tree::write_info(file, data);
        else if ( format == settings::FileFormat::XML )
            boost::property_tree::write_xml(file, data);
        else
            boost::property_tree::write_json(file, data);
    }
    else
    {
        ErrorLog("sys") << "Cannot write settings to " << filename;
    }
}

Storage::value_type Storage::get_value(const key_type& path)
{
    maybe_load();
    if ( auto var = data.get_optional<value_type>(path) )
        return *var;
    throw melanobot::StorageError("Storage key not found: " + path);
}
Storage::sequence Storage::get_sequence(const key_type& path)
{
    maybe_load();
    if ( auto child = data.get_child_optional(path) )
        return node_to_sequence(*child);
    throw melanobot::StorageError("Storage key not found: " + path);
}
Storage::table Storage::get_map(const key_type& path)
{
    maybe_load();
    if ( auto child = data.get_child_optional(path) )
        return node_to_map(*child);
    throw melanobot::StorageError("Storage key not found: " + path);
}

Storage::value_type Storage::maybe_get_value(const key_type& path,
                                             const value_type& default_value )
{
    maybe_load();
    if ( auto var = data.get_optional<std::string>(path) )
        return *var;
    return default_value;
}
Storage::sequence Storage::maybe_get_sequence(const key_type& path)
{
    maybe_load();
    if ( auto child = data.get_child_optional(path) )
        return node_to_sequence(*child);
    return {};
}
Storage::table Storage::maybe_get_map(const key_type& path)
{
    maybe_load();
    if ( auto child = data.get_child_optional(path) )
        return node_to_map(*child);
    return table{};
}

void Storage::put(const key_type& path, const value_type& value)
{
    data.put(path, value);
    cache_policy.mark_dirty();
    maybe_save();
}
void Storage::put(const key_type& path, const sequence& value)
{
    data.put_child(path, node_from_sequence(value));
    cache_policy.mark_dirty();
    maybe_save();
}
void Storage::put(const key_type& path, const table& value)
{
    data.put_child(path, node_from_map(value));
    cache_policy.mark_dirty();
    maybe_save();
}
void Storage::put(const key_type& path, const key_type& key, const value_type& value)
{
    auto pt = data.get_child_optional(path);
    if ( !pt )
        pt = data.put_child(path, {});
    pt->push_back({key, PropertyTree(value)});
    cache_policy.mark_dirty();
    maybe_save();
}

Storage::value_type Storage::maybe_put(const key_type& path, const value_type& value)
{
    maybe_load();
    if ( auto child = data.get_optional<value_type>(path) )
        return *child;
    put(path, value);
    return value;
}
Storage::sequence Storage::maybe_put(const key_type& path, const sequence& value)
{
    maybe_load();
    if ( auto child = data.get_child_optional(path) )
        return node_to_sequence(*child);
    put(path, value);
    return value;
}
Storage::table Storage::maybe_put(const key_type& path, const table& value)
{
    maybe_load();
    if ( auto child = data.get_child_optional(path) )
        return node_to_map(*child);
    put(path, value);
    return value;
}

void Storage::append(const key_type& path, const value_type& element)
{
    maybe_load();
    if ( auto child = data.get_child_optional(path) )
        child->put(std::to_string(child->size()), element);
    else
        data.put(path+".0", element);
}

int Storage::erase(const key_type& path)
{
    // Start from the root
    PropertyTree* node = &data;
    // Get the path
    PropertyTree::path_type path_tail = path;
    // Must point to a node to erase it
    if ( path_tail.empty() )
        return 0;

    // Search the parent of the node pointed by path
    while ( !path_tail.single() )
    {
        // path_tail always contains at least two elements
        auto next = path_tail.reduce();
        // find the next node one level deep,
        // only follows the first one if more match
        auto it = node->find(next);
        if ( it == node->not_found() )
            return 0;
        // repeat on the next node
        node = &it->second;
    }

    // path_tail contains exactly one element
    if ( int erased = node->erase(path_tail.reduce()) )
    {
        // erased at least one node
        maybe_save();
        return erased;
    }

    return 0;
}
int Storage::erase(const key_type& path, const key_type& key)
{
    if ( auto child = data.get_child_optional(path) )
    {
        if ( int erased = child->erase(key) )
        {
            maybe_save();
            return erased;
        }
    }

    return 0;
}


}} // namespace storage::file
