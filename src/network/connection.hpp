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
#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <atomic>
#include <chrono>
#include <cstdint>
#include <sstream>

#include "string/string.hpp"
#include "string/logger.hpp"
#include "user/auth_system.hpp"
#include "user/user_manager.hpp"

class Melanobot;

namespace network {

typedef std::chrono::steady_clock Clock;
typedef Clock::time_point         Time;
typedef Clock::duration           Duration;

/**
 * \brief Identifies a network server
 */
struct Server
{
    std::string host;
    uint16_t    port;

    Server( const std::string& host, uint16_t port ) : host(host), port(port) {}


    /**
     * \brief Creates object from host:port string
     */
    explicit Server( const std::string& server )
        : port(0)
    {
        auto p = server.find(':');
        host = server.substr(0,p);
        if ( p != std::string::npos && p < server.size()-1 && std::isdigit(server[p+1]) )
            port = std::stoul(server.substr(p+1));
    }

    std::string name() const
    {
        std::ostringstream ss;
        ss << host << ':' << port;
        return ss.str();
    }
};

/**
 * \brief A command to send to a connection
 */
struct Command
{
    std::string                 command;        ///< Command name
    std::vector<std::string>    parameters;     ///< Optional parameters
    int                         priority;       ///< Priority, higher = handled sooner
    Time                        timein;         ///< Time of creation
    Time                        timeout;        ///< Time it becomes obsolete

    Command ( const std::string&        command = {},
        const std::vector<std::string>& parameters = {},
        int                             priority = 0,
        const Time&                     timeout = Time::max() )
    :   command(command),
        parameters(parameters),
        priority(priority),
        timein(Clock::now()),
        timeout(timeout)
    {}

    Command ( const std::string&        command,
        const std::vector<std::string>& parameters,
        int                             priority,
        const Duration&                 duration )
    :   command(command),
        parameters(parameters),
        priority(priority),
        timein(Clock::now()),
        timeout(Clock::now()+duration)
    {}

    bool operator< ( const Command& other ) const
    {
        return priority < other.priority ||
            ( priority == other.priority && timein > other.timein );
    }
};

/**
 * \brief A message originating from a connection
 */
struct Message
{
    class Connection*        source;  ///< Connection originating this message
    std::string              raw;     ///< Raw contents
    std::string              command; ///< Protocol command name
    std::vector<std::string> params;  ///< Tokenized parameters
    std::string              from;    ///< (optional) Name of the user who created this command

    std::string              message; ///< (optional) Simple message contents
    std::vector<std::string> channels;///< (optional) Simple message origin
    bool                     action{0};///<(optional) Simple message is an action
    bool                     direct{0};///<(optional) Simple message has been addessed to the bot directly
};

/**
 * \brief A message given to a connection
 *
 * This is similar to \c Command but at a higher level
 * (doesn't require knowledge of the protocol used by the connection)
 */
struct OutputMessage
{
    OutputMessage(std::string target,
                  const string::FormattedString& message,
                  int priority = 0,
                  const string::FormattedString& prefix = {},
                  const string::FormattedString& from = {},
                  const Time& timeout = Clock::time_point::max()
    ) : target(target), message(message), from(from), prefix(prefix),
        action(false), priority(priority), timeout(timeout)
    {}


    /**
     * \brief Channel or user id to which the message shall be delivered to
     */
    std::string target;
    /**
     * \brief Message contents
     */
    string::FormattedString message;
    /**
     * \brief If not empty, the bot will make it look like the message comes from this user
     */
    string::FormattedString from;
    /**
     * \brief Prefix to prepend to the message
     */
    string::FormattedString prefix;
    /**
     * \brief Whether the message is an action
     */
    bool action = false;
    /**
     * \brief Priority, higher = handled sooner
     */
    int priority = 0;
    /**
     * \brief Time at which this message becomes obsolete
     */
    Time timeout;
};

/**
 * \brief Abstract service connection
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
    typedef std::atomic<Status> AtomicStatus;

    virtual ~Connection() {}

    /**
     * \brief Returns the server object to which this connection is connected to
     */
    virtual Server server() const = 0;

    /**
     * \brief Schedules a command for execution
     */
    virtual void command ( const Command& cmd ) = 0;

    /**
     * \brief Sends a message to the given channel
     */
    virtual void say ( const OutputMessage& message ) = 0;

    /**
     * \brief Returns the connection status
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
    virtual void disconnect(const std::string& message = {}) = 0;

    /**
     * \brief Disconnect and stop all processing
     */
    virtual void stop() = 0;

    /**
     * \brief Start processing messages
     */
    virtual void start() = 0;

    /**
     * \brief Get the string formatter
     */
    virtual string::Formatter* formatter() const = 0;

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
     * \param group Group name
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
     * \brief Get a vector with the users in the given group
     */
    virtual std::vector<user::User> users_in_group(const std::string& group) const = 0;

    /**
     * \brief Name of the service provided by this connection,
     *        as seen by the handled protocol
     */
    virtual std::string name() const = 0;

    /* *
     * \brief Get a connection property
     */
    //virtual std::string get_property(const std::string& property) const = 0;

    /* *
     * \brief Set a connection property
     */
    //virtual bool set_property(const std::string& property, const std::string value ) = 0;
};

#define REGISTER_CONNECTION(name,function) \
    static network::ConnectionFactory::RegisterConnection RegisterConnection_##name(#name,function)
/**
 * \brief Creates connections from settings
 */
class ConnectionFactory
{
public:
    typedef std::function<Connection*(Melanobot* bot, const Settings&)> Contructor;
    /**
     * \brief Auto-registration helper
     */
    class RegisterConnection
    {
    public:
        RegisterConnection(const std::string& protocol_name, const Contructor& function)
        {
            ConnectionFactory::instance().register_connection(protocol_name, function);
        }
    };

    /**
     * \brief Singleton instance
     */
    static ConnectionFactory& instance()
    {
        static ConnectionFactory singleton;
        return singleton;
    };

    /**
     * \brief Registers a connection type
     * \throws CriticalException If a protocol is defined twice
     */
    void register_connection ( const std::string& protocol_name, const Contructor& function )
    {
        if ( factory.count(protocol_name) )
            CRITICAL_ERROR("Re-registering connection protocol "+protocol_name);
        factory[protocol_name] = function;
    }

    /**
     * \brief Creates a connection from its settings
     */
    Connection* create(Melanobot* bot, const Settings& settings)
    {
        std::string protocol = settings.get("protocol",std::string());
        auto it = factory.find(protocol);
        if ( it != factory.end() )
            return it->second(bot, settings);

        Log("sys",'!',0) << color::red << "Error" << color::nocolor
            << ": Uknown connection protocol "+protocol;
        return nullptr;
    }

private:
    ConnectionFactory(){}
    ConnectionFactory(const ConnectionFactory&) = delete;

    std::unordered_map<std::string,Contructor> factory;
};


} // namespace network
#endif // CONNECTION_HPP
