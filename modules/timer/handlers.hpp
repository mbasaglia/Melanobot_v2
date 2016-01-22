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
#ifndef MELANOBOT_MODULES_TIMER_HANDLER_HPP
#define MELANOBOT_MODULES_TIMER_HANDLER_HPP

#include <list>
#include "handler/handler.hpp"
#include "timer-queue.hpp"
#include "melanolib/time/time_parser.hpp"

namespace timer {

class Remind : public handler::SimpleAction
{
public:
    Remind(const Settings& settings, MessageConsumer* parent)
        : SimpleAction("remind",settings,parent)
    {
        synopsis += "who time message...";
        help = "Sends a message at the given time";
        reply_ok = settings.get("reply_ok", reply_ok);
        reply_no = settings.get("reply_no", reply_no);
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

    struct Item
    {
        std::string message;
        std::string connection;
        std::string target;
        melanolib::time::DateTime timeout;
    };

    std::string reply_ok = "Got it!";
    std::string reply_no = "Forget it!";
    std::string reply = "<%from> %to, remember %message";
    std::string storage_id = "remind"; ///< ID used in storage
    std::list<Item> items;
    std::mutex mutex;

    void load_items();
    void store_items();
    void schedule_item(const Item& item);

    bool schedule_reply(const network::Message& msg);
    Properties replacements(const network::Message& src, const std::string& to, const std::string& message) const;

};

} // namespace timer
#endif // MELANOBOT_MODULES_TIMER_HANDLER_HPP
