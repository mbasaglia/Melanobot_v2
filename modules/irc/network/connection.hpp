/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015-2017 Mattia Basaglia
 * \license
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

#ifndef IRC_CONNECTION_HPP
#define IRC_CONNECTION_HPP

#include <list>
#include <queue>
#include <regex>
#include <stdexcept>
#include <unordered_map>

#include "network/connection.hpp"
#include "irc/network/buffer.hpp"

/**
 * \brief Namespace contaning classes and functions handling the IRC protocol
 */
namespace irc {

/**
 * \brief IRC connection
 */
class IrcConnection : public network::Connection
{
public:
    /**
     * \brief Create from settings
     */
    static std::unique_ptr<IrcConnection> create(
        const Settings& settings, const std::string& name);

    /**
     * \thread main \lock none
     */
    IrcConnection(const network::Server& server,
                  const Settings&        settings = {},
                  const std::string&     name = {});
    
    /**
     * \thread main \lock none
     */
    ~IrcConnection() override;

    /**
     * \thread ? \lock data
     */
    network::Server server() const override;
    
    /**
     * \thread external \lock data
     */
    std::string description() const override;

    /**
     * \thread external \lock data(sometimes) buffer(indirect)
     */
    void command ( network::Command c ) override;

    /**
     * \thread external \lock data(sometimes) buffer(indirect)
     */
    void say ( const network::OutputMessage& message ) override;

    /**
     * \thread ? \lock none
     */
    Status status() const override;

    /**
     * \thread external \lock none
     */
    std::string protocol() const override;

    /**
     * \thread external \lock buffer(indirect) data(indirect)
     */
    void connect() override;

    /**
     * \thread external \lock buffer(indirect)
     */
    void disconnect(const string::FormattedString& message = {}) override;

    /**
     * \thread external \lock none
     */
    string::Formatter* formatter() const override;

    /**
     * \thread external \lock none
     * \param mask A list of channels names separated by commas or spaces,
     *             the wildcard \c * is supported
     *             and \c ! matches private messages
     */
    bool channel_mask(const std::vector<std::string>& channels,
                      const std::string& mask) const override;

    /**
     * \brief disconnect and connect
     * \thread external \lock buffer(indirect) data(indirect)
     */
    void reconnect(const string::FormattedString& quit_message = {}) override;

    /**
     * \return The bot's current nick
     * \thead external \lock data
     */
    std::string name() const override;

    /**
     * \brief Get whether a user has the given authorization level
     * \thead external \lock data
     */
    bool user_auth(const std::string& local_id,
                   const std::string& auth_group) const override;
    /**
     * \brief Update the properties of a user by local_id
     * \thead external \lock data
     */
    void update_user(const std::string& local_id,
                     const Properties& properties) override;
    /**
     * \thead external \lock data
     */
    void update_user(const std::string& local_id,
                     const user::User& updated) override;
    /**
     * \thead external \lock data
     */
    user::User get_user(const std::string& local_id) const override;

    /**
     * \thead external \lock data
     */
    std::vector<user::User> get_users( const std::string& channel ) const override;

    /**
     * \param user  A user local_id, if it begins with a \@, it's considered a
     *              host name, if it begins with a !, it's considered a global_id
     * \param group A list of groups separated by commas or spaces
     * \thead external \lock data
     */
    bool add_to_group(const std::string& user, const std::string& group) override;

    /**
     * \param user  A user local_id, if it begins with a \@, it's considered a
     *              host name, if it begins with a !, it's considered a global_id
     * \param group A list of groups separated by commas or spaces
     * \thead external \lock data
     */
    bool remove_from_group(const std::string& user, const std::string& group) override;

    /**
     * \thead external \lock data
     */
    std::vector<user::User> users_in_group(const std::string& group) const override;
    /**
     * \thead external \lock data
     */
    std::vector<user::User> real_users_in_group(const std::string& group) const override;

    /**
     * \brief Build a user::User from an extended name
     * \param exname A user local_id, if it begins with a \@, it's considered a
     *               host name, if it begins with a !, it's considered a global_id
     */
    user::User build_user(const std::string& exname) const;

    /**
     * \brief Returns properties reported by RPL_ISUPPORT, for features
     *        without a value "1" is reported.
     * \thead external \lock data
     */
    LockedProperties properties() override;

    /*
     * \thead external \lock data
     */
    string::FormattedProperties pretty_properties() const override;

    /**
     * \thead external \lock data
     */
    user::UserCounter count_users(const std::string& channel = {}) const override;

private:
    friend class Buffer;

    /**
     * \brief Handle a IRC message
     * \thread irc_in \lock data(sometimes) buffer(indirect, sometimes)
     */
    void handle_message(network::Message line);

    /**
     * \brief Extablish connection to the IRC server
     * \thread irc_in \lock buffer(indirect)
     */
    void login();

    /**
     * \brief AUTH to the server
     * \thread irc_in \lock buffer(indirect)
     */
    void auth();

    /**
     * \brief Read members from the given settings
     */
    void read_settings(const Settings& settings);

    /**
     * \brief Stop caused by an error
     */
    void error_stop(const std::string& message);

    /**
     * \brief Removes a user from a channel and
     *        if it has no more channels from the user manager
     * \thread external \lock data
     */
    void remove_from_channel(const std::string& user_id,
                             const std::vector<std::string>& channels);

    /**
     * \brief Parse :Nick!User@host
     * \see http://tools.ietf.org/html/rfc2812#section-2.3.1
     */
    user::User parse_prefix(const std::string& prefix);

    mutable std::mutex mutex;

    /**
     * \brief IRC server to connect to
     */
    network::Server main_server;

    /**
     * \brief Server the bot is connected to
     */
    network::Server current_server;

    /**
     * \brief Command buffer
     */
    Buffer buffer;

    /**
     * \brief Network/Bouncer password
     */
    std::string server_password;
    /**
     * \brief Contains the IRC features advertized by the server
     *
     * As seen on 005 (RPL_ISUPPORT)
     * \see http://www.irc.org/tech_docs/005.html
     */
    PropertyTree properties_;
    /**
     * \brief Current bot nick
     */
    std::string current_nick;
    /**
     * \brief Current bot nick (normalized to lowercase)
     */
    std::string current_nick_lowecase;
    /**
     * \brief Nick that should be used
     */
    std::string preferred_nick;
    /**
     * \brief Nick used by the latest NICK command
     */
    std::string attempted_nick;
    /**
     * \brief Whether private messages to other users shall be done via "NOTICE"
     *        instead than "PRIVMSG"
     */
    bool private_notice = true;
    /**
     * \brief Input formatter
     */
    string::Formatter* formatter_ = nullptr;
    /**
     * \brief List of commands which could not be processed right away
     */
    std::list<network::Command> scheduled_commands;
    /**
     * \brief Connection status
     */
    AtomicStatus connection_status;

    /**
     * \brief User manager
     */
    user::UserManager user_manager;
    /**
     * \brief User authorization system
     */
    user::AuthSystem  auth_system;
};

} // namespace irc
#endif // IRC_CONNECTION_HPP
