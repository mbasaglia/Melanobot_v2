/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015-2017 Mattia Basaglia
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

#include "handlers.hpp"
#include "melanobot/storage.hpp"
#include "melanobot/melanobot.hpp"
#include "melanolib/time/time_parser.hpp"

namespace timer {

bool Remind::on_handle(network::Message& msg)
{
    std::stringstream stream(msg.message);
    std::string to;

    if ( stream >> to )
    {
        if ( melanolib::string::icase_equal(to, "me") )
            to = msg.from.name;

        /// \todo Reject invalid time formats
        melanolib::time::TimeParser parser(stream);
        auto date_time = parser.parse_time_point();

        std::string message = parser.get_remainder();
        if ( !message.empty() )
        {
            auto reply_replacements = replacements(msg, to, message, date_time);
            schedule_item({
                reply.replaced(reply_replacements),
                msg.destination->config_name(),
                reply_channel(msg),
                date_time
            });

            reply_to(msg, reply_ok.replaced(reply_replacements));
            return true;
        }
    }

    reply_to(msg, reply_no);
    return true;
}


string::FormattedProperties Remind::replacements(
    const network::Message& src,
    const std::string& to,
    const std::string& message,
    const melanolib::time::DateTime& date_time
) const
{
    string::FormattedProperties props = src.destination->pretty_properties();
    props.insert({
        {"channel", melanolib::string::implode(", ", src.channels)},
        {"message", src.source->decode(message)},

        {"from", src.source->decode(src.from.name)},
        {"from.host", src.from.host},
        {"from.global_id", src.from.global_id},
        {"from.local_id", src.from.local_id},

        {"to", src.source->decode(to)},

        {"date", melanolib::time::format_char(date_time, 'c')},
    });
    return props;
}

void Remind::load_items()
{
    {
        auto lock = make_lock(mutex);
        items.clear();
    }
    
    if ( !melanobot::has_storage() )
        return;

    std::string path = "remind."+storage_id;

    unsigned count = melanolib::string::to_uint(
        melanobot::storage().maybe_get_value(path+".count", "0")
    );

    if ( count > 0 )
    {
        string::FormatterConfig fmt;
        for ( unsigned i = 0; i < count; i++ )
        {
            auto map = melanobot::storage().get_map(path+"."+melanolib::string::to_string(i));
            schedule_item({
                fmt.decode(map["message"]),
                map["connection"],
                map["target"],
                melanolib::time::parse_time(map["timeout"])
            });
        }
    }

    melanobot::storage().erase(path);
}

void Remind::store_items()
{
    auto lock = make_lock(mutex);
    std::string path = "remind."+storage_id;

    melanobot::storage().put(path, "count", melanolib::string::to_string(items.size()));

    string::FormatterConfig fmt;

    unsigned i = 0;
    for ( const auto& item : items )
    {
        melanobot::StorageBase::table map{
            {"message", item.message.encode(fmt)},
            {"connection", item.connection},
            {"target", item.target},
            {"timeout", melanolib::time::format_char(item.timeout, 'c')}
        };
        melanobot::storage().put(path+"."+melanolib::string::to_string(i), map);
        i++;
    }
}

void Remind::schedule_item(const Item& item)
{
    network::Time time = melanolib::time::time_point_convert<network::Time>(
        item.timeout.time_point()
    );

    auto lock = make_lock(mutex);
    items.push_back(item);
    auto iterator = items.end();
    --iterator;
    lock.unlock();

    TimerQueue::instance().push(
        time,
        [this, item, iterator]() {
            network::Connection* destination = melanobot::Melanobot::instance().connection(item.connection);
            if ( !destination )
                return;
            network::OutputMessage out {
                item.message,
                false,
                item.target,
                priority
            };
            deliver(destination, out);
            auto lock = make_lock(mutex);
            items.erase(iterator);
        },
        this
    );
}

bool Defer::on_handle(network::Message& src)
{
    auto msg = src;

    std::stringstream stream(msg.message);
    melanolib::time::TimeParser parser(stream);
    auto date_time = parser.parse_time_point();

    msg.message = parser.get_remainder();
    if ( msg.message.empty() )
    {
        reply_to(msg, reply_no);
        return true;
    }

    network::Time time = melanolib::time::time_point_convert<network::Time>(
        date_time.time_point()
    );

    TimerQueue::instance().push(
        time,
        [msg](){melanobot::Melanobot::instance().message(msg);},
        this
    );


    reply_to(msg, melanolib::time::strftime(date_time, reply_ok));

    return true;
}

} // namespace timer
