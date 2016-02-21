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

#include "unvanquished-connection.hpp"
#include "xonotic.hpp"

namespace unvanquished {


std::unique_ptr<UnvanquishedConnection> UnvanquishedConnection::create(
    const Settings& settings, const std::string& name)
{
    if ( settings.get("protocol", "") != "unvanquished" )
    {
        throw melanobot::ConfigurationError("Wrong protocol for Unvanquished connection");
    }

    network::Server server(settings.get("server", ""));
    if ( !server.port )
        server.port = 27960;
    server.host = settings.get("server.host", server.host);
    server.port = settings.get("server.port", server.port);
    if ( server.host.empty() || !server.port )
    {
        throw melanobot::ConfigurationError("Unvanquished connection with no server");
    }

    auto password = settings.get("rcon_password", "");
    return melanolib::New<UnvanquishedConnection>(server, password, settings, name);
}

UnvanquishedConnection::UnvanquishedConnection(
    const network::Server&  server,
    const std::string&      password,
    const Settings&         settings,
    const std::string&      name)
    : Connection(name),
      Daemon(server, password),
      status_(DISCONNECTED)
{
    formatter_ = string::Formatter::formatter(
        settings.get("string_format",std::string("xonotic"))); /// \todo Unv formatter

    cmd_say = string::FormatterConfig().decode(
        settings.get("say", "pr $to $prefix$message"));

    cmd_say_as = string::FormatterConfig().decode(
        settings.get("say_as", "pr $to $prefix$from: $message"));

    cmd_say_action = string::FormatterConfig().decode(
        settings.get("say_action", "pr $to ^4* ^*$prefix$from $message"));

    add_polling_command({"rcon", {"status"}, 1024});

    status_polling = network::Timer{
        [this]{request_status();},
        melanolib::time::seconds(settings.get("status_delay", 60))
    };
}

UnvanquishedConnection::~UnvanquishedConnection()
{
    stop();
}

void UnvanquishedConnection::connect()
{
    if ( !Daemon::connected() )
    {
        // status: DISCONNECTED -> WAITING (needs connections)
        status_ = WAITING;

        // Just connected, clear all
        auto lock = make_lock(mutex);
        user_manager.clear();
        lock.unlock();

        Daemon::connect();
    }
}

void UnvanquishedConnection::on_connect()
{
    // status: WAITING -> CONNECTING (needs rcon info)
    status_ = CONNECTING;
    Daemon::on_connect();
}

void UnvanquishedConnection::on_disconnect()
{
    // status: * -> DISCONNECTED
    status_ = DISCONNECTED;
}

void UnvanquishedConnection::disconnect(const string::FormattedString& message)
{
    // status: * -> DISCONNECTED
    status_polling.stop();
    if ( Daemon::connected() )
    {
        if ( !message.empty() && status_ > CONNECTING )
            say({message});
    }
    close_connection();

    auto lock = make_lock(mutex);
    user_manager.clear();
}

void UnvanquishedConnection::close_connection()
{
    // status: CONNECTED|CHECKING -> DISCONNECTED + message
    // status: * -> DISCONNECTED
    if ( status_ > CONNECTING )
        network::Message().disconnected().send(this);
    status_ = DISCONNECTED;
    if ( Daemon::connected() )
        Daemon::disconnect();
}

void UnvanquishedConnection::update_user(const std::string& local_id,
                                         const Properties& properties)
{
    auto lock = make_lock(mutex);
    user::User* user = user_manager.user(local_id);
    if ( user )
    {
        user->update(properties);

        auto it = properties.find("global_id");
        if ( it != properties.end() )
            Log("unv",'!',3) << "Player " << color::dark_cyan << user->local_id
                << color::nocolor << " is authed as " << color::cyan << it->second;
    }
}

void UnvanquishedConnection::update_user(const std::string& local_id,
                                         const user::User& updated)
{
    auto lock = make_lock(mutex);
    user::User* user = user_manager.user(local_id);
    if ( user )
    {
        *user = updated;

        if ( !updated.global_id.empty() )
            Log("unv",'!',3) << "User " << color::dark_cyan << user->local_id
                << color::nocolor << " is authed as " << color::cyan << updated.global_id;
    }
}

user::User UnvanquishedConnection::get_user(const std::string& local_id) const
{
    if ( local_id.empty() )
        return {};

    auto lock = make_lock(mutex);

    if ( local_id == "-1" )
    {
        user::User user;
        user.origin = const_cast<UnvanquishedConnection*>(this);
        user.local_id = "-1";
        user.host = server().name();
        user.name = properties_.get("cvar.sv_hostname","");
        if ( user.name.empty() )
            user.name = "(Server Admin)";
        return user;
    }

    if ( const user::User* user = user_manager.user(local_id) )
        return *user;

    return {};
}

std::vector<user::User> UnvanquishedConnection::get_users( const std::string& channel_mask ) const
{
    auto lock = make_lock(mutex);
    auto list = user_manager.users();
    return {list.begin(), list.end()};
}


std::string UnvanquishedConnection::name() const
{
    auto lock = make_lock(mutex);
    return properties_.get("cvar.sv_hostname", "");
}

user::UserCounter UnvanquishedConnection::count_users(const std::string& channel) const
{
    auto lock = make_lock(mutex);
    user::UserCounter c;
    for ( const auto& user : user_manager )
        (user.host.empty() ? c.bots : c.users)++;
    c.max = properties_.get("cvar.sv_maxclients",0);
    return c;
}

LockedProperties UnvanquishedConnection::properties()
{
    return LockedProperties(&mutex, &properties_);
}

string::FormattedProperties UnvanquishedConnection::pretty_properties() const
{
    user::UserCounter count = count_users();

    auto lock = make_lock(mutex);

    string::FormattedString host;
    if ( auto opt = properties_.get_optional<std::string>("cvar.sv_hostname") )
        host = formatter_->decode(*opt);
    else
        host = "(unconnected) " + server().name();

    return string::FormattedProperties {
        {"players",     std::to_string(count.users)},
        {"bots",        std::to_string(count.bots)},
        {"total",       std::to_string(count.users+count.bots)},
        {"max",         std::to_string(count.max)},
        {"free",        std::to_string(count.max-count.users)},
        {"map",         properties_.get("map", "?")},
        {"hostname",    host},
        {"server",      server().name()}
    };
}

void UnvanquishedConnection::say(const network::OutputMessage& message)
{
    string::FormattedString prefix_stream;
    if ( !message.prefix.empty() )
        prefix_stream << message.prefix << ' ' << color::nocolor;

    string::FormattedProperties message_properties = {
        {"to",      message.target.empty() ? "-1" : message.target},
        {"prefix",  prefix_stream},
        {"from",    message.from.copy() << color::nocolor},
        {"message", message.message.copy() << color::nocolor},
    };

    auto cmd = message.action ? cmd_say_action :
               message.from.empty() ? cmd_say : cmd_say_as;
    cmd.replace(message_properties);
    auto formatted = cmd.encode(*formatter_);
    command({"rcon", {formatted}, message.priority, message.timeout});
}


void UnvanquishedConnection::command ( network::Command cmd )
{
    // discards commands sent too early
    /// \todo Could queue instead of discarding
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
                ErrorLog("unv") << "Wrong parameters for \""+cmd.parameters[0]+"\"";
                return;
            }
            if ( cmd.parameters[0] == "set" )
            {
                auto lock = make_lock(mutex);
                properties_.put("cvar."+cmd.parameters[1], cmd.parameters[2]);
            }
            cmd.parameters[2] = xonotic::quote_string(cmd.parameters[2]);
        }

        std::string command = melanolib::string::implode(" ",cmd.parameters);
        if ( command.empty() )
        {
            ErrorLog("unv") << "Empty rcon command";
            return;
        }

        rcon_command(command);
    }
    else
    {
        Log("unv",'<',1) << cmd.command;
        write(cmd.command);
    }
}

