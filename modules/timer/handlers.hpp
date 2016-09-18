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
#ifndef MELANOBOT_MODULES_TIMER_HANDLER_HPP
#define MELANOBOT_MODULES_TIMER_HANDLER_HPP

#include <list>
#include "melanobot/handler.hpp"
#include "timer-queue.hpp"

namespace timer {

/**
 * \brief Sends a message at the specified time
 */
class Remind : public melanobot::SimpleAction
{
public:
    Remind(const Settings& settings, MessageConsumer* parent)
        : SimpleAction("remind", settings, parent)
    {
        synopsis += "who time message...";
        help = "Sends a message at the given time";
        reply_ok = read_string(settings, "reply_ok", "Got it!");
        reply_no = read_string(settings, "reply_no", "Forget it!");
        reply = read_string(settings, "reply", "<$from> $to, remember $message");
        storage_id = settings.get("storage_id", storage_id);

        load_items();
    }

    ~Remind()
    {
        TimerQueue::instance().remove(this);
        store_items();
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        if ( schedule_reply(msg) )
            reply_to(msg, reply_ok);
        else
            reply_to(msg, reply_no);
        return true;
    }

private:

    /**
     * \brief Message information
     *
     * Useful to serialize the pending messages into storage and to
     * not depend on the connection object having continuous lifetime
     */
    struct Item
    {
        string::FormattedString message; ///< Message text (already formatted as for \p reply)
        std::string connection; ///< Name of the connection as from Melanobot
        std::string target;     ///< Message target (channel or user)
        melanolib::time::DateTime timeout; ///< Time at which the message should be delivered
    };

    string::FormattedString reply_ok;   ///< Reply acknowledging the message will be processed
    string::FormattedString reply_no;   ///< Reply given when a message has been discarded
    string::FormattedString reply;      ///< Message formatting
    std::string storage_id = "remind"; ///< ID used in storage
    std::list<Item> items; ///< List of stored messages
    std::mutex mutex; ///< Mutex guarding \p items

    /**
     * \brief Loads items from storage
     */
    void load_items();

    /**
     * \brief Saves items to storage
     */
    void store_items();

    /**
     * \brief Schedules an item into the timer service
     */
    void schedule_item(const Item& item);

    /**
     * \brief Extracts an Item from the message and schedules it
     * \returns \b false if the message cannot be processed
     */
    bool schedule_reply(const network::Message& msg);

    /**
     * \brief Returns the replacements used by \p reply
     */
    string::FormattedProperties replacements(const network::Message& src,
                                             const std::string& to,
                                             const std::string& message) const;

};

/**
 * \brief Defers a command
 */
class Defer : public melanobot::SimpleAction
{
public:
    Defer(const Settings& settings, MessageConsumer* parent)
        : SimpleAction("defer", settings, parent)
    {
        synopsis += "time command...";
        help = "Executes a command at the given time";
        reply_ok = settings.get("reply_ok", reply_ok);
        reply_no = settings.get("reply_no", reply_no);
    }

    ~Defer()
    {
        TimerQueue::instance().remove(this);
    }

protected:
    bool on_handle(network::Message& msg);

private:
    /// \todo this could use something like ${time:%c %e}
    std::string reply_ok = "Got it! (%c %e)";   ///< Reply acknowledging the message will be processed
    std::string reply_no = "Forget it!";///< Reply given when a message has been discarded
};

} // namespace timer
#endif // MELANOBOT_MODULES_TIMER_HANDLER_HPP
