/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2016 Mattia Basaglia
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
#ifndef MELANOBOT_MODULE_GITHUB_REPLACE_PTREE_HPP
#define MELANOBOT_MODULE_GITHUB_REPLACE_PTREE_HPP

#include "string/string.hpp"
#include "settings.hpp"

namespace github {


inline string::FormattedString&& replace(
    string::FormattedString&& str,
    const PropertyTree& tree)
{
    str.replace(
        [&tree](const std::string& id)
            -> melanolib::Optional<string::FormattedString>
        {
            auto get = tree.get_optional<std::string>(id);
            if ( get )
                return string::FormattedString(*get);
            return {};
        }
    );
    return std::move(str);
}


} // namespace github
#endif // MELANOBOT_MODULE_GITHUB_REPLACE_PTREE_HPP
