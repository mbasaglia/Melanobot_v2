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
 */
#ifndef STRING_HPP
#define STRING_HPP

#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <sstream>
#include <vector>

#include "formatter.hpp"
#include "color.hpp"

/**
 * \brief Namespace for string formatting
 */
namespace string {

/**
 * \brief Generic element of a string
 */
class Element
{
public:
    virtual ~Element() {}

    /**
     * \brief Visitor format to string
     */
    virtual std::string to_string(const Formatter& formatter) const = 0;
    std::string to_string(Formatter* formatter) const { return to_string(*formatter); }
    /**
     * \brief Returns the number of visible characters used to represent the element
     */
    virtual int char_count() const { return 0; }
};

/**
 * \brief Simple class representing some code which will clear colors and formats
 */
class ClearFormatting : public Element
{
public:
    std::string to_string(const Formatter& formatter) const override
    {
        return formatter.clear();
    }
};

/**
 * \brief Simple ASCII character
 */
class Character : public Element
{
public:
    Character(char c) : c(c) {}

    std::string to_string(const Formatter& formatter) const override
    {
        return formatter.ascii(c);
    }

    int char_count() const override { return 1; }

    /**
     * \brief Returns the character
     */
    char character() const noexcept { return c; }

private:
    char c;
};

/**
 * \brief Simple ASCII string
 */
class AsciiSubstring : public Element
{
public:
    AsciiSubstring(std::string s) : s(std::move(s)) {}

    std::string to_string(const Formatter& formatter) const override
    {
        return formatter.ascii(s);
    }

    int char_count() const override { return s.size(); }

    /**
     * \brief Returns the string
     */
    const std::string& string() const noexcept { return s; }

private:
    std::string s;
};

/**
 * \brief Color code
 */
class Color : public Element
{
public:
    Color ( color::Color12  color ) : color_(std::move(color)) {}

    std::string to_string(const Formatter& formatter) const override
    {
        return formatter.color(color_);
    }

    /**
     * \brief Returns the color
     */
    color::Color12 color() const noexcept { return color_; }

private:
    color::Color12 color_;
};

/**
 * \brief Formatting flag
 */
class Format : public Element
{
public:
    Format ( FormatFlags  flags ) : flags_(std::move(flags)) {}

    std::string to_string(const Formatter& formatter) const override
    {
        return formatter.format_flags(flags_);
    }
    /**
     * \brief Returns the format flags
     */
    FormatFlags flags() const noexcept { return flags_; }

private:
    FormatFlags flags_;
};

/**
 * \brief Unicode point
 */
class Unicode : public Element
{
public:
    Unicode ( std::string utf8, uint32_t point )
        : utf8_(std::move(utf8)), point_(point) {}

    std::string to_string(const Formatter& formatter) const override
    {
        return formatter.unicode(*this);
    }

    int char_count() const override { return 1; }

    /**
     * \brief Returns the UTF-8 representation
     */
    const std::string& utf8() const { return utf8_; }
    /**
     * \brief Returns the Unicode code point
     */
    uint32_t point() const { return point_; }

private:
    std::string utf8_;
    uint32_t    point_;
};

/**
 * \brief qfont character
 */
class QFont : public Element
{
public:
    explicit QFont ( unsigned index ) : index_(index) {}

    /**
     * \brief Returns the qfont index
     */
    unsigned index() const { return index_; }

    std::string to_string(const Formatter& formatter) const override
    {
        return formatter.qfont(*this);
    }

    int char_count() const override { return 1; }

    /**
     * \brief Gets an alternative representation of the character
     * \return An ASCII string aproximating the qfont character
     */
    std::string alternative() const
    {
        if ( index_ < qfont_table.size() )
            return qfont_table[index_];
        return "";
    }

    /**
     * \brief The qfont character as a custom unicode character
     */
    uint32_t unicode_point() const
    {
        return 0xE000 | index_;
    }

private:
    /**
     * \brief Maps weird characters to ASCII strings
     */
    static std::vector<std::string> qfont_table;
    unsigned index_; ///< Index in the qfont_table
};

/**
 * \brief A formatted string
 */
class FormattedString
{
public:
    using container      = std::vector<std::shared_ptr<const Element>>;
    using value_type     =  container::value_type;
    using reference      = container::reference;
    using const_reference= container::const_reference;
    using iterator       = container::iterator;
    using const_iterator = container::const_iterator;
    using difference_type= container::difference_type;
    using size_type      = container::size_type;