void UnvanquishedConnection::on_network_error(const std::string& message)
{
    ErrorLog("unv","Network Error") << message;
    close_connection();
    return;
}

void UnvanquishedConnection::on_receive(const std::string& command,
                                        const std::string& message)
{
    Daemon::on_receive(command, message);

    if ( command == "rconInfoResponse" )
    {
        auto lock = make_lock(mutex);
        for ( const auto& cmd : startup_commands )
        {
            this->command(cmd);
        }
        lock.unlock();

        request_status();
        status_polling.start();
        network::Message().connected().send(this);
    }

    network::Message msg;
    msg.command = command;
    msg.raw = msg.command+' '+message;
    msg.params.push_back(message);
    forward_message(msg);
}


void UnvanquishedConnection::forward_message(network::Message& msg)
{
    Log("unv",'>',4) << formatter_->decode(msg.raw); // 4 'cause spams
    if ( msg.raw.empty() )
        return;

    msg.send(this);
}

void UnvanquishedConnection::on_receive_log(const std::string& line)
{
    if ( line.empty() )
        return;

    network::Message msg;
    msg.command = "print";
    msg.raw = line;

    // cvars
    if ( msg.raw[0] == '"' )
    {
        // Note cvars can actually have any characters in their identifier
        // and values, this regex limits to:
        //  * The identifier to contain only alphanumeric or + - _
        //  * The value anything but "
        static std::regex regex_cvar(
            R"cvar("([^"]+)" - "([^"]*)^7"^7 - .*)cvar",
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
        }
    }
    // status reply
    else if ( msg.raw == "(begin server status)" )
    {
        check_user_start();
    }
    else if ( status_ == CHECKING )
    {
        static std::regex regex_status1_players(
            R"(players:  \d+ / (\d+))",
            std::regex::ECMAScript|std::regex::optimize
        );
        static std::regex regex_status1(
            "([a-z]+):\\s+(.*)",
            std::regex::ECMAScript|std::regex::optimize
        );
        static std::regex regex_status1_player(
            //     num    score   ping     IP     port    name
            R"(\s*(\d+)\s+(\S+)\s+(\S+)\s+(\S+)\s+(\S+)\s+(.*))",
            std::regex::ECMAScript|std::regex::optimize
        );
        std::smatch match;

        if ( msg.raw == "(end server status)" )
        {
            check_user_end();
        }
        else if ( std::regex_match(msg.raw, match, regex_status1_players) )
        {
            properties_.put<std::string>("cvar.sv_maxclients", match[1]);
        }
        else if ( std::regex_match(msg.raw, match, regex_status1) )
        {
            std::string status_name = match[1];
            std::string status_value = match[2];
            if ( status_name == "hostname" )
                status_name = "cvar.sv_hostname";
            auto lock = make_lock(mutex);
            properties_.put(status_name, status_value);
        }
        else if ( std::regex_match(msg.raw, match, regex_status1_player) )
        {
            check_user(match);
        }
    }
    else
    {
        static std::regex regex_map_header(
            R"(Listing (\d+) maps:)",
            std::regex::ECMAScript|std::regex::optimize
        );

        std::smatch match;

        auto lock = make_lock(mutex);
        if ( map_checking )
        {
            maps_.push_back(line);
            map_checking--;
        }
        else if ( std::regex_match(msg.raw, match, regex_map_header) )
        {
            map_checking = melanolib::string::to_uint(match[1]);
            maps_.clear();
        }
    }


    forward_message(msg);
}

