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
#include "xonotic-connection.hpp"

#include <ctime>

#include <boost/format.hpp>

#include <openssl/hmac.h>

#include "math.hpp"

namespace xonotic {


std::string hmac_md4(const std::string& input, const std::string& key)
{
    HMAC_CTX ctx;
    HMAC_CTX_init(&ctx);

    HMAC_Init_ex(&ctx, key.data(), key.size(), EVP_md4(), nullptr);

    HMAC_Update(&ctx, reinterpret_cast<const unsigned char*>(input.data()), input.size());

    unsigned char out[16];
    unsigned int out_size = 0;
    HMAC_Final(&ctx, out, &out_size);

    HMAC_CTX_cleanup(&ctx);

    return std::string(out,out+out_size);
}

std::unique_ptr<XonoticConnection> XonoticConnection::create(Melanobot* bot, const Settings& settings)
{
    if ( settings.get("protocol",std::string()) != "xonotic" )
    {
        /// \todo accept similar protocols? eg: nexuiz
        ErrorLog("xon") << "Wrong protocol for Xonotic connection";
        return nullptr;
    }

    network::Server server ( settings.get("server",std::string()) );
    if ( !server.port )
        server.port = 26000;
    server.host = settings.get("server.host",server.host);
    server.port = settings.get("server.port",server.port);
    if ( server.host.empty() || !server.port )
    {
        ErrorLog("xon") << "Xonotic connection with no server";
        return nullptr;
    }

    return std::make_unique<XonoticConnection>(bot, server, settings);
}

XonoticConnection::XonoticConnection ( Melanobot* bot,
                                       const network::Server& server,
                                       const Settings& settings )
    : server_(server), status_(DISCONNECTED), bot(bot)
{
    formatter_ = string::Formatter::formatter(
        settings.get("string_format",std::string("xonotic")));
    io.max_datagram_size(settings.get("datagram_size",1400));
    io.on_error = [this](const std::string& msg)
    {
        ErrorLog("xon","Network Error") << msg;
        close_connection();
    };
    io.on_async_receive = [this](const std::string& datagram)
    {
        read(datagram);
    };
    io.on_failure = [this]()
    {
        stop();
    };

    rcon_secure = settings.get("rcon_secure",rcon_secure);
    rcon_password = settings.get("rcon_password",rcon_password);

    cmd_say = settings.get("say","say %message");
    cmd_say_as = settings.get("say_as", "say \"%prefix%from^7: %message\"");

    // Preset templates
    if ( cmd_say_as == "modpack" )
    {
        cmd_say_as = "sv_cmd ircmsg %prefix%from^7: %message";
    }
    else if ( cmd_say_as == "sv_adminnick" )
    {
        cmd_say_as ="set Melanobot_sv_adminnick \"$sv_adminnick\";"
                    "set sv_adminnick \"^3%prefix^3%from^3\";"
                    "say \"^7%message\";"
                    "set sv_adminnick \"Melanobot_sv_adminnick\"";
    }

    status_polling = network::Timer{[this]{request_status();},
        timer::seconds(settings.get("status_delay",60))};
}

void XonoticConnection::connect()
{
    if ( !io.connected() )
    {
        status_ = CONNECTING;

        if ( io.connect(server_) )
        {
            if ( !thread_input.joinable() )
                thread_input = std::move(std::thread([this]{io.run_input();}));
            // Just connected, clear all
            Lock lock(mutex);
                rcon_buffer.clear();    /// \todo Maybe don't clear?
                cvars.clear();          // cvars might have chan
                user_manager.clear();   // same with users
            lock.unlock();
            update_connection();
        }
    }

}

void XonoticConnection::disconnect(const std::string& message)
{
    status_polling.stop();
    if ( io.connected() )
    {
        if ( !message.empty() && status_ > CONNECTING )
            say({message});
        cleanup_connection();
    }
    close_connection();
    Lock lock(mutex);
    rcon_buffer.clear();
    user_manager.clear();
    lock.unlock();
}
void XonoticConnection::update_user(const std::string& local_id,
                                    const Properties& properties)
{
    Lock lock(mutex);
    user::User* user = user_manager.user(local_id);
    if ( user )
    {
        user->update(properties);

        /// \todo this might not be needed if crypto_id isn't good enough
        auto it = properties.find("global_id");
        if ( it != properties.end() )
            Log("xon",'!',3) << "Player " << color::dark_cyan << local_id
                << color::nocolor << " is authed as " << color::cyan << it->second;
    }
}

user::User XonoticConnection::get_user(const std::string& local_id) const
{
    Lock lock(mutex);
    const user::User* user = user_manager.user(local_id);
    if ( user )
        return *user;
    return {};
}

std::vector<user::User> XonoticConnection::get_users( const std::string& channel_mask ) const
{
    Lock lock(mutex);
    auto list = user_manager.users();
    return {list.begin(), list.end()};
}

std::string XonoticConnection::name() const
{
    return "Todo";
}

std::string XonoticConnection::get_property(const std::string& property) const
{
    Lock lock(mutex);
    if ( string::starts_with(property,"cvar.") )
    {
        auto it = cvars.find(property.substr(5));
        return it != cvars.end() ? it->second : "";
    }
    else if ( property == "say_as" )
    {
        return cmd_say_as;
    }
    else if ( property == "say" )
    {
        return cmd_say;
    }
    else if ( property == "host" )
    {
        auto it = properties.find(property);
        if ( it == properties.end() )
        {
            it = cvars.find("cvar.hostname");
            if ( it == cvars.end() )
                return description();
        }
        return it->second;
    }
    auto it = properties.find(property);
    return it != properties.end() ? it->second : "";
}

bool XonoticConnection::set_property(const std::string& property,
                                     const std::string& value )
{

    Lock lock(mutex);
    if ( string::starts_with(property,"cvar.") )
        cvars[property.substr(5)] = value;
    else if ( property == "say_as" )
        cmd_say_as = value;
    else if ( property == "say" )
        cmd_say = value;
    else
        properties[property] = value;
    return true;
}

void XonoticConnection::say ( const network::OutputMessage& message )
{
    string::FormattedString prefix_stream;
    if ( !message.prefix.empty() )
        prefix_stream << message.prefix << ' ';

    string::FormattedString from_stream;
    if ( !message.from.empty() )
    {
        if ( message.action )
            from_stream << "* ";
        from_stream << message.from;
    }

    std::string prefix   = prefix_stream.encode(formatter_);
    std::string from     = from_stream.encode(formatter_);
    std::string contents = message.message.encode(formatter_);
    Properties message_properties = {
        {"prefix",              prefix},
        {"from",                from},
        {"message",             contents},
        //{"from_quoted",         quote_string(from)},
        //{"message_quoted",      quote_string(contents)},
    };

    auto expl = string::regex_split(message.from.empty() ? cmd_say : cmd_say_as,";\\s*");
    for ( const auto & cmd : expl )
        command({"rcon", { string::replace(cmd,message_properties,"%")},
                message.priority, message.timeout});
}

void XonoticConnection::command ( network::Command cmd )
{
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
            /// \todo validate cmd.parameters[1] as a proper cvar name
            cmd.parameters[2] = quote_string(cmd.parameters[2]);
        }

