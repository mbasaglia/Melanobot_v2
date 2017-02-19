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
#ifndef MELANOBOT_MODULE_GITHUB_CONTROLLER_HPP
#define MELANOBOT_MODULE_GITHUB_CONTROLLER_HPP

#include "network/async_service.hpp"
#include "github-listeners.hpp"
#include "event_source.hpp"


namespace github {

/**
 * \brief Single instance of a github connection
 */
class GitHubController : public AsyncService
{
public:
    explicit GitHubController(httpony::Auth auth = {}, std::string api_url = "https://api.github.com")
        : api_url_(std::move(api_url)), auth_(std::move(auth))
    {}

    void initialize(const Settings& settings) override;

    ~GitHubController();

    void stop() override;

    void start() override;

    bool running() const override;

    std::string name() const override
    {
        return "GitHub at " + api_url_;
    }

    /**
     * \brief Builds a request for the relative \p url
     */
    web::Request request(const std::string& url) const;

    const httpony::Auth& auth() const
    {
        return auth_;
    }

    const std::string& api_url() const
    {
        return api_url_;
    }

private:
    void poll();

    void create_listener(EventSource& src, const Settings::value_type& listener, const Settings& extra = {});

    std::vector<EventSource> sources;
    std::string api_url_;
    melanolib::time::Timer timer; ///< Polling timer
    std::vector<std::unique_ptr<GitHubEventListener>> listeners;
    httpony::Auth auth_;
};

/**
 * \brief Class that keeps track of \c GitHubEventSource objects
 */
class ControllerRegistry : public melanolib::Singleton<ControllerRegistry>
{
public:
    MaybePtr<const GitHubController> get_source(const httpony::Auth& auth, const std::string& api_url) const
    {
        for ( auto src : sources )
        {
            bool equal = src->auth().user == auth.user && src->auth().password == auth.password;
            if ( src->api_url() == api_url && (auth.empty() || equal) )
                return {src, false};
        }

        return MaybePtr<const GitHubController>::make(auth);
    }

    void register_source(const GitHubController* source)
    {
        auto it = std::find(sources.begin(), sources.end(), source);
        if ( it == sources.end() )
            sources.push_back(source);
    }

    void unregister_source(const GitHubController* source)
    {
        auto it = std::find(sources.begin(), sources.end(), source);
        if ( it != sources.end() )
        {
            if ( it != sources.end() -1 )
                std::swap(*it, sources.back());
            sources.pop_back();
        }
    }

private:
    ControllerRegistry(){}
    friend ParentSingleton;

    std::vector<const GitHubController*> sources;

};

} // namespace github
#endif // MELANOBOT_MODULE_GITHUB_CONTROLLER_HPP
