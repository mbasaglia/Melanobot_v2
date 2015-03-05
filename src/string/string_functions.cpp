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
#include "string_functions.hpp"

namespace string {

std::string elide ( std::string text, int length )
{
    if ( int(text.size()) <= length )
        return std::move(text);

    int p = length-3;
    if ( !std::isspace(text[p+1]) )
        for ( ; p >= 0 && !std::isspace(text[p]); p-- );
    for ( ; p >= 0 && std::isspace(text[p]); p-- );

    text.erase(p+1);
    text += "...";

    return std::move(text);
}

std::string add_slashes ( const std::string& input, const std::string& characters )
{
    std::string out;
    out.reserve(input.size());
    std::string::size_type it = 0;
    std::string::size_type prev = 0;
    while ( true )
    {
        it = input.find_first_of(characters,it);
        out.insert(out.size(),input,prev,it-prev);
        if ( it == std::string::npos )
            break;
        out.push_back('\\');
        prev = it;
        it++;
    }

    return std::move(out);
}

std::string str_replace(const std::string& input, const std::string& from, const std::string& to)
{
    std::string out;
    out.reserve(input.size());
    std::string::size_type it = 0;
    std::string::size_type prev = 0;
    while ( true )
    {
        it = input.find(from,it);
        out.insert(out.size(),input,prev,it-prev);
        if ( it == std::string::npos )
            break;
        out += to;
        it += from.size();
        prev = it;
    }
    out.shrink_to_fit();
    return std::move(out);
}

bool simple_wildcard(const std::string& text, const std::string& pattern)
{
    std::regex regex_pattern (
        "^"+str_replace(add_slashes(pattern,"^$\\.+?()[]{}|"),"*",".*")+"$" );
    return std::regex_match(text,regex_pattern);
}


std::vector<std::string> regex_split(const std::string& input,
                                     const std::regex& pattern,
                                     bool skip_empty )
{
    std::vector<std::string> out;
    std::sregex_token_iterator iter(input.begin(), input.end(), pattern, -1);
    while ( iter != std::sregex_token_iterator() )
    {
        std::string match = *iter;
        if ( !skip_empty || !match.empty() )
            out.push_back(match);
        ++iter;
    }
    return std::move(out);
}


} // namespace string
