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
#ifndef MELANOBOT_MODULE_GITHUB_CONNECTION_HPP
#define MELANOBOT_MODULE_GITHUB_CONNECTION_HPP

#include "network/async_service.hpp"
#include "github-listeners.hpp"
#include "repository.hpp"


namespace github {

struct Auth
{
    std::string username; ///< Username for basic auth
    std::string password; ///< Password / OAuth personal access token

    bool empty() const
    {
        return username.empty() || password.empty();
    }

    bool operator==(const Auth& oth) const
    {
        return username == oth.username && password == oth.password;
    }
};

/**
 * \brief Single instance of a github connection
 */
class GitHubEventSource : public AsyncService
{
public:
    explicit GitHubEventSource(Auth auth = {}, std::string api_url = "https://api.github.com")
        : api_url_(std::move(api_url)), auth_(std::move(auth))
    {}

    void initialize(const Settings& settings) override;

    ~GitHubEventSource();

    void stop() override;

    void start() override;

    /**
     * \brief Builds a request for the relative \p url
     */
    web::Request request(const std::string& url) const;

    const Auth& auth() const
    {
        return auth_;
    }

    const std::string& api_url() const
    {
        return api_url_;
    }

private:
    void poll();

    std::vector<Repository> repositories;
    std::string api_url_;
    melanolib::time::Timer timer; ///< Polling timer
    std::vector<std::unique_ptr<GitHubEventListener>> listeners;
    Auth auth_;
};

/**
 * \brief Class that keeps track of \c GitHubEventSource objects
 */
class SourceRegistry : public melanolib::Singleton<SourceRegistry>
{
public:
    MaybePtr<const GitHubEventSource> get_source(const Auth& auth, const std::string& api_url) const
    {
        for ( auto src : sources )
        {
            if ( ( auth.empty() || src->auth() == auth ) &&
                    src->api_url() == api_url )
                return {src, false};
        }

        return MaybePtr<const GitHubEventSource>::make(auth);
    }

    void register_source(const GitHubEventSource* source)
    {
        auto it = std::find(sources.begin(), sources.end(), source);
        if ( it == sources.end() )
            sources.push_back(source);
    }

    void unregister_source(const GitHubEventSource* source)
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
    SourceRegistry(){}
    friend ParentSingleton;

    std::vector<const GitHubEventSource*> sources;

};

} // namespace github
#endif // MELANOBOT_MODULE_GITHUB_CONNECTION_HPP
