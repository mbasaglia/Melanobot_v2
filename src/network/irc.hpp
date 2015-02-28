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

#ifndef IRC_HPP
#define IRC_HPP

#include <algorithm>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include <boost/asio.hpp>

#include "concurrent_container.hpp"
#include "connection.hpp"
#include "../melanobot.hpp"
#include "../settings.hpp"
#include "../string/logger.hpp"

namespace network {
namespace irc {

/**
 * \brief Converts a string to lower case
 * \see http://tools.ietf.org/html/rfc2812#section-2.2
 */
inline std::string strtolower ( std::string string )
{
    std::transform(string.begin(),string.end(),string.begin(),[](char c)->char {
        switch (c) {
            case '[': return '{';
            case ']': return '}';
            case '\\': return '|';
            case '~': return '^';
            default:  return std::tolower(c);
        }
    });
    return string;
}
/**
 * \brief Converts a string to upper case
 * \see http://tools.ietf.org/html/rfc2812#section-2.2
 */
inline std::string strtoupper ( std::string string )
{
    std::transform(string.begin(),string.end(),string.begin(),[](char c)->char {
        switch (c) {
            case '{': return '[';
            case '}': return ']';
            case '|': return '\\';
            case '^': return '~';
            default:  return std::toupper(c);
        }
    });
    return string;
}

/**
 * \brief A collection of servers
 */
class Network
{
public:
    typedef std::vector<Server> ServerList;

    explicit Network ( const Server& server ) : Network(ServerList{server}) {}

    explicit Network ( const ServerList& servers )
        : servers(servers)
    {
        if ( servers.empty() )
            CRITICAL_ERROR("Empty network");
    }

    Network ( const Network& other ) : Network(other.servers) {}

    /**
     * \note Deleted because it would require a lock
     */
    Network& operator= ( const Network& other ) = delete;

    /**
     * \brief Current server
     */
    const Server& current() const
    {
        return servers[current_server];
    }

    /**
     * \brief Advances to the next server in the list and returns it
     * \note Acts as a circular buffer, will always return a valid server
     */
    const Server& next()
    {
        ++current_server;
        if ( current_server == servers.size() )
            current_server = 0;
        return current();
    }

    /**
     * \brief Number of servers
     * \note Always >= 0
     */
    int size() const
    {
        return servers.size();
    }

private:
    ServerList servers;
    /**
     * \todo needs atomic?
     */
    std::atomic<unsigned> current_server { 0 };
};

class IrcConnection;

/**
 * \brief IRC buffer
 *
 * Contains the network connection and performs flood checking
 */
class Buffer
{
public:
    explicit Buffer(IrcConnection& irc, const Settings& settings = {});

    /**
     * \brief Inserts a command to the buffer
     * \thread external \lock buffer
     */
    void insert(const Command& cmd);

    /**
     * \brief Process the top command (if any)
     * \thread queue \lock buffer
     */
    void process();

    /**
     * \brief Outputs directly a line from the command
     * \thread irc queue \lock none
     */
    void write(const Command& cmd);

    /**
     * \brief Connect to the given server
     * \thread irc, queue \lock none
     */
    void connect(const Server& server);

    /**
     * \brief Disconnect from the server
     * \thread irc, queue \lock none
     */
    void disconnect();

    /**
     * \brief Starts the io service
     */
    void start();

    /**
     * \brief Stops the io service
     */
    void stop();

    /**
     * \brief Checks if the connection is active
     * \thread irc, queue \lock none
     */
    bool connected() const;

private:

    IrcConnection& irc;

// Flood:
    /**
     * \brief Store messages when it isn't possible to send them
     */
    ConcurrentPriorityQueue<Command> buffer;
    /**
     * \brief Thread running the output
     */
    std::thread thread_output;
    /**
     * \brief Thread running the input
     */
    std::thread thread_input;
    /**
     * \brief Maximum nuber of bytes in a message (longer messages will be truncated)
     * \see http://tools.ietf.org/html/rfc2812#section-2.3
     */
    unsigned    flood_max_length = 510;
    /**
     * \brief Message timer
     * \see http://tools.ietf.org/html/rfc2813#section-5.8
     */
    Time        flood_timer;
    /**
     * \brief Maximum duration the flood timer can be ahead of now
     * \see http://tools.ietf.org/html/rfc2813#section-5.8
     */
    Duration    flood_timer_max;
    /**
     * \brief Message penalty
     * \see http://tools.ietf.org/html/rfc2813#section-5.8
     */
    Duration    flood_timer_penalty;

// Network:
    boost::asio::streambuf              buffer_read;
    boost::asio::streambuf              buffer_write;
    boost::asio::io_service             io_service;
    boost::asio::ip::tcp::resolver      resolver{io_service};
    boost::asio::ip::tcp::socket        socket{io_service};

