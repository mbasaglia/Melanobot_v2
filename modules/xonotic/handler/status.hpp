/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright  Mattia Basaglia
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
#ifndef XONOTIC_HANDLER_STATUS_HPP
#define XONOTIC_HANDLER_STATUS_HPP

#include "handler/handler.hpp"
#include "string/string_functions.hpp"

namespace xonotic {

/**
 * \brief Show server connect/disconnect messages
 */
class ConnectionEvents : public handler::Handler
{
public:
    ConnectionEvents( const Settings& settings, HandlerContainer* parent )
        : Handler(settings, parent)
    {
        connect = settings.get("connect","Server #2#%host#-# connected.");
        disconnect = settings.get("connect","#-b#Warning!#-# Server #1#%host#-# disconnected.");
    }

    bool can_handle(const network::Message& msg) const override
    {
        return Handler::can_handle(msg) &&
            string::is_one_of(msg.command,{"CONNECTED","DISCONNECTED"});
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        string::FormatterConfig fmt;
        /// \todo Should the host property be returned from description()?
        Properties props = {
            {"host",msg.source->formatter()->decode(msg.source->get_property("host")).encode(&fmt)},
            {"server",msg.source->server().name()}
        };
        const std::string& str = msg.command == "CONNECTED" ? connect : disconnect;
        reply_to(msg,fmt.decode(string::replace(str,props,"%")));
        return true;
    }

private:
    std::string connect;
    std::string disconnect;
};

} // namespace xonotic
#endif // XONOTIC_HANDLER_STATUS_HPP
