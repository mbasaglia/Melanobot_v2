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

#include "github-controller.hpp"

namespace github {

void GitHubController::initialize(const Settings& settings)
{
    api_url_ = settings.get("api_url", api_url_);
    if ( api_url_.empty() )
        throw melanobot::ConfigurationError("Missing GitHub API URL");

    if ( api_url_.back() == '/' )
        api_url_.pop_back();

    auth_.user = settings.get("username", "");
    auth_.password = settings.get("password", "");

    for ( const auto& source : settings )
    {
        if ( source.second.empty() || !source.second.data().empty() )
            continue;

        auto src_iter = std::find_if(sources.begin(), sources.end(),
            [&source](const EventSource& other){
                return source.first == other.name();
            });

        if ( src_iter == sources.end() )
        {
            sources.emplace_back(source.first);
            src_iter = sources.end() - 1;
        }

        for ( const auto& listener : source.second )
        {
            create_listener(*src_iter, listener);
        }
    }

    if ( sources.empty() )
        throw melanobot::ConfigurationError("No sources for github connection");

    timer = melanolib::time::Timer(
        [this]{poll();},
        std::chrono::minutes(settings.get("poll_interval", 10)),
        true
    );
}

void GitHubController::create_listener(EventSource& src, const Settings::value_type& listener, const Settings& extra)
{
    Settings settings = listener.second;
    ::settings::merge(settings, extra, false);

    if ( listener.first == "Group" )
    {
        for ( const auto& child : listener.second )
        {
            if ( child.second.empty() && !child.second.data().empty() )
                settings.put(child.first, child.second.data());
            else
                create_listener(src, child, settings);
        }
        return;
    }

    try
    {
        auto listen = ListenerFactory::instance().build(listener.first, settings);
        auto listener_ptr = listen.get();
        listeners.emplace_back(std::move(listen));
        for ( const auto& event_type : listener_ptr->event_types() )
            src.add_listener(
                event_type,
                [listener_ptr](const PropertyTree& json){
                    listener_ptr->handle_event(json);
                }
            );
    }
    catch ( const melanobot::MelanobotError& err )
    {
        ErrorLog("git") << err.what();
    }

}

GitHubController::~GitHubController()
{
    stop();
}


void GitHubController::poll()
{
    for ( auto& src : sources )
    {
        src.poll_events(*this);
    }
}


void GitHubController::stop()
{
    ControllerRegistry::instance().unregister_source(this);
    timer.stop();
}

void GitHubController::start()
{
    poll();
    timer.start();
    ControllerRegistry::instance().register_source(this);
}

bool GitHubController::running() const
{
    return timer.running();
}

web::Request GitHubController::request(const std::string& url) const
{
    auto request = web::Request("GET", api_url_+url);
    if ( !auth_.user.empty() && !auth_.password.empty() )
        request.auth = auth_;
    return request;
}

} // namespace github
