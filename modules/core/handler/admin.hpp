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
#ifndef HANDLER_ADMIN_HPP
#define HANDLER_ADMIN_HPP

#include "group.hpp"
#include "melanobot.hpp"
#include "storage_base.hpp"

namespace core {

/**
 * \brief Quits the bot
 */
class AdminQuit: public handler::SimpleAction
{
public:
    AdminQuit(const Settings& settings, MessageConsumer* parent)
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
        Melanobot::instance().stop();
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
    AdminGroup(const Settings& settings, MessageConsumer* parent)
        : AbstractList(settings.get("group",""),true,settings,parent)
    {
        std::string conn_name = settings.get("connection",settings.get("source",""));
        connection = Melanobot::instance().connection(conn_name);
        group = settings.get("group","");
        if ( !connection || group.empty() )
            throw ConfigurationError();

        description = settings.get("description","the "+group+" group");
        ignore = settings.get("ignore","");

        storage = settings.get("storage",storage);
        storage_name = "groups."+connection->config_name()+"."+group;
    }

    void initialize() override
    {
        if ( storage && storage::has_storage() )
        {
            auto config_users = elements();
            auto storage_users = storage::storage().maybe_put(storage_name,config_users);

            // Add elements from the storage
            for ( const auto& user : storage_users )
                if ( !ignored(user) )
                    connection->add_to_group(user,group);

            // Sort the two sequences to make them into sets
            std::sort(config_users.begin(), config_users.end());
            std::sort(storage_users.begin(), storage_users.end());
            // Create the output set
            std::vector<std::string> remove;
            // Reserve an approximation of required items
            if ( config_users.size() > storage_users.size() )
                remove.reserve(config_users.size() - storage_users.size());
            // Get the difference between config_users and storage_users
            std::set_difference(
                config_users.begin(), config_users.end(),
                storage_users.begin(), storage_users.end(),
                std::inserter(remove, remove.begin())
            );
            // Remove elements which are in the config but not in the storage
            for ( const auto& user : remove )
                if ( !ignored(user) )
                    connection->remove_from_group(user,group);
        }
    }

    bool add(const std::string& element) override
    {
        if ( !ignored(element) && connection->add_to_group(element,group) )
        {
            save_in_storage();
            return true;
        }
        return false;
    }

    bool remove(const std::string& element) override
    {
        if ( !ignored(element) && connection->remove_from_group(element,group) )
        {
            save_in_storage();
            return true;
        }
        return false;
    }

    bool clear() override
    {
        auto users = connection->users_in_group(group);
        int removed = 0;
        for ( const auto& user : users )
        {
            auto string = user_string(user);
            if ( string && !ignored(*string) &&
                    connection->remove_from_group(*string,group) )
                removed++;
        }
        save_in_storage();
        return removed > 0;
    }

    std::vector<std::string> elements() const override
    {
        auto users = connection->users_in_group(group);
        std::vector<std::string> names;
        for ( const user::User& user : users )
        {
            if ( auto string = user_string(user) )
                names.push_back(*string);
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
    /**
     * \brief Saves the group in the storage system
     */
    void save_in_storage()
    {
        if ( storage && storage::has_storage() )
            storage::storage().put(storage_name,elements());
    }

    /**
     * \brief Whether a user should be ignored
     */
    bool ignored(const std::string& user)
    {
        return !ignore.empty() && connection->user_auth(user, ignore);
    }

    /**
     * \brief Make a string from a user
     * \todo Move to connection?
     * \todo Less useful if Connection functions took a User object instead of a string
     * \see IrcConnection::build_user()
     */
    static Optional<std::string> user_string(const user::User& user)
    {
        if ( !user.global_id.empty() )
            return '!'+user.global_id;
        else if ( !user.host.empty() )
            return '@'+user.host;
        else if ( !user.local_id.empty() )
            return user.local_id;
        else if ( !user.name.empty() )
            return user.name;
        return {};
    }

    network::Connection*connection{nullptr};///< Managed connection
    std::string group;          ///< Managed user group
    std::string description;    ///< Used as list_name property
    std::string ignore;         ///< Group to be ignored on add/remove
    bool        storage{true};  ///< Whether to save change in the storage
    std::string storage_name;   ///< Name to be used in the storage to hold the list
};

/**
 * \brief Discards messages coming from certain users
 */
class FilterGroup: public handler::Handler
{
public:
    FilterGroup(const Settings& settings, MessageConsumer* parent)
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
class AdminReconnect: public handler::SimpleAction
{
public:
    AdminReconnect(const Settings& settings, MessageConsumer* parent)
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
class AdminConnect: public handler::SimpleAction
{
public:
    AdminConnect(const Settings& settings, MessageConsumer* parent)
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
class AdminDisconnect: public handler::SimpleAction
{
public:
    AdminDisconnect(const Settings& settings, MessageConsumer* parent)
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
class Chanhax: public handler::Handler
{
public:
    Chanhax(const Settings& settings, MessageConsumer* parent)
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

} // namespace core
#endif // HANDLER_ADMIN_HPP
