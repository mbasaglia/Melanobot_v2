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
#ifndef STORAGE_HPP
#define STORAGE_HPP

#include "storage_base.hpp"
#include "cache_policy.hpp"

namespace storage {
namespace file {

/**
 * \brief Storage system
 *
 * \section store_prot Storage Protocol
 *
 * \par get \e key
 * Returns the value associtated with \b key,
 * or an error if \b key doesn't exists
 *
 * \par maybe_get \e key \e default
 * If \b key has been already defined, returns its value,
 * otherwise returns \b default.
 *
 * \par put \e key \e value
 * Assigns \b value to \b key, returns \b value.
 *
 * \par maybe_put put \e key \e value
 * Assigns \b value to \b key only if \b key has not already been defined.
 * Returns the final value of \b key.
 *
 * \par append \e key \e value \e separator
 * If \b key has a non-empty value, appends both \b separator and \b value to
 * the existsing contents, otherwise assigns \b value.
 * Returns the final value of \b key.
 *
 * \par delete \e key
 * Removes \e key. Returns an error if \b key wasn't defined.
 */
class Storage : public StorageBase
{
public:

    Storage(const Settings& settings);

    ~Storage();

    /**
     * \brief Get the value at the given path
     * \throws Error if the path doesn't exist
     */
    value_type get_value(const key_type& path) override;
    /**
     * \brief Get a sequence at the given path
     * \throws Error if the path doesn't exist
     */
    sequence get_sequence(const key_type& path) override;
    /**
     * \brief Get a map at the given path
     * \throws Error if the path doesn't exist
     */
    table get_map(const key_type& path) override;


    /**
     * \brief Get the value at the given path
     * \returns The found value or \c default_value if the path isn't defined
     */
    value_type maybe_get_value(const key_type& path,
                               const value_type& default_value={} ) override;
    /**
     * \brief Get a sequence at the given path
     * \returns The found value or an empty sequence if the path isn't defined
     */
    sequence maybe_get_sequence(const key_type& path) override;
    /**
     * \brief Get a map at the given path
     * \returns The found value or an empty table if the path isn't defined
     */
    table maybe_get_map(const key_type& path) override;

    /**
     * \brief Sets the value at \c path
     */
    void put(const key_type& path, const value_type& value) override;
    /**
     * \brief Sets the value at \c path as a sequence
     */
    void put(const key_type& path, const sequence& value) override;
    /**
     * \brief Sets the value at \c path as a map
     */
    void put(const key_type& path, const table& value) override;
    /**
     * \brief Sets the value at \c path.key
     */
    void put(const key_type& path, const key_type& key, const value_type& value) override;

    /**
     * \brief Assigns only if the path doesn't already exist
     * \returns if \c path is not found, \c value otherwise the value associated
     *          with the given path
     */
    value_type maybe_put(const key_type& path, const value_type& value) override;
    sequence   maybe_put(const key_type& path, const sequence& value) override;
    table      maybe_put(const key_type& path, const table& value) override;

    /**
     * \brief Appends an element to a sequence
     */
    void append(const key_type& path, const value_type& element) override;

    /**
     * \brief Erases a path
     * \returns The number of erased elements
     */
    int erase(const key_type& path) override;
    /**
     * \brief Erases a key at path
     * \returns The number of erased elements
     */
    int erase(const key_type& path, const key_type& key) override;

    /**
     * \brief Ensures all chached data is saved
     */
    void save() override;

    /**
     * \brief Ensures all chached data is refreshed
     */
    void load() override;

protected:
    /**
     * \brief Calls save() only if the cache policy requires it
     */
    void maybe_save();
    /**
     * \brief Calls load() only if the cache policy requires it
     */
    void maybe_load();

    /**
     * \brief Converts a property tree node to a sequence
     */
    static sequence node_to_sequence(const PropertyTree& node);
    /**
     * \brief Converts a property tree node to a table
     */
    static table node_to_map(const PropertyTree& node);
    /**
     * \brief Converts a sequence to a property tree node
     */
    static PropertyTree node_from_sequence(const sequence& value);
    /**
     * \brief Converts a table to a property tree node
     */
    static PropertyTree node_from_map(const table& value);

private:
    PropertyTree         data;          ///< Stored data
    std::string          filename;      ///< File to store the data in
    settings::FileFormat format;        ///< Storage file format (JSON or XML only)
    cache::Policy        cache_policy;  ///< Cache policy
};



} // namespace file
} // namespace storage
#endif // STORAGE_HPP
