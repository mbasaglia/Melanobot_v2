/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015-2016 Mattia Basaglia
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

#include "config_factory.hpp"
#include "handler/handler.hpp"

Melanobot& Melanobot::load(const Settings& settings)
{
    static bool loaded = false;
    if ( loaded )
        CRITICAL_ERROR("Melanobot settings loaded twice");
    loaded = true;

    /// \todo All this could be moved to main()
    melanobot::ConfigFactory::instance().load_templates(settings.get_child("templates",{}));
    melanobot::ConfigFactory::instance().build(settings.get_child("bot",{}), &instance());

    if ( instance().connections.empty() )
    {
        settings::global_settings.put("exit_code",1);
        CRITICAL_ERROR("Creating a bot with no connections");
    }

    return instance();
}

Melanobot::Melanobot() : MessageConsumer(nullptr)
{
}

Melanobot::~Melanobot()
{
    stop();
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
    
    for ( const auto& handler : handlers )
        handler->initialize();
    
    for ( auto &conn : connections )
        conn.second->start();

    while ( messages.active() )
    {
        network::Message msg;
        messages.pop(msg);
        if ( !messages.active() )
            break;

        if ( !msg.source )
        {
            ErrorLog("sys") << "Received a message without source";
            continue;
        }
        if ( !msg.destination )
            msg.destination = msg.source;

        handle(msg);
    }

    for ( const auto& handler : handlers )
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
    return it->second.get();
}

void Melanobot::add_connection(std::string suggested_name, const Settings& settings)
{
    if ( suggested_name == "Connection" )
        suggested_name.clear();
    suggested_name = settings.get("name",suggested_name);

    if ( suggested_name.empty() )
    {
        ErrorLog("sys") << "Cannot create unnamed connection";
        return;
    }
    else if ( connections.count(suggested_name) )
    {
        ErrorLog("sys") << "Connection " << string::FormatFlags::BOLD << suggested_name
            << string::FormatFlags::NO_FORMAT << " already exists.";
        return;
    }

    auto conn = network::ConnectionFactory::instance().create(settings,suggested_name);

    if ( conn )
    {
        Log("sys",'!') << "Created connection " << color::green << suggested_name;
        connections[suggested_name] = std::move(conn);
    }
}

void Melanobot::add_handler(std::unique_ptr<handler::Handler> && handler)
{
    instance().handlers.push_back(std::move(handler));
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

/// \todo Same code as in AbstractGroup, if possible create a common base
bool Melanobot::handle ( network::Message& msg )
{
    for ( const auto& handler : handlers )
        if ( handler->handle(msg) )
            return true;
    return false;
}
