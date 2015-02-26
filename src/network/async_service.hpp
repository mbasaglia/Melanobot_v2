/**
 * \file
 *
 * \author Mattia Basaglia
 *
 * \section License
 *
 * Copyright (C) 2015 Mattia Basaglia
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#ifndef ASYNC_SERVICE_HPP
#define ASYNC_SERVICE_HPP

#include <functional>
#include <string>
#include <map>

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
    std::string location;
    std::string command;
    std::string parameters;
};

/**
 * \brief Result of a request
 */
struct Response
{
    std::string error_message; ///< Message in the case of error, if empty not an error
    std::string contents;      ///< Message contents
};

/**
 * \brief Callback used by asyncronous calls
 */
typedef std::function<void(const Response&)> AsyncCallback;

/**
 * \brief Base class for external services that might take some time to execute
 */
class AsyncService
{
public:
    virtual ~AsyncService() {}

    /**
     * \brief Asynchronous query
     */
    virtual void async_query (const Request& request, const AsyncCallback& callback) = 0;
    /**
     * \brief Synchronous query
     */
    virtual Response query (const Request& request) = 0;

protected:
    /**
     * \brief Quick way to create a successful response
     */
    Response ok(const std::string& contents)
    {
        Response r;
        r.contents = contents;
        return r;
    }
    /**
     * \brief Quick way to create a failure response
     */
    Response error(const std::string& error_message)
    {
        Response r;
        r.error_message = error_message;
        return r;
    }
};
/**
 * \brief HTTP networking utilities
 * \todo move to its own file
 */
namespace http {

/**
 * \brief Request parameters
 */
typedef std::map<std::string,std::string> Parameters;

/**
 * \brief Encode a string to make it fit to be used in a url
 * \todo urldecode (?)
 * \see http://www.faqs.org/rfcs/rfc3986.html
 */
std::string urlencode ( const std::string& text );

/**
 * \brief Creates a query string from the given parameters
 */
std::string build_query ( const Parameters& params );

/**
 * \brief Creates a GET request
 */
Request get(const std::string& url, const Parameters& params = Parameters());
/**
 * \brief Creates a simple POST request
 */
Request post(const std::string& url, const Parameters& params = Parameters());

/**
 * \brief HTTP client
 */
class Client : public AsyncService
{
public:
    void async_query (const Request& request, const AsyncCallback& callback) override;
    Response query (const Request& request) override;
};

} // namespace network::http

} // namespace network
#endif // ASYNC_SERVICE_HPP
