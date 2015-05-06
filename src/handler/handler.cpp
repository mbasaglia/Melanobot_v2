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
    const std::string& handler_name,
    const Settings& settings,
    handler::HandlerContainer* parent) const
{
    auto type = settings.get_optional<std::string>("template");
    if ( !type )
    {
        ErrorLog("sys") << "Error creating " << handler_name
                << ": missing template reference";
        return nullptr;
    }
    Settings source = parent->melanobot()->get_template(*type);
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
    return build(handler_name,source,parent);
}


std::unique_ptr<Handler> HandlerFactory::build(const std::string& handler_name,
    const Settings& settings, handler::HandlerContainer* parent) const
{
    std::string type = settings.get("type",handler_name);
    if ( type == "Template" )
    {
        return build_template(handler_name, settings, parent);
    }
    else if ( type == "Connection" )
    {
        parent->melanobot()->add_connection(handler_name, settings);
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
