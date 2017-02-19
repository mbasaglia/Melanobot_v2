/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015-2017 Mattia Basaglia
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
#include "irc/network/connection.hpp"

#include "string/logger.hpp"
#include "melanolib/string/stringutils.hpp"
#include "irc/network/functions.hpp"
#include "melanobot/melanobot.hpp"

namespace irc {

std::unique_ptr<IrcConnection> IrcConnection::create(
    const Settings& settings, const std::string& name)
{
    if ( settings.get("protocol", std::string()) != "irc" )
    {
        throw melanobot::ConfigurationError("Wrong protocol for IRC connection");
    }

    network::Server server ( settings.get("server", std::string()) );
    if ( !server.port )
        server.port = 6667;
    server.host = settings.get("server.host", server.host);
    server.port = settings.get("server.port", server.port);
    if ( server.host.empty() || !server.port )
    {
        throw melanobot::ConfigurationError("IRC connection with no server");
    }

    return melanolib::New<IrcConnection>(server, settings, name);
}

IrcConnection::IrcConnection ( const network::Server&   server,
                               const Settings&          settings,
                               const std::string&       name )
    : AuthConnection(name),
     main_server(server),
     current_server(server),
     buffer(*this, settings.get_child("buffer", {}))
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
    server_password  = settings.get("server.password", std::string());
    main_server.host = settings.get("server.host", main_server.host);
    main_server.port = settings.get("server.port", main_server.port);

    preferred_nick   = settings.get("nick", "PleaseNameMe");
    properties_.put("config.nick", preferred_nick);

    formatter_ = string::Formatter::formatter(settings.get("string_format", std::string("irc")));
    connection_status = DISCONNECTED;

    private_notice = settings.get("notice", private_notice);

    auto channels = melanolib::string::comma_split(settings.get("channels", ""));
    for ( const auto& chan : channels )
        command({"JOIN", {chan}, 1024});

    setup_auth(settings);
}

void IrcConnection::connect()
{
    if ( !buffer.connected() )
    {
        connection_status = WAITING;
        if ( !buffer.connect(main_server) )
            return;
        auto lock = make_lock(mutex);
        current_server = main_server;
        lock.unlock();
        connection_status = CONNECTING;
        login();
        buffer.start();
    }
}

void IrcConnection::disconnect(const string::FormattedString& message)
{
    if ( connection_status > CONNECTING )
        buffer.write({"QUIT", {message.encode(*formatter_)}, 1024});
    if ( connection_status != DISCONNECTED )
        buffer.disconnect();
    auto lock = make_lock(mutex);
    connection_status = DISCONNECTED;
    network::Message().disconnected().send(this);
    current_nick = "";
    current_server = main_server;
    properties_.erase("005");
    user_manager.clear();
    buffer.stop();
}

void IrcConnection::reconnect(const string::FormattedString& quit_message)
{
    auto lock = make_lock(mutex);
        user::User* user = user_manager.user(current_nick);
        if ( user )
        {
            for (const auto& chan: user->channels)
                scheduled_commands.push_back({"JOIN", {chan}});
        }
    lock.unlock();

    disconnect(quit_message);
    connect();
}

void IrcConnection::error_stop(const std::string& message)
{
    disconnect();
    network::Message msg;
    msg.type = network::Message::ERROR;
    msg.message = message;
    msg.send(this);
}

network::Server IrcConnection::server() const
{
    auto lock = make_lock(mutex);
    return current_server;
}

std::string IrcConnection::description() const
{
    auto lock = make_lock(mutex);
    std::string irc_network = properties_.get("005.NETWORK", "");
    if ( !irc_network.empty() )
        irc_network = '('+irc_network+") ";
    return irc_network+current_server.name();
}

