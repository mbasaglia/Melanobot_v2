/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015-2016 Mattia Basaglia
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


bool HandlerFactory::build_template(
    const std::string&  handler_name,
    const Settings&     settings,
    MessageConsumer*    parent) const
{
    auto type = settings.get_optional<std::string>("template");
    if ( !type )
    {
        ErrorLog("sys") << "Error creating " << handler_name
                << ": missing template reference";
        return false;
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
    return build(handler_name,source,parent);
}


bool HandlerFactory::build(
    const std::string&  handler_name,
    const Settings&     settings,
    MessageConsumer*    parent) const
{
    std::string type = settings.get("type",handler_name);

    if ( !settings.get("enabled",true) )
    {
        Log("sys",'!') << "Skipping disabled handler " << color::red << handler_name;
        return false;
    }

    try
    {
        auto it = factory.find(type);
        if ( it != factory.end() )
        {
            parent->add_handler(it->second(settings, parent));
            return true;
        }

        auto pit = pseudo_factory.find(type);
        if ( pit != pseudo_factory.end() )
        {
            return pit->second(handler_name, settings, parent);
        }

        ErrorLog("sys") << "Unknown handler type: " << handler_name;

    }
    catch ( const ConfigurationError& error )
    {
        ErrorLog("sys") << "Error creating " << handler_name << ": "
            << error.what();
    }

    return false;
}

void HandlerFactory::register_handler(const std::string& name,
                                      const CreateFunction& func)
{
    if ( avoid_duplicate(name) )
        factory[name] = func;
}

void HandlerFactory::register_pseudo_handler(const std::string& name,
                                             const PseudoHandlerFunction& func)
{
    if ( avoid_duplicate(name) )
        pseudo_factory[name] = func;
}

bool HandlerFactory::avoid_duplicate(const std::string& name) const
{
    if ( factory.count(name) || pseudo_factory.count(name) )
    {
        ErrorLog("sys") << name
            << " has already been registered to the handler factory";
        return false;
    }
    return true;
}

HandlerFactory::HandlerFactory()
{
    register_pseudo_handler("Template",
        [this](const std::string&  handler_name,
               const Settings&     settings,
               MessageConsumer*    parent)
        {
            return build_template(handler_name, settings, parent);
        }
    );

    register_pseudo_handler("Connection",
        [this](const std::string&  handler_name,
               const Settings&     settings,
               MessageConsumer*    parent)
        {
            Melanobot::instance().add_connection(handler_name, settings);
            return true;
        }
    );
}


} // namespace handler
