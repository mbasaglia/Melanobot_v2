/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015-2016 Mattia Basaglia
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
#ifndef DARKPLACES_HPP
#define DARKPLACES_HPP

#include <list>

#include "connection_details.hpp"
#include "network/udp_io.hpp"
#include "network/time.hpp"
#include "concurrency/concurrency.hpp"

namespace xonotic {

class Darkplaces
{
public:
    explicit Darkplaces(xonotic::ConnectionDetails details);

    virtual ~Darkplaces();

    /**
     * \brief Whether the widget is connected to the darkplaces server
     */
    bool connected() const;


    /**
     * \brief Returns darkplaces connection details
     */
    const ConnectionDetails& details() const { return connection_details; }

    /**
     * \brief Sets darkplaces connection details and reconnects if needed
     */
    void set_details(const ConnectionDetails& details);

    /**
     * \brief Runs a rcon command
     */
    void rcon_command(std::string command);

    /**
     * \brief Writes a raw command to the darkplaces server
     */
    void write(std::string line);

    /**
     * \brief Open the connection to the darkplaces server
     * \returns Whether the connection has been successful
     */
    bool connect();

    /**
     * \brief Close the darkplaces connection and clear connection data
     */
    void disconnect();

    /**
     * \brief Diconnect and connect from darkplaces
     */
    bool reconnect();

    /**
     * \brief Local connection endpoint
     */
    network::Server local_endpoint() const;

protected:
    /**
     * \brief Called after a successful connection
     */
    virtual void on_connect() {}

    /**
     * \brief Called after being disconnected
     */
    virtual void on_disconnect() {}

    /**
     * \brief Called before being disconnected, when the connection is still open
     * \note If the connection is closed due to a network error, this might not be called
     */
    virtual void on_disconnecting() {}

    /**
     * \brief Called when received a datagram containing some log
     *
     * will follow several calls to on_receive_log() (one per log line)
     * and on_log_end()
     */
    virtual void on_log_begin() {}

    /**
     * \brief Called when received a datagram containing some log
     *
     * After on_log_begin() and on_receive_log()
     */
    virtual void on_log_end() {}

    /**
     * \brief Called after a successful network input
     * Followed by on_receive_log() or on_receive()
     */
    virtual void on_network_input(const std::string& datagram) {}

    /**
     * \brief Called after receiving a log line
     */
    virtual void on_receive_log(const std::string& line) {}

    /**
     * \brief Called after receiving a message other than log
     */
    virtual void on_receive(const std::string& cmd, const std::string& msg) {}

    /**
     * \brief Called when there is a UDP connection error
     */
    virtual void on_network_error(const std::string& msg) {}
    
    /**
     * \brief Attempty to read a message synchronously
     */
    void sync_read();

private:
    /**
     * \brief Clear connection data
     */
    void clear();

    /**
     * \brief Handles a Xonotic datagram
     */
    void read(const std::string& datagram);

    /**
     * \brief Handles a challenge recived from the server to be used in rcon_secure 2
     */
    void handle_challenge(const std::string& challenge);

    /**
     * \brief Asks the server for a challenge to be used in rcon_secure 2
     */
    void request_challenge();

    /**
     * \brief A command to be used with rcon_secure >= 2
     */
    struct Rcon2Command
    {
        std::string     command;        ///< Raw command string
        bool            challenged;     ///< Whether a challenge has been sent
        network::Time   timeout;        ///< Challenge timeout
        Rcon2Command(std::string command)
            : command(std::move(command)), challenged(false) {}
    };

    std::mutex                  mutex;
    std::string                 header{"\xff\xff\xff\xff"};     ///< Connection message header
    std::string                 line_buffer;                    ///< Buffer for overflowing messages from Xonotic
    xonotic::ConnectionDetails  connection_details;
    network::UdpIo              io;
    std::thread                 thread_input;
    std::list<Rcon2Command>     rcon2_buffer;                   ///< Buffer for rcon_secure 2 messages to be challenged
};

} // namespace xonotic
#endif // DARKPLACES_HPP
