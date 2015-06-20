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
#ifndef LISTS_HANDLERS_HPP
#define LISTS_HANDLERS_HPP

#include "core/handler/group.hpp"
#include "network/async_service.hpp"
#include "time/time_string.hpp"

/**
 * \brief Namespace for handlers managing lists
 */
namespace lists {

/**
 * \brief Simple manager for a fixed list
 */
class FixedList : public core::AbstractList
{
public:
    FixedList(const Settings& settings, MessageConsumer* parent)
        : AbstractList("list", true, settings, parent),
        list_id("lists."+trigger)
    {
        network::require_service("storage");
    }

    bool add(const std::string& element) override
    {
        network::service("storage")->query({"append", list_id, {element}});
        return true;
    }

    std::vector<std::string> elements() const override
    {
        return string::char_split(raw_get(),'\n');
    }

    bool remove(const std::string& element) override
    {
        auto list = elements();
        auto it = std::remove(list.begin(), list.end(), element);
        if ( it == list.end() )
            return false;
        list.erase(it, list.end());
        raw_set(string::implode("\n",list));
        return true;
    }

    bool clear() override
    {
        raw_set("");
        return true;
    }

    std::string get_property(const std::string& name) const override
    {
        if ( name == "list_name" )
            return trigger;
        return AbstractList::get_property(name);
    }

protected:
    /**
     * \brief Get the raw value of the list as from storage
     */
    std::string raw_get() const
    {
        return network::service("storage")
            ->query({"maybe_get", list_id, {""}}).contents;
    }

    /**
     * \brief Sets the raw value (elements separated by tabs)
     */
    void raw_set(const std::string& element_string) const
    {
        network::service("storage")
            ->query({"put", list_id, {element_string}});
    }


private:
    std::string list_id; ///< List name in the storage system
};

/**
 * \brief Manages replies for a DynamicReply
 */
class DynamicReplyManager : public handler::SimpleAction
{
public:
    DynamicReplyManager(const Settings& settings, MessageConsumer* parent)
        : SimpleAction("reply", settings, parent)
    {
        network::require_service("storage");
        list_id = "lists."+settings.get("list","dynamic_reply");

        std::string separator = settings.get("separator","->");
        regex_reply = std::regex("(.+)\\s+"+string::regex_escape(separator)+"\\s+(.+)",
                                 std::regex::ECMAScript|std::regex::optimize);

        help = "Adds a dynamic reply";
        synopsis += "trigger "+separator+" reply";
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        std::smatch match;
        if ( std::regex_match(msg.message, match, regex_reply) )
        {
            std::string record = std::string(match[1])+'\r'+std::string(match[2]);
            network::service("storage")->query({"append", list_id, {record}});
            network::service("storage")->query({"put", list_id+".last_updated",
                {timer::format_char(timer::DateTime(),'c')}});
            reply_to(msg, "Added the given reply");
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
class DynamicReply : public handler::Handler
{
public:
    DynamicReply(const Settings& settings, MessageConsumer* parent)
        : Handler(settings, parent)
    {
        network::require_service("storage");
        list_id = "lists."+settings.get("list","dynamic_reply");
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
        auto records = string::char_split(
            network::service("storage")->query({"maybe_get", list_id, {""}}).contents,
            '\n'
        );
        replies.clear();
        for ( const auto& record : records )
        {
            auto sep = record.find('\r');
            if ( sep != std::string::npos && sep+1 < record.size() )
                replies[record.substr(0,sep)] = record.substr(sep+1);
        }
    }

    std::string timestamp()
    {
        return network::service("storage")
            ->query({"maybe_get", list_id+".last_updated", {""}}).contents;
    }

    std::string list_id;        ///< List name in the storage system
    std::unordered_map<std::string,std::string> replies; ///< Trigger/reply map
    std::string last_updated;   ///< Timestamp for load_replies

};

} // namespace lists
#endif // LISTS_HANDLERS_HPP
