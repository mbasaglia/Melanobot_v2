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
#include "http.hpp"

#include <cctype>
#include <iomanip>
#include <sstream>

#include <cpr/cpr.h>
#include <cpr/util.h>

#include "string/logger.hpp"
#include "config.hpp"


namespace web {

std::string Request::full_url() const
{
    if ( !parameters.empty() )
    {
        std::string url = resource;
        if ( url.find('?') == std::string::npos )
            url += '?';
        else
            url += '&';
        url += build_query(parameters);
        return url;
    }
    return resource;
}

std::string urlencode(const std::string& text)
{
    return cpr::util::urlEncode(text);
}

static cpr::Parameters cpr_parameters(const Parameters& params)
{
    cpr::Parameters parameters;
    for ( const auto & param : params )
    {
        parameters.AddParameter({param.first, param.second});
    }
    return parameters;
}

std::string build_query(const Parameters& params)
{
    return cpr_parameters(params).content;
}

void HttpService::initialize(const Settings& settings)
{
    if ( user_agent.empty() )
        user_agent = PROJECT_NAME "/" PROJECT_VERSION " (" PROJECT_WEBSITE ") "
                     "cpr";
    user_agent = settings.get("user_agent", user_agent );
    max_redirs = settings.get("redirects", max_redirs);
}

Response HttpService::query (const Request& request)
{
    std::string url = request.full_url();

    cpr::Session session;
    cpr::Response response;


    session.SetUrl(cpr::Url(url));
    session.SetHeader({
        {"User-Agent", user_agent}
    });
    if ( max_redirs )
    {
        session.SetMaxRedirects(max_redirs);
        session.SetRedirect(true);
    }
    else
    {
        session.SetMaxRedirects(0);
        session.SetRedirect(false);
    }
    /// TODO SetTimeout

    if ( request.command == "GET" )
    {
        response = session.Get();
    }
    else if ( request.command == "POST" )
    {
        cpr::Payload payload{};
        payload.content = request.body;
        session.SetPayload(payload);
        response = session.Post();
    }
    else if ( request.command == "PUT" )
    {
        cpr::Payload payload{};
        payload.content = request.body;
        session.SetPayload(payload);
        response = session.Put();
    }
    else if ( request.command == "HEAD" )
    {
        response = session.Head();
    }

    Log("web",'<') << request.command << ' ' << request.resource;

    if ( response.error )
    {
        ErrorLog("web") << "Error processing " << request.resource;
    }

    return Response(response.text, response.url, response.status_code, response.error.message );
}

} // namespace web
