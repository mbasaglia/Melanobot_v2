/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015-2017 Mattia Basaglia
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
#ifndef TELEGRAM_FORMATTER_HPP
#define TELEGRAM_FORMATTER_HPP

#include "string/string.hpp"

namespace telegram {

class FormatterMarkDown: public string::FormatterUtf8
{
private:
    struct TagContext : public Context
    {
        std::string close_bold()
        {
            if ( bold )
            {
                bold = false;
                return "*";
            }
            return "";
        }
        std::string open_bold()
        {
            if ( !bold )
            {
                bold = true;
                return "*";
            }
            return "";
        }

        std::string close_italic()
        {
            if ( italic )
            {
                italic = false;
                return "_";
            }
            return "";
        }

        std::string open_italic()
        {
            if ( !italic )
            {
                italic = true;
                return "_";
            }
            return "";
        }

        std::string close_all()
        {
            return close_italic() + close_bold();
        }

        bool bold = false;
        bool italic = false;

    };

public:
    std::unique_ptr<Context> context() const override
    {
        return melanolib::New<TagContext>();
    }

    std::string string_end(Context* context) const override
    {
        if ( auto ctx = dynamic_cast<TagContext*>(context) )
            return ctx->close_all();
        return {};
    }

    std::string to_string(char c, Context* context) const override
    {
        if ( c == '_' || c== '*' )
            return "\\" + c;
        return std::string(1, c);
    }

    std::string to_string(const string::AsciiString& s, Context* context) const override
    {
        return melanolib::string::replace(
            melanolib::string::replace(s, "*", "\\*"),
            "_", "\\_"
        );
    }

    std::string to_string(string::FormatFlags flags, Context* context) const override
    {
        std::string flagstring;
        if ( auto ctx = dynamic_cast<TagContext*>(context) )
        {
            if ( flags & string::FormatFlags::BOLD )
                flagstring += ctx->open_bold();
            else
                flagstring += ctx->close_bold();

            if ( flags & string::FormatFlags::ITALIC )
                flagstring += ctx->open_italic();
            else
                flagstring += ctx->close_italic();
        }
        return flagstring;
    }

    std::string to_string(string::ClearFormatting clear, Context* context) const override
    {
        if ( auto ctx = dynamic_cast<TagContext*>(context) )
            return ctx->close_all();
        return {};
    }

    string::FormattedString decode(const std::string& source) const override
    {
        string::FormattedString str;

        melanolib::string::Utf8Parser parser(source);

        string::AsciiString ascii;

        string::FormatFlags flags;

        while ( !parser.finished() )
        {
            melanolib::string::Utf8Parser::Byte byte = parser.next_ascii();
            if ( melanolib::string::ascii::is_ascii(byte) )
            {
                bool change_flags = false;
                if ( byte == '_' )
                {
                    change_flags = true;
                    flags ^= string::FormatFlags::ITALIC;
                }
                else if ( byte == '*' )
                {
                    change_flags = true;
                    flags ^= string::FormatFlags::BOLD;
                }

                if ( change_flags )
                {
                    str.append(ascii);
                    ascii.clear();
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

    std::string name() const override
    {
        return "telegram-md";
    }
};

} // namespace telegram
#endif // TELEGRAM_FORMATTER_HPP
