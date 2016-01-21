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
    }

    ~Remind()
    {
        TimerQueue::instance().remove(this);
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        if ( parse(msg) )
            reply_to(msg, reply_ok);
        else
            reply_to(msg, reply_no);
        return true;
    }

private:
    std::string reply_ok = "Got it!";
    std::string reply_no = "Forget it!";
    std::string reply = "<%from.name> %to, remember %message";

    bool parse(const network::Message& msg) const
    {
        std::stringstream stream(msg.message);

        std::string to;
        stream >> to;

        if ( to.empty() || !stream )
            return false;

        if ( melanolib::string::icase_equal(to, "me") )
            to = msg.from.name;

        melanolib::time::TimeParser parser(stream);
        network::Time time = melanolib::time::time_point_convert<network::Time>(
            parser.parse_time_point().time_point()
        );

        std::string message = parser.get_remainder();
        if ( message.empty() )
            return false;

        TimerQueue::instance().push(
            time,
            [this, to, src=msg, message]() {
                remind(src, to, message);
            },
            this
        );

        return true;
    }


    Properties replacements(const network::Message& src, const std::string& to, const std::string& message) const
    {
        string::FormatterConfig fmt;
        Properties props = src.destination->pretty_properties();
        props.insert({
            {"channel", melanolib::string::implode(", ", src.channels)},
            {"message", message},

            {"from", src.source->encode_to(src.from.name, fmt)},
            {"from.host", src.from.host},
            {"from.global_id", src.from.global_id},
            {"from.local_id", src.from.local_id},

            {"to", src.source->encode_to(to, fmt)},
        });
        return props;
    }

    void remind(const network::Message& src, const std::string& to, const std::string& msg) const
    {
        auto str = melanolib::string::replace(reply, replacements(src, to, msg), "%");
        reply_to(src, str);
    }
};

} // namespace timer
#endif // MELANOBOT_MODULES_TIMER_HANDLER_HPP
