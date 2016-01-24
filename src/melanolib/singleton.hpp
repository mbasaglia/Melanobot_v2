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
#ifndef MELANOLIB_SINGLETON_HPP
#define MELANOLIB_SINGLETON_HPP
namespace melanolib {

/**
 * \brief Curiously recursive base to quickly implement a singleton
 *
 * All you have to do is implement a private default constructor
 * and befriend ParentSingleton
 */
template<class Derived>
    class Singleton
{
public:
    /**
     * \brief Returns the singleton instance
     */
    static Derived& instance()
    {
        static Derived singleton;
        return singleton;
    }

protected:
    using ParentSingleton = Singleton;
    Singleton(){}

private:
    Singleton(const Singleton&) = delete;
    Singleton(Singleton&&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    Singleton& operator=(Singleton&&) = delete;
};

} // namespace melanolib
#endif // MELANOLIB_SINGLETON_HPP
