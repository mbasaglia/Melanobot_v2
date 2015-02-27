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

#include "irc.hpp"
#include "../string/logger.hpp"

#define LOCK(mutexname) std::lock_guard<std::mutex> lock_(mutexname)

namespace network {
namespace irc {

Buffer::Buffer(IrcConnection& irc, const settings::Settings& settings)
    : irc(irc)
{
    flood_timer = Clock::now();
    flood_timer_max = std::chrono::seconds(settings.get("flood.timer_max",10));
    flood_timer_penalty = std::chrono::seconds(settings.get("flood.timer_penalty",2));
    flood_max_length = settings.get("flood.max_length",510);
}

void Buffer::run()
{
    /// \todo wait if the io_service isn't running from the start and handle reconnects
    schedule_read();
    /// \todo execute process() in parallel to io_service.run()
    io_service.run();
}

void Buffer::insert(const Command& cmd)
{
    LOCK(mutex);
    buffer.push(cmd);

    /// \todo remove from here (needs to be async with flood conditions
    write(cmd);
}

bool Buffer::empty() const
{
    LOCK(mutex);
    return buffer.empty();
}

void Buffer::process()
{
    mutex.lock();
        auto now = Clock::now();
        while ( !buffer.empty() && buffer.top().timeout < now )
            buffer.pop();
        if ( buffer.empty() )
            return;
        Command cmd = buffer.top();
        buffer.pop();
    mutex.unlock();
    write(cmd);
}

void Buffer::clear()
{
    LOCK(mutex);
    buffer = std::priority_queue<Command>();
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

void Buffer::connect(const Server& server)
{
    if ( connected() )
        disconnect();
    boost::asio::ip::tcp::resolver::query query(server.host, std::to_string(server.port));
    boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
    boost::asio::connect(socket, endpoint_iterator);
    /// \todo async with error checking + cycle (ie: notify irc that connection failed and needs a reconnect)
}

void Buffer::disconnect()
{
    /// \todo try catch
    socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
    socket.close();
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

void Buffer::write_line ( std::string line )
{
    /// \note this is synchronous, if it becomes async, keep QUIT as a sync message before disconnect()
    /// \todo maybe try to trip \r and \n from line
    std::ostream request_stream(&buffer_write);
    if ( line.size() > flood_max_length )
    {
        Log("irc",'!',4) << "Truncating " << irc.formatter()->decode(line);
        line.erase(flood_max_length-1);
    }
    request_stream << line << "\r\n";
    Log("irc",'<',1) << irc.formatter()->decode(line);
    boost::asio::write(socket, buffer_write);
}


void Buffer::on_read_line(const boost::system::error_code &error)
{
    if (error)
    {
        if ( error != boost::asio::error::eof )
            Log("irc",'!',0) << color::red << "Network Error" << color::nocolor
                << ':' << error.message();
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

    irc.handle_message(msg);

    schedule_read();
}

REGISTER_CONNECTION(irc,&IrcConnection::create);

IrcConnection* IrcConnection::create(Melanobot* bot, const settings::Settings& settings)
{
    if ( settings.get("protocol",std::string()) != "irc" )
    {
        error("Wrong protocol for IRC connection");
        return nullptr;
    }

    Network::ServerList network;

    Server server ( settings.get("server",std::string()) );
    if ( !server.port )
        server.port = 6667;
    server.host = settings.get("server.host",server.host);
    server.port = settings.get("server.port",server.port);
    if ( !server.host.empty() && server.port )
        network.push_back(server);

    /// \todo handle named networks (insert at the end of \c network)

    if ( network.empty() )
    {
        error("IRC connection with no server");
        return nullptr;
    }

    return new IrcConnection(bot, Network(network), settings);
}

IrcConnection::IrcConnection ( Melanobot* bot, const Network& network, const settings::Settings& settings )
    : bot(bot), network(network), buffer(*this, settings.get_child("buffer",{}))
{
    network_password = settings.get("server.password",std::string());
    preferred_nick   = settings.get("nick","PleaseNameMe");
    auth_nick        = settings.get("auth.nick",preferred_nick);
    auth_password    = settings.get("auth.password",std::string());
    modes            = settings.get("modes",std::string());
    formatter_ = string::Formatter::formatter(settings.get("string.format",std::string("irc")));
}
IrcConnection::IrcConnection ( Melanobot* bot, const Server& server, const settings::Settings& settings )
    : IrcConnection(bot,Network{server},settings)
{}

IrcConnection::~IrcConnection()
{
    buffer.disconnect();
}

void IrcConnection::run()
{
    connect();
    buffer.run();
}

void IrcConnection::error ( const std::string& message )
{
    Log("irc",'!',0) << color::red << "Error" << color::nocolor << ": " << message;
}

const Server& IrcConnection::server() const
{
    LOCK(mutex);
    return network.current();
}

void IrcConnection::command ( const Command& c )
{
    Command cmd = c;
    std::string command = strtoupper(cmd.command);

    /// \todo Ugly if chain
    if ( command == "PRIVMSG" || command == "NOTICE" )
    {
        if ( cmd.parameters.size() != 2 )
            return error("Wrong parameters for PRIVMSG");
        std::string to = strtolower(cmd.parameters[0]);
        {
            LOCK(mutex);
            if ( to == current_nick_lowecase )
                return error("Cannot send PRIVMSG to self");
        }
        if ( cmd.parameters[1].empty() )
            return error("Empty PRIVMSG");

        cmd.parameters[0] = to;
    }
    else if ( command == "PASS" )
    {
        if ( status() != Connection::WAITING )
            return error("PASS called at a wrong time");
        if ( cmd.parameters.size() != 1 )
            return error("Ill-formed PASS");
    }
    else if ( command == "NICK" )
    {
        if ( cmd.parameters.size() != 1 )
            return error("Ill-formed NICK");
        /// \todo validate nick
        {
            LOCK(mutex);
            attempted_nick = strtolower(cmd.parameters[0]);
            if ( attempted_nick == current_nick_lowecase )
            {
                attempted_nick = "";
                current_nick = cmd.parameters[0];
                return;
            }
        }
    }
    else if ( command == "USER" )
    {
        if ( cmd.parameters.size() != 4 )
            return error("Ill-formed USER");
    }
    else if ( command == "MODE" )
    {
        LOCK(mutex);
        if ( cmd.parameters.size() == 1 )
        {
            cmd.parameters.resize(2);
            cmd.parameters[1] = cmd.parameters[0];
            cmd.parameters[0] = current_nick;
        }
        else if ( cmd.parameters.size() != 2 || strtolower(cmd.parameters[0]) == current_nick_lowecase )
            return error("Ill-formed MODE");
        /// \todo sanitaze the mode string
    }
    else if ( command == "JOIN" )
    {
        if ( cmd.parameters.size() < 1 )
            return error("Ill-formed JOIN");
        /// \todo Discard already joined channels,
        /// keep track of too many channels,
        /// check that channel names are ok
        /// http://tools.ietf.org/html/rfc2812#section-3.2.1
    }
    else if ( command == "PART" )
    {
        if ( cmd.parameters.size() < 1 )
            return error("Ill-formed PART");
        /// \todo Discard unexisting channels
        /// http://tools.ietf.org/html/rfc2812#section-3.2.2
    }
    /**
     * \todo custom commands: RECONNECT
     */
    /* Skipped: * = maybe later o = maybe disallow completely
     * OPER     o
     * SERVICE  o
     * QUIT     *
     * SQUIT    o
     * MODE     *
     * TOPIC    *
     * NAMES    *
     * LIST     o
     * INVITE   *
     * KICK     *
     * MOTD     o
     * LUSERS
     * VERSION
     * STATS
     * LINKS
     * TIME
     * CONNECT  o
     * TRACE    o
     * ADMIN    o
     * INFO
     * SERVLIST
     * SQUERY
     * WHO
     * WHOIS    *
     * WHOWAS
     * KILL     o
     * PING     *
     * PONG     *
     * ERROR    o
     * AWAY     *
     * REHASH   o
     * DIE      o
     * RESTART  o
     * SUMMON   o
     * USERS
     * WALLOPS
     * USERHOST
     * ISON     *
     */

    buffer.insert(cmd);
}

void IrcConnection::say ( const std::string& channel, const std::string& message,
    int priority, const Time& timeout )
{
    command({"PRIVMSG", {channel, message}, priority, timeout});
}

void IrcConnection::say_as ( const std::string& channel,
    const std::string& name,
    const std::string& message,
    int priority,
    const Time& timeout )
{
    /// \todo name thing as a possibly colored string
    command({"PRIVMSG", {channel, '<'+name+'>'+message}, priority, timeout});
}

Connection::Status IrcConnection::status() const
{
    /// \todo
    return Connection::DISCONNECTED;
}

std::string IrcConnection::protocol() const
{
    return "irc";
}

std::string IrcConnection::connection_name() const
{
    /// \todo this should be set at construction and never changed
    /// => no lock
    return "todo";
}


void IrcConnection::connect()
{
    if ( !buffer.connected() )
    {
        buffer.connect(network.current());
        /// \todo cycle servers until finds a connection, or has cycled through too many servers
        /// then trigger a graceful quit
        login();
    }
}

void IrcConnection::disconnect()
{
    if ( status() > CONNECTING )
        buffer.write({"QUIT",{},1024});
    buffer.disconnect();
}

string::Formatter* IrcConnection::formatter()
{
    return formatter_;
}

void IrcConnection::reconnect()
{
    disconnect();
    network.next();
}

void IrcConnection::handle_message(const Message& msg)
{
    /// \todo append Message to Melanobot

    // see http://tools.ietf.org/html/rfc2812#section-5.1

    if ( msg.command == "001" && msg.params.size() >= 1 )
    {
        // RPL_WELCOME
        mutex.lock();
        current_nick = msg.params[0];
        current_nick_lowecase = strtolower(current_nick);
        mutex.unlock();
        auth();
    }
    else if ( msg.command == "433" && msg.params.size() >= 1 )
    {
        // ERR_NICKNAMEINUSE
        mutex.lock();
        if ( strtolower(attempted_nick) == strtolower(msg.params[0]) )
        {
            Log("irc",'!',4) << attempted_nick << " is taken, trying a new nick";
            /// \todo check nick max length
            /// \todo system to try to get the best nick possible
            Command cmd {{"NICK"},{attempted_nick+'_'},1024};
            mutex.unlock();
            command(cmd);
        }
        else
            mutex.unlock();
    }
    else if ( msg.command == "464" || msg.command == "465" || msg.command == "466" )
    {
        // banned from server
        reconnect();
    }
    else if ( msg.command == "PING" )
    {
        /// \todo read timeout in settings
        /// \todo set timer to the latest server message and call PING when too old
        command({"PONG",msg.params,1024,std::chrono::minutes(3)});
    }

    /// \todo handle non-numeric commands
    /// \todo print a message for all ERR_ responses (verbosity > 2)
    /* Skipped:
     * 0xx server-client
     * 002
     * 003
     * 004      *
     * 005      *
     * 2xx-3xx Replies to commands
     * 302
     * 303
     * 301
     * 305
     * 306
     * 311      *
     * 312
     * 313
     * 317
     * 318
     * 319      *
     * 314      ?
     * 369
     * 321
     * 322      ?
     * 323
     * 325
     * 324      ?
     * 331      ?
     * 332      ?
     * 341
     * 342
     * 346
     * 347
     * 348
     * 349
     * 351      ?
     * 352
     * 315
     * 353      *
     * 366      ?
     * 364
     * 365
     * 367      *
     * 368      ?
     * 371
     * 374
     * 372
     * 376
     * 381
     * 382
     * 383
     * 391      ?
     * 393
     * 394
     * 305
     * 200-210
     * 262
     * 211
     * 212
     * 219
     * 242
     * 243
     * 221      ?
     * 234      ?
     * 235      ?
     * 251-255
     * 256-259
     * 263      ?
     * 4xx errors
     * 401
     * 402
     * 403
     * 404      ?
     * 405      *
     * 406
     * 407      ?
     * 408
     * 409
     * 411
     * 412-415  ?
     * 421
     * 422
     * 423
     * 424
     * 431
     * 432
     * 436      ?
     * 437
     * 441
     * 442
     * 443
     * 444
     * 445
     * 446      ?
     * 451      *
     * 461
     * 462
     * 463
     * 464      *
     * 465
     * 467
     * 472
     * 473      *
     * 474      *
     * 475
     * 476
     * 477
     * 478      *
     * 481
     * 482
     * 483
     * 484
     * 485
     * 491
     * 501
     * 502
     */
}

void IrcConnection::login()
{
    if ( !network_password.empty() )
        command({"PASS",{network_password},1024});
    command({"NICK",{preferred_nick},1024});
    command({"USER",{preferred_nick, "0", preferred_nick, preferred_nick},1024});
}

void IrcConnection::auth()
{
    if ( !auth_password.empty() )
        command({"AUTH",{auth_nick, auth_password},1024});
    if ( !modes.empty() )
        command({"MODE",{current_nick, modes},1024});

}

} // namespace irc
} // namespace network
