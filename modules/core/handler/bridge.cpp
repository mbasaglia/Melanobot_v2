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

#include "bridge.hpp"

namespace handler {

Bridge::Bridge(const Settings& settings, handler::HandlerContainer* parent)
    : Group(settings,parent)
{

    std::string destination_name = settings.get("destination","");
    if ( !destination_name.empty() )
        destination = bot->connection(destination_name);
    dst_channel = settings.get_optional<std::string>("dst_channel");
}

bool Bridge::on_handle(network::Message& msg)
{
    network::Message targeted = msg;
    if ( destination )
        targeted.destination = destination;
    return Group::on_handle(targeted);
}

void Bridge::attach(network::Connection* connection)
{
    destination = connection;
    if ( connection )
        Log("sys",'!',3) << "Bridge attached to "
            << color::green << connection->description()
            << color::nocolor << " using protocol "
            << color::white << connection->protocol();
    else
        Log("sys",'!',3) << "Bridge detached";
}

void Bridge::attach_channel(Optional<std::string> channel)
{
    dst_channel = channel;
    if ( channel )
        Log("sys",'!',3) << "Bridge attached to channel "
            << color::dark_green << *channel;
    else
        Log("sys",'!',3) << "Bridge detached from channel";

}

BridgeChat::BridgeChat(const Settings& settings, handler::HandlerContainer* parent)
    : Handler(settings,parent)
{
    int timeout_seconds = settings.get("timeout",0);
    if ( timeout_seconds > 0 )
        timeout = std::chrono::duration_cast<network::Duration>(
            std::chrono::seconds(timeout_seconds) );

    ignore_self = settings.get("ignore_self",ignore_self);

    from = settings.get_optional<std::string>("from");
}

bool BridgeChat::can_handle(const network::Message& msg) const
{
    return msg.type == network::Message::CHAT && !msg.direct &&
        (!ignore_self || msg.from.name != msg.source->name());
}

bool BridgeChat::on_handle(network::Message& msg)
{
    reply_to(msg, network::OutputMessage(
        msg.source->decode(msg.message),
        msg.type == network::Message::ACTION,
        {},
        priority,
        msg.source->decode(from ? *from : msg.from.name),
        {},
        timeout == network::Duration::zero() ?
            network::Time::max() :
            network::Clock::now() + timeout
    ));
    return true;
}

BridgeAttach::BridgeAttach(const Settings& settings, handler::HandlerContainer* parent)
    : SimpleAction("attach",settings,parent)
{
    protocol = settings.get("protocol",protocol); /// \todo unused?
    detach = settings.get("detach",detach);
}

void BridgeAttach::initialize()
{
    // This ensures we aren't in Group constructor when called
    if ( !(this->parent = get_parent<Bridge>()) )
        throw ConfigurationError{};
}

bool BridgeAttach::on_handle(network::Message& msg)
{
    auto conn = bot->connection(msg.message);
    if ( conn || detach )
        parent->attach(conn);
    else
        ErrorLog("sys") << "Trying to detach a bridge";
    return true;
}

BridgeAttachChannel::BridgeAttachChannel(const Settings& settings,
                                         handler::HandlerContainer* parent)
    : SimpleAction("channel",settings,parent)
{}

void BridgeAttachChannel::initialize()
{
    // This ensures we aren't in Group constructor when called
    if ( !(this->parent = get_parent<Bridge>()) )
        throw ConfigurationError{};
}

bool BridgeAttachChannel::on_handle(network::Message& msg)
{
    if ( msg.message.empty() )
        parent->attach_channel({});
    else
        parent->attach_channel(msg.message);

    return true;
}

} // namespace handler
