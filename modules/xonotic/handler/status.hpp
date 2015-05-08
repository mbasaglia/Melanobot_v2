/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015 Mattia Basaglia
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
#ifndef XONOTIC_HANDLER_STATUS_HPP
#define XONOTIC_HANDLER_STATUS_HPP

#include "string/string_functions.hpp"
#include "xonotic/xonotic.hpp"
#include "core/handler/connection_monitor.hpp"
#include "xonotic/xonotic-connection.hpp"

namespace xonotic {

/**
 * \brief Lists players in a xonotic server
 */
class ListPlayers : public handler::ConnectionMonitor
{
public:
    ListPlayers(const Settings& settings, handler::HandlerContainer* parent)
        : ConnectionMonitor("who", settings, parent)
    {
        bots = settings.get("bots", bots);
        help = "Shows the players on the server";
    }

protected:
    bool on_handle(network::Message& msg)
    {
        auto users = monitored->get_users();
        Properties props = monitored->message_properties();

        string::FormatterConfig fmt;

        std::vector<string::FormattedString> list;
        for ( const user::User& user: users )
            if ( bots || !user.host.empty() )
                list.push_back(monitored->decode(user.name));

        if ( list.empty() )
            reply_to(msg,fmt.decode(string::replace(reply_empty, props, "%")));
        else
            reply_to(msg,fmt.decode(string::replace(reply, props, "%"))
                << string::implode(string::FormattedString(", "),list));

        return true;
    }


    bool        bots{false};                            ///< Show bots
    std::string reply{"#1#%players#-#/#1#%max#-#: "};   ///< Reply when there are players (followed by the list)
    std::string reply_empty{"Server is empty"};         ///< Reply when there are no players
};

/**
 * \brief Lists detailed info on server and players
 */
class XonoticStatus : public handler::ConnectionMonitor
{
public:
    XonoticStatus(const Settings& settings, handler::HandlerContainer* parent)
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
            users.erase(std::remove_if(users.begin(),users.end(),
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
        string::FormatterConfig fmt;

        static std::vector<std::string> server_info {
            "Players: #1#%active#-# active, #1#%spectators#-# spectators, #1#%bots#-# bots, #1#%players#-#/#1#%max#-# total",
            "Map: #1#%map#-#, Game: #1#%gametype#-#, Mutators: %mutators",
        };
        auto props = monitored->message_properties();
        int active = 0;
        int spectators = 0;
        for ( const auto& user : users )
        {
            if ( !user.host.empty() )
            {
                if ( user.property("frags") == "-666" )
                    spectators++;
                else
                    active++;
            }
        }
        props["active"] = std::to_string(active);
        props["spectators"] = std::to_string(spectators);

        for ( const auto& info : server_info )
            reply_to(msg, fmt.decode(string::replace(info,props,"%")));

        return true;
    }

    void print_users(const network::Message& msg,
                     const std::vector<user::User>& users) const
    {
        reply_to(msg, string::FormattedString() << string::FormatFlags::BOLD
            << string::FormattedString("ip address").pad(21,0) << " "
            << "pl ping frags slot name");
        for ( const auto& user : users )
            reply_to(msg, string::FormattedString()
                << string::FormattedString(user.host).pad(21,0) << " "
                << string::FormattedString(user.property("pl")).pad(2) << " "
                << string::FormattedString(user.property("ping")).pad(4) << " "
                << string::FormattedString(user.property("frags")).pad(5) << " "
                << " #" << string::FormattedString(user.property("entity")).pad(2,0) << " "
                << monitored->decode(user.name)
            );
    }
};


/**
 * \brief Lists xonotic maps matching a query
 */
class XonoticMaps : public handler::ConnectionMonitor
{
public:
    XonoticMaps(const Settings& settings, handler::HandlerContainer* parent)
        : ConnectionMonitor("maps", settings, parent)
    {
        help = "Shows the maps on the server";
        synopsis += " [query]";
        max_print = settings.get("max_print", max_print);
        regex = settings.get("regex", regex);
    }

    void initialize() override
    {
        if ( auto xon = dynamic_cast<xonotic::XonoticConnection*>(monitored) )
            xon->add_polling_command({"rcon",{"g_maplist"}});
    }

protected:
    bool on_handle(network::Message& msg)
    {
        auto maps = string::regex_split(monitored->get_property("cvar.g_maplist"),"\\s+");
        int total = maps.size();
        if ( !msg.message.empty() )
        {
            if ( regex )
            {
                try {
                    std::regex pattern(msg.message);
                    maps.erase(std::remove_if(maps.begin(),maps.end(),
                        [&pattern](const std::string& map){
                            return !std::regex_search(map,pattern);
                    }),maps.end());
                } catch (const std::regex_error& err) {
                    ErrorLog("sys", "RegEx Error") << err.what();
                    maps.clear();
                }
            }
            else
            {
                maps.erase(std::remove_if(maps.begin(),maps.end(),
                    [&msg](const std::string& map) {
                        return map.find(msg.message) == std::string::npos;
                }),maps.end());
            }
        }

        string::FormattedString str;
        str << color::red << maps.size() << color::nocolor << "/"
            << color::red << total << color::nocolor << " maps match";
        reply_to(msg,str);

        auto r = string::Color(color::red).to_string(*msg.destination->formatter());
        auto nc = string::Color(color::nocolor).to_string(*msg.destination->formatter());
        if ( max_print >= 0 && int(maps.size()) <= max_print && !maps.empty())
            reply_to(msg,r+string::implode(nc+", "+r,maps));

        return true;
    }

    bool regex = false;
    int max_print = 6;
};


} // namespace xonotic
#endif // XONOTIC_HANDLER_STATUS_HPP
