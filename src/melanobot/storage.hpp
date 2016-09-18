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
#ifndef STORAGE_BASE_HPP
#define STORAGE_BASE_HPP

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>

#include "error.hpp"
#include "settings.hpp"

namespace melanobot {

/**
 * \brief Error arising from StorageBase function calls
 */
class StorageError : public MelanobotError
{
public:
    using MelanobotError::MelanobotError;
};

class StorageBase
{
public:
    using key_type      = std::string; ///< Type used to identify values
    using value_type    = std::string; ///< Type used to represent values
    using sequence      = std::vector<value_type>; ///< Sequence of value
    /// Associative container for key/value
    using table         = std::unordered_map<key_type, value_type>;

    virtual ~StorageBase() {}

    /**
     * \brief Get the value at the given path
     * \throws Error if the path doesn't exist
     */
    virtual value_type get_value(const key_type& path) = 0;
    /**
     * \brief Get a sequence at the given path
     * \throws Error if the path doesn't exist
     */
    virtual sequence get_sequence(const key_type& path) = 0;
    /**
     * \brief Get a map at the given path
     * \throws Error if the path doesn't exist
     */
    virtual table get_map(const key_type& path) = 0;


    /**
     * \brief Get the value at the given path
     * \returns The found value or \c default_value if the path isn't defined
     */
    virtual value_type maybe_get_value(const key_type& path,
                                       const value_type& default_value={} ) = 0;
    /**
     * \brief Get a sequence at the given path
     * \returns The found value or an empty sequence if the path isn't defined
     */
    virtual sequence maybe_get_sequence(const key_type& path) = 0;
    /**
     * \brief Get a map at the given path
     * \returns The found value or an empty table if the path isn't defined
     */
    virtual table maybe_get_map(const key_type& path) = 0;

    /**
     * \brief Sets the value at \c path
     */
    virtual void put(const key_type& path, const value_type& value) = 0;
    /**
     * \brief Sets the value at \c path as a sequence
     */
    virtual void put(const key_type& path, const sequence& value) = 0;
    /**
     * \brief Sets the value at \c path as a map
     */
    virtual void put(const key_type& path, const table& value) = 0;
    /**
     * \brief Sets the value at \c path.key
     */
    virtual void put(const key_type& path, const key_type& key, const value_type& value) = 0;

    /**
     * \brief Appends an element to a sequence
     */
    virtual void append(const key_type& path, const value_type& element) = 0;

    /**
     * \brief Assigns only if the path doesn't already exist
     * \returns if \c path is not found, \c value otherwise the value associated
     *          with the given path
     */
    virtual value_type  maybe_put(const key_type& path, const value_type& value) = 0;
    virtual sequence    maybe_put(const key_type& path, const sequence& value) = 0;
    virtual table       maybe_put(const key_type& path, const table& value) = 0;

    /**
     * \brief Erases a path
     * \returns The number of erased elements
     */
    virtual int erase(const key_type& path) = 0;
    /**
     * \brief Erases a key at path
     * \returns The number of erased elements
     */
    virtual int erase(const key_type& path, const key_type& key) = 0;

    /**
     * \brief Ensures all chached data is saved
     */
    virtual void save() = 0;

    /**
     * \brief Ensures all chached data is refreshed
     */
    virtual void load() = 0;
};

/**
 * \brief Class used to create storage object and initialize the global storage
 */
class StorageFactory : public melanolib::Singleton<StorageFactory>
{
public:
    /// Function type used to create storage objects
    using Constructor = std::function<std::unique_ptr<StorageBase>(const Settings&)>;

    /**
     * \brief Creates a storage object based on the settings
     */
    std::unique_ptr<StorageBase> create(const Settings& settings) const;
    /**
     * \brief Registers a new storage type
     */
    void register_type(const std::string& name, const Constructor& ctor);
    /**
     * \brief Sets the global storage to the result of create()
     */
    void initilize_global_storage(const Settings& settings) const;

private:
    StorageFactory(){}
    friend ParentSingleton;

    std::map<std::string, Constructor> constructors;

};

/**
 * \brief Returns a reference to the storage object
 * \pre set_storage() has been called
 * \throws ConfigurationError if that is not the case
 */
StorageBase& storage();

/**
 * \brief Initialized the storage object from a pointer
 * \pre set_storage() has not be called yet
 * \post storage() returns a valid object
 * \throws ConfigurationError if called more than once
 */
void set_storage(std::unique_ptr<StorageBase>&& pointer);

/**
 * \brief Returns whether there is a storage system installed
 */
bool has_storage();

} // namespace melanobot
#endif // STORAGE_BASE_HPP
