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
#ifndef HANDLER_HPP
#define HANDLER_HPP

#include <functional>
#include <memory>
#include <string>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>

#include "network/connection.hpp"
#include "settings.hpp"
#include "string/logger.hpp"
#include "string/string_functions.hpp"
#include "melanobot.hpp"

/**
 * \brief Namespace for classes that handle connection messages
 *
 * \see Handler and SimpleAction for base classes.
 */
namespace handler {

/**
 * \brief Message handler abstract base class
 *
 * To create concrete handlers, inherit this class and specialize the virtual
 * methods (see their own documentation).
 *
 * For simple cases, inherit \c SimpleAction and specialize on_handle().
 *
 * To make a handler visible to \c HandlerFactory, use REGISTER_HANDLER(class_name,public_name) .
 *
 * The constructor of a handler class must have the same signature as the
 * one used here to work properly.
 */
class Handler : public handler::HandlerContainer
{
public:
    Handler( const Settings& settings, Melanobot* bot ) : bot ( bot )
    {
        auth = settings.get("auth",auth);
        priority = settings.get("priority",priority);
        if ( !bot )
            throw ConfigurationError();
    }
    Handler(const Handler&) = delete;
    Handler& operator=(const Handler&) = delete;
    virtual ~Handler() {}

    /**
     * \brief Attempt to handle the message
     * \pre msg.source and msg.destination not null
     * \return \b true if the message has been handled and needs no further processing
     * \note Unless you really need to, override on_handle()
     */
    virtual bool handle(network::Message& msg)
    {
        if ( can_handle(msg) )
            return on_handle(msg);
        return false;
    }

    /**
     * \brief Checks if a message can be handled
     * \pre msg.source not null
     * \return \b true if the message can be handled by the handler
     */
    virtual bool can_handle(const network::Message& msg) const
    {
        return authorized(msg);
    }

    /**
     * \brief Called at startup in case it needs to perform some initialization
     * \todo Run after the connection has been established? (needs to know what connections this class handles)
     */
    virtual void initialize() {}
    /**
     * \brief Called when the connection ends
     * \todo see todo in initialize(), also is this needed?
     */
    virtual void finalize() {}

    /**
     * \brief Checks if a message is authorized to be executed by this message
     */
    virtual bool authorized(const network::Message& msg) const
    {
        return auth.empty() || msg.source->user_auth(msg.from,auth);
    }

    /**
     * \brief Get a property by name
     * \note If you inherit a Handler class, it is recommended that you
     *       override this for properties of that class but still call the
     *       parent version.
     * \note The Help handler uses the property "name" to determine if a handler
     *       has to be displayed, and uses "synopsis" and "help" to display
     *       information
     */
    virtual std::string get_property(const std::string& name) const
    {
        if ( name == "auth" )
            return auth;
        else if ( name == "priority" )
            return std::to_string(priority);
        return {};
    }

    /**
     * \brief Simply appends globally the given property
     */
    void populate_properties(const std::vector<std::string>& properties, PropertyTree& output) const override
    {
        for ( const auto& p : properties )
        {
            std::string value = get_property(p);
            if ( !value.empty() )
                output.put(p,value);
        }
    }

protected:
    /**
     * \brief Required for concrete handlers, performs the handler actions
     * \return \b true if the message has been handled and needs no further processing
     */
    virtual bool on_handle(network::Message& msg) = 0;

    /**
     * \brief Send a reply to a message
     *
     * If \c msg comes from multiple channels, the first one is used
     */
    virtual void reply_to(const network::Message& msg, const string::FormattedString& text) const
    {
        std::string channel;
        if ( msg.dst_channel )
            channel = *msg.dst_channel;
        else if ( !msg.channels.empty() )
            channel = msg.channels[0];
        msg.destination->say(network::OutputMessage(channel,text,priority));
    }

    void reply_to(const network::Message& msg, const std::string& text) const
    {
        reply_to(msg, (string::FormattedStream("utf8") << text).str());
    }

    /**
     * \brief Authorization group required for a user message to be handled
     */
    std::string auth;
    /**
     * \brief Message priority
     */
    int priority = 0;
    /**
     * \brief Pointer to the main bot
     */
    Melanobot*  bot = nullptr;
};

/**
 * \brief A simple action sends a message to the same connection it came from
 *
 * Follows the most basic example to define a SimpleAction handler:
 * \code{.cpp}
class MyAction : public SimpleAction
{
public:
    MyAction(const Settings& settings, Melanobot* bot)
        : SimpleAction("default_handler",settings,bot)
    {
        some_setting = settings.get("some_setting",some_setting);
        synopsis += "
        help = "Help message for MyAction";
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        reply_to(msg,"I got: \""+msg.message+"\" from "+msg.from);
        return true;
    }

private:
    std::string some_setting = "Default value";
};
REGISTER_HANDLER(MyAction,MyAction);
 * \endcode
 */
class SimpleAction : public Handler
{
public:
    /**
     * \brief Sets up the trigger
     * \param default_trigger Default value for \c trigger (must not be empty)
     * \param settings        Settings
     * \param bot             Pointer to the bot instance (cannot be null)
     * \throws ConfigurationError If the requirements stated above are not met
     */
    SimpleAction(const std::string& default_trigger, const Settings& settings, Melanobot* bot)
        : SimpleAction ( default_trigger, settings, bot, false ) {}

