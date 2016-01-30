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
#include "melanolib/string/stringutils.hpp"
#include "encoding.hpp"

namespace string {

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
    {
        ErrorLog("sys") << "Trying to encode a string without formatter";
        return {};
    }
    return encode(*formatter);
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
        return "\x1b[39m";

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
    codes.push_back( flags & FormatFlags::ITALIC ? 3 : 23 );
    return  "\x1b["+melanolib::string::implode(";",codes)+"m";
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

    parser.callback_ascii = [&parser,&str,&ascii,push_ascii](uint8_t byte)
    {
        if ( byte == '\x1b' && parser.input.peek() == '[' )
        {
            push_ascii();
            parser.input.ignore();
            std::vector<int> codes;
            while (parser.input)
            {
                int i = 0;
                if ( parser.input.get_int(i) )
                {
                    codes.push_back(i);
                    char next = parser.input.next();
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
                else if ( i == 3 )
                {
                    flags |= FormatFlags::ITALIC;
                    use_flags = true;
                }
                else if ( i == 23 )
                {
                    flags &= ~FormatFlags::ITALIC;
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
                        i |= 0b1000;
                        flags = FormatFlags::NO_FORMAT;
                        use_flags = false;
                    }
                    str.append<Color>(color::Color12::from_4bit(i));
                }
                else if ( i >= 90 && i <= 97 )
                {
                    i -= 90;
                    i |= 0b1000;
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

std::string FormatterAnsiBlack::color(const color::Color12& color) const
{
    if ( color.is_valid() && color.to_4bit() == 0 )
        return FormatterAnsi::color(color::silver);
    return FormatterAnsi::color(color);
}
std::string FormatterAnsiBlack::name() const
{
    return FormatterAnsi::name()+"_black";
}

std::string FormatterConfig::color(const color::Color12& color) const
{
    if ( !color.is_valid() )
        return "#";

    switch ( color.to_bit_mask() )
    {
        case 0x000: return "#0#";
        case 0xf00: return "#1#";
        case 0x0f0: return "#2#";
        case 0xff0: return "#3#";
        case 0x00f: return "#4#";
        case 0xf0f: return "#5#";
        case 0x0ff: return "#6#";
        case 0xfff: return "#7#";
    }
    return std::string("#x")+color.hex_red()+color.hex_green()+color.hex_blue()+'#';
}
std::string FormatterConfig::format_flags(FormatFlags flags) const
{
    std::string r = "#-";
    if ( flags & FormatFlags::BOLD )
        r += "b";
    if ( flags & FormatFlags::UNDERLINE )
        r += "u";
    if ( flags & FormatFlags::ITALIC )
        r += "i";
    return r+'#';
}
std::string FormatterConfig::clear() const
{
    return "#-#";
}
std::string FormatterConfig::ascii(char input) const
{
    return input == '#' ? "##" : std::string(1,input);
}
std::string FormatterConfig::ascii(const std::string& input) const
{
    return melanolib::string::replace(input,"#","##");
}
FormattedString FormatterConfig::decode(const std::string& source) const
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

    parser.callback_ascii = [&parser,&str,&ascii,push_ascii](uint8_t byte)
    {
        if ( byte == '#' )
        {
            std::string format = parser.input.get_line('#');

            if ( format.empty() )
            {
                ascii += '#';
            }
            else
            {
                push_ascii();
                if ( format[0] == '-' )
                {
                    FormatFlags flag;
                    for ( std::string::size_type i = 1; i < format.size(); i++ )
                    {
                        if ( format[i] == 'b' )
                            flag |= FormatFlags::BOLD;
                        else if ( format[i] == 'u' )
                            flag |= FormatFlags::UNDERLINE;
                        else if ( format[i] == 'i' )
                            flag |= FormatFlags::ITALIC;
                    }
                    if ( !flag )
                        str << ClearFormatting();
                    else
                        str << flag;
                }
                else if ( format[0] == 'x' )
                {
                    str << color::Color12(format.substr(1));
                }
                else if ( std::isdigit(format[0]) )
                {
                    str << color::Color12::from_4bit(0b1000|(format[0]-'0'));
                }
                else
                {
                    str << color::Color12::from_name(format);
                }
            }
        }
        else
        {
            ascii += byte;
        }
    };

    parser.callback_utf8 = [this,&str,push_ascii](uint32_t unicode,const std::string& utf8)
    {
        push_ascii();
        str.append<Unicode>(utf8,unicode);
    };
    parser.callback_end = push_ascii;

    parser.parse(source);

    return str;

}
std::string FormatterConfig::name() const
{
    return "config";
}

} // namespace string
