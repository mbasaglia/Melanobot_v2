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
    enum EventType
    {
        NoEvent = 0x00,
        Events  = 0x01,
        Issues  = 0x02,
    };

    using ListenerFunctor = std::function<void(const PropertyTree&)>;

private:
    struct RepoListener
    {
        EventType event_type;
        ListenerFunctor callback;
    };

public:
    Repository(std::string name);

    const std::string& name() const;

    void add_listener(EventType event_type, const ListenerFunctor& listener);

    void poll_events(const std::string& api_url);

private:
    void dispatch_event(EventType event_type, const web::Response& response) const;

    std::string name_;
    int event_types_ = NoEvent;
    std::string etag_;
    melanolib::time::DateTime last_poll_;
    std::vector<RepoListener> listeners_;
};


} // namespace github
#endif // MELANOBOT_MODULE_GITHUB_REPOSITORY_HPP
