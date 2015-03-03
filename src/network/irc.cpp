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

REGISTER_CONNECTION(irc,&network::irc::IrcConnection::create);
REGISTER_LOG_TYPE(irc,color::dark_magenta);

namespace network {
namespace irc {

/// \note the real regex would be stricter
std::regex UserNick::prefix_regex{":?([^!@]+)(?:!([^@]+))?(?:@(.*))",
        std::regex_constants::syntax_option_type::optimize |
        std::regex_constants::syntax_option_type::ECMAScript };

UserNick::UserNick ( const std::string& prefix )
{
    std::smatch match;

    nick = prefix;
    if ( std::regex_match(prefix,match,prefix_regex) )
    {
        nick = match[1].str();
        user = match[2].str();
        host = match[3].str();
    }
}

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
    if ( !thread_input.joinable() )
        thread_input.join();
    if ( !thread_output.joinable() )
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

IrcConnection* IrcConnection::create(Melanobot* bot, const Settings& settings)
{
    if ( settings.get("protocol",std::string()) != "irc" )
    {
        ErrorLog("irc") << "Wrong protocol for IRC connection";
        return nullptr;
    }

    Server server ( settings.get("server",std::string()) );
    if ( !server.port )
        server.port = 6667;
    server.host = settings.get("server.host",server.host);
    server.port = settings.get("server.port",server.port);
    if ( server.host.empty() || !server.port )
    {
        ErrorLog("irc") << "IRC connection with no server";
        return nullptr;
    }

    return new IrcConnection(bot, server, settings);
}

IrcConnection::IrcConnection ( Melanobot* bot, const Server& server, const Settings& settings )
    : bot(bot), main_server(server), current_server(server), buffer(*this, settings.get_child("buffer",{}))
{
    read_settings(settings);
}

IrcConnection::~IrcConnection()
{
    buffer.stop();
}

void IrcConnection::read_settings(const Settings& settings)
{
    /// \note doesn't read buffer settings
    /// \todo if called from somewhere that is not the constructor, lock data
    server_password  = settings.get("server.password",std::string());
    main_server.host = settings.get("server.host",main_server.host);
    main_server.port = settings.get("server.port",main_server.port);

    preferred_nick   = settings.get("nick","PleaseNameMe");
    modes            = settings.get("modes",std::string());

    auth_nick        = settings.get("auth.nick",preferred_nick);
    auth_password    = settings.get("auth.password",std::string());

    formatter_ = string::Formatter::formatter(settings.get("string_format",std::string("irc")));
    connection_status = DISCONNECTED;

    std::istringstream ss ( settings.get("channels",std::string()) );
    std::string chan;
    while ( ss >> chan )
        channels_to_join.push_back(chan);
}


void IrcConnection::start()
{
    connect();
    buffer.start();
}

void IrcConnection::stop()
{
    disconnect();
    buffer.stop();
}

const Server& IrcConnection::server() const
{
    LOCK(mutex);
    return current_server;
}

void IrcConnection::command ( const Command& c )
{
    Command cmd = c;
    std::string command = strtoupper(cmd.command);

    if ( command == "PRIVMSG" || command == "NOTICE" )
    {
        if ( cmd.parameters.size() != 2 )
        {
            ErrorLog("irc") << "Wrong parameters for PRIVMSG";
            return;
        }
        std::string to = strtolower(cmd.parameters[0]);
        {
            LOCK(mutex);
            if ( to == current_nick_lowecase )
            {
                ErrorLog("irc") << "Cannot send PRIVMSG to self";
                return;
            }
        }
        if ( cmd.parameters[1].empty() )
        {
            ErrorLog("irc") << "Empty PRIVMSG";
            return;
        }

        cmd.parameters[0] = to;
    }
    else if ( command == "PASS" )
    {
        if ( status() != Connection::WAITING )
        {
            ErrorLog("irc") << "PASS called at a wrong time";
            return;
        }
        if ( cmd.parameters.size() != 1 )
        {
            ErrorLog("irc") << "Ill-formed PASS";
            return;
        }
    }
    else if ( command == "NICK" )
    {
        if ( cmd.parameters.size() != 1 )
        {
            ErrorLog("irc") << "Ill-formed NICK";
            return;
        }
        /// \todo validate nick
        {
            LOCK(mutex);
            attempted_nick = cmd.parameters[0];
            if ( strtolower(attempted_nick) == current_nick_lowecase )
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
        {
            ErrorLog("irc") << "Ill-formed USER";
            return;
        }
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
        else if ( cmd.parameters.size() != 2 ||
            strtolower(cmd.parameters[0]) == current_nick_lowecase )
        {
            ErrorLog("irc") << "Ill-formed MODE";
            return;
        }
        /// \todo sanitaze the mode string
        /// \todo allow channel mode
    }
    else if ( command == "JOIN" )
    {
        if ( cmd.parameters.size() < 1 )
        {
            ErrorLog("irc") << "Ill-formed JOIN";
            return;
        }
        /// \todo Discard already joined channels,
        /// keep track of too many channels,
        /// check that channel names are ok
        /// http://tools.ietf.org/html/rfc2812#section-3.2.1
    }
    else if ( command == "PART" )
    {
        if ( cmd.parameters.size() < 1 )
        {
            ErrorLog("irc") << "Ill-formed PART";
            return;
        }
        /// \todo Discard unexisting channels
        /// http://tools.ietf.org/html/rfc2812#section-3.2.2
    }
    else if ( command == "RECONNECT" )
    {
        // Custom command
        reconnect();
        return;
    }
    else if ( command == "DISCONNECT" )
    {
        // Custom command
        disconnect();
        return;
    }
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


void IrcConnection::say ( const std::string& channel,
        const string::FormattedString& message,
        int priority,
        const Time& timeout )
{
    command({"PRIVMSG", {channel, message.encode(formatter_)}, priority, timeout});
}

void IrcConnection::say_as ( const std::string& channel,
        const string::FormattedString& name,
        const string::FormattedString& message,
        const string::FormattedString& prefix,
        int priority,
        const Time& timeout )
{
    string::FormattedString msg (
        string::FormattedStream() << prefix << " <" << name << "> " << message
    );

    command({"PRIVMSG", {channel, msg.encode(formatter_)}, priority, timeout});
}

Connection::Status IrcConnection::status() const
{
    return connection_status;
}

std::string IrcConnection::protocol() const
{
    return "irc";
}

void IrcConnection::connect()
{
    if ( !buffer.connected() )
    {
        connection_status = WAITING;
        buffer.connect(main_server);
        mutex.lock();
        current_server = main_server;
        mutex.unlock();
        /// \todo If connection fails, trigger a graceful quit
        connection_status = CONNECTING;
        login();
    }
}

void IrcConnection::disconnect()
{
    if ( status() > CONNECTING )
        buffer.write({"QUIT",{},1024});
    buffer.disconnect();
    connection_status = DISCONNECTED;
}

string::Formatter* IrcConnection::formatter()
{
    return formatter_;
}

void IrcConnection::reconnect()
{
    disconnect();
    connect();
}

void IrcConnection::handle_message(Message msg)
{
    if ( msg.command == "001" && msg.params.size() >= 1 )
    {
        // RPL_WELCOME
        mutex.lock();
        /// \todo read current_server
        current_nick = msg.params[0];
        current_nick_lowecase = strtolower(current_nick);
        for ( const auto& s : channels_to_join )
            command({"JOIN",{s}});
        channels_to_join.clear();
        mutex.unlock();
        auth();
        connection_status = CONNECTED;
    }
    else if ( msg.command == "433" && msg.params.size() >= 2 )
    {
        // ERR_NICKNAMEINUSE
        mutex.lock();
        if ( strtolower(attempted_nick) == strtolower(msg.params[1]) )
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
    else if ( msg.command == "PRIVMSG" )
    {
        if ( msg.params.size() != 2 || msg.params[1].empty() )
            return; // Odd PRIVMSG format

        {
            LOCK(mutex);
            if ( strtolower(msg.from) == current_nick_lowecase )
                return; // received our own message for some reason, disregard
        }

        std::string message = msg.params[1];

        std::string nickfrom = UserNick(msg.from).nick;
        msg.from = nickfrom;
        msg.message = message;

        if ( strtolower(msg.params[0]) == current_nick_lowecase )
        {
            msg.channels = { strtolower(nickfrom) };
            msg.direct = 1;
        }
        else
        {
            msg.channels = { msg.params[0] };
        }

        // Handle CTCP
        if ( msg.message[0] == '\1' )
        {
            static std::regex regex_ctcp("\1([^ \1]+)(?: ([^\1]+))?\1",
                std::regex_constants::syntax_option_type::optimize |
                std::regex_constants::syntax_option_type::ECMAScript);
            std::smatch match;
            std::regex_match(message,match,regex_ctcp);
            msg.message.clear();
            std::string ctcp = strtoupper(match[1].str());

            if ( ctcp == "ACTION" )
            {
                msg.action = 1;
            }
            else
            {
                msg.command = "CTCP";
                msg.params = { ctcp };
                if ( !match[2].str().empty() )
                    msg.params.push_back(match[2].str());
            }
        }
        else
        {
            /// \todo escape []\^{|} on current_nick
            std::regex regex_direct(current_nick+":\\s*(.*)");
            std::smatch match;
            if ( std::regex_match(message,match,regex_direct) )
            {
                msg.direct = true;
                msg.message = match[1];
            }
        }
    }
    else if ( msg.command == "NOTICE" )
    {
        // http://tools.ietf.org/html/rfc2812#section-3.3.2
        // Discard because you should never send automatic replies
        return;
    }
    else if ( msg.command == "ERROR" )
    {
        ErrorLog errl("irc","Server Error:");
        if ( msg.params.empty() )
            errl << "Unknown error";
        else
            errl << msg.params[0];
        bot->stop(); /// \todo is this the right thing to do?
    }
    // see http://tools.ietf.org/html/rfc2812#section-3 (Messages)
    /* Skipped: * = maybe later X = surely later
     * PASS
     * USER
     * OPER
     * SERVICE
     * QUIT     X
     * SQUIT
     * JOIN     X
     * PART     X
     * MODE     *
     * TOPIC    *
     * NAMES    X
     * LIST
     * INVITE   X
     * KICK     X
     * MOTD
     * LUSERS
     * VERSION
     * STATS
     * LINKS
     * TIME
     * CONNECT
     * TRACE
     * ADMIN
     * INFO
     * SERVLIST
     * SQUERY
     * WHO
     * WHOIS
     * WHOWAS
     * KILL
     * PONG     *
     * AWAY     *
     * REHASH
     * DIE
     * RESTART
     * SUMMON
     * USERS    *
     * WALLOPS
     * USERHOST
     * ISON
     */
    // see http://tools.ietf.org/html/rfc2812#section-5.1 (Numeric)
    /// \todo print a message for all ERR_ responses (verbosity > 2)
    /* Skipped:
     * 0xx server-client
     * 002
     * 003
     * 004      *
     * 005      *
     * 2xx-3xx Replies to commands
     * 302
     * 303      *
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
     * 353      X
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

    bot->message(msg);
}

void IrcConnection::login()
{
    if ( !server_password.empty() )
        command({"PASS",{server_password},1024});
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
