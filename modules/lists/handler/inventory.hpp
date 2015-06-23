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
#ifndef LISTS_HANDLER_INVENTORY_HPP
#define LISTS_HANDLER_INVENTORY_HPP

#include "handler/handler.hpp"
#include "storage_base.hpp"
#include "string/language.hpp"
#include "math.hpp"

namespace lists {

/**
 * \brief Shows the items in the inventory
 */
class InventoryList : public handler::SimpleAction
{
public:
    InventoryList(const Settings& settings, MessageConsumer* parent)
        : SimpleAction("inventory", settings, parent)
    {
        list_id = "lists."+settings.get("list",trigger);
        help = "Shows the inventory";
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        auto elements = storage::storage().maybe_get_sequence(list_id);
        if ( elements.empty() )
            reply_to(msg,network::OutputMessage(string::FormattedString("is empty"),true));
        else
            reply_to(msg,network::OutputMessage(string::FormattedString("has "+string::implode(", ",elements)),true));
        return true;
    }

private:
    std::string list_id;        ///< List name in the storage system
};

/**
 * \brief Adds an item to the inventory via an action
 */
class InventoryPut : public handler::Handler
{
public:
    InventoryPut(const Settings& settings, MessageConsumer* parent)
        : Handler(settings, parent)
    {
        list_id = "lists."+settings.get("list",list_id);
        action = settings.get("action",action);
        max_items = settings.get("max_items",max_items);
        /// \todo help and synopsis (as get_property)
    }

public:
    bool can_handle(const network::Message& message) const override
    {
        return message.type == network::Message::ACTION &&
            string::starts_with(
                string::strtolower(message.message),
                string::strtolower(action+' '+message.source->name()+' ')
            );
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        std::string item = msg.message.substr(action.size()+msg.source->name().size()+2);
        item = string::English().pronoun_to3rd(item,msg.from.name,msg.source->name());
        std::string reply = "takes "+item;

        auto inventory = storage::storage().maybe_get_sequence(list_id);
        if ( max_items > 0 && inventory.size() >= max_items )
        {
            std::vector<std::string> dropped(inventory.size()-max_items+1);
            for ( auto& drop : dropped )
            {
                int rand = math::random(inventory.size()-1);
                auto it = inventory.begin()+rand;
                drop = *it;
                // swap + pop to avoid moving items around for no reason
                std::swap(*it,inventory.back());
                inventory.pop_back();
            }
            reply += " and drops "+string::implode(", ",dropped);
        }
        
        inventory.push_back(item);
        storage::storage().put(list_id, inventory);
        reply_to(msg,network::OutputMessage(string::FormattedString(reply), true));
        return true;
    }

private:
    std::string list_id = "inventory";  ///< List name in the storage system
    std::string action = "gives";       ///< Action the user must perform
    unsigned    max_items = 6;          ///< If more than this many items are insterted, it will drop some items
};

} // namespace lists
#endif // LISTS_HANDLER_INVENTORY_HPP
