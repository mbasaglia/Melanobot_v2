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
#ifndef HTTP_HPP
#define HTTP_HPP

#include "network/async_service.hpp"

#include <map>

namespace web {

/**
 * \brief Request parameters
 */
using Parameters = std::map<std::string,std::string>;

using Headers = std::map<std::string,std::string>;

/**
 * \brief A network request
 */
struct Request
{
    Request() = default;
    Request(std::string command,
            std::string resource,
            Parameters  parameters = {},
            std::string body = {},
            Headers     headers = {}
           )
        : command(std::move(command)),
          resource(std::move(resource)),
          parameters(std::move(parameters)),
          body(std::move(body)),
          headers(std::move(headers))
    {}

    Request& get(std::string url)
    {
        return method_get().set_url(std::move(url));
    }

    Request& method_get()
    {
        command = "GET";
        return *this;
    }

    Request& method_post()
    {
        command = "GET";
        return *this;
    }

    Request& set_param(const std::string& name, const std::string& value)
    {
        parameters[name] = value;
        return *this;
    }

    Request& set_params(Parameters params)
    {
        std::swap(parameters, params);
        return *this;
    }

    Request& set_header(const std::string& name, const std::string& value)
    {
        headers[name] = value;
        return *this;
    }

    Request& set_headers(Headers new_headers)
    {
        std::swap(headers, new_headers);
        return *this;
    }

    Request& set_body(std::string contents)
    {
        std::swap(body, contents);
        return *this;
    }

    Request& set_url(std::string url)
    {
        std::swap(resource, url);
        return *this;
    }

    /**
     * \brief Resource + query
     */
    std::string full_url() const;

    std::string command;    ///< Protocol-specific command
    std::string resource;   ///< Name/identifier for the requested resource
    Parameters  parameters; ///< GET query parameters
    std::string body;       ///< POST/PUT body
    Headers     headers;    ///< Headers
};

/**
 * \brief Result of a request
 */
struct Response
{
    Response(
        std::string contents,
        std::string resource,
        int status_code = 200,
        std::string error_message = {},
        Headers     headers = {}
    ) : contents(std::move(contents)),
        resource(std::move(resource)),
        status_code(status_code),
        error_message(std::move(error_message)),
        headers(std::move(headers))
    {}

    bool success() const
    {
        return status_code < 400;
    }

    std::string contents;       ///< Response contents
    std::string resource;       ///< Name/identifier for the requested resource
    int         status_code = 200;
    std::string error_message;  ///< Message in the case of error, if empty not an error
    Headers     headers;        ///< Headers
};

/**
 * \brief Callback used by asyncronous calls
 */
using AsyncCallback = std::function<void(const Response&)>;

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
 * \brief HTTP Service
 */
class HttpService : public AsyncService, public melanolib::Singleton<HttpService>
{
public:
    Response query (const Request& request);
    void initialize(const Settings& settings) override;


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

    void async_query (const Request& request, const AsyncCallback& callback)
    {
        requests.push({request, callback});
    }

private:
    HttpService(){}
    ~HttpService()
    {
        stop();
    }
    friend ParentSingleton;

    std::string user_agent;
    int         max_redirs = 3;
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

} // namespace web
#endif // HTTP_HPP
