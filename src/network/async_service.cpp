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

#include <cctype>
#include <iomanip>
#include <sstream>

#define BOOST_NETWORK_ENABLE_HTTPS
#include <boost/network/protocol/http/client.hpp>

#include "../logger.hpp"


namespace network {

namespace http {

std::string urlencode ( const std::string& text )
{
    std::ostringstream ss;
    ss << std::hex << std::uppercase;
    for ( char c : text )
    {
        if ( std::isalnum(c) || c == '-' || c == '.' || c == '_' || c == '~' )
            ss << c;
        else
            ss << '%' << std::setw(2) << std::setfill('0') << int(c);
    }
    return ss.str();
}

std::string build_query ( const Parameters& params )
{
    std::ostringstream ss;
    auto last = params.end(); --last;
    for ( auto it = params.begin(); it != params.end(); ++it )
    {
        ss << urlencode(it->first) << '=' << urlencode(it->second);
        if ( it != last )
            ss << '&';
    }
    return ss.str();
}

Request get(const std::string& url, const Parameters& params)
{
    Request r;

    r.location = url;
    r.command = "GET";

    if ( !params.empty() )
    {
        if ( url.find('?') == std::string::npos )
            r.location += '?';
        r.location += build_query(params);
    }

    return r;
}

Request post(const std::string& url, const Parameters& params)
{
    Request r;

    r.location = url;
    r.command = "POST";
    r.parameters = build_query(params);

    return r;
}

void Client::async_query (const Request& request, const AsyncCallback& callback)
{
    /// \todo async
    callback(query(request));
}

Response Client::query (const Request& request)
{
    typedef boost::network::http::client client_type;

    client_type client{client_type::options().follow_redirects(true)};
    try {
        client_type::request netrequest(request.location);
        client_type::response response;

         /// \todo POST/PUT with custom content type
        if ( request.command == "GET" )
            response = client.get(netrequest);
        else if ( request.command == "POST" )
            response = client.post(netrequest,request.parameters);
        else if ( request.command == "PUT" )
            response = client.put(netrequest,request.parameters);
        else if ( request.command == "HEAD" )
            response = client.head(netrequest);
        else if ( request.command == "DELETE" )
            response = client.delete_(netrequest);

        log("web",'>',request.command+' '+request.location);
        return ok(body(response)); /// \todo preserve headers
    } catch (std::exception & e) {
        log("web",'!',"Error processing "+request.location);
        return error(e.what());
    }
}

} // namespace network::http
} // namespace network
