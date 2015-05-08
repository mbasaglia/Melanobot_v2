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
#ifndef XONOTIC_CONNECTION_HPP
#define XONOTIC_CONNECTION_HPP

#include <thread>

#include <boost/asio.hpp>

#include "network/connection.hpp"
#include "network/udp_io.hpp"
#include "concurrent_container.hpp"
#include "user/user_manager.hpp"
#include "melanobot.hpp"
#include "xonotic.hpp"

namespace xonotic {


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
    static std::unique_ptr<XonoticConnection> create(
        Melanobot* bot, const Settings& settings, const std::string& name);

    /**
     * \thread main \lock none
     */
    XonoticConnection ( Melanobot* bot, const network::Server& server,
                        const Settings& settings={}, const std::string& name={} );

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
    /**
     * \thread external \lock data
     */
    std::string get_property(const std::string& property) const override;
    /**
     * \thread external \lock data
     */
    bool set_property(const std::string& property, const std::string& value ) override;
    /**
     * \thread external \lock data
     */
    Properties message_properties() const override;

    /**
     * \brief Adds a command that needs to be sent regularly to the server
     * \param command    Command to be sent
     * \param continuous If true send every each \c status_delay, otherwise only at match start
     */
    void add_polling_command(const network::Command& command, bool continuous=false);

    /**
     * \thead external \lock data
     */
    user::UserCounter count_users(const std::string& channel = {}) const override;

    /**
     * \brief Returns the value of a cvar (or the empty string)
     * \thread external \lock data
     *
     * Equivalent to get_property("cvar.name")
     */
    std::string get_cvar(const std::string& name) const
    {
        Lock lock(mutex);
        auto it = cvars.find(name);
        return it != cvars.end() ? it->second : "";
    }

    // Dummy methods:
    bool channel_mask(const std::vector<std::string>&, const std::string& ) const override
    {
        return false;
    }
    bool user_auth(const std::string&, const std::string&) const override
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
    std::vector<user::User> real_users_in_group(const std::string& group) const override
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
    std::string         cmd_say_action;                 ///< Command used to show actions
    Properties          cvars;                          ///< Cvar values
    Properties          properties;                     ///< Misc properties (eg: map, gametype)
    std::list<SecureRconCommand>rcon_buffer;            ///< Buffer for rcon secure commands


    network::Server     server_;                        ///< Connection server
    AtomicStatus        status_{DISCONNECTED};          ///< Connection status

    user::UserManager   user_manager;                   ///< Keeps track of players
    Melanobot*          bot = nullptr;                  ///< Main bot

    network::UdpIo      io;                             ///< Handles networking

    std::thread         thread_input;   ///< Thread handling input
    mutable std::mutex  mutex;          ///< Guard data races
    network::Timer      status_polling; ///< Timer used to gether the connection status
    std::vector<network::Command> polling_match; ///< Commands to send on match start
    std::vector<network::Command> polling_status; ///< Commands to send on status_polling

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

    /**
     * \brief Closes the connection, allowing it to be re-opened automatically
     */
    void close_connection();

    /**
     * \brief Clear match info (players. map etc)
     * \note The caller must have a lock on \c mutex
     */
    void clear_match();

    /**
     * \brief Initializes user checking
     */
    void check_user_start();
    /**
     * \brief Checks a player
     */
    void check_user(const std::smatch& match);
    /**
     * \brief Finalizes user checking
     */
    void check_user_end();

};

} // namespace xonotic
#endif // XONOTIC_CONNECTION_HPP
