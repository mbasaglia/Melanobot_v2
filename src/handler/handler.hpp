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
 */
namespace handler {

/**
 * \brief Message handler abstract base class
 * \todo User authorization level, help entries
 */
class Handler
{
public:
    Handler( const Settings& settings )
    {
        auth = settings.get("auth",auth);
    }

    virtual ~Handler() {}

    /**
     * \brief Attempt to handle the message
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
     * \return \b true if the message can be handled by the handler
     */
    virtual bool can_handle(const network::Message& msg)
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
        return auth.empty() || ( msg.source && msg.source->user_auth(msg.from,auth));
    }

protected:
    /**
     * \brief Attempt to handle the message
     * \return \b true if the message has been handled and needs no further processing
     */
    virtual bool on_handle(network::Message& msg) = 0;


    /**
     * \brief Send a reply to a message
     */
    virtual void reply_to(const network::Message& msg, const string::FormattedString& text) const = 0;

    void reply_to(const network::Message& msg, const std::string& text) const
    {
        reply_to(msg, (string::FormattedStream("utf8") << text).str());
    }

    /**
     * \brief Authorization group required for a user message to be handled
     */
    std::string auth;
};

/**
 * \brief A simple action sends a message to the same connection it came from
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

    bool can_handle(const network::Message& msg) override
    {
        return msg.source == source && source &&
            authorized(msg) && !msg.message.empty() &&
            ( msg.direct || !direct ) && msg.channels.size() < 2 &&
            string::starts_with(msg.message,trigger);
    }

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

protected:
    using Handler::reply_to; // Show the overload to derived classes
    void reply_to(const network::Message& msg, const string::FormattedString& text) const override
    {
        std::string chan = msg.channels.empty() ? std::string() : msg.channels[0];
        source->say(chan,text);
    }

    Melanobot*           bot;
    network::Connection* source = nullptr; ///< Connection which created the message
    std::string          trigger;          ///< String identifying the action
    bool                 direct = true;    ///< Whether the message needs to be direct
    int                  priority = 0;     ///< Response message priority

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
        : Handler(settings)
    {
        trigger   = settings.get("trigger",default_trigger);
        priority  = settings.get("priority",priority);
        direct    = settings.get("direct",direct);
        source    = bot->connection(settings.get("source",""));
        this->bot = bot;
        if ( !bot || (!allow_notrigger && trigger.empty()) )
            throw ConfigurationError();
    }
};

/**
 * \brief A simple group of actions which share some settings
 * \todo Filter channels
 */
class SimpleGroup : public SimpleAction
{
public:
    SimpleGroup(const Settings& settings, Melanobot* bot);
    ~SimpleGroup() override;

    bool can_handle(const network::Message& msg) override;

protected:
    bool on_handle(network::Message& msg) override;

    std::vector<Handler*> children;
    std::string           channels;
};

#define REGISTER_HANDLER(class_name,public_name) \
    static HandlerFactory::RegisterHandler<class_name> \
        RegisterHandler_##public_name(#public_name, \
            [] ( const Settings& settings, Melanobot* bot ) -> Handler* { \
                return new class_name(settings,bot); \
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
    typedef std::function<Handler*(const Settings&,Melanobot*)> CreateFunction;

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
    Handler* build(const std::string& handler_name,
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
