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
#ifndef IRC_FUNCTIONS_HPP
#define IRC_FUNCTIONS_HPP

#include <algorithm>
#include <cctype>
#include <string>

namespace irc {

/**
 * \brief Whether a character is a valid nickname character
 * \see http://tools.ietf.org/html/rfc2812#section-2.3.1
 */
inline bool is_nickchar(char c)
{
    return std::isalnum(c) || c == '-' ||
        ( c >= 0x5B && c <= 0x60 ) || ( c >= 0x7B && c <= 0x7D );
}

/**
 * \brief Converts a string to lower case
 * \see http://tools.ietf.org/html/rfc2812#section-2.2
 * \todo locale stuff if it's worth the effort
 */
inline std::string strtolower ( std::string string )
{
    std::transform(string.begin(),string.end(),string.begin(),[](char c)->char {
        switch (c) {
            case '[': return '{';
            case ']': return '}';
            case '\\': return '|';
            case '~': return '^';
            default:  return std::tolower(c);
        }
    });
    return string;
}
/**
 * \brief Converts a string to upper case
 * \see http://tools.ietf.org/html/rfc2812#section-2.2
 */
inline std::string strtoupper ( std::string string )
{
    std::transform(string.begin(),string.end(),string.begin(),[](char c)->char {
        switch (c) {
            case '{': return '[';
            case '}': return ']';
            case '|': return '\\';
            case '^': return '~';
            default:  return std::toupper(c);
        }
    });
    return string;
}


} // namespace irc

#endif // IRC_FUNCTIONS_HPP
