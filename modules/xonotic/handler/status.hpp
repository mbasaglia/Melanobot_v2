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
#ifndef XONOTIC_HANDLER_STATUS_HPP
#define XONOTIC_HANDLER_STATUS_HPP

#include <sstream>
#include "melanolib/string/stringutils.hpp"
#include "xonotic/xonotic.hpp"
#include "core/handler/connection_monitor.hpp"
#include "xonotic/xonotic-connection.hpp"
#include "string/replacements.hpp"

namespace xonotic {

/**
 * \brief Lists players in a xonotic server
 */
class ListPlayers : public core::ConnectionMonitor
{
public:
    ListPlayers(const Settings& settings, MessageConsumer* parent)
        : ConnectionMonitor("who", settings, parent)
    {
        bots = settings.get("bots", bots);
        reply = read_string(settings, "reply", "$(1)$players$(-)/$(1)$max$(-): ");
        reply_empty = read_string(settings, "reply_empty", "Server is empty");
        help = "Shows the players on the server";
    }

protected:
    bool on_handle(network::Message& msg)
    {
        auto users = monitored->get_users();
        auto props = monitored->pretty_properties();

        std::vector<string::FormattedString> list;
        for ( const user::User& user: users )
            if ( bots || !user.host.empty() )
                list.push_back(monitored->decode(user.name));

        if ( list.empty() )
            reply_to(msg, reply_empty.replaced(props));
        else
            reply_to(msg, reply.replaced(props)
                << string::implode(string::FormattedString(", "), list));

        return true;
    }

private:
    bool                    bots{false};    ///< Show bots
    string::FormattedString reply;          ///< Reply when there are players (followed by the list)
    string::FormattedString reply_empty;    ///< Reply when there are no players
};

/**
 * \brief Lists detailed info on server and players
 */
class XonoticStatus : public core::ConnectionMonitor
{
public:
    XonoticStatus(const Settings& settings, MessageConsumer* parent)
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
                [&ascii, &msg](const user::User& user) {
                    return msg.source->encode_to(user.name, ascii)
                        .find(msg.message) == std::string::npos;
            }), users.end());

            if ( users.empty() )
                reply_to(msg, "(No users match the query)");
            else
                print_users(msg, users);
            return true;
        }

        if ( !users.empty() )
            print_users(msg, users);

        static std::vector<std::string> server_info {
            "Players: $(1)$active$(-) active, $(1)$spectators$(-) spectators, $(1)$bots$(-) bots, $(1)$players$(-)/$(1)$max$(-) total",
            "Map: $(1)$map$(-), Game: $(1)$gametype$(-), Mutators: $mutators",
        };
        auto props = monitored->pretty_properties();
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

        string::FormatterConfig cfg;
        for ( const auto& info : server_info )
            reply_to(msg, cfg.decode(info).replaced(props));

        return true;
    }

    void print_users(const network::Message& msg,
                     const std::vector<user::User>& users) const
    {
        reply_to(msg, string::FormattedString() << string::FormatFlags::BOLD
            << string::Padding("ip address", 21, 0) << " "
            << "pl ping frags slot name");
        for ( const auto& user : users )
            reply_to(msg, string::FormattedString()
                << string::Padding(user.host, 21, 0) << " "
                << string::Padding(user.property("pl"), 2) << " "
                << string::Padding(user.property("ping"), 4) << " "
                << string::Padding(user.property("frags"), 5) << " "
                << " #" << string::Padding(user.property("entity"), 2, 0) << " "
                << monitored->decode(user.name)
            );
    }
};

/**
 * \brief Lists xonotic maps matching a query
 */
class XonoticMaps : public core::ConnectionMonitor
{
public:
    XonoticMaps(const Settings& settings, MessageConsumer* parent)
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
            xon->add_polling_command({"rcon", {"g_maplist"}});
    }

