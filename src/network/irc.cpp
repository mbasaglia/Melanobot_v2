/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015 Mattia Basaglia
 * \license
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

#include "string/logger.hpp"
#include "string/string_functions.hpp"

#define LOCK(mutexname) std::lock_guard<std::mutex> lock_(mutexname)

REGISTER_CONNECTION(irc,&network::irc::IrcConnection::create);
REGISTER_LOG_TYPE(irc,color::dark_magenta);

namespace network {
namespace irc {

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

    for ( const auto pt: settings.get_child("users",{}) )
    {
        if ( pt.first.empty() )
            continue;
        std::vector<std::string> groups = string::comma_split(pt.second.data());

        user::User user;

        if ( pt.first[0] == '!' && pt.first.size() > 1 )
            user.global_id = pt.first.substr(1);
        else if ( pt.first[0] == '@' && pt.first.size() > 1 )
            user.host = pt.first.substr(1);
        else
            user.name = pt.first;

        if ( !groups.empty() )
        {
            auth_system.add_user(user,groups);
            Log("irc",'!',3) << "Registered user " << color::cyan
                << pt.first << color::nocolor << " in "
                << string::implode(", ",groups);
        }
    }
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

Server IrcConnection::server() const
{
    LOCK(mutex);
    return current_server;
}

void IrcConnection::handle_message(Message msg)
{
    if ( msg.command == "001" )
    {
        // RPL_WELCOME: prefix 001 target :message
        if ( msg.params.size() < 1 ) return;

        mutex.lock();
            /// \todo read current_server
            current_nick = msg.params[0];
            current_nick_lowecase = strtolower(current_nick);
            // copy so we can unlock before command()
            auto ctj = channels_to_join;
            channels_to_join.clear();
        mutex.unlock();
        auth();
        connection_status = CONNECTED;
        for ( const auto& s : ctj )
            command({"JOIN",{s}}); /// \todo maybe do more than one channel per command
    }
    else if ( msg.command == "353" )
    {
        // RPL_NAMREPLY: prefix 353 target channel_type channel :users...
        if ( msg.params.size() < 4 ) return;

        std::string channel = msg.params[2];
        msg.channels = { channel };

        std::vector<std::string> users = string::regex_split(msg.params[3],"\\s+");
        mutex.lock();
            for ( auto& user : users )
            {
                if ( user[0] == '@' || user[0] == '+' )
                {
                    /// \todo maybe it would be useful to store operator/voiced status
                    user.erase(0,1);
                }
                user::User *found = user_manager.user(user);
                if ( !found )
                {
                    user::User new_user;
                    new_user.name = new_user.local_id = user;
                    found = user_manager.add_user(new_user);
                    Log("irc",'!',2) << "Added user " << color::dark_green << user;
                }
                found->add_channel(channel);
                Log("irc",'!',3) << "User " << color::dark_cyan << user
                    << color::dark_green << " joined " << color::nocolor << channel;
            }
        mutex.unlock();
    }
    else if ( msg.command == "433" )
    {
        // ERR_NICKNAMEINUSE
        if ( msg.params.size() < 2 ) return;

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

        user::User userfrom = parse_prefix(msg.from);
        msg.from = userfrom.local_id;
        msg.message = message;

        mutex.lock();
            if ( strtolower(msg.params[0]) == current_nick_lowecase )
            {
                msg.channels = { strtolower(userfrom.local_id) };
                msg.direct = 1;
            }
            else
            {
                msg.channels = { msg.params[0] };
            }

        user::User *user = user_manager.user(userfrom.local_id);
        if ( user )
            user->host = userfrom.host;
        mutex.unlock();

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
            mutex.lock();
                std::regex regex_direct(string::regex_escape(current_nick)+":\\s*(.*)");
            mutex.unlock();
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
    else if ( msg.command == "JOIN" )
    {
        /// \todo handle the case when this notifies the bot itself has joined a channel
        if ( msg.params.size() >= 1 )
        {
            user::User user = parse_prefix(msg.from);
            user.channels = msg.params;
            mutex.lock();
                user::User *found = user_manager.user(user.local_id);
                if ( !found )
                {
                    user_manager.add_user(user);
                    Log("irc",'!',2) << "Added user " << color::dark_green << user.name;
                }
                else
                {
                    // we might not have the host if the user was added via 353
                    found->host = user.host;

                    for ( const auto& c : user.channels )
                        found->add_channel(c);
                }
            mutex.unlock();
            Log("irc",'!',3) << "User " << color::dark_cyan << user.name
                << color::dark_green << " joined " << color::nocolor
                << string::implode(", ",user.channels);
            msg.from = user.name;
            msg.channels = user.channels;
        }
    }
    else if ( msg.command == "PART" )
    {
        if ( msg.params.size() >= 1 )
        {
            user::User user = parse_prefix(msg.from);
            user.channels = string::comma_split(msg.params[0]);
            mutex.lock();
                user::User *found = user_manager.user(user.local_id);
                if ( found )
                {
                    for ( const auto& c : user.channels )
                        found->remove_channel(c);

                    Log("irc",'!',3) << "User " << color::dark_cyan << found->name
                        << color::dark_red << " parted " << color::nocolor
                        << string::implode(", ",user.channels);

                    if ( found->channels.empty() )
                    {
                        user_manager.remove_user(found->local_id);
                        Log("irc",'!',2) << "Removed user " << color::dark_red << user.name;
                    }
                }
            mutex.unlock();
            msg.from = user.local_id;
            msg.channels = user.channels;
        }
    }
    else if ( msg.command == "QUIT" )
    {
        user::User user = parse_prefix(msg.from);
        msg.from = user.local_id;
        mutex.lock();
            user::User *found = user_manager.user(user.local_id);
            if ( found )
            {
                msg.channels = found->channels;
                user_manager.remove_user(user.local_id);
                Log("irc",'!',2) << "Removed user " << color::dark_red << user.name;

                if ( strtolower(preferred_nick) == strtolower(user.local_id) )
                {
                    mutex.unlock();
                    command({"NICK",{preferred_nick}});
                    // doesn't need to lock
                    // but lock again to match the following unlock
                    mutex.lock();
                }
            }
        mutex.unlock();
    }
    else if ( msg.command == "NICK" )
    {
        if ( msg.params.size() == 1 )
        {
            user::User user = parse_prefix(msg.from);
            msg.from = user.local_id;
            mutex.lock();
                user::User *found = user_manager.user(user.local_id);
                if ( found )
                {
                    msg.channels = found->channels;
                    found->name = found->local_id = msg.params[0];

                    Log("irc",'!',2) << "Renamed user " << color::dark_cyan << user.name
                        << color::nocolor << " to " << color::dark_cyan << found->name;

                    if ( strtolower(user.name) == current_nick_lowecase )
                    {
                        current_nick = found->name;
                        current_nick_lowecase = strtolower(current_nick);
                        attempted_nick.clear();
                    }
                }
            mutex.unlock();
        }
    }
    // see http://tools.ietf.org/html/rfc2812#section-3 (Messages)
    /* Skipped: * = maybe later X = surely later
     * PASS
     * USER
     * OPER
     * SERVICE
     * SQUIT
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

void IrcConnection::command ( const Command& c )
{
    Command cmd = c;
    std::string command = strtoupper(cmd.command);

    if ( command == "PRIVMSG" || command == "NOTICE" )
    {
        if ( cmd.parameters.size() != 2 )
        {
            ErrorLog("irc") << "Wrong parameters for " << command;
            return;
        }
        std::string to = strtolower(cmd.parameters[0]);
        {
            LOCK(mutex);
            if ( to == current_nick_lowecase )
            {
                ErrorLog("irc") << "Cannot send " << command << " to self";
                return;
            }
        }
        if ( cmd.parameters[1].empty() )
        {
            ErrorLog("irc") << "Empty " << command;
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
        std::string new_nick;

        if ( cmd.parameters.size() == 1 )
            for ( char c : cmd.parameters[0] )
            {
                /// \todo if size > max nick length, break
                if ( !is_nickchar(c) )
                    break;
                new_nick += c;
            }

        if ( new_nick.empty() )
        {
            ErrorLog("irc") << "Ill-formed NICK";
            return;
        }
        cmd.parameters[0] = new_nick;

        /// \todo validate nick
        {
            LOCK(mutex);
            if ( new_nick == current_nick )
                return;
            if ( attempted_nick.empty() )
                preferred_nick = new_nick;
            attempted_nick = new_nick;
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
        if ( connection_status <= CONNECTING )
        {
            channels_to_join.push_back(cmd.parameters[0]);
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

void IrcConnection::disconnect(const std::string& message)
{
    if ( connection_status > CONNECTING )
        buffer.write({"QUIT",{message},1024});
    if ( connection_status != DISCONNECTED )
        buffer.disconnect();
    connection_status = DISCONNECTED;
}

string::Formatter* IrcConnection::formatter() const
{
    return formatter_;
}
bool IrcConnection::channel_mask(const std::vector<std::string>& channels,
                                 const std::string& mask) const
{
    std::vector<std::string> masks = std::move(string::comma_split(mask));
    for ( const auto& m : masks )
    {
        bool match = false;
        if ( m == "!" )
            match = std::any_of(channels.begin(),channels.end(),
                [](const std::string& ch){ return !ch.empty() && ch[0]!='#'; });
        else
            match = string::simple_wildcard(channels,m);
        if ( match )
            return true;
    }
    return false;
}

void IrcConnection::reconnect()
{
    disconnect();
    connect();
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


user::User IrcConnection::parse_prefix(const std::string& prefix)
{
    /// \note the real regex would be stricter
    static std::regex regex_prefix{":?([^!@ ]+)(?:![^@ ]+)?(?:@(\\S+))?",
        std::regex_constants::syntax_option_type::optimize |
        std::regex_constants::syntax_option_type::ECMAScript };

    std::smatch match;

    user::User user;

    if ( std::regex_match(prefix,match,regex_prefix) )
    {
        user.name = user.local_id = match[1].str();
        user.host = match[2].str();
    }

    return user;
}

bool IrcConnection::user_auth(const std::string& local_id,
                              const std::string& auth_group) const
{
    LOCK(mutex);
    const user::User* user = user_manager.user(local_id);
    if ( !user )
        return false;
    return auth_system.in_group(*user,auth_group);
}

void IrcConnection::update_user(const std::string& local_id,
                                const Properties& properties)
{
    LOCK(mutex);
    user::User* user = user_manager.user(local_id);
    if ( user )
        user->update(properties);
}

} // namespace irc
} // namespace network