    template<class Iterator>
        FormattedString( const Iterator& i, const Iterator& j )
            : elements(i,j) {}
    explicit FormattedString(Formatter* formatter)
        : input_formatter(formatter) {}
    FormattedString(std::string ascii_string)
    {
        if ( !ascii_string.empty() )
            append<AsciiSubstring>(std::move(ascii_string));
    }
    FormattedString(const char* ascii_string)
        : FormattedString(std::string(ascii_string)) {}
    FormattedString() = default;
    FormattedString(const FormattedString&) = default;
    FormattedString(FormattedString&&) noexcept = default;
    FormattedString& operator=(const FormattedString&) = default;
    FormattedString& operator=(FormattedString&&) noexcept = default;

    iterator        begin()       { return elements.begin();}
    iterator        end()         { return elements.end();  }
    const_iterator  begin() const { return elements.begin();}
    const_iterator  end()   const { return elements.end();  }
    const_iterator  cbegin()const { return elements.begin();}
    const_iterator  cend()  const { return elements.end();  }

    reference       operator[] (size_type i)       { return elements[i]; }
    const_reference operator[] (size_type i) const { return elements[i]; }

    size_type size()  const { return elements.size(); }
    bool      empty() const { return elements.empty(); }

    void push_back( const_reference r )
    {
        elements.push_back(r);
    }

    iterator insert(const const_iterator& p, const value_type& t)
    {
        return elements.insert(p,t);
    }
    iterator insert(const const_iterator& p, size_type n, const value_type& t)
    {
        return elements.insert(p, n, t);
    }
    template<class InputIterator>
        iterator insert(const const_iterator& p, const InputIterator& i, const InputIterator& j )
        {
            return elements.insert(p, i, j);
        }
    iterator insert(const const_iterator& p, const std::initializer_list<value_type>& il )
    {
        return elements.insert(p,il);
    }

    void pop_back()
    {
        elements.pop_back();
    }

    iterator erase(const const_iterator& q)
    {
        return elements.erase(q);
    }
    iterator erase(const const_iterator& p, const const_iterator& q)
    {
        return elements.erase(p,q);
    }
    void clear()
    {
        elements.clear();
    }

    void assign(size_type n, const value_type& t)
    {
        elements.assign(n, t);
    }
    template<class InputIterator>
        void assign(const InputIterator& i, const InputIterator& j )
        {
            elements.assign(i, j);
        }
    void assign(const std::initializer_list<value_type>& il )
    {
        elements.assign(il);
    }

// Non-C++ container methods

    /**
     * \brief Adds some padding round the string
     * \param count     Minimum number of characters required,
     *                  if less than char_count(), won't do anything
     * \param fill      Character to use to fill
     * \param align     Alignment (0 = left, 1 = right)
     */
    FormattedString& pad(int count, double align = 1, char fill=' ')
    {
        count -= char_count();
        if ( count <= 0 )
            return *this;

        int before = count * align;
        if ( before )
            elements.insert(elements.begin(),
                melanolib::New<string::AsciiSubstring>(
                    std::string(before,fill)
            ));
        if ( count-before )
            elements.push_back(
                melanolib::New<string::AsciiSubstring>(
                    std::string(count-before,fill)
            ));

        return *this;
    }

    /**
     * \brief Counts the number of characters
     */
    int char_count() const
    {
        int c = 0;
        for ( const auto& p : elements )
            c += p->char_count();
        return c;
    }

    /**
     * \brief Append an element
     */
    template<class ElementType, class... Args>
        void append ( Args&&... args )
        {
            elements.push_back(std::make_shared<ElementType>(std::forward<Args>(args)...));
        }

    void append ( const_reference element )
    {
        elements.push_back(element);
    }


    void append ( const FormattedString& string )
    {
        elements.insert(elements.end(),string.begin(),string.end());
    }

    /**
     * \brief Encode the string using the given formatter
     * \throws CriticalException if the formatter is invalid
     */
    std::string encode(const std::string& format) const
    {
        return encode(Formatter::formatter(format));
    }

    /**
     * \brief Encode the string using the given formatter
     * \throws CriticalException if the formatter is invalid
     */
    std::string encode(Formatter* formatter) const;

    std::string encode(const Formatter& formatter) const
    {
        std::string s;
        for ( const auto& e : elements )
            s += e->to_string(formatter);
        return s;
    }

    // stream-like operations

