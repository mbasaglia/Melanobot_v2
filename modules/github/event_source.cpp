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

#include "event_source.hpp"
#include "melanolib/time/time_string.hpp"
#include "melanobot/storage.hpp"
#include "httpony/formats/json.hpp"
#include "github-controller.hpp"

namespace github {


EventSource::EventSource(const std::string& name, const PropertyTree& settings)
    : PushReceiver(name, settings, name)
{
    polling_ = settings.get("polling", false);
    if ( melanobot::has_storage() )
    {
        etag_ = melanobot::storage().maybe_get_value(storage_path("etag"), "");
        auto last_poll = melanobot::storage().maybe_get_value(storage_path("last_poll"), "");
        if ( !last_poll.empty() )
            last_poll_ = melanolib::time::parse_time(last_poll).time_point();
        else
            last_poll_ = melanolib::time::DateTime().time_point();
    }
}

std::string EventSource::storage_path(const std::string& suffix) const
{
    return "github." + name() + "." + suffix;
}

void EventSource::add_listener(const std::string& event_type, const ListenerFunctor& listener)
{
    if ( event_type.empty() )
        return;

    listeners_.push_back({event_type, listener});
}


std::string EventSource::api_path() const
{
    return "/" + name() + "/events";
}

void EventSource::poll_events(const GitHubController& source)
{
    if ( !polling_ )
        return;

    auto request = source.request(api_path());
    {
        auto lock = make_lock(mutex);
        if ( !etag_.empty() )
            request.headers["If-None-Match"] = etag_;
        current_poll_ = PollTime::clock::now();
    }

    web::HttpClient::instance().async_query(
        std::move(request),
        [this](web::Request& request, web::Response& response) {
            /// \todo Handle X-Poll-Interval (and rate limit stuff)
            auto it = response.headers.find("ETag");
            if ( it != response.headers.end() )
            {
                auto lock = make_lock(mutex);
                etag_ = it->second;
                if ( melanobot::has_storage() )
                    melanobot::storage().put(storage_path("etag"), etag_);
            }
            if ( !response.status.is_error() )
                dispatch_events(response.body.input());
        }
    );
}

void EventSource::dispatch_events(httpony::io::InputContentStream& body)
{
    PropertyTree content;
    try
    {
        /// \todo Figure why without the extra reference, it fails to read all of the data
        auto& stream = body;
        content = httpony::json::JsonParser().parse(stream);
    }
    catch(httpony::json::JsonError&)
    {
        ErrorLog("github") << "Malformed event data";
        return;
    }

    melanolib::time::DateTime::Time poll_time;
    if ( polling_ )
    {
        auto lock = make_lock(mutex);
        poll_time = last_poll_;
        last_poll_ = current_poll_;
        if ( melanobot::has_storage() )
            melanobot::storage().put(
                storage_path("last_poll"),
                melanolib::time::format_char(last_poll_, 'c')
            );
    }

    std::map<std::string, PropertyTree> events;
    for ( const auto& event : content )
    {
        auto type = event.second.get("type", "");
        if ( !type.empty() )
        {
            if ( polling_ )
            {
                auto time = melanolib::time::parse_time(event.second.get("created_at", "")).time_point();
                if ( time <= poll_time )
                    continue;
            }
            events[type].add_child("event", std::move(event.second));
        }
    }

    for ( const auto& listener : listeners_ )
    {
        auto it = events.find(listener.event_type);
        if ( it != events.end() )
            listener.callback(it->second);
    }
}

web::Response EventSource::receive_push(const RequestItem& request)
{
    process_events(request.request.body.input());
    return web::Response();
}

} // namespace github
