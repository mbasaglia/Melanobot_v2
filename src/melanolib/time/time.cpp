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
#include "time.hpp"
#include "time_parser.hpp"

#include <sstream>

namespace melanolib {
namespace time {

DateTime parse_time(const std::string& text)
{
    std::istringstream ss(text);
    TimeParser parser(ss);
    return parser.parse_time_point();
}

DateTime::Duration parse_duration(const std::string& text)
{
    std::istringstream ss(text);
    TimeParser parser(ss);
    return parser.parse_duration();
}

} // namespace timer
} // namespace melanolib
