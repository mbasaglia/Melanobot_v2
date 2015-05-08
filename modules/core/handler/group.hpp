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
 * \brief Base class for group-like handlers
 */
class AbstractGroup : public SimpleAction
{
public:
    using SimpleAction::SimpleAction;

    void initialize() override
    {
        for ( const auto& h : children )
            h->initialize();
    }

    void finalize() override
    {
        for ( const auto& h : children )
            h->finalize();
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        for ( const auto& h : children )
            if ( h->handle(msg) )
                return true;
        return false;
    }

    /**
     * \brief Creates children from settings
     * \param child_settings    Settings for all the children
     * \param default_settings  Settings to fallback to for every child
     */
    void add_children(Settings child_settings,
                      const Settings& default_settings={});


    std::vector<std::unique_ptr<Handler>> children;  ///< Contained handlers
};

/**
 * \brief A simple group of actions which share settings
 */
class Group : public AbstractGroup
{
public:
    Group(const Settings& settings, handler::HandlerContainer* parent);

    bool can_handle(const network::Message& msg) const override;

    void populate_properties(const std::vector<std::string>& properties, PropertyTree& output) const override;

    std::string get_property(const std::string& name) const override
    {
        if ( name == "auth" )
            return auth;
        else if ( name == "name" )
            return this->name;
        else if ( name == "help_group" )
            return help_group;
        return SimpleAction::get_property(name);
    }

    /**
     * \brief Checks if a message is authorized to be executed by this message
     */
    bool authorized(const network::Message& msg) const
    {
        return auth.empty() || msg.source->user_auth(msg.from.local_id,auth);
    }

protected:
    bool on_handle(network::Message& msg) override;

    void output_filter(network::OutputMessage& output) const override
    {
        output.prefix = string::FormattedString(prefix) << output.prefix;
    }

    /**
     * \brief Authorization group required for a user message to be handled
     */
    std::string          auth;
    std::string          channels;          ///< Channel filter
    network::Connection* source = nullptr;  ///< Accepted connection (Null => all connections)
    std::string          name;              ///< Name to show in help
    std::string          help_group;        ///< Selects whether to be shown in help
    bool                 pass_through=false;///< Whether it should keep processing the message after a match
    std::string          prefix;            ///< Output message prefix
};

/**
 * \brief Handles a list of elements (base class)
 * \note Derived classes shall provide the property \c list_name
 *       which contains a human-readable name of the list,
 *       used for descriptions of the handler.
 */
class AbstractList : public AbstractGroup
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
        return SimpleAction::get_property(name);
    }

protected:
    bool on_handle(network::Message& msg) override;
};

/**
 * \brief Very basic group with preset handlers, configurable from the config
 */
class PresetGroup : public handler::AbstractGroup
{
public:
    PresetGroup( const std::initializer_list<std::string>& preset,
                 const Settings& settings, HandlerContainer* parent)
        : AbstractGroup("",settings,parent)
    {
        add_children(settings::merge_copy(settings,
            settings::from_initializer(preset), false));
        synopsis = "";
        help = "";
    }

    bool can_handle(const network::Message&) const override { return true; }
};

/**
 * \brief A group muticasting to its children
 * (which should be SimpleActions with a non-empty trigger)
 */
class Multi : public Group
{
public:
    Multi(const Settings& settings, handler::HandlerContainer* parent);

    bool can_handle(const network::Message& msg) const override;


    bool handle(network::Message& msg) override
    {
        return Handler::handle(msg);
    }

protected:
    bool on_handle(network::Message& msg) override;

private:
    std::vector<std::string> prefixes;
};

} // namespace handler
#endif // HANDLER_SIMPLE_GROUP
