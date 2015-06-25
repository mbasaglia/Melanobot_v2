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

#include "core/handler/group.hpp"
#include "storage_base.hpp"
#include "string/language.hpp"
#include "math.hpp"

namespace lists {

/**
 * \brief Used by \c InventoryManager to show the items in the inventory
 */
class InventoryList : public handler::SimpleAction
{
public:
    InventoryList(std::string list_id, const Settings& settings, MessageConsumer* parent)
        : SimpleAction("list","(?:list\\b)?\\s*", settings, parent),
          list_id(std::move(list_id))
    {
        help = "Shows the inventory";
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        auto elements = storage::storage().maybe_get_sequence(list_id);
        if ( elements.empty() )
            reply_to(msg,network::OutputMessage("is empty",true));
        else
            reply_to(msg,network::OutputMessage("has "+string::implode(", ",elements),true));
        return true;
    }

private:
    std::string list_id;        ///< List name in the storage system
};

/**
 * \brief Used by \c InventoryManager to remove all elements of the inventory
 */
class InventoryClear : public handler::SimpleAction
{
public:
    InventoryClear(std::string list_id, std::string auth,
                   const Settings& settings, MessageConsumer* parent)
    : SimpleAction("clear",settings,parent),
          list_id(std::move(list_id)),
          auth(std::move(auth))
    {
        help = "Removes all elements from the inventory";
    }

    bool can_handle(const network::Message& msg) const override
    {
        return handler::SimpleAction::can_handle(msg) &&
            ( auth.empty() || msg.source->user_auth(msg.from.local_id, auth) );
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        auto elements = storage::storage().maybe_get_sequence(list_id);
        if ( elements.empty() )
        {
            reply_to(msg,network::OutputMessage("was already empty",true));
        }
        else
        {
            elements.clear();
            storage::storage().put(list_id, elements);
            reply_to(msg,network::OutputMessage("is now empty",true));
        }
        return true;
    }

private:
    std::string list_id;        ///< List name in the storage system
    std::string auth;           ///< User group with the rights to use this handler
};

/**
 * \brief Shows the items in the inventory
 * \todo Handle string formatting in the inventory handlers
 */
class InventoryManager : public core::AbstractActionGroup
{
public:
    InventoryManager(const Settings& settings, MessageConsumer* parent)
        : AbstractActionGroup("inventory", settings, parent)
    {
        std::string list_id = "lists."+settings.get("list",trigger);
        help = "Shows the inventory";
        add_handler(New<InventoryClear>(list_id,settings.get("clear","admin"),Settings{},this));
        add_handler(New<InventoryList>(list_id,Settings{},this));
    }
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

        if ( item.empty() )
            return false;

        item = string::English().pronoun_to3rd(item,msg.from.name,msg.source->name());

        auto inventory = storage::storage().maybe_get_sequence(list_id);
        // Check that item isn't already in the inventory
        if ( std::find(inventory.begin(), inventory.end(), item) != inventory.end())
        {
            reply_to(msg,network::OutputMessage("already had "+item, true));
            return true;
        }

        std::string reply = "takes "+item;
        // Remove extra items
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
        reply_to(msg,network::OutputMessage(reply, true));
        return true;
    }

private:
    std::string list_id = "inventory";  ///< List name in the storage system
    std::string action = "gives";       ///< Action the user must perform
    unsigned    max_items = 6;          ///< If more than this many items are insterted, it will drop some items
};


/**
 * \brief Removes an item from the inventory via an action
 */
class InventoryTake : public handler::Handler
{
public:
    InventoryTake(const Settings& settings, MessageConsumer* parent)
        : Handler(settings, parent)
    {
        list_id = "lists."+settings.get("list",list_id);
        action = settings.get("action",action);
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

        if ( item.empty() )
            return false;

        item = string::English().pronoun_to3rd(item,msg.from.name,msg.source->name());
        auto inventory = storage::storage().maybe_get_sequence(list_id);

        auto iter = std::find(inventory.begin(), inventory.end(), item);
        if ( iter != inventory.end())
        {
            // swap + pop to avoid moving items around for no reason
            std::swap(*iter, inventory.back());
            inventory.pop_back();
            storage::storage().put(list_id, inventory);
            reply_to(msg,network::OutputMessage("gives "+msg.from.name+" "+item, true));
            return true;
        }
        reply_to(msg,network::OutputMessage("doesn't have "+item, true));
        return true;
    }

private:
    std::string list_id = "inventory";  ///< List name in the storage system
    std::string action = "takes from";  ///< Action the user must perform
};

} // namespace lists
#endif // LISTS_HANDLER_INVENTORY_HPP
