/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015-2017 Mattia Basaglia
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
#ifndef MELANOBOT_MODULE_XONOTIC_UNVANQUISHED_CONNECTION_HPP
#define MELANOBOT_MODULE_XONOTIC_UNVANQUISHED_CONNECTION_HPP

#include "network/connection.hpp"
#include "user/user_manager.hpp"
#include "daemon.hpp"

namespace unvanquished {

class UnvanquishedConnection : public network::Connection, protected Daemon
{
public:
    /**
     * \brief Create from settings
     */
    static std::unique_ptr<UnvanquishedConnection> create(
        const Settings& settings, const std::string& name);

    /**
     * \thread main \lock none
     */
    UnvanquishedConnection( const network::Server&  server,
                            const std::string&      password,
                            const Settings&         settings = {},
                            const std::string&      name = {} );

    ~UnvanquishedConnection();

    network::Server server() const override
    {
        return Daemon::server();
    }

    std::string description() const override
    {
        return Daemon::server().name();
    }

    void command(network::Command cmd) override;

    void say(const network::OutputMessage& message) override;

    Status status() const override
    {
        return status_;
    }

    std::string protocol() const override
    {
        return "unvanquished";
    }

    void connect() override;

    void disconnect(const string::FormattedString& message = {}) override;

    void reconnect(const string::FormattedString& quit_message = {}) override
    {
        disconnect(quit_message);
        connect();
    }

    string::Formatter* formatter() const override
    {
        return formatter_;
    }

    void update_user(const std::string& local_id,
                     const Properties& properties) override;

    void update_user(const std::string& local_id,
                     const user::User& updated) override;

    user::User get_user(const std::string& local_id) const override;

    std::vector<user::User> get_users(const std::string& channel_mask = "") const override;

    std::string name() const override;

    LockedProperties properties() override;

    string::FormattedProperties pretty_properties() const override;

    /**
     * \brief Adds a command that needs to be sent regularly to the server
     * \param command    Command to be sent
     */
    void add_polling_command(const network::Command& command);

    /**
     * \brief Adds a command that needs to be sent once fully connected
     * \param command    Command to be sent
     */
    void add_startup_command(const network::Command& command);

    user::UserCounter count_users(const std::string& channel = {}) const override;

    // Dummy methods:
    bool channel_mask(const std::vector<std::string>&, const std::string& ) const override
    {
        return false;
    }
    bool user_auth(const std::string&, const std::string& auth_group) const override
    {
        return auth_group.empty();
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

    std::vector<std::string> maps() const;

private:
    void on_connect() override;
    void on_disconnect() override;
    void on_network_error(const std::string& message) override;
    void on_receive(const std::string& command, const std::string& message) override;
    void on_receive_log(const std::string& line) override;

    /**
     * \brief Disconnects from the ndaemon server and ensure a consistent state
     */
    void close_connection();

    /**
     * \brief Logs and sends the message to the bot
     */
    void forward_message(network::Message& msg);

    /**
     * \brief Marks all users as unchecked
     */
    void check_user_start();

    /**
     * \brief Updates an user based on a status match
     */
    void check_user(const std::smatch& match);

    /**
     * \brief Removes unchecked players
     */
    void check_user_end();

    /**
     * \brief Sends polling commands
     */
    void request_status();

    string::Formatter*            formatter_ = nullptr; ///< String formatter
    PropertyTree                  properties_;      ///< Misc properties (eg: map, gametype)
    AtomicStatus                  status_;          ///< Connection status
    user::UserManager             user_manager;     ///< Keeps track of players
    mutable std::recursive_mutex  mutex;            ///< Guard data races
    string::FormattedString       cmd_say;
    string::FormattedString       cmd_say_as;
    string::FormattedString       cmd_say_action;
    network::Timer                status_polling;   ///< Timer used to gether the connection status
    std::vector<network::Command> polling_status;   ///< Commands to send on status_polling
    std::vector<network::Command> startup_commands; ///< Commands to run once fully connected
    std::vector<std::string>      maps_;
    std::size_t                   map_checking;
};

} // namespace unvanquished
#endif // MELANOBOT_MODULE_XONOTIC_UNVANQUISHED_CONNECTION_HPP
