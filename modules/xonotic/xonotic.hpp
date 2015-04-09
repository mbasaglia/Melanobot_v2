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
#ifndef XONOTIC_HPP
#define XONOTIC_HPP
#include <regex>

namespace xonotic {

/**
 * \brief Escapes characters and puts the string in double quotes
 */
inline std::string quote_string(const std::string& text)
{
    static std::regex regex_xonquote(R"(([\\"]))",
        std::regex::ECMAScript|std::regex::optimize
    );
    return '"'+std::regex_replace(text,regex_xonquote,"\\$&")+'"';
}

/**
 * \brief Returns the human-readable gametyp name from a short name
 */
std::string gametype_name(const std::string& short_name);

} // namespace xonotic
#endif // XONOTIC_HPP
