/**
 * \file
 *
 * \author Mattia Basaglia
 *
 * \copyright Copyright (C) 2015 Mattia Basaglia
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

} // namespace melanolib
#endif // MELANOLIB_UTILS_HPP
