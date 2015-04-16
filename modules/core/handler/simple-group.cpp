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
#include "simple-group.hpp"

namespace handler {

SimpleGroup::SimpleGroup(const Settings& settings, handler::HandlerContainer* parent)
    : SimpleAction("",settings,parent)
{
    // Gather settings
    channels = settings.get("channels","");
    name = settings.get("name",trigger);
    help_group = settings.get("help_group",help_group);

    std::string source_name = settings.get("source","");
    if ( !source_name.empty() )
        source = bot->connection(source_name);
    synopsis = help = "";

    // Copy relevant defaults to show the children
    Settings default_settings;
    for ( const auto& p : settings )
        if ( !p.second.data().empty() &&
                !string::is_one_of(p.first,{"trigger","auth","name","type"}) )
        {
            default_settings.put(p.first,p.second.data());
        }

    // Initialize children
    Settings child_settings = settings;
    for ( auto& p : child_settings )
    {
        /// \note children are recognized by the fact that they
        // start with an uppercase name
        if ( !p.first.empty() && std::isupper(p.first[0]) )
        {
            settings::merge(p.second,default_settings,false);
            auto hand = handler::HandlerFactory::instance().build(
                p.first,
                p.second,
                this
            );
            if ( hand )
                children.push_back(std::move(hand));
        }
    }
}
void SimpleGroup::initialize()
{
    for ( const auto& h : children )
        h->initialize();
}

void SimpleGroup::finalize()
{
    for ( const auto& h : children )
        h->finalize();
}

bool SimpleGroup::on_handle(network::Message& msg)
{
    for ( const auto& h : children )
        if ( h->handle(msg) )
            return true;
    return false;
}

bool SimpleGroup::can_handle(const network::Message& msg) const
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


/**
 * \brief Used by \c AbstractList to add elements
 */
class ListInsert : public SimpleAction
{
public:
    ListInsert(std::string trigger, const Settings& settings, handler::HandlerContainer* parent)
    : SimpleAction(trigger,settings,parent),
        parent(dynamic_cast<AbstractList*>(parent))
    {
        if ( !parent )
            throw ConfigurationError();
        synopsis += " element...";
        help = "Add elements to the list";
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        std::vector<std::string> ok;
        std::vector<std::string> ko;

        for ( const auto& s : string::comma_split(msg.message) )
            ( parent->add(s) ? ok : ko ).push_back(s);

        if ( !ok.empty() )
            reply_to(msg,string::FormattedString() <<
                "Added to "+parent->get_property("list_name")
                +": " << color::green << string::implode(" ",ok));
        else if ( ko.empty() )
            reply_to(msg,"No items were added to "
                +parent->get_property("list_name"));

        if ( !ko.empty() )
            reply_to(msg,string::FormattedString() <<
                string::FormatFlags::BOLD << "Not" << string::FormatFlags::NO_FORMAT <<
                " added to "+parent->get_property("list_name")
                    +": " << color::dark_yellow << string::implode(" ",ko));

        return true;
    }

    AbstractList* parent;
};

/**
 * \brief Used by \c AbstractList to remove elements
 */
class ListRemove : public SimpleAction
{
public:
    ListRemove(std::string trigger, const Settings& settings, handler::HandlerContainer* parent)
    : SimpleAction(trigger,settings,parent),
        parent(dynamic_cast<AbstractList*>(parent))
    {
        if ( !parent )
            throw ConfigurationError();
        synopsis += " element...";
        help = "Remove elements from the list";
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        std::vector<std::string> ok;
        std::vector<std::string> ko;

        for ( const auto& s : string::comma_split(msg.message) )
            ( parent->remove(s) ? ok : ko ).push_back(s);

        if ( !ok.empty() )
            reply_to(msg,string::FormattedString() <<
                "Removed from "+parent->get_property("list_name")
                +": " << color::red << string::implode(" ",ok));
        else if ( ko.empty() )
            reply_to(msg,"No items were removed from "
                +parent->get_property("list_name"));

        if ( !ko.empty() )
            reply_to(msg,string::FormattedString() <<
                string::FormatFlags::BOLD << "Not" << string::FormatFlags::NO_FORMAT <<
                " removed from "+parent->get_property("list_name")
                    +": " << color::dark_yellow << string::implode(" ",ko));

        return true;
    }

private:
    AbstractList* parent;
};

/**
 * \brief Used by \c AbstractList to remove all elements
 */
class ListClear : public SimpleAction
{
public:
    ListClear(const Settings& settings, handler::HandlerContainer* parent)
    : SimpleAction("clear",settings,parent),
        parent(dynamic_cast<AbstractList*>(parent))
    {
        if ( !parent )
            throw ConfigurationError();
        help = "Removes all elements from the list";
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        if ( parent->clear() )
            reply_to(msg,parent->get_property("list_name")+" has been cleared");
        else
            reply_to(msg,parent->get_property("list_name")+" could not be cleared");

        return true;
    }

    AbstractList* parent;
};

AbstractList::AbstractList(const std::string& default_trigger, bool clear,
                           const Settings& settings, handler::HandlerContainer* parent)
    : SimpleGroup(settings, parent)
{
    if ( trigger.empty() )
        trigger = name = default_trigger;

    Settings child_settings;
    for ( const auto& p : settings )
    {
        if ( !p.second.data().empty() )
        {
            if ( p.first == "edit" )
                child_settings.put("auth",p.second.data());
            else if ( p.first != "trigger" && p.first != "auth" && p.first != "name" )
                child_settings.put(p.first,p.second.data());
        }
    }

    children.push_back(std::make_unique<ListInsert>("+",child_settings,this));
    children.push_back(std::make_unique<ListInsert>("add",child_settings,this));
    children.push_back(std::make_unique<ListRemove>("-",child_settings,this));
    children.push_back(std::make_unique<ListRemove>("rm",child_settings,this));
    if ( clear )
        children.push_back(std::make_unique<ListClear>(child_settings,this));
}

bool AbstractList::on_handle(network::Message& msg)
{
    if ( msg.message.empty() )
    {
        auto elem = elements();
        if ( elem.empty() )
            reply_to(msg,get_property("list_name")+" is empty");
        else
            reply_to(msg,get_property("list_name")+": "+string::implode(" ",elem));
        return true;
    }

    return SimpleGroup::on_handle(msg);
}

} // namespace handler
