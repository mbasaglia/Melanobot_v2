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
#ifndef QUICKSTREAM_HPP
#define QUICKSTREAM_HPP

#include <string>
#include <regex>

namespace string {

/**
 * \brief Quick and simple unformatted string input stream
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
    using regex_type    = std::basic_regex<CharT,Traits>;
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
     * \brief Whether it reached the end of the string
     */
    explicit operator bool() const noexcept
    {
        return eof();
    }

    /**
     * \brief Extracts and returns the next character
     * \return A valid character or Traits::eof()
     */
    int_type get() noexcept
    {
        pos++;
        return pos >= source.size() ? Traits::eof() : source[pos];
    }

    /**
     * \brief Undoes get() or ignore()
     */
    void unget() noexcept
    {
        pos--;
    }

    /**
     * \brief Returns the current read position
     */
    pos_type tellg() const noexcept
    {
        return pos;
    }


    /**
     * \brief Changes the read position
     */
    void seekg(pos_type p) const noexcept
    {
        pos = p;
    }

    /**
     * \brief Returns the next character without extracting it
     * \return A valid character or Traits::eof()
     */
    int_type peek() const noexcept
    {
        return pos+1 < source.size() ? source[pos+1] : Traits::eof();
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
        for ( size_type i = 0; i < n && !eof() && source[pos] != delim; i++ )
            pos++;
    }

    /**
     * \brief Get a string, until \c delim
     *
     * \c delim is extracted but not inserted in the returned string
     */
    string_type getline(char_type delim = '\n')
    {
        pos++;
        auto begin = pos;
        while ( !eof() && source[pos] != delim() )
            pos++;
        return source.substr(begin,pos-begin());
    }

    /**
     * \brief Reads a simple int (without sign) expressed in base 10
     */
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
     * \brief Returns whether the source matches the given regex,
     * starting from the current position
     * \note It doesn't move forward the stream position
     */
    bool regex_match(const regex_type& regex, match_type& match,
                     std::regex_constants::match_flag_type match_flags =
                        std::regex_constants::match_default)
    {
        // clear match?
        if ( eof() )
            return false;
        match_flags |= std::regex_constants::match_continuous;
        return std::regex_search(source.cbegin()+pos(), source.end(), match,
                                regex, match_flags);
    }

private:
    string_type source; ///< Source string
    pos_type pos;       ///< Position in the source string
};

using QuickStream = BasicQuickStream<char>;

} // namespace string
#endif // QUICKSTREAM_HPP
