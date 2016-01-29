/**
 * \file
 *
 * \author Mattia Basaglia
 *
 * \copyright Copyright 2015-2016 Mattia Basaglia
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
 *
 */

#include "module/melanomodule.hpp"

#include <boost/filesystem.hpp>
#include <boost/range/istream_range.hpp>
#include <regex>

using namespace melanolib::library;

namespace module {

void gather_metadata(const std::string& search_path,
                     std::vector<module::Melanomodule>& modules)
{
    static std::regex regex(R"re(lib(melanomodule_(?:-|[_.a-zA-Z0-9])+).so)re",
                            std::regex::optimize|std::regex::ECMAScript);

    if ( !boost::filesystem::exists(search_path) )
        return;

    using diriter = boost::filesystem::directory_iterator;
    boost::iterator_range<diriter> range{diriter(search_path), diriter{}};

    for ( const auto& entry : range )
    {
        if ( !boost::filesystem::is_regular_file(entry) )
            continue;
        std::smatch match;
        std::string basename = entry.path().filename().string();
        if ( std::regex_match(basename, match, regex) )
        {
            Log("sys",'!',4) << "\tLoading library " << entry;
            try
            {
                Library lib(entry.path().native(),
                            ExportLocal|LoadLazy|NoUnload|LoadThrows);
                auto module = lib.call_function<module::Melanomodule>(
                    match[1].str()+"_metadata");
                module.library = lib;
                modules.push_back(module);
                Log("sys",'!',3) << "\tFound module "
                                << module.name << ' ' << module.version;
            }
            catch ( const LibraryError& error )
            {
                ErrorLog errlog("sys","Module Error");
                if ( settings::global_settings.get("debug", 0) )
                    errlog << error.library_file << ": ";
                errlog  << error.what();
            }
        }
    }
}

void filter_dependencies(std::vector<module::Melanomodule>& modules)
{
    std::vector<module::Melanomodule> loaded_modules;
    loaded_modules.reserve(modules.size());
    while ( !modules.empty() )
    {
        auto size = modules.size();

        auto it = modules.begin();
        while ( it != modules.end() )
        {
            if ( it->dependencies_satisfied(loaded_modules) )
            {
                loaded_modules.push_back(*it);
                it = modules.erase(it);
            }
            else
            {
                ++it;
            }
        }

        if ( modules.size() == size )
        {
            Log log("sys",'!');
            log << "The following modules have unsatisfied dependencies:";
            for ( const auto& mod : modules )
            {
                log << ' ' << mod.name << '(' << mod.version << ')';
            }
            break;
        }
    }

    std::swap(loaded_modules, modules);
}

void filter_deprecation(std::vector<module::Melanomodule>& modules)
{
    std::sort(modules.begin(), modules.end(), module::Melanomodule::lexcompare);

    auto it = std::unique( modules.begin(), modules.end(),
        [](const auto& a, const auto& b) { return a.name == b.name; });

    if ( it != modules.end() )
    {
        Log log("sys",'!');
        log << "The following modules are deprecated:";
        for( auto i = it; i != modules.end(); ++i )
        {
            log << ' ' << i->name << '(' << i->version << ')';
        }
        modules.erase(it, modules.end());
    }
}

std::vector<module::Melanomodule> find_modules(const std::vector<std::string>& paths)
{
    if ( paths.empty() )
        return {};

    Log("sys",'!',3) << "Searching for modules";
    std::vector<module::Melanomodule> modules;
    for ( const auto& path : paths )
        gather_metadata(path, modules);
    filter_dependencies(modules);
    filter_deprecation(modules);
    return modules;
}

} // namespace module
