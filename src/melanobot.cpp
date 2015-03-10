/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015 Mattia Basaglia
 * \license
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

#include "handler/handler.hpp"

Melanobot::Melanobot(const Settings& settings )
{
    static int counter = 0;
    for(const auto& pt : settings.get_child("connections",{}))
    {
        network::Connection* conn = network::ConnectionFactory::instance().create(this,pt.second);

        std::string id = !pt.first.empty() ? pt.first :
            "unnamed_connection_"+std::to_string(++counter);

        if ( conn )
            connections[id] = conn;
        else
            ErrorLog("sys") << "Could not create connection "
                << string::FormatFlags::BOLD << id;
    }

    if ( connections.empty() )
    {
        Settings::global_settings.put("exit_code",1);
        ErrorLog("sys") << "Creating a bot with no connections";
    }

    for(const auto& pt : settings.get_child("handlers",{}))
    {
        handler::Handler *hand = handler::HandlerFactory::instance().build(pt.first,pt.second,this);
        if ( hand )
            handlers.push_back(hand);
    }

}
Melanobot::~Melanobot()
{
    stop();
    for ( auto &conn : connections )
        delete conn.second;
    for ( auto handler : handlers )
        delete handler;
}
void Melanobot::stop()
{
    messages.stop();
    for ( auto &conn : connections )
        conn.second->stop();
}
void Melanobot::run()
{
    if ( connections.empty() )
        return;
    
    for ( auto &conn : connections )
        conn.second->start();

    for ( auto handler : handlers )
        handler->initialize();

    while ( messages.active() )
    {
        network::Message msg;
        messages.pop(msg);
        if ( !messages.active() )
            break;
        for ( auto handler : handlers )
            if ( handler->handle(msg) )
                break;
    }

    for ( auto handler : handlers )
        handler->finalize();
}

void Melanobot::message(const network::Message& msg)
{
    messages.push(msg);
}

network::Connection* Melanobot::connection(const std::string& name) const
{
    auto it = connections.find(name);
    if ( it == connections.end() )
        return nullptr;
    return it->second;
}

void Melanobot::populate_properties(const std::vector<std::string>& properties, PropertyTree& output) const
{
    for ( unsigned i = 0; i < handlers.size(); i++ )
    {
        PropertyTree child;
        handlers[i]->populate_properties(properties,child);
        if ( !child.empty() || !child.data().empty() )
        {
            std::string name = handlers[i]->get_property("name");
            if ( name.empty() )
                name = std::to_string(i);
            output.put_child(name,child);
        }
    }
}


