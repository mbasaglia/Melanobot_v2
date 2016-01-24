/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015-2016 Mattia Basaglia
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
#ifndef XONOTIC_CONNECTION_DETAILS_HPP
#define XONOTIC_CONNECTION_DETAILS_HPP

#include "network/server.hpp"

namespace xonotic {

/**
 * \brief Xonotic connection information
 */
struct ConnectionDetails
{
    enum Secure {
        NO        = 0,  ///< Plaintext rcon
        TIME      = 1,  ///< Time-based digest
        CHALLENGE = 2   ///< Challeng-based security
    };

    ConnectionDetails(network::Server     server,
            std::string         rcon_password,
            Secure              rcon_secure = NO)
        : server(std::move(server)),
          rcon_password(std::move(rcon_password)),
          rcon_secure(rcon_secure)
    {
    }

    network::Server     server;         ///< Connection server
    std::string         rcon_password;  ///< Rcon Password
    Secure              rcon_secure;    ///< Rcon Secure
};

} // namespace xonotic

#endif // XONOTIC_CONNECTION_DETAILS_HPP
