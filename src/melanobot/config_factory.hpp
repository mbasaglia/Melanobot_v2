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
#ifndef MELANOBOT_CONFIG_FACTORY_HPP
#define MELANOBOT_CONFIG_FACTORY_HPP

#include <functional>
#include "message/message_consumer.hpp"

namespace melanobot {


/**
 * \brief Class that creates object (via functors) from the settings
 */
class ConfigFactory : public melanolib::Singleton<ConfigFactory>
{
public:
    /**
     * \brief Function object used for item construction
     */
    using CreateFunction = std::function<bool (const std::string&, const Settings&, MessageConsumer*)>;

    /**
     * \brief Builds a single item from its name and settings
     * \param name      Name of the object as from the property tree
     * \param settings  Settings for this item
     * \param parent    Parent (for hanlers)
     * \returns \b true on success
     */
    bool build(
        const std::string& name,
        const Settings& settings,
        MessageConsumer* parent
    ) const;

    /**
     * \brief Builds all items in \p settings
     */
    void build(const Settings& settings, MessageConsumer* parent) const;

    /**
     * \brief Loads \p settings as templates
     */
    void load_templates(const Settings& settings);

    /**
     * \brief Builds a handler from a template
     *
     * Inserts the created handler into \c parent
     */
    bool build_template(
        const std::string&      handler_name,
        const Settings&         settings,
        MessageConsumer*        parent) const;

    /**
     * \brief Registers a config item
     */
    void register_item(const std::string& name, const CreateFunction& func);

private:

    ConfigFactory();
    friend ParentSingleton;

    std::unordered_map<std::string, CreateFunction> factory;

    /// Settings containing configuration template definitions
    Settings templates;
};

} // namespace melanobot
#endif // MELANOBOT_CONFIG_FACTORY_HPP
