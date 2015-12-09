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
#ifndef MELANOBOT_LIBRARY_HPP
#define MELANOBOT_LIBRARY_HPP

#include <string>
#include <stdexcept>
#include <memory>

#include "utils.hpp"

namespace library {

using LoadFlags = int;

enum LoadFlagsEnum
{
    LoadLazy    = 0x0001, ///< Resolve symbols only when needed
    LoadNow     = 0x0002, ///< Resolve symbols when the library is loaded
    ExportGlobal= 0x0100, ///< Make symbols in the library available to other libraries
    ExportLocal = 0x0000, ///< Don't make symbols in the library available to other libraries
    DeepBind    = 0x0008, ///< Prefer library symbol definitions over clashing global symbols
    NoUnload    = 0x1000, ///< Don't unload the library when it's closed
};

struct SymbolNotFoundError : public std::runtime_error
{
    SymbolNotFoundError(const std::string& symbol, const std::string& library)
        : runtime_error(library + ": " + "could not resolve \"" + symbol + '\"')
    {}
};

/**
 * \brief Class representing a dynamic library, loaded at runtime
 */
class Library
{
public:
    static constexpr LoadFlags default_load_flags = LoadLazy|ExportLocal|DeepBind;

    /**
     * \brief Loads the given library
     */
    explicit Library(const std::string& library_file, library::LoadFlags flags = default_load_flags);

    /**
     * \brief Closes the library
     */
    ~Library();

    /**
     * \brief Name of the file this library has been loaded from
     */
    std::string filename() const;

    /**
     * \brief Whether an error has occurred
     */
    bool error() const;

    /**
     * \brief The error message for the latest error
     * \pre error() returns true
     */
    std::string error_string() const;

    /**
     * \brief True if there is no error
     */
    explicit operator bool() const
    {
        return !error();
    }

    /**
     * \brief Resolves a global variable returns it as a reference
     * \throws SymbolNotFoundError if \p name cannot be resolved
     */
    template<class T>
        T& resolve_global(const std::string& name) const
        {
            if ( void* ptr = resolve_raw(name) )
                return *reinterpret_cast<T>(ptr);
            throw SymbolNotFoundError(name, filename());
        }

    /**
     * \brief Resolves a function and returns it as a function pointer
     * \throws SymbolNotFoundError if \p name cannot be resolved
     */
    template<class T>
        utils::FunctionPointer<T> resolve_function(const std::string& name) const
        {
            if ( void* ptr = resolve_raw(name) )
                return reinterpret_cast<utils::FunctionPointer<T>>(ptr);
            throw SymbolNotFoundError(name, filename());
        }

    /**
     * \brief Resolves a function and calls it with the given arguments
     * \throws SymbolNotFoundError if \p name cannot be resolved
     */
    template<class Ret, class... Args>
        Ret call_function(const std::string& name, Args&&... args) const
        {
            auto func = resolve_function<Ret(Args...)>(name);
            return func(std::forward<Args>(args)...);
        }

private:
    /**
     * \brief Resolves a symbol and returns it as a void pointer
     * \returns \b nullptr if \p name cannot be resolved
     */
    void* resolve_raw(const std::string& name)  const;

    class Private;
    std::shared_ptr<Private> p;

};

} // namespace library
#endif // MELANOBOT_LIBRARY_HPP
