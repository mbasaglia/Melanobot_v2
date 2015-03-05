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
#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <string>
#include <cstdint>
#include <unordered_map>

#include <boost/property_tree/ptree.hpp>


/**
 * \brief Class containing hierarchical settings
 */
class Settings : public boost::property_tree::ptree
{
public:
    typedef boost::property_tree::ptree PTree;

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
     * \brief Settings with global information
     */
    static Settings global_settings;

    /**
     * \brief Parses the program options and returns the configurations
     *
     * It will also load the logger configuration
     */
    static Settings initialize ( int argc, char** argv );

    /**
     * \brief Tries to find a file from which settings can be loaded
     */
    static std::string find_config( FileFormat format = FileFormat::AUTO );

    Settings() {}
    Settings(const PTree& p) : PTree(p) {}

    /**
     * \brief Load settings from file
     * \throw CriticalException If \c file_name isn't valid
     */
    explicit Settings ( const std::string& file_name, FileFormat format = FileFormat::AUTO );

    /**
     * \brief Whether a child node/property exists
     */
    bool has_child ( const path_type& path ) const
    {
        return get_child_optional(path);
    }

    /**
     * \brief Merge a child node with the supplied values
     * \param path      Path to the child
     * \param child     Node with the properties to be read
     * \param overwrite If \b true all of the properties of \c child will be used,
     *                  if \b false, only those not already found in the tree
     */
    void merge_child( const path_type& path, const Settings& child, bool overwrite)
    {
        for ( const auto& prop : child )
        {
            path_type prop_path = path;
            prop_path /= prop.first;
            if ( overwrite || !has_child(prop_path) )
                put(prop_path,prop.second.data());
        }
    }

private:
    /**
     * \brief Maps extensions to file formats
     */
    static std::unordered_map<std::string,FileFormat> format_extension;


    /**
     * \brief Tries to find a config file in the given directory
     */
    static std::string find_config ( const std::string& dir, FileFormat format);
};



/**
 * \brief Class representing an error occurring during configuration
 */
class ConfigurationError : public std::runtime_error
{
public:
    ConfigurationError(const std::string& msg = "Invalid configuration parameters")
        : std::runtime_error(msg)
    {}
};

#endif // SETTINGS_HPP
