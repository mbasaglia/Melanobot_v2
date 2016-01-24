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
#ifndef HANDLER_HPP
#define HANDLER_HPP

#include <functional>
#include <memory>
#include <string>
#include <stdexcept>
#include <unordered_map>

#include "network/connection.hpp"
#include "settings.hpp"
#include "string/logger.hpp"
#include "melanolib/string/stringutils.hpp"
#include "message/message_consumer.hpp"

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
 * To make a handler visible to \c HandlerFactory, use Melanomodule::register_handler.
 *
 * The constructor of a handler class must have the same signature as the
 * one used here to work properly.
 */
class Handler : public MessageConsumer
{
public:
    Handler(const Settings& settings, MessageConsumer* parent)
        : MessageConsumer(parent)
    {
        priority = settings.get("priority",priority);
    }
    Handler(const Handler&) = delete;
    Handler& operator=(const Handler&) = delete;
    virtual ~Handler() {}

    /**
     * \brief Attempts to handle the message
     * \pre msg.source and msg.destination not null
     * \return \b true if the message has been handled and needs no further processing
     * \note Unless you really need to, override on_handle()
     */
    bool handle(network::Message& msg) override
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
        return true;
    }

    /**
     * \brief Called at startup in case it needs to perform some initialization
     */
    virtual void initialize() {}
    /**
     * \brief Called when the connection ends
     * \todo is this needed?
     */
    virtual void finalize() {}

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
        if ( name == "priority" )
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
    
    /**
     * \brief Noop, by default handlers don't have children
     */
    void add_handler(std::unique_ptr<handler::Handler>&&) override {}

protected:
    /**
     * \brief Required for concrete handlers, performs the handler actions
     * \return \b true if the message has been handled and needs no further processing
     */
    virtual bool on_handle(network::Message& msg) = 0;

    /**
     * \brief Returns the preferred output channel for the given message
     *
     * If \c msg comes from multiple channels, the first one is used
     */
    virtual std::string reply_channel(const network::Message& msg) const
    {
        std::string channel;
        if ( !msg.channels.empty() )
            channel = msg.channels[0];
        return channel;
    }

    /**
     * \brief Send a reply to a message
     * \note \c output priority and channel will be overwritten
     */
    virtual void reply_to(const network::Message& input, network::OutputMessage output) const
    {
        output.target = reply_channel(input);
        output.priority = priority;
        deliver(input.destination,output);
    }
    void reply_to(const network::Message& msg, const string::FormattedString& text) const
    {
        reply_to(msg, {text,false});
    }

    /**
     * \brief Message priority
     * \todo Add filter to enforce it?
     */
    int priority{0};
};

/**
 * \brief A simple action sends a message to the same connection it came from
 *
 * Follows the most basic example to define a SimpleAction handler:
 * \code{.cpp}
class MyAction : public handler::SimpleAction
{
public:
    MyAction(const Settings& settings, MessageConsumer* parent)
        : SimpleAction("default_handler",settings,parent)
    {
        some_setting = settings.get("some_setting",some_setting);
        synopsis += " text...";
        help = "Help message for MyAction";
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        reply_to(msg,"I got: \""+msg.message+"\" from "+msg.from.name);
        return true;
    }

private:
    std::string some_setting = "Default value";
};
module.register_handler<MyAction>("MyAction");
 * \endcode
 */
class SimpleAction : public Handler
{
public:
    /**
     * \brief Sets up the trigger
     * \param default_trigger Default value for \c trigger
     * \param settings        Settings
     * \param parent          Parent object
     */
    SimpleAction(const std::string& default_trigger,
                 const Settings&    settings,
                 MessageConsumer*   parent)
        : Handler(settings,parent),
          trigger(default_trigger)
    {
        load_settings(settings);
        pattern = std::regex(
            melanolib::string::regex_escape(trigger)+"(?:$|\\s+|\\b\\s*)",
            std::regex::ECMAScript|std::regex::optimize
        );
    }


    SimpleAction(const std::string& default_trigger,
                 const std::string& pattern,
                 const Settings&    settings,
                 MessageConsumer*   parent)
        : Handler(settings,parent),
          trigger(default_trigger),
          pattern(pattern,std::regex::ECMAScript|std::regex::optimize)
    {
        load_settings(settings);
    }

    /**
     * Checks authorization, \c direct, \c trigger and available reply channels.
     */
    bool can_handle(const network::Message& msg) const override
    {
        return msg.type == network::Message::CHAT && (msg.direct || !direct);
    }

    /**
     * Checks with can_handle(),
     * then creates a new message which will have the trigger stripped off
     * and calls on_handle() with that.
     */
    virtual bool handle(network::Message& msg) override;

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
        if ( name == "name" || name == "trigger" )
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
    std::string trigger;            ///< String identifying the action
    std::regex  pattern;            ///< Pattern the message needs to match
    bool        direct = false;     ///< Whether the message needs to be direct
    std::string synopsis;           ///< Help synopsis (uses FormatterConfig)
    std::string help="Undocumented";///< Help string (uses FormatterConfig)
    bool        public_reply = true;///< Whether to reply publicly or just to the sender of the message

    std::string reply_channel(const network::Message& msg) const override
    {
        std::string channel;
        if ( !public_reply )
            channel = msg.from.local_id;
        else if ( !msg.channels.empty() )
            channel = msg.channels[0];
        return channel;
    }

    /**
     * \brief Checks if a message matches the pattern
     * \param msg               Message to check
     * \param result[out]       Output from the regex check
     */
    bool matches_pattern(const network::Message& msg, std::smatch& result)
    {
        return std::regex_search(msg.message, result, pattern,
                                 std::regex_constants::match_continuous);
    }

    /**
     * \brief Loads settings
     */
    void load_settings(const Settings& settings)
    {
        trigger      = settings.get("trigger",trigger);
        synopsis     = "#gray##-b#"+trigger+"#-##gray#";
        direct       = settings.get("direct",direct);
        public_reply = settings.get("public",public_reply);
    }

};

/**
 * \brief Handler Factory
 */
class HandlerFactory : public melanolib::Singleton<HandlerFactory>
{
public:
    /**
     * \brief Function object type used to create instances
     */
    using CreateFunction = std::function<std::unique_ptr<Handler>(const Settings&,MessageConsumer*)>;

    /**
     * \brief Builds a handler from its name and settings
     *
     * Inserts the created handler into \c parent
     */
    void build(
        const std::string&      handler_name,
        const Settings&         settings,
        MessageConsumer*        parent) const;

    /**
     * \brief Builds a handler from a template
     *
     * Inserts the created handler into \c parent
     */
    void build_template(
        const std::string&      handler_name,
        const Settings&         settings,
        MessageConsumer*        parent) const;

    /**
     * \brief Register a handler type
     * \param name Public name
     * \param fun  Function creating an object and returning a unique_ptr
     */
    void register_handler(const std::string& name, const CreateFunction& func)
    {
        if ( factory.count(name) )
            ErrorLog("sys") << "Overwriting handler " << name;
        factory[name] = func;
    }

private:
    HandlerFactory() {}
    friend ParentSingleton;

    std::unordered_map<std::string,CreateFunction> factory;
};

} // namespace handler
#endif // HANDLER_HPP
