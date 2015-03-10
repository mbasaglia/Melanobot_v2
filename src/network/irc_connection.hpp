/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015 Mattia Basaglia
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

#include "connection.hpp"
#include "irc_buffer.hpp"

#include "melanobot.hpp"

namespace network {
/**
 * \brief Namespace contaning classes and functions handling the IRC protocol
 */
namespace irc {

/**
 * \brief IRC connection
 */
class IrcConnection : public Connection
{
public:
    /**
     * \brief Create from settings
     */
    static IrcConnection* create(Melanobot* bot, const Settings& settings);

    /**
     * \thread main \lock none
     */
    IrcConnection(Melanobot* bot, const Server& server, const Settings& settings = {});
    
    /**
     * \thread main \lock none
     */
    ~IrcConnection() override;

    /**
     * \thread external \lock none
     */
    void start() override;

    /**
     * \thread external \lock buffer(indirect)
     */
    void stop() override;

    /**
     * \thread ? \lock data
     */
    Server server() const override;

    /**
     * \thread external \lock data(sometimes) buffer(indirect)
     */
    void command ( const Command& cmd ) override;

    /**
     * \thread external \lock data(sometimes) buffer(indirect)
     */
    void say ( const OutputMessage& message ) override;

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
    void disconnect(const std::string& message = {}) override;

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
    void reconnect();

    /**
     * \return The bot's current nick
     * \thead external \lock data
     */
    std::string name() const override;

    /**
     * \brief Parse :Nick!User@host
     * \see http://tools.ietf.org/html/rfc2812#section-2.3.1
     */
    static user::User parse_prefix(const std::string& prefix);

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
     * \brief Build a user::User from an extended name
     * \param exname A user local_id, if it begins with a \@, it's considered a
     *               host name, if it begins with a !, it's considered a global_id
     */
    user::User build_user(const std::string& exname) const;

    /**
     * \brief Returns properties reported by RPL_ISUPPORT, for features
     *        without a value "1" is reported.
     */
    std::string get_property(const std::string& property) const override;

    /**
     * \brief Always fails
     */
    bool set_property(const std::string& property, const std::string value ) override;

private:
    friend class Buffer;

    /**
     * \brief Handle a IRC message
     * \thread irc_in \lock data(sometimes) buffer(indirect, sometimes)
     */
    void handle_message(Message line);

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

    mutable std::mutex mutex;

    Melanobot* bot;

    /**
     * \brief IRC server to connect to
     */
    Server main_server;

    /**
     * \brief Server the bot is connected to
     */
    Server current_server;

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
    Properties server_features;
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
     * \brief Nick used to AUTH
     */
    std::string auth_nick;
    /**
     * \brief Password used to AUTH
     */
    std::string auth_password;
    /**
     * \brief Modes to set after AUTH
     */
    std::string modes;
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
    std::list<Command> scheduled_commands;
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

} // namespace network::irc
} // namespace network
#endif // IRC_CONNECTION_HPP
