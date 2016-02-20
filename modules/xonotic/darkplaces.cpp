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
#include "darkplaces.hpp"
#include "encryption.hpp"

namespace xonotic {

Darkplaces::Darkplaces(network::Server server,
        std::string rcon_password, Secure rcon_secure)
    : Engine(std::move(server), std::move(rcon_password), 1400),
      rcon_secure_(rcon_secure)
{
}

std::pair<melanolib::cstring_view, melanolib::cstring_view>
    Darkplaces::split_command(melanolib::cstring_view message) const
{
    if ( message[0] == 'n' )
        return {
            {message.begin(), message.begin()+1},
            {message.begin()+1, message.end()}
        };
    return Engine::split_command(message);
}

melanolib::cstring_view Darkplaces::filter_challenge(melanolib::cstring_view message) const
{
    return {message.begin(), melanolib::math::min(message.begin()+11, message.end())};
}

void Darkplaces::challenged_command(const std::string& challenge, const std::string& command)
{
    std::string challenge_command = challenge+' '+command;
    std::string key = crypto::hmac_md4(challenge_command, password());
    write("srcon HMAC-MD4 CHALLENGE "+key+' '+challenge_command);
}

void Darkplaces::rcon_command(std::string command)
{
    command.erase(std::remove_if(command.begin(), command.end(),
        [](char c){return c == '\n' || c == '\0' || c == '\xff';}),
        command.end());

    if ( rcon_secure_ == Secure::NO )
    {
        write("rcon "+password()+' '+command);
    }
    else if ( rcon_secure_ == Secure::TIME )
    {
        auto message = std::to_string(std::time(nullptr))+".000000 "+command;
        write("srcon HMAC-MD4 TIME "+ crypto::hmac_md4(message, password())
            +' '+message);
    }
    else if ( rcon_secure_ == Secure::CHALLENGE )
    {
        schedule_challenged_command(command);
    }
}


bool Darkplaces::is_log(melanolib::cstring_view command) const
{
    return command[0] == 'n';
}

} // namespace xonotic