void UnvanquishedConnection::check_user_start()
{
    // Marks all users as unchecked
    auto lock = make_lock(mutex);
    for ( user::User& user : user_manager )
        user.checked = false;

    status_ = CHECKING;
}

void UnvanquishedConnection::check_user(const std::smatch& match)
{
    auto lock = make_lock(mutex);

    Properties props = {
        {"local_id", match[1]},
        {"score",    match[2]},
        {"ping",     match[3]},
        {"host",     match[4] == "bot" ? std::string(): match[4].str()+':'+match[5].str()},
        {"name",     match[6]},
    };

    if ( user::User* user = user_manager.user(match[1]) )
    {
        user->update(props);
        user->checked = true;
    }
    else
    {
        user::User new_user;
        new_user.origin = this;
        new_user.update(props);
        user_manager.add_user(new_user);

        network::Message msg;
        msg.type = network::Message::JOIN;
        msg.from = new_user;
        Log("unv",'!',3) << "Added user " << decode(new_user.name);
        msg.send(this);
    }
}

void UnvanquishedConnection::check_user_end()
{
    if ( status_ < CONNECTING )
        return;

    // Removes all unchecked users
    /// \todo Send part command
    auto lock = make_lock(mutex);
    auto it = user_manager.users_reference().begin();
    while ( it != user_manager.users_reference().end() )
    {
        if ( !it->checked )
        {
            network::Message msg;
            msg.type = network::Message::PART;
            msg.from = *it;
            Log("unv",'!',3) << "Removed user " << decode(it->name);
            msg.send(this);
            it = user_manager.users_reference().erase(it);
        }
        else
        {
            ++it;
        }
    }

    status_ = CONNECTED;
}

void UnvanquishedConnection::request_status()
{
    // status: CONNECTING  -> CHECKING
    // status: CONNECTED   -> CHECKING
    // status: DISCONNECTED-> WAITING
    if ( status_ >= CONNECTING )
    {
        status_ = CHECKING;

        auto lock = make_lock(mutex);
        for ( const auto& cmd : polling_status )
            command(cmd);
    }
    else
    {
        close_connection();
        connect();
    }
}

void UnvanquishedConnection::add_polling_command(const network::Command& command)
{
    auto lock = make_lock(mutex);

    for ( const auto& cmd : polling_status )
        if ( cmd.command == command.command && cmd.parameters == command.parameters )
            return;
    polling_status.push_back(command);
}

void UnvanquishedConnection::add_startup_command(const network::Command& command)
{
    auto lock = make_lock(mutex);
    startup_commands.push_back(command);
    if ( status_ == CONNECTED || status_ == CHECKING )
    {
        this->command(command);
    }
}

std::vector<std::string> UnvanquishedConnection::maps() const
{
    /// \todo could send a sync call to get the maps when maps_ is empty
    auto lock = make_lock(mutex);
    return maps_;
}


} // namespace unvanquished