        std::string rcon_command = string::implode(" ",cmd.parameters);
        if ( rcon_command.empty() )
        {
            ErrorLog("xon") << "Empty rcon command";
            return;
        }

        if ( rcon_secure <= 0 )
        {
            Log("xon",'<',2) << formatter()->decode(rcon_command);
            write("rcon "+rcon_password+' '+rcon_command);
        }
        else if ( rcon_secure == 1 )
        {
            Log("xon",'<',2) << formatter()->decode(rcon_command);
            std::string argbuf = (boost::format("%ld.%06d %s")
                % std::time(nullptr) % math::random(1000000)
                % rcon_command).str();
            std::string key = hmac_md4(argbuf,rcon_password);
            write("srcon HMAC-MD4 TIME "+key+' '+argbuf);
        }
        else
        {
            Lock lock(mutex);
            rcon_buffer.push_back(rcon_command);
            lock.unlock();
            request_challenge();
        }
    }
    else
    {
        Log("xon",'<',1) << cmd.command;
        write(cmd.command);
    }
}

void XonoticConnection::request_challenge()
{
    Lock lock(mutex);
    if ( !rcon_buffer.empty() && !rcon_buffer.front().challenged )
    {
        rcon_buffer.front().challenged = true;
        /// \todo timeout from settings
        rcon_buffer.front().timeout = network::Clock::now() + std::chrono::seconds(5);
        Log("xon",'<',5) << "getchallenge";
        write("getchallenge");
    }
}

