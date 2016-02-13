/**
 * \file
 *
 * \author Mattia Basaglia
 *
 * \copyright Copyright 2015-2016 Mattia Basaglia
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef MELANOLIB_UTILS_HPP
#define MELANOLIB_UTILS_HPP

#include <iosfwd>
#include <tuple>
#include <type_traits>

namespace melanolib {

/**
 * \brief Template to retrieve information about a function signature
 *
 * Use as FunctionSignature<Ret (Args...)> or FunctionSignature<Pointer>
 */
template<class T>
    struct FunctionSignature;

template<class Ret, class...Args>
    struct FunctionSignature<Ret(Args...)>
    {
        using pointer_type = Ret (*) (Args...);
        using return_type = Ret;
        using arguments_types = std::tuple<Args...>;
    };

template<class Ret, class...Args>
    struct FunctionSignature<Ret(*)(Args...)>
        : public FunctionSignature<Ret(Args...)>
    {
    };

/**
 * \brief Clean syntax to get a function pointer type
 */
template<class T>
    using FunctionPointer = typename FunctionSignature<T>::pointer_type;

namespace detail {
    /**
     * \brief Type used to detect the operators defined in this namespace
     */
    class WrongOverload{};

    /**
     * \brief Type to which anything can be conveted
     */
    struct Gobble{ template<class T> Gobble(T&&); };

    /**
     * \brief Adds overload that if matched, means there is no better alternative
     */
    WrongOverload operator<< (std::ostream&, Gobble);

    /**
     * \brief Type that inherits from \c std::true_type or \c std::false_type
     *        based on whether \p T has a proper stream operator<<.
     * \note Defined in this namespace to have visibility to the \c Gobble overload.
     */
    template<class T>
    class StreamInsertable : public
        std::integral_constant<bool,
            !std::is_same<
                decltype(std::declval<std::ostream&>() << std::declval<T>()),
                WrongOverload
            >::value
        >
    {};
}

using detail::StreamInsertable;

/**
 * \brief Type that inherits from \c std::true_type or \c std::false_type
 *        based on whether std::decay_t<T> would do something
 */
template<class T>
    struct CanDecay : std::integral_constant<bool,
            std::is_reference<T>::value ||
            std::is_const<T>::value     ||
            std::is_volatile<T>::value  ||
            std::is_array<T>::value     >
    {};

/**
 * \brief Type that inherits from \c std::true_type or \c std::false_type
 *        based on whether \p T can be converted to \c std::string.
 */
template<class T>
    struct StringConvertible : std::is_convertible<T, std::string>
    {};

} // namespace melanolib
#endif // MELANOLIB_UTILS_HPP
