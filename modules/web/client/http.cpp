/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015-2017 Mattia Basaglia
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

#include "string/logger.hpp"
#include "config.hpp"


namespace web {

void HttpClient::initialize(const Settings& settings)
{
    std::string user_agent = settings.get("user_agent", "" );
    if ( user_agent.empty() )
    {
        httpony::UserAgent default_user_agent(
            PROJECT_NAME "/" PROJECT_VERSION " (" PROJECT_WEBSITE ") "
        );
        set_user_agent( default_user_agent + httpony::UserAgent::default_user_agent());
    }
    set_max_redirects(settings.get("redirects", max_redirects()));

    int timeout_seconds = timeout() ? timeout()->count() : 0;
    timeout_seconds = settings.get("timeout", timeout_seconds);
    if ( timeout_seconds <= 0 )
        clear_timeout();
    else
        set_timeout(melanolib::time::seconds(timeout_seconds));
}

void HttpClient::on_error(Request& request, const OperationStatus& status)
{
    ErrorLog("web") << "Error processing " << request.uri.full() << ": " << status;
}

void HttpClient::on_response(Request& request, Response& response)
{
    Log("web", '>') << response.status.code << ' ' << request.uri.full();
}

void HttpClient::process_request(Request& request)
{
    ParentClient::process_request(request);
    Log("web", '<') << request.method << ' ' << request.uri.full();
}

} // namespace web
