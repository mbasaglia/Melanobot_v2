/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015-2016 Mattia Basaglia
 * \license
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
#ifndef ASYNC_SERVICE_HPP
#define ASYNC_SERVICE_HPP

#include <functional>
#include <string>
#include <thread>

#include "string/logger.hpp"
#include "concurrency/container.hpp"


/**
 * \brief Base class for external services that might take some time to execute.
 *
 * To specialize, inherit this class and register to ServiceRegistry
 * using Melanomodule::register_service(). The derived classes should act as singletons.
 *
 * You should also have a corresponding log type to use for the service,
 * registered with Melanomodule::register_log_type().
 */
class AsyncService
{
public:

    virtual ~AsyncService()
    {
    }

    /**
     * \brief Loads the service settings
     */
    virtual void initialize(const Settings& settings) = 0;

    /**
     * \brief Starts the service
     */
    virtual void start() = 0;


    /**
     * \brief Stops the service
     */
    virtual void stop() = 0;

    /**
     * \brief Service name
     */
    virtual std::string name() const = 0;

    /**
     * \brief Whether the service is active and functioning properly
     */
    virtual bool running() const = 0;
};

/**
 * \brief Class storing the service objects
 */
class ServiceRegistry : public melanolib::Singleton<ServiceRegistry>
{
public:

    /**
     * \brief Registers a service object
     * \note It is assumed that the registered objects will clean up after themselves
     */
    void register_service(const std::string& name, AsyncService* object);

    /**
     * \brief Loads the service settings
     */
    void initialize(const Settings& settings);

    /**
     * \brief Starts all services
     */
    void start();

    /**
     * \brief Stops all services
     */
    void stop();

    /**
     * \brief Get service by name
     */
    AsyncService* service(const std::string& name) const;

private:
    /**
     * \brief Registry entry, keeping track of loaded services
     */
    struct Entry
    {
        explicit Entry(AsyncService* service)
            : service(service)
        {}

        AsyncService* service;
        bool loaded = false;
        bool started = false;
    };

    using EntryMap = std::unordered_map<std::string, Entry>;

    ServiceRegistry(){}
    friend ParentSingleton;
    
    ~ServiceRegistry()
    {
        stop();
    }

    void load_service(EntryMap::value_type& entry, const Settings& settings);

    EntryMap services;
};

#endif // ASYNC_SERVICE_HPP
