/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2016 Mattia Basaglia
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
#ifndef MELANOBOT_MODULE_XONOTIC_ENGINE_HPP
#define MELANOBOT_MODULE_XONOTIC_ENGINE_HPP

#include <list>

#include "network/udp_io.hpp"
#include "network/time.hpp"
#include "concurrency/concurrency.hpp"
#include "melanolib/string/string_view.hpp"
#include "settings.hpp"

namespace xonotic {

/**
 * \brief Base class for Quake-like engines.
 */
class Engine
{
public:
    explicit Engine(network::Server server, std::string rcon_password,
                    std::size_t max_datagram_size);

    virtual ~Engine();

    /**
     * \brief Whether the widget is connected to the server
     */
    bool connected() const;

    /**
     * \brief Returns connection details
     */
    network::Server server() const;

    /**
     * \brief Sets connection details and reconnects
     */
    void set_server(const network::Server& server);

    std::string password() const;

    void set_password(const std::string& password);

    void set_challenge_timeout(std::chrono::seconds seconds);

    std::chrono::seconds challenge_timeout() const;

    /**
     * \brief Writes a raw command to the server
     */
    void write(std::string line);

    /**
     * \brief Open the connection to the server
     * \returns Whether the connection has been successful
     */
    bool connect();

    /**
     * \brief Close the connection and clear connection data
     */
    void disconnect();

    /**
     * \brief Diconnect and connect again
     */
    bool reconnect();

    /**
     * \brief Local connection endpoint
     */
    network::Server local_endpoint() const;

protected:

    /**
     * \brief Attempty to read a message synchronously
     */
    void sync_read();

    void schedule_challenged_command(const std::string& command);

    /**
     * \brief Runs a rcon command
     */
    virtual void rcon_command(std::string command) = 0;

    /**
     * \brief Extracts the command from an out of band message
     */
    virtual std::pair<melanolib::cstring_view, melanolib::cstring_view>
        split_command(melanolib::cstring_view message) const;

    /**
     * \brief Parses an info string into a map
     */
    Properties parse_info_string(const std::string& string);

private:

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
     * \brief Whether the given out of band resonse header is a log packet
     */
    virtual bool is_log(melanolib::cstring_view command) const = 0;

    /**
     * \brief Executes the given command using a challenge
     */
    virtual void challenged_command(const std::string& challenge, const std::string& command) = 0;

    /**
     * \brief Whether the given out of band resonse header is a challenge response
     */
    virtual bool is_challenge_response(melanolib::cstring_view command) const;

    /**
     * \brief The command to send in order to obtain a challenge
     */
    virtual std::string challenge_request() const;


    /**
     * \brief Just 'cause darkplaces is weird
     */
    virtual melanolib::cstring_view filter_challenge(melanolib::cstring_view message) const;

    /**
     * \brief Clear connection data
     */
    void clear();

    /**
     * \brief Handles a datagram
     */
    void read(const std::string& datagram);

    /**
     * \brief Handles a challenge recived from the server to be used for challenged rcon
     */
    void handle_challenge(const std::string& challenge);

    /**
     * \brief Asks the server for a challenge
     */
    void request_challenge();

    /**
     * \brief A command to be used for challenges
     */
    struct ChallengedCommand
    {
        std::string     command;        ///< Raw command string
        bool            challenged;     ///< Whether a challenge has been sent
        network::Time   timeout;        ///< Challenge timeout

        ChallengedCommand(std::string command)
            : command(std::move(command)), challenged(false) {}

        void challenge(std::chrono::seconds duration)
        {
            challenged = true;
            timeout = network::Clock::now() + duration;
        }
    };

    mutable std::mutex          mutex;
    std::string                 header = "\xff\xff\xff\xff";    ///< Connection message header
    std::string                 line_buffer;                    ///< Buffer for overflowing messages
    network::Server             rcon_server;
    std::string                 rcon_password;
    network::UdpIo              io;
    std::thread                 thread_input;
    std::list<ChallengedCommand>challenged_buffer;              ///< Buffer for messages to be challenged
    std::chrono::seconds        rcon_challenge_timeout;
};

} // namespace xonotic
#endif // MELANOBOT_MODULE_XONOTIC_ENGINE_HPP
