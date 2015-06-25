/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015 Mattia Basaglia
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


} // namespace storage
