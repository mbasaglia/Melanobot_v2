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

#include "network/async_service.hpp"
#include "cache_policy.hpp"

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
class Storage : public network::ThreadedAsyncService
{
public:
    /**
     * \brief Error arising from explicit calls
     */
    class Error : public std::invalid_argument
    {
    public:
        using std::invalid_argument::invalid_argument;
    };

    void initialize(const Settings& settings) override;

    void start() override;

    void stop() override;

    network::Response query (const network::Request& request) override;

    bool auto_load() const override { return true; }

    /**
     * \brief Get the value of the given key
     * \throws Error if the key doesn't exist
     */
    std::string get(const std::string& key);
    /**
     * \brief Get the value of the given key
     * \returns The found value or \c default_value if the key isn't defined
     */
    std::string maybe_get(const std::string& key, const std::string& default_value = {});

    /**
     * \brief Assigns the given value
     * \returns \c value
     */
    std::string put(const std::string& key, const std::string& value);

    /**
     * \brief Assigns only if the key doesn't already exist
     * \returns if \c key is not found, \c value otherwise the value associated
     *          with the given key
     */
    std::string maybe_put(const std::string& key, const std::string& value);

    /**
     * \brief Append an element/string to the given key
     *
     * If \b key has a non-empty value, appends both \b separator and \b value to
     * the existsing contents, otherwise assigns \b value.
     * 
     * \returns the final value of \b key.
     */
    std::string append(const std::string& key,
                       const std::string& value,
                       const std::string& separator = "\n");

    /**
     * \brief Erases a key
     * \throws Error if the key doesn't exist
     */
    void erase(const std::string& key);

protected:
    /**
     * \brief Ensures all chached data is saved
     */
    void save();
    /**
     * \brief Calls save() only if the cache policy requires it
     */
    void maybe_save();

    /**
     * \brief Ensures all chached data is refreshed
     */
    void load();
    /**
     * \brief Calls load() only if the cache policy requires it
     */
    void maybe_load();

private:
    PropertyTree         data;          ///< Stored data
    std::string          filename;      ///< File to store the data in
    settings::FileFormat format;        ///< Storage file format (JSON or XML only)
    bool                 lazy_save{0};  ///< If \b true, save only on stop()
    cache::Policy        cache_policy;  ///< Cache policy
};

#endif // STORAGE_HPP
