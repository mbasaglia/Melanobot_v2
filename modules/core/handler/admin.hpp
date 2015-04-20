/**
 * \file
 * \brief This file defines handlers which allows admin to administrate the bot
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

#ifndef HANDLER_ADMIN_HPP
#define HANDLER_ADMIN_HPP

namespace handler {

/**
 * \brief Quits the bot
 */
class AdminQuit: public SimpleAction
{
public:
    AdminQuit(const Settings& settings, handler::HandlerContainer* parent)
        : SimpleAction("quit",settings,parent)
    {
        message = settings.get("message",message);
        synopsis += " [message]";
        help = "Shuts down the bot";
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        std::string quit_msg;
        if ( !msg.message.empty() )
            quit_msg = msg.message;
        else
            quit_msg = message;

        msg.destination->disconnect(quit_msg);
        bot->stop();
        return true;
    }

private:
    std::string message = "Bye!";
};

/**
 * \brief Manages a user group
 */
class AdminGroup: public AbstractList
{
public:
    AdminGroup(const Settings& settings, handler::HandlerContainer* parent)
        : AbstractList(settings.get("group",""),false,settings,parent)
    {
        std::string conn_name = settings.get("connection",settings.get("source",""));
        connection = bot->connection(conn_name);
        if ( !connection )
            throw ConfigurationError();

        description = settings.get("description","the "+trigger+" group");
        ignore = settings.get("ignore","");
    }

    bool add(const std::string& element) override
    {
        if ( ignore.empty() || !connection->user_auth(element, ignore) )
            return connection->add_to_group(element,trigger);
        return false;
    }

    bool remove(const std::string& element) override
    {
        if ( ignore.empty() || !connection->user_auth(element, ignore) )
            return connection->remove_from_group(element,trigger);
        return false;
    }

    bool clear() override
    {
        return false;
    }

    std::vector<std::string> elements() const override
    {
        auto users = connection->users_in_group(trigger);
        std::vector<std::string> names;
        for ( const user::User& user : users )
        {
            if ( !user.global_id.empty() )
                names.push_back('!'+user.global_id);
            else if ( !user.host.empty() )
                names.push_back('@'+user.host);
            else if ( !user.local_id.empty() )
                names.push_back(user.local_id);
            else if ( !user.name.empty() )
                names.push_back(user.name);
        }
        return names;
    }


    std::string get_property(const std::string& name) const override
    {
        if ( name == "list_name" )
            return description;
        return AbstractList::get_property(name);
    }
private:
    network::Connection*connection{nullptr};///< Managed connection
    std::string description; ///< Used as list_name property
    std::string ignore;      ///< Group to be ignored on add/remove
};

/**
 * \brief Discards messages coming from certain users
 */
class FilterGroup: public Handler
{
public:
    FilterGroup(const Settings& settings, handler::HandlerContainer* parent)
        : Handler(settings,parent)
    {
        ignore = settings.get("ignore",ignore);
        if ( ignore.empty() )
            throw ConfigurationError();
    }

    bool can_handle(const network::Message& msg) const override
    {
        return msg.source->user_auth(msg.from.local_id,ignore);
    }

private:
    bool on_handle(network::Message&) override
    {
        return true;
    }

    std::string ignore; ///< Group to ignore
};

/**
 * \brief Makes the bot reconnect
 */
class AdminReconnect: public SimpleAction
{
public:
    AdminReconnect(const Settings& settings, handler::HandlerContainer* parent)
        : SimpleAction("reconnect",settings,parent)
    {
        message = settings.get("message",message);
        synopsis += " [message]";
        help = "Reconnects bot";
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        std::string quit_msg;
        if ( !msg.message.empty() )
            quit_msg = msg.message;
        else
            quit_msg = message;

        msg.destination->reconnect(quit_msg);
        return true;
    }

private:
    std::string message = "Reconnecting...";
};

/**
 * \brief Makes the bot Connect
 */
class AdminConnect: public SimpleAction
{
public:
    AdminConnect(const Settings& settings, handler::HandlerContainer* parent)
        : SimpleAction("connect",settings,parent)
    {
        help = "Connects bot";
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        msg.destination->connect();
        return true;
    }
};

/**
 * \brief Makes the bot disconnect
 */
class AdminDisconnect: public SimpleAction
{
public:
    AdminDisconnect(const Settings& settings, handler::HandlerContainer* parent)
        : SimpleAction("disconnect",settings,parent)
    {
        message = settings.get("message",message);
        synopsis += " [message]";
        help = "Disconnects bot";
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        std::string quit_msg;
        if ( !msg.message.empty() )
            quit_msg = msg.message;
        else
            quit_msg = message;

        msg.destination->disconnect(quit_msg);
        return true;
    }

private:
    std::string message = "Disconnecting...";
};

/**
 * \brief Changes the channel of a message
 */
class Chanhax: public Handler
{
public:
    Chanhax(const Settings& settings, handler::HandlerContainer* parent)
        : Handler(settings,parent),
        trigger(settings.get("trigger","chanhax")),
        regex_chanhax (
            "(.+)\\s*"+string::regex_escape(trigger)+"\\s+(\\S+)",
            std::regex::ECMAScript|std::regex::optimize
        )
    {

    }

    bool can_handle(const network::Message& msg) const override
    {
        return msg.type == network::Message::CHAT && !msg.message.empty();
    }

    std::string get_property(const std::string& name) const override
    {
        if ( name == "name" || name == "trigger" )
            return trigger;
        else if ( name == "help" )
            return "Changes the channel of the message";
        else if ( name == "synopsis" )
            return "(message) "+trigger+" channel...";
        return Handler::get_property(name);
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        std::smatch match;
        if ( std::regex_match(msg.message,match,regex_chanhax) )
        {
            msg.message = match[1];
            msg.channels = {match[2]};
        }
        return false;
    }

private:
    std::string trigger;
    std::regex  regex_chanhax;
};

} // namespace handler
#endif // HANDLER_ADMIN_HPP
