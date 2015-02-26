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
#ifndef STRING_HPP
#define STRING_HPP

#include <cstdint>
#include <functional>
#include <iterator>
#include <stdexcept>
#include <string>
#include <sstream>
#include <unordered_map>
#include <vector>

#include "color.hpp"
#include "logger.hpp"

/**
 * \brief Namespace for string formatting
 */
namespace string {

/**
 * \brief Turn a container into a string
 * \pre Container::const_iterator is a ForwardIterator
 *      Container::value_type has the stream operator
 * \note Should work on arrays just fine
 */
template<class Container>
    std::string implode(const std::string& glue, const Container& elements)
    {
        auto iter = std::begin(elements);
        auto end = std::end(elements);
        if ( iter == end )
            return "";

        std::ostringstream ss;
        while ( true )
        {
            ss << *iter;
            ++iter;
            if ( iter != end )
                ss << glue;
            else
                break;
        }

        return ss.str();
    }

/**
 * \brief Class used to parse and convert UTF-8
 */
class Utf8Parser
{
public:
    typedef uint8_t Byte;

    std::function<void(uint8_t)>                     callback_ascii;
    std::function<void(uint32_t,const std::string&)> callback_utf8;
    std::function<void(const std::string&)>          callback_invalid;
    std::function<void()>                            callback_end;

    std::istringstream input;

    /**
     * \brief Parse the string
     */
    void parse(const std::string& string);

    /**
     * \brief Whether the end of the string has been reached
     */
    bool finished() const { return !input; }

    /**
     * \brief Encode a unicode value to UTF-8
     */
    static std::string encode(uint32_t value);


private:
    std::string           utf8;         ///< Multibyte string
    uint32_t              unicode;      ///< Multibyte value
    unsigned              length = 0;   ///< Multibyte length

    /**
     * \brief Handles an invalid/incomplete sequence
     */
    void check_valid();

    template<class... FuncArgs, class... PassedArgs>
        void call_back(const std::function<void(FuncArgs...)>& function, PassedArgs... args)
        {
            if ( function )
                function(args...);
        }
};


/**
 * \brief String formatting flags
 */
class FormatFlags
{
public:
    enum FormatFlagsEnum {
        NO_FORMAT = 0,
        BOLD      = 1,
        UNDERLINE = 2,
    };
    constexpr FormatFlags(FormatFlagsEnum flags) : flags(flags) {}

    constexpr FormatFlags() = default;

    explicit constexpr operator bool() const { return flags; }

    constexpr FormatFlags operator| ( const FormatFlags& o ) const
    {
        return flags|o.flags;
    }

    constexpr FormatFlags operator& ( const FormatFlags& o ) const
    {
        return flags&o.flags;
    }

    constexpr FormatFlags operator^ ( const FormatFlags& o ) const
    {
        return flags^o.flags;
    }

    constexpr FormatFlags operator~ () const
    {
        return ~flags;
    }

    constexpr bool operator== ( const FormatFlags& o ) const
    {
        return flags == o.flags;
    }

    FormatFlags& operator|= ( const FormatFlags& o )
    {
        flags |= o.flags;
        return *this;
    }

    FormatFlags& operator&= ( const FormatFlags& o )
    {
        flags &= o.flags;
        return *this;
    }

    FormatFlags& operator^= ( const FormatFlags& o )
    {
        flags ^= o.flags;
        return *this;
    }

private:
    constexpr FormatFlags(int flags) : flags(flags) {}

    int flags = 0;
};

class Unicode;
class QFont;
class FormattedString;

/**
 * \brief Abstract formatting visitor (and factory)
 * \note Formatters should register here
 */
class Formatter
{
private:
    /**
     * \brief Internal factory
     *
     * Factory needs a singleton, Formatter is abstract so it needs a different
     * class, but Formatter is the place to go to access the factory.
     *
     * Also note that this doesn't really create objects since they don't need
     * any data, so the objects are created only once.
     */
    class FormatterFactory
    {
    public:
        static FormatterFactory& instance()
        {
            static FormatterFactory factory;
            return factory;
        }

        ~FormatterFactory()
        {
            for ( const auto& f : formatters )
                delete f.second;
        }

