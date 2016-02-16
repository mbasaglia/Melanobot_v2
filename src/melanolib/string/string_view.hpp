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
#ifndef MELANOLIB_STRING_VIEW_HPP
#define MELANOLIB_STRING_VIEW_HPP

#include <iterator>
#include <type_traits>
#include "melanolib/c++-compat.hpp"

namespace melanolib {

template<class T>
    class basic_string_view
{
public:
    using value_type = T;
    using reference = value_type&;
    using pointer = value_type*;
    using iterator = value_type*;
    using const_iterator = const value_type*;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using difference_type = typename std::iterator_traits<iterator>::difference_type;
    using size_type = std::size_t;
    using string_type = std::basic_string<std::remove_cv_t<value_type>>;

    constexpr basic_string_view(pointer begin, pointer end)
        : begin_(begin), end_(end)
    {
    }

    constexpr basic_string_view(pointer begin, size_type size)
        : begin_(begin), end_(begin+size)
    {
    }

    constexpr basic_string_view() noexcept
        : begin_(nullptr), end_(nullptr)
    {
    }

    template<std::size_t size>
        constexpr basic_string_view(value_type (&array)[size]) noexcept
        :  basic_string_view(array, size > 0 && array[size-1] == '\0' ? size-1 : size)
    {}

    template<class Iterator, class =
        typename std::enable_if<
            std::is_same<typename Iterator::value_type, value_type>::value
        >::type>
    constexpr basic_string_view(const Iterator& begin, const Iterator& end)
        : basic_string_view(&*begin, &*end)
    {
    }

    template<class Container, class =
        typename std::enable_if_t<
            std::is_same<typename Container::value_type, value_type>::value &&
            !std::is_same<std::remove_cv_t<Container>, basic_string_view>::value
        > >
    constexpr basic_string_view(Container& container)
        : basic_string_view(container.data(), container.size())
    {
    }

    basic_string_view(const basic_string_view& oth) noexcept = default;
    basic_string_view(basic_string_view&& oth) noexcept = default;

    basic_string_view& operator=(const basic_string_view& oth) noexcept = default;
    basic_string_view& operator=(basic_string_view&& oth) noexcept = default;

    constexpr size_type size() const noexcept
    {
        return end_ - begin_;
    }

    constexpr bool empty() const noexcept
    {
        return end_ == begin_;
    }

    constexpr reference operator[] (size_type i) const
    {
        return begin_[i];
    }

    constexpr explicit operator bool() const noexcept
    {
        return begin_;
    }

    constexpr iterator begin() const noexcept
    {
        return begin_;
    }

    constexpr iterator end() const noexcept
    {
        return end_;
    }

    constexpr const_iterator cbegin() const noexcept
    {
        return begin_;
    }

    constexpr const_iterator cend() const noexcept
    {
        return end_;
    }

    constexpr const_iterator rbegin() const
    {
        return reverse_iterator(end_);
    }

    constexpr const_iterator rend() const
    {
        return reverse_iterator(begin_);
    }

    constexpr const_reverse_iterator crbegin() const
    {
        return reverse_iterator(cbegin());
    }

    constexpr const_reverse_iterator crend() const
    {
        return reverse_iterator(cbegin());
    }

    constexpr bool operator== (const basic_string_view& rhs) const noexcept
    {
        return begin_ == rhs.begin_ && end_ == rhs.end_;
    }

    constexpr bool operator!= (const basic_string_view& rhs) const noexcept
    {
        return !(*this == rhs);
    }

    SUPER_CONSTEXPR bool strequal(const basic_string_view& rhs) const noexcept
    {
        if ( size() != rhs.size() )
            return false;

        for( size_type i = 0; i < size(); i++ )
        {
            if ( begin_[i] != rhs[i] )
                return false;
        }

        return true;
    }

    string_type str() const
    {
        return string_type(begin_, end_);
    }

private:
    T* begin_;
    T* end_;
};

using string_view = basic_string_view<char>;
using cstring_view = basic_string_view<const char>;

} // namespace melanolib
#endif // MELANOLIB_STRING_VIEW_HPP
