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

#include "handler/handler.hpp"

namespace handler {

/**
 * \brief Prints a message when a user joins a channel
 */
class IrcJoinMessage: public Handler
{
public:
    IrcJoinMessage(const Settings& settings, Melanobot* bot)
        : Handler(settings,bot)
    {
        message = settings.get("message",message);
        on_self = settings.get("on_self",on_self);
        on_others=settings.get("on_others",on_others);
        if ( message.empty() || !(on_others || on_self) )
            throw ConfigurationError();
    }

    bool can_handle(const network::Message& msg) const override
    {
        return Handler::can_handle(msg) && !msg.channels.empty() &&
            msg.command == "JOIN" &&
            ( ( on_others && msg.from != msg.source->name() ) ||
              ( on_self && msg.from == msg.source->name() ) );
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        user::User user = msg.source->get_user(msg.from);
        reply_to(msg,string::replace(message,{
            {"channel",msg.channels[0]},
            {"nick", user.name},
            {"host", user.host},
            {"name", user.global_id}
        },'%'));
        return true;
    }

private:
    std::string message;
    bool        on_self = false;
    bool        on_others = true;
};
REGISTER_HANDLER(IrcJoinMessage,IrcJoinMessage);

/**
 * \brief Prints a message when a user is kicked from a channel
 */
class IrcKickMessage: public Handler
{
public:
    IrcKickMessage(const Settings& settings, Melanobot* bot)
        : Handler(settings,bot)
    {
        message = settings.get("message",message);
        on_self = settings.get("on_self",on_self);
        on_others=settings.get("on_others",on_others);
        if ( message.empty() || !(on_others || on_self) )
            throw ConfigurationError();
    }

    bool can_handle(const network::Message& msg) const override
    {
        return Handler::can_handle(msg) && !msg.channels.empty() &&
            msg.command == "KICK" && msg.params.size() >= 2 &&
            msg.params[0] != msg.source->name() &&
            ( ( on_others && msg.from != msg.source->name() ) ||
              ( on_self && msg.from == msg.source->name() ) );
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        reply_to(msg,string::replace(message,{
            {"channel",msg.channels[0]},
            {"kicker", msg.from},
            {"kicked", msg.params[0]},
            {"message", msg.params.size() > 1 ? msg.params.back() : "" }
        },'%'));
        return true;
    }

private:
    std::string message;
    bool        on_self = false;
    bool        on_others = true;
};
REGISTER_HANDLER(IrcKickMessage,IrcKickMessage);

/**
 * \brief Joins again once kicked
 */
class IrcKickRejoin: public Handler
{
public:
    IrcKickRejoin(const Settings& settings, Melanobot* bot)
        : Handler(settings,bot)
    {
        message = settings.get("message",message);
    }

    bool can_handle(const network::Message& msg) const override
    {
        return Handler::can_handle(msg) && !msg.channels.empty() &&
            msg.command == "KICK" && msg.params.size() >= 2 &&
            msg.params[0] == msg.source->name();
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        msg.destination->command({"JOIN",msg.channels,priority});
        if ( !message.empty() )
            reply_to(msg,string::replace(message,{
                {"channel", msg.channels[0]},
                {"kicker", msg.from},
                {"message", msg.params.size() > 1 ? msg.params.back() : "" }
            },'%'));
        return true;
    }

private:
    std::string message;
};
REGISTER_HANDLER(IrcKickRejoin,IrcKickRejoin);

} // namespace handler
