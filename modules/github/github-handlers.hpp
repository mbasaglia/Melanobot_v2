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

#include "github-controller.hpp"
#include "melanobot/handler.hpp"
#include "httpony/formats/json.hpp"
#include "replace-ptree.hpp"
#include "gitio.hpp"

namespace github {

/**
 * \brief Base class for handlers that send requests to the GitHub API
 */
class GitHubBase : public melanobot::SimpleAction
{
public:
    GitHubBase(const std::string& default_trigger,
               const Settings&    settings,
               MessageConsumer*   parent)
        : SimpleAction(default_trigger, settings, parent)
    {
        auth.user = settings.get("username", "");
        auth.password = settings.get("password", "");
        api_url = settings.get("api_url", api_url);
    }

protected:
    /**
     * \brief Sends an asynchronous github request
     */
    void request_github(const network::Message& msg, const std::string& url)
    {
        query(url,
            [this, msg](web::Request& request, web::Response& response)
            {
                httpony::json::JsonParser parser;
                parser.throws(false);
                PropertyTree tree = parser.parse(response.body, request.uri.full());
                if ( response.status.is_error() )
                    github_failure(msg, tree);
                else
                    github_success(msg, tree);
            });
    }

    template<class Callback>
    void query(const std::string& url, const Callback& callback)
    {
        auto ptr = ControllerRegistry::instance().get_source(auth, api_url);
        web::HttpClient::instance().async_query(ptr->request(url), callback);
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
    httpony::Auth auth;
    std::string api_url = "https://api.github.com";
};

/**
 * \brief Base class for handlers that ooperate on github sources
 */
class GitHubSourceBase : public GitHubBase
{
public:
    GitHubSourceBase(const std::string& default_trigger,
               const Settings&    settings,
               MessageConsumer*   parent)
        : GitHubBase(default_trigger, settings, parent)
    {
        git_source = settings.get("git_source", "");
        if ( git_source.empty() )
            throw melanobot::ConfigurationError("Missing github source");
    }

protected:
    void request_github_source(const network::Message& msg, const std::string& url)
    {
        request_github(msg, relative_url(url));
    }

