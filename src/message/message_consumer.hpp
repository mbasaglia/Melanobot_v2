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
#ifndef MESSAGE_CONSUMER_HPP
#define MESSAGE_CONSUMER_HPP

#include "settings.hpp"
#include "network/connection.hpp"

namespace melanobot {
    class Handler;
} // namespace melanobot

/**
 * \brief Base for classes which consume input messages
 *        (and might produce output messages)
 */
class MessageConsumer
{
public:
    explicit MessageConsumer(MessageConsumer* parent) : parent(parent){}
    virtual ~MessageConsumer() {}

    /**
     * \brief Populates \c output from properties of its children
     */
    virtual void populate_properties(const std::vector<std::string>& properties, PropertyTree& output) const = 0;

    /**
     * \brief Attempts to handle the message
     * \pre msg.source and msg.destination not null
     * \return \b true if the message has been handled and needs no further processing
     */
    virtual bool handle(network::Message& msg) = 0;

    /**
     * \brief Adds a child handler
     * \pre \c handler points to a valid object (ie: not nullptr)
     */
    virtual void add_handler(std::unique_ptr<melanobot::Handler>&& handler) = 0;

protected:
    /**
     * \brief Find the parent with the given type
     * \tparam ConsumerT Type (derived from MessageConsumer) to look for
     *
     * Walks up the tree to find a parent with the given type
     * \return The found parent or \b nullptr if not found
     */
    template<class ConsumerT>
        ConsumerT* get_parent() const
        {
            static_assert(std::is_base_of<MessageConsumer,ConsumerT>::value,
                          "Expected MessageConsumer type");
            if ( !parent )
                return nullptr;
            if ( auto p = dynamic_cast<ConsumerT*>(parent) )
                return p;
            return parent->get_parent<ConsumerT>();
        }

    /**
     * \brief Delivers a message to the destination applying filters of all the parents
     */
    void deliver(network::Connection* destination,
                 network::OutputMessage& output) const
    {
        output_filter(output);
        if ( parent )
            parent->deliver(destination, output);
        else
            destination->say(output);
    }

    /**
     * \brief Filters an output message
     */
    virtual void output_filter(network::OutputMessage& output) const {}

    /**
     * \brief Pointer to a handler containing this one
     */
    MessageConsumer* parent{nullptr};

};

#endif // MESSAGE_CONSUMER_HPP
