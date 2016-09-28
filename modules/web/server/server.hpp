/**
 * \file
 *
 * \author Mattia Basaglia
 *
 * \copyright Copyright 2016 Mattia Basaglia
 *
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
#ifndef MELANOBOT_MODULE_WEB_SERVER_HPP
#define MELANOBOT_MODULE_WEB_SERVER_HPP

#include "base_pages.hpp"
#include "network/async_service.hpp"

namespace web {

/**
 * \brief HTTP server service
 */
class HttpServer : public AsyncService, public HttpRequestHandler,
                   public httpony::BasicPooledServer<httpony::ssl::SslServer>
{
    using ParentServer = httpony::BasicPooledServer<httpony::ssl::SslServer>;
public:
    HttpServer() : ParentServer(1, httpony::IPAddress{}, false) {}

    void initialize(const Settings& settings) override;

    void start() override
    {
        ParentServer::start();
        Log("wsv", '!') << "Started " << name();
    }

    void stop() override
    {
        Log("wsv", '!') << "Stopping " << name();
        ParentServer::stop();
        Log("wsv", '!') << "Server stopped";
    }

    bool running() const override
    {
        return Server::running();
    }

    std::string name() const override;

    void respond(httpony::Request& request, const httpony::Status& status) override;

    std::string format_info(const std::string& template_string,
                            const Request& request,
                            const Response& response) const
    {
        std::ostringstream stream;
        ParentServer::log_response(template_string, request, response, stream);
        return stream.str();
    }

protected:
    void error(httpony::io::Connection& connection, const httpony::OperationStatus& what) const override
    {
        ErrorLog("wsv") << "Server error: " << connection.remote_address() << ": " << what;
    }

private:
    void log_response(const Request& request, const Response& response) const
    {
        Log("wsv", '<') << format_info(log_format, request, response);
    }

    Response unhandled_error(const WebPage::RequestItem& request, const Status& status) const override
    {
        return ErrorPage::canned_response(status, request.request.protocol);
    }

    std::string log_format = "%h %l %u %t \"%r\" %s %b \"%{Referer}i\" \"%{User-Agent}i\"";
    httpony::Headers headers;
};

} // namespace web
#endif // MELANOBOT_MODULE_WEB_SERVER_HPP
