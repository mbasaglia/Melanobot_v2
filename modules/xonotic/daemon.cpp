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

#include "daemon.hpp"
#include "string/logger.hpp"
#include <openssl/aes.h>

namespace unvanquished {

// See MAX_MSGLEN in daemon/src/engine/qcommon/qcommon.h for max_datagram_size
Daemon::Daemon(network::Server server, std::string rcon_password)
    : Engine(std::move(server), std::move(rcon_password), 32768)
{
}

void Daemon::rcon_command(std::string command)
{
    command.erase(std::remove_if(command.begin(), command.end(),
        [](char c){return c == '\n' || c == '\0' || c == '\xff';}),
        command.end());

    if ( rcon_secure_ == Secure::Unencrypted )
    {
        Log("unv", '<', 4) << command;
        write("rcon "+password()+' '+command);
    }
    else if ( rcon_secure_ == Secure::EncryptedPlain )
    {
        Log("unv", '<', 4) << command;
        /// \todo AES encryption
    }
    else if ( rcon_secure_ == Secure::EncryptedChallenge )
    {
        schedule_challenged_command(command);
    }
}

bool Daemon::is_log(melanolib::cstring_view command) const
{
    return command.strequal("print");
}

void Daemon::challenged_command(const std::string& challenge, const std::string& command)
{
    Log("unv", '<', 4) << command;
    /// \todo AES encryption
}

bool Daemon::is_challenge_response(melanolib::cstring_view command) const
{
    return command.strequal("challengeResponseNew");
}

std::string Daemon::challenge_request() const
{
    return "getchallengenew";
}

void Daemon::on_connect()
{
    write("rconinfo");
}

void Daemon::on_receive(const std::string& cmd, const std::string& message)
{
    if ( cmd == "rconInfoResponse" )
    {
        auto info = parse_info_string(message);
        rcon_secure_ = Secure(melanolib::string::to_int(info["secure"]));
        set_challenge_timeout(std::chrono::seconds(
            melanolib::string::to_int(info["timeour"])
        ));
    }
}

} // namespace unvanquished
