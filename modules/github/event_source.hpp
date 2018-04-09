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
#ifndef MELANOBOT_MODULE_GITHUB_EVENT_SOURCE_HPP
#define MELANOBOT_MODULE_GITHUB_EVENT_SOURCE_HPP


#include "web/client/http.hpp"
#include "melanolib/time/date_time.hpp"
#include "web/server/push_pages.hpp"

namespace github {

class EventSource : public web::PushReceiver
{
public:

    using ListenerFunctor = std::function<void(const PropertyTree&)>;
    using PollTime = melanolib::time::DateTime::Time;

private:
    struct SourceListener
    {
        std::string event_type;
        ListenerFunctor callback;
    };

public:
    EventSource(const std::string& name, const PropertyTree& settings);

    EventSource(EventSource&& src)
        : PushReceiver(std::move(src)),
          etag_(std::move(src.etag_)),
          listeners_(std::move(src.listeners_)),
          last_poll_(std::move(src.last_poll_)),
          current_poll_(std::move(src.current_poll_))
    {}

    EventSource& operator=(EventSource&& src)
    {
        name_ = std::move(src.name_);
        etag_ = std::move(src.etag_);
        listeners_ = std::move(src.listeners_);
        last_poll_ = std::move(src.last_poll_);
        current_poll_ = std::move(src.current_poll_);
        return *this;
    }

    void add_listener(const std::string& event_type, const ListenerFunctor& listener);

    void poll_events(const class GitHubController& source);

    /**
     * \brief Path component of a full API event url
     */
    std::string api_path() const;

protected:
    web::Response receive_push(const RequestItem& request) override;

private:
    void dispatch_events(httpony::io::InputContentStream& body);

    std::string storage_path(const std::string& suffix) const;

    std::string etag_;
    std::vector<SourceListener> listeners_;
    PollTime last_poll_;
    PollTime current_poll_;
    bool polling_ = false;
    std::mutex mutex;
};


} // namespace github
#endif // MELANOBOT_MODULE_GITHUB_EVENT_SOURCE_HPP
