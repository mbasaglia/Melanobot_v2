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
#ifndef HANDLER_WEB_API_HPP
#define HANDLER_WEB_API_HPP

#include "handler/handler.hpp"
#include "http.hpp"
#include "string/json.hpp"

namespace handler {

/**
 * \brief Simple base class for handlers calling web resources
 */
class SimpleWebApi : public SimpleAction
{
public:
    SimpleWebApi(const std::string& default_trigger, const Settings& settings,
                 handler::HandlerContainer* parent)
        : SimpleAction(default_trigger, settings, parent)
    {
        network::require_service("web");
    }

protected:
    /**
     * \brief Sends an asynchronous http request
     * \note Not yet asynchronous
     */
    void request_http(const network::Message& msg, const network::Request& request)
    {
        network::service("web")->async_query(request,
            [this,msg](const network::Response& resp)
            {
                if ( resp.error_message.empty() )
                    http_success(msg,resp);
                else
                    http_failure(msg,resp);
            });
    }

    /**
     * \brief Called when request_http() gets a successfult response
     */
    virtual void http_success(const network::Message& msg, const network::Response& response) = 0;


    /**
     * \brief Called when request_http() fails
     */
    virtual void http_failure(const network::Message&, const network::Response&) {}

};

/**
 * \brief Simple base class for handlers using AJAX web APIs
 */
class SimpleJson : public SimpleWebApi
{
public:
    using SimpleWebApi::SimpleWebApi;

protected:
    /**
     * \brief Sends an asynchronous http request
     */
    void request_json(const network::Message& msg, const network::Request& request)
    {
        request_http(msg,request);
    }

    void http_success(const network::Message& msg, const network::Response& response) override
    {
        Settings ptree;
        JsonParser parser;
        parser.throws(false);
        ptree = parser.parse_string(response.contents,response.origin);

        if ( parser.error() )
            json_failure(msg);
        else
            json_success(msg,ptree);
    }

    void http_failure(const network::Message& msg, const network::Response&) override
    {
        json_failure(msg);
    }

    /**
     * \brief Called when the json request succeedes
     */
    virtual void json_success(const network::Message& msg, const Settings& parsed) = 0;

    /**
     * \brief Called when the json request fails
     */
    virtual void json_failure(const network::Message&) {}

};


} // namespace handler
#endif // HANDLER_WEB_API_HPP
