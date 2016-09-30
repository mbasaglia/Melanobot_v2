/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015-2016 Mattia Basaglia
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
#include "logger.hpp"
#include "string/replacements.hpp"
#include "melanolib/string/encoding.hpp"
#include "melanolib/string/language.hpp"
#include "melanolib/string/stringutils.hpp"
#include "melanolib/utils/type_utils.hpp"

namespace string {

Formatter::Registry::Registry()
{
    // plural:
    //  number  anything convertible to ascii and then to int
    //  word    convertible to ascii, should be a single word
    FilterRegistry::instance().register_filter("plural",
        [](const std::vector<string::FormattedString>& args) -> string::FormattedString
        {
            if ( args.size() != 2 )
                return {};
            string::FormatterAscii ascii;
            int number = melanolib::string::to_int(args[0].encode(ascii));
            std::string word = args[1].encode(ascii);
            return melanolib::string::english.pluralize(number, word);
        }
    );

    // ucfirst:
    //  word    anything convertible to ascii
    FilterRegistry::instance().register_filter("ucfirst",
        [](const std::vector<string::FormattedString>& args) -> string::FormattedString
        {
            if ( args.size() != 1 )
                return {};
            string::FormatterAscii ascii;
            std::string word = args[0].encode(ascii);
            if ( !word.empty() )
                word[0] = std::toupper(word[0]);
            return word;
        }
    );

    // if:
    //  lhs     anything convertible to ascii
    //  rhs     anything convertible to ascii
    //  if-true string used if lhs == rhs
    //  [else]  string used if lhs != rhs
    FilterRegistry::instance().register_filter("if",
        [](const std::vector<string::FormattedString>& args) -> string::FormattedString
        {
            if ( args.size() < 3 )
                return {};
            string::FormatterAscii ascii;
            std::string lhs = args[0].encode(ascii);
            std::string rhs = args[1].encode(ascii);
            if ( lhs == rhs )
                return args[2];
            else if ( args.size() > 3 )
                return args[3];
            return {};
        }
    );

    add_formatter(default_formatter = new FormatterUtf8);
    add_formatter(new FormatterAscii);
    add_formatter(new FormatterAnsi(true));
    add_formatter(new FormatterAnsi(false));
    add_formatter(new FormatterConfig);
    add_formatter(new FormatterAnsiBlack(true));
    add_formatter(new FormatterAnsiBlack(false));
}

Formatter* Formatter::Registry::formatter(const std::string& name)
{
    auto it = formatters.find(name);
    if ( it == formatters.end() )
    {
        ErrorLog("sys") << "Invalid formatter: " << name;
        return default_formatter;
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


std::string FormatterUtf8::to_string(const Unicode& c, Context* context) const
{
    return c.utf8();
}

FormattedString FormatterUtf8::decode(const std::string& source) const
{
    FormattedString str;

    melanolib::string::Utf8Parser parser(source);

    string::AsciiString ascii;

    while ( !parser.finished() )
    {
        melanolib::string::Utf8Parser::Byte byte = parser.next_ascii();
        if ( melanolib::string::ascii::is_ascii(byte) )
        {
            ascii += byte;
        }
        else
        {
            melanolib::string::Unicode unicode = parser.next();
            if ( unicode.valid() )
            {
                if ( !ascii.empty() )
                {
                    str.append(ascii);
                    ascii.clear();
                }
                str.append(std::move(unicode));
            }
        }
    }

    if ( !ascii.empty() )
        str.append(ascii);

    return str;
}

std::string FormatterUtf8::name() const
{
    return "utf8";
}


std::string FormatterAscii::to_string(const Unicode& c, Context* context) const
{
    return std::string(1, melanolib::string::Utf8Parser::to_ascii(c.utf8()));
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

std::string FormatterAnsi::to_string(const color::Color12& color, Context* context) const
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

std::string FormatterAnsi::to_string(FormatFlags flags, Context* context) const
{
    std::vector<int> codes;
    codes.push_back( flags & FormatFlags::BOLD ? 1 : 22 );
    codes.push_back( flags & FormatFlags::UNDERLINE ? 4 : 24 );
    codes.push_back( flags & FormatFlags::ITALIC ? 3 : 23 );
    return  "\x1b["+melanolib::string::implode(";", codes)+"m";
}

std::string FormatterAnsi::to_string(ClearFormatting clear, Context* context) const
{
    return "\x1b[0m";
}

std::string FormatterAnsi::to_string(const Unicode& c, Context* context) const
{
    if ( utf8 )
        return c.utf8();
    return std::string(1, melanolib::string::Utf8Parser::to_ascii(c.utf8()));
}

FormattedString FormatterAnsi::decode(const std::string& source) const
{
    FormattedString str;

    melanolib::string::Utf8Parser parser(source);

    string::AsciiString ascii;

    auto push_ascii = [&ascii, &str]()
    {
        if ( !ascii.empty() )
        {
            str.append(ascii);
            ascii.clear();
        }
    };

    while ( !parser.finished() )
    {
        melanolib::string::Utf8Parser::Byte byte = parser.next_ascii();
        if ( melanolib::string::ascii::is_ascii(byte) )
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
        }
        else
        {
            melanolib::string::Unicode unicode = parser.next();
            if ( unicode.valid() && utf8 )
            {
                push_ascii();
                str.append(std::move(unicode));
            }
        }
    }

    push_ascii();

    return str;
}
std::string FormatterAnsi::name() const
{
    return utf8 ? "ansi-utf8" : "ansi-ascii";
}

std::string FormatterAnsiBlack::to_string(const color::Color12& color, Context* context) const
{
    if ( color.is_valid() && color.to_4bit() == 0 )
        return FormatterAnsi::to_string(color::silver, context);
    return FormatterAnsi::to_string(color, context);
}
std::string FormatterAnsiBlack::name() const
{
    return FormatterAnsi::name()+"_black";
}


std::string FormatterConfig::to_string(const color::Color12& color, Context* context) const
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

std::string FormatterConfig::to_string(FormatFlags flags, Context* context) const
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

std::string FormatterConfig::to_string(ClearFormatting clear, Context* context) const
{
    return "$(-)";
}

std::string FormatterConfig::to_string(char input, Context* context) const
{
    return input == '$' ? "$$" : std::string(1, input);
}

std::string FormatterConfig::to_string(const AsciiString& input, Context* context) const
{
    return melanolib::string::replace(input, "$", "$$");
}


class ConfigParser
{
public:
    ConfigParser(const std::string& source)
        : parser(source) {}

    FormattedString parse()
    {
        FormattedString output;
        lex();
        parse_string(output);
        return output;
    }

private:
    struct Token
    {
        enum Type
        {
            Invalid,
            Unicode,
            Argument,
            Variable,
            FunctionBegin,
            FunctionEnd,
            SimpleChar,
        };

        using Variant = melanolib::Variant<
            std::string,
            char,
            melanolib::string::Unicode
        >;

        Token(Type type = Invalid) : type(type) {}

        template<class T>
            Token(Type type, T&& value)
        : type(type),
          value(std::forward<T>(value))
        {}

        Type type;

        Variant value;
    };

    void parse_string(FormattedString& output)
    {
        AsciiString ascii;

        auto push_ascii = [&ascii, &output]()
        {
            if ( !ascii.empty() )
            {
                output.append(ascii);
                ascii.clear();
            }
        };

        while ( lookahead.type != Token::Invalid )
        {
            if ( lookahead.type == Token::Unicode )
            {
                push_ascii();
                output.append(value<melanolib::string::Unicode>());
                lex();
            }
            else if ( lookahead.type == Token::Variable )
            {
                push_ascii();
                output.append(Placeholder(value<std::string>()));
                lex();
            }
            else if ( lookahead.type == Token::FunctionBegin )
            {
                push_ascii();
                parse_function_call(output);
            }
            else if ( lookahead.type == Token::SimpleChar || lookahead.type == Token::FunctionEnd )
            {
                ascii += value<char>();
                lex();
            }
            else
            {
                ascii += value<std::string>();
                lex();
            }
        }
        push_ascii();
    }

    void parse_function_call(FormattedString& output)
    {
        if ( lex(true).type != Token::Argument )
            return skip_function();

        std::string function = value<std::string>();
        if ( function[0] == '-' )
        {
            if ( function.size() == 1 )
                output.append(ClearFormatting());
            else
                output.append(format_flags(function));
            return skip_function();
        }

        if ( function == "nocolor" )
        {
            output.append(color::Color12());
            return skip_function();
        }

        auto color = format_color(function);
        if ( color.is_valid() )
        {
            output.append(color);
            return skip_function();
        }

        std::vector<FormattedString> args;
        while ( lex(true).type == Token::Argument )
        {
            args.push_back(ConfigParser(value<std::string>()).parse());
        }

        if ( lookahead.type == Token::FunctionEnd )
            output.append(FilterCall(function, args));

        return skip_function();
    }

    void skip_function()
    {
        while ( lookahead.type != Token::FunctionEnd && lookahead.type != Token::Invalid )
            lex();
        lex();
    }

    Token lex(bool parameter_mode = false)
    {
        return lookahead = lex_token(parameter_mode);
    }

    Token lex_token(bool parameter_mode)
    {
        if ( parser.finished() )
            return Token::Invalid;

        auto character = parser.next_ascii();

        if ( character == '$' )
            return lex_dollar();

        if ( character == ')' )
            return {Token::FunctionEnd, ')'};

        if ( !melanolib::string::ascii::is_ascii(character) )
            return {Token::Unicode, parser.next()};

        if ( parameter_mode )
        {
            parser.input.unget();
            return lex_param();
        }

        return {Token::SimpleChar, character};
    }

    Token lex_dollar()
    {
        if ( parser.finished() )
            return {Token::SimpleChar, '$'};

        auto character = parser.next_ascii();

        if ( character == '$' )
            return {Token::SimpleChar, '$'};
        if ( character == '(' )
            return Token::FunctionBegin;
        if ( character == '{' )
            return {Token::Variable, parser.input.get_line('}')};
        if ( is_identifier(character) )
            return {
                Token::Variable,
                char(character) + parser.input.get_until(is_not_identifier, false)
            };
        if ( melanolib::string::ascii::is_ascii(character) )
            parser.input.unget();
        return {Token::SimpleChar, '$'};
    }

    Token lex_param()
    {
        parser.input.get_until(
            [](char c) { return !melanolib::string::ascii::is_space(c); },
            false
        );

        char next = parser.input.next();
        if ( next == ')' )
            return {Token::FunctionEnd, ')'};

        if ( parser.input.eof() )
            return Token::Invalid;

        std::string arg;
        if ( next == '\'' || next == '"' )
            arg = parser.input.get_line(next);
        else
            arg = next + parser.input.get_until(
                [](char c){ return c == ')' || melanolib::string::ascii::is_space(c); },
                false
            );

        if ( arg.empty() )
            return Token::Invalid;
        return {Token::Argument, arg};
    }

    /**
     * \pre format[0] == '-'
     */
    FormatFlags format_flags(const std::string& format)
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

    color::Color12 format_color(const std::string& format)
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

    static constexpr bool is_identifier(uint8_t byte)
    {
        return melanolib::string::ascii::is_alnum(byte) || byte == '_' || byte == '-';
    }

    static constexpr bool is_not_identifier(uint8_t byte)
    {
        return !is_identifier(byte);
    }

    template<class T>
        T& value()
    {
        return melanolib::get<T>(lookahead.value);
    }


    melanolib::string::Utf8Parser parser;
    Token lookahead;
};

FormattedString FormatterConfig::decode(const std::string& source) const
{
    return ConfigParser(source).parse();
}

std::string FormatterConfig::name() const
{
    return "config";
}

} // namespace string
