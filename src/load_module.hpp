/**
 * \file
 *
 * \author Mattia Basaglia
 *
 * \copyright Copyright (C) 2015 Mattia Basaglia
 *
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
#ifndef MELANOBOT_LOAD_MODULE_HPP
#define MELANOBOT_LOAD_MODULE_HPP

#include "melanomodule.hpp"

namespace module {

std::vector<module::Melanomodule> find_modules(const std::vector<std::string>& paths);

/// \todo Load from multiple paths
template<class... InitArgs>
    std::vector<module::Melanomodule>
        initialize_modules(const std::vector<std::string>& paths, InitArgs&&... init_args)
{
    using namespace melanolib::library;
    std::vector<module::Melanomodule> modules = find_modules(paths);

    if ( modules.empty() )
        return {};

    std::vector<module::Melanomodule> loaded_modules;
    Log("sys",'!') << "Loading modules";
    for ( const auto& mod : modules )
    {
        try
        {
            mod.library->reload(ExportGlobal|LoadNow|LoadThrows);
            mod.library->call_function<void, InitArgs...>(
                "melanomodule_"+mod.name+"_initialize",
                std::forward<InitArgs>(init_args)...
            );
            loaded_modules.push_back(mod);
            Log("sys",'!') << "\tLoaded module "
                            << mod.name << ' ' << mod.version;
        }
        catch ( const LibraryError& error )
        {
            ErrorLog errlog("sys","Module Error");
            if ( settings::global_settings.get("debug", 0) )
                errlog << error.library_file << ": ";
            errlog  << error.what();
        }

    }

    return loaded_modules;
}

} // namespace module
#endif // MELANOBOT_LOAD_MODULE_HPP
