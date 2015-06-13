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

    bool auto_load() const override { return false; }

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
