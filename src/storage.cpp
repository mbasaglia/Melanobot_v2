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
#include "storage.hpp"

#include "settings.hpp"

namespace storage {

/**
 * \brief Object used by storage() and set_storage()
 */
static std::unique_ptr<StorageBase> storage_pointer;

StorageBase& storage()
{
    if ( storage_pointer )
        return *storage_pointer;
    throw ConfigurationError("Storage system not initialized");
}

void set_storage(std::unique_ptr<StorageBase>&& pointer)
{
    if ( storage_pointer )
        throw ConfigurationError("Storage system already initialized");
    storage_pointer = std::move(pointer);
}

bool has_storage()
{
    return bool(storage_pointer);
}

std::unique_ptr<StorageBase> StorageFactory::create(const Settings& settings) const
{
    auto name = settings.get_optional<std::string>("type");
    if ( name )
    {
        auto it = constructors.find(*name);
        if ( it != constructors.end() )
            return it->second(settings);
    }

    return {};
}

void StorageFactory::initilize_global_storage(const Settings& settings) const
{
    set_storage(create(settings));
}

void StorageFactory::register_type(const std::string& name, const Constructor& ctor)
{
    if ( constructors.count(name) )
        throw Error(name+" is already a registered type of storage");

    constructors[name] = ctor;
}


} // namespace storage