void XonoticConnection::write(std::string line)
{
    line.erase(std::remove_if(line.begin(), line.end(),
        [](char c){return c == '\n' || c == '\0' || c == '\xff';}),
        line.end());

    io.write(header+line);
}

void XonoticConnection::read(const std::string& datagram)
{
    if ( datagram.size() < 5 || !string::starts_with(datagram,"\xff\xff\xff\xff") )
    {
        ErrorLog("xon","Network Error") << "Invalid datagram: " << datagram;
        return;
    }

    if ( status_ == CONNECTING )
        status_ = CHECKING;

    if ( datagram[4] != 'n' )
    {
        network::Message msg;
        std::istringstream socket_stream(datagram.substr(4));
        socket_stream >> msg.command; // info, status et all
        msg.raw = datagram;
        if ( socket_stream.peek() == ' ' )
            socket_stream.ignore(1);
        msg.params.emplace_back();
        std::getline(socket_stream,msg.params.back());
        Log("xon",'>',4) << formatter_->decode(msg.raw);
        handle_message(msg);
        return;
    }

    Lock lock(mutex);
    std::istringstream socket_stream(line_buffer+datagram.substr(5));
    line_buffer.clear();
    lock.unlock();

    // convert the datagram into lines
    std::string line;
    while (socket_stream)
    {
        std::getline(socket_stream,line);
        if (socket_stream.eof())
        {
            lock.lock();
            line_buffer = line;
            lock.unlock();
            break;
        }
        network::Message msg;
        msg.command = "n"; // log
        msg.raw = line;
        Log("xon",'>',4) << formatter_->decode(msg.raw); // 4 'cause spams
        handle_message(msg);
    }
}

