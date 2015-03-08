/**
 * \file
 * \brief This file contains the definitions for the most basic and reusable handlers
 * 
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
#include "handler.hpp"

namespace handler {

REGISTER_HANDLER(SimpleGroup, Group);

/// \todo option co copy settings from another group
SimpleGroup::SimpleGroup(const Settings& settings, Melanobot* bot)
    : SimpleAction("",settings,bot,true)
{
    channels = settings.get("channels","");
    name = settings.get("name",trigger);
    std::string source_name = settings.get("source","");
    if ( !source_name.empty() )
        source = bot->connection(source_name);
    synopsis = help = "";

    Settings default_settings;
    for ( const auto& p : settings )
        if ( !p.second.data().empty() && p.first != "trigger" &&
                p.first != "auth" && p.first != "name" )
            default_settings.put(p.first,p.second.data());

    Settings child_settings = settings;

    for ( const auto& p : child_settings )
    {
        if ( p.second.data().empty() )
        {
            child_settings.merge_child(p.first,default_settings,false);
            handler::Handler *hand = handler::HandlerFactory::instance().build(
                p.first,
                p.second,
                bot
            );
            if ( hand )
                children.push_back(hand);
        }
    }
}

SimpleGroup::~SimpleGroup()
{
    for ( Handler* h : children )
        delete h;
}

bool SimpleGroup::on_handle(network::Message& msg)
{
    for ( Handler* h : children )
        if ( h->handle(msg) )
            return true;
    return false;
}

bool SimpleGroup::can_handle(const network::Message& msg)
{
    return SimpleAction::can_handle(msg) && (!source || msg.source == source) &&
        (channels.empty() || msg.source->channel_mask(msg.channels, channels));
}

void SimpleGroup::populate_properties(const std::vector<std::string>& properties, PropertyTree& output) const
{
    Handler::populate_properties(properties, output);

    for ( unsigned i = 0; i < children.size(); i++ )
    {
        PropertyTree child;
        children[i]->populate_properties(properties,child);
        if ( !child.empty() || !child.data().empty() )
        {
            std::string name = children[i]->get_property("name");
            if ( name.empty() )
                name = std::to_string(i);
            output.put_child(name,child);
        }
    }
}

} // namespace handler
