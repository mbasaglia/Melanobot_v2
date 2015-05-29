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
#ifndef POSIX_HANDLERS_HPP
#define POSIX_HANDLERS_HPP

#include <fstream>
#include <boost/filesystem.hpp>
#include "handler/handler.hpp"

namespace posix {

/**
 * \brief Base for handlers that affect the loop in melanobot.sh
 */
class MelanobotShBase : public handler::SimpleAction
{
public:
    MelanobotShBase(const std::string& default_trigger,
                      const Settings& settings,
                      MessageConsumer* parent)
        : SimpleAction(default_trigger,settings,parent)
    {
        if ( !settings::global_settings.get_child_optional("settings.tmp_dir") )
            throw ConfigurationError();
    }

protected:

    /**
     * \brief Sets the quit action
     */
    static void set_action(const std::string& action)
    {
        std::ofstream(tmp_file("action")) << action << "\n";
    }

    /**
     * \brief Gets the quit action
     */
    static std::string get_action()
    {
        std::string action;
        std::ifstream(tmp_file("action")) >> action;
        return action;
    }

    /**
     * \brief Returns the name of a temporary melanobot.sh file
     */
    static std::string tmp_file(const std::string& file)
    {
        return settings::global_settings.get("settings.tmp_dir",".")+"/"+file;
    }

};

/**
 * \brief Handler that affects the loop in melanobot.sh
 */
class MelanobotShAction : public MelanobotShBase
{
public:
    MelanobotShAction(const Settings& settings, MessageConsumer* parent)
        : MelanobotShBase(settings.get("action",""),settings,parent)
    {
        action = settings.get("action",action);
        if ( action.empty() )
            throw ConfigurationError();
        help = "Changes the quit action to "+action;
    }

protected:
    bool on_handle(network::Message& msg)
    {
        set_action(action);
        reply_to(msg, string::FormattedString()
            << "Changed quit action to " << color::yellow << action);
        return true;
    }

    std::string action;         ///< Action to set
};

/**
 * \brief Handler that restarts the bot
 */
class MelanobotShRestart : public MelanobotShBase
{
public:
    MelanobotShRestart(const Settings& settings, MessageConsumer* parent)
        : MelanobotShBase("restart",settings,parent)
    {
        help = "Restarts the bot";
    }

protected:
    bool on_handle(network::Message& msg)
    {
        if ( get_action() != "loop" )
            set_action("restart");
        bot()->stop();
        return true;
    }

};

/**
 * \brief Handler that quits the bot (disabling automatic restarts)
 */
class MelanobotShQuit : public MelanobotShBase
{
public:
    MelanobotShQuit(const Settings& settings, MessageConsumer* parent)
        : MelanobotShBase("quit",settings,parent)
    {
        help = "Quits the bot";
    }

protected:
    bool on_handle(network::Message& msg)
    {
        set_action("quit");
        bot()->stop();
        return true;
    }

};

} // namespace posix
#endif // POSIX_HANDLERS_HPP
