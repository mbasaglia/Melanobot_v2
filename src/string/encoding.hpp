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
#ifndef STRING_ENCODING_HPP
#define STRING_ENCODING_HPP

#include "quickstream.hpp"
#include "functional.hpp"
#include "string.hpp"

namespace string {

struct DecodeEnvironment
{
    QuickStream input;
    const Formatter& formatter;
    FormattedString output;
    std::string ascii_substring;

    DecodeEnvironment(const std::string& input, const Formatter& formatter)
        : input(input), formatter(formatter) {}

    /**
     * \brief Pushes \c ascii_substring into \c output
     */
    void push_ascii()
    {
        if ( !ascii_substring.empty() )
        {
            output.append<AsciiSubstring>(ascii_substring);
            ascii_substring.clear();
        }
    }
};

/**
 * \brief Character encoding
 */
class CharEncoding
{
public:
    virtual FormattedString parse(const std::string& input, const Formatter& formatter) const = 0;

    virtual std::string encode(const Unicode& c) const = 0;

protected:
    /**
     * \brief Decodes an ASCII byte from a stream
     */
    void decode_ascii(DecodeEnvironment& env, uint8_t byte) const
    {
        env.formatter.decode_ascii(env,byte);
    }
    /**
     * \brief Decodes a Unicode pont from a stream
     */
    void decode_unicode(DecodeEnvironment& env, const Unicode& unicode) const
    {
        env.formatter.decode_unicode(env,unicode);
    }
    /**
     * \brief Decodes an invalid sequence from a stream
     */
    void decode_invalid(DecodeEnvironment& env, const std::string& parsed) const
    {
        env.formatter.decode_invalid(env,parsed);
    }
    /**
     * \brief Decodes the end of a stream
     */
    void decode_end(DecodeEnvironment& env) const
    {
        env.formatter.decode_end(env);
    }
};

/**
 * \brief Class used to parse and convert UTF-8
 */
class Utf8Encoding : public CharEncoding
{
public:
    using Byte = uint8_t;

    /**
     * \brief Parse the string
     */
    FormattedString parse(const std::string& string, const Formatter& fmt) const override;

    std::string encode(const Unicode& c) const override
    {
        return c.utf8();
    }

    /**
     * \brief Encode a unicode value to UTF-8
     */
    static std::string encode(uint32_t value);

    /**
     * \brief Whether a byte is a valid ascii character
     */
    static bool is_ascii(Byte b) { return b < 128; }

    /**
     * \brief Transliterate a single character to ascii
     */
    static char to_ascii(uint32_t unicode);
    /**
     * \brief Transliterate a single character to ascii
     */
    static char to_ascii(const std::string& utf8_char);
};


class AsciiEncoding : public CharEncoding
{
public:
    FormattedString parse(const std::string& string, const Formatter& formatter) const override;
    std::string encode(const Unicode& c) const override;
};

} // namespace string
#endif // STRING_ENCODING_HPP
