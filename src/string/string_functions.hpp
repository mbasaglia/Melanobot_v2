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
#ifndef STRING_FUNCTIONS_HPP
#define STRING_FUNCTIONS_HPP

#include <algorithm>
#include <iterator>
#include <sstream>
#include <string>

namespace string {

/**
 * \brief Turn a container into a string
 * \pre Container::const_iterator is a ForwardIterator
 *      Container::value_type has the stream operator
 * \note Should work on arrays just fine
 */
template<class Container>
    std::string implode(const std::string& glue, const Container& elements)
    {
        auto iter = std::begin(elements);
        auto end = std::end(elements);
        if ( iter == end )
            return "";

        std::ostringstream ss;
        while ( true )
        {
            ss << *iter;
            ++iter;
            if ( iter != end )
                ss << glue;
            else
                break;
        }

        return ss.str();
    }

/**
 * \brief Whether a string starts with the given prefix
 */
inline bool starts_with(const std::string& haystack, const std::string& prefix)
{
    auto it1 = haystack.begin();
    auto it2 = prefix.begin();
    while ( it1 != haystack.end() && it2 != prefix.end() && *it1 == *it2 )
    {
        ++it1;
        ++it2;
    }
    return it2 == prefix.end();
}

} // namespace string
#endif // STRING_FUNCTIONS_HPP