    /**
     * \brief While active, keep processing reads
     */
    void run_input();
    /**
     * \brief While active, keep processing writes
     */
    void run_output();

    /**
     * \brief Schedules an asynchronous line read
     * \thread irc, async_read \lock none (async)
     */
    void schedule_read();

    /**
     * \brief Writes a line to the socket
     * \thread irc, queue \lock none TODO?
     */
    void write_line ( std::string line );

    /**
     * \brief Async hook on network input
     * \thread async_read \lock none
     */
    void on_read_line(const boost::system::error_code &error);


};

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
    IrcConnection(Melanobot* bot, const Network& network, const Settings& settings = {});
    /**
     * \thread main \lock none
     */
    IrcConnection(Melanobot* bot, const Server& server, const Settings& settings = {});
    
    /**
     * \thread main \lock none
     */
    ~IrcConnection() override;

    /**
     * \thread irc \lock none
     */
    void start() override;

    /**
     * \thread main \lock buffer(indirect)
     */
    void stop() override;

    /**
     * \thread ? \lock buffer
     */
    const Server& server() const override;

    /**
     * \thread irc, external, async_read \lock data(sometimes) buffer(call)
     */
    void command ( const Command& cmd ) override;

    /**
     * \thread external \lock data(sometimes) buffer(call)
     */
    void say ( const std::string& channel,
        const std::string& message,
        int priority = 0,
        const Time& timeout = Time::max() ) override;

    /**
     * \thread external \lock data(sometimes) buffer(call)
     */
    void say_as ( const std::string& channel,
        const std::string& name,
        const std::string& message,
        int priority = 0,
        const Time& timeout = Time::max()  ) override;

    /**
     * \thread ? \lock none
     */
    Status status() const override;

    /**
     * \thread ? \lock none
     */
    std::string protocol() const override;

    /**
     * \thread ? \lock none
     */
    std::string connection_name() const override;

    /**
     * \thread main, external \lock buffer(indirect) data(indirect)
     */
    void connect() override;

    /**
     * \thread main, external, irc, async_read \lock buffer(indirect)
     */
    void disconnect() override;

    /**
     * \thread external \lock none
     */
    string::Formatter* formatter() override;

    /**
     * \brief disconnect and connect
     * \thread async_read \lock buffer(indirect) data(indirect)
     */
    void reconnect();

    /**
     * \brief Bot's current nick
     * \thread ? \lock data
     */
    std::string nick() const;

private:

    friend class Buffer;

    /**
     * \brief Handle a IRC message
     * \thread async_read \lock data(sometimes) buffer(indirect, sometimes)
     */
    void handle_message(const Message& line);

    /**
     * \brief Extablish connection to the IRC server
     * \thread async_read \lock data buffer(indirect)
     */
    void login();

    /**
     * \brief AUTH to the server
     * \thread async_read \lock data buffer(indirect)
     */
    void auth();

    mutable std::mutex mutex;

    Melanobot* bot;

    /**
     * \brief IRC network
     */
    Network network;

    /**
     * \brief Command buffer
     */
    Buffer buffer;

    /**
     * \brief Network/Bouncer password
     */
    std::string network_password;
    /**
     * \brief Contains the IRC features advertized by the server
     *
     * As seen on 005 (RPL_ISUPPORT)
     * \see http://www.irc.org/tech_docs/005.html
     */
    std::unordered_map<std::string,std::string> server_features;
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
     * \brief Input formatter
     */
    string::Formatter* formatter_ = nullptr;
};

} // namespace network::irc
} // namespace network
#endif // IRC_HPP
