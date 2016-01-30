/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright  Mattia Basaglia
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
#ifndef ERROR_HPP
#define ERROR_HPP

#include <stdexcept>

namespace melanobot {

/**
 * \brief Generic Melanobot-related errors
 */
class MelanobotError : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

/**
 * \brief Class representing an error occurring during configuration
 */
class ConfigurationError : public MelanobotError
{
public:
    ConfigurationError(const std::string& msg = "Invalid configuration parameters")
        : MelanobotError(msg)
    {}
};

} // namespace melanobot
#endif // ERROR_HPP
