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
#include "http.hpp"

#include <cctype>
#include <iomanip>
#include <sstream>

#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/Exception.hpp>

#include "../string/logger.hpp"
#include "config.hpp"


REGISTER_LOG_TYPE(web,color::dark_blue);
REGISTER_SERVICE(network::http::HttpService,web);

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
            r.parameters += '?';
        else
            r.parameters += '&';
        r.parameters += build_query(params);
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

void HttpService::initialize(const Settings& settings)
{
    /// \todo
    if ( user_agent.empty() )
        user_agent = PROJECT_NAME "/" PROJECT_VERSION " (" PROJECT_WEBSITE ") "
                     "cURLpp/" LIBCURLPP_VERSION ;
    user_agent = settings.get("user_agent", user_agent );
    max_redirs = settings.get("user_agent",max_redirs);
}

Response HttpService::query (const Request& request)
{
    try {
        curlpp::Easy netrequest;
        std::string url = request.location;

        /// \todo read these from settings
        netrequest.setOpt(curlpp::options::UserAgent(user_agent));
        if ( max_redirs )
        {
            netrequest.setOpt(curlpp::options::MaxRedirs(max_redirs));
            netrequest.setOpt(curlpp::options::FollowLocation(true));
        }

        if ( request.command == "GET" )
        {
            url += request.parameters;
        }
        else if ( request.command == "POST" )
        {
            netrequest.setOpt(curlpp::options::PostFields(request.parameters));
        }
        else if ( request.command == "PUT" )
        {
            netrequest.setOpt(curlpp::options::PostFields(request.parameters));
            netrequest.setOpt(curlpp::options::Put(true));
        }
        else if ( request.command == "HEAD" )
        {
            netrequest.setOpt(curlpp::options::NoBody(true));
        }

        netrequest.setOpt(curlpp::options::Url(url));

        Log("web",'<') << request.command << ' ' << request.location;
        std::stringstream ss;
        ss << netrequest;

        return ok(ss.str(),request);

    } catch (std::exception & e) {
        Log("web",'!') << "Error processing " << request.location;
        return error(e.what(),request);
    }
}

} // namespace network::http
} // namespace network
