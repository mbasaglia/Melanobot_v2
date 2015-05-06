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
        CRITICAL_ERROR("Trying to encode a string without formatter");
    return encode(*formatter);
}

std::string FormatterPlain::ascii(char c) const
{
    return std::string(1,c);
}
std::string FormatterPlain::ascii(const std::string& s) const
{
    return s;
}
std::string FormatterPlain::color(const color::Color12& color) const
{
    return "";
}
std::string FormatterPlain::format_flags(FormatFlags flags) const
{
    return "";
}
std::string FormatterPlain::clear() const
{
    return "";
}
std::string FormatterPlain::unicode(const Unicode& c) const
{
    return encoding()->encode(c);
}
std::string FormatterPlain::qfont(const QFont& c) const
{
    return c.alternative();
}
FormattedString FormatterPlain::decode(const std::string& source) const
{
    return Utf8Encoding().parse(source,*this);
}
std::string FormatterPlain::name() const
{
    return "plain";
}

void decode_ascii(DecodeEnvironment& env, uint8_t byte) const
{
    env.ascii_substring += byte;
}
void decode_unicode(DecodeEnvironment& env, const Unicode& unicode) const
{
    env.push_ascii();
    env.output.push_back(unicode);
}
void decode_end(DecodeEnvironment& env) const
{
    env.push_ascii();
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
    return  "\x1b["+implode(";",codes)+"m";
}
std::string FormatterAnsi::clear() const
{
    return "\x1b[0m";
}
void FormatterAnsi::decode_ascii(DecodeEnvironment& env, uint8_t byte) const
{
    if ( byte == '\x1b' && env.input.peek() == '[' )
    {
        env.push_ascii();
        env.input.ignore();
        std::vector<int> codes;
        while (env.input)
        {
            int i = 0;
            if ( env.input.get_int(i) )
            {
                codes.push_back(i);
                char next = env.input.next();
                if ( next != ';' )
                {
                    if ( next != 'm' )
                        env.input.unget();
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
                env.output.append<ClearFormatting>();
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
                env.output.append<Color>(color::nocolor);
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
                env.output.append<Color>(color::Color12::from_4bit(i));
            }
            else if ( i >= 90 && i <= 97 )
            {
                i -= 90;
                i |= 0b1000;
                env.output.append<Color>(color::Color12::from_4bit(i));
            }
        }
        if ( use_flags )
            env.output.append<Format>(flags);
    }
    else
    {
        env.ascii_substring += byte;
    }
}
std::string FormatterAnsi::name() const
{
    return "ansi";
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
    return string::replace(input,"#","##");
}
FormattedString FormatterConfig::decode(const std::string& source) const
{
    FormattedString str;

    Utf8Encoding parser;

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
