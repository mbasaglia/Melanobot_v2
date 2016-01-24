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

#include "irc/network/buffer.hpp"

#include "irc/network/connection.hpp"

namespace irc {

/**
 * \todo Mimic interface of UdpIo and remove dependency on IrcConnection
 * Maybe it could become TcpIo And have a separate templated? class
 * containing TcpIo or UdpIo which handles flood settings
 */
Buffer::Buffer(IrcConnection& irc, const Settings& settings)
    : irc(irc)
{
    flood_timer = network::Clock::now();
    flood_timer_max = std::chrono::seconds(settings.get("timer_max",10));
    flood_message_penalty = std::chrono::seconds(settings.get("message_penalty",2));
    flood_bytes_penalty = settings.get("bytes_penalty",120);
    flood_max_length = settings.get("max_length",510);
}
Buffer::~Buffer()
{
    stop();
}

void Buffer::run_output()
{
    buffer.start();
    while (buffer.active())
        process();
}
void Buffer::run_input()
{
    schedule_read();
    boost::system::error_code err;
    io_service.run(err);
    if ( err )
    {
        ErrorLog("irc","Network Error") << err.message();
        irc.error_stop();
    }
}

void Buffer::start()
{
    buffer.start();
    if ( io_service.stopped() )
        io_service.reset();
    if ( !thread_output.joinable() )
        thread_output = std::move(std::thread([this]{run_output();}));
    if ( !thread_input.joinable() )
        thread_input = std::move(std::thread([this]{run_input();}));
}

void Buffer::stop()
{
    disconnect();
    io_service.stop();
    buffer.stop();
    if ( thread_input.joinable() )
        thread_input.join();
    if ( thread_output.joinable() )
        thread_output.join();
}

void Buffer::insert(const network::Command& cmd)
{
    buffer.push(cmd);
}

void Buffer::process()
{
    network::Command cmd;

    do
    {
        buffer.pop(cmd);
        if ( !buffer.active() )
            return;

        auto max_timer = network::Clock::now() + flood_timer_max;
        if ( flood_timer+flood_message_penalty > max_timer )
        {
            std::this_thread::sleep_for(std::max(flood_message_penalty,flood_timer-max_timer));
        }
    }
    while ( cmd.timeout < network::Clock::now() );

    write(cmd);
}

void Buffer::write(const network::Command& cmd)
{
    std::string line = cmd.command;
    for ( auto it = cmd.parameters.begin(); it != cmd.parameters.end(); ++it )
    {
        if ( it == cmd.parameters.end()-1 && it->find(' ') != std::string::npos )
            line += " :";
        else
            line += ' ';
        line += *it;
    }
    write_line(line);
}

void Buffer::write_line ( std::string line )
{
    line.erase(std::remove_if(line.begin(), line.end(),
        [](char c){return c == '\n' || c == '\r' || c == '\0';}),
        line.end());

    std::ostream request_stream(&buffer_write);
    if ( line.size() > flood_max_length )
    {
        Log("irc",'!',4) << "Truncating " << irc.decode(line);
        line.erase(flood_max_length-1);
    }
    request_stream << line << "\r\n";
    Log("irc",'<',1) << irc.decode(line);

    boost::system::error_code error;
    boost::asio::write(socket, buffer_write, error);
    if (error && error != boost::asio::error::eof )
    {
        ErrorLog("irc","Network Error") << error.message();
    }

    flood_timer = std::max(flood_timer,network::Clock::now());
    flood_timer += flood_message_penalty;
    if ( flood_bytes_penalty > 0 )
        flood_timer += std::chrono::seconds(line.size()/flood_bytes_penalty);
}

bool Buffer::connect(const network::Server& server)
{
    if ( connected() )
        disconnect();
    try {
        buffer.start();
        boost::asio::ip::tcp::resolver::query query(server.host, std::to_string(server.port));
        boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
        boost::asio::connect(socket, endpoint_iterator);
        flood_timer = network::Clock::now();
    } catch ( const boost::system::system_error& err ) {
        ErrorLog("irc","Network Error") << err.what();
        irc.error_stop();
        return false;
    }
    return true;
}

void Buffer::disconnect()
{
    if ( connected() )
    {
        try {
            socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
            socket.close();
        } catch ( const boost::system::system_error& err ) {
            ErrorLog("irc","Network Error") << err.what();
        }
    }
}

bool Buffer::connected() const
{
    return socket.is_open();
}

void Buffer::schedule_read()
{
    boost::asio::async_read_until(socket, buffer_read, "\r\n",
        [this](const boost::system::error_code& error, std::size_t)
        { return on_read_line(error); });
}

void Buffer::on_read_line(const boost::system::error_code &error)
{
    if (error)
    {
        if ( error != boost::asio::error::eof )
             ErrorLog("irc","Network Error") << error.message();
        return;
    }

    std::istream buffer_stream(&buffer_read);
    std::string line;
    std::getline(buffer_stream,line,'\r');
    Log("irc",'>',1) << irc.decode(line);
    buffer_stream.ignore(2,'\n');

    irc.handle_message(parse_line(line));

    schedule_read();
}


network::Message Buffer::parse_line(const std::string& line) const
{
    network::Message msg;

    msg.raw = line;
    std::istringstream socket_stream(msg.raw);

    if ( socket_stream.peek() == ':' )
    {
        socket_stream.ignore();
        socket_stream >> msg.from.name;
    }
    socket_stream >> msg.command;

    char c;
    std::string param;
    while ( socket_stream >> c )
    {
        if ( c == ':' )
        {
            std::getline (socket_stream, param);
            msg.params.push_back(param);
            break;
        }
        else
        {
            socket_stream.putback(c);
            socket_stream >> param;
            msg.params.push_back(param);
        }
    }

    return msg;
}

void Buffer::clear(int priority)
{
    buffer.remove_if([priority](const network::Command& cmd){
        return cmd.priority <= priority;
    });
}

} // namespace irc
