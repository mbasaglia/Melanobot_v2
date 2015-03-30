/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright  Mattia Basaglia
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
#ifndef NETWORK_HPP
#define NETWORK_HPP

#include <sstream>

#include "string/string_functions.hpp"
#include "time/time.hpp"

namespace network {

/**
 * \brief Identifies a network server
 */
struct Server
{
    std::string host;
    uint16_t    port;

    /**
     * \brief Creates an object from explicit values
     */
    Server( std::string host, uint16_t port ) : host(std::move(host)), port(port) {}

    /**
     * \brief Creates object from host:port string
     */
    explicit Server( const std::string& server )
        : port(0)
    {
        auto p = server.find(':');
        host = server.substr(0,p);
        if ( p != std::string::npos && p < server.size()-1 && std::isdigit(server[p+1]) )
            port = string::to_uint(server.substr(p+1));
    }

    /**
     * \brief Creates an invalid server
     */
    Server() : host(""), port(0) {}

    Server(const Server&) = default;
    Server(Server&&) noexcept = default;
    Server& operator=(const Server&) = default;
    Server& operator=(Server&&) noexcept(std::is_nothrow_move_assignable<std::string>::value) = default;

    /**
     * \brief Name as host:port
     */
    std::string name() const
    {
        std::ostringstream ss;
        ss << host << ':' << port;
        return ss.str();
    }
};
} // namespace network
#endif // NETWORK_HPP
