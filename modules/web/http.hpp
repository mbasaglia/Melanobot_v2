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

#include "aliases.hpp"
#include "network/async_service.hpp"

namespace web {


/**
 * \brief HTTP Service
 */
class HttpClient : public AsyncService,
                   public httpony::BasicAsyncClient<httpony::ssl::SslClient>,
                   public melanolib::Singleton<HttpClient>
{
    using ParentClient = httpony::BasicAsyncClient<httpony::ssl::SslClient>;
public:
    void initialize(const Settings& settings) override;

    template<class OnResponse, class OnError = melanolib::Noop>
        void async_query(Request&& request,
                         const OnResponse& on_response,
                         const OnError& on_error = {})
    {
        ParentClient::async_query(std::move(request), on_response, melanolib::Noop{}, on_error);
    }

    void start() override
    {
        ParentClient::start();
    }

    void stop() override
    {
        ParentClient::stop();
    }

    bool running() const override
    {
        return ParentClient::started();
    }


    std::string name() const override
    {
        return "HTTP Client";
    }

protected:
    void on_error(Request& request, const OperationStatus& status) override;
    void on_response(Request& request, Response& response) override;
    void process_request(Request& request) override;

private:
    HttpClient()
    {
        set_max_redirects(3);
        set_timeout(melanolib::time::seconds(10));
    }
    friend ParentSingleton;
};

} // namespace web
#endif // HTTP_HPP
