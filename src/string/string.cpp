/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015-2017 Mattia Basaglia
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

#include "string.hpp"
#include "replacements.hpp"


namespace string {

static FormattedString expand_tree_node(const boost::property_tree::ptree& node)
{
    if ( !node.data().empty() )
        return FormattedString(node.data());

    FormattedString result;
    if ( !node.empty() )
    {
        for ( const auto& item: node )
        {
            result << ListItem(expand_tree_node(item.second));
        }
    }
    return result;
}

void FormattedString::replace(const boost::property_tree::ptree& tree)
{
    replace(
        [&tree](const std::string& id)
            -> melanolib::Optional<FormattedString>
        {
            auto value = tree.get_child_optional(id);
            if ( value )
            {
                return expand_tree_node(*value);
            }
            return {};
        }
    );
}


} // namespace string
