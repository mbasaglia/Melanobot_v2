/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015-2016 Mattia Basaglia
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
#ifndef MELANOBOT_MODULE_GITHUB_HANDLERS_HPP
#define MELANOBOT_MODULE_GITHUB_HANDLERS_HPP

#include "github-source.hpp"
#include "melanobot/handler.hpp"
#include "string/json.hpp"
#include "replace-ptree.hpp"

namespace github {

class GitHubBase : public melanobot::SimpleAction
{
public:
    GitHubBase(const std::string& default_trigger,
               const Settings&    settings,
               MessageConsumer*   parent)
        : SimpleAction(default_trigger, settings, parent)
    {
        auth.username = settings.get("username", "");
        auth.password = settings.get("password", "");
        api_url = settings.get("api_url", api_url);
    }

protected:
    /**
     * \brief Sends an asynchronous github request
     */
    void request_github(const network::Message& msg, const std::string& url)
    {
        auto ptr = SourceRegistry::instance().get_source(auth, api_url);
        web::HttpService::instance().async_query(ptr->request(url),
            [this, msg](const web::Response& resp)
            {
                JsonParser parser;
                parser.throws(false);
                PropertyTree tree = parser.parse_string(resp.contents);
                if ( resp.success() )
                    github_success(msg, tree);
                else
                    github_failure(msg, tree);
            });
    }

    /**
     * \brief Called when request_github() gets a successful response
     */
    virtual void github_success(const network::Message& msg, const PropertyTree& response) = 0;


    /**
     * \brief Called when request_github() fails
     */
    virtual void github_failure(const network::Message&, const PropertyTree&) {}

private:
    Auth auth;
    std::string api_url = "https://api.github.com";
};

class GitHubRepoBase : public GitHubBase
{
public:
    GitHubRepoBase(const std::string& default_trigger,
               const Settings&    settings,
               MessageConsumer*   parent)
        : GitHubBase(default_trigger, settings, parent)
    {
        repo = settings.get("repo", "");
        if ( repo.empty() )
            throw melanobot::ConfigurationError("Missing repository");
    }

protected:
    void request_github_repo(const network::Message& msg, const std::string& url)
    {
        request_github(msg, "/repos/"+repo+url);
    }

private:
    std::string repo;
};

class GitHubIssue : public GitHubRepoBase
{
public:
    GitHubIssue(const Settings& settings, MessageConsumer* parent)
        : GitHubRepoBase("issue", settings, parent)
    {
        reply = read_string(settings, "reply", "$(-b)#$number$(-) - $(-i)$title$(-) ($color$state$(-)): $html_url");
        reply_failure = read_string(settings, "reply_failure", "I didn't find issue $(-b)$message$(b)");
        reply_invalid = read_string(settings, "reply_invalid", "Which issue?");
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        static std::regex strip_request(R"(\s*#?(\d+)\s*)",
            std::regex::optimize|std::regex::ECMAScript);
        std::smatch match;
        if ( std::regex_match(msg.message, match, strip_request) )
        {
            request_github_repo(msg, "/issues/"+match[1].str());
        }
        else
        {
            reply_to(msg, reply_invalid.replaced(msg.source->pretty_properties(msg.from)));
        }

        return true;
    }

    void github_success(const network::Message& msg, const PropertyTree& response) override
    {
        reply_to(msg, replace(reply.copy(), response));
    }

    void github_failure(const network::Message& msg, const PropertyTree& response) override
    {
        auto r = reply_failure.replaced(msg.source->pretty_properties(msg.from));
        r.replace("message", msg.message);
        reply_to(msg, std::move(r));
    }

private:
    string::FormattedString reply;
    string::FormattedString reply_failure;
    string::FormattedString reply_invalid;
};

} // namespace github
#endif // MELANOBOT_MODULE_GITHUB_HANDLERS_HPP
