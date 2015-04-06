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

#include "c++-compat.hpp"
#include "string/string.hpp"


using PropertyTree = boost::property_tree::ptree;

/**
 * \brief Class containing hierarchical settings
 * \todo Avoid copies on conversion from PropertyTree:
 *       store PropertyTree as an optionally-owned pointer and copy relevant
 *       functions in the interface.
 *       (This would also allow merge() to be non-static)
 */
class Settings : public PropertyTree
{
public:
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
    Settings(const PropertyTree& p) : PropertyTree(p) {}
    Settings(const Settings&) = default;
    Settings(Settings&&) = default;
    Settings& operator=(const Settings&) = default;
    Settings& operator=(Settings&&) = default;

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
     * \brief Merge a node with the supplied values
     * \param target    Node to be overwritten
     * \param source    Node with the properties to be read
     * \param overwrite If \b true all of the properties of \c source will be used,
     *                  if \b false, only those not already found in the tree
     */
    static void merge( PropertyTree& target, const PropertyTree& source, bool overwrite)
    {
        for ( const auto& prop : source )
        {
            if ( overwrite || !target.get_child_optional(prop.first) )
                target.put(prop.first,prop.second.data());
        }
    }

    /**
     * \brief Possibly convert between std::optional and boost::optional
     */
    template<class T, class S/*,
        class = std::enable_if_t<!std::is_same<Optional<T>,boost::optional<T>>::value>,
        class = std::enable_if_t<std::is_convertible<S,std::string>::value>*/>
    Optional<T> get_optional(S&& path) const
    {
        auto opt = PropertyTree::get_optional<T>(std::forward<S>(path));
        return opt ? *opt : Optional<T>();
    }

    /**
     * \brief Get a formatted string, decoded using the config formatter
     */
    string::FormattedString get_formatted_string(const std::string& path,
                                                 const std::string& default_value={})
    {
        return string::FormatterConfig().decode(get(path,default_value));
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

std::ostream& operator<< ( std::ostream& stream, const Settings& settings );

/**
 * \brief Key-value map used to store object properties
 */
using Properties = std::unordered_map<std::string,std::string>;

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
