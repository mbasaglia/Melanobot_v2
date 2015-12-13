/**
 * \file
 * \brief This file defined some compatibility for features of C++14 and beyond
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
#ifndef MELANOLIB_CXX_COMPAT_HPP
#define MELANOLIB_CXX_COMPAT_HPP


#if defined(__cpp_constexpr) && __cpp_constexpr >= 201304
#  define SUPER_CONSTEXPR constexpr
#else
#  define SUPER_CONSTEXPR
#endif

#ifdef __has_include
// optional
/*#if __has_include(<optional>)
#    include<optional>
    template<class T>
        using melanolib::Optional = std::optional<T>;
#  elif __has_include(<experimental/optional>)
#    include <experimental/optional>
    template<class T>
        using melanolib::Optional = std::experimental::optional<T>;
#  elif __has_include(<boost/optional.hpp>)*/
#  if __has_include(<boost/optional.hpp>)
#    include <boost/optional.hpp>
    namespace melanolib {
        template<class T>
            using Optional = boost::optional<T>;
    } // namespace melanolib
#  else
#     error "Missing <optional>"
#  endif
// any
#if __has_include(<any>)
#    include<any>
    namespace melanolib {
        using Any = std::any;
    } // namespace melanolib
// #  elif __has_include(<experimental/any>)
// #    include <experimental/any>
//     using Any = std::experimental::any;
#  elif __has_include(<boost/any.hpp>)
#    include <boost/any.hpp>
    namespace melanolib {
        using Any = boost::any;
    } // namespace melanolib
#  else
#     error "Missing <any>"
#  endif
#endif // __has_include

#include <memory>

namespace melanolib {

/**
 * \brief Just a shorter version of std::make_unique
 */
template<class T, class... Args>
auto New (Args&&... args)
{
    return std::make_unique<T>(std::forward<Args>(args)...);
}

} // namespace melanolib

#endif // MELANOLIB_CXX_COMPAT_HPP
