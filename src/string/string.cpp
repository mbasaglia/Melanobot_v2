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


std::string FormattedString::encode(Formatter* formatter) const
{
    if ( !formatter )
        CRITICAL_ERROR("Trying to encode a string without formatter");
    std::string s;
    for ( const auto& e : elements )
        s += e->to_string(*formatter);
    return s;
}

Formatter* Formatter::FormatterFactory::formatter(const std::string& name)
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

void Formatter::FormatterFactory::add_formatter(Formatter* instance)
{
    if ( formatters.count(instance->name()) )
        ErrorLog("sys") << "Overwriting formatter: " << instance->name();
    formatters[instance->name()] = instance;
    if ( ! default_formatter )
        default_formatter = instance;
}

static Formatter::RegisterDefaultFormatter register_default(new FormatterUtf8);

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
            str.append(std::make_shared<AsciiSubstring>(ascii));
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
        str.append(std::make_shared<Unicode>(utf8,unicode));
    };
    parser.callback_end = push_ascii;

    parser.parse(source);

    return str;
}
std::string FormatterUtf8::name() const
{
    return "utf8";
}


REGISTER_FORMATTER(FormatterAscii,ascii);

std::string FormatterAscii::unicode(const Unicode& c) const
{
    /// \todo Transliterate (eg: è -> e)
    return "?";
}
FormattedString FormatterAscii::decode(const std::string& source) const
{
    FormattedString str;
    str.append(std::make_shared<AsciiSubstring>(source));
    return str;
}

std::string FormatterAscii::name() const
{
    return "ascii";
}


REGISTER_FORMATTER(FormatterAnsi,ansi_utf8)(true);
REGISTER_FORMATTER(FormatterAnsi,ansi_ascii)(false);

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
    /// \todo Transliterate
    return "?";
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
            str.append(std::make_shared<AsciiSubstring>(ascii));
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
                    str.append(std::make_shared<ClearFormatting>());
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
                    str.append(std::make_shared<Color>(color::nocolor));
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
                    str.append(std::make_shared<Color>(color::Color12::from_4bit(i)));
                }
                else if ( i >= 90 && i <= 97 )
                {
                    i -= 90;
                    i &= 0xf;
                    str.append(std::make_shared<Color>(color::Color12::from_4bit(i)));
                }
            }
            if ( use_flags )
                str.append(std::make_shared<Format>(flags));
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
            str.append(std::make_shared<Unicode>(utf8,unicode));
    };
    parser.callback_end = push_ascii;

    parser.parse(source);

    return str;
}
std::string FormatterAnsi::name() const
{
    return utf8 ? "ansi-utf8" : "ansi-ascii";
}

REGISTER_FORMATTER(FormatterIrc,irc);

std::string FormatterIrc::color(const color::Color12& color) const
{
    int ircn = 1;

    if ( !color.is_valid() )
        return "\xf"; // no color


    switch ( color.to_4bit() )
    {
        case 0b0000: ircn =  1; break; // black
        case 0b0001: ircn =  5; break; // dark red
        case 0b0010: ircn =  3; break; // dark green
        case 0b0011: ircn =  7; break; // dark yellow
        case 0b0100: ircn =  2; break; // dark blue
        case 0b0101: ircn =  6; break; // dark magenta
        case 0b0110: ircn = 10; break; // dark cyan
        case 0b0111: ircn = 15; break; // silver
        case 0b1000: ircn = 14; break; // grey
        case 0b1001: ircn =  4; break; // red
        case 0b1010: ircn =  9; break; // green
        case 0b1011: ircn =  8; break; // yellow
        case 0b1100: ircn = 12; break; // blue
        case 0b1101: ircn = 13; break; // magenta
        case 0b1110: ircn = 11; break; // cyan
        case 0b1111: ircn =  0; break; // white
    }
    return '\3'+std::to_string(ircn);
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
std::string FormatterIrc::clear() const
{
    return "\xf";
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
                    str.append(std::make_shared<Format>(flags));
                }
                break;
            case '\x1f':
                if ( !(flags & FormatFlags::UNDERLINE) )
                {
                    flags |= FormatFlags::UNDERLINE;
                    str.append(std::make_shared<Format>(flags));
                }
                break;
            case '\xf':
                flags = FormatFlags::NO_FORMAT;
                str.append(std::make_shared<ClearFormatting>());
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
                        if ( std::isdigit(parser.input.peek()) )
                        {
                            parser.input.ignore();
                            if ( std::isdigit(parser.input.peek()) )
                                parser.input.ignore();
                        }
                        else
                            parser.input.unget();
                    }
                }
                str.append(std::make_shared<Color>(FormatterIrc::color_from_string(fg)));
                break;
            }
            case '\x1d': case '\x16':
                // Skip unsupported format flags
                break;
            default:
                str.append(std::make_shared<Character>(byte));
                break;
        }
    };
    parser.callback_utf8 = [&str](uint32_t unicode,const std::string& utf8)
    {
        str.append(std::make_shared<Unicode>(utf8,unicode));
    };

    parser.parse(source);

    return str;
}
std::string FormatterIrc::name() const
{
    return "irc";
}

