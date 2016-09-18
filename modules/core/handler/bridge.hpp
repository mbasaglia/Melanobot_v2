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
#ifndef HANDLER_BRIDGE_HPP
#define HANDLER_BRIDGE_HPP

#include "group.hpp"

namespace core {

/**
 * \brief Acts as bridge across connections
 */
class Bridge : public Group
{
public:
    Bridge(const Settings& settings, MessageConsumer* parent);

    /**
     * \brief Attach to the given connection
     */
    void attach(network::Connection* connection);

    /**
     * \brief Attach to the given channel
     */
    void attach_channel(melanolib::Optional<std::string> channel);

protected:
    bool on_handle(network::Message& msg) override;

    void output_filter(network::OutputMessage& output) const override
    {
        Group::output_filter(output);
        if ( dst_channel && output.target.empty() )
            output.target = *dst_channel;
    }

    network::Connection*  destination{nullptr};  ///< Message destination
    melanolib::Optional<std::string> dst_channel;           ///< Message destination channel
};

/**
 * \brief Simply echoes chat messages (to be used in a Bridge group)
 */
class BridgeChat : public melanobot::Handler
{
public:
    BridgeChat(const Settings& settings, MessageConsumer* parent);
    bool can_handle(const network::Message& msg) const override;

protected:
    bool on_handle(network::Message& msg) override;

    network::Duration timeout{network::Duration::zero()};///< Output message timeout
    bool              ignore_self{true};                 ///< Ignore bot messages
    melanolib::Optional<std::string> from;                   ///< Override from
};

/**
 * \brief Attach the parent bridge to the provided connection
 */
class BridgeAttach : public melanobot::SimpleAction
{
public:
    BridgeAttach(const Settings& settings, MessageConsumer* parent);
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
class BridgeAttachChannel : public melanobot::SimpleAction
{
public:
    BridgeAttachChannel(const Settings& settings, MessageConsumer* parent);
    void initialize() override;

protected:
    bool on_handle(network::Message& msg) override;

    Bridge* parent{nullptr}; ///< Bridge object to apply the attachment to
};

/**
 * \brief Base class for JoinMessage and similar
 */
class EventMessageBase: public melanobot::Handler
{
public:
    EventMessageBase(network::Message::Type type,
                     const Settings& settings,
                     ::MessageConsumer* parent)
        : Handler(settings, parent), type(type)
    {
        message = read_string(settings, "message", "");
        action  = settings.get("action", action);
        discard_self = settings.get("discard_self", discard_self);
        discard_others = settings.get("discard_others", discard_others);
        if ( message.empty() || (discard_others && discard_self) )
            throw melanobot::ConfigurationError();

        int timeout_seconds = settings.get("timeout", 0);
        if ( timeout_seconds > 0 )
            timeout = std::chrono::duration_cast<network::Duration>(
                std::chrono::seconds(timeout_seconds) );
    }

    bool can_handle(const network::Message& msg) const override
    {
        if ( msg.type != type )
            return false;
        if ( discard_others && !involves_self(msg) )
            return false;
        if ( discard_self && involves_self(msg) )
            return false;
        return true;
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        auto from = msg.source->decode(msg.from.name);
        reply_to(msg, network::OutputMessage(
            message.replaced(message_replacements(msg)),
            action,
            reply_channel(msg),
            priority,
            action ? std::move(from) : string::FormattedString(),
            {},
            timeout == network::Duration::zero() ?
                network::Time::max() :
                network::Clock::now() + timeout
        ));
        return true;
    }

    virtual string::FormattedProperties message_replacements(const network::Message& msg) const
    {
        auto props = msg.source->pretty_properties(msg.from);
        props.insert({
            {"channel", melanolib::string::implode(", ", msg.channels)},
            {"message", msg.source->decode(msg.message)}
        });
        return props;
    }

private:
    /**
     * \brief Checks if the message involves the bot's user
     */
    bool involves_self(const network::Message& msg) const
    {
        return msg.from.name == msg.source->name() || (
            !(msg.victim.local_id.empty() && msg.victim.name.empty()) &&
            msg.victim.name == msg.source->name() );
    }

    network::Message::Type type;///< Type of messages to be handled
    string::FormattedString message;        ///< Message to send
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
    JoinMessage(const Settings& settings, ::MessageConsumer* parent)
        : EventMessageBase(network::Message::JOIN, settings, parent)
    {}
};

/**
 * \brief Prints a message when a user parts a channel (or server)
 */
class PartMessage: public EventMessageBase
{
public:
    PartMessage(const Settings& settings, ::MessageConsumer* parent)
        : EventMessageBase(network::Message::PART, settings, parent)
    {}
};
/**
 * \brief Prints a message when a user is kicked from a channel
 */
class KickMessage: public EventMessageBase
{
public:
    KickMessage(const Settings& settings, ::MessageConsumer* parent)
        : EventMessageBase(network::Message::KICK, settings, parent)
    {}

protected:
    string::FormattedProperties message_replacements(const network::Message& msg) const override
    {
        auto props = msg.source->pretty_properties(msg.from);
        props.insert({
            {"channel", melanolib::string::implode(", ", msg.channels)},
            {"message", msg.source->decode(msg.message)},

            {"kicker", msg.source->decode(msg.from.name)},
            {"kicker.host", msg.from.host},
            {"kicker.global_id", msg.from.global_id},
            {"kicker.local_id", msg.from.local_id},

            {"kicked", msg.source->decode(msg.victim.name)},
            {"kicked.host", msg.victim.host},
            {"kicked.global_id", msg.victim.global_id},
            {"kicked.local_id", msg.victim.local_id}
        });
        return props;
    }
};

/**
 * \brief Prints a message when a user changes name
 * \note $name expands to the old name, $message to the new one
 */
class RenameMessage: public EventMessageBase
{
public:
    RenameMessage(const Settings& settings, ::MessageConsumer* parent)
        : EventMessageBase(network::Message::RENAME, settings, parent)
    {}
};

} // namespace core
#endif // HANDLER_BRIDGE_HPP
