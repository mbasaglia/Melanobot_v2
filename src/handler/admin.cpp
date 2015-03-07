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
    {}

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
    {}

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
    {}

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
    {}

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


} // namespace handler
