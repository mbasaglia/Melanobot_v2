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
#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <atomic>
#include <chrono>
#include <cstdint>
#include <sstream>

#include "../string/string.hpp"
#include "../string/logger.hpp"

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

    Command ( const std::string&        command,
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
     * \brief Keeps the connection going (synchronous)
     */
    virtual void run() = 0;

    /**
     * \brief Returns the server object to which this connection is connected to
     */
    virtual const Server& server() const = 0;

    /**
     * \brief Schedules a command for execution
     */
    virtual void command ( const Command& cmd ) = 0;

    /**
     * \brief Sends a message to the given channel
     */
    virtual void say ( const std::string& channel,
        const std::string& message,
        int priority = 0,
        const Time& timeout = Clock::time_point::max() ) = 0;

    /**
     * \brief Sends a message to the given channel with the given name
     */
    virtual void say_as ( const std::string& channel,
        const std::string& name,
        const std::string& message,
        int priority = 0,
        const Time& timeout = Clock::time_point::max()  ) = 0;

    /**
     * \brief Returns the connection status
     */
    virtual Status status() const = 0;

    /**
     * \brief Protocol identifier
     */
    virtual std::string protocol() const = 0;

    /**
     * \brief Connection name
     */
    virtual std::string connection_name() const = 0;

    /**
     * \brief Initialize the connection
     */
    virtual void connect() = 0;

    /**
     * \brief Close the connection
     */
    virtual void disconnect() = 0;

    /**
     * \brief Get the string formatter
     */
    virtual string::Formatter* formatter() = 0;

};

/**
 * \brief A message originating from a connection
 */
struct Message
{
    Connection*              source;  ///< Connection originating this message
    std::string              raw;     ///< Raw contents
    std::string              command; ///< Protocol command name
    std::vector<std::string> params;  ///< Tokenized parameters
    std::string              message; ///< (optional) Message contents
    std::vector<std::string> channels;///< (optional) Channels originating from
    std::string              from;    ///< (optional) Name of the user who created this command
};

#define REGISTER_CONNECTION(name,function) \
    static ConnectionFactory::RegisterConnection register_##name(#name,function)
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
