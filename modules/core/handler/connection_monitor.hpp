/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015-2017 Mattia Basaglia
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

#include "melanobot/handler.hpp"
#include "melanobot/melanobot.hpp"

namespace core {

/**
 * \brief Base for handlers needing to query a connection while
 * sending and receiving messages from a different connection
 */
class ConnectionMonitor : public melanobot::SimpleAction
{
public:
    ConnectionMonitor(const std::string& default_trigger,
                      const Settings& settings,
                      MessageConsumer* parent)
        : SimpleAction(default_trigger, settings, parent)
    {
        std::string monitored_name = settings.get("monitored", "");
        if ( !monitored_name.empty() )
            monitored = melanobot::Melanobot::instance().connection(monitored_name);
        if ( !monitored )
            throw melanobot::ConfigurationError();
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
        connected = read_string(settings, "connected",
                                "$(dark_green)Server is connected");
        disconnected = read_string(settings, "disconnected",
                                   "$(red)Server is not connected");
        help = "Shows whether the server is connected";
    }

protected:
    bool on_handle(network::Message& msg)
    {
        if ( monitored->status() >= network::Connection::CHECKING )
            reply_to(msg, connected);
        else
            reply_to(msg, disconnected);
        return true;
    }

    string::FormattedString connected;
    string::FormattedString disconnected;
};

/**
 * \brief Shows the server description
 */
class MonitorReply : public ConnectionMonitor
{
public:
    MonitorReply(const Settings& settings, MessageConsumer* parent)
        : ConnectionMonitor("", settings, parent)
    {
        reply = read_string(settings, "reply", "");
        if ( reply.empty() )
            throw melanobot::ConfigurationError();
        help = settings.get("help", help);
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        reply_to(msg, reply.replaced(monitored->pretty_properties()));
        return true;
    }

    string::FormattedString reply;
};

} // namespace core
#endif // CONNECTION_MONITOR_HPP
