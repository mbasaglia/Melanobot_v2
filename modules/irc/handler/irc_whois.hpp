/**
 * \file
 * \brief This file contains handlers for WHOIS recognition on IRC
 * 
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
#ifndef IRC_HANDLER_WHOIS
#define IRC_HANDLER_WHOIS

#include "handler/handler.hpp"

namespace irc {
namespace handler {

/**
 * \brief Sets the global id based on a 330 reply from whois (IRC)
 */
class Whois330 : public ::handler::Handler
{
public:
    Whois330(const Settings& settings, ::handler::HandlerContainer* parent)
        : Handler(settings,parent) {}

    bool can_handle(const network::Message& msg) const override
    {
        return msg.command == "330" && msg.params.size() > 2 &&
            msg.source == msg.destination;
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        msg.source->update_user(msg.params[1],{{"global_id",msg.params[2]}});
        return true;
    }

};

/**
 * \brief Asks Q for WHOIS or USERS information when a user joins a channel (IRC)
 * \note This will only work if the bot has a Q account, and USERS requires +k
 *       or better on the channel.
 */
class QSendWhois : public ::handler::Handler
{
public:
    QSendWhois(const Settings& settings, ::handler::HandlerContainer* parent)
        : Handler(settings,parent)
    {
        q_bot = settings.get("q_to",q_bot);
    }

    bool can_handle(const network::Message& msg) const override
    {
        return msg.type == network::Message::JOIN && msg.params.size() == 1 &&
            msg.source->protocol() == "irc";
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        if ( msg.source->name() == msg.from.name )
        {
            msg.destination->command({"PRIVMSG",{q_bot,"users "+msg.channels[0]},priority});
        }
        else
        {
            if ( msg.from.global_id.empty() )
                msg.destination->command({"PRIVMSG",{q_bot,"whois "+msg.from.name},priority});
        }

        return false; // reacts to the message but allows to do futher processing
    }

private:
    std::string q_bot = "Q@CServe.quakenet.org";
};

/**
 * \brief Parses responses from Q WHOIS and USER (IRC)
 */
class QGetWhois : public ::handler::Handler
{
public:
    QGetWhois(const Settings& settings, ::handler::HandlerContainer* parent)
        : Handler(settings,parent)
    {
        q_bot = settings.get("q_from",q_bot);
    }

    bool can_handle(const network::Message& msg) const override
    {
        return msg.command == "NOTICE" && msg.from.name == q_bot &&
            msg.source->protocol() == "irc" &&
            msg.params.size() == 2;
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        static std::regex regex_qwhois (
            "-Information for user (\\S+) \\(using account ([^)]+)\\):",
            std::regex_constants::syntax_option_type::optimize |
            std::regex_constants::syntax_option_type::ECMAScript );

        static std::regex regex_startusers (
            "Users currently on #[^]+:",
            std::regex_constants::syntax_option_type::optimize |
            std::regex_constants::syntax_option_type::ECMAScript );

        static std::regex regex_users (
            R"([ @+](\S+)\s+(\S+)\s+(?:\+[a-z]+)?\s+\([^@]+@[^)]+\))",
            std::regex_constants::syntax_option_type::optimize |
            std::regex_constants::syntax_option_type::ECMAScript );

        std::smatch match;
        if ( std::regex_match(msg.params[1],match,regex_qwhois) || (
            expects_users && std::regex_match(msg.params[1],match,regex_users) ))
        {
            msg.source->update_user(match[1],{{"global_id",match[2]}});
            return true;
        }
        else if ( std::regex_match(msg.params[1],regex_startusers) )
        {
            expects_users = true;
            return true;
        }
        else if ( expects_users && msg.params[1] == "End of list." )
        {
            expects_users = false;
            return true;
        }

        return false;
    }

private:
    std::string q_bot = "Q";
    bool expects_users = false; ///< Whether it's parsing the output of USERS
};

/**
 * \brief Sends a WHOIS about the message sender
 */
class WhoisCheckMe : public ::handler::SimpleAction
{
public:
    WhoisCheckMe(const Settings& settings, ::handler::HandlerContainer* parent)
        : SimpleAction("checkme",settings,parent)
    {}

    bool can_handle(const network::Message& msg) const override
    {
        return SimpleAction::can_handle(msg) &&
            msg.source->protocol() == "irc";
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        msg.destination->command({"WHOIS",{msg.from.name},priority});
        return true;
    }

private:
    std::string sources_url;
};


} // namespace handler
} // namespace irc
#endif // IRC_HANDLER_WHOIS
