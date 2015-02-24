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
#include "../logger.hpp"

#define LOCK(mutexname) std::lock_guard<std::mutex> lock_(mutexname)

namespace network {
namespace irc {

Buffer::Buffer(IrcConnection& irc, const Settings& settings)
    : irc(irc)
{
    flood_timer = Clock::now();
    flood_timer_max = std::chrono::seconds(settings.get("flood.timer_max",10));
    flood_timer_penalty = std::chrono::seconds(settings.get("flood.timer_penalty",2));
    flood_max_length = settings.get("flood.timer_max",510);
}

void Buffer::insert(const Command& cmd)
{
    LOCK(mutex);
    buffer.push(cmd);
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
    log("irc",'<',cmd.command,0);
    /// \todo write to network (and add \r\n )
}

IrcConnection::IrcConnection ( Melanobot* bot, const Network& network, const Settings& settings )
    : bot(bot), network(network), buffer(*this, settings.get_child("buffer",{}))
{
    network_password = settings.get("network.password",std::string());
    preferred_nick = settings.get("nick","BotWithLazyOwner");
    auth_nick  = settings.get("auth.nick",preferred_nick);
    auth_password = settings.get("auth.password",std::string());
    modes = settings.get("modes",std::string());
}
IrcConnection::IrcConnection ( Melanobot* bot, const Server& server, const Settings& settings )
    : IrcConnection(bot,Network{server},settings)
{}

void IrcConnection::error ( const std::string& message )
{
    /// \todo log("irc",'!',ColorString()+color::red+"Error"+color::nocolor+": "+message);
}

const std::regex IrcConnection::re_message { "(?:(:[^ ]+) )?([a-zA-Z]+|[0-9]{3}) ?(.*)",
    std::regex_constants::syntax_option_type::optimize |
    std::regex_constants::syntax_option_type::ECMAScript
};

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
            if ( strtolower(cmd.parameters[0]) == current_nick_lowecase )
            {
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
    for ( auto it = cmd.parameters.begin(); it != cmd.parameters.end(); ++it )
    {
        if ( it == cmd.parameters.end()-1 && it->find(' ') != std::string::npos )
            cmd.command += " :";
        else
            cmd.command += ' ';
        cmd.command += *it;
    }

    if ( int(cmd.command.size()) > buffer.max_message_length() )
    {
        log("irc",'!',"Truncating "+cmd.command,4);
        cmd.command = cmd.command.substr(0,buffer.max_message_length());
    }
    cmd.parameters.clear();

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
    /// \todo
}

void IrcConnection::disconnect()
{
    if ( status() > CONNECTING )
        command({"QUIT",{},1024});
    /// \todo wait the QUIT to get through (maybe send directly) and close the connection
}


Message IrcConnection::parse_message(const std::string& line) const
{
    Message message;
    message.raw = line;
    if ( message.raw.size() > 0 && message.raw.back() == '\n' )
        message.raw.pop_back();
    if ( message.raw.size() > 0 && message.raw.back() == '\r' )
        message.raw.pop_back();

    std::smatch match;
    std::regex_search(message.raw, match, re_message);
    message.from = match[1].str();
    message.command = match[2].str();

    std::string params = match[3].str();
    auto it = params.begin();
    for ( int i = 0; i < 14 && it != params.end() && *it != ':'; i++ )
    {
        auto next = std::find(it,params.end(),' ');
        message.params.push_back(std::string(it,next));
        it = next;
        if ( it != params.end() )
                ++it;
    }
    if ( it != params.end() )
    {
        if ( *it == ':' )
            ++it;
        message.message = std::string(it,params.end());
        message.params.push_back(message.message);
    }
    return message;
}

void IrcConnection::handle_line(const std::string& line)
{
    Message message = parse_message(line);
    /// \todo append Message to Melanobot
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
    command({"MODE",{current_nick, modes},1024});

}

} // namespace irc
} // namespace network