    FormattedString& operator<< ( const std::string& text )
    {
        if ( !text.empty() )
        {
            if ( input_formatter )
                append(input_formatter->decode(text));
            else
                append(std::make_shared<AsciiSubstring>(text));
        }
        return *this;
    }
    FormattedString& operator<< ( const char* text )
    {
        return *this << std::string(text);
    }
    FormattedString& operator<< ( const color::Color12& color )
    {
        append(std::make_shared<Color>(color));
        return *this;
    }
    FormattedString& operator<< ( const FormatFlags& format_flags )
    {
        append(std::make_shared<Format>(format_flags));
        return *this;
    }
    FormattedString& operator<< ( FormatFlags::FormatFlagsEnum format_flags )
    {
        append(std::make_shared<Format>(format_flags));
        return *this;
    }
    FormattedString& operator<< ( ClearFormatting )
    {
        append(std::make_shared<ClearFormatting>());
        return *this;
    }
    FormattedString& operator<< ( char c )
    {
        append(std::make_shared<Character>(c));
        return *this;
    }
    FormattedString& operator<< ( const FormattedString& string )
    {
        if ( !string.empty() )
            append(string);
        return *this;
    }
    template <class T>
        FormattedString& operator<< ( const T& obj )
        {
            std::ostringstream ss;
            ss << obj;
            return *this << ss.str();
        }

private:
    container elements;
    /**
     * \brief Formatter used to decode std::string input using stream oprations
     *
     * If it's \b nullptr, it will be interpreted as an ASCII string
     */
    Formatter *input_formatter{nullptr};
};

/**
 * \brief Merge several formatted strings
 */
template<class Container, class =
    std::enable_if_t<
        std::is_same<
            FormattedString,
            std::decay_t<typename Container::value_type>
        >::value
    >>
FormattedString implode (const FormattedString& separator, const Container& elements)
{
    auto iter = std::begin(elements);
    auto end = std::end(elements);
    if ( iter == end )
        return {};

    FormattedString ret;
    while ( true )
    {
        ret << *iter;
        ++iter;
        if ( iter != end )
            ret << separator;
        else
            break;
    }
    return ret;
}

/**
 * \brief Plain UTF-8
 */
class FormatterUtf8 : public Formatter
{
public:
    std::string ascii(char c) const override;
    std::string ascii(const std::string& s) const override;
    std::string color(const color::Color12& color) const override;
    std::string format_flags(FormatFlags flags) const override;
    std::string clear() const override;
    std::string unicode(const Unicode& c) const override;
    std::string qfont(const QFont& c) const override;
    FormattedString decode(const std::string& source) const override;
    std::string name() const override;
};

/**
 * \brief Plain ASCII
 */
class FormatterAscii : public FormatterUtf8
{
public:
    std::string unicode(const Unicode& c) const override;
    FormattedString decode(const std::string& source) const override;
    std::string name() const override;
};

/**
 * \brief ANSI-formatted UTF-8 or ASCII
 */
class FormatterAnsi : public FormatterUtf8
{
public:
    explicit FormatterAnsi(bool utf8) : utf8(utf8) {}

    std::string color(const color::Color12& color) const override;
    std::string format_flags(FormatFlags flags) const override;
    std::string clear() const override;
    std::string unicode(const Unicode& c) const override;
    FormattedString decode(const std::string& source) const override;
    std::string name() const override;

private:
    bool utf8;
};

/**
 * \brief IRC formatter optimized for black backgrounds
 * \todo Might be better to have a generic color filter option in the formatter
 */
class FormatterAnsiBlack : public FormatterAnsi
{
public:
    using FormatterAnsi::FormatterAnsi;
    std::string color(const color::Color12& color) const override;
    std::string name() const override;
};

/**
 * \brief Custom string formatting (Utf-8)
 *
 * Style formatting:
 *      * ##            #
 *      * #-#           clear all formatting
 *      * #-b#          bold
 *      * #-u#          underline
 *      * #1#           red
 *      * #xf00#        red
 *      * #red#         red
 *      * #nocolor#     no color
 */
class FormatterConfig : public FormatterUtf8
{
public:
    explicit FormatterConfig() {}

    std::string ascii(char c) const override;
    std::string ascii(const std::string& s) const override;
    std::string color(const color::Color12& color) const override;
    std::string format_flags(FormatFlags flags) const override;
    std::string clear() const override;
    FormattedString decode(const std::string& source) const override;
    std::string name() const override;

};

} // namespace string

#endif // STRING_HPP
