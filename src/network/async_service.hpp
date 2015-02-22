/**
 * \file
 *
 * \author Mattia Basaglia
 *
 * \section License
 *
 * Copyright (C)  Mattia Basaglia
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

#include <atomic>
#include <functional>
#include <list>
#include <mutex>
#include <thread>

namespace network {

/**
 * \brief Result of a request
 */
struct Response
{
    std::string error_message; ///< Message in the case of error, if empty not an error
    std::string contents;      ///< Message contents
};

typedef std::function<void(const Response&)> AsyncCallback;

/**
 * \brief Base class for external services that might take some time to execute
 */
class AsyncService
{
protected:

    Response ok(const std::string& contents)
    {
        Response r;
        r.contents = contents;
        return r;
    }
    Response error(const std::string& error_message)
    {
        Response r;
        r.error_message = error_message;
        return r;
    }
};

class HttpRequest : public AsyncService
{
public:

    /**
     * \brief Synchronous Get request
     * \todo parameters
     */
    Response get(const std::string& url);

    /**
     * \brief Asynchronous Get request
     * \todo parameters
     */
    void async_get(const std::string& url, const AsyncCallback& callback);
};

} // namespace network
#endif // ASYNC_SERVICE_HPP
