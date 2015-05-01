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
#ifndef HANDLER_BRIDGE_HPP
#define HANDLER_BRIDGE_HPP

#include "simple-group.hpp"

namespace handler {

/**
 * \brief Acts as bridge across connections
 */
class Bridge : public SimpleGroup
{
public:
    Bridge(const Settings& settings, handler::HandlerContainer* parent);

    /**
     * \brief Attach to the given connection
     */
    void attach(network::Connection* connection);

    /**
     * \brief Attach to the given channel
     */
    void attach_channel(Optional<std::string> channel);

protected:
    bool on_handle(network::Message& msg) override;

    void output_filter(network::OutputMessage& output) const
    {
        if ( dst_channel && output.target.empty() )
            output.target = *dst_channel;
        output.prefix = string::FormattedString(prefix) << output.prefix;
    }

    network::Connection*  destination{nullptr};  ///< Message destination
    Optional<std::string> dst_channel;           ///< Message destination channel
    std::string           prefix;                ///< Output message prefix
};

/**
 * \brief Simply echoes chat messages (to be used in a Bridge group)
 */
class BridgeChat : public Handler
{
public:
    BridgeChat(const Settings& settings, handler::HandlerContainer* parent);
    bool can_handle(const network::Message& msg) const override;

protected:
    bool on_handle(network::Message& msg) override;

    network::Duration timeout{network::Duration::zero()};///< Output message timeout
    bool              ignore_self{true};                 ///< Ignore bot messages
    Optional<std::string> from;                   ///< Override from
};

/**
 * \brief Attach the parent bridge to the provided connection
 */
class BridgeAttach : public SimpleAction
{
public:
    BridgeAttach(const Settings& settings, handler::HandlerContainer* parent);
    void initialize() override;

protected:
    bool on_handle(network::Message& msg) override;

    std::string protocol;    ///< Limit only to connections with this protocol
    bool    detach{true};    ///< Allow using this to detach the bridge
    Bridge* parent{nullptr}; ///< Bridge object to apply the attachment to
};
/**
 * \brief Attach the parent bridge to the provided channel
 */
class BridgeAttachChannel : public SimpleAction
{
public:
    BridgeAttachChannel(const Settings& settings, handler::HandlerContainer* parent);
    void initialize() override;

protected:
    bool on_handle(network::Message& msg) override;

    Bridge* parent{nullptr}; ///< Bridge object to apply the attachment to
};

/**
 * \brief Base class for JoinMessage and similar
 */
class EventMessageBase: public ::handler::Handler
{
public:
    EventMessageBase(network::Message::Type type,
                     const Settings& settings,
                     ::handler::HandlerContainer* parent)
        : Handler(settings,parent), type(type)
    {
        message = settings.get("message",message);
        action  = settings.get("action",action);
        discard_self = settings.get("discard_self",discard_self);
        discard_others=settings.get("discard_others",discard_others);
        if ( message.empty() ||  (discard_others && discard_self) )
            throw ConfigurationError();

        int timeout_seconds = settings.get("timeout",0);
        if ( timeout_seconds > 0 )
            timeout = std::chrono::duration_cast<network::Duration>(
                std::chrono::seconds(timeout_seconds) );
    }

    bool can_handle(const network::Message& msg) const override
    {
        return msg.type == type && (
                ( discard_others && msg.from.name == msg.source->name() ) ||
                ( discard_self && msg.from.name != msg.source->name() ) );
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        string::FormatterConfig fmt;
        auto from = msg.source->formatter()->decode(msg.from.name);
        auto str = string::replace(message,message_replacements(msg),"%");
        reply_to(msg,network::OutputMessage(
            fmt.decode(str),
            action,
            reply_channel(msg),
            priority,
            action ? from : string::FormattedString(),
            {},
            timeout == network::Duration::zero() ?
                network::Time::max() :
                network::Clock::now() + timeout
        ));
        return true;
    }

    virtual Properties message_replacements(const network::Message& msg) const
    {
        string::FormatterConfig fmt;
        auto from = msg.source->formatter()->decode(msg.from.name);
        return {
            {"channel",string::implode(", ",msg.channels)},
            {"name", from.encode(fmt)},
            {"host", msg.from.host},
            {"global_id", msg.from.global_id},
            {"message", msg.source->formatter()->decode(msg.message).encode(fmt)}
        };
    }

private:
    network::Message::Type type;///< Type of messages to be handled
    std::string message;        ///< Message to send
    bool        action = false; ///< Whether it should output an action
    bool        discard_self{0};///< Whether not triggered when the joining user has the same name as the source connection
    bool        discard_others{0};///< Whether not triggered when the joining user name differs from the source connection
    network::Duration timeout{network::Duration::zero()};///< Output message timeout
};

/**
 * \brief Prints a message when a user joins a channel (or server)
 */
class JoinMessage: public EventMessageBase
{
public:
    JoinMessage(const Settings& settings, ::handler::HandlerContainer* parent)
        : EventMessageBase(network::Message::JOIN,settings,parent)
    {}
};

/**
 * \brief Prints a message when a user parts a channel (or server)
 */
class PartMessage: public EventMessageBase
{
public:
    PartMessage(const Settings& settings, ::handler::HandlerContainer* parent)
        : EventMessageBase(network::Message::PART,settings,parent)
    {}
};
/**
 * \brief Prints a message when a user is kicked from a channel
 */
class KickMessage: public EventMessageBase
{
public:
    KickMessage(const Settings& settings, ::handler::HandlerContainer* parent)
        : EventMessageBase(network::Message::KICK,settings,parent)
    {}

protected:
    Properties message_replacements(const network::Message& msg) const override
    {
        string::FormatterConfig fmt;
        return {
            {"channel",string::implode(", ",msg.channels)},
            {"message",msg.message},

            {"kicker", msg.source->formatter()->decode(msg.from.name).encode(fmt)},
            {"kicker.host", msg.from.host},
            {"kicker.global_id", msg.from.global_id},
            {"kicker.local_id", msg.from.local_id},

            {"kicked", msg.source->formatter()->decode(msg.victim.name).encode(fmt)},
            {"kicked.host", msg.victim.host},
            {"kicked.global_id", msg.victim.global_id},
            {"kicked.local_id", msg.victim.local_id}
        };
    }
};

/**
 * \brief Prints a message when a user changes name
 * \note %name expands to the old name, %message to the new one
 */
class RenameMessage: public EventMessageBase
{
public:
    RenameMessage(const Settings& settings, ::handler::HandlerContainer* parent)
        : EventMessageBase(network::Message::RENAME,settings,parent)
    {}
};

} // namespace handler
#endif // HANDLER_BRIDGE_HPP
