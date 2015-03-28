/**
 * \file
 * \brief This file defined some compatibility for features of C++14 and beyond
 * \todo any and optional
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

#endif // CXX_COMPAT_HPP
