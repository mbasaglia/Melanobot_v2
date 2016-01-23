/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015 Mattia Basaglia
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
};

/**
 * \brief Class storing the service objects
 */
class ServiceRegistry : public melanolib::Singleton<ServiceRegistry>
{
public:
    ~ServiceRegistry()
    {
    }

    /**
     * \brief Registers a service object
     * \note It is assumed that the registered objects will clean up after themselves
     */
    void register_service(const std::string& name, AsyncService* object)
    {
        if ( ServiceRegistry().instance().services.count(name) )
            ErrorLog("sys") << "Overwriting service " << name;
        ServiceRegistry().instance().services[name] = {object, false};
    }

    /**
     * \brief Loads the service settings
     */
    void initialize(const Settings& settings)
    {
        for ( const auto& p : settings )
        {
            auto it = services.find(p.first);
            if ( it == services.end() )
            {
                ErrorLog("sys") << "Trying to load an unknown service: " << p.first;
            }
            else
            {
                Log("sys",'!') << "Loading service: " << p.first;
                it->second.service->initialize(p.second);
                it->second.loaded = true;
            }
        }

        for ( auto& p : services )
            if ( !p.second.loaded )
            {
                p.second.service->initialize({});
                p.second.loaded = true;
            }
    }

    /**
     * \brief Starts all services
     */
    void start()
    {
        for ( const auto& p : services )
            if ( p.second.loaded )
                p.second.service->start();
    }

    /**
     * \brief Stops all services
     */
    void stop()
    {
        for ( const auto& p : services )
            if ( p.second.loaded )
                p.second.service->stop();
    }

    /**
     * \brief Get service by name
     */
    AsyncService* service(const std::string& name) const
    {
        auto it = services.find(name);
        if ( it == services.end() )
        {
            ErrorLog("sys") << "Trying to access unknown service: " << name;
            return nullptr;
        }

        if ( !it->second.loaded )
        {
            ErrorLog("sys") << "Trying to access an unloaded service: " << name;
            return nullptr;
        }

        return it->second.service;
    }

private:
    /**
     * \brief Registry entry, keeping track of loaded services
     */
    struct Entry
    {
        AsyncService* service;
        bool loaded;
    };

    std::unordered_map<std::string, Entry> services;

    ServiceRegistry(){}
    friend ParentSingleton;
};

#endif // ASYNC_SERVICE_HPP
