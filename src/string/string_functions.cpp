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

std::string replace(const std::string& input, const std::string& from, const std::string& to)
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
        "^"+replace(add_slashes(pattern,"^$\\.+?()[]{}|"),"*",".*")+"$" );
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

std::string::size_type similarity(const std::string& s1, const std::string& s2)
{
    /**
     * \note  This can be done more accurately but it doesn't matter for the
     * purpose of where this function is called so a simple yet inaccurate
     * algorithm is acceptable.
     *
     */

    typedef std::string::size_type int_;
    int_ result = 0;

    int_ i1 = 0; // Current position on s1
    int_ i2 = 0; // Current position on s2

    while ( i1 < s1.size() && i2 < s2.size() )
    {
        // find the where the first character of a string is found in the other
        int_ next1 = s1.find(s2[i2],i1);
        int_ next2 = s2.find(s1[i1],i2);

        // found: the character of s2 isn't too far in s1
        if ( next1 < next2 )
        {
            result += i1 == next1 ? 3 : 1;
            i1 = next1+1; // moving forward in s1
            i2++; // using that character in s2
        }
        // same as above, other way round
        else if ( next1 > next2 )
        {
            result += i2 == next2 ? 3 : 1;
            i1++; // using that character in s1
            i2 = next2+1; // moving forward in s2
        }
        // here next1 == next2
        // not found
        else if ( next1 == std::string::npos )
        {
            // skip this character
            i1++;
            i2++;
        }
        // found on the same place
        else
        {
            result += i1 == next1 ? 3 : 1;
            i1 = next1+1; // moving forward in s1
            i2 = next2+1; // moving forward in s2
        }
    }

    return result;
}


unsigned long to_uint(const std::string& string,
                     unsigned long base,
                     unsigned long default_value)
try {
    return std::stoul(string,0,base);
} catch(const std::exception&) {
    return default_value;
}


std::string replace(const std::string& subject, const Properties& map, char prefix)
{
    /// \todo could use a better algorithm, maybe build a trie from \c map
    std::string output;

    std::string::size_type start = 0;
    std::string::size_type pos = 0;
    for ( ; pos < subject.size(); pos++ )
    {
        if ( subject[pos] == prefix )
        {
            output += subject.substr(start,pos-start);
            start = pos;
            for ( pos++; pos < subject.size() && subject[pos] != prefix; pos++ )
            {
                auto it = map.find(subject.substr(start+1,pos-start));
                if ( it != map.end() )
                {
                    output += it->second;
                    start = pos+1;
                    break;
                }
            }
        }
    }
    if ( start < pos )
        output += subject.substr(start,pos-start);

    return std::move(output);
}

} // namespace string
