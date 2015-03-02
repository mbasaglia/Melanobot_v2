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
#ifndef ASYNC_SERVICE_HPP
#define ASYNC_SERVICE_HPP

#include <functional>
#include <string>

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
    std::string origin;        ///< Originating URL
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
    Response ok(const std::string& contents, const Request& origin)
    {
        Response r;
        r.contents = contents;
        r.origin = origin.location;
        return r;
    }
    /**
     * \brief Quick way to create a failure response
     */
    Response error(const std::string& error_message, const Request& origin)
    {
        Response r;
        r.error_message = error_message;
        r.origin = origin.location;
        return r;
    }
};

} // namespace network
#endif // ASYNC_SERVICE_HPP
