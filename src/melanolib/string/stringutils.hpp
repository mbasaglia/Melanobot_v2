/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015-2016 Mattia Basaglia
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
#ifndef MELANOLIB_STRING_UTILS_HPP
#define MELANOLIB_STRING_UTILS_HPP

#include <algorithm>
#include <iterator>
#include <regex>
#include <sstream>
#include <string>
#include <type_traits>
#include <unordered_map>

namespace melanolib {
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

/**
 * \brief Whether a string ends with the given suffix
 */
inline bool ends_with(const std::string& haystack, const std::string& suffix)
{
    auto it1 = haystack.rbegin();
    auto it2 = suffix.rbegin();
    while ( it1 != haystack.rend() && it2 != suffix.rend() && *it1 == *it2 )
    {
        ++it1;
        ++it2;
    }
    return it2 == suffix.rend();
}

/**
 * \brief String to lower case
 */
inline std::string strtolower ( std::string string )
{
    std::transform(string.begin(),string.end(),string.begin(), (int(*)(int))std::tolower);
    return string;
}

/**
 * \brief String to upper case
 */
inline std::string strtoupper ( std::string string )
{
    std::transform(string.begin(),string.end(),string.begin(), (int(*)(int))std::toupper);
    return string;
}

/**
 * \brief If the string is longer than \c length,
 * truncates to the last word and adds an ellipsis
 */
std::string elide ( std::string text, int length );

/**
 * \brief Collapse all sequences of spaces to a single space character ' '
 */
inline std::string collapse_spaces ( const std::string& text )
{
    static std::regex regex_spaces("\\s+");
    return std::regex_replace(text,regex_spaces," ");
}

/**
 * \brief Escape all occurrences of \c characters with slashes
 */
std::string add_slashes ( const std::string& input, const std::string& characters );
/**
 * \brief Escapes \c input to be inserted in a regex
 */
inline std::string regex_escape( const std::string& input )
{
    return add_slashes(input,"^$\\.*+?()[]{}|");
}

/**
 * \brief Replace all occurrences of \c from in \c text to \c to
 */
std::string replace(const std::string& input, const std::string& from, const std::string& to);

/**
 * \brief Replaces the keys of \c map to the respective values in \c subject
 * \param subject The string to be searched in
 * \param map     Term/replacement map
 * \param prefix  (Optional) prefix to prepend to all terms
 */
std::string replace(const std::string& subject,
                    const std::unordered_map<std::string,std::string>& map,
                    const std::string& prefix = {});

/**
 * \brief Checks if \c text matches the wildcard \c pattern
 *
 * \c * matches any sequence of characters, all other characters match themselves
 */
bool simple_wildcard(const std::string& text, const std::string& pattern);

/**
 * \brief Checks if any of the elements in \c input matches the wildcard \c pattern
 *
 * \c * matches any sequence of characters, all other characters match themselves
 * \tparam Container A container of \c std::string
 */
template <class Container, class = std::enable_if_t<!std::is_convertible<Container,std::string>::value>>
    bool simple_wildcard(const Container& input, const std::string& pattern)
    {
        static_assert(std::is_convertible<typename Container::value_type,std::string>::value,
            "simple_wildcard requires a string container"
        );
        return std::any_of(input.begin(),input.end(),
            [pattern](const std::string& t) { return simple_wildcard(t,pattern); });
    }


/**
 * \brief Separate the string into components separated by \c pattern
 */
std::vector<std::string> regex_split(const std::string& input,
                                     const std::regex& pattern,
                                     bool skip_empty = true );

inline std::vector<std::string> regex_split(const std::string& input,
                                            const std::string& pattern,
                                            bool skip_empty = true )
{
    return regex_split ( input, std::regex(pattern), skip_empty );
}

/**
 * \brief Separate the string into components separated by \c separator
 */
std::vector<std::string> char_split(const std::string& input,
                                    char separator,
                                    bool skip_empty = true);


/**
 * \brief Split a string of element separated by commas and spaces
 */
inline std::vector<std::string> comma_split(const std::string& input,bool skip_empty = true)
{
    static std::regex regex_commaspace ( "(,\\s*)|(\\s+)",
        std::regex_constants::optimize |
        std::regex_constants::ECMAScript );
    return string::regex_split(input,regex_commaspace,skip_empty);
}

/**
 * \brief Returns a value representing how similar the two strings are
 */
std::string::size_type similarity(const std::string& s1, const std::string& s2);

/**
 * \brief Converts \c string to an unsigned integer
 * \returns The corresponding integer or \c default_value on failure
 */
inline unsigned long to_uint(const std::string& string,
                      unsigned long base = 10,
                      unsigned long default_value = 0) noexcept
try {
    return std::stoul(string,0,base);
} catch(const std::exception&) {
    return default_value;
}

/**
 * \brief Converts \c string to an integer
 * \returns The corresponding integer or \c default_value on failure
 */
inline long to_int(const std::string& string,
            unsigned long base = 10,
            long default_value = 0) noexcept
try {
    return std::stol(string,0,base);
} catch(const std::exception&) {
    return default_value;
}


/**
 * \brief Check if a string is one of a given set
 */
inline bool is_one_of(const std::string& string, const std::initializer_list<std::string>& il )
{
    return std::find(il.begin(),il.end(),string) != il.end();
}

/**
 * \brief Case-insensitive string comparison
 */
inline bool icase_equal(const std::string& a, const std::string& b) noexcept
{
    return std::equal(a.begin(), a.end(), b.begin(), b.end(),
                      [](char l, char r) { return std::tolower(l) == std::tolower(r); });
}

/**
 * \brief Converts a number to string padding with zeros to have at least
 *      \c digits digits
 */
template<class T>
    std::string to_string(T number,int digits=-1)
    {
        auto s = std::to_string(number);
        if ( int(s.size()) < digits )
            s = std::string(digits-s.size(),'0')+s;
        return s;
    }

} // namespace string
} // namespace melanolib
#endif // MELANOLIB_STRING_UTILS_HPP
