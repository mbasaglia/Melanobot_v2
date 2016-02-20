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
#ifndef MELANOBOT_MODULES_XONOTIC_HANDLERS_UNVANQUISHED_HPP
#define MELANOBOT_MODULES_XONOTIC_HANDLERS_UNVANQUISHED_HPP

#include "core/handler/connection_monitor.hpp"

namespace unvanquished {


/**
 * \brief Lists detailed info on server and players
 */
class UnvanquishedStatus : public core::ConnectionMonitor
{
public:
    UnvanquishedStatus(const Settings& settings, MessageConsumer* parent)
        : ConnectionMonitor("status", settings, parent)
    {
        synopsis += " [filter]";
        help = "Shows detailed information on the players and server";
    }

protected:
    bool on_handle(network::Message& msg)
    {
        std::vector<user::User> users = monitored->get_users();

        if ( !msg.message.empty() )
        {
            string::FormatterAscii ascii;
            users.erase(std::remove_if(users.begin(), users.end(),
                [&ascii,&msg](const user::User& user) {
                    return msg.source->encode_to(user.name,ascii)
                        .find(msg.message) == std::string::npos;
            }),users.end());

            if ( users.empty() )
                reply_to(msg,"(No users match the query)");
            else
                print_users(msg,users);
            return true;
        }

        if ( !users.empty() )
            print_users(msg,users);

        static std::vector<std::string> server_info {
            "Players: $(1)$players$(-) active, $(1)$bots$(-) bots, $(1)$players$(-)/$(1)$max$(-) total",
            "Map: $(1)$map$(-)",
        };
        string::FormatterConfig cfg;
        auto props = monitored->pretty_properties();
        for ( const auto& info : server_info )
            reply_to(msg, cfg.decode(info).replaced(props));

        return true;
    }

    void print_users(const network::Message& msg,
                     const std::vector<user::User>& users) const
    {
        reply_to(msg, string::FormattedString() << string::FormatFlags::BOLD
            << string::Padding("ip address", 21, 0) << " "
            << string::Padding("ping", 10, 0) << " "
            << "score num name");
        for ( const auto& user : users )
            reply_to(msg, string::FormattedString()
                << string::Padding(user.host, 21, 0) << " "
                << string::Padding(user.property("ping"), 10, 0) << " "
                << string::Padding(user.property("score"), 5) << " "
                << string::Padding(user.local_id, 3, 0) << " "
                << monitored->decode(user.name)
            );
    }
};

} // namespace unvanquished
#endif // MELANOBOT_MODULES_XONOTIC_HANDLERS_UNVANQUISHED_HPP