void IrcConnection::remove_from_channel(const std::string& user_id,
                                        const std::vector<std::string>& channels)
{
    if ( channels.empty() )
        return;

    auto lock = make_lock(mutex);
    if ( strtolower(user_id) == current_nick_lowecase )
    {
        // Can it ever receive more than one channel?
        std::string channel = channels[0];
        std::list<user::User*> chan_users
            = std::move(user_manager.channel_user_pointers(channel));
        // NOTE: chan_users must contain unique values!
        for ( user::User* user : chan_users )
        {
            user->remove_channel(channel);
            if ( user->channels.empty() )
            {
                Log("irc", '!', 2) << "Removed user " << color::dark_red << user->name;
                user_manager.remove_user(user->local_id);
            }
            else
            {
                Log("irc", '!', 3) << "User " << color::dark_cyan << user->name
                    << color::dark_red << " parted " << color::nocolor
                    << channel;
            }
        }
    }
    else
    {
        user::User *found = user_manager.user(user_id);
        if ( found )
        {
            for ( const auto& c : channels )
                found->remove_channel(c);

            if ( found->channels.empty() )
            {
                Log("irc", '!', 2) << "Removed user " << color::dark_red << found->name;
                user_manager.remove_user(found->local_id);
            }
            else
            {
                Log("irc", '!', 3) << "User " << color::dark_cyan << found->name
                    << color::dark_red << " parted " << color::nocolor
                    << melanolib::string::implode(", ", channels);
            }
        }
    }
}

