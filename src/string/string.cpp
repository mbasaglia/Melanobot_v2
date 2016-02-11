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
#include "replacements.hpp"
#include "melanolib/utils.hpp"

namespace string {

std::string FormatterUtf8::to_string(const Unicode& c) const
{
    return c.utf8();
}

FormattedString FormatterUtf8::decode(const std::string& source) const
{
    FormattedString str;

    Utf8Parser parser;

    string::AsciiString ascii;

    auto push_ascii = [&ascii,&str]()
    {
        if ( !ascii.empty() )
        {
            str.append(ascii);
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
        str.append(Unicode(utf8,unicode));
    };
    parser.callback_end = push_ascii;

    parser.parse(source);

    return str;
}

std::string FormatterUtf8::name() const
{
    return "utf8";
}


std::string FormatterAscii::to_string(const Unicode& c) const
{
    return std::string(1,Utf8Parser::to_ascii(c.utf8()));
}

FormattedString FormatterAscii::decode(const std::string& source) const
{
    FormattedString str;
    str.append(source);
    return str;
}

std::string FormatterAscii::name() const
{
    return "ascii";
}

std::string FormatterAnsi::to_string(const color::Color12& color) const
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

std::string FormatterAnsi::to_string(FormatFlags flags) const
{
    std::vector<int> codes;
    codes.push_back( flags & FormatFlags::BOLD ? 1 : 22 );
    codes.push_back( flags & FormatFlags::UNDERLINE ? 4 : 24 );
    codes.push_back( flags & FormatFlags::ITALIC ? 3 : 23 );
    return  "\x1b["+melanolib::string::implode(";",codes)+"m";
}

std::string FormatterAnsi::to_string(ClearFormatting clear) const
{
    return "\x1b[0m";
}

std::string FormatterAnsi::to_string(const Unicode& c) const
{
    if ( utf8 )
        return c.utf8();
    return std::string(1,Utf8Parser::to_ascii(c.utf8()));
}

FormattedString FormatterAnsi::decode(const std::string& source) const
{
    FormattedString str;

    Utf8Parser parser;

    AsciiString ascii;

    auto push_ascii = [&ascii,&str]()
    {
        if ( !ascii.empty() )
        {
            str.append(ascii);
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
                if ( i == 0 )
                {
                    str.append(ClearFormatting());
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
                    str.append(color::nocolor);
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
                    str.append(color::Color12::from_4bit(i));
                }
                else if ( i >= 90 && i <= 97 )
                {
                    i -= 90;
                    i |= 0b1000;
                    str.append(color::Color12::from_4bit(i));
                }
            }
            if ( use_flags )
                str.append(flags);
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
            str.append(Unicode(utf8, unicode));
    };
    parser.callback_end = push_ascii;

    parser.parse(source);

    return str;
}
std::string FormatterAnsi::name() const
{
    return utf8 ? "ansi-utf8" : "ansi-ascii";
}

std::string FormatterAnsiBlack::to_string(const color::Color12& color) const
{
    if ( color.is_valid() && color.to_4bit() == 0 )
        return FormatterAnsi::to_string(color::silver);
    return FormatterAnsi::to_string(color);
}
std::string FormatterAnsiBlack::name() const
{
    return FormatterAnsi::name()+"_black";
}


std::string FormatterConfig::to_string(const color::Color12& color) const
{
    if ( !color.is_valid() )
        return "$(nocolor)";

    switch ( color.to_bit_mask() )
    {
        case 0x000: return "$(0)";
        case 0xf00: return "$(1)";
        case 0x0f0: return "$(2)";
        case 0xff0: return "$(3)";
        case 0x00f: return "$(4)";
        case 0xf0f: return "$(5)";
        case 0x0ff: return "$(6)";
        case 0xfff: return "$(7)";
    }
    return std::string("$(x")+color.hex_red()+color.hex_green()+color.hex_blue()+')';
}

std::string FormatterConfig::to_string(FormatFlags flags) const
{
    std::string r = "$(-";
    if ( flags & FormatFlags::BOLD )
        r += "b";
    if ( flags & FormatFlags::UNDERLINE )
        r += "u";
    if ( flags & FormatFlags::ITALIC )
        r += "i";
    return r+')';
}

std::string FormatterConfig::to_string(ClearFormatting clear) const
{
    return "$(-)";
}

std::string FormatterConfig::to_string(char input) const
{
    return input == '$' ? "$$" : std::string(1, input);
}

std::string FormatterConfig::to_string(const AsciiString& input) const
{
    return melanolib::string::replace(input,"$","$$");
}

/**
 * \pre format[0] == '-'
 */
static FormatFlags cfg_format_flags(const std::string& format)
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

    return flag;
}

static color::Color12 cfg_format_color(const std::string& format)
{
    if ( format.size() == 4 && format[0] == 'x' &&
        std::all_of(format.begin()+1, format.end(),
            melanolib::FunctionPointer<int(int)>(std::isxdigit))
       )
    {
        return color::Color12(format.substr(1));
    }

    if ( format.size() == 1 && std::isdigit(format[0]) )
        return color::Color12::from_4bit(0b1000|(format[0]-'0'));

    return color::Color12::from_name(format);
}

static bool cfg_next_arg(char c)
{
    return c == ')' || std::isspace(c);
}

static std::vector<FormattedString> cfg_args(
    melanolib::string::QuickStream& stream,
    const FormatterConfig& cfg
)
{
    std::vector<FormattedString> args;
    while ( true )
    {
        stream.get_until(
            [](char c) { return !std::isspace(c); },
            false
        );
        char next = stream.next();
        if ( stream.eof() || next == ')' )
            break;

        std::string arg;
        if ( next == '\'' || next == '"' )
            arg =stream.get_line(next);
        else
            arg = next+stream.get_until(cfg_next_arg, false);

        args.push_back(cfg.decode(arg));
    }

    return args;
}

FormattedString FormatterConfig::decode(const std::string& source) const
{
    FormattedString str;

    Utf8Parser parser;

    AsciiString ascii;

    auto push_ascii = [&ascii,&str]()
    {
        if ( !ascii.empty() )
        {
            str.append(ascii);
            ascii.clear();
        }
    };

    parser.callback_ascii =
        [&parser, &str, &ascii, push_ascii, this](uint8_t byte)
    {
        if ( byte != '$' )
        {
            ascii += byte;
            return;
        }

        char next = parser.input.next();

        if ( next == '(' )
        {
            std::string format = parser.input.get_until(cfg_next_arg);

            if ( format.empty() )
                return;

            push_ascii();
            if ( format[0] == '-' )
            {
                if ( auto flag = cfg_format_flags(format) )
                    str << flag;
                else
                    str << ClearFormatting();

                return;
            }

            if ( format == "nocolor" )
            {
                str << color::Color12();
                return;
            }

            auto color = cfg_format_color(format);
            if ( color.is_valid() )
            {
                str << color;
                return;
            }

            str.append(FilterCall(format, cfg_args(parser.input, *this)));
        }
        else if ( next == '{' )
        {
            std::string id = parser.input.get_line('}');
            if ( !id.empty() )
            {
                push_ascii();
                str.append(Placeholder(id));
            }

        }
        else if ( next == '$' )
        {
            ascii += '$';
        }
        else if ( std::isalnum(next) || next == '_' )
        {
            std::string id = next+parser.input.get_until(
                [](char c) { return !std::isalnum(c) && c != '_' && c != '.'; },
                false
            );
            push_ascii();
            str.append(Placeholder(id));
        }
        else
        {
            ascii += byte;
            parser.input.unget();
        }
    };

    parser.callback_utf8 = [this,&str,push_ascii](uint32_t unicode,const std::string& utf8)
    {
        push_ascii();
        str.append(Unicode(utf8, unicode));
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
