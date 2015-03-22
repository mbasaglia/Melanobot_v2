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
 *
 * \todo Try to check if boost/algorithm/string stuff can be used in here
 *       http://www.boost.org/doc/libs/1_55_0/doc/html/string_algo.html
 *       (Some of those are identical to functions defined here)
 */

#include "string.hpp"

#include <list>
#include <regex>

#include <boost/iterator/iterator_concepts.hpp>

#include "logger.hpp"
#include "string_functions.hpp"
#include "functional.hpp"
#include "config.hpp"

#ifdef HAS_ICONV
#       include <iconv.h>
#endif

namespace string {

char Utf8Parser::to_ascii(uint32_t unicode)
{
    if ( unicode < 128 )
        return char(unicode);
    return to_ascii(encode(unicode));
}

char Utf8Parser::to_ascii(const std::string& utf8)
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

void Utf8Parser::parse(const std::string& string)
{
    input.clear();
    input.str(string);
    while ( true )
    {
        uint8_t byte = input.get();

        if ( !input )
            break;

        // 0... .... => ASCII
        if ( byte < 0b1000'0000 )
        {
            check_valid();
            callback(callback_ascii,byte);
        }
        // 11.. .... => Begin multibyte
        else if ( (byte & 0b1100'0000) == 0b1100'0000 )
        {
            check_valid();
            utf8.push_back(byte);

            // extract number of leading 1s
            while ( byte & 0b1000'0000 )
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
                callback(callback_utf8,unicode,utf8);
                unicode = 0;
                length = 0;
                utf8.clear();
            }
        }
    }
    check_valid();
    callback(callback_end);
}

std::string Utf8Parser::encode(uint32_t value)
{
    if ( value < 128 )
        return std::string(1,char(value));

    std::list<uint8_t> s;

    uint8_t head = 0;
    while ( value )
    {
        s.push_back((value&0b0011'1111)|0b1000'0000);
        value >>= 6;
        head <<= 1;
        head |= 1;
    }

    if ( (uint8_t(s.back())&0b0011'1111) > (1 << (7 - s.size())) )
    {
        head <<= 1;
        head |= 1;
        s.push_back(0);
    }

    s.back() |= head << (8 - s.size());

    return std::string(s.rbegin(),s.rend());
}

void Utf8Parser::check_valid()
{
    if ( length != 0 )
    {
        // premature end of a multi-byte character
        callback(callback_invalid,utf8);
        length = 0;
        utf8.clear();
        unicode = 0;
    }
}

std::vector<std::string> QFont::qfont_table = {
    "",   " ",  "-",  " ",  "_",  "#",  "+",  ".",  "F",  "T",  " ",  "#",  ".",  "<",  "#",  "#", // 0
    "[",  "]",  ":)", ":)", ":(", ":P", ":/", ":D", "<",  ">",  ".",  "-",  "#",  "-",  "-",  "-", // 1
    "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?", // 2
    "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?", // 3
    "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?", // 4
    "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?", // 5
    "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?", // 6
    "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?", // 7
    "=",  "=",  "=",  "#",  "!",  "[o]","[u]","[i]","[c]","[c]","[r]","#",  "?",  ">",  "#",  "#", // 8
    "[",  "]",  ":)", ":)", ":(", ":P", ":/", ":D", "<",  ">",  "#",  "X",  "#",  "-",  "-",  "-", // 9
    " ",  "!",  "\"", "#",  "$",  "%",  "&",  "\"", "(",  ")",  "*",  "+",  ",",  "-",  ".",  "/", // 10
    "0",  "1",  "2",  "3",  "4",  "5",  "6",  "7", "8",  "9",  ":",  ";",  "<",  "=",  ">",  "?",  // 11
    "@",  "A",  "B",  "C",  "D",  "E",  "F",  "G", "H",  "I",  "J",  "K",  "L",  "M",  "N",  "O",  // 12
    "P",  "Q",  "R",  "S",  "T",  "U",  "V",  "W", "X",  "Y",  "Z",  "[",  "\\", "]",  "^",  "_",  // 13
    ".",  "A",  "B",  "C",  "D",  "E",  "F",  "G", "H",  "I",  "J",  "K",  "L",  "M",  "N",  "O",  // 14
    "P",  "Q",  "R",  "S",  "T",  "U",  "V",  "W", "X",  "Y",  "Z",  "{",  "|",  "}",  "~",  "<"   // 15
};


std::string FormattedString::encode(Formatter* formatter) const
{
    if ( !formatter )
        CRITICAL_ERROR("Trying to encode a string without formatter");
    std::string s;
    for ( const auto& e : elements )
        s += e->to_string(*formatter);
    return s;
}

Formatter::Registry::Registry()
{
    add_formatter(default_formatter = new FormatterUtf8);
    add_formatter(new FormatterAscii);
    add_formatter(new FormatterAnsi(true));
    add_formatter(new FormatterAnsi(false));
}

Formatter* Formatter::Registry::formatter(const std::string& name)
{
    auto it = formatters.find(name);
    if ( it == formatters.end() )
    {
        ErrorLog("sys") << "Invalid formatter: " << name;
        if ( default_formatter )
            return default_formatter;
        CRITICAL_ERROR("Trying to access an invalid formatter");
    }
    return it->second;
}

void Formatter::Registry::add_formatter(Formatter* instance)
{
    if ( formatters.count(instance->name()) )
        ErrorLog("sys") << "Overwriting formatter: " << instance->name();
    formatters[instance->name()] = instance;
    if ( ! default_formatter )
        default_formatter = instance;
}

std::string FormatterUtf8::ascii(char c) const
{
    return std::string(1,c);
}
std::string FormatterUtf8::ascii(const std::string& s) const
{
    return s;
}
std::string FormatterUtf8::color(const color::Color12& color) const
{
    return "";
}
std::string FormatterUtf8::format_flags(FormatFlags flags) const
{
    return "";
}
std::string FormatterUtf8::clear() const
{
    return "";
}
std::string FormatterUtf8::unicode(const Unicode& c) const
{
    return c.utf8();
}
std::string FormatterUtf8::qfont(const QFont& c) const
{
    return c.alternative();
}
FormattedString FormatterUtf8::decode(const std::string& source) const
{
    FormattedString str;

    Utf8Parser parser;

    std::string ascii;

    auto push_ascii = [&ascii,&str]()
    {
        if ( !ascii.empty() )
        {
            str.append<AsciiSubstring>(ascii);
            ascii.clear();
        }
    };

    parser.callback_ascii = [&str,&ascii](uint8_t byte)
    {
        ascii += byte;
    };
    parser.callback_utf8 = [&str,push_ascii](uint32_t unicode,const std::string& utf8)
    {
        push_ascii();
        str.append<Unicode>(utf8,unicode);
    };
    parser.callback_end = push_ascii;

    parser.parse(source);

    return str;
}
std::string FormatterUtf8::name() const
{
    return "utf8";
}


std::string FormatterAscii::unicode(const Unicode& c) const
{
    return std::string(1,Utf8Parser::to_ascii(c.utf8()));
}
FormattedString FormatterAscii::decode(const std::string& source) const
{
    FormattedString str;
    str.append<AsciiSubstring>(source);
    return str;
}

std::string FormatterAscii::name() const
{
    return "ascii";
}

std::string FormatterAnsi::color(const color::Color12& color) const
{
    if ( !color.is_valid() )
        return "\x1b[0m";

    auto c4b = color.to_4bit();
    bool bright = c4b & 8;
    int number = c4b & ~8;

    /// \todo add a flag to select whether to use the non-conformant 9x or ;1
    return std::string("\x1b[")+(bright ? "9" : "3")+std::to_string(number)+"m";
    //return "\x1b[3"+std::to_string(number)+";"+(bright ? "1" : "22")+"m";
}
std::string FormatterAnsi::format_flags(FormatFlags flags) const
{
    std::vector<int> codes;
    codes.push_back( flags & FormatFlags::BOLD ? 1 : 22 );
    codes.push_back( flags & FormatFlags::UNDERLINE ? 4 : 24 );
    return  "\x1b["+implode(";",codes)+"m";
}
std::string FormatterAnsi::clear() const
{
    return "\x1b[0m";
}
std::string FormatterAnsi::unicode(const Unicode& c) const
{
    if ( utf8 )
        return c.utf8();
    return std::string(1,Utf8Parser::to_ascii(c.utf8()));
}
FormattedString FormatterAnsi::decode(const std::string& source) const
{
    FormattedString str;

    Utf8Parser parser;

    std::string ascii;

    auto push_ascii = [&ascii,&str]()
    {
        if ( !ascii.empty() )
        {
            str.append<AsciiSubstring>(ascii);
            ascii.clear();
        }
    };

    parser.callback_ascii = [&parser,&str,&ascii](uint8_t byte)
    {
        if ( byte == '\x1b' && parser.input.peek() == '[' )
        {
            parser.input.ignore();
            std::vector<int> codes;
            while (parser.input)
            {
                int i = 0;
                if ( parser.input >> i )
                {
                    codes.push_back(i);
                    char next = parser.input.get();
                    if ( next != ';' )
                    {
                        if ( next != 'm' )
                            parser.input.unget();
                        break;
                    }
                }
            }
            FormatFlags flags;
            bool use_flags = false;
            for ( int i : codes )
            {
                if ( i == 0)
                {
                    str.append<ClearFormatting>();
                }
                else if ( i == 1 )
                {
                    flags |= FormatFlags::BOLD;
                    use_flags = true;
                }
                else if ( i == 22 )
                {
                    flags &= ~FormatFlags::BOLD;
                    use_flags = true;
                }
                else if ( i == 4 )
                {
                    flags |= FormatFlags::UNDERLINE;
                    use_flags = true;
                }
                else if ( i == 24 )
                {
                    flags &= ~FormatFlags::UNDERLINE;
                    use_flags = true;
                }
                else if ( i == 39 )
                {
                    str.append<Color>(color::nocolor);
                }
                else if ( i >= 30 && i <= 37 )
                {
                    i -= 30;
                    if ( flags == FormatFlags::BOLD )
                    {
                        i &= 0xf;
                        flags = FormatFlags::NO_FORMAT;
                        use_flags = false;
                    }
                    str.append<Color>(color::Color12::from_4bit(i));
                }
                else if ( i >= 90 && i <= 97 )
                {
                    i -= 90;
                    i &= 0xf;
                    str.append<Color>(color::Color12::from_4bit(i));
                }
            }
            if ( use_flags )
                str.append<Format>(flags);
        }
        else
        {
            ascii += byte;
        }
    };
    parser.callback_utf8 = [this,&str,push_ascii](uint32_t unicode,const std::string& utf8)
    {
        push_ascii();
        if ( this->utf8 )
            str.append<Unicode>(utf8,unicode);
    };
    parser.callback_end = push_ascii;

    parser.parse(source);

    return str;
}
std::string FormatterAnsi::name() const
{
    return utf8 ? "ansi-utf8" : "ansi-ascii";
}

} // namespace string
