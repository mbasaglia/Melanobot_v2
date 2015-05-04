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
                      handler::HandlerContainer* parent)
        : SimpleAction(default_trigger,settings,parent)
    {
        std::string monitored_name = settings.get("monitored","");
        if ( !monitored_name.empty() )
            monitored = bot->connection(monitored_name);
        if ( !monitored )
            throw ConfigurationError();
    }

protected:
    network::Connection* monitored{nullptr}; ///< Monitored connection
};

} // namespace handler
#endif // CONNECTION_MONITOR_HPP
