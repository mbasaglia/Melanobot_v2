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
#include <type_traits>

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
 * \brief Maximum between two values
 */
template<class T>
    inline constexpr T max(T&& a, T&& b)
    {
        return a < b ? b : a;
    }

/**
 * \brief Maximum among several values
 */
template<class T, class...Ts>
    inline constexpr T max(T&& a,  Ts&&... b)
    {
        return max(std::forward<T>(a), max(std::forward<Ts>(b)...));
    }

/**
 * \brief Minimum between two values
 */
template<class T>
    inline constexpr T min(T&& a, T&& b)
    {
        return !(b < a) ? a : b;
    }

/**
 * \brief Minimum among several values
 */
template<class T, class...Ts>
    inline constexpr T min(T&& a,  Ts&&... b)
    {
        return min(std::forward<T>(a), min(std::forward<Ts>(b)...));
    }

/**
 * \brief Absolute value
 */
template<class T>
    inline constexpr T abs(T&& x)
    {
        return x < 0 ? -x : x;
    }

/**
 * \brief Normalize a value
 * \pre  value in [min, max] && min < max
 * \post value in [0, 1]
 */
template<class Real>
    inline constexpr Real normalize(Real value, Real min, Real max)
{
    return (value - min) / (max - min);
}

/**
 * \brief Denormalize a value
 * \pre  value in [0, 1] && min < max
 * \post value in [min, max]
 */
template<class Real>
    inline constexpr Real denormalize(Real value, Real min, Real max)
{
    return value * (max - min) + min;
}

/**
 * \brief Clamp a value inside a range
 * \tparam Argument Argument type (Must be a floating point type)
 * \param min_value Minimum allowed value
 * \param value     Variable to be bounded
 * \param max_value Maximum allowed value
 * \pre min_value < max_value
 * \post value in [min_value, max_value]
 */
template<class Argument>
    constexpr auto bound(Argument&& min_value, Argument&& value, Argument&& max_value)
    {
        return max(std::forward<Argument>(min_value),
            min(std::forward<Argument>(value), std::forward<Argument>(max_value))
        );
    }

template<class Argument, class Arg2, class = std::enable_if_t<!std::is_same<Argument, Arg2>::value>>
    constexpr auto bound(Arg2&& min_value, Argument&& value, Arg2&& max_value)
    {
        using Common = std::common_type_t<Arg2, Argument>;
        return bound<Common>(
            Common(std::forward<Arg2>(min_value)),
            Common(std::forward<Argument>(value)),
            Common(std::forward<Arg2>(max_value))
        );
    }

} // namespace math
} // namespace melanolib

#endif // MELANOLIB_MATH_HPP