        /**
         * \brief Get a registered formatter
         */
        Formatter* formatter(const std::string& name)
        {
            auto it = formatters.find(name);
            if ( it == formatters.end() )
            {
                Log("sys",'!',0) << ::color::red << "Error" << ::color::nocolor
                    << ": Invalid formatter: " << name;
                if ( default_formatter )
                    return default_formatter;
                CRITICAL_ERROR("Trying to access an invalid formatter");
            }
            return it->second;
        }

        /**
         * \brief Register a formatter
         */
        void add_formatter(Formatter* instance)
        {
            if ( formatters.count(instance->name()) )
                Log("sys",'!',0) << ::color::red << "Error" << ::color::nocolor
                    << ": Overwriting formatter: " << instance->name();
            formatters[instance->name()] = instance;
            if ( ! default_formatter )
                default_formatter = instance;
        }

        std::unordered_map<std::string,Formatter*> formatters;
        Formatter* default_formatter = nullptr;
    };
public:
    /**
     * \brief Quick way to register a formatter automatically (via static objects)
     */
    struct RegisterFormatter
    {
        RegisterFormatter(Formatter* formatter)
        {
            Formatter::FormatterFactory::instance().add_formatter(formatter);
        }
    };
    /**
     * \brief Same as RegisterFormatter but it also sets it as default
     * \note There should be exactly one of these
     */
    struct RegisterDefaultFormatter : public RegisterFormatter
    {
        RegisterDefaultFormatter(Formatter* formatter)
            : RegisterFormatter(formatter)
        {
            Formatter::FormatterFactory::instance().default_formatter = formatter;
        }
    };

    virtual ~Formatter() {}
    /**
     * \brief Encode a single ASCII character
     */
    virtual std::string ascii(char c) const = 0;
    /**
     * \brief Encode a color code
     */
    virtual std::string color(const color::Color12& color) const = 0;
    /**
     * \brief Encode format flags
     */
    virtual std::string format_flags(FormatFlags flags) const = 0;
    /**
     * \brief Encode a unicode (non-ASCII) character
     */
    virtual std::string unicode(const Unicode& c) const = 0;
    /**
     * \brief Encode a Darkpaces weird character
     */
    virtual std::string qfont(const QFont& c) const = 0;
    /**
     * \brief Decode a string
     */
    virtual FormattedString decode(const std::string& source) const = 0;

    /**
     * \brief Name of the format
     */
    virtual std::string name() const = 0;

    /**
     * \brief Get a string formatter from name
     */
    static Formatter* formatter(const std::string& name)
    {
        return FormatterFactory::instance().formatter(name);
    }
};

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
    virtual bool operator== (char c) const { return false; }
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
    bool operator== (char ch) const override { return c == ch; }

private:
    char c;
};
/**
 * \brief Color code
 */
class Color : public Element
{
public:
    Color ( const color::Color12& color ) : color(color) {}

    std::string to_string(const Formatter& formatter) const override
    {
        return formatter.color(color);
    }

private:
    color::Color12 color;
};

/**
 * \brief Formatting flag
 */
class Format : public Element
{
public:
    Format ( const FormatFlags& flags ) : flags(flags) {}

    std::string to_string(const Formatter& formatter) const override
    {
        return formatter.format_flags(flags);
    }

private:
    FormatFlags flags;
};

/**
 * \brief Unicode point
 */
class Unicode : public Element
{
public:
    Unicode ( const std::string& utf8, uint32_t point )
        : utf8_(utf8), point_(point) {}

    std::string to_string(const Formatter& formatter) const override
    {
        return formatter.unicode(*this);
    }

    const std::string& utf8() const { return utf8_; }
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

    unsigned index() const { return index_; }

    std::string to_string(const Formatter& formatter) const override
    {
        return formatter.qfont(*this);
    }

    std::string alternative() const
    {
        if ( index_ >= 0 && index_ < qfont_table.size() )
            return qfont_table[index_];
        return "";
    }

    uint32_t unicode_point() const
    {
        return 0xE000 | index_;
    }

private:
    /**
     * \brief Maps weird characters to ASCII strings
     */
    static std::vector<std::string> qfont_table;
    unsigned index_;
};

