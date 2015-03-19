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

namespace xonotic {

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
}

void XonoticConnection::command ( network::Command cmd )
{
    if ( cmd.command == "rcon" )
    {
        if ( cmd.parameters.empty() )
            return;

        if ( cmd.parameters[0] == "set" )
        {
            if ( cmd.parameters.size() != 3 )
            {
                ErrorLog("xon") << "Wrong parameters for \"set\"";
                return;
            }
            /// \todo validate cmd.parameters[1] as a proper cvar name
            cmd.parameters[2] = quote_string(cmd.parameters[2]);
        }

        /// \todo rcon_secure
        cmd.command += ' '+rcon_password;
    }

    write(cmd.command +
        (cmd.parameters.empty() ? "" : ' '+string::implode(" ",cmd.parameters)));
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
            /// \todo the host name for log_dest_udp needs to be read from settings
            /// (and if settings don't provide it, fallback to what local_endpoint() gives)
            command({"rcon",{"set", "log_dest_udp", io.local_endpoint().name()}});
        }
    }

}

void XonoticConnection::disconnect(const std::string& message)
{
    if ( io.connected() )
    {
        command({"rcon",{"set", "log_dest_udp", ""}});
        if ( !message.empty() && status_ > CONNECTING )
            say((string::FormattedStream() << message).str());
        status_ = DISCONNECTED;
        io.disconnect();
        if ( thread_input.joinable() )
            thread_input.join();
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


void XonoticConnection::write(std::string line)
{
    line.erase(std::remove_if(line.begin(), line.end(),
        [](char c){return c == '\n' || c == '\0' || c == '\xff';}),
        line.end());

    Log("xon",'<',1) << formatter()->decode(line);

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
        Log("xon",'>',4) << formatter_->decode(msg.raw);
        handle_message(msg);
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

    msg.source = msg.destination = this;
    bot->message(msg);
}

} // namespace xonotic