void IrcConnection::handle_message(network::Message msg)
{
    if ( msg.command.empty() ) return;

    msg.from = parse_prefix(msg.from.name);

    user::User *from_user = nullptr;

    if ( !std::isdigit(msg.command[0]) )
    {
        auto lock = make_lock(mutex);
        from_user = user_manager.user(msg.from.local_id);
        if ( from_user )
        {
            from_user->host = msg.from.host;
            msg.from = *from_user;
        }
    }

    if ( msg.command == "001" )
    {
        // RPL_WELCOME: prefix 001 target :message
        if ( msg.params.size() < 1 ) return;

        auto lock = make_lock(mutex);
            current_nick = msg.params[0];
            current_server.host = msg.from.name;
            current_nick_lowecase = strtolower(current_nick);
            // copy so we can unlock before command()
        lock.unlock();
        connection_status = CONNECTED;
        msg.connected();
    }
    else if ( msg.command == "002" )
    {
        // these could be executed on 001, but this gives time to
        // messages triggered on CONNECTED to take over if needed
        auto lock = make_lock(mutex);
            std::list<network::Command> missed_commands;
            scheduled_commands.swap(missed_commands);
        lock.unlock();
        for ( auto& c : missed_commands )
        {
            c.timein = network::Clock::now();
            command(c);
        }
    }
    else if ( msg.command == "005" )
    {
        // RPL_ISUPPORT: prefix 005 target option[=value]... :are supported by this server
        /**
         * \todo Use MAXCHANNELS/CHANLIMIT NICKLEN CHANNELLEN CHANTYPES PREFIX CASEMAPPING
         */
        for ( std::string::size_type i = 1; i < msg.params.size()-1; i++ )
        {
            auto eq = msg.params[i].find('=');
            std::string name = msg.params[i].substr(0, eq);
            std::string value = eq == std::string::npos ? "1" : msg.params[i].substr(eq+1);
            properties_.put("005."+name, value);
        }
    }
    else if ( msg.command == "353" )
    {
        // RPL_NAMREPLY: prefix 353 target channel_type channel :users...
        if ( msg.params.size() < 4 ) return;

        std::string channel = msg.params[2];
        msg.channels = { channel };

        std::vector<std::string> users = melanolib::string::regex_split(msg.params[3], "\\s+");

        {
            auto lock = make_lock(mutex);
            for ( auto& user : users )
            {
                if ( user[0] == '@' || user[0] == '+' )
                {
                    /// \todo maybe it would be useful to store operator/voiced status
                    user.erase(0, 1);
                }
                user::User *found = user_manager.user(user);
                if ( !found )
                {
                    user::User new_user;
                    new_user.origin = this;
                    new_user.name = new_user.local_id = user;
                    found = user_manager.add_user(new_user);
                    Log("irc", '!', 2) << "Added user " << color::dark_green << user;
                }
                found->add_channel(strtolower(channel));
                Log("irc", '!', 3) << "User " << color::dark_cyan << user
                    << color::dark_green << " joined " << color::nocolor << channel;
            }
        }
    }
    else if ( msg.command == "433" )
    {
        // ERR_NICKNAMEINUSE
        if ( msg.params.size() < 2 ) return;

        auto lock = make_lock(mutex);
        if ( strtolower(attempted_nick) == strtolower(msg.params[1]) )
        {
            Log("irc", '!', 4) << attempted_nick << " is taken, trying a new nick";
            /// \todo check nick max length
            /// \todo system to try to get the best nick possible
            network::Command cmd {{"NICK"}, {attempted_nick+'_'}, 1024};
            lock.unlock();
            command(cmd);
        }
    }
    else if ( msg.command == "464" || msg.command == "465" || msg.command == "466" )
    {
        // banned from server
        reconnect();
    }
    else if ( msg.command == "PING" )
    {
        /// \todo read PING timeout in settings
        /// \todo set timer to the latest server message and call PING when too old
        command({"PONG", msg.params, 1024, std::chrono::minutes(3)});
    }
    else if ( msg.command == "PRIVMSG" )
    {
        if ( msg.params.size() != 2 || msg.params[1].empty() )
            return; // Odd PRIVMSG format

        {
            auto lock = make_lock(mutex);
            if ( strtolower(msg.from.name) == current_nick_lowecase )
                return; // received our own message for some reason, disregard

            if ( strtolower(msg.params[0]) == current_nick_lowecase )
            {
                msg.channels = { msg.from.local_id };
                msg.direct = 1;
            }
            else
            {
                msg.channels = { msg.params[0] };
            }
        }

        msg.chat(msg.params[1]);

        // Handle CTCP
        if ( msg.message[0] == '\1' )
        {
            static std::regex regex_ctcp("\1([^ \1]+)(?: ([^\1]+))?\1",
                std::regex_constants::optimize |
                std::regex_constants::ECMAScript);
            std::smatch match;
            std::regex_match(msg.message, match, regex_ctcp);
            msg.message.clear();
            std::string ctcp = strtoupper(match[1].str());

            if ( ctcp == "ACTION" )
            {
                msg.action(match[2]);
            }
            else
            {
                msg.type = network::Message::UNKNOWN;
                msg.command = "CTCP";
                msg.params = { ctcp };
                if ( !match[2].str().empty() )
                    msg.params.push_back(match[2].str());
            }
        }
        else
        {
            auto lock = make_lock(mutex);
                /// \todo Case-Insensitive?
                /// \todo Update only on nick changes
                std::regex regex_direct(melanolib::string::regex_escape(current_nick)+"[:,]\\s+(.+)");
            lock.unlock();
            std::smatch match;
            if ( std::regex_match(msg.message, match, regex_direct) )
            {
                msg.direct = true;
                msg.message = match[1];
            }
        }
    }
    else if ( msg.command == "ERROR" )
    {
        ErrorLog errl("irc", "Server Error");
        if ( msg.params.empty() )
            errl << "Unknown error";
        else
            errl << msg.params[0];
        msg.type = network::Message::ERROR;
        disconnect();
    }
    else if ( msg.command == "JOIN" )
    {
        if ( msg.params.empty() )
            return;
        msg.channels = msg.params;
        {
            auto lock = make_lock(mutex);
            if ( !from_user )
            {
                msg.from.channels = msg.channels;
                from_user = user_manager.add_user(msg.from);
                Log("irc", '!', 2) << "Added user " << color::dark_green << msg.from.name;
            }
            else
            {
                // we might not have the host if the user was added via 353
                from_user->host = msg.from.host;

                for ( const auto& c :  msg.from.channels )
                    from_user->add_channel(strtolower(c));
            }
            msg.from.channels = from_user->channels;
        }
        msg.type = network::Message::JOIN;

        Log("irc", '!', 3) << "User " << color::dark_cyan << msg.from.name
            << color::dark_green << " joined " << color::nocolor
            << melanolib::string::implode(", ", msg.channels);
    }
    else if ( msg.command == "PART" )
    {
        if ( msg.params.size() >= 1 )
        {
            msg.channels = melanolib::string::comma_split(msg.params[0]);
            remove_from_channel(msg.from.name, msg.channels);
            if ( msg.params.size() > 1 )
                msg.message = msg.params[1];
            msg.type = network::Message::PART;
        }
    }
    else if ( msg.command == "QUIT" )
    {
        auto lock = make_lock(mutex);
        if ( strtolower(msg.from.name) == current_nick_lowecase )
        {
            user_manager.clear();
        }
        else if ( from_user )
        {
            msg.channels = from_user->channels;
            user_manager.remove_user(msg.from.local_id);
            Log("irc", '!', 2) << "Removed user " << color::dark_red << msg.from.name;

            if ( strtolower(preferred_nick) == strtolower(msg.from.name) )
            {
                lock.unlock();
                command({"NICK", {preferred_nick}});
            }
            if ( !msg.params.empty() )
                msg.message = msg.params[0];
            msg.type = network::Message::PART;
        }
    }
    else if ( msg.command == "NICK" )
    {
        if ( msg.params.size() == 1 )
        {
            auto lock = make_lock(mutex);
            if ( from_user )
            {
                msg.channels = from_user->channels;
                from_user->name = from_user->local_id = msg.params[0];

                Log("irc", '!', 2) << "Renamed user " << color::dark_cyan << msg.from.name
                    << color::nocolor << " to " << color::dark_cyan << from_user->name;

                if ( strtolower(msg.from.name) == current_nick_lowecase )
                {
                    current_nick = from_user->name;
                    current_nick_lowecase = strtolower(current_nick);
                    attempted_nick.clear();
                }
                msg.message = msg.params[0];
                msg.type = network::Message::RENAME;
            }
        }
    }
    else if ( msg.command == "KICK" )
    {
        if ( msg.params.size() < 2 )
            return;
        msg.channels = melanolib::string::comma_split(msg.params[0]);
        /// \note Assumes a single victim
        msg.victim = get_user(msg.params[1]);
        if ( msg.params.size() > 2 )
            msg.message = msg.params.back();
        remove_from_channel(msg.params[1], msg.channels);
        msg.type = network::Message::KICK;
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

    msg.send(this);
}

void IrcConnection::command(network::Command cmd)
{
    if ( cmd.command.empty() ) return;

    if ( cmd.parameters.empty() && cmd.command.find(' ') != std::string::npos )
    {
        auto msg = buffer.parse_line(cmd.command);
        cmd.command = msg.command;
        cmd.parameters = msg.params;
    }

    cmd.command = strtoupper(cmd.command);


    if ( connection_status <= CONNECTING && cmd.command != "PASS" &&
        cmd.command != "NICK" && cmd.command != "USER" &&
        cmd.command != "PONG" && cmd.command != "MODE" )
    {
        scheduled_commands.push_back(cmd);
        return;
    }

    if ( cmd.command == "PRIVMSG" || cmd.command == "NOTICE" )
    {
        if ( cmd.parameters.size() != 2 )
        {
            ErrorLog("irc") << "Wrong parameters for " << cmd.command;
            return;
        }
        std::string to = strtolower(cmd.parameters[0]);
        {
            auto lock = make_lock(mutex);
            if ( to == current_nick_lowecase )
            {
                ErrorLog("irc") << "Cannot send " << cmd.command << " to self";
                return;
            }
        }
        if ( cmd.parameters[1].empty() )
        {
            ErrorLog("irc") << "Empty " << cmd.command;
            return;
        }

        cmd.parameters[0] = to;
    }
    else if ( cmd.command == "PASS" )
    {
        if ( status() > Connection::CONNECTING )
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
    else if ( cmd.command == "NICK" )
    {
        std::string new_nick;

        if ( cmd.parameters.size() == 1 )
        {
            auto nick_length = properties_.get("005.NICKLEN", std::string::npos);
            nick_length = std::min(nick_length, cmd.parameters[0].size());
            /// \note NICK validation is very basic, the spec is more precise
            for ( unsigned i = 0; i < nick_length; i++ )
            {
                if ( !is_nickchar(cmd.parameters[0][i]) )
                    break;
                new_nick += cmd.parameters[0][i];
            }
        };

        if ( new_nick.empty() )
        {
            ErrorLog("irc") << "Ill-formed NICK";
            return;
        }
        cmd.parameters[0] = new_nick;

        {
            auto lock = make_lock(mutex);
            if ( new_nick == current_nick )
                return;
            if ( attempted_nick.empty() )
                preferred_nick = new_nick;
            attempted_nick = new_nick;
        }
    }
    else if ( cmd.command == "USER" )
    {
        if ( cmd.parameters.size() != 4 )
        {
            ErrorLog("irc") << "Ill-formed USER";
            return;
        }
    }
    else if ( cmd.command == "MODE" )
    {
        auto lock = make_lock(mutex);
        if ( cmd.parameters.size() == 1 )
        {
            cmd.parameters.resize(2);
            cmd.parameters[1] = cmd.parameters[0];
            cmd.parameters[0] = current_nick;
        }
        /*else if ( cmd.parameters.size() != 2 ||
            strtolower(cmd.parameters[0]) != current_nick_lowecase )
        {
            ErrorLog("irc") << "Ill-formed MODE";
            return;
        }
        */
        /// \todo sanitaze the mode string
    }
    else if ( cmd.command == "JOIN" )
    {
        if ( cmd.parameters.size() < 1 || cmd.parameters.size() > 2 )
        {
            ErrorLog("irc") << "Ill-formed JOIN";
            return;
        }

        /**
         * \todo Could keep track of channel keys, so subsequent join requests
         * only need the channel name
         */

        std::vector<std::string> channels = melanolib::string::comma_split(cmd.parameters[0]);
        auto lock = make_lock(mutex);

            user::User* self_user = user_manager.user(current_nick);
            if ( self_user )
            {
                /// \todo Keep keys aligned to channels
                std::sort(self_user->channels.begin(), self_user->channels.end());
                std::sort(channels.begin(), channels.end());
                std::transform(channels.begin(), channels.end(),
                               channels.begin(), strtolower);

                std::vector<std::string> temp_channels;
                temp_channels.reserve(channels.size());
                std::set_difference(
                    channels.begin(), channels.end(),
                    self_user->channels.begin(), self_user->channels.end(),
                    std::back_inserter(temp_channels));
                std::swap(channels, temp_channels);
            }
        lock.unlock();

        if ( channels.empty() )
            return;

        // Remove keys if empty
        if ( cmd.parameters.size() == 2 && cmd.parameters.back().empty() )
            cmd.parameters.pop_back();

        /// \todo keep track of too many channels,
        /// check that channel names are ok
        /// http://tools.ietf.org/html/rfc2812#section-3.2.1
    }
    else if ( cmd.command == "PART" )
    {
        if ( cmd.parameters.size() < 1 )
        {
            ErrorLog("irc") << "Ill-formed PART";
            return;
        }

        auto lock = make_lock(mutex);
        user::User* self_user = user_manager.user(current_nick);
        if ( self_user )
        {
            auto it = std::find(self_user->channels.begin(),
                                self_user->channels.end(),
                                strtolower(cmd.parameters[0]));
            if ( it == self_user->channels.end() )
                return;
        }
    }
    // Custom command
    else if ( cmd.command == "CLEARBUFFER" )
    {
        buffer.clear(cmd.priority);
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

void IrcConnection::say ( const network::OutputMessage& message )
{
    string::FormattedString str;
    if ( !message.prefix.empty() )
        str << message.prefix << ' ' << color::nocolor;
    if ( !message.from.empty() )
    {
        if ( message.action )
            str << "* " << message.from << ' ';
        else
            str << '<' << message.from << color::nocolor << "> ";
    }
    str << message.message;
    std::string text = str.encode(*formatter_);
    if ( message.action && message.from.empty() )
        text = "\1ACTION "+text+'\1';

    std::string irc_command = "PRIVMSG";
    if ( private_notice && message.target[0] != '#' )
        irc_command = "NOTICE";
    command({irc_command, {message.target, text}, message.priority, message.timeout});
}

network::Connection::Status IrcConnection::status() const
{
    return connection_status;
}

std::string IrcConnection::protocol() const
{
    return "irc";
}


string::Formatter* IrcConnection::formatter() const
{
    return formatter_;
}

bool IrcConnection::channel_mask(const std::vector<std::string>& channels,
                                 const std::string& mask) const
{
    std::vector<std::string> masks = melanolib::string::comma_split(strtolower(mask));
    for ( const auto& m : masks )
    {
        bool match = false;
        if ( m == "!" )
        {
            // find channels not starting with #
            match = std::any_of(channels.begin(), channels.end(),
                [](const std::string& ch){ return !ch.empty() && ch[0]!='#'; });
        }
        else
        {
            for ( const auto& chan : channels )
                if ( melanolib::string::simple_wildcard(strtolower(chan), m) )
                {
                    match = true;
                    break;
                }
        }

        if ( match )
            return true;
    }
    return false;
}

void IrcConnection::login()
{
    if ( !server_password.empty() )
        command({"PASS", {server_password}, 1024});
    command({"NICK", {preferred_nick}, 1024});
    command({"USER", {preferred_nick, "0", preferred_nick, preferred_nick}, 1024});
}


user::User IrcConnection::parse_prefix(const std::string& prefix)
{
    /// \note the real regex would be stricter
    static std::regex regex_prefix{":?([^!@ ]+)(?:![^@ ]+)?(?:@(\\S+))?",
        std::regex_constants::optimize |
        std::regex_constants::ECMAScript };

    std::smatch match;

    user::User user;

    if ( std::regex_match(prefix, match, regex_prefix) )
    {
        user.name = user.local_id = match[1].str();
        user.host = match[2].str();
        user.origin = this;
    }

    return user;
}

std::string IrcConnection::name() const
{
    auto lock = make_lock(mutex);
    return current_nick;
}


bool IrcConnection::is_private_channel(const std::string& channel) const
{
    return channel.empty() || channel[0] != '#';
}


std::string IrcConnection::normalize_channel(const std::string& channel) const
{
    return strtolower(channel);
}

user::User IrcConnection::build_user(const std::string& exname) const
{
    if ( exname.empty() )
        return {};

    user::User user;
    user.origin = const_cast<IrcConnection*>(this);

    if ( exname[0] == '!' && exname.size() > 1 )
        user.global_id = exname.substr(1);
    else if ( exname[0] == '@' && exname.size() > 1 )
        user.host = exname.substr(1);
    else
        user.name = exname;

    return user;
}

LockedProperties IrcConnection::properties()
{
    return LockedProperties(&mutex, &properties_);
}

string::FormattedProperties IrcConnection::pretty_properties() const
{
    auto lock = make_lock(mutex);
    return string::FormattedProperties{
        {"network",             properties_.get("005.NETWORK", "")},
        {"default_server",      main_server.name()},
        {"server",              current_server.name()},
        {"nick",                current_nick},
        {"default_nick",        preferred_nick},
    };
}

} // namespace irc
