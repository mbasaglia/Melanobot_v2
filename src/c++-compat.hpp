/**
 * \file
 * \brief This file defined some compatibility for features of C++14 and beyond
 * \author Mattia Basaglia
 * \copyright Copyright  Mattia Basaglia
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
#ifndef CXX_COMPAT_HPP
#define CXX_COMPAT_HPP


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
        using Optional = std::optional<T>;
#  elif __has_include(<experimental/optional>)
#    include <experimental/optional>
    template<class T>
        using Optional = std::experimental::optional<T>;
#  elif __has_include(<boost/optional.hpp>)*/
#  if __has_include(<boost/optional.hpp>)
#    include <boost/optional.hpp>
    template<class T>
        using Optional = boost::optional<T>;
#  else
#     error "Missing <optional>"
#  endif
// any
#if __has_include(<optional>)
#    include<any>
     using Any = std::any;
#  elif __has_include(<experimental/any>)
#    include <experimental/any>
    using Any = std::experimental::any;
#  elif __has_include(<boost/any.hpp>)
#    include <boost/any.hpp>
    using Any = boost::any;
#  else
#     error "Missing <any>"
#  endif
#endif // __has_include

#endif // CXX_COMPAT_HPP
