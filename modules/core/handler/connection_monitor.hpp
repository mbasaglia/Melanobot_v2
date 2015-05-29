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
#ifndef CONNECTION_MONITOR_HPP
#define CONNECTION_MONITOR_HPP

#include "handler/handler.hpp"

namespace handler {

/**
 * \brief Base for handlers needing to query a connection while
 * sending and receiving messages from a different connection
 */
class ConnectionMonitor : public handler::SimpleAction
{
public:
    ConnectionMonitor(const std::string& default_trigger,
                      const Settings& settings,
                      MessageConsumer* parent)
        : SimpleAction(default_trigger,settings,parent)
    {
        std::string monitored_name = settings.get("monitored","");
        if ( !monitored_name.empty() )
            monitored = bot()->connection(monitored_name);
        if ( !monitored )
            throw ConfigurationError();
    }

protected:
    network::Connection* monitored{nullptr}; ///< Monitored connection
};

/**
 * \brief Shows a message saying whether the server is connected or not
 */
class MonitorServerStatus : public ConnectionMonitor
{
public:
    MonitorServerStatus(const Settings& settings, MessageConsumer* parent)
        : ConnectionMonitor("status", settings, parent)
    {
        connected = settings.get("connected", connected);
        disconnected = settings.get("disconnected", disconnected);
        help = "Shows whether the server is connected";
    }

protected:
    bool on_handle(network::Message& msg)
    {
        string::FormatterConfig fmt;
        if ( monitored->status() >= network::Connection::CHECKING )
            reply_to(msg,fmt.decode(connected));
        else
            reply_to(msg,fmt.decode(disconnected));
        return true;
    }

    std::string connected = "#dark_green#Server is connected";
    std::string disconnected = "#red#Server is not connected";
};

/**
 * \brief Shows the server description
 */
class MonitorReply : public ConnectionMonitor
{
public:
    MonitorReply(const Settings& settings, MessageConsumer* parent)
        : ConnectionMonitor("",settings,parent)
    {
        reply = settings.get("reply", reply);
        if ( reply.empty() )
            throw ConfigurationError();
        help = settings.get("help", help);
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        string::FormatterConfig fmt;
        reply_to(msg,fmt.decode(string::replace(reply,monitored->message_properties(),"%")));
        return true;
    }

    std::string reply;
};

} // namespace handler
#endif // CONNECTION_MONITOR_HPP
