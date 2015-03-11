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
#ifndef HTTP_HPP
#define HTTP_HPP

#include "async_service.hpp"

#include <map>

namespace network {

/**
 * \brief HTTP networking utilities
 */
namespace http {

/**
 * \brief Request parameters
 */
typedef std::map<std::string,std::string> Parameters;

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
 * \brief Creates a GET request
 */
Request get(const std::string& url, const Parameters& params = Parameters());
/**
 * \brief Creates a simple POST request
 */
Request post(const std::string& url, const Parameters& params = Parameters());

/**
 * \brief HTTP Service
 */
class HttpService : public ThreadedAsyncService
{
public:
    Response query (const Request& request) override;
    void initialize(const Settings& settings) override;
    bool auto_load() const override { return true; }

    static HttpService& instance()
    {
        static HttpService singleton;
        return singleton;
    }

private:
    std::string user_agent;
    int         max_redirs = 3;
};

} // namespace network::http
} // namespace network
#endif // HTTP_HPP
