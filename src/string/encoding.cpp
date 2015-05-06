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

#include "encoding.hpp"
#include "functional.hpp"

#ifdef HAS_ICONV
#       include <iconv.h>
#endif

namespace string {

char Utf8Encoding::to_ascii(uint32_t unicode)
{
    if ( unicode < 128 )
        return char(unicode);
    return to_ascii(encode(unicode));
}

char Utf8Encoding::to_ascii(const std::string& utf8)
{
#ifdef HAS_ICONV
    // With the C locale, //TRANSLIT won't work properly
    static auto only_once = setlocale (LC_ALL, ""); (void)only_once;

    char ascii = '?';
    char * cutf8 = (char*)utf8.data();
    size_t cutf8size = utf8.size();
    char * cascii = &ascii;
    size_t casciisize = 1;

    iconv_t iconvobj  = iconv_open("ASCII//TRANSLIT", "UTF-8");
    iconv(iconvobj, &cutf8, &cutf8size, &cascii, &casciisize);
    iconv_close(iconvobj);
    return ascii;
#else
    return '?';
#endif
}

FormattedString Utf8Encoding::parse(const std::string& string, const Formatter& formatter)
{
    std::string utf8;         // Multibyte string
    uint32_t    unicode;      // Multibyte value
    unsigned    length = 0;   // Multibyte length
    DecodeEnvironment env(string, formatter);
    // Handles an invalid/incomplete sequence
    auto check_valid = [&]()
    {
        if ( length != 0 )
        {
            // premature end of a multi-byte character
            decode_invalid(env,utf8);
            length = 0;
            utf8.clear();
            unicode = 0;
        }
    };

    while ( !env.input.eof() )
    {
        uint8_t byte = env.input.next();

        // 0... .... => ASCII
        if ( byte < 0b1000'0000 ) // '
        {
            check_valid();
            decode_ascii(env,byte);
        }
        // 11.. .... => Begin multibyte
        else if ( (byte & 0b1100'0000) == 0b1100'0000 )
        {
            check_valid();
            utf8.push_back(byte);

            // extract number of leading 1s
            while ( byte & 0b1000'0000 ) // '
            {
                length++;
                byte <<= 1;
            }

            // Restore byte (leading 1s have been eaten off)
            byte >>= length;
            unicode = byte;
        }
        // 10.. .... => multibyte tail
        else if ( length > 0 )
        {
            utf8.push_back(byte);
            unicode <<= 6;
            unicode |= byte&0b0011'1111; //'
            if ( utf8.size() == length )
            {
                decode_unicode(env,Unicode(unicode,utf8));
                unicode = 0;
                length = 0;
                utf8.clear();
            }
        }
    }
    check_valid();
    decode_end(env);
    return env.output;
}

std::string Utf8Encoding::encode(uint32_t value)
{
    if ( value < 128 )
        return std::string(1,char(value));

    std::basic_string<uint8_t> s;

    uint8_t head = 0;
    while ( value )
    {
        s.push_back((value&0b0011'1111)|0b1000'0000);
        value >>= 6;
        head <<= 1;
        head |= 1;
    }

    if ( (uint8_t(s.back())&0b0011'1111) > (1 << (7 - s.size())) ) // '
    {
        head <<= 1;
        head |= 1;
        s.push_back(0);
    }

    s.back() |= head << (8 - s.size());

    return std::string(s.rbegin(),s.rend());
}

FormattedString AsciiEncoding::parse(const std::string& string, const Formatter& formatter) const
{
    DecodeEnvironment env(string, formatter);

    while ( !env.input.eof() )
    {
        uint8_t byte = env.input.next();
        if ( byte < 128 )
            decode_ascii(env,byte);
        else
            decode_invalid(env,std::string(1,byte));

    }

    return env.output();
}
std::string encode(const Unicode& c) const
{
    Utf8Encoding::to_ascii(c.utf8());
}

} // namespace string
