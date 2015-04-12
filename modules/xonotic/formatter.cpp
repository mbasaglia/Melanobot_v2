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
#include "formatter.hpp"

#include <regex>

#include "string/string_functions.hpp"

namespace xonotic {


std::string Formatter::ascii(char c) const
{
    return c == '^' ? "^^" : std::string(1,c);
}
std::string Formatter::ascii(const std::string& input) const
{
    return string::replace(input,"^","^^");
}
std::string Formatter::color(const color::Color12& color) const
{
    if ( !color.is_valid() )
        return "^7";
    
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
std::string Formatter::format_flags(string::FormatFlags) const
{
    return "";
}
std::string Formatter::clear() const
{
    return "^7";
}
std::string Formatter::unicode(const string::Unicode& c) const
{
    return c.utf8();
}
std::string Formatter::qfont(const string::QFont& c) const
{
    return string::Utf8Parser::encode(0xE000|c.index());
}
string::FormattedString Formatter::decode(const std::string& source) const
{
    string::FormattedString str;
    string::Utf8Parser parser;
    std::string ascii;

    auto push_ascii = [&ascii,&str]()
    {
        if ( !ascii.empty() )
        {
            str.append<string::AsciiSubstring>(ascii);
            ascii.clear();
        }
    };

    parser.callback_ascii = [&str,&parser,&ascii,push_ascii](uint8_t byte)
    {
        if ( byte == '^' )
        {
            auto pos = parser.input.tellg();
            char next = parser.input.get();
            if ( next == '^' )
            {
                ascii += '^';
                return;
            }
            else if ( std::isdigit(next) )
            {
                push_ascii();
                str.append<string::Color>(Formatter::color_from_string(std::string(1,next)));
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
                    push_ascii();
                    str.append<string::Color>(color_from_string(col));
                    return;
                }
            }
            parser.input.seekg(pos); /// \todo could just append from pos to the current position
        }
        ascii += byte;
    };
    parser.callback_utf8 = [&str,push_ascii](uint32_t unicode,const std::string& utf8)
    {
        push_ascii();
        if ( (unicode & 0xff00) == 0xe000 )
            str.append<string::QFont>(unicode&0xff);
        else
            str.append<string::Unicode>(utf8,unicode);
    };
    parser.callback_end = push_ascii;

    parser.parse(source);

    return str;

}
std::string Formatter::name() const
{
    return "xonotic";
}

color::Color12 Formatter::color_from_string(const std::string& color)
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

} // namespace xonotic
