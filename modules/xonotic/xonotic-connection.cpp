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
    : server_(server), bot(bot)
{
    formatter_ = string::Formatter::formatter(
        settings.get("string_format",std::string("xonotic")));
    io.max_datagram_size(settings.get("datagram_size",1400));
    io.on_error = [](const std::string& msg)
    {
        ErrorLog("xon","Network Error") << msg;
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

    /// \todo resend on Unknown command "Melanobot_.*"
    conn_commands = {
        {"alias", "Melanobot_restore_sv_adminnick", "set sv_adminnick \"$Melanobot_sv_adminnick\""},
        {"alias", "Melanobot_set_sv_adminnick", "set Melanobot_sv_adminnick \"$sv_adminnick\"; set sv_adminnick \"^3${1 q}^3\";"},
        {"alias", "Melanobot_say_as", "Melanobot_set_sv_adminnick \"${1 q}\"; say \"${2 asis}\"; Melanobot_restore_sv_adminnick"}
    };
}

void XonoticConnection::connect()
{
    if ( !io.connected() )
    {
        status_ = CONNECTING;

        Lock lock(mutex);
        rcon_buffer.clear();
        lock.unlock();

        if ( io.connect(server_) )
        {
            if ( !thread_input.joinable() )
                thread_input = std::move(std::thread([this]{io.run_input();}));
            update_connection();
        }
    }

}

void XonoticConnection::disconnect(const std::string& message)
{
    if ( io.connected() )
    {
        cleanup_connection();
        if ( !message.empty() && status_ > CONNECTING )
            say((string::FormattedStream() << message).str());
        status_ = DISCONNECTED;
        io.disconnect();
        if ( thread_input.joinable() )
            thread_input.join();
        Lock lock(mutex);
        rcon_buffer.clear();
    }
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
    return property+"todo";
}

bool XonoticConnection::set_property(const std::string& property,
                                     const std::string& value )
{
    /// \todo
    (void)(property);
    (void)(value);
    return false;
}

void XonoticConnection::say ( const network::OutputMessage& message )
{
    string::FormattedStream str;
    if ( !message.prefix.empty() )
        str << message.prefix << ' ' << color::nocolor;
    if ( !message.from.empty() )
    {
        if ( message.action )
            str << "* ";
        str << message.from;
    }
    std::vector<std::string> cmd;

    std::string from = str.encode(formatter_);
    if ( !from.empty() )
    {
        cmd = { "Melanobot_say_as", quote_string(from),
            quote_string(message.message.encode(formatter_)) };
    }
    else
    {
        cmd = { "say", quote_string(message.message.encode(formatter_)) };
    }

    command({"rcon", cmd, message.priority, message.timeout});
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

    if ( msg.command == "n" )
    {
        if ( msg.raw[0] == '\1' )
        {
            static std::regex regex_chat("^\1(.*)\\^7: (.*)",
                std::regex::ECMAScript|std::regex::optimize
            );
            std::smatch match;
            if ( std::regex_match(msg.raw,match,regex_chat) )
            {
                msg.from = match[1];
                msg.message = match[2];
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
    /// \todo the host name for log_dest_udp needs to be read from settings
    /// (and if settings don't provide it, fallback to what local_endpoint() gives)
    command({"rcon",{"set", "log_dest_udp", io.local_endpoint().name()}});

    for ( const auto& v : conn_commands )
        command({"rcon",v});
}

void XonoticConnection::cleanup_connection()
{
    Lock lock(mutex);
    rcon_buffer.clear();
    lock.unlock();

    command({"rcon",{"set", "log_dest_udp", ""}});
    // always ensure at least this is performed (even on rcon_secure 2)
    read(io.read());


    for ( const auto& v : conn_commands )
    {
        if ( v.size() < 2 )
            continue;

        if ( v[0] == "set" )
            command({"rcon", {"unset", v[1]}});
        else if ( v[0] == "alias" )
            command({"rcon", {"unalias", v[1]}});
    }
}

} // namespace xonotic
