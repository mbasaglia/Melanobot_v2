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
 * \brief Namespace for network operations
 *
 * This includes asyncronous calls and network protocol operations
 */
namespace network {

/**
 * \brief A network request
 */
struct Request
{
    std::string resource;       ///< Name/identifier for the requested resource
    std::string command;        ///< Protocol-specific command
    std::vector<std::string> parameters; ///< Parameters for the request
};

/**
 * \brief Result of a request
 */
struct Response
{
    std::string error_message;  ///< Message in the case of error, if empty not an error
    std::string contents;       ///< Response contents
    std::string resource;       ///< Name/identifier for the requested resource
};

/**
 * \brief Callback used by asyncronous calls
 */
using AsyncCallback = std::function<void(const Response&)>;

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

    virtual ~AsyncService() {}

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
     * \brief Asynchronous query
     */
    virtual void async_query (const Request& request, const AsyncCallback& callback) = 0;

    /**
     * \brief Synchronous query
     */
    virtual Response query (const Request& request) = 0;

    /**
     * \brief Whether the service should be loaded without explicit configuration
     */
    virtual bool auto_load() const = 0;

protected:
    AsyncService() = default;
    AsyncService(const AsyncService&) = delete;
    AsyncService(AsyncService&&) = delete;
    AsyncService& operator=(const AsyncService&) = delete;
    AsyncService& operator=(AsyncService&&) = delete;

    /**
     * \brief Quick way to create a successful response
     */
    Response ok(const std::string& contents, const Request& origin)
    {
        Response r;
        r.contents = contents;
        r.resource = origin.resource;
        return r;
    }
    /**
     * \brief Quick way to create a failure response
     */
    Response error(const std::string& error_message, const Request& origin)
    {
        Response r;
        r.error_message = error_message;
        r.resource = origin.resource;
        return r;
    }
};

/**
 * \brief An AsyncService implemented with a separate thread.
 *
 * Handles asynchronous calls as a queue of synchronous calls performed
 * sequentially in a separate thread.
 *
 * Derived classes only need to override query() and auto_load().
 */
class ThreadedAsyncService : public AsyncService
{
public:

    ~ThreadedAsyncService() override
    {
        stop();
    }

    void start() override
    {
        requests.start();
        if ( !thread.joinable() )
            thread = std::move(std::thread([this]{run();}));
    }

    void stop() override
    {
        requests.stop();
        if ( thread.joinable() )
            thread.join();
    }

    void async_query (const Request& request, const AsyncCallback& callback) override
    {
        requests.push({request,callback});
    }

private:
    /**
     * \brief Internal request record
     */
    struct Item
    {
        Request       request;
        AsyncCallback callback;
    };

    ConcurrentQueue<Item> requests;
    std::thread thread;

    void run()
    {
        while ( requests.active() )
        {
            Item item;
            requests.pop(item);
            if ( !requests.active() )
                break;
            item.callback(query(item.request));
        }
    }
};

/**
 * \brief Class storing the service objects
 */
class ServiceRegistry
{
public:

    /**
     * \brief Registers a service object
     * \note It is assumed that the registered objects will clean up after themselves
     */
    void register_service(const std::string& name, AsyncService* object)
    {
        if ( ServiceRegistry().instance().services.count(name) )
            ErrorLog("sys") << "Overwriting service " << name;
        ServiceRegistry().instance().services[name] =
            {object, object->auto_load()};
    }

    static ServiceRegistry& instance()
    {
        static ServiceRegistry singleton;
        return singleton;
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

    std::unordered_map<std::string,Entry> services;

    ServiceRegistry(){}
    ServiceRegistry(const ServiceRegistry&) = delete;
    ServiceRegistry(ServiceRegistry&&) = delete;
    ServiceRegistry& operator=(const ServiceRegistry&) = delete;
    ServiceRegistry& operator=(ServiceRegistry&&) = delete;
};

/**
 * \brief Get service by name (less verbose than through ServiceRegistry)
 */
inline AsyncService* service(const std::string& name)
{
   return ServiceRegistry::instance().service(name);
}

/**
 * \brief Returns a pointer to the service object or throws an exception
 * \throws ConfigurationError if the service is not active
 */
inline AsyncService* require_service(const std::string& name)
{
   if ( AsyncService* serv =  ServiceRegistry::instance().service(name) )
       return serv;
   throw ConfigurationError("Missing required service :"+name);
}


} // namespace network
#endif // ASYNC_SERVICE_HPP
