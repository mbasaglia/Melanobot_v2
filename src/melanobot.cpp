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
            connections.push_back(Connection(conn));
    }
    if ( connections.empty() )
        ErrorLog("sys") << "Creating a bot with no connections";
}
Melanobot::~Melanobot()
{
    stop();
}
void Melanobot::stop()
{
    Melanobot::keep_running = false;
    for ( auto &conn : connections )
    {
        conn.stop();
        delete conn.connection;
    }
}
void Melanobot::run()
{
    while ( keep_running )
    {
        network::Message msg;
        get_message(msg);
        if ( !keep_running )
            break;
        /// \todo process messages
    }
}

void Melanobot::message(const network::Message& msg)
{
    std::lock_guard<std::mutex> lock(mutex);
    messages.push(msg);
    condition.notify_one();
}

void Melanobot::get_message(network::Message& msg)
{
    std::unique_lock<std::mutex> lock(mutex);
    condition.wait(lock,[this]{return !messages.empty() && keep_running;});
    if ( !keep_running )
        return;
    msg = messages.front();
    messages.pop();
}
