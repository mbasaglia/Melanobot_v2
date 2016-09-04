/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2016 Mattia Basaglia
 * \section License
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
#ifndef MELANOBOT_MODULE_GITHUB_GITIO_HPP
#define MELANOBOT_MODULE_GITHUB_GITIO_HPP

#include "web/http.hpp"

namespace github {

/**
 * \brief Shortens a url using git.io
 */
inline std::string git_io_shorten(const std::string& url)
{
    web::Response response;
    web::HttpClient::instance().query(
        web::Request("POST", web::Uri("https://git.io", {{"url", url}})),
        response
    );
    if ( response.status.code == 201 )
        return response.headers["Location"];
    return url;
}

} // namespace github
#endif // MELANOBOT_MODULE_GITHUB_GITIO_HPP
