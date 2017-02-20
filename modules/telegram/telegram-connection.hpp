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
#ifndef MELANOBOT_MODULES_TELEGRAM_CONNECTION_HPP
#define MELANOBOT_MODULES_TELEGRAM_CONNECTION_HPP

#include <functional>

#include "web/server/push_pages.hpp"
#include "network/connection.hpp"

namespace telegram{


class TelegramConnection : public network::AuthConnection, web::PushReceiver
{
public:
    using ApiCallback = std::function<void(PropertyTree)>;
    using ErrorCallback = std::function<void()>;

    static std::unique_ptr<TelegramConnection> create(
        const Settings& settings,
        const std::string& name
    );

    /**
     * \thread main \lock none
     */
    TelegramConnection(
        const std::string&  api_endpoint,
        const std::string&  token,
        const Settings&     settings = {},
        const std::string&  name = {}
    );

    ~TelegramConnection();

    /**
     * \thread external \lock none
     */
    network::Server server() const override;

    /**
     * \thread external \lock data
     */
    std::string description() const override;

    /**
     * \thread external \lock none
     */
    void command(network::Command cmd) override;


    /**
     * \thread external \lock none
     */
    void say(const network::OutputMessage& message) override;

    /**
     * \brief Sends a POST request to the api
     * \param method    API method
     * \param payload   Data to send
     * \param callback  Callback called after a response has been received
     * \param on_error  Callback called on network error
     * \thread external \lock none
     */
    void post(const std::string& method,
              const PropertyTree& payload,
              const ApiCallback& callback,
              const ErrorCallback& on_error = {}
    );

    /**
     * \brief Sends a Get request to the api
     * \thread external \lock none
     */
    void get(const std::string& method,
             const ApiCallback& callback,
             const ErrorCallback& on_error = {}
            );

    /**
     * \thread external \lock none
     */
    Status status() const override;

    /**
     * \thread external \lock none
     */
    std::string protocol() const override;

    /**
     * \thread external \lock data(asynchronously)
     */
    void connect() override;

    /**
     * \thread external \lock none
     */
    void disconnect(const string::FormattedString&) override;

    /**
     * \thread external \lock data(asynchronously)
     */
    void reconnect(const string::FormattedString& quit_message = {}) override;

    /**
     * \thread external \lock none
     */
    string::Formatter* formatter() const override
    {
        return formatter_;
    }

    /**
     * \thread external \lock data
     */
    std::string name() const override;

    /**
     * \thead external \lock data
     */
    LockedProperties properties() override;

    /**
     * \thead external \lock data
     */
    string::FormattedProperties pretty_properties() const;

    user::User build_user(const std::string& local_id) const override;

protected:
    /**
     * \thead external \lock none
     */
    web::Response receive_push(const RequestItem& request) override;

private:
    /**
     * \brief Callback used to just log error requests
     * \thead external \lock none
     */
    void log_errors(const PropertyTree& response) const;

    /**
     * \brief Sends a request to the api
     * \thead external \lock none
     */
    void request(web::Request&& request,
                 const ApiCallback& callback,
                 const ErrorCallback& on_error
    );

    /**
     * \thead external \lock data
     */
    std::string storage_key() const;

    /**
     * \brief Processes event payloads
     * \thead input \lock none
     */
    void process_events(httpony::io::InputContentStream& body);

    /**
     * \brief Processes a single event
     * \thead input \lock none
     */
    void process_event(PropertyTree& event);

    /**
     * \brief Returns the URI for the given API method
     * \thead external \lock none
     */
    web::Uri api_uri(const std::string& method) const;

    /**
     * \brief Returns a user from a json object
     * \thead external \lock none
     */
    user::User user_attributes(const PropertyTree& user) const;

    web::Uri api_base;
    PropertyTree properties_;

    /**
     * \brief I/O formatter
     */
    string::Formatter* formatter_ = nullptr;
    /**
     * \brief Connection status
     */
    AtomicStatus connection_status;
    /**
     * \brief User manager
     */
    user::UserManager user_manager;
    /**
     * \brief User authorization system
     */
    user::AuthSystem  auth_system;

    ApiCallback log_errors_callback;

// Webhook/Push
    /**
     * \brief Whether to use weeb hooks (otherwise polling)
     */
    bool webhook = true;

    /**
     * \brief URL given to telegram to send push requests to
     *
     * This should be end up sending requests to the PushPage associated
     * with this connection
     */
    std::string webhook_url;

    /**
     * \brief Maximum number of connections telegram can make to webhook_url
     */
    int webhook_max_connections = 1;

// Polling
    /**
     * \brief Polls the API for event updates
     */
    void poll();

    melanolib::time::Timer polling_timer;

    /**
     * \brief Last event read
     */
    std::atomic<uint64_t> event_id{0};
};

} // namespace telegram
#endif // MELANOBOT_MODULES_TELEGRAM_CONNECTION_HPP
