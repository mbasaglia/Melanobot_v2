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

#include "string.hpp"

#include <list>

#include <boost/iterator/iterator_concepts.hpp>

namespace string {

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
            call_back(callback_ascii,byte);
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
                call_back(callback_utf8,unicode,utf8);
                unicode = 0;
                length = 0;
                utf8.clear();
            }
        }
    }
    check_valid();
    call_back(callback_end);
}

std::string Utf8Parser::encode(uint32_t value)
{
    if ( value < 128 )
        return std::string(1,char(value));

    std::list<uint8_t> s;

    uint8_t head;
    while ( value )
    {
        s.push_back((value&0b0011'1111)|0b1000'0000);
        value >>= 6;
        head <<= 1;
        head |= 1;
    }

    if ( uint8_t(s.back()) > (1 << (7 - s.size())) )
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
        call_back(callback_invalid,utf8);
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


#define REGISTER_FORMATTER(classname,name) \
    static Formatter::RegisterFormatter register_##name = new classname

static Formatter::RegisterDefaultFormatter register_default(new FormatterUtf8(false));
REGISTER_FORMATTER(FormatterUtf8,ansi_utf8)(true);

std::string FormatterUtf8::ascii(char c) const
{
    return std::string(1,c);
}
std::string FormatterUtf8::color(const color::Color12& color) const
{
    return colors ? color.to_ansi() : "";
}
std::string FormatterUtf8::format_flags(FormatFlags flags) const
{
    if ( !colors )
        return "";

    std::vector<int> codes;
    codes.push_back( flags & FormatFlags::BOLD ? 1 : 22 );
    codes.push_back( flags & FormatFlags::UNDERLINE ? 4 : 24 );
    return  "\x1b["+implode(";",codes)+"m";
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

    /// \todo ansi colors
    parser.callback_ascii = [&str](uint8_t byte)
    {
        str.append(new Character(byte));
    };
    parser.callback_utf8 = [&str](uint32_t unicode,const std::string& utf8)
    {
        str.append(new Unicode(utf8,unicode));
    };

    parser.parse(source);

    return str;
}
std::string FormatterUtf8::name() const
{
    return colors ? "ansi-utf8" : "utf8";
}


REGISTER_FORMATTER(FormatterAscii,ascii)(false);
REGISTER_FORMATTER(FormatterAscii,ansi_ascii)(true);

std::string FormatterAscii::unicode(const Unicode& c) const
{
    /// \todo Transliterate (eg: è -> e)
    return "?";
}
FormattedString FormatterAscii::decode(const std::string& source) const
{
    /// \todo ansi colors
    FormattedString str;
    for ( char c : source )
        if ( c < 0b1000'0000 ) // '
            str.append(new Character(c));
    return str;
}

std::string FormatterAscii::name() const
{
    return colors ? "ansi-ascii" : "ascii";
}

REGISTER_FORMATTER(FormatterIrc,irc);

std::string FormatterIrc::color(const color::Color12& color) const
{
    return color.to_irc();
}
std::string FormatterIrc::format_flags(FormatFlags flags) const
{
    if ( flags == FormatFlags::NO_FORMAT )
        return "\xf"; /// \note clears color as well
    std::string format;
    if ( flags & FormatFlags::BOLD )
        format += "\x2";
    if ( flags & FormatFlags::UNDERLINE )
        format += "\x1f";
    return format;
}
FormattedString FormatterIrc::decode(const std::string& source) const
{
    FormattedString str;

    Utf8Parser parser;

    FormatFlags flags;
    parser.callback_ascii = [&str,&flags,&parser](uint8_t byte)
    {
        // see https://github.com/myano/jenni/wiki/IRC-String-Formatting
        switch ( byte )
        {
            case '\2':
                if ( !(flags & FormatFlags::BOLD) )
                {
                    flags |= FormatFlags::BOLD;
                    str.append(new Format(flags));
                }
                break;
            case '\x1f':
                if ( !(flags & FormatFlags::UNDERLINE) )
                {
                    flags |= FormatFlags::UNDERLINE;
                    str.append(new Format(flags));
                }
                break;
            case '\xf':
                flags = FormatFlags::NO_FORMAT;
                str.append(new Format(flags));
                str.append(new Color(color::nocolor));
                break;
            case '\3':
            {
                std::string fg;
                if ( std::isdigit(parser.input.peek()) )
                {
                    fg += parser.input.get();
                    if ( std::isdigit(parser.input.peek()) )
                        fg += parser.input.get();

                    if ( parser.input.peek() == ',' )
                    {
                        parser.input.ignore();
                        if ( std::isdigit(parser.input.peek()) ) parser.input.ignore();
                        if ( std::isdigit(parser.input.peek()) ) parser.input.ignore();
                    }
                }
                str.append(new Color(color::Color12::from_irc(fg)));
                break;
            }
            case '\x1d': case '\x16':
                // Skip unsupported format flags
                break;
            default:
                str.append(new Character(byte));
                break;
        }
    };
    parser.callback_utf8 = [&str](uint32_t unicode,const std::string& utf8)
    {
        str.append(new Unicode(utf8,unicode));
    };

    parser.parse(source);

    return str;
}
std::string FormatterIrc::name() const
{
    return "irc";
}

REGISTER_FORMATTER(FormatterDarkplaces,dp);
std::string FormatterDarkplaces::ascii(char c) const
{
    return std::string(1,c);
}
std::string FormatterDarkplaces::color(const color::Color12& color) const
{
    return color.to_dp();
}
std::string FormatterDarkplaces::format_flags(FormatFlags) const
{
    return "";
}
std::string FormatterDarkplaces::unicode(const Unicode& c) const
{
    return c.utf8();
}
std::string FormatterDarkplaces::qfont(const QFont& c) const
{
    return Utf8Parser::encode(0xE000|c.index());
}
FormattedString FormatterDarkplaces::decode(const std::string& source) const
{
    FormattedString str;

    Utf8Parser parser;

    FormatFlags flags;
    parser.callback_ascii = [&str,&flags,&parser](uint8_t byte)
    {
        if ( byte == '^' )
        {
            auto pos = parser.input.tellg();
            char next = parser.input.get();
            if ( next == '^' )
            {
                str.append(new Character('^'));
                return;
            }
            else if ( std::isdigit(next) )
            {
                str.append(new Color(color::Color12::from_dp(std::string(1,next))));
                return;
            }
            else if ( next == 'x' )
            {
                std::string col = "^x";
                int i = 0;
                for ( ; i < 3 && std::isxdigit(parser.input.peek()); i++ )
                {
                    col += parser.input.get();
                }
                if ( col.size() == 5 )
                {
                    str.append(new Color(color::Color12::from_dp(col)));
                    return;
                }
            }
            parser.input.seekg(pos);
        }
        str.append(new Character(byte));
    };
    parser.callback_utf8 = [&str](uint32_t unicode,const std::string& utf8)
    {
        if ( (unicode & 0xff00) == 0xe000 )
            str.append(new QFont(unicode&0xff));
        else
            str.append(new Unicode(utf8,unicode));
    };

    parser.parse(source);

    return str;

}
std::string FormatterDarkplaces::name() const
{
    return "dp";
}


} // namespace string
