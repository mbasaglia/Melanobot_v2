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
#ifndef MELANOLIB_MATH_HPP
#define MELANOLIB_MATH_HPP

#include <algorithm>

namespace melanolib {

/**
 * \brief Namespace for math utilities
 */
namespace math {

/**
 * \brief Get a uniform random integer
 */
long random();

/**
 * \brief Get a uniform random integer between 0 and \c max (inclusive)
 */
long random(long max);

/**
 * \brief Get a uniform random integer between \c min and \c max (inclusive)
 */
long random(long min, long max);

/**
 * \brief Get a uniform random number between 0 and 1
 */
double random_real();

/**
 * \brief Truncates a number
 * \tparam Return   Return type (Must be an integral type)
 * \tparam Argument Argument type (Must be a floating point type)
 */
template<class Return=int, class Argument=double>
    constexpr Return truncate(Argument x)
    {
        return Return(x);
    }

/**
 * \brief Round a number (constexpr-friendly)
 * \tparam Return   Return type (Must be an integral type)
 * \tparam Argument Argument type (Must be a floating point type)
 */
template<class Return=int, class Argument=double>
    constexpr Return round(Argument x)
    {
        return truncate<Return>(x+Argument(0.5));
    }

/**
 * \brief Get the fractional part of a floating-point number
 * \tparam Argument Argument type (Must be a floating point type)
 */
template<class Argument>
    constexpr auto fractional(Argument x)
    {
        return x - truncate<long long>(x);
    }

/**
 * \brief Clamp a value inside a range
 * \tparam Argument Argument type (Must be a floating point type)
 * \param min Minimum allowed value
 * \param x   Variable to be bounded
 * \param max Maximum allowed value
 */
template<class Argument, class Arg2=Argument>
    constexpr auto bound(Arg2 min, Argument x, Arg2 max)
    {
        return x < max ?  std::max<Argument>(x,min) : max;
    }

} // namespace math
} // namespace melanolib

#endif // MELANOLIB_MATH_HPP
