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
#include "melanobot/melanobot.hpp"

namespace melanobot {

ConfigFactory::ConfigFactory()
{
    register_item("Template",
        [this](const std::string& handler_name, const Settings& settings, MessageConsumer* parent)
        {
            auto type = settings.get_optional<std::string>("template");
            if ( type )
            {
                auto source = templates.get_child_optional(*type);
                if ( source )
                    return build_template(handler_name, settings, parent, *source);
            }
            ErrorLog("sys") << "Error creating " << handler_name
                    << ": missing template reference";
            return false;
        }
    );

    register_item("Connection",
        [this](const std::string& handler_name, const Settings& settings, MessageConsumer*)
        {
            melanobot::Melanobot::instance().add_connection(handler_name, settings);
            return true;
        }
    );
}

bool ConfigFactory::build_template(
    const std::string&  handler_name,
    Settings     settings,
    MessageConsumer*    parent,
    Settings            template_source) const
{
    Properties arguments;
    for ( const auto& ch : template_source )
        if ( melanolib::string::starts_with(ch.first, "@") )
        {
            std::string key = ch.first.substr(1);
            arguments[ch.first] = settings.get(key, ch.second.data());
            settings.erase(key);
        }
    ::settings::recurse(template_source, [&arguments](Settings& node){
        node.data() = melanolib::string::replace(node.data(), arguments);
    });

    ::settings::merge(template_source, settings, true);

    /// \todo recursion check
    return build(handler_name, "Group", template_source, parent);
}

bool ConfigFactory::build(
    const std::string&  handler_name,
    const Settings&     settings,
    MessageConsumer*    parent) const
{
    return build(handler_name, handler_name, settings, parent);
}

bool ConfigFactory::build(
    const std::string&  handler_name,
    const std::string&  default_type,
    const Settings&     settings,
    MessageConsumer*    parent) const
{
    std::string type = settings.get("type", default_type);

    if ( !settings.get("enabled", true) )
    {
        Log("sys", '!') << "Skipping disabled handler " << color::red << handler_name;
        return false;
    }

    try
    {
        auto pit = factory.find(type);
        if ( pit != factory.end() )
        {
            return pit->second(handler_name, settings, parent);
        }

        ErrorLog("sys") << "Unknown handler type: " << type << " for " << handler_name;

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


    for ( const auto &pair : templates )
    {

        register_item(pair.first,
            [this, &pair](const std::string& handler_name, const Settings& settings, MessageConsumer* parent)
            {
                return build_template(handler_name, settings, parent, pair.second);
            }
        );
    }
}

} // namespace melanobot
