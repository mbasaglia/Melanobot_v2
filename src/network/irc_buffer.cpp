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

#include "irc_buffer.hpp"

#include "irc_connection.hpp"

namespace network {
namespace irc {

Buffer::Buffer(IrcConnection& irc, const Settings& settings)
    : irc(irc)
{
    flood_timer = Clock::now();
    flood_timer_max = std::chrono::seconds(settings.get("timer_max",10));
    flood_timer_penalty = std::chrono::seconds(settings.get("timer_penalty",2));
    flood_max_length = settings.get("max_length",510);
}

void Buffer::run_output()
{
    buffer.start();
    while (buffer.active())
        process();
}
void Buffer::run_input()
{

    /// \todo catch exception thrown on broken pipe
    schedule_read();
    io_service.run();
}

void Buffer::start()
{
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

void Buffer::insert(const Command& cmd)
{
    buffer.push(cmd);
}

void Buffer::process()
{
    Command cmd;

    do
    {
        buffer.pop(cmd);
        if ( !buffer.active() )
            return;

        auto max_timer = Clock::now() + flood_timer_max;
        if ( flood_timer+flood_timer_penalty > max_timer )
            std::this_thread::sleep_for(max_timer-flood_timer);
    }
    while ( cmd.timeout < Clock::now() );

    write(cmd);
}

void Buffer::write(const Command& cmd)
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
    /// \note this is synchronous, if it becomes async, keep QUIT as a sync message before disconnect()
    /// \todo maybe try to strip \r and \n from line
    std::ostream request_stream(&buffer_write);
    if ( line.size() > flood_max_length )
    {
        Log("irc",'!',4) << "Truncating " << irc.formatter()->decode(line);
        line.erase(flood_max_length-1);
    }
    request_stream << line << "\r\n";
    Log("irc",'<',1) << irc.formatter()->decode(line);
    boost::asio::write(socket, buffer_write);
    flood_timer += flood_timer_penalty;
}

void Buffer::connect(const Server& server)
{
    if ( connected() )
        disconnect();
    boost::asio::ip::tcp::resolver::query query(server.host, std::to_string(server.port));
    boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
    boost::asio::connect(socket, endpoint_iterator);
    flood_timer = Clock::now();
    /// \todo async with error checking
}

void Buffer::disconnect()
{
    /// \todo try catch
    if ( connected() )
    {
        socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
        socket.close();
    }
}

bool Buffer::connected() const
{
    return socket.is_open();
}

void Buffer::schedule_read()
{
    boost::asio::async_read_until(socket, buffer_read, "\r\n",
        std::bind(&Buffer::on_read_line, this, std::placeholders::_1));
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
    Message msg;
    std::getline(buffer_stream,msg.raw,'\r');
    Log("irc",'>',1) << irc.formatter()->decode(msg.raw);
    buffer_stream.ignore(2,'\n');
    std::istringstream socket_stream(msg.raw);

    if ( socket_stream.peek() == ':' )
    {
        socket_stream.ignore();
        socket_stream >> msg.from;
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

    msg.source = &irc;
    irc.handle_message(std::move(msg));

    schedule_read();
}


} // namespace irc
} // namespace network
