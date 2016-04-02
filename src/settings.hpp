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
#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <string>
#include <cstdint>
#include <unordered_map>

#include <boost/property_tree/ptree.hpp>

#include "melanolib/utils/c++-compat.hpp"
#include "string/string.hpp"
#include "melanobot/error.hpp"

using PropertyTree = boost::property_tree::ptree;
using Settings = PropertyTree;

namespace settings
{
    /**
    * \brief File format used to open/save settings
    */
    enum class FileFormat
    {
        AUTO, ///< Deduce automatically
        JSON,
        INI,
        XML,
        INFO,
    };

    /**
     * \brief File checking policy
     */
    enum class FileCheck
    {
        NO_CHECK, ///< Don't check if the file exists or not
        EXISTING, ///< The file must be already existing
        CREATE,   ///< The file must be created if not found
    };

    /**
     * \brief Settings with global information
     */
    extern Settings global_settings;

    /**
     * \brief Parses the program options and returns the configurations
     *
     * It will also load the logger configuration
     */
    Settings initialize ( int argc, char** argv );

    /**
     * \brief Tries to find a file from which settings can be loaded
     */
    std::string find_config( FileFormat format = FileFormat::AUTO );


    /**
     * \brief Load settings from file
     */
    Settings load( const std::string& file_name, FileFormat format = FileFormat::AUTO );

    /**
     * \brief Whether a child node/property exists
     */
    inline bool has_child ( const Settings& s, const Settings::path_type& path )
    {
        return bool(s.get_child_optional(path));
    }

    /**
     * \brief Merge a node with the supplied values
     * \param target    Node to be overwritten
     * \param source    Node with the properties to be read
     * \param overwrite If \b true all of the properties of \c source will be used,
     *                  if \b false, only those not already found in the tree
     */
    void merge(Settings& target, const Settings& source, bool overwrite);

    /**
     * \brief Same as \c merge but instead of modifying \c target, it returns a new object
     */
    inline Settings merge_copy(const Settings& target, const Settings& source, bool overwrite)
    {
        Settings copy = target;
        merge(copy,source,overwrite);
        return copy;
    }

    /**
     * \brief Recursively calls a functor on every node of the tree
     */
    template<class Func>
        void recurse(Settings& sett, const Func& func)
        {
            func(sett);
            for ( auto& child : sett )
                recurse(child.second,func);
        }

    /**
     * \brief Recursively calls a functor on every node of the tree
     *
     * If \c func returns true, \c breakable_recurse returns
     */
    template<class Func>
        bool breakable_recurse(Settings& sett, const Func& func)
        {
            if ( func(sett) )
                return true;
            for ( auto& child : sett )
                if ( breakable_recurse(child.second,func) )
                    return true;
            return false;
        }

    /**
     * \brief Initialize Settings from a simple initializer list
     * \todo Could use a structure to allow a recursive initializer
     */
    inline Settings from_initializer(const std::initializer_list<std::string>& init)
    {
        Settings sett;
        for ( const auto& key : init )
            sett.put_child(key, {});
        return sett;
    }

    /**
     * \brief Gets the full path to a data file
     * \param path      Path relative to the data directory
     * \param check     Whether to check that the path exists
     * \return The path to the requested file or an empty string
     *         if \c check is \b EXISTING and the file doesn't exist
     */
    std::string data_file(const std::string& path, FileCheck check = FileCheck::EXISTING);
}

std::ostream& operator<< ( std::ostream& stream, const Settings& settings );

/**
 * \brief Key-value map used to store object properties
 */
using Properties = std::unordered_map<std::string,std::string>;

/**
 * \brief Converts flat properties to a tree
 *
 * Properties containing "." will be split into several nodes
 */
PropertyTree properties_to_tree(const Properties& properties);

#endif // SETTINGS_HPP
