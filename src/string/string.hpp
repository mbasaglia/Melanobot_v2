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


using ReplacementFunctor = std::function<
    melanolib::Optional<class FormattedString> (const std::string&)
    >;

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

    virtual std::unique_ptr<Element> clone() const = 0;

    virtual void replace(const ReplacementFunctor& func) {}

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

    std::unique_ptr<Element> clone() const override
    {
        return melanolib::New<ClearFormatting>();
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

    std::unique_ptr<Element> clone() const override
    {
        return melanolib::New<Character>(c);
    }

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

    std::unique_ptr<Element> clone() const override
    {
        return melanolib::New<AsciiSubstring>(s);
    }

private:
    std::string s;
};

/**
 * \brief Color code
 */
class Color : public Element
{
public:
    Color(color::Color12 color) : color_(std::move(color)) {}

    std::string to_string(const Formatter& formatter) const override
    {
        return formatter.color(color_);
    }

    /**
     * \brief Returns the color
     */
    color::Color12 color() const noexcept { return color_; }

    std::unique_ptr<Element> clone() const override
    {
        return melanolib::New<Color>(color_);
    }

private:
    color::Color12 color_;
};

/**
 * \brief Formatting flag
 */
class Format : public Element
{
public:
    Format(FormatFlags  flags) : flags_(std::move(flags)) {}

    std::string to_string(const Formatter& formatter) const override
    {
        return formatter.format_flags(flags_);
    }
    /**
     * \brief Returns the format flags
     */
    FormatFlags flags() const noexcept { return flags_; }

    std::unique_ptr<Element> clone() const override
    {
        return melanolib::New<Format>(flags_);
    }

private:
    FormatFlags flags_;
};

/**
 * \brief Unicode point
 */
class Unicode : public Element
{
public:
    Unicode(std::string utf8, uint32_t point)
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

    std::unique_ptr<Element> clone() const override
    {
        return melanolib::New<Unicode>(*this);
    }

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
    explicit QFont(unsigned index) : index_(index) {}

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

