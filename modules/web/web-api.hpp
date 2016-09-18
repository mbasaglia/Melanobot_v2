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
#ifndef HANDLER_WEB_API_HPP
#define HANDLER_WEB_API_HPP

#include "melanobot/handler.hpp"
#include "http.hpp"
#include "string/json.hpp"

namespace web {

/**
 * \brief Simple base class for handlers calling web resources
 */
class SimpleWebApi : public melanobot::SimpleAction
{
public:
    using SimpleAction::SimpleAction;

protected:
    /**
     * \brief Sends an asynchronous http request
     */
    void request_http(const network::Message& msg, web::Request&& request)
    {
        web::HttpClient::instance().async_query(
            std::move(request),
            [this, msg](web::Request& request, web::Response& response)
            {
                if ( response.status.is_error() )
                    http_failure(msg, request, response);
                else
                    http_success(msg, request, response);
            }),
            [this, msg](web::Request& request, const OperationStatus& status)
            {
                network_failure(msg, request, status);
            }
        ;
    }

    /**
     * \brief Called when request_http() gets a successful response
     */
    virtual void http_success(const network::Message& msg, web::Request& request, web::Response& response) = 0;

    /**
     * \brief Called when request_http() gets an error response
     */
    virtual void http_failure(const network::Message&, web::Request&, web::Response&) {}

    /**
     * \brief Called when request_http() encounters an error before it can receive a response
     */
    virtual void network_failure(const network::Message&, web::Request&, const OperationStatus&) {}

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
    void request_json(const network::Message& msg, web::Request&& request)
    {
        request_http(msg, std::move(request));
    }

    void http_success(const network::Message& msg, web::Request& request, web::Response& response) override
    {
        Settings ptree;
        JsonParser parser;
        try {
            ptree = parser.parse_string(response.body.read_all(), request.uri.full());
            json_success(msg, ptree);
        } catch ( const JsonError& err ) {
            ErrorLog errlog("web", "JSON Error");
            if ( settings::global_settings.get("debug", 0) )
                errlog << err.file << ':' << err.line << ": ";
            errlog << err.what();
            json_failure(msg);
        }
    }

    void http_failure(const network::Message& msg, web::Request&, web::Response&) override
    {
        json_failure(msg);
    }

    void network_failure(const network::Message& msg, web::Request&, const OperationStatus&) override
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


} // namespace web
#endif // HANDLER_WEB_API_HPP
