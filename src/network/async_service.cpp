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
#include "async_service.hpp"
#include <boost/network/protocol/http/client.hpp>
//#include <boost/network/include/http/client.hpp>

namespace network {

void HttpRequest::async_get(const std::string& url, const AsyncCallback& callback)
{
    /// \todo async
    callback(get(url));
}

Response HttpRequest::get(const std::string& url)
{
    namespace http = boost::network::http;
    try {
        http::client client;
        http::client::request request(url);
        http::client::response response = client.get(request);
        return ok(body(response));
    } catch (std::exception & e) {
        return error(e.what());
    }
}


} // namespace network
