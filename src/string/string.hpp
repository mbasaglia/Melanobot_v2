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
#include "melanobot/error.hpp"

/**
 * \brief Namespace for string formatting
 */
namespace string {


using ReplacementFunctor = std::function<
    melanolib::Optional<class FormattedString> (const std::string&)
    >;

/**
 * \brief Namespace with templated functions for Element objects
 *
 * They can be specialized to add more types
 */
namespace element {

    template<class T>
        void replace(T& subject, const ReplacementFunctor& repl)
        {
        }


    /**
     * \brief Conversion template for values that can be converted to a string
     *        using streams.
     */
    template<class T>
        std::enable_if_t<melanolib::StreamInsertable<T>::value, std::string>
        to_string(const Formatter& formatter, const T& t, Formatter::Context* context)
    {
        std::ostringstream ss;
        ss << t;
        return formatter.to_string(ss.str(), context);
    }

    /**
     * \brief Conversion template for values that need explicit conversions
     */
    template<class T>
        std::enable_if_t<!melanolib::StreamInsertable<T>::value, std::string>
        to_string(const Formatter& formatter, const T& t, Formatter::Context* context)
    {
        return "";
    }

} // namespace element

/**
 * \brief Private namespace with overloads that send the right types to the right functions
 */
namespace detail {

    class OveloadTag{};

    /**
     * \brief Calls replace() on types that implement that function
     */
    template<class T>
        auto replace_dispatch(T& subject, const ReplacementFunctor& repl, OveloadTag) -> decltype(subject.replace(repl))
        {
            return subject.replace(repl);
        }

    /**
     * \brief Fallback
     */
    template<class T>
    void replace_dispatch(T& subject, const ReplacementFunctor& repl, ...)
    {
        element::replace(subject, repl);
    }



    /**
     * \brief Calls the right virtual function
     */
    template<class T>
        auto to_string_dispatch(const Formatter& formatter, T&& t,
                                Formatter::Context* context, OveloadTag)
        -> decltype(formatter.to_string(std::forward<T>(t), context))
    {
        return formatter.to_string(std::forward<T>(t), context);
    }

    /**
     * \brief Calls a member to_string
     */
    template<class T>
        auto to_string_dispatch(const Formatter& formatter, T&& t,
                                Formatter::Context* context, OveloadTag)
        -> decltype(t.to_string(formatter))
    {
        return t.to_string(formatter);
    }

    /**
     * \brief Calls a member to_string
     */
    template<class T>
        auto to_string_dispatch(const Formatter& formatter, T&& t,
                                Formatter::Context* context, OveloadTag)
        -> decltype(t.to_string(formatter, context))
    {
        return t.to_string(formatter, context);
    }

    /**
     * Fallback
     */
    template<class T>
        std::string to_string_dispatch(const Formatter& formatter, T&& t,
                                       Formatter::Context* context, ...)
        {
            return element::to_string(formatter, t, context);
        }

} // namespace detail

/**
 * \brief Type-erased element of a string
 */
class Element
{
private:
    struct HolderBase
    {
        virtual ~HolderBase() {}
        virtual std::string to_string(const Formatter&, Formatter::Context*) const = 0;
        virtual std::unique_ptr<HolderBase> clone() const = 0;
        virtual const std::type_info& type() const noexcept = 0;
        virtual void replace(const ReplacementFunctor& repl) = 0;
    };

    template<class T>
        struct Holder : HolderBase
        {
            Holder(T object) : object(std::move(object)) {}

            std::string to_string(const Formatter& visitor, Formatter::Context* context) const override
            {
                return detail::to_string_dispatch(visitor, object, context, detail::OveloadTag{});
            }

            void replace(const ReplacementFunctor& repl) override
            {
                detail::replace_dispatch(object, repl, detail::OveloadTag{});
            }

            std::unique_ptr<HolderBase> clone() const override
            {
                return melanolib::New<Holder<T>>(object);
            }

            const std::type_info& type() const noexcept override
            {
                return typeid(T);
            }

            T object;
        };


    template<class T, class SFINAE = void>
        struct HolderTraits;

    template<class T>
        using HolderType = Holder<typename HolderTraits<T>::HeldType>;

public:
    template<class T>
        using HeldType = typename HolderTraits<T>::HeldType;

    template<class T>
    explicit Element(T&& object)
        : holder( melanolib::New<HolderType<T>>(std::forward<T>(object)) )
        {}

    Element(Element&&) noexcept = default;
    Element& operator= (Element&&) noexcept = default;


    Element(Element& oth) : holder(oth.holder->clone()) {}
    Element(const Element& oth) : holder(oth.holder->clone()) {}

    Element& operator=(const Element& oth) noexcept
    {
        holder = oth.holder->clone();
        return *this;
    }

