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
#include "xonotic-connection.hpp"

#include <ctime>

#include <boost/format.hpp>

#include <openssl/hmac.h>

#include "melanolib/math/math.hpp"

namespace xonotic {

std::unique_ptr<XonoticConnection> XonoticConnection::create(
    const Settings& settings, const std::string& name)
{
    if ( settings.get("protocol", std::string()) != "xonotic" )
    {
        /// \todo accept similar protocols? eg: nexuiz
        throw melanobot::ConfigurationError("Wrong protocol for Xonotic connection");
    }

    network::Server server ( settings.get("server", std::string()) );
    if ( !server.port )
        server.port = 26000;
    server.host = settings.get("server.host", server.host);
    server.port = settings.get("server.port", server.port);
    if ( server.host.empty() || !server.port )
    {
        throw melanobot::ConfigurationError("Xonotic connection with no server");
    }

    auto password = settings.get("rcon_password", "");
    auto secure= Darkplaces::Secure(settings.get("rcon_secure", 0));
    return melanolib::New<XonoticConnection>(server, password, secure, settings, name);
}

XonoticConnection::XonoticConnection (
    const network::Server&  server,
    const std::string&      password,
    Darkplaces::Secure      secure,
    const Settings&         settings,
    const std::string&      name
)
    : Connection(name),
      Darkplaces(server, password, secure),
      status_(DISCONNECTED)
{
    formatter_ = string::Formatter::formatter(
        settings.get("string_format", std::string("xonotic")));

    cmd_say = settings.get("say", "say $prefix$message");
    cmd_say_as = settings.get("say_as", "say \"$prefix$from^7: $message\"");
    cmd_say_action = settings.get("say_action", "say \"^4* ^3$prefix$from^3 $message\"");

    // Preset templates
    if ( cmd_say_as == "modpack" )
    {
        cmd_say = "sv_cmd ircmsg $prefix$message";
        cmd_say_as = "sv_cmd ircmsg $prefix$from: $message";
        cmd_say_action = "sv_cmd ircmsg ^4* ^3$prefix$from^3 $message";
    }
    else if ( cmd_say_as == "sv_adminnick" )
    {
        cmd_say =   "Melanobot_nick_push;"
                    "set sv_adminnick \"^3$prefix^3\";"
                    "say ^7$prefix$message;"
                    "Melanobot_nick_pop";

        cmd_say_as ="Melanobot_nick_push;"
                    "set sv_adminnick \"^3$prefix^3\";"
                    "say ^7$from: $message;"
                    "Melanobot_nick_pop";

        cmd_say_action ="Melanobot_nick_push;"
                    "set sv_adminnick \"^3$prefix^3\";"
                    "say ^4* ^3$from^3 $message;"
                    "Melanobot_nick_pop";
    }

    add_polling_command({"rcon", {"status 1"}, 1024}, true);
    add_polling_command({"rcon", {"log_dest_udp"}, 1024}, true);
    status_polling = network::Timer{[this]{request_status();},
        melanolib::time::seconds(settings.get("status_delay", 60))};
}

void XonoticConnection::connect()
{
    if ( !Darkplaces::connected() )
    {
        // status: DISCONNECTED -> WAITING
        status_ = WAITING;

        // Just connected, clear all
        auto lock = make_lock(mutex);
            properties_.erase("cvar");// cvars might have changed
            clear_match();
            user_manager.clear();
        lock.unlock();

        Darkplaces::connect();
    }
}

void XonoticConnection::on_connect()
{
    update_connection();
}

void XonoticConnection::disconnect(const string::FormattedString& message)
{
    // status: * -> DISCONNECTED
    status_polling.stop();
    if ( Darkplaces::connected() )
    {
        if ( !message.empty() && status_ > CONNECTING )
            say({message});
        cleanup_connection();
    }
    close_connection();
    auto lock = make_lock(mutex);
    clear_match();
    user_manager.clear();
    lock.unlock();
}

void XonoticConnection::update_user(const std::string& local_id,
                                    const Properties& properties)
{
    auto lock = make_lock(mutex);
    user::User* user = user_manager.user(local_id);
    if ( user )
    {
        user->update(properties);

        /// \todo this might not be needed if crypto_id isn't good enough
        auto it = properties.find("global_id");
        if ( it != properties.end() )
            Log("xon", '!', 3) << "Player " << color::dark_cyan << user->local_id
                << color::nocolor << " is authed as " << color::cyan << it->second;
    }
}

void XonoticConnection::update_user(const std::string& local_id,
                                   const user::User& updated)
{
    auto lock = make_lock(mutex);
    user::User* user = user_manager.user(local_id);
    if ( user )
    {
        *user = updated;

        if ( !updated.global_id.empty() )
            Log("xon", '!', 3) << "User " << color::dark_cyan << user->local_id
                << color::nocolor << " is authed as " << color::cyan << updated.global_id;
    }
}

user::User XonoticConnection::get_user(const std::string& local_id) const
{
    if ( local_id.empty() )
        return {};

    auto lock = make_lock(mutex);

    if ( local_id == "0" || local_id == "#0" )
    {
        user::User user;
        user.origin = const_cast<XonoticConnection*>(this);
        user.local_id = "0";
        user.properties["entity"] = "0";
        user.host = server().name();
        user.name = properties_.get("cvar.sv_adminnick", "");
        if ( user.name.empty() )
            user.name = "(Server Admin)";
        return user;
    }

    const user::User* user = local_id[0] == '#' ?
        user_manager.user_by_property("entity", local_id.substr(1))
        : user_manager.user(local_id);

    if ( user )
        return *user;

    return {};
}

std::vector<user::User> XonoticConnection::get_users( const std::string& channel_mask ) const
{
    auto lock = make_lock(mutex);
    auto list = user_manager.users();
    return {list.begin(), list.end()};
}

std::string XonoticConnection::name() const
{
    auto lock = make_lock(mutex);
    return properties_.get("cvar.sv_adminnick", "");
}

user::UserCounter XonoticConnection::count_users(const std::string& channel) const
{
    auto lock = make_lock(mutex);
    user::UserCounter c;
    for ( const auto& user : user_manager )
        (user.host.empty() ? c.bots : c.users)++;
    c.max = properties_.get("cvar.g_maxplayers", 0);
    return c;
}

LockedProperties XonoticConnection::properties()
{
    return LockedProperties(&mutex, &properties_);
}

string::FormattedProperties XonoticConnection::pretty_properties() const
{
    user::UserCounter count = count_users();

    auto lock = make_lock(mutex);

    std::string gt = properties_.get("match.gametype", "?");
    if ( count.max == 2 )
        gt = "duel";

    string::FormattedString host;
    if ( auto opt = properties_.get_optional<std::string>("host") )
        host = formatter_->decode(*opt);
    else
        host = "(unconnected) " + server().name();

    return string::FormattedProperties {
        {"players",     std::to_string(count.users)},
        {"bots",        std::to_string(count.bots)},
        {"total",       std::to_string(count.users+count.bots)},
        {"max",         std::to_string(count.max)},
        {"free",        std::to_string(count.max-count.users)},
        {"map",         properties_.get("match.map", "?")},
        {"gt",          gt},
        {"gametype",    xonotic::gametype_name(gt)},
        {"mutators",    properties_.get("match.mutators", "")},
        {"hostname",    host},
        {"sv_server",   server().name()}
    };
}

void XonoticConnection::say(const network::OutputMessage& message)
{
    string::FormattedString prefix_stream;
    if ( !message.prefix.empty() )
        prefix_stream << message.prefix << ' ' << color::nocolor;

    Properties message_properties = {
        {"prefix",  prefix_stream.encode(*formatter_)},
        {"from",    (message.from.copy() << color::nocolor).encode(*formatter_)},
        {"message", (message.message.copy() << color::nocolor).encode(*formatter_)},
    };

    auto expl = melanolib::string::regex_split(
        message.action ? cmd_say_action :
            ( message.from.empty() ? cmd_say : cmd_say_as ),
        ";\\s*");
    for ( const auto & cmd : expl )
        command({"rcon", { melanolib::string::replace(cmd, message_properties, "$")},
                message.priority, message.timeout});
}

void XonoticConnection::command ( network::Command cmd )
{
    // discards commands sent too early
    if ( status_ == DISCONNECTED )
        return;

    if ( cmd.command == "rcon" )
    {
        if ( cmd.parameters.empty() )
            return;

        if ( cmd.parameters[0] == "set" || cmd.parameters[0] == "alias" )
        {
            if ( cmd.parameters.size() != 3 )
            {
                ErrorLog("xon") << "Wrong parameters for \""+cmd.parameters[0]+"\"";
                return;
            }
            if ( cmd.parameters[0] == "set" )
            {
                auto lock = make_lock(mutex);
                properties_.put("cvar."+cmd.parameters[1], cmd.parameters[2]);
            }
            cmd.parameters[2] = quote_string(cmd.parameters[2]);
        }

        std::string command = melanolib::string::implode(" ", cmd.parameters);
        if ( command.empty() )
        {
            ErrorLog("xon") << "Empty rcon command";
            return;
        }

        rcon_command(command);
        /// \todo Log("xon", '<', 2) << decode(rcon_command); when actually being executed
    }
    else
    {
        Log("xon", '<', 1) << cmd.command;
        write(cmd.command);
    }
}

void XonoticConnection::on_network_error(const std::string& message)
{
    ErrorLog("xon", "Network Error") << message;
    close_connection();
    return;
}

void XonoticConnection::on_network_input(const std::string&)
{
    // status: WAITING -> CONNECTING
    if ( status_ == WAITING )
        status_ = CONNECTING;
}

void XonoticConnection::on_receive(const std::string& command,
                                   const std::string& message)
{

    network::Message msg;
    msg.command = command;
    msg.raw = msg.command+' '+message;
    msg.params.push_back(message);
    handle_message(msg);
}

void XonoticConnection::on_receive_log(const std::string& line)
{
    network::Message msg;
    msg.command = "n"; // log
    msg.raw = line;
    handle_message(msg);
}

void XonoticConnection::handle_message(network::Message& msg)
{
    Log("xon", '>', 4) << formatter_->decode(msg.raw); // 4 'cause spams
    if ( msg.raw.empty() )
        return;

    if ( msg.command == "n" && !msg.raw.empty() )
    {
        // chat
        if ( msg.raw[0] == '\1' )
        {
            static std::regex regex_chat("^\1(.*)\\^7: (.*)",
                std::regex::ECMAScript|std::regex::optimize
            );
            std::smatch match;
            if ( std::regex_match(msg.raw, match, regex_chat) )
            {
                msg.from.name = match[1];
                msg.chat(match[2]);
            }
        }
        // cvars
        else if ( msg.raw[0] == '"' )
        {
            // Note cvars can actually have any characters in their identifier
            // and values, this regex limits to:
            //  * The identifier to contain only alphanumeric or + - _
            //  * The value anything but "
            static std::regex regex_cvar(
                R"cvar("([^"]+)" is "([^"]*)".*)cvar",
                std::regex::ECMAScript|std::regex::optimize
            );
            std::smatch match;
            if ( std::regex_match(msg.raw, match, regex_cvar) )
            {
                std::string cvar_name = match[1];
                std::string cvar_value = match[2];
                auto lock = make_lock(mutex);
                properties_.put("cvar."+cvar_name, cvar_value);
                lock.unlock();

                if ( cvar_name == "log_dest_udp" )
                {
                    // status: INVALID    -> CONNECTING
                    // status: CONNECTING -> CONNECTED + message
                    // status: CHECKING   -> CONNECTED
                    if ( cvar_value != local_endpoint().name() )
                    {
                        update_connection();
                    }
                    else if ( status_ == CONNECTING )
                    {
                        check_user_end();
                        status_ = CONNECTED;
                        network::Message().connected().send(this);
                    }
                    else
                    {
                        check_user_end();
                        status_ = CONNECTED;
                    }
                }
            }
        }
        // join/part
        else if ( msg.raw[0] == ':' )
        {
            static std::regex regex_join(
                R"regex(:join:(\d+):(\d+):((?:[0-9]+(?:\.[0-9]+){3})|(?:[[:xdigit:]](?::[[:xdigit:]]){7})|bot):(.*))regex",
                std::regex::ECMAScript|std::regex::optimize
            );
            static std::regex regex_part(
                R"regex(:part:(\d+))regex",
                std::regex::ECMAScript|std::regex::optimize
            );
            static std::regex regex_gamestart(
                R"regex(:gamestart:([a-z]+)_([^:]*):[0-9.]*)regex",
                std::regex::ECMAScript|std::regex::optimize
            );
            static std::regex regex_name(
                R"regex(:name:(\d+):(.*))regex",
                std::regex::ECMAScript|std::regex::optimize
            );
            static std::regex regex_mutators(
                R"regex(:gameinfo:mutators:LIST:(.*))regex",
                std::regex::ECMAScript|std::regex::optimize
            );

            std::smatch match;
            if ( std::regex_match(msg.raw, match, regex_join) )
            {
                user_manager.users_reference().remove_if(
                    [match](const user::User& u) {
                        return u.property("entity") == match[2]; });
                user::User user;
                user.origin = this;
                user.local_id = match[1]; // playerid
                user.properties["entity"] = match[2]; // entity number
                if ( match[3] != "bot" )
                    user.host = match[3];
                user.name = match[4];
                auto lock = make_lock(mutex);
                user_manager.add_user(user);
                lock.unlock();
                msg.from = user;
                msg.type = network::Message::JOIN;
                Log("xon", '!', 3) << "Added user " << decode(user.name);
            }
            else if ( std::regex_match(msg.raw, match, regex_part) )
            {
                auto lock = make_lock(mutex);
                if ( user::User *found = user_manager.user(match[1]) )
                {
                    Log("xon", '!', 3) << "Removed user " << decode(found->name);
                    msg.from = *found;
                    user_manager.remove_user(found->local_id);
                    msg.type = network::Message::PART;
                }
            }
            else if ( std::regex_match(msg.raw, match, regex_gamestart) )
            {
                auto lock = make_lock(mutex);
                clear_match();
                //check_user_start();
                msg.command = "gamestart";
                msg.params = { match[1], match[2] };
                properties_.put<std::string>("match.gametype", match[1]);
                properties_.put<std::string>("match.map", match[2]);
                for ( const auto& cmd : polling_match )
                    command(cmd);
            }
            else if ( std::regex_match(msg.raw, match, regex_name) )
            {
                auto lock = make_lock(mutex);
                if ( user::User *found = user_manager.user(match[1]) )
                {
                    msg.from = *found;
                    msg.message = match[2];
                    msg.type = network::Message::RENAME;
                    Log("xon", '!', 3) << "Renamed user " << decode(found->name)
                        << color::nocolor << " to " << decode(msg.message);
                    found->name = msg.message;
                }
            }
            else if ( std::regex_match(msg.raw, match, regex_mutators) )
            {
                auto lock = make_lock(mutex);
                properties_.put("match.mutators", melanolib::string::replace(match[1], ":", ", "));
            }
        }
        // status reply
        else if ( status_ == CHECKING || status_ == CONNECTING )
        {
            static std::regex regex_status1_players(
                R"(players:  \d+ active \((\d+) max\))",
                std::regex::ECMAScript|std::regex::optimize
            );
            static std::regex regex_status1(
                "([a-z]+):\\s+(.*)",
                std::regex::ECMAScript|std::regex::optimize
            );
            static std::regex regex_status1_player(
                //rowcol  IP      $pl    ping    time    frags   entity     name
                R"(\^[37](\S+)\s+(\S+)\s+(\S+)\s+(\S+)\s+(\S+)\s+#([0-9]+)\s+\^7(.*))",
                std::regex::ECMAScript|std::regex::optimize
            );
            std::smatch match;
            if ( std::regex_match(msg.raw, match, regex_status1_players) )
            {
                properties_.put<std::string>("cvar.g_maxplayers", match[1]);
            }
            else if ( std::regex_match(msg.raw, match, regex_status1) )
            {
                std::string status_name = match[1];
                std::string status_value = match[2];
                auto lock = make_lock(mutex);
                if ( status_name == "map" )
                    status_name = "match.map";
                properties_.put(status_name, status_value);
            }
            else if ( std::regex_match(msg.raw, match, regex_status1_player) )
            {
                check_user(match);
            }
        }
    }
    msg.send(this);
}

void XonoticConnection::update_connection()
{
    // status: WAITING -> CONNECTING
    if ( status_ > DISCONNECTED )
    {
        status_ = WAITING;

        command({"rcon", {"set", "log_dest_udp", local_endpoint().name()}, 1024});

        command({"rcon", {"set", "sv_eventlog", "1"}, 1024});

        /// \todo maybe this should be an option
        command({"rcon", {"set", "sv_logscores_bots", "1"}, 1024});

        command({"rcon", {"alias", "Melanobot_nick_push", "set Melanobot_sv_adminnick \"$sv_adminnick\""}, 1024});
        command({"rcon", {"alias", "Melanobot_nick_pop", "set sv_adminnick \"$Melanobot_sv_adminnick\"; sv_adminnick"}, 1024});

        // status: WAITING -> CONNECTING
        if ( status_ == WAITING )
            sync_read();

        // Make sure the connection is being checked
        if ( !status_polling.running() )
            status_polling.start();

        /// \todo should:
        ///     * wait while status_ == WAITING
        ///     * return if status_ == DISCONNECTED
        ///     * keep going if status_ >= CONNECTING
        // Commands failed
        if ( status_ < CONNECTING )
            return;

        // Request status right away (will also be called later by status_polling)
        request_status();

        // Run match commands as well
        for ( const auto& cmd : polling_match )
            command(cmd);
    }
}

void XonoticConnection::cleanup_connection()
{
    command({"rcon", {"set", "log_dest_udp", ""}, 1024});

    // try to actually clear log_dest_udp when challenges are required
    if ( rcon_secure() >= Darkplaces::Secure::CHALLENGE )
        sync_read();

    auto lock = make_lock(mutex);
    properties_.erase("cvar");
    clear_match();
}

void XonoticConnection::request_status()
{
    // status: CONNECTING  -> CONNECTING
    // status: CONNECTED   -> CHECKING
    // status: DISCONNECTED-> WAITING
    if ( status_ >= CONNECTING )
    {
        if ( status_ == CONNECTED )
            status_ = CHECKING;
        check_user_start();
        for ( const auto& cmd : polling_status )
            command(cmd);
    }
    else
    {
        close_connection();
        connect();
    }
}

void XonoticConnection::add_polling_command(const network::Command& command, bool continuous)
{
    auto& list = (continuous ? polling_status : polling_match);
    for ( const auto& cmd : list )
        if ( cmd.command == command.command && cmd.parameters == command.parameters )
            return;
    list.push_back(command);
}

void XonoticConnection::close_connection()
{
    // status: CONNECTED|CHECKING -> DISCONNECTED + message
    // status: * -> DISCONNECTED
    if ( status_ > CONNECTING )
        network::Message().disconnected().send(this);
    status_ = DISCONNECTED;
    if ( Darkplaces::connected() )
        Darkplaces::disconnect();
}

void XonoticConnection::clear_match()
{
    properties_.erase("match");
}

void XonoticConnection::check_user_start()
{
    // Marks all users as unchecked
    auto lock = make_lock(mutex);
    for ( user::User& user : user_manager )
        user.checked = false;
}

/**
 * \pre \c match a successful result of \c regex_status1_player:
 *      * \b 1 IP|botclient
 *      * \b 2 $pl
 *      * \b 3 ping
 *      * \b 4 time
 *      * \b 5 frags
 *      * \b 6 entity number (no #)
 *      * \b 7 name
 */
void XonoticConnection::check_user(const std::smatch& match)
{
    auto lock = make_lock(mutex);
    user::User* user = user_manager.user_by_property("entity", match[6]);
    Properties props = {
        {"host",   match[1] == "botclient" ? std::string(): match[1]},
        {"pl",     match[2]},
        {"ping",   match[3]},
        {"time",   match[4]},
        {"frags",  match[5]},
        {"entity", match[6]},
        {"name",   match[7]},
    };
    if ( user )
    {
        user->update(props);
        user->checked = true;
    }
    else
    {
        /// \todo Send join command
        user::User new_user;
        new_user.origin = this;
        new_user.update(props);
        user_manager.add_user(new_user);
    }
}

void XonoticConnection::check_user_end()
{
    // Removes all unchecked users
    /// \todo Send part command
    auto lock = make_lock(mutex);
    user_manager.users_reference().remove_if(
        [](const user::User& user) { return !user.checked; } );
}


} // namespace xonotic
