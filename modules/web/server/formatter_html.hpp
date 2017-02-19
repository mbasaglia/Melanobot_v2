/**
 * \file
 *
 * \author Mattia Basaglia
 *
 * \copyright Copyright 2015-2017 Mattia Basaglia
 *
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
#ifndef MELANOBOT_MODULE_WEB_FORMATTER_HTML_HPP
#define MELANOBOT_MODULE_WEB_FORMATTER_HTML_HPP

#include "httpony/formats/quick_xml.hpp"
#include "string/string.hpp"

namespace web {

/**
 * \brief UTF-8 with IRC colors
 */
class FormatterHtml : public string::Formatter
{
private:
    struct SpanContext : public Context
    {
        std::string close_spans()
        {
            std::string result;
            while ( span_count > 0 )
            {
                span_count--;
                result += "</span>";
            }
            return result;
        }

        std::size_t span_count = 0;
    };

public:
    std::unique_ptr<Context> context() const override
    {
        return melanolib::New<SpanContext>();
    }

    std::string string_begin(Context* context) const override
    {
        if ( auto ctx = dynamic_cast<SpanContext*>(context) )
            ctx->span_count = 0;
        return {};
    }
    std::string string_end(Context* context) const override
    {
        if ( auto ctx = dynamic_cast<SpanContext*>(context) )
            return ctx->close_spans();
        return {};
    }

    std::string to_string(const string::AsciiString& s, Context* context) const override
    {
        return httpony::quick_xml::amp_escape(s);
    }

    std::string to_string(const string::Unicode& c, Context* context) const override
    {
        if ( c.utf8().size() == 1 )
            return to_string(c.utf8(), context);
        return c.utf8();
    }

    std::string to_string(const color::Color12& color, Context* context) const override
    {
        if ( auto ctx = dynamic_cast<SpanContext*>(context) )
            ctx->span_count++;
        return "<span style='color:" + color.to_html() + "'>";
    }

    std::string to_string(string::FormatFlags flags, Context* context) const override
    {
        if ( auto ctx = dynamic_cast<SpanContext*>(context) )
            ctx->span_count++;
        return std::string() + "<span style='"
            "font-weight:" + (flags & string::FormatFlags::BOLD ? "bold" : "normal") + ";"
            "text-decoration:" + (flags & string::FormatFlags::UNDERLINE ? "underline" : "none") + ";"
            "font-style:" + (flags & string::FormatFlags::ITALIC ? "italic" : "normal") + ";"
            "'>"
        ;
    }

    std::string to_string(string::ClearFormatting clear, Context* context) const override
    {
        if ( auto ctx = dynamic_cast<SpanContext*>(context) )
            return ctx->close_spans();
        return {};
    }

    /**
     * \todo Should at least decode what it encodes
     */
    string::FormattedString decode(const std::string& source) const override
    {

        melanolib::string::Utf8Parser parser(source);

        string::FormattedString str;
        string::AsciiString ascii;
        bool in_tag = false;

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
                // NOTE: not 100% accurate, but good enough to skip <span>s
                /// \todo Proper parsing of < <? <! <[ <--
                /// \todo Parse `&entities;`
                /// \todo Parse `<span style=''>`
                if ( !in_tag )
                {
                    if ( byte == '<' )
                        in_tag = true;
                    else
                        ascii += byte;
                }
                else if ( byte == '>' )
                {
                    in_tag = false;
                }
            }
            else
            {
                melanolib::string::Unicode unicode = parser.next();
                if ( unicode.valid() )
                {
                    push_ascii();
                    str.append(std::move(unicode));
                }
            }
        }

        push_ascii();
        return str;
    }

    std::string name() const override
    {
        return "html";
    }
};

} // namespace web
#endif // MELANOBOT_MODULE_WEB_FORMATTER_HTML_HPP
