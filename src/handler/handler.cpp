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
namespace handler {

std::unique_ptr<Handler> HandlerFactory::build_template(
    Melanobot*          bot,
    const std::string&  handler_name,
    const Settings&     settings,
    MessageConsumer*    parent) const
{
    auto type = settings.get_optional<std::string>("template");
    if ( !type )
    {
        ErrorLog("sys") << "Error creating " << handler_name
                << ": missing template reference";
        return nullptr;
    }
    Settings source = bot->get_template(*type);
    Properties arguments;
    for ( const auto& ch : source )
        if ( string::starts_with(ch.first,"@") )
        {
            arguments[ch.first] = settings.get(ch.first.substr(1),ch.second.data());
        }
    ::settings::recurse(source,[arguments](Settings& node){
        node.data() = string::replace(node.data(),arguments);
    });
    /// \todo recursion check
    return build(bot,handler_name,source,parent);
}


std::unique_ptr<Handler> HandlerFactory::build(
    Melanobot*          bot,
    const std::string&  handler_name,
    const Settings&     settings,
    MessageConsumer*    parent) const
{
    std::string type = settings.get("type",handler_name);

    if ( !settings.get("enabled",true) )
    {
        Log("sys",'!') << "Skipping disabled handler " << color::red << handler_name;
        return nullptr;
    }

    if ( type == "Template" )
    {
        return build_template(bot, handler_name, settings, parent);
    }
    else if ( type == "Connection" )
    {
        bot->add_connection(handler_name, settings);
        return nullptr;
    }

    auto it = factory.find(type);
    if ( it != factory.end() )
    {
        try {
            return it->second(settings, parent);
        } catch ( const ConfigurationError& error )
        {
            ErrorLog("sys") << "Error creating " << handler_name << ": "
                << error.what();
            return nullptr;
        }
    }
    ErrorLog("sys") << "Unknown handler type: " << handler_name;
    return nullptr;
}

} // namespace handler
