/**
 * \file
 *
 * \author Mattia Basaglia
 *
 * \copyright Copyright (C) 2017 Mattia Basaglia
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef MELANOBOT_MODULES_WEB_PUSH_PAGE_HPP
#define MELANOBOT_MODULES_WEB_PUSH_PAGE_HPP

#include "base_pages.hpp"

namespace web {

/**
 * \brief Base class for receivers of push notifications
 */
class PushReceiver
{
public:
    using RequestItem = WebPage::RequestItem;

    PushReceiver(
        const std::string& name,
        const Settings& settings,
        const std::string& default_uri
    ) : name(name)
    {
        uri = WebPage::read_uri(settings, default_uri);
        registry()[name] = this;
    }

    ~PushReceiver()
    {
        registry().erase(name);
    }

    bool matches(const RequestItem& request) const
    {
        return request.path.match_prefix(uri);
    }

    virtual web::Response receive_push(const RequestItem& request) = 0;

    /**
     * \brief Returns an instance from its name
     */
    static PushReceiver* get_from_name(const std::string& name)
    {
        auto iter = registry().find(name);
        return iter == registry().end() ? nullptr : iter->second;
    }

private:
    static std::map<std::string, PushReceiver*>& registry()
    {
        static std::map<std::string, PushReceiver*> instance;
        return instance;
    }

    std::string name;
    web::UriPath uri;
};

/**
 * \brief Finds dynamically a PushReceiver and forwards requests to it
 */
class PushPage : public WebPage
{
public:
    explicit PushPage(const Settings& settings)
    {
        receiver_name = settings.get("receiver", "");
        if ( receiver_name.empty() )
            throw melanobot::ConfigurationError("Missing receiver name");
    }

    bool matches(const RequestItem& request) const
    {
        if ( !get_receiver(true) )
            return false;
        return receiver->matches(request);
    }

    Response respond(const RequestItem& request) const override
    {
        if ( !get_receiver(false) )
            return Response(StatusCode::InternalServerError);
        return receiver->receive_push(request);
    }

private:
    PushReceiver* get_receiver(bool force) const
    {
        if ( force || !receiver )
        {
            receiver = PushReceiver::get_from_name(receiver_name);
            if ( !receiver )
                ErrorLog("web") << "Push receiver not found: " << receiver_name;
        }
        return receiver;
    }

    std::string receiver_name;
    mutable PushReceiver* receiver = nullptr;
};

} // namespace web
#endif // MELANOBOT_MODULES_WEB_PUSH_PAGE_HPP