protected:
    virtual std::vector<std::string>  get_maps() const
    {
        return melanolib::string::regex_split(
            monitored->properties().get("cvar.g_maplist"), "\\s+");
    }

    bool on_handle(network::Message& msg)
    {
        auto maps = get_maps();
        int total = maps.size();
        if ( !msg.message.empty() )
        {
            if ( regex )
            {
                try {
                    std::regex pattern(msg.message);
                    maps.erase(std::remove_if(maps.begin(), maps.end(),
                        [&pattern](const std::string& map){
                            return !std::regex_search(map, pattern);
                    }), maps.end());
                } catch (const std::regex_error& err) {
                    ErrorLog("sys", "RegEx Error") << err.what();
                    maps.clear();
                }
            }
            else
            {
                maps.erase(std::remove_if(maps.begin(), maps.end(),
                    [&msg](const std::string& map) {
                        return map.find(msg.message) == std::string::npos;
                }), maps.end());
            }
        }

        string::FormattedString str;
        str << color::red << maps.size() << color::nocolor << "/"
            << color::red << total << color::nocolor << " maps match";
        reply_to(msg, std::move(str));

        auto r = msg.destination->formatter()->to_string(color::red);
        auto nc = msg.destination->formatter()->to_string(color::nocolor);
        if ( max_print >= 0 && int(maps.size()) <= max_print && !maps.empty())
            reply_to(msg, r + melanolib::string::implode(nc + ", " + r, maps));

        return true;
    }

private:
    bool regex = false;
    int max_print = 6;
};


/**
 * \brief Manage xonotic bans
 */
class XonoticBan : public core::ConnectionMonitor
{
public:
    XonoticBan(const Settings& settings, MessageConsumer* parent)
        : ConnectionMonitor("ban", settings, parent)
    {
        synopsis += "$(-) refresh | list | rm $(-i)banid$(-)... | "
            "(ip $(-i)address$(-) | player (##$(-i)entity$(-)|name)) [$(-i)duration$(-) [:$(-i)reason$(-)]]";
        help = "Manage xonotic bans";
    }

    void initialize() override
    {
        if ( auto xon = dynamic_cast<xonotic::XonoticConnection*>(monitored) )
            xon->add_polling_command({"rcon", {"banlist"}});
    }

protected:
    bool on_handle(network::Message& msg)
    {
        if ( msg.message == "refresh" )
        {
            refresh();
            reply_to(msg, "Ban list refreshed");
            return true;
        }
        if ( msg.message.empty() || msg.message == "list" )
        {
            show_bans(msg);
            return true;
        }
        else if ( melanolib::string::starts_with(msg.message, "rm ") )
        {
            unban(msg);
            return true;
        }

        static std::regex regex_ban(
            //                       1=entity     2=name              3=address        4=time     5=reason
            R"((?:(?:player\s+(?:(?:#([0-9]+))|([a-zA-Z0-9]+)))|(?:ip\s+(\S+)))(?:\s+([^:]+)(?::\s*(.*))?)?)",
            // |  |           |  |   ^number^| ^-name-------^|| |       ^ip-^|||     ^time-^|reason^--^| |
            // |  |           |  ^-#entity---^               || |            |||            ^-reason---^ |
            // |  |           ^-player identifier------------^| |            ||^-extra info--------------^
            // |  ^-player------------------------------------^ ^-ip---------^|
            // ^-selection (player|ip)----------------------------------------^
            std::regex::ECMAScript|std::regex::optimize);

        std::smatch match;
        if ( std::regex_match(msg.message, match, regex_ban) )
        {
            if ( match[1].matched || match[2].matched )
                kickban(msg, match);
            else
                ban(msg, match);
            return true;
        }

        reply_to(msg, "Invalid call, see help for usage"); // AKA RTFM
        return true;
    }

private:
    friend class XonoticKick;

    /**
     * \brief Finds a user to kickban
     */
    static melanolib::Optional<user::User> find_user(network::Connection*monitored, const std::smatch& match)
    {
        std::vector<user::User> users = monitored->get_users();
        auto kicked = users.end();
        // find user with given entity number
        if ( match[1].matched )
        {
            kicked = std::find_if(users.begin(), users.end(),
                [&match](const user::User& user) {
                    return user.property("entity") == match[1];
                });
        }
        // find user with given name
        else
        {
            string::FormatterAscii ascii;
            kicked = std::find_if(users.begin(), users.end(),
                [&match, &ascii, monitored](const user::User& user) {
                    return monitored->encode_to(user.name, ascii)
                        .find(match[2]) == std::string::npos;
                });
        }
        if ( kicked == users.end() )
            return {};
        return *kicked;
    }

