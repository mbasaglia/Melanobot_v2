/**
 * \file
 * \brief This file defines handlers which allows admin to administrate the bot
 *        (Mostly IRC-specific action)
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

/**
 * \brief Quits the bot
 */
class AdminQuit: public SimpleAction
{
public:
    AdminQuit(const Settings& settings, Melanobot* bot)
        : SimpleAction("quit",settings,bot)
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

        msg.source->disconnect(quit_msg);
        bot->stop();
        return true;
    }

private:
    std::string message = "Bye!";
};
REGISTER_HANDLER(AdminQuit,Quit);

/**
 * \brief Changes the bot nick (IRC)
 */
class AdminNick: public SimpleAction
{
public:
    AdminNick(const Settings& settings, Melanobot* bot)
        : SimpleAction("nick",settings,bot)
    {
        synopsis += " nickname";
        help = "Changes the bot nickname";
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        if ( !msg.message.empty() )
        {
            msg.source->command({"NICK",{msg.message}});
            return true;
        }
        return false;
    }
};
REGISTER_HANDLER(AdminNick,Nick);

/**
 * \brief Makes the bot join channels (IRC)
 */
class AdminJoin: public SimpleAction
{
public:
    AdminJoin(const Settings& settings, Melanobot* bot)
        : SimpleAction("join",settings,bot)
    {
        synopsis += " channel...";
        help = "Makes the bot join one or more channels";
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        std::vector<std::string> channels;
        if ( !msg.message.empty() )
            channels = string::comma_split(msg.message);
        else if ( !msg.channels.empty() )
            channels = msg.channels;
        else
            return false;
        msg.source->command({"JOIN",channels});
        return true;
    }
};
REGISTER_HANDLER(AdminJoin,Join);

/**
 * \brief Makes the bot part channels (IRC)
 */
class AdminPart: public SimpleAction
{
public:
    AdminPart(const Settings& settings, Melanobot* bot)
        : SimpleAction("part",settings,bot)
    {
        synopsis += " [channel]";
        help = "Makes the bot part a channel";
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        std::string channel;
        if ( !msg.message.empty() )
            channel = msg.message; /// \todo gather part message
        else if ( msg.channels.size() == 1 )
            channel = msg.channels[0];
        else
            return false;
        msg.source->command({"PART",{channel}});
        return true;
    }
};
REGISTER_HANDLER(AdminPart,Part);


/**
 * \brief Makes the bot join channels (IRC)
 * \note Use this inside a group
 */
class AcceptInvite: public Handler
{
public:
    AcceptInvite(const Settings& settings, Melanobot* bot)
        : Handler(settings,bot)
    {}

protected:
    bool on_handle(network::Message& msg) override
    {
        if ( msg.command == "INVITE" && msg.params.size() == 2 )
        {
            msg.source->command({"JOIN",{msg.params[1]}});
            return true;
        }
        return false;
    }
};
REGISTER_HANDLER(AcceptInvite,AcceptInvite);

/**
 * \brief Makes the bot execute a raw command (IRC)
 */
class AdminRaw: public SimpleAction
{
public:
    AdminRaw(const Settings& settings, Melanobot* bot)
        : SimpleAction("raw",settings,bot)
    {
        synopsis += " command";
        help = "Sends raw IRC commands";
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        if ( !msg.message.empty() )
        {
            std::istringstream ss(msg.message);
            network::Command cmd;
            ss >> cmd.command;
            char c;
            std::string param;
            while ( ss >> c )
            {
                if ( c == ':' )
                {
                    std::getline(ss, param);
                    cmd.parameters.push_back(param);
                    break;
                }
                else
                {
                    ss.putback(c);
                    ss >> param;
                    cmd.parameters.push_back(param);
                }
            }
            msg.source->command(cmd);
        }
        return true;
    }
};
REGISTER_HANDLER(AdminRaw,Raw);


/**
 * \brief Manages a user group
 */
class AdminGroup: public AbstractList
{
public:
    AdminGroup(const Settings& settings, Melanobot* bot)
        : AbstractList(settings.get("group",""),false,settings,bot)
    {
        if ( !source )
            throw ConfigurationError();

        description = settings.get("description","the "+trigger+" group");
        ignore = settings.get("ignore","");
    }

    bool add(const std::string& element) override
    {
        if ( ignore.empty() || !source->user_auth(element, ignore) )
            return source->add_to_group(element,trigger);
        return false;
    }

    bool remove(const std::string& element) override
    {
        if ( ignore.empty() || !source->user_auth(element, ignore) )
            return source->remove_from_group(element,trigger);
        return false;
    }

    bool clear() override
    {
        return false;
    }

    std::vector<std::string> elements() const override
    {
        auto users = source->users_in_group(trigger);
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
    std::string description; ///< Used as list_name property
    std::string ignore;      ///< Group to be ignored on add/remove
};
REGISTER_HANDLER(AdminGroup,AdminGroup);

/**
 * \brief Discards messages coming from certain users
 */
class FilterGroup: public Handler
{
public:
    FilterGroup(const Settings& settings, Melanobot* bot)
        : Handler(settings,bot)
    {
        if ( auth.empty() )
            throw ConfigurationError();
    }

private:
    bool on_handle(network::Message&) override
    {
        return true;
    }
};
REGISTER_HANDLER(FilterGroup,FilterGroup);

/**
 * \brief Makes the bot reconnect
 */
class AdminReconnect: public SimpleAction
{
public:
    AdminReconnect(const Settings& settings, Melanobot* bot)
        : SimpleAction("reconnect",settings,bot)
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

        msg.source->reconnect(quit_msg);
        return true;
    }

private:
    std::string message = "Reconnecting...";
};
REGISTER_HANDLER(AdminReconnect,Reconnect);

} // namespace handler
