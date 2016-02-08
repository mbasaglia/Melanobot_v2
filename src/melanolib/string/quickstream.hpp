/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015-2016 Mattia Basaglia
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
#ifndef QUICKSTREAM_HPP
#define QUICKSTREAM_HPP

#include <string>
#include <regex>

namespace melanolib {
namespace string {

/**
 * \brief Quick and simple unformatted string input stream
 * \invariant
 *      * \c pos Points to the next character returned by next()
 *      * if eof() returns true, next() returns Traits::eof()
 * \note Part of the interfacte is different from std::istream as it behaves
 *       differently from it
 */
template <class CharT, class Traits=std::char_traits<CharT>>
    class BasicQuickStream
{
public:
    using char_type     = CharT;
    using string_type   = std::basic_string<CharT,Traits>;
    using pos_type      = typename string_type::size_type;
    using size_type     = typename string_type::size_type;
    using int_type      = typename Traits::int_type;
    using regex_type    = std::basic_regex<CharT>;
    using match_type    = std::match_results<typename string_type::const_iterator>;

    BasicQuickStream() {}
    BasicQuickStream(std::string input) : source(std::move(input)) {}

    /**
     * \brief Returns the contained string
     */
    const string_type& str() const noexcept
    {
        return source;
    }

    /**
     * \brief Change the source string
     */
    void str(const string_type& string)
    {
        source = string;
        pos = 0;
    }

    /**
     * \brief Whether it reached the end of the string
     */
    bool eof() const noexcept
    {
        return pos >= source.size();
    }

    /**
     * \brief Whether it reached past the end of the string
     */
    explicit operator bool() const noexcept
    {
        return pos <= source.size();
    }

    /**
     * \brief Clears errors
     */
    void clear() noexcept
    {
        if ( pos > source.size() )
            pos = source.size();
    }

    /**
     * \brief Extracts and returns the next character
     * \return A valid character or Traits::eof()
     */
    int_type next() noexcept
    {
        int_type c = pos >= source.size() ? Traits::eof() : source[pos];
        pos++;
        return c;
    }

    /**
     * \brief Undoes get() or ignore()
     */
    void unget() noexcept
    {
        if ( pos > 0 )
            pos--;
    }

    /**
     * \brief Returns the current read position
     */
    pos_type tell_pos() const noexcept
    {
        return pos;
    }


    /**
     * \brief Changes the read position
     */
    void set_pos(pos_type p) noexcept
    {
        pos = p;
    }

    /**
     * \brief Returns the next character without extracting it
     * \return A valid character or Traits::eof()
     */
    int_type peek() const noexcept
    {
        return pos < source.size() ? source[pos] : Traits::eof();
    }

    /**
     * \brief Extracts the next character
     */
    void ignore() noexcept
    {
        pos++;
    }

    /**
     * \brief Extracts \c n characters
     */
    void ignore(size_type n) noexcept
    {
        pos += n;
    }

    /**
     * \brief Extracts until \c delim has been foun or at most \c n characters
     */
    void ignore(size_type n, char_type delim) noexcept
    {
        size_type i = 0;
        for ( ; i < n && !eof() && source[pos] != delim; i++ )
            pos++;
        if ( !eof() && i != n )
            pos++;
    }

    /**
     * \brief Get a string, until \c delim
     *
     * \c delim is extracted but not inserted in the returned string
     */
    string_type get_line(char_type delim = '\n')
    {
        return get_until([delim](char_type c){ return c == delim; }, true);
    }

    /**
     * \brief Get a string, untile \p predicate is true (or eof)
     * \tparam Predicate    A predicate taking chars
     * \param predicate     Termination condition
     * \param skip_match    If \c true, it will skip the first character for which \p predicate is \c true
     */
    template<class Predicate>
        string_type get_until(const Predicate& predicate, bool skip_match = true)
    {
        auto begin = pos;
        while ( !eof() && !predicate(source[pos]) )
            pos++;
        auto end = pos-begin;
        if ( !eof() && skip_match )
            pos++;
        return source.substr(begin,end);
    }

    /**
     * \brief Reads a simple int (without sign) expressed in base 10
     * \return \b true on success
     */
    bool get_int(int& out) noexcept
    {
        if ( eof() || source[pos] < '0' || source[pos] > '9' )
            return false;
        out = get_int();
        return true;
    }

    int get_int() noexcept
    {
       int ret = 0;
       while ( !eof() && source[pos] >= '0' && source[pos] <= '9' )
       {
           ret = ret * 10 + source[pos]-'0';
           pos++;
       }
       return ret;
    }

    /**
     * \brief Extract a string matching the given regex, starting at the current position
     */
    std::string get_regex(const regex_type& regex,
                          std::regex_constants::match_flag_type match_flags =
                            std::regex_constants::match_default)
    {
        if ( eof() )
            return {};

        match_type match;
        if ( regex_match(regex,match,match_flags) )
        {
            pos += match.length();
            return match[0];
        }

        return {};
    }

    /**
     * \brief Extract a regex result matching the given regex, starting at the current position
     * \return \b true if the regex has been matched
     */
    bool get_regex(const regex_type& regex, match_type& match,
                   std::regex_constants::match_flag_type match_flags =
                        std::regex_constants::match_default)
    {
        if ( regex_match(regex,match,match_flags) )
        {
            pos += match.length();
            return true;
        }
        return false;
    }


    /**
     * \brief Returns whether the source matches the given regex,
     * starting from the current position
     * \note It doesn't move forward the stream position
     */
    bool regex_match(const regex_type& regex, match_type& match,
                     std::regex_constants::match_flag_type match_flags =
                        std::regex_constants::match_default) const
    {
        // clear match?
        if ( eof() )
            return false;
        match_flags |= std::regex_constants::match_continuous;
        return std::regex_search(source.cbegin()+pos, source.cend(), match,
                                regex, match_flags);
    }

    bool regex_match(const regex_type& regex,
                     std::regex_constants::match_flag_type match_flags =
                        std::regex_constants::match_default) const
    {
        match_type m;
        return regex_match(regex, m, match_flags);
    }

private:
    string_type source; ///< Source string
    pos_type    pos = 0;///< Position in the source string
};

using QuickStream = BasicQuickStream<char>;

} // namespace string
} // namespace melanolib
#endif // QUICKSTREAM_HPP
