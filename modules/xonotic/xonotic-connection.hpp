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

/**
 * \brief Escapes characters and puts the string in double quotes
 */
inline std::string quote_string(const std::string& text)
{
    static std::regex regex_xonquote(R"(([\\"]))",
        std::regex::ECMAScript|std::regex::optimize
    );
    return '"'+std::regex_replace(text,regex_xonquote,"\\$&")+'"';
}

/**
 * \brief Compute the MD4 HMAC of the given input with the given key
 */
std::string hmac_md4(const std::string& input, const std::string& key);

/**
 * \brief Creates a connection to a Xonotic server
 */
class XonoticConnection : public network::Connection {

public:
    /**
     * \brief Create from settings
     * \todo remove these functions and add a ctor overload w/o server object
     *          (which might throw)
     */
    static std::unique_ptr<XonoticConnection> create(Melanobot* bot, const Settings& settings);

    /**
     * \thread main \lock none
     */
    XonoticConnection ( Melanobot* bot, const network::Server& server,
                                const Settings& settings={} );

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
     * \thread external \lock none (reading a constant value)
     * \todo connection server might differ from the publicly visible one (eg localhost)
     */
    std::string description() const override
    {
        return server_.name();
    }

    /**
     * \thread any \lock none
     */
    void command ( network::Command cmd ) override;

    /**
     * \thread external \lock none
     */
    void say ( const network::OutputMessage& message ) override;

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
    void connect() override;

    /**
     * \thread external \lock none(todo: data)
     */
    void disconnect(const std::string& message = {}) override;

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
                     const Properties& properties) override;

    user::User get_user(const std::string& local_id) const override;

    std::vector<user::User> get_users( const std::string& channel_mask = "" ) const override;

    std::string name() const override;
    std::string get_property(const std::string& property) const override;
    bool set_property(const std::string& property, const std::string& value ) override;

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
    /**
     * \brief A command to be used with rcon_secure >= 2
     */
    struct SecureRconCommand
    {
        std::string     command;        ///< Raw command string
        bool            challenged;     ///< Whether a challenge has been sent
        network::Time   timeout;        ///< Challenge timeout
        SecureRconCommand(std::string command)
            : command(std::move(command)), challenged(false) {}
    };

    std::string         line_buffer;                    ///< Buffer used for truncated lines
    string::Formatter*  formatter_{nullptr};            ///< String formatter

    std::string         header{"\xff\xff\xff\xff"};     ///< Connection message header
    std::string         rcon_password;                  ///< Rcon Password
    int                 rcon_secure{0};                 ///< Rcon secure protocol
    std::string         cmd_say;                        ///< Command used to say messages
    std::string         cmd_say_as;                     ///< Command used to say messages as another user
    Properties          cvars;                          ///< Cvar values
    std::list<SecureRconCommand>rcon_buffer;            ///< Buffer for rcon secure commands


    network::Server     server_;                        ///< Connection server
    AtomicStatus        status_{DISCONNECTED};          ///< Connection status

    user::UserManager   user_manager;                   ///< Keeps track of players
    Melanobot*          bot = nullptr;                  ///< Main bot

    network::UdpIo      io;                             ///< Handles networking

    std::thread         thread_input;   ///< Thread handling input
    mutable std::mutex  mutex;          ///< Guard data races
    network::Timer      status_polling; ///< Timer used to gether the connection status


    /**
     * \brief Writes a raw line to the socket
     * \thread any \lock none
     */
    void write(std::string line);
    /**
     * \brief Async hook on network input
     * \thread xon_input \lock none
     */
    void read(const std::string& datagram);

    /**
     * \brief Interprets a message and sends it to the bot
     * \thread xon_input \lock data
     */
    void handle_message(network::Message& msg);

    /**
     * \brief Sends commands needed to keep the connection going
     * \thread any \lock data
     */
    void update_connection();

    /**
     * \brief Cleanup what update_connection() does
     * \thread external (disconnect) \lock data
     */
    void cleanup_connection();

    /**
     * \brief If there are commands needing a challenge, ask for it
     * \thread any \lock data
     */
    void request_challenge();

    /**
     * \brief Sends the commands needed to determine the connection status
     */
    void request_status();

    /**
     * \brief Generates a virtual message (not a protocol message)
     */
    void virtual_message(std::string command);

};

} // namespace xonotic
#endif // XONOTIC_CONNECTION_HPP