    std::string relative_url(const std::string& url)
    {
        return "/" + git_source + url;
    }

private:
    std::string git_source;
};

/**
 * \brief Gets the details of a single issue
 */
class GitHubIssue : public GitHubSourceBase
{
public:
    GitHubIssue(const Settings& settings, MessageConsumer* parent)
        : GitHubSourceBase("issue", settings, parent)
    {
        reply = read_string(settings, "reply", "$(-b)#$number$(-) - $(-i)$title$(-) ($color$state$(-)): $(git_io $html_url)");
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
            request_github_source(msg, "/issues/"+match[1].str());
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

/**
 * \brief Gets the details of a single release
 */
class GitHubRelease : public GitHubSourceBase
{
public:
    GitHubRelease(const Settings& settings, MessageConsumer* parent)
        : GitHubSourceBase("release", settings, parent)
    {
        reply = read_string(settings, "reply", "$(ucfirst $release_type) $(-b)$name$(-): $(git_io $html_url)");
        reply_failure = read_string(settings, "reply_failure", "I didn't find any such release");
        reply_asset = read_string(settings, "reply_asset", " * $name $(git_io $browser_download_url) $(-b)$human_size$(-), $download_count downloads");
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        std::string which = melanolib::string::trimmed(msg.message);
        if ( which.empty() || which == "latest" )
        {
            request_github_source(msg, "/releases/latest");
        }
        else
        {
            query(relative_url("/releases"),
                [msg, this, which](web::Request& request, web::Response& response)
                {
                    if ( response.status.is_error() )
                    {
                        github_failure(msg, {});
                    }
                    else
                    {
                        httpony::json::JsonParser parser;
                        parser.throws(false);
                        find_release(msg, which, parser.parse(response.body, request.uri.full()));
                    }
                });
        }

        return true;
    }

    void github_success(const network::Message& msg, const PropertyTree& response) override
    {
        auto reply = replace(this->reply.copy(), response);

        std::string release_type = response.get("payload.prerelease", false) ? "pre-release" : "release";
        if ( response.get("payload.draft", false) )
            release_type = "draft "+release_type;

        reply.replace("release_type", release_type);

        reply_to(msg, std::move(reply));

        for ( const auto& dload : response.get_child("assets", {}) )
        {
            auto dload_reply = replace(reply_asset.copy(), dload.second);
            dload_reply.replace("human_size", melanolib::string::pretty_bytes(
                dload.second.get<uint64_t>("size", 0)));
            reply_to(msg, std::move(dload_reply));
        }
    }

    void github_failure(const network::Message& msg, const PropertyTree& response) override
    {
        auto r = reply_failure.replaced(msg.source->pretty_properties(msg.from));
        r.replace("message", msg.message);
        reply_to(msg, std::move(r));
    }

private:
    void find_release(const network::Message& msg, const std::string& which, const PropertyTree& releases)
    {
        const PropertyTree* best_release = nullptr;
        int max_score = 0;


        for ( const auto& release : releases )
        {
            int score = melanolib::math::max(
                melanolib::string::similarity(which, release.second.get("tag", "")),
                melanolib::string::similarity(which, release.second.get("name", "")),
                melanolib::string::similarity(which, release.second.get("body", ""))
            );

            if ( score > max_score )
            {
                max_score = score;
                best_release = &release.second;
            }
        }

        if ( best_release )
            github_success(msg, *best_release);
        else
            github_failure(msg, {});
    }

    string::FormattedString reply;
    string::FormattedString reply_asset;
    string::FormattedString reply_failure;
};


/**
 * \brief Searches code in github
 */
class GitHubSearch : public GitHubBase
{
public:
    GitHubSearch(const Settings& settings, MessageConsumer* parent)
        : GitHubBase("code search", settings, parent)
    {
        reply = read_string(settings, "reply", " * [$(dark_magenta)$repository.full_name$(-)] $(dark_red)$path$(-) @ $(-b)$(short_sha $sha)$(-): $(git_io $html_url)");
        reply_invalid = read_string(settings, "reply", "$(dark_blue)std$(green)::$(blue)cout$(-) << $(dark_red)\"Search for what?\"$(-);");
        reply_failure = read_string(settings, "reply_failure", "I didn't find anything about $query");
        force = settings.get("force", force);
        max_results = settings.get("max_results", max_results);
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        std::string what = melanolib::string::trimmed(msg.message);
        if ( what.empty() )
        {
            reply_to(msg, reply_invalid.replaced(msg.source->pretty_properties(msg.from)));
        }
        else
        {
            std::string what_full = what;
            if ( !force.empty() )
                what_full += ' ' + force;

            query("/search/code?q="+web::urlencode(what_full),
                [msg, this, what](web::Request& request, web::Response& response)
                {
                    httpony::json::JsonParser parser;
                    parser.throws(false);
                    auto json = parser.parse(response.body, request.uri.full());

                    if ( !response.status.is_error() && json.get("total_count", 0) > 0 )
                    {
                        std::size_t i = 0;
                        for ( auto& match : json.get_child("items") )
                        {
                            if ( i++ >= max_results )
                                break;
                            match.second.put("query", what);
                            github_success(msg, match.second);
                        }
                    }
                    else
                    {
                        json.put("query", what);
                        github_failure(msg, json);
                    }
                });
        }

        return true;
    }

    void github_success(const network::Message& msg, const PropertyTree& response) override
    {
        reply_to(msg, replace(reply.copy(), response));
    }

    void github_failure(const network::Message& msg, const PropertyTree& response) override
    {
        reply_to(msg, replace(reply_failure.copy(), response));
    }

private:
    string::FormattedString reply;          ///< Reply to give when we have a result
    string::FormattedString reply_invalid;  ///< Reply to give on an invalid search item
    string::FormattedString reply_failure;  ///< Reply to give on a search with no results / API failure
    std::string             force;          ///< Search query to append to every request
    std::size_t             max_results = 3;///< Maximum number of returned results
};


/**
 * \brief Interface to git.io
 */
class GitIo : public melanobot::SimpleAction
{
public:
    GitIo(const Settings& settings, MessageConsumer* parent)
        : SimpleAction("git.io", settings, parent)
    {
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        reply_to(msg, git_io_shorten(msg.message));
        return true;
    }
};


} // namespace github
#endif // MELANOBOT_MODULE_GITHUB_HANDLERS_HPP
