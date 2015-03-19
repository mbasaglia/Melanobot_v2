/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright  Mattia Basaglia
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
#ifndef XONOTIC_CONNECTION_HPP
#define XONOTIC_CONNECTION_HPP

#include <thread>

#include <boost/asio.hpp>

#include "network/connection.hpp"
#include "network/udp_io.hpp"
#include "concurrent_container.hpp"
#include "user/user_manager.hpp"
#include "melanobot.hpp"

namespace xonotic {

class XonoticConnection : public network::Connection {

public:
    /**
     * \brief Create from settings
     */
    static std::unique_ptr<XonoticConnection> create(Melanobot* bot, const Settings& settings)
    {
        if ( settings.get("protocol",std::string()) != "xonotic" )
        {
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

    /**
     * \thread main \lock none
     */
    XonoticConnection ( Melanobot* bot, const network::Server& server,
                                const Settings& settings={} )
        : server_(server), bot(bot)
    {
        formatter_ = string::Formatter::formatter(settings.get("string_format",std::string("xonotic")));
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

    /**
     * \thread main \lock none
     */
    ~XonoticConnection() override
    {
        stop();
    }

    /**
     * \thread external \lock none (reading a constant value)
     * \todo return io.local_endpoint() ?
     */
    network::Server server() const override
    {
        return server_;
    }

    /**
     * \thread any \lock none
     */
    void command ( network::Command cmd ) override
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

                /// \todo this quoting thing as a stand-alone function
                static std::regex regex_xonquote(R"(([\\"]))",
                    std::regex::ECMAScript|std::regex::optimize
                );
                cmd.parameters[2] = '"'+std::regex_replace(cmd.parameters[2],regex_xonquote,"\\$&")+'"';
            }

            cmd.command += ' '+rcon_password;
        }

        write(cmd.command +
            (cmd.parameters.empty() ? "" : ' '+string::implode(" ",cmd.parameters)));
    }

    /**
     * \thread external \lock none
     */
    void say ( const network::OutputMessage& message ) override
    {
        (void)message;
        /// \todo
    }

    /**
     * \thread external \lock none
     */
    Status status() const override
    {
        return status_;
    }

    /**
     * \thread external \lock none
     */
    std::string protocol() const override
    {
        return "xonotic";
    }

    /**
     * \thread external \lock none(todo: data)
     */
    void connect() override
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

    /**
     * \thread external \lock none(todo: data)
     */
    void disconnect(const std::string& message = {}) override
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

    /**
     * \thread external \lock none(todo: data)
     */
    void reconnect(const std::string& quit_message = {}) override
    {
        disconnect(quit_message);
        connect();
    }

    /**
     * \thread external \lock none
     */
    string::Formatter* formatter() const override
    {
        return formatter_;
    }

    void update_user(const std::string& local_id,
                     const Properties& properties) override
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

    user::User get_user(const std::string& local_id) const override
    {
        Lock lock(mutex);
        const user::User* user = user_manager.user(local_id);
        if ( user )
            return *user;
        return {};
    }

    std::vector<user::User> get_users( const std::string& channel_mask = "" ) const override
    {
        Lock lock(mutex);
        auto list = user_manager.users();
        return {list.begin(), list.end()};
    }

    std::string name() const override { return "Todo"; }
    std::string get_property(const std::string& property) const override
    {
        return property+"todo";
    }
    bool set_property(const std::string& property, const std::string& value ) override
    {
        /// \todo
        (void)(property);
        (void)(value);
        return false;
    }

    // Dummy methods:
    bool channel_mask(const std::vector<std::string>&, const std::string& ) const override
    {
        return false;
    }
    bool user_auth(const std::string&, const std::string&) const
    {
        /// \todo (?)
        return false;
    }
    bool add_to_group(const std::string&, const std::string&) override
    {
        return false;
    }
    bool remove_from_group(const std::string&, const std::string&) override
    {
        return false;
    }
    std::vector<user::User> users_in_group(const std::string&) const override
    {
        return {};
    }

private:
    network::Server     server_;                        ///< Connection server
    std::string         header = "\xff\xff\xff\xff";    ///< Connection message header
    std::string         rcon_password;                  ///< Rcon Password
    int                 rcon_secure = 0;                ///< Rcon secure protocol
    AtomicStatus        status_{DISCONNECTED};          ///< Connection status
    string::Formatter*  formatter_ = nullptr;           ///< String formatter

    user::UserManager   user_manager;                   ///< Keeps track of players
    Melanobot*          bot = nullptr;                  ///< Main bot

    network::UdpIo      io;                             ///< Handles networking

    std::thread         thread_input;   ///< Thread handling input
    mutable std::mutex  mutex;          ///< Guard data races


    /**
     * \brief Writes a raw line to the socket
     * \thread any \lock none
     */
    void write(std::string line)
    {
        line.erase(std::remove_if(line.begin(), line.end(),
            [](char c){return c == '\n' || c == '\0' || c == '\xff';}),
            line.end());

        Log("xon",'<',1) << formatter()->decode(line);

        io.write(header+line);
    }
    /**
     * \brief Async hook on network input
     * \thread xon_input \lock none
     */
    void read(const std::string& datagram)
    {
        network::Message msg;
        msg.raw = datagram;
        Log("xon",'>',4) << formatter_->decode(msg.raw); // 4 'cause spams

        std::istringstream socket_stream(msg.raw);

        if ( socket_stream && socket_stream.peek() == '\xff' )
        {
            while ( socket_stream && socket_stream.peek() == '\xff' )
                socket_stream.ignore(1);
            socket_stream >> msg.command;
            socket_stream.ignore(1);
        }
        std::string string;
        std::getline(socket_stream,string);
        msg.params.push_back(string);

        msg.source = msg.destination = this;
        handle_message(msg);
    }

    /**
     * \brief Interprets a message and sends it to the bot
     */
    void handle_message(network::Message& msg)
    {
        /// \todo parse msg
        bot->message(msg);
    }



};

} // namespace xonotic
#endif // XONOTIC_CONNECTION_HPP