/**
 * \brief A formatted string
 */
class FormattedString
{
public:
    typedef std::vector<std::shared_ptr<const Element>> container;
    typedef container::value_type      value_type;
    typedef container::reference       reference;
    typedef container::const_reference const_reference;
    typedef container::iterator        iterator;
    typedef container::const_iterator  const_iterator;
    typedef container::difference_type difference_type;
    typedef container::size_type       size_type;

    /**
     * \brief Append an element (takes ownership)
     */
    void append ( Element* element )
    {
        elements.push_back(container::value_type{element});
    }

    void append ( const FormattedString& string )
    {
        elements.insert(elements.end(),string.begin(),string.end());
    }

    std::string encode(const std::string& format) const
    {
        Formatter* formatter = Formatter::formatter(format);
        std::string s;
        for ( const auto& e : elements )
            s += e->to_string(*formatter);
        return s;
    }

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

    iterator erase(const const_iterator& q)
    {
        return elements.erase(q);
    }
    iterator erase(const const_iterator& p, const const_iterator& q)
    {
        return elements.erase(p,q);
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

private:
    container elements;
};

/**
 * \brief Simple way to create FormattedString objects
 */
class FormattedStream
{
public:
    FormattedStream(const std::string& input_formatter)
        : formatter(Formatter::formatter(input_formatter)) {}

    explicit operator FormattedString() const { return buffer; }

    const FormattedStream& operator<< ( const std::string& text ) const
    {
        buffer.append(formatter->decode(text));
        return *this;
    }
    const FormattedStream& operator<< ( const color::Color12& color ) const
    {
        buffer.append(new Color(color));
        return *this;
    }
    const FormattedStream& operator<< ( const FormatFlags& format_flags ) const
    {
        buffer.append(new Format(format_flags));
        return *this;
    }
    const FormattedStream& operator<< ( char c ) const
    {
        buffer.append(new Character(c));
        return *this;
    }
    template <class T>
        const FormattedStream& operator<< ( const T& obj ) const
        {
            std::ostringstream ss;
            ss << obj;
            buffer.append(formatter->decode(ss.str()));
            return *this;
        }

    std::string encode(const std::string& output_formatter) const
    {
        return buffer.encode(output_formatter);
    }

private:
    mutable FormattedString buffer;
    Formatter* formatter;
};

/**
 * \brief UTF-8 (Plain, or with ANSI color)
 */
class FormatterUtf8 : public Formatter
{
public:
    explicit FormatterUtf8 ( bool colors ) : colors ( colors ) {}

    std::string ascii(char c) const override;
    std::string color(const color::Color12& color) const override;
    std::string format_flags(FormatFlags flags) const override;
    std::string unicode(const Unicode& c) const override;
    std::string qfont(const QFont& c) const override;
    FormattedString decode(const std::string& source) const override;
    std::string name() const override;

protected:
    bool colors;
};

/**
 * \brief ASCII (Plain, or with ANSI color)
 */
class FormatterAscii : public FormatterUtf8
{
public:
    using FormatterUtf8::FormatterUtf8;

    std::string unicode(const Unicode& c) const override;
    FormattedString decode(const std::string& source) const override;
    std::string name() const override;
};

/**
 * \brief UTF-8 with IRC colors
 */
class FormatterIrc : public FormatterUtf8
{
public:
    FormatterIrc() : FormatterUtf8(false) {}

    std::string color(const color::Color12& color) const override;
    std::string format_flags(FormatFlags flags) const override;
    FormattedString decode(const std::string& source) const override;
    std::string name() const override;
};

/**
 * \brief Darkplaces UTF-8
 */
class FormatterDarkplaces : public Formatter
{
public:
    std::string ascii(char c) const override;
    std::string color(const color::Color12& color) const override;
    std::string format_flags(FormatFlags flags) const override;
    std::string unicode(const Unicode& c) const override;
    std::string qfont(const QFont& c) const override;
    FormattedString decode(const std::string& source) const override;
    std::string name() const override;
};

} // namespace string
#endif // STRING_HPP
