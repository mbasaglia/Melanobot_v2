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

#include "dyn_library.hpp"

#include <dlfcn.h>

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
};

Library::Library(const std::string& library_file, LoadFlags flags)
    : p(std::make_shared<Private>())
{
    p->filename = library_file;
    p->handle = dlopen(library_file.c_str(), flags);
    if ( !p->handle )
        p->gather_error();
}

Library::~Library()
{
    if ( p.unique() && p->handle )
        dlclose(p->handle);
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
    auto ret = dlsym(p->handle, name.c_str());
    if ( !ret )
        p->gather_error();
    return ret;
}

} // namespace library
