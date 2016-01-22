/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2016 Mattia Basaglia
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
#include "storage.hpp"
#include "melanobot.hpp"

namespace timer {


bool Remind::schedule_reply(const network::Message& src)
{
    std::stringstream stream(src.message);

    std::string to;
    stream >> to;

    if ( to.empty() || !stream )
        return false;

    if ( melanolib::string::icase_equal(to, "me") )
        to = src.from.name;

    melanolib::time::TimeParser parser(stream);
    auto date_time = parser.parse_time_point();

    std::string message = parser.get_remainder();
    if ( message.empty() )
        return false;
    message = melanolib::string::replace(reply, replacements(src, to, message), "%");

    schedule_item({
        message,
        src.destination->config_name(),
        reply_channel(src),
        date_time
    });

    return true;
}


Properties Remind::replacements(const network::Message& src,
                                const std::string& to,
                                const std::string& message) const
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

void Remind::load_items()
{
    {
        Lock lock(mutex);
        items.clear();
    }
    
    if ( !storage::has_storage() )
        return;

    std::string path = "remind."+storage_id;

    unsigned count = melanolib::string::to_uint(
        storage::storage().maybe_get_value(path+".count", "0")
    );

    if ( count > 0 )
    {
        for ( unsigned i = 0; i < count; i++ )
        {
            auto map = storage::storage().get_map(path+"."+melanolib::string::to_string(i));
            schedule_item({
                map["message"],
                map["connection"],
                map["target"],
                melanolib::time::parse_time(map["timeout"])
            });
        }
    }

    storage::storage().erase(path);
}

void Remind::store_items()
{
    Lock lock(mutex);
    std::string path = "remind."+storage_id;

    storage::storage().put(path, "count", melanolib::string::to_string(items.size()));

    unsigned i = 0;
    for ( const auto& item : items )
    {
        storage::StorageBase::table map{
            {"message", item.message},
            {"connection", item.connection},
            {"target", item.target},
            {"timeout", melanolib::time::format_char(item.timeout, 'c')}
        };
        storage::storage().put(path+"."+melanolib::string::to_string(i), map);
        i++;
    }
}

void Remind::schedule_item(const Item& item)
{
    network::Time time = melanolib::time::time_point_convert<network::Time>(
        item.timeout.time_point()
    );

    Lock lock(mutex);
    items.push_back(item);
    auto iterator = items.end();
    --iterator;
    lock.unlock();

    TimerQueue::instance().push(
        time,
        [this, item, iterator]() {
            network::Connection* destination = Melanobot::instance().connection(item.connection);
            if ( !destination )
                return;
            string::FormatterConfig fmt;
            network::OutputMessage out {
                fmt.decode(item.message),
                false,
                item.target,
                priority
            };
            deliver(destination, out);
            Lock lock(mutex);
            items.erase(iterator);
        },
        this
    );
}


} // namespace timer
