/**
 * \file
 * \brief File containing Handlers which manage responses to various IRC commands
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
#ifndef IRC_HANDLER_IRC_ACTIONS
#define IRC_HANDLER_IRC_ACTIONS

#include "handler/handler.hpp"

namespace irc {
namespace handler {

/**
 * \brief Joins again once kicked
 */
class IrcKickRejoin: public ::handler::Handler
{
public:
    IrcKickRejoin(const Settings& settings, ::handler::HandlerContainer* parent)
        : Handler(settings,parent)
    {
        message = settings.get("message",message);
    }

    bool can_handle(const network::Message& msg) const override
    {
        return !msg.channels.empty() && msg.type == network::Message::KICK &&
            msg.victim.name == msg.source->name();
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        msg.destination->command({"JOIN",msg.channels,priority});
        if ( !message.empty() )
            reply_to(msg,string::replace(message,{
                {"channel", msg.channels[0]},
                {"kicker", msg.from.name},
                {"message", msg.params.size() > 1 ? msg.params.back() : "" }
            },"%"));
        return true;
    }

private:
    std::string message;
};

} // namespace handler
} // namespace irc
#endif // IRC_HANDLER_IRC_ACTIONS
