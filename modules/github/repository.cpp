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
        auto poll_str = melanobot::storage().maybe_get_value("github."+name_+".last_poll", "");
        if ( !poll_str.empty() )
            last_poll_ = melanolib::time::parse_time(poll_str);

        etag_ = melanobot::storage().maybe_get_value("github."+name_+".etag", "");
    }
}

const std::string& Repository::name() const
{
    return name_;
}

void Repository::add_listener(EventType event_type, const ListenerFunctor& listener)
{
    if ( !event_type )
        return;

    event_types_ |= event_type;
    listeners_.push_back({event_type, listener});
}

void Repository::poll_events(const std::string& api_url)
{
    std::string url = api_url + "/repos/" + name_;
    if ( event_types_ & Issues )
    {
        auto since = melanolib::time::format(last_poll_, "Y-m-d\\TH:i:s\\z");
        last_poll_ = melanolib::time::DateTime();
        web::HttpService::instance().async_query(
            web::Request().get(url+"/issues").set_param("since", since),
            [this](const web::Response& response) {
                dispatch_event(Issues, response);
            }
        );

        if ( melanobot::has_storage() )
        {
            melanobot::storage().put(
                "github."+name_+".last_poll",
                melanolib::time::format_char(last_poll_, 'c')
            );
        }
    }

    if ( event_types_ & Events )
    {
        web::HttpService::instance().async_query(
            web::Request().get(url+"/events").set_header("ETag", etag_),
            [this](const web::Response& response) {
                auto it = response.headers.find("ETag");
                if ( it != response.headers.end() )
                {
                    etag_ = it->second;
                    melanobot::storage().put("github."+name_+".etag", etag_);
                }
                dispatch_event(Events, response);
            }
        );
    }
}

void Repository::dispatch_event(EventType event_type, const web::Response& response) const
{
    if ( !response.success() )
        return;

    JsonParser parser;
    parser.throws(false);
    auto json = parser.parse_string(response.contents, response.resource);
    if ( parser.error() )
        return;

    for ( const auto& listener : listeners_ )
    {
        if ( listener.event_type == event_type )
            listener.callback(json);
    }
}

} // namespace github
