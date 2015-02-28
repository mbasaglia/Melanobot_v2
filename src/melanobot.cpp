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
#include "melanobot.hpp"


Melanobot::Melanobot(const Settings& settings )
{
    for(const auto& pt : settings.get_child("connections",{}))
    {
        network::Connection* conn = network::ConnectionFactory::instance().create(this,pt.second);
        if ( conn )
            connections.push_back(conn);
    }
    if ( connections.empty() )
        ErrorLog("sys") << "Creating a bot with no connections";
}
Melanobot::~Melanobot()
{
    for ( auto conn : connections )
        delete conn;
}
void Melanobot::run()
{
    /// \todo to each its own thread
    for ( auto conn : connections )
        conn->run();
}