    std::unique_ptr<Element> clone() const override
    {
        return melanolib::New<QFont>(index_);
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
class FormattedString : public Element
{
private:
    template<class T> class Tag {};

public:

    /**
     * \brief Object that contains a dynamic instance of \c Element
     *
     * Copying this object performs a deep copy of the contained value
     */
    class value_type
    {
    public:

        value_type(std::unique_ptr<Element> ptr) noexcept
            : ptr(std::move(ptr))
        {}

        template<class Y>
            value_type(std::unique_ptr<Y> ptr) noexcept
            : ptr(std::move(ptr))
        {}

        template<class E, class... Args>
            value_type(Tag<E>, Args&&... args)
            : ptr(melanolib::New<E>(std::forward<Args>(args)...))
        {}

        value_type(const value_type& oth)
        {
            ptr = oth->clone();
        }

        value_type(value_type&&) noexcept = default;

        value_type& operator=(const value_type& oth)
        {
            ptr = oth->clone();
            return *this;
        }

        value_type& operator=(value_type&&) noexcept = default;

        value_type clone() const
        {
            return ptr->clone();
        }

        const Element* operator->() const
        {
            return ptr.get();
        }

        const Element& operator*() const
        {
            return *ptr;
        }

        Element* operator->()
        {
            return ptr.get();
        }

        Element& operator*()
        {
            return *ptr;
        }

    private:
        std::unique_ptr<Element> ptr;
    };

    using container         = std::vector<value_type>;
    using reference         = container::reference;
    using const_reference   = container::const_reference;
    using iterator          = container::iterator;
    using const_iterator    = container::const_iterator;
    using difference_type   = container::difference_type;
    using size_type         = container::size_type;

    template<class Iterator>
        FormattedString( const Iterator& i, const Iterator& j )
            : elements(i,j) {}

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

    void push_back(value_type r)
    {
        elements.push_back(std::move(r));
    }

    iterator insert(const const_iterator& p, value_type t)
    {
        return elements.insert(p, std::move(t));
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
        return elements.insert(p, il);
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
        return elements.erase(p, q);
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
            elements.emplace(elements.begin(),
                Tag<string::AsciiSubstring>(),
                std::string(before,fill)
            );
        if ( count-before )
            elements.emplace_back<>(
                Tag<string::AsciiSubstring>(),
                std::string(count-before,fill)
            );

        return *this;
    }

    /**
     * \brief Append an element
     */
    template<class ElementType, class... Args>
        void append(Args&&... args)
        {
            elements.emplace_back(Tag<ElementType>(), std::forward<Args>(args)...);
        }

    void append(value_type element)
    {
        elements.push_back(std::move(element));
    }


    void append(const FormattedString& string)
    {
        elements.reserve(size() + string.size());
        for ( const auto& element : string )
            elements.emplace_back(element/*.clone()*/);
    }

    /**
     * \brief Encode the string using the given formatter
     */
    std::string encode(const Formatter& formatter) const
    {
        std::string s;
        for ( const auto& e : elements )
            s += e->to_string(formatter);
        return s;
    }

    /**
     * \brief Deep copy
     */
    FormattedString copy() const
    {
        FormattedString c;
        c.elements.reserve(elements.size());
        for ( const auto& element : *this )
            c.elements.emplace_back(element/*.clone()*/);
        return c;
    }

    /**
     * \brief Replace placeholders based on a map
     */
    template<class Map, class = typename Map::iterator>
        void replace(const Map& map)
        {
            replace(
                [&map](const std::string& id)
                    -> melanolib::Optional<FormattedString>
                {
                    auto it = map.find(id);
                    if ( it != map.end() )
                        return FormattedString(it->second);
                    return {};
                }
            );
        }

    void replace(const std::string& placeholder, const FormattedString& string)
    {
        replace(
            [&string, &placeholder](const std::string& id)
                -> melanolib::Optional<FormattedString>
            {
                if ( id == placeholder )
                    return string.copy();
                return {};
            }
        );
    }

    /**
     * \brief Replace placeholders based on a map
     */
    template<class Map, class = typename Map::iterator>
        FormattedString replaced(const Map& map) const
        {
            FormattedString str = copy();
            str.replace(map);
            return str;
        }

    FormattedString replaced(const ReplacementFunctor& func) const
    {
        FormattedString str = copy();
        str.replace(func);
        return str;
    }

    FormattedString replaced(const std::string& placeholder, const FormattedString& string)
    {
        FormattedString str = copy();
        str.replace(placeholder, string);
        return str;
    }

// Element overrides

    /**
     * \brief Counts the number of characters
     */
    int char_count() const final
    {
        int c = 0;
        for ( const auto& p : elements )
            c += p->char_count();
        return c;
    }

    std::string to_string(const Formatter& formatter) const final
    {
        return encode(formatter);
    }

    std::unique_ptr<Element> clone() const override
    {
        return melanolib::New<FormattedString>(copy());
    }

    void replace(const ReplacementFunctor& func) override
    {
        for ( auto& element : elements )
            element->replace(func);
    }

// Stream-like operations

    FormattedString& operator<<(const std::string& text) &
    {
        if ( !text.empty() )
        {
            elements.emplace_back(Tag<AsciiSubstring>(), text);
        }
        return *this;
    }
    FormattedString& operator<<(const char* text) &
    {
        return *this << std::string(text);
    }
    FormattedString& operator<<(const color::Color12& color) &
    {
        elements.emplace_back(Tag<Color>(), color);
        return *this;
    }
    FormattedString& operator<<(const FormatFlags& format_flags) &
    {
        elements.emplace_back(Tag<Format>(), format_flags);
        return *this;
    }
    FormattedString& operator<<(FormatFlags::FormatFlagsEnum format_flags) &
    {
        elements.emplace_back(Tag<Format>(), format_flags);
        return *this;
    }
    FormattedString& operator<<(ClearFormatting) &
    {
        elements.emplace_back(Tag<ClearFormatting>());
        return *this;
    }
    FormattedString& operator<<(char c) &
    {
        elements.emplace_back(Tag<Character>(), c);
        return *this;
    }
    FormattedString& operator<<(const FormattedString& string) &
    {
        if ( !string.empty() )
            append(string);
        return *this;
    }
    template <class T>
        FormattedString& operator<< ( const T& obj ) &
        {
            std::ostringstream ss;
            ss << obj;
            return *this << ss.str();
        }

    template <class T>
        FormattedString&& operator<<(T&& obj) &&
        {
            static_cast<FormattedString&>(*this) << std::forward<T>(obj);
            return std::move(*this);
        }

private:
    container elements;
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
 *      * $$            $
 *      * $(-)          clear all formatting
 *      * $(-b)         bold
 *      * $(-u)         underline
 *      * $(1)          red
 *      * $(xf00)       red
 *      * $(red)        red
 *      * $(nocolor)    no color
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

using FormattedProperties = std::unordered_map<std::string, FormattedString>;

} // namespace string

#endif // STRING_HPP
