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
#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <atomic>
#include <cstdint>

#include "concurrency/locked_properties.hpp"
#include "message/input_message.hpp"
#include "message/output_message.hpp"
#include "string/logger.hpp"
#include "user/auth_system.hpp"
#include "user/user_counter.hpp"
#include "user/user_manager.hpp"

namespace network {

/**
 * \brief Abstract service connection
 *
 * Instances of this class will be created by \c Melanobot from the
 * configuration and will send \c Message objects and receive either
 * \c Command or \c OutputMessage objects.
 *
 * To create your own class, inherith this one and override the pure virtual
 * methods. Then use Melanomodule::register_connection() to register
 * the class to the \c ConnectionFactory.
 *
 * It is recommended that you add a static method called \c create in the
 * derived class with the following signature:
 * \code{.cpp}
 *      std::unique_ptr<YourClass> create(const Settings& settings, const std::string& name)
 * \endcode
 * to be used with Melanomodule::register_connection().
 *
 * You should also have a corresponding log type to use for the connection,
 * registered with Melanomodule::register_connection().
 */
class Connection
{
public:
    enum Status
    {
        DISCONNECTED,   ///< Connection is completely disconnected
        WAITING,        ///< Needs something before connecting
        CONNECTING,     ///< Needs some protocol action before becoming usable
        CHECKING,       ///< Connected, making sure the connection is alive
        CONNECTED,      ///< All set
    };
    using AtomicStatus = std::atomic<Status>;

    explicit Connection(std::string config_name) : config_name_(std::move(config_name)) {}
    Connection(const Connection&) = delete;
    Connection(Connection&&) = delete;
    Connection& operator=(const Connection&) = delete;
    Connection& operator=(Connection&&) = delete;

    virtual ~Connection() {}

    /**
     * \brief Returns the server object to which this connection is connected to
     */
    virtual Server server() const = 0;

    /**
     * \brief Returns a 1-line description of the connection (including server info)
     */
    virtual std::string description() const = 0;

    /**
     * \brief Schedules a command for execution
     */
    virtual void command ( Command cmd ) = 0;

    /**
     * \brief Sends a message to the given channel
     */
    virtual void say ( const OutputMessage& message ) = 0;

    /**
     * \brief Returns the connection status
     * \todo is this ever called?
     */
    virtual Status status() const = 0;

    /**
     * \brief Protocol identifier
     */
    virtual std::string protocol() const = 0;

    /**
     * \brief Initialize the connection
     */
    virtual void connect() = 0;

    /**
     * \brief Close the connection
     */
    virtual void disconnect(const string::FormattedString& message = {}) = 0;

    /**
     * \brief Disconnect and connect again
     */
    virtual void reconnect(const string::FormattedString& quit_message = {}) = 0;

    /**
     * \brief Disconnect and stop all processing
     */
    virtual void stop() { disconnect(); }

    /**
     * \brief Start processing messages
     */
    virtual void start() { connect(); }

    /**
     * \brief Get the string formatter
     */
    virtual string::Formatter* formatter() const = 0;

    /**
     * \brief Decodes \c input with formatter()
     */
    string::FormattedString decode(const std::string& input) const
    {
        return formatter()->decode(input);
    }

    /**
     * \brief Decodes \c input with formatter() and re-encodes it using \c target_format
     */
    std::string encode_to(const std::string& input,
                          const string::Formatter& target_format) const
    {
        return decode(input).encode(target_format);
    }

    /**
     * \brief Whether a list of channels matches the mask
     * (Meaning depends on the specialized class)
     */
    virtual bool channel_mask(const std::vector<std::string>& channels,
                              const std::string& mask) const = 0;

    /**
     * \brief Get whether a user has the given authorization level
     */
    virtual bool user_auth(const std::string& local_id,
                           const std::string& auth_group) const = 0;
    /**
     * \brief Update the properties of a user by local_id
     */
    virtual void update_user(const std::string& local_id,
                             const Properties& properties) = 0;

    /**
     * \brief Update the properties of a user by local_id
     * \param local_id ID of the user as previously seen by the connection
     * \param updated  New user, all properties of the new user will be overwritten
     */
    virtual void update_user(const std::string& local_id,
                             const user::User& updated) = 0;

