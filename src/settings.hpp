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
#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <boost/property_tree/ptree.hpp>

namespace settings {

/**
 * \brief Class containing hierarchical settings
 */
typedef boost::property_tree::ptree Settings;

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
 * \brief Basically get the executable directory
 */
void initialize ( int argc, char** argv );

/**
 * \brief Load settings from file
 */
Settings load ( const std::string& file_name, FileFormat format = FileFormat::AUTO );

/**
 * \brief Tries to find a file from which settings can be loaded
 */
std::string find_config( FileFormat format = FileFormat::AUTO );

} // namespace settings
#endif // SETTINGS_HPP