color::Color12 FormatterIrc::color_from_string(const std::string& color)
{
    using namespace color;

    static std::regex regex = std::regex( "\3?([0-9]{2})",
        std::regex_constants::syntax_option_type::optimize |
        std::regex_constants::syntax_option_type::ECMAScript
    );

    std::smatch match;
    if ( !std::regex_match(color,match,regex) )
        return Color12();

    switch ( string::to_uint(match[1].str()) )
    {
        case  0: return white;
        case  1: return black;
        case  2: return dark_blue;
        case  3: return dark_green;
        case  4: return red;
        case  5: return dark_red;
        case  6: return dark_magenta;
        case  7: return dark_yellow;
        case  8: return yellow;
        case  9: return green;
        case 10: return dark_cyan;
        case 11: return cyan;
        case 12: return blue;
        case 13: return magenta;
        case 14: return gray;
        case 15: return silver;
        default: return Color12();
    }
}
REGISTER_FORMATTER(FormatterDarkplaces,dp);
std::string FormatterDarkplaces::ascii(char c) const
{
    return std::string(1,c);
}
std::string FormatterDarkplaces::ascii(const std::string& s) const
{
    return s;
}
std::string FormatterDarkplaces::color(const color::Color12& color) const
{
    switch ( color.to_bit_mask() )
    {
        case 0x000: return "^0";
        case 0xf00: return "^1";
        case 0x0f0: return "^2";
        case 0xff0: return "^3";
        case 0x00f: return "^4";
        case 0xf0f: return "^6";
        case 0x0ff: return "^5";
        case 0xfff: return "^7";
        case 0x888: return "^8";
        case 0xccc: return "^9";
    }
    return std::string("^x")+color.hex_red()+color.hex_green()+color.hex_blue();
}
std::string FormatterDarkplaces::format_flags(FormatFlags) const
{
    return "";
}
std::string FormatterDarkplaces::clear() const
{
    return "^7";
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
                str.append(std::make_shared<Character>('^'));
                return;
            }
            else if ( std::isdigit(next) )
            {
                str.append(std::make_shared<Color>(FormatterDarkplaces::color_from_string(std::string(1,next))));
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
                    str.append(std::make_shared<Color>(FormatterDarkplaces::color_from_string(col)));
                    return;
                }
            }
            parser.input.seekg(pos);
        }
        str.append(std::make_shared<Character>(byte));
    };
    parser.callback_utf8 = [&str](uint32_t unicode,const std::string& utf8)
    {
        if ( (unicode & 0xff00) == 0xe000 )
            str.append(std::make_shared<QFont>(unicode&0xff));
        else
            str.append(std::make_shared<Unicode>(utf8,unicode));
    };

    parser.parse(source);

    return str;

}
std::string FormatterDarkplaces::name() const
{
    return "dp";
}

color::Color12 FormatterDarkplaces::color_from_string(const std::string& color)
{
    using namespace color;

    static std::regex regex = std::regex( "\\^?([[:digit:]]|x([[:xdigit:]]{3}))",
        std::regex_constants::syntax_option_type::optimize |
        std::regex_constants::syntax_option_type::extended
    );
    std::smatch match;
    if ( !std::regex_match(color,match,regex) )
        return Color12();

    if ( !match[2].str().empty() )
        return Color12(match[2]);

    switch (match[1].str()[0])
    {
        case '0': return black;
        case '1': return red;
        case '2': return green;
        case '3': return yellow;
        case '4': return blue;
        case '5': return cyan;
        case '6': return magenta;
        case '7': return white;
        case '8': return gray;
        case '9': return silver;
    }
    return Color12();
}

} // namespace string