    /**
     * Checks authorization, \c direct, \c trigger and available reply channels.
     */
    bool can_handle(const network::Message& msg) const override
    {
        return authorized(msg) && ( msg.direct || !direct ) &&
            msg.channels.size() < 2 && string::starts_with(msg.message,trigger);
    }

    /**
     * Checks with can_handle(),
     * then creates a new message which will have the trigger stripped off
     * and calls on_handle() with that.
     */
    virtual bool handle(network::Message& msg)
    {
        if ( can_handle(msg) )
        {
            network::Message trimmed_msg = msg;
            auto it = trimmed_msg.message.begin()+trigger.size();
            it = std::find_if(it,trimmed_msg.message.end(),[](char c){return !std::isspace(c);});
            trimmed_msg.message.erase(trimmed_msg.message.begin(),it);
            return on_handle(trimmed_msg);
        }
        return false;
    }

    /**
     * Extra properties:
     *  * trigger
     *  * name (same as trigger)
     *  * direct
     *  * help
     *  * synopsis
     */
    std::string get_property(const std::string& name) const override
    {
        if ( name == "trigger" || name == "name" )
            return trigger;
        else if ( name == "direct" )
            return direct ? "1" : "0";
        else if ( name == "synopsis" )
            return synopsis;
        else if ( name == "help" )
            return help;
        return Handler::get_property(name);
    }

protected:

    std::string          trigger;            ///< String identifying the action
    bool                 direct = false;     ///< Whether the message needs to be direct
    /// \todo help and synopsis should be formatted strings
    std::string          synopsis;           ///< Help synopsis
    std::string          help="Undocumented";///< Help string
    bool                 public_reply = true;///< Whether to reply publicly or just to the sender of the message

    using Handler::reply_to;
    void reply_to(const network::Message& msg, const string::FormattedString& text) const
    {
        std::string channel;
        if ( msg.dst_channel )
            channel = *msg.dst_channel;
        else if ( !public_reply )
            channel = msg.from;
        else if ( !msg.channels.empty() )
            channel = msg.channels[0];
        msg.destination->say(network::OutputMessage(channel,text,priority));
    }

private:

    friend class SimpleGroup;

    /**
     * \brief Sets up the trigger
     * \param default_trigger Default value for \c trigger
     * \param settings        Settings
     * \param bot             Pointer to the bot instance (cannot be null)
     * \param allow_notrigger Whether \c default_trigger can be empty
     * \throws ConfigurationError If the requirements stated above are not met
     */
    SimpleAction(const std::string& default_trigger, const Settings& settings,
                 Melanobot* bot, bool allow_notrigger)
        : Handler(settings,bot)
    {
        trigger      = settings.get("trigger",default_trigger);
        synopsis     = trigger;
        direct       = settings.get("direct",direct);
        public_reply = settings.get("public",public_reply);
        if ( !allow_notrigger && trigger.empty() )
            throw ConfigurationError();
    }
};

/**
 * \brief A simple group of actions which share settings
 * \todo Add a simple way to create pre-defined group classes which will have
 *       a set of preset actions (eg: Q Whois system, Cup management, Xonotic integration)
 */
class SimpleGroup : public SimpleAction
{
public:
    SimpleGroup(const Settings& settings, Melanobot* bot);

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
                 const Settings& settings, Melanobot* bot);

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

/**
 * \brief Registers a Handler to the HandlerFactory
 * \param class_name  Class to be registered
 * \param public_name Name to be used in the configuration, as a C++ symbol
 */
#define REGISTER_HANDLER(class_name,public_name) \
    static HandlerFactory::RegisterHandler<class_name> \
        RegisterHandler_##public_name(#public_name, \
            [] ( const Settings& settings, Melanobot* bot )  \
                -> std::unique_ptr<Handler> { \
                return std::make_unique<class_name>(settings,bot); \
        })

/**
 * \brief Handler Factory
 */
class HandlerFactory
{
public:
    /**
     * \brief Function object type used to create instances
     */
    typedef std::function<std::unique_ptr<Handler>(const Settings&,Melanobot*)> CreateFunction;

    /**
     * \brief Dummy class for auto-registration
     */
    template <class HandlerClass>
    struct RegisterHandler
    {
        static_assert(std::is_base_of<Handler,HandlerClass>::value, "Wrong class for HandlerFactory");
        RegisterHandler(const std::string& name, const CreateFunction& func)
        {
            std::string lcname = string::strtolower(name);
            if ( HandlerFactory().instance().factory.count(lcname) )
                ErrorLog("sys") << "Overwriting handler " << name;
            HandlerFactory().instance().factory[lcname] = func;
        }
    };

    static HandlerFactory& instance()
    {
        static HandlerFactory singleton;
        return singleton;
    }

    /**
     * \brief Build a handler from its name and settings
     * \return \c nullptr if it could not be created
     */
    std::unique_ptr<Handler> build(const std::string& handler_name,
                   const Settings& settings, Melanobot* bot) const
    {
        auto it = factory.find(settings.get("type",string::strtolower(handler_name)));
        if ( it != factory.end() )
        {
            try {
                return it->second(settings, bot);
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

private:
    HandlerFactory() {}
    HandlerFactory(const HandlerFactory&) = delete;

    std::unordered_map<std::string,CreateFunction> factory;
};

} // namespace handler
#endif // HANDLER_GROUP_HPP
