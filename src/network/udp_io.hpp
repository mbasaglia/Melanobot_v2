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
#ifndef UDP_IO_HPP
#define UDP_IO_HPP

#include <boost/asio.hpp>

#include "functional.hpp"
#include "server.hpp"

namespace network {

/**
 * \brief Class providing a simple interface for UDP connections
 */
class UdpIo
{
public:
    /**
     * \brief Called when a network error arises
     */
    std::function<void(const std::string& message)> on_error;
    /**
     * \brief Called when an error will make it impossible to continue
     *  processing the connection.
     * \note Always called after a call to on_error
     */
    std::function<void()> on_failure;
    /**
     * \brief Called after a successful asynchronous read
     */
    std::function<void(const std::string& datagram)> on_async_receive;

    UdpIo() = default;
    UdpIo(const UdpIo&) = delete;
    UdpIo(UdpIo&&) = delete;
    UdpIo& operator=(const UdpIo&) = delete;
    UdpIo& operator=(UdpIo&&) = delete;

    /**
     * \brief Maximum size of a datagram in bytes
     */
    std::string::size_type max_datagram_size() const
    {
        return max_bytes;
    }

    /**
     * \brief Sets the maximum size of a datagram in bytes
     */
    void max_datagram_size(std::string::size_type size)
    {
        max_bytes = size;
    }

    /**
     * \brief Connects to the given server
     * \returns \b true on success
     */
    bool connect(const Server& server)
    {
        try {
            boost::asio::ip::udp::resolver::query query(
                server.host, std::to_string(server.port));
            boost::asio::ip::udp::endpoint endpoint = *resolver.resolve(query);
            socket.connect(endpoint);
            if ( io_service.stopped() )
                io_service.reset();
            return true;
        } catch ( const boost::system::system_error& err ) {
            callback(on_error,err.what());
            callback(on_failure);
            return false;
        }
    }

    /**
     * \brief Disconnects (if connected)
     */
    void disconnect()
    {
        if ( connected() )
        {
            boost::system::error_code ec;
            socket.close(ec);
            if ( ec )
                callback(on_error,ec.message());
            io_service.stop();
        }
    }

    /**
     * \brief Checks if the socket is connected
     */
    bool connected() const
    {
        return socket.is_open();
    }

    /**
     * \brief Synchronous write
     *
     * Writes \c datagram to the udp socket
     * \returns \b true on success
     * \todo maybe truncate to \c max_bytes
     */
    bool write(std::string datagram)
    {
        try {
            socket.send(boost::asio::buffer(datagram));
            return true;
        } catch ( const boost::system::system_error& err ) {
            callback(on_error,err.what());
            return false;
        }
    }

    /**
     * \brief Synchronous read
     *
     * Reads a datagram (max \c max_bytes) and returns it as a string
     */
    std::string read ()
    {
        try {
            std::string datagram;
            datagram.resize(max_bytes);
            auto len = socket.receive(boost::asio::mutable_buffers_1(&datagram[0], max_bytes));
            datagram.resize(len);
            return datagram;
        } catch ( const boost::system::system_error& err ) {
            callback(on_error,err.what());
            return {};
        }
    }

    /**
     * \brief Blocks while connected and handles asynchronous reads
     */
    void run_input()
    {
        schedule_read();
        boost::system::error_code err;
        io_service.run(err);
        if ( err )
        {
            callback(on_error,err.message());
            callback(on_failure);
        }
    }

    /**
     * \brief Endpoint used to send datagram to
     */
    network::Server remote_endpoint() const
    {
        if ( !connected() ) return {};
        boost::system::error_code err;
        auto ep = socket.remote_endpoint(err);
        return err ? network::Server() : network::Server{ep.address().to_string(), ep.port()};
    }

    /**
     * \brief Local connection endpoint
     */
    network::Server local_endpoint() const
    {
        if ( !connected() ) return {};
        boost::system::error_code err;
        auto ep = socket.local_endpoint(err);
        return err ? network::Server() : network::Server{ep.address().to_string(), ep.port()};
    }

private:
    boost::asio::io_service             io_service;             ///< IO service
    boost::asio::ip::udp::resolver      resolver{io_service};   ///< Query resolver
    boost::asio::ip::udp::socket        socket{io_service};     ///< Socket
    std::string::size_type              max_bytes = 1024;       ///< Max size of a datagram
    std::string                         receive_buffer;         ///< Buffer for async reads

    /**
     * \brief Schedules an asyncrhonous read
     */
    void schedule_read()
    {
        receive_buffer.resize(max_bytes);
        socket.async_receive(
            boost::asio::mutable_buffers_1(&receive_buffer[0], max_bytes),
                [this](const boost::system::error_code& error, std::size_t nbytes)
                { return on_receive(error,nbytes); });
    }

    /**
     * \brief Async read callback
     */
    void on_receive(const boost::system::error_code& error, std::size_t nbytes)
    {
        if (error)
        {
            callback(on_error,error.message());
            return;
        }
        callback(on_async_receive,receive_buffer.substr(0,nbytes));
        schedule_read();
    }

};
} // namespace network
#endif // UDP_IO_HPP
