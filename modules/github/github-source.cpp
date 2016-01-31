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

#include "github-source.hpp"

namespace github {

void GitHubEventSource::initialize(const Settings& settings)
{
    api_url = settings.get("api_url", api_url);
    if ( api_url.empty() )
        throw melanobot::ConfigurationError("Missing GitHub API URL");

    if ( api_url.back() == '/' )
        api_url.pop_back();

    for ( const auto& repo : settings )
    {
        if ( repo.first.find('/') == std::string::npos )
            continue;

        auto repo_iter = std::find_if(repositories.begin(), repositories.end(),
            [name=repo.first](const Repository& repo){
                return repo.name() == name;
            });

        if ( repo_iter == repositories.end() )
        {
            repositories.emplace_back(repo.first);
            repo_iter = repositories.end() - 1;
        }

        for ( const auto& listener : repo.second )
        {
            try
            {
                auto listen = ListenerFactory::instance().build(listener.first, listener.second);
                auto listener_ptr = listen.get();
                listeners.emplace_back(std::move(listen));
                repo_iter->add_listener(
                    listener_ptr->event_type(),
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
    }

    if ( repositories.empty() )
        throw melanobot::ConfigurationError("No repositories for github connection");

    timer = melanolib::time::Timer(
        [this]{poll();},
        std::chrono::minutes(settings.get("poll_interval", 10)),
        true
    );
}

GitHubEventSource::~GitHubEventSource()
{
    stop();
}


void GitHubEventSource::poll()
{
    for ( auto& repo : repositories )
    {
        repo.poll_events(api_url);
    }
}


void GitHubEventSource::stop()
{
    timer.stop();
}

void GitHubEventSource::start()
{
    poll();
    timer.start();
}

} // namespace github
