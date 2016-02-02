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
#ifndef MELANOBOT_MODULE_GITHUB_REPOSITORY_HPP
#define MELANOBOT_MODULE_GITHUB_REPOSITORY_HPP


#include "web/http.hpp"
#include "melanolib/time/time.hpp"

namespace github {

class Repository
{
public:

    using ListenerFunctor = std::function<void(const PropertyTree&)>;
    using PollTime = melanolib::time::DateTime::Time;

private:
    struct RepoListener
    {
        std::string event_type;
        ListenerFunctor callback;
    };

public:
    Repository(std::string name);

    Repository(Repository&& repo)
        : name_(std::move(repo.name_)),
          etag_(std::move(repo.etag_)),
          listeners_(std::move(repo.listeners_))
    {}

    Repository& operator=(Repository&& repo)
    {
        name_ = std::move(repo.name_);
        etag_ = std::move(repo.etag_);
        listeners_ = std::move(repo.listeners_);
        return *this;
    }


    const std::string& name() const;

    void add_listener(const std::string& event_type, const ListenerFunctor& listener);

    void poll_events(const class GitHubEventSource& source);

private:
    void dispatch_events(const web::Response& response);

    std::string name_;
    std::string etag_;
    std::vector<RepoListener> listeners_;
    PollTime last_poll_;
    PollTime current_poll_;
    std::mutex mutex;
};


} // namespace github
#endif // MELANOBOT_MODULE_GITHUB_REPOSITORY_HPP
