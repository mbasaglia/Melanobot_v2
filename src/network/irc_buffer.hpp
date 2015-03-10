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
#ifndef IRC_BUFFER_HPP
#define IRC_BUFFER_HPP

#include <mutex>
#include <thread>

#include <boost/asio.hpp>

#include "connection.hpp"
#include "concurrent_container.hpp"
#include "string/logger.hpp"

namespace network {
namespace irc {

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
    ~Buffer() { stop(); }

    /**
     * \brief Inserts a command to the buffer
     * \thread external \lock buffer
     */
    void insert(const Command& cmd);

    /**
     * \brief Process the top command (if any)
     * \thread irc_out \lock buffer(waits for input, waits for flood timer)
     */
    void process();

    /**
     * \brief Outputs directly a line from the command
     * \thread irc_out, external(quit) \lock none
     */
    void write(const Command& cmd);

    /**
     * \brief Connect to the given server
     * \thread external \lock none
     */
    bool connect( const network::Server& server );

    /**
     * \brief Disconnect from the server
     * \thread external \lock none
     */
    void disconnect();

    /**
     * \brief Starts the io service
     * \thread main
     */
    void start();

    /**
     * \brief Stops the io service
     * \thread main
     */
    void stop();

    /**
     * \brief Checks if the connection is active
     * \thread external \lock none
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
     *
     * When \c flood_timer reaches \c flood_timer_max, the buffer will have
     * to wait a while before sending a new message to the server
     */
    Duration    flood_timer_max;
    /**
     * \brief Message penalty
     * \see http://tools.ietf.org/html/rfc2813#section-5.8
     *
     * Fixed amount added to \c flood_timer with each message.
     */
    Duration    flood_message_penalty;
    /**
     * \brief Message size penalty
     *
     * Number of bytes which will cause an extra second to be added
     * \c flood_timer when a message is sent to the server
     */
    int         flood_bytes_penalty = 0;

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
     * \thread irc_in \lock none (async)
     */
    void schedule_read();

    /**
     * \brief Writes a line to the socket
     * \thread irc_out, external(quit) \lock none
     */
    void write_line ( std::string line );

    /**
     * \brief Async hook on network input
     * \thread irc_in \lock none
     */
    void on_read_line(const boost::system::error_code &error);


};

} // namespace irc
} // namespace network
#endif // IRC_BUFFER_HPP
