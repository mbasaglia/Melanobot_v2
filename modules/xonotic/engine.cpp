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

#include "engine.hpp"
#include "melanolib/string/quickstream.hpp"
#include "melanolib/utils/type_utils.hpp"

namespace xonotic {

Engine::Engine(network::Server server, std::string rcon_password,
               std::size_t max_datagram_size)
    : rcon_server(std::move(server)),
      rcon_password(std::move(rcon_password)),
      rcon_challenge_timeout(5)
{

    io.max_datagram_size(max_datagram_size);
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


Engine::~Engine()
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

void Engine::set_server(const network::Server& server)
{
    auto lock = make_lock(mutex);
    if ( server != rcon_server )
    {
        rcon_server = server;
        reconnect();
    }
}

network::Server Engine::server() const
{
    auto lock = make_lock(mutex);
    return rcon_server;
}

std::chrono::seconds Engine::challenge_timeout() const
{
    auto lock = make_lock(mutex);
    return rcon_challenge_timeout;
}

void Engine::set_challenge_timeout(std::chrono::seconds seconds)
{
    auto lock = make_lock(mutex);
    rcon_challenge_timeout = seconds;
    /// \todo could clear up the existing command queue
}

std::string Engine::password() const
{
    auto lock = make_lock(mutex);
    return rcon_password;
}

void Engine::set_password(const std::string& password)
{
    auto lock = make_lock(mutex);
    rcon_password = password;
}

void Engine::disconnect()
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

bool Engine::connect()
{
    if ( !io.connected() )
    {
        if ( io.connect(rcon_server) )
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

bool Engine::reconnect()
{
    disconnect();
    return connect();
}

void Engine::clear()
{
    auto lock = make_lock(mutex);
    challenged_buffer.clear();
}

bool Engine::connected() const
{
    return io.connected();
}


network::Server Engine::local_endpoint() const
{
    return io.local_endpoint();
}

void Engine::sync_read()
{
    read(io.read());
}

std::pair<melanolib::cstring_view, melanolib::cstring_view>
    Engine::split_command(melanolib::cstring_view message) const
{
    auto it = std::find_if(message.begin(), message.end(),
        melanolib::FunctionPointer<int (int)>(std::isspace));
    return {
        {message.begin(), it},
        {melanolib::math::min(it+1, message.end()), message.end()}
    };
}

bool Engine::is_challenge_response(melanolib::cstring_view command) const
{
    return command == "challenge";
}

melanolib::cstring_view Engine::filter_challenge(melanolib::cstring_view message) const
{
    return message;
}


void Engine::read(const std::string& datagram)
{
    if ( datagram.size() < header.size()+1 ||
         datagram.substr(0, header.size()) != header )
    {
        on_network_error("Invalid datagram: "+datagram);
        return;
    }

    on_network_input(datagram);

    melanolib::cstring_view command, message;
    std::tie(command, message) = split_command({
        datagram.data()+header.size(),
        datagram.data()+datagram.size()
    });

    if ( !is_log(command) )
    {

        if ( is_challenge_response(command) )
            handle_challenge(filter_challenge(message).str());
        else
            on_receive(command.str(), message.str());
        return;
    }

    auto lock = make_lock(mutex);
    melanolib::string::QuickStream socket_stream(line_buffer + message.str());
    line_buffer.clear();
    lock.unlock();

    on_log_begin();

    // convert the datagram into lines
    while (!socket_stream.eof())
    {
        auto line = socket_stream.get_line();
        if ( socket_stream.eof() && socket_stream.peek_back() != '\n' )
        {
            lock.lock();
            line_buffer = line;
            lock.unlock();
            break;
        }
        on_receive_log(line);
    }

    on_log_end();
}

void Engine::handle_challenge(const std::string& challenge)
{
    auto lock = make_lock(mutex);
    if ( challenged_buffer.empty() || challenge.empty() )
        return;

    auto rcon_command = challenged_buffer.front();
    if ( !rcon_command.challenged ||
        rcon_command.timeout < network::Clock::now() )
    {
        challenged_buffer.front().challenged = false;
        lock.unlock();
        request_challenge();
        return;
    }

    challenged_buffer.pop_front();
    bool request_new_challenge = !challenged_buffer.empty();
    lock.unlock();

    challenged_command(challenge, rcon_command.command);

    if ( request_new_challenge )
        request_challenge();
}

void Engine::request_challenge()
{
    auto lock = make_lock(mutex);
    if ( !challenged_buffer.empty() && !challenged_buffer.front().challenged )
    {
        challenged_buffer.front().challenge(rcon_challenge_timeout);
        write(challenge_request());
    }
}

std::string Engine::challenge_request() const
{
    return "getchallenge";
}

void Engine::schedule_challenged_command(const std::string& command)
{
    auto lock = make_lock(mutex);
    challenged_buffer.push_back(command);
    lock.unlock();
    request_challenge();
}

void Engine::write(std::string line)
{
    io.write(header+line);
}

Properties Engine::parse_info_string(const std::string& string)
{
    static const char INFO_SEPARATOR = '\\';
    Properties map;

    if ( string.empty() )
    {
        return map;
    }

    melanolib::string::QuickStream input(string);
    while ( !input.eof() )
    {
        input.ignore(1, INFO_SEPARATOR);
        std::string key = input.get_line(INFO_SEPARATOR);
        std::string value = input.get_line(INFO_SEPARATOR);
        if ( !key.empty() )
        {
            map[key] = value;
        }
    }

    return map;
}

} // namespace xonotic
