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

#include "repository.hpp"
#include "melanolib/time/time_string.hpp"
#include "melanobot/storage.hpp"
#include "string/json.hpp"

namespace github {


Repository::Repository(std::string name)
    : name_(std::move(name))
{
    if ( melanobot::has_storage() )
    {
        etag_ = melanobot::storage().maybe_get_value("github."+name_+".etag", "");
        auto last_poll = melanobot::storage().maybe_get_value("github."+name_+".last_poll", "");
        if ( !last_poll.empty() )
            last_poll_ = melanolib::time::parse_time(last_poll).time_point();
    }
}

const std::string& Repository::name() const
{
    return name_;
}

void Repository::add_listener(const std::string& event_type, const ListenerFunctor& listener)
{
    if ( event_type.empty() )
        return;

    listeners_.push_back({event_type, listener});
}

void Repository::poll_events(const std::string& api_url)
{
    std::string url = api_url + "/repos/" + name_;

    auto request = web::Request().get(url+"/events");
    {
        Lock lock(mutex);
        if ( !etag_.empty() )
            request.set_header("If-None-Match", etag_);
        current_poll_ = PollTime::clock::now();
    }

    web::HttpService::instance().async_query(
        request,
        [this](const web::Response& response) {
            /// \todo Handle X-Poll-Interval
            auto it = response.headers.find("ETag");
            if ( it != response.headers.end() )
            {
                Lock lock(mutex);
                etag_ = it->second;
                if ( melanobot::has_storage() )
                    melanobot::storage().put("github."+name_+".etag", etag_);
            }
            dispatch_events(response);
        }
    );
}

void Repository::dispatch_events(const web::Response& response)
{
    if ( !response.success() )
        return;

    JsonParser parser;
    parser.throws(false);
    auto json = parser.parse_string(response.contents, response.resource);
    if ( parser.error() )
        return;

    melanolib::time::DateTime::Time poll_time;
    {
        Lock lock(mutex);
        poll_time = last_poll_;
        last_poll_ = current_poll_;
        if ( melanobot::has_storage() )
            melanobot::storage().put(
                "github."+name_+".last_poll",
                melanolib::time::format_char(last_poll_, 'c')
            );
    }

    std::map<std::string, PropertyTree> events;
    for ( const auto& event : json )
    {
        auto type = event.second.get("type", "");
        auto time = melanolib::time::parse_time(event.second.get("created_at", "")).time_point();
        if ( !type.empty() && time > poll_time )
            events[type].add_child("event", std::move(event.second));
    }

    for ( const auto& listener : listeners_ )
    {
        auto it = events.find(listener.event_type);
        if ( it != events.end() )
            listener.callback(it->second);
    }
}

} // namespace github
