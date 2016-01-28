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

#include "config_factory.hpp"
#include "melanobot.hpp"

namespace melanobot {

ConfigFactory::ConfigFactory()
{
    register_item("Template",
        [this](const std::string& handler_name, const Settings& settings, MessageConsumer* parent)
        {
            return build_template(handler_name, settings, parent);
        }
    );

    register_item("Connection",
        [this](const std::string& handler_name, const Settings& settings, MessageConsumer*)
        {
            Melanobot::instance().add_connection(handler_name, settings);
            return true;
        }
    );
}

bool ConfigFactory::build_template(
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
    Settings source = templates.get_child(*type, {});
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

bool ConfigFactory::build(
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
        auto pit = factory.find(type);
        if ( pit != factory.end() )
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

void ConfigFactory::build(const Settings& settings, MessageConsumer* parent) const
{
    for(const auto& pt : settings)
    {
        build(pt.first, pt.second, parent);
    }
}

void ConfigFactory::register_item(const std::string& name, const CreateFunction& func)
{
    if ( factory.count(name) )
        ErrorLog("sys") << name << " has already been registered to the handler factory";
    else
        factory[name] = func;
}

void ConfigFactory::load_templates(const Settings& settings)
{
    /// \todo It could register templates by name
    templates = settings;
}

} // namespace melanobot