void XonoticConnection::handle_message(network::Message& msg)
{
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
            if ( std::regex_match(msg.raw,match,regex_chat) )
            {
                msg.from.name = match[1];
                msg.message = match[2];
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
                Lock lock(mutex);
                cvars[cvar_name] = cvars[cvar_value];
                lock.unlock();
                /// \todo keep in sync when update_connection() is changed
                if ( cvar_name == "log_dest_udp" )
                {
                    if ( cvar_value != io.local_endpoint().name() )
                    {
                        update_connection();
                    }
                    else if ( status_ == CHECKING )
                    {
                        status_ = CONNECTED;
                        virtual_message("CONNECTED");
                    }
                }
            }
        }
        // join/part
        else if ( msg.raw[0] == ':' )
        {
            static std::regex regex_join(
                R"regex(^:join:(\d+):(\d+):((?:[0-9]+(?:\.[0-9]+){3})|(?:[[:xdigit:]](?::[[:xdigit:]]){7})):(.*))regex",
                std::regex::ECMAScript|std::regex::optimize
            );
            static std::regex regex_part(
                R"regex(^:part:(\d+))regex",
                std::regex::ECMAScript|std::regex::optimize
            );
            std::smatch match;
            if ( std::regex_match(msg.raw,match,regex_join) )
            {
                user::User user;
                user.local_id = match[1]; // playerid
                user.properties["entity"] = match[2]; // entity number
                user.host = match[3]; // ip address or "bot"
                user.name = match[4];
                Lock lock(mutex);
                user_manager.add_user(user);
                lock.unlock();
                msg.command = "join";
                msg.from = user;
                Log("xon",'!',2) << "Added user " << formatter()->decode(user.name);
            }
            else if ( std::regex_match(msg.raw,match,regex_part) )
            {
                Lock lock(mutex);
                if ( user::User *found = user_manager.user(match[1]) )
                {
                    Log("xon",'!',2) << "Removed user " << formatter()->decode(found->name);
                    msg.from = *found;
                    msg.command = "part";
                    user_manager.remove_user(found->local_id);
                }
            }
        }
        // status reply
        else if ( status_ == CHECKING )
        {
            static std::regex regex_status1(
                "([a-z]+):\\s+(.*)",
                std::regex::ECMAScript|std::regex::optimize
            );
            std::smatch match;
            if ( std::regex_match(msg.raw, match, regex_status1) )
            {
                std::string status_name = match[1];
                std::string status_value = match[2];
                Lock lock(mutex);
                properties[status_name] = status_value;
            }
        }
    }
    else if ( msg.command == "challenge" )
    {
        Lock lock(mutex);
        if ( msg.params.size() != 1 || rcon_buffer.empty() )
        {
            ErrorLog("xon") << "Got an odd challenge from the server";
            return;
        }

        auto rcon_command = rcon_buffer.front();
        if ( !rcon_command.challenged || rcon_command.timeout < network::Clock::now() )
        {
            lock.unlock();
            rcon_command.challenged = false;
            request_challenge();
            return;
        }

        rcon_buffer.pop_front();
        lock.unlock();

        Log("xon",'<',2) << formatter()->decode(rcon_command.command);
        std::string challenge = msg.params[0].substr(0,11);
        std::string challenge_command = challenge+' '+rcon_command.command;
        std::string key = hmac_md4(challenge_command, rcon_password);
        write("srcon HMAC-MD4 CHALLENGE "+key+' '+challenge_command);

        request_challenge();

        return; // don't propagate challenge messages
    }

    msg.source = msg.destination = this;
    bot->message(msg);
}

void XonoticConnection::update_connection()
{
    if ( status_ > DISCONNECTED )
    {
        status_ = CONNECTING;

        /// \todo the host name for log_dest_udp needs to be read from settings
        /// (and if settings don't provide it, fallback to what local_endpoint() gives)
        /// see also in handle_message()
        command({"rcon",{"set","log_dest_udp",io.local_endpoint().name()},1024});

        command({"rcon",{"set", "sv_eventlog", "1"},1024});

        // Make sure the connection is being checked
        if ( !status_polling.running() )
            status_polling.start();

        /// \todo should:
        ///     * wait while status_ == CONNECTING
        ///     * return if status_ == DISCONNECTED
        ///     * keep going if status_ >= CHECKING
        // Commands failed
        if ( status_ < CHECKING )
            return;

        // Request status right away (will also be called later by status_polling)
        request_status();
    }
}

void XonoticConnection::cleanup_connection()
{

    command({"rcon",{"set", "log_dest_udp", ""},1024});

    // try to actually clear log_dest_udp when challenges are required
    if ( rcon_secure >= 2 )
        read(io.read());

    Lock lock(mutex);
    rcon_buffer.clear();
    cvars.clear();
}

void XonoticConnection::request_status()
{
    if ( status_ > CONNECTING )
    {
        /// \todo attribute holding these commands, check timeouts etc.
        command({"rcon",{"status 1"},1024});
        command({"rcon",{"log_dest_udp"},1024});
    }
    else
    {
        close_connection();
        connect();
    }
}


void XonoticConnection::virtual_message(std::string command)
{
    network::Message msg;
    msg.source = msg.destination = this;
    msg.command = std::move(command);
    bot->message(msg);
}

void XonoticConnection::close_connection()
{
    if ( status_ > CONNECTING )
        virtual_message("DISCONNECTED");
    status_ = DISCONNECTED;
    if ( io.connected() )
        io.disconnect();
    if ( thread_input.joinable() &&
            thread_input.get_id() != std::this_thread::get_id() )
        thread_input.join();
}
} // namespace xonotic
