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
#include "handler.hpp"
#include "melanobot.hpp"

namespace handler {

bool SimpleAction::handle(network::Message& msg)
{
    if ( can_handle(msg) )
    {
        if ( trigger.empty() )
            return on_handle(msg);

        std::smatch match;
        if ( matches_pattern(msg, match) )
        {
            std::string old = msg.message;
            msg.message.erase(0, match.length());
            auto ret = on_handle(msg);
            msg.message = old;
            return ret;
        }
    }
    return false;
}


void HandlerFactory::build_template(
    const std::string&  handler_name,
    const Settings&     settings,
    MessageConsumer*    parent) const
{
    auto type = settings.get_optional<std::string>("template");
    if ( !type )
    {
        ErrorLog("sys") << "Error creating " << handler_name
                << ": missing template reference";
        return;
    }
    Settings source = Melanobot::instance().get_template(*type);
    Properties arguments;
    for ( const auto& ch : source )
        if ( melanolib::string::starts_with(ch.first,"@") )
        {
            arguments[ch.first] = settings.get(ch.first.substr(1),ch.second.data());
        }
    ::settings::recurse(source,[arguments](Settings& node){
        node.data() = melanolib::string::replace(node.data(),arguments);
    });
    /// \todo recursion check
    build(handler_name,source,parent);
}


void HandlerFactory::build(
    const std::string&  handler_name,
    const Settings&     settings,
    MessageConsumer*    parent) const
{
    std::string type = settings.get("type",handler_name);

    if ( !settings.get("enabled",true) )
    {
        Log("sys",'!') << "Skipping disabled handler " << color::red << handler_name;
        return;
    }

    if ( type == "Template" )
    {
        build_template(handler_name, settings, parent);
    }
    else if ( type == "Connection" )
    {
        Melanobot::instance().add_connection(handler_name, settings);
    }
    else
    {
        auto it = factory.find(type);
        if ( it != factory.end() )
        {
            try {
                parent->add_handler(it->second(settings, parent));
            } catch ( const ConfigurationError& error ) {
                ErrorLog("sys") << "Error creating " << handler_name << ": "
                    << error.what();
            }
        }
        else
        {
            ErrorLog("sys") << "Unknown handler type: " << handler_name;
        }
    }
}

} // namespace handler