    std::string to_string(const Formatter& formatter, Formatter::Context* context) const
    {
        return holder->to_string(formatter, context);
    }

    void replace(const ReplacementFunctor& repl)
    {
        holder->replace(repl);
    }

    Element clone() const
    {
        return Element(holder->clone());
    }

    template<class T>
        HeldType<T> value() const
    {
        if ( has_type<T>() )
            return static_cast<const HolderType<T>*>(holder.get())->object;
        throw melanobot::MelanobotError("Bad element cast");
    }

    template<class T>
        HeldType<T>& reference()
    {
        if ( has_type<T>() )
            return static_cast<HolderType<T>*>(holder.get())->object;
        throw melanobot::MelanobotError("Bad element cast");
    }

    template<class T>
        const HeldType<T>& reference() const
    {
        if ( has_type<T>() )
            return static_cast<const HolderType<T>*>(holder.get())->object;
        throw melanobot::MelanobotError("Bad element cast");
    }

    template<class T>
        bool has_type() const noexcept
    {
        return holder->type() == typeid(HeldType<T>);
    }

private:
    explicit Element(std::unique_ptr<HolderBase>&& holder)
        : holder(std::move(holder))
        {}

    std::unique_ptr<HolderBase> holder;
};

/**
 * \brief Base case, a holder contains \p T
 */
template<class T>
    struct Element::HolderTraits<T,
        std::enable_if_t<!melanolib::CanDecay<T>::value &&
                         !melanolib::StringConvertible<T>::value>
    >
    {
        using HeldType = T;
    };

/**
 * \brief cv-qualified, reference or array fall back to decayed type
 */
template<class T>
    struct Element::HolderTraits<T,
        std::enable_if_t<melanolib::CanDecay<T>::value &&
                         !melanolib::StringConvertible<T>::value>
    >
        : HolderTraits<std::remove_cv_t<std::decay_t<T>>>
    {};

/**
 * \brief \c FormatFlags::FormatFlagsEnum uses \c FormatFlags
 */
template<>
    struct Element::HolderTraits<FormatFlags::FormatFlagsEnum>
    {
        using HeldType = FormatFlags;
    };

/**
 * \brief Anything that can be implicitly converted to \c std::string, does so
 */
template<class T>
    struct Element::HolderTraits<T, std::enable_if_t<melanolib::StringConvertible<T>::value>>
    {
        using HeldType = std::string;
    };
/**
 * \brief A formatted string
 */
class FormattedString
{
public:

    using value_type        = Element;
    using container         = std::vector<value_type>;
    using reference         = container::reference;
    using const_reference   = container::const_reference;
    using iterator          = container::iterator;
    using const_iterator    = container::const_iterator;
    using difference_type   = container::difference_type;
    using size_type         = container::size_type;

    template<class Iterator>
        FormattedString( const Iterator& i, const Iterator& j )
            : elements(i, j) {}

    FormattedString(AsciiString ascii_string)
    {
        if ( !ascii_string.empty() )
            append(std::move(ascii_string));
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
     * \brief Append an element
     */
    template<class T>
        std::enable_if_t<
            !std::is_same<Element::HeldType<T>, FormattedString>::value &&
            !std::is_same<Element::HeldType<T>, Element>::value
        >
        append(T&& element)
        {
            elements.emplace_back(std::forward<T>(element));
        }

    void append(value_type element)
    {
        elements.push_back(std::move(element));
    }


    void append(const FormattedString& string)
    {
        elements.reserve(size() + string.size());
        for ( const auto& element : string )
            elements.emplace_back(element.clone());
    }

    /**
     * \brief Encode the string using the given formatter
     */
    std::string encode(const Formatter& formatter) const
    {
        auto context = formatter.context();
        return encode(formatter, context.get());
    }

    std::string encode(const Formatter& formatter, Formatter::Context* context) const
    {
        std::string s = formatter.string_begin(context);
        for ( const auto& e : elements )
            s += e.to_string(formatter, context);
        return s + formatter.string_end(context);
    }

    /**
     * \brief Deep copy
     */
    FormattedString copy() const
    {
        FormattedString c;
        c.elements.reserve(elements.size());
        for ( const auto& element : *this )
            c.elements.emplace_back(element.clone());
        return c;
    }

    /**
     * \brief Replace placeholders based on a functor
     */
    void replace(const ReplacementFunctor& func)
    {
        for ( auto& element : elements )
            element.replace(func);
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

// Stream-like operations
    template <class T>
        FormattedString& operator<<(T&& obj) &
        {
            append(std::forward<T>(obj));
            return *this;
        }

    template <class T>
        FormattedString&& operator<<(T&& obj) &&
        {
            append(std::forward<T>(obj));
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


using FormattedProperties = std::unordered_map<std::string, FormattedString>;

} // namespace string

#endif // STRING_HPP
