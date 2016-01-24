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
#include "darkplaces.hpp"

#include <openssl/hmac.h>


static std::string hmac_md4(const std::string& input, const std::string& key)
{
    HMAC_CTX ctx;
    HMAC_CTX_init(&ctx);

    HMAC_Init_ex(&ctx, key.data(), key.size(), EVP_md4(), nullptr);

    HMAC_Update(&ctx, reinterpret_cast<const unsigned char*>(input.data()), input.size());

    unsigned char out[16];
    unsigned int out_size = 0;
    HMAC_Final(&ctx, out, &out_size);

    HMAC_CTX_cleanup(&ctx);

    return std::string(out,out+out_size);
}

namespace xonotic {

Darkplaces::Darkplaces(ConnectionDetails connection_details)
    : connection_details(std::move(connection_details))
{
    io.max_datagram_size(1400);
    io.on_error = [this](const std::string& msg)
    {
        on_network_error(msg);
    };
    io.on_async_receive = [this](const std::string& datagram)
    {
        read(datagram);
    };
    io.on_failure = [this]()
    {
        disconnect();
    };
}

Darkplaces::~Darkplaces()
{
    if ( connected() )
    {
        disconnect();
    }
    else
    {
        // In case the connection failed
        if ( thread_input.joinable() )
            thread_input.join();
    }
}

void Darkplaces::disconnect()
{
    if ( io.connected() )
        on_disconnecting();
    io.disconnect();
    if ( thread_input.joinable() &&
            thread_input.get_id() != std::this_thread::get_id() )
        thread_input.join();
    clear();
    on_disconnect();
}

bool Darkplaces::connect()
{
    if ( !io.connected() )
    {
        if ( io.connect(connection_details.server) )
        {
            if ( thread_input.get_id() == std::this_thread::get_id() )
                return false;

            if ( thread_input.joinable() )
                thread_input.join();

            thread_input = std::move(std::thread([this]{
                clear();
                io.run_input();
            }));

            on_connect();
        }
    }

    return io.connected();
}

bool Darkplaces::reconnect()
{
    disconnect();
    return connect();
}

void Darkplaces::clear()
{
    Lock lock(mutex);
    rcon2_buffer.clear();
}

bool Darkplaces::connected() const
{
    return io.connected();
}


void Darkplaces::sync_read()
{
    read(io.read());
}

void Darkplaces::read(const std::string& datagram)
{
    if ( datagram.size() < 5 || datagram.substr(0,4) != "\xff\xff\xff\xff" )
    {
        on_network_error("Invalid datagram: "+datagram);
        return;
    }

    on_network_input(datagram);

    // non-log/rcon output
    if ( datagram[4] != 'n' )
    {
        auto space = std::find(datagram.begin(), datagram.end(), ' ');
        std::string command(datagram.begin()+4, space);
        std::string message;
        if ( space != datagram.end() )
            message = std::string(space+1, datagram.end());

        if ( command == "challenge" )
            handle_challenge(message.substr(0,11));

        on_receive(command, message);
        return;
    }

    Lock lock(mutex);
    std::istringstream socket_stream(line_buffer+datagram.substr(5));
    line_buffer.clear();
    lock.unlock();

    on_log_begin();

    // convert the datagram into lines
    std::string line;
    while (socket_stream)
    {
        std::getline(socket_stream,line);
        if (socket_stream.eof())
        {
            if (!line.empty())
            {
                lock.lock();
                line_buffer = line;
                lock.unlock();
            }
            break;
        }
        on_receive_log(line);
    }

    on_log_end();
}

void Darkplaces::handle_challenge(const std::string& challenge)
{
    Lock lock(mutex);
    if ( rcon2_buffer.empty() || challenge.empty() ||
        connection_details.rcon_secure != xonotic::ConnectionDetails::CHALLENGE)
        return;

    auto rcon_command = rcon2_buffer.front();
    if ( !rcon_command.challenged ||
        rcon_command.timeout < network::Clock::now() )
    {
        rcon2_buffer.front().challenged = false;
        lock.unlock();
        request_challenge();
        return;
    }

    rcon2_buffer.pop_front();
    bool request_new_challenge = !rcon2_buffer.empty();
    lock.unlock();

    std::string challenge_command = challenge+' '+rcon_command.command;
    std::string key = hmac_md4(challenge_command, connection_details.rcon_password);
    write("srcon HMAC-MD4 CHALLENGE "+key+' '+challenge_command);

    if ( request_new_challenge )
        request_challenge();

}

void Darkplaces::request_challenge()
{
    Lock lock(mutex);
    if ( !rcon2_buffer.empty() && !rcon2_buffer.front().challenged )
    {
        rcon2_buffer.front().challenged = true;
        /// \todo timeout from settings
        rcon2_buffer.front().timeout = network::Clock::now() + std::chrono::seconds(5);
        write("getchallenge");
    }
}

void Darkplaces::set_details(const ConnectionDetails& details)
{
    bool should_reconnect = details.server != connection_details.server;
    connection_details = details;
    if ( should_reconnect )
        reconnect();
}

void Darkplaces::rcon_command(std::string command)
{
    command.erase(std::remove_if(command.begin(), command.end(),
        [](char c){return c == '\n' || c == '\0' || c == '\xff';}),
        command.end());

    if ( connection_details.rcon_secure == xonotic::ConnectionDetails::NO )
    {
        write("rcon "+connection_details.rcon_password+' '+command);
    }
    else if ( connection_details.rcon_secure == xonotic::ConnectionDetails::TIME )
    {
        auto message = std::to_string(std::time(nullptr))+".000000 "+command;
        write("srcon HMAC-MD4 TIME "+
            hmac_md4(message, connection_details.rcon_password)
            +' '+message);
    }
    else if ( connection_details.rcon_secure == xonotic::ConnectionDetails::CHALLENGE )
    {
        Lock lock(mutex);
        rcon2_buffer.push_back(command);
        lock.unlock();
        request_challenge();
    }
}

void Darkplaces::write(std::string line)
{
    io.write(header+line);
}

network::Server Darkplaces::local_endpoint() const
{
    return io.local_endpoint();
}

} // namespace xonotic
