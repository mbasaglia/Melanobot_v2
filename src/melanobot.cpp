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
    stop();
    for ( auto &conn : connections )
        delete conn;
}
void Melanobot::stop()
{
    messages.stop();
    for ( auto &conn : connections )
        conn->stop();
}
void Melanobot::run()
{
    for ( auto &conn : connections )
        conn->start();

    while ( messages.active() )
    {
        network::Message msg;
        messages.pop(msg);
        if ( !messages.active() )
            break;
        /// \todo process messages
        Log("sys",'!') << msg.raw;
    }
}

void Melanobot::message(const network::Message& msg)
{
    messages.push(msg);
}
