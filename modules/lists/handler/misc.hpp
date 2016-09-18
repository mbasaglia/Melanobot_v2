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
#ifndef LISTS_HANDLER_MISC_HPP
#define LISTS_HANDLER_MISC_HPP

#include "core/handler/group.hpp"
#include "melanobot/storage.hpp"
#include "melanolib/time/time_string.hpp"

/**
 * \brief Namespace for handlers managing lists
 */
namespace lists {

/**
 * \brief Simple manager for a fixed list
 * \todo Items with spaces should be a single element
 */
class FixedList : public core::AbstractList
{
public:
    FixedList(const Settings& settings, MessageConsumer* parent)
        : AbstractList("list", true, settings, parent),
        list_id("lists."+trigger)
    {}

    bool add(const std::string& element) override
    {
        melanobot::storage().append(list_id, element);
        return true;
    }

    std::vector<std::string> elements() const override
    {
        return melanobot::storage().maybe_get_sequence(list_id);
    }

    bool remove(const std::string& element) override
    {
        auto list = melanobot::storage().maybe_get_sequence(list_id);
        auto it = std::remove(list.begin(), list.end(), element);
        if ( it == list.end() )
            return false;
        list.erase(it, list.end());
        melanobot::storage().put(list_id, list);
        return true;
    }

    bool clear() override
    {
        melanobot::storage().erase(list_id);
        return true;
    }

    std::string get_property(const std::string& name) const override
    {
        if ( name == "list_name" )
            return trigger;
        return AbstractList::get_property(name);
    }

private:
    std::string list_id; ///< List name in the storage system
};

/**
 * \brief Manages replies for a DynamicReply
 */
class DynamicReplyManager : public melanobot::SimpleAction
{
public:
    DynamicReplyManager(const Settings& settings, MessageConsumer* parent)
        : SimpleAction("reply", settings, parent)
    {
        list_id = "lists."+settings.get("list", "dynamic_reply");

        std::string separator = settings.get("separator", "->");
        regex_reply = std::regex("(.+)\\s+"+melanolib::string::regex_escape(separator)+"(\\s+(.+))?",
                                 std::regex::ECMAScript|std::regex::optimize);

        help = "Adds a dynamic reply";
        synopsis += " trigger "+separator+" reply";
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        std::smatch match;
        if ( std::regex_match(msg.message, match, regex_reply) )
        {
            if ( match[2].matched )
            {
                melanobot::storage().put(list_id+".map", match[1], match[3]);
                reply_to(msg, "Added the given reply");
            }
            else
            {
                melanobot::storage().erase(list_id+".map", match[1]);
                reply_to(msg, "Removed the given reply");
            }
            melanobot::storage().put(list_id+".last_updated",
                                   melanolib::time::format_char(melanolib::time::DateTime(), 'c'));
        }
        else
        {
            reply_to(msg, "Wrong syntax");
        }
        return true;
    }

private:

    std::string list_id;        ///< List name in the storage system
    std::regex  regex_reply;    ///< Regex used to separate trigger from reply
};

/**
 * \brief Reports back a dynamic reply
 */
class DynamicReply : public melanobot::Handler
{
public:
    DynamicReply(const Settings& settings, MessageConsumer* parent)
        : Handler(settings, parent)
    {
        list_id = "lists."+settings.get("list", "dynamic_reply");
        load_replies();
    }

    bool can_handle(const network::Message& msg) const override
    {
        return msg.type == network::Message::CHAT;
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        if ( last_updated < timestamp() )
            load_replies();

        auto it = replies.find(msg.message);
        if ( it != replies.end() )
        {
            reply_to(msg, it->second);
            return true;
        }

        return false;
    }

private:
    /**
     * \brief Loads the list of replies from the storage system
     */
    void load_replies()
    {
        last_updated = timestamp();
        replies = melanobot::storage().maybe_get_map(list_id+".map");
    }

    std::string timestamp()
    {
        return melanobot::storage().maybe_get_value(list_id+".last_updated");
    }

    std::string list_id;        ///< List name in the storage system
    std::unordered_map<std::string, std::string> replies; ///< Trigger/reply map
    std::string last_updated;   ///< Timestamp for load_replies

};

} // namespace lists
#endif // LISTS_HANDLER_MISC_HPP
