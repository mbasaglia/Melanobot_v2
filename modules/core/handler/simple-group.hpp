/**
 * \file
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
#ifndef HANDLER_SIMPLE_GROUP_HPP
#define HANDLER_SIMPLE_GROUP_HPP

#include "handler/handler.hpp"

/**
 * \brief Namespace for classes that handle connection messages
 *
 * \see Handler and SimpleAction for base classes.
 */
namespace handler {

/**
 * \brief A simple group of actions which share settings
 * \todo Add a simple way to create pre-defined group classes which will have
 *       a set of preset actions (eg: Q Whois system, Cup management, Xonotic integration)
 */
class SimpleGroup : public SimpleAction
{
public:
    SimpleGroup(const Settings& settings, handler::HandlerContainer* parent);

    bool can_handle(const network::Message& msg) const override;

    void populate_properties(const std::vector<std::string>& properties, PropertyTree& output) const override;

    std::string get_property(const std::string& name) const override
    {
        if ( name == "name" )
            return this->name;
        else if ( name == "help_group" )
            return help_group;
        return SimpleAction::get_property(name);
    }

protected:
    bool on_handle(network::Message& msg) override;

    std::vector<std::unique_ptr<Handler>> children;         ///< Contained handlers
    std::string           channels;         ///< Channel filter
    /// \todo if posible, merge name and help_group
    std::string           name;             ///< Name to show in help
    std::string           help_group;       ///< Selects whether to be shown in help
    network::Connection*  source = nullptr; ///< Accepted connection (Null => all connections)
};

/**
 * \brief Handles a list of elements (base class)
 * \note Derived classes shall provide the property \c list_name
 *       which contains a human-readable name of the list,
 *       used for descriptions of the handler.
 */
class AbstractList : public SimpleGroup
{
public:
    /**
     * \param default_trigger   Default trigger/group name
     * \param clear             Whether to allow clearing the list
     * \param settings          Handler settings
     * \param bot               Main bot
     */
    AbstractList(const std::string& default_trigger, bool clear,
                 const Settings& settings, handler::HandlerContainer* parent);

    /**
     * \brief Adds \c element to the list
     * \return \b true on success
     */
    virtual bool add(const std::string& element) = 0;
    /**
     * \brief Removes \c element from the list
     * \return \b true on success
     */
    virtual bool remove(const std::string& element) = 0;
    /**
     * \brief Removes all elements from the list
     * \return \b true on success
     */
    virtual bool clear() = 0;

    /**
     * \brief Returns a vector containing all the elements of the string
     */
    virtual std::vector<std::string> elements() const = 0;

    std::string get_property(const std::string& name) const override
    {
        if ( name == "help" )
            return "Manages "+get_property("list_name");
        return SimpleGroup::get_property(name);
    }

protected:
    bool on_handle(network::Message& msg) override;
};

} // namespace handler
#endif // HANDLER_SIMPLE_GROUP
