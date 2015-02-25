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
#include <regex>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include <boost/asio.hpp>

#include "connection.hpp"
#include "../melanobot.hpp"
#include "../settings.hpp"

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

class Network
{
public:
    typedef std::vector<Server> ServerList;

    explicit Network ( const Server& server ) : Network(ServerList{server}) {}

    explicit Network ( const ServerList& servers )
        : servers(servers)
    {
        if ( servers.empty() )
            throw std::logic_error("Empty network");
    }

    Network ( const Network& other ) : Network(other.servers) {}

    /**
     * \note Deleted because it would require a lock
     */
    Network& operator= ( const Network& other ) = delete;

    const Server& current() const
    {
        return servers[current_server];
    }

    const Server& next()
    {
        ++current_server;
        if ( current_server == servers.size() )
            current_server = 0;
        return current();
    }

    int size() const
    {
        return servers.size();
    }

private:
    ServerList servers;
    std::atomic<unsigned> current_server { 0 };
};

class IrcConnection;

class Buffer
{
public:
    explicit Buffer(IrcConnection& irc, const Settings& settings = {});

    /**
     * \brief Run the async process
     */
    void run();

    /**
     * \brief Inserts a command to the buffer
     */
    void insert(const Command& cmd);

    /**
     * \brief Check if the buffer is empty
     */
    bool empty() const;

    /**
     * \brief Process the top command (if any)
     */
    void process();
    /**
     * \brief Clear all commands
     */
    void clear();

    /**
     * \brief Outputs directly a line from the command
     */
    void write(const Command& cmd);

    /**
     * \brief Connect to the given server
     */
    void connect(const Server& server);

    /**
     * \brief Disconnect from the server
     */
    void disconnect();

    bool connected() const;

private:
    mutable std::mutex mutex; /// locks access to the buffer

    IrcConnection& irc;

// Flood:
    /**
     * \brief Store messages when it isn't possible to send them
     */
    std::priority_queue<Command> buffer;
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
     * \brief Schedules an asynchronous line read
     */
    void schedule_read();

    /**
     * \brief Writes a line to the socket
     */
    void write_line ( std::string line );

    /**
     * \brief Async hook on network input
     */
    void on_read_line(const boost::system::error_code &error);


};

class IrcConnection : public Connection
{
public:
    /**
     * \brief Logs some kind of non-fatal error (eg: malformed commands)
     */
    static void error(const std::string& message);

    IrcConnection(Melanobot* bot, const Network& network, const Settings& settings = {});
    IrcConnection(Melanobot* bot, const Server& server, const Settings& settings = {});
    ~IrcConnection() override;

    void run() override;

    const Server& server() const override;

    void command ( const Command& cmd ) override;

    void say ( const std::string& channel,
        const std::string& message,
        int priority = 0,
        const Time& timeout = Time::max() ) override;

    void say_as ( const std::string& channel,
        const std::string& name,
        const std::string& message,
        int priority = 0,
        const Time& timeout = Time::max()  ) override;

    Status status() const override;

    std::string protocol() const override;

    std::string connection_name() const override;

    void connect() override;

    void disconnect() override;

    /**
     * \brief Quit and connect
     */
    void reconnect();

    std::string nick() const;
    std::string normalized_nick() const;

private:

    friend class Buffer;

    /**
     * \brief Handle a IRC message
     */
    void handle_message(const Message& line);

    /**
     * \brief Extablish connection to the IRC server
     */
    void login();

    /**
     * \brief AUTH to the server
     */
    void auth();

    /**
     * \brief Regular expression for IRC messages
     */
    static const std::regex re_message;

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
};

} // namespace network::irc
} // namespace network
#endif // IRC_HPP