    /**
     * \brief Returns a copy of the user object identified by \c local_id
     * \return A copy of the internal record identifying the user or
     *         an empty object if the user is not found.
     */
    virtual user::User get_user(const std::string& local_id) const = 0;

    /**
     * \brief Get a vector with the users in the given channel
     *        or all the users if empty
     */
    virtual std::vector<user::User> get_users( const std::string& channel_mask = "" ) const = 0;

    /**
     * \brief Adds a user identified by \c user to the group \c group
     * \param user  The user \c local_id, derived classes may perform some
     *              parsing to interpret it as a host or global_id
     * \param group Group name or CSV list of names
     * \return whether the insertion was successful
     */
    virtual bool add_to_group(const std::string& user, const std::string& group) = 0;

    /**
     * \brief Removes a user identified by \c user to the group \c group
     * \param user  The user \c local_id, derived classes may perform some
     *              parsing to interpret it as a host or global_id
     * \param group Group name
     */
    virtual bool remove_from_group(const std::string& user, const std::string& group) = 0;

    /**
     * \brief Get a vector with the users in the given group (as set from the config)
     */
    virtual std::vector<user::User> users_in_group(const std::string& group) const = 0;

    /**
     * \brief Get a vector with the users in the given group (currently connected)
     */
    virtual std::vector<user::User> real_users_in_group(const std::string& group) const = 0;

    /**
     * \brief Name of the service provided by this connection,
     *        as seen by the handled protocol
     */
    virtual std::string name() const = 0;

    /**
     * \brief Get connection properties
     */
    virtual LockedProperties properties() = 0;

    /**
     * \brief Counts the number of users in a channel (or the whole connection),
     *        as visible to the bot.
     */
    virtual user::UserCounter count_users(const std::string& channel = {} ) const = 0;

    /**
     * \brief Returns a list of properties used to message formatting
     * \note Returned properties should be formatted using the FormatterConfig
     */
    virtual string::FormattedProperties pretty_properties() const = 0;

    /**
     * \brief Returns a list of properties used to message formatting
     * \note Returned properties should be formatted using the FormatterConfig
     */
    virtual string::FormattedProperties pretty_properties(const user::User& user) const
    {
        string::FormattedProperties props = pretty_properties();
        props.insert({
            {"name",     decode(user.name)},
            {"ip",       user.host},
            {"local_id", user.local_id},
            {"global_id", user.global_id},
            {"host",     user.host},
        });
        props.insert(user.properties.begin(), user.properties.end());
        return props;
    }

    /**
     * \brief Returns the name of the connection as used in the config
     */
    const std::string& config_name() const { return config_name_; }

private:
    std::string config_name_;
};

/**
 * \brief Base class for connections with a single user and channel
 */
class SingleUnitConnection : public Connection
{
public:
    using Connection::Connection;


    bool channel_mask(const std::vector<std::string>&,  const std::string&) const override { return true; }
    bool user_auth(const std::string&, const std::string&) const override { return true; }
    void update_user(const std::string&, const Properties& ) override {}
    void update_user(const std::string&, const user::User& ) override {}
    user::User get_user(const std::string&) const override { return {}; }
    std::vector<user::User> get_users( const std::string& ) const override  { return {}; }
    bool add_to_group(const std::string&, const std::string&) override { return false; }
    bool remove_from_group(const std::string&, const std::string& ) override { return false; }
    std::vector<user::User> users_in_group(const std::string&) const override { return {}; }
    std::vector<user::User> real_users_in_group(const std::string& group) const override { return {}; }
    user::UserCounter count_users(const std::string& channel = {}) const override { return {}; }
    std::string name() const override { return protocol(); }
};

/**
 * \brief Creates connections from settings
 */
class ConnectionFactory : public melanolib::Singleton<ConnectionFactory>
{
public:
    using Contructor = std::function<std::unique_ptr<Connection>
        (const Settings& settings, const std::string&name)>;

    /**
     * \brief Registers a connection type
     * \throws ConfigurationError If a protocol is defined twice
     */
    void register_connection ( const std::string& protocol_name,
                               const Contructor& function );

    /**
     * \brief Creates a connection from its settings
     */
    std::unique_ptr<Connection> create(const Settings& settings,
                                       const std::string& name
                                      );

private:
    ConnectionFactory(){}
    friend ParentSingleton;

    std::unordered_map<std::string, Contructor> factory;
};


} // namespace network

#endif // CONNECTION_HPP
