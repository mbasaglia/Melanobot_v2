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

#include "bridge.hpp"

namespace handler {

Bridge::Bridge(const Settings& settings, Melanobot* bot)
    : SimpleGroup(settings,bot)
{

    std::string destination_name = settings.get("destination","");
    if ( destination_name.empty() )
        throw ConfigurationError();

    destination = bot->connection(destination_name);
    dst_channel = settings.get_optional<std::string>("dst_channel");
}

bool Bridge::on_handle(network::Message& msg)
{
    network::Message& targeted = msg;
    targeted.destination = destination;
    if ( dst_channel )
        targeted.dst_channel = dst_channel;
    return SimpleGroup::on_handle(targeted);
}

} // namespace handler
