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
 *
 */

#include "library.hpp"

#include <dlfcn.h>

namespace melanolib {
namespace library {

class Library::Private
{
public:
    void* handle = nullptr;
    const char* error_string = nullptr;
    std::string filename;

    void gather_error()
    {
        error_string = dlerror();
    }

    void close()
    {
        dlclose(handle);
    }

    void open(LoadFlags flags)
    {
        bool throws = flags & LoadThrows;
        flags &= ~LoadThrows;
        handle = dlopen(filename.c_str(), flags);
        if ( !handle )
        {
            gather_error();
            if ( throws )
                throw LibraryError(filename, error_string);
        }
    }

    void* resolve(const std::string& symbol)
    {
        auto ret = dlsym(handle, symbol.c_str());
        if ( !ret )
            gather_error();
        return ret;
    }
};

Library::Library(const std::string& library_file, LoadFlags flags)
    : p(std::make_shared<Private>())
{
    p->filename = library_file;
    p->open(flags);
}

Library::~Library()
{
    if ( p.unique() )
        p->close();
}

bool Library::error() const
{
    return !p->handle || p->error_string;
}

std::string Library::error_string() const
{
    return p->error_string;
}

std::string Library::filename() const
{
    return p->filename;
}

void* Library::resolve_raw(const std::string& name) const
{
    return p->resolve(name);
}


void Library::reload(library::LoadFlags flags) const
{
    p->close();
    p->open(flags);
}

} // namespace library
} // namespace melanolib