    /**
     * \brief Handles a kickban
     */
    void kickban(const network::Message& msg, const std::smatch& match)
    {
        if ( auto kicked = find_user(monitored, match) )
        {
            std::vector<std::string> params = {"kickban",
                "#"+kicked->property("entity") };
            string::FormattedString notice;
            notice << "Banning #" << kicked->property("entity") << " "
                << kicked->host << " " << monitored->decode(kicked->name);
            if ( match[4].matched )
            {
                params.push_back(std::to_string(
                    std::chrono::duration_cast<melanolib::time::seconds>(
                        melanolib::time::parse_duration(match[4])
                ).count()));
                notice << " for "+params[2]+" seconds";
                if ( match[5].matched )
                    params.push_back(match[5]);
            }
            reply_to(msg, std::move(notice));
            monitored->command({"rcon", params, priority});
            refresh();
        }
        else
        {
            reply_to(msg, "Player not found");
        }
    }

    /**
     * \brief Handles a ban
     */
    void ban(const network::Message& msg, const std::smatch& match)
    {
        std::vector<std::string> params = {"ban", match[3]};
        std::string notice = "Banning "+params[1];
        if ( match[4].matched )
        {
            params.push_back(std::to_string(
                std::chrono::duration_cast<melanolib::time::seconds>(
                    melanolib::time::parse_duration(match[4])
                ).count()));
            notice += " for "+params[2]+" seconds";
            if ( match[5].matched )
                params.push_back(match[5]);
        }
        reply_to(msg, notice);
        monitored->command({"rcon", params, priority});
        refresh();
    }

    /**
     * \brief Removes bans
     */
    void unban(const network::Message& msg)
    {
        std::stringstream ss(msg.message);
        ss.ignore(3);
        char c;
        std::string id;
        while ( ss )
        {
            ss >> c;
            if ( c != '#' )
                ss.unget();
            if ( !(ss >> id) )
                break;
            monitored->command({"rcon", {"unban", "#"+id}, priority});
        }
        reply_to(msg, "Removing given bans");
        refresh();
    }

    /**
     * \brief Prints the active bans
     */
    void show_bans(const network::Message& msg)
    {
        auto banlist = monitored->properties().get_child("banlist");
        if ( banlist.empty() )
        {
            reply_to(msg, "No active bans");
            return;
        }

        using namespace string;
        
        for ( const auto& ban : banlist )
            reply_to(msg, FormattedString()
                << color::red << Padding("#"+ban.first, 4) << " "
                << color::dark_cyan << Padding(ban.second.get("ip", ""), 16, 0) << " "
                << color::nocolor << Padding(ban.second.get("time", "?"), 6)
                << " seconds"
            );
    }

    /**
     * \brief Updates the ban list
     */
    void refresh()
    {
        // defer 1 because apparently the server doesn't update bans right away
        monitored->command({"rcon", {"defer", "1", "banlist"}, priority});
    }
};

/**
 * \brief Kicks players
 */
class XonoticKick : public core::ConnectionMonitor
{
public:
    XonoticKick(const Settings& settings, MessageConsumer* parent)
        : ConnectionMonitor("kick", settings, parent)
    {
        synopsis += "$(-)##$(-i)entity$(-)|name";
        help = "Kicks a player";
    }


protected:
    bool on_handle(network::Message& msg)
    {
        static std::regex regex_kick(
            //               1=entity     2=name
            R"(\s*(?:(?:#([0-9]+))|([a-zA-Z0-9]+)))",
            //    |  |   ^number^| ^-name-------^|
            //    |  ^-#entity---^               |
            //    ^-player identifier------------^
            std::regex::ECMAScript|std::regex::optimize);

        std::smatch match;
        if ( std::regex_match(msg.message, match, regex_kick) )
            kick(msg, match);
        else
            reply_to(msg, "Invalid call, see help for usage"); // AKA RTFM

        return true;
    }

private:

    /**
     * \brief Handles a kick
     */
    void kick(const network::Message& msg, const std::smatch& match)
    {
        if ( auto kicked = XonoticBan::find_user(monitored, match) )
        {
            std::vector<std::string> params = {"kick",
                "# "+kicked->property("entity") };
            string::FormattedString notice;
            notice << "Kicking #" << kicked->property("entity") << " "
                << kicked->host << " " << monitored->decode(kicked->name);
            reply_to(msg, std::move(notice));
            monitored->command({"rcon", params, priority});
        }
        else
        {
            reply_to(msg, "Player not found");
        }
    }
};


} // namespace xonotic
#endif // XONOTIC_HANDLER_STATUS_HPP
