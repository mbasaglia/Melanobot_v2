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
#ifndef MELANOBOT_MODULE_GITHUB_LISTENERS_HPP
#define MELANOBOT_MODULE_GITHUB_LISTENERS_HPP

#include <map>
#include "melanobot/melanobot.hpp"
#include "melanolib/string/language.hpp"
#include "replace-ptree.hpp"

namespace github {

/**
 * \brief Makes gir refs identifiers more human-readable
 */
inline std::string ref_to_branch(const std::string& ref)
{
    static melanolib::string::StringTrie unref{
        "refs/",
        "refs/heads/",
        "refs/remotes/",
    };

    return melanolib::string::replace(ref, unref);
}

/**
 * \todo Provide mappings between actor.name and connection users
 * \todo Allow some kind of grouping to avoid repetition
 *       of \p destination, \p target and friends
 */
class GitHubEventListener
{
public:
    GitHubEventListener(
        const Settings& settings,
        std::vector<std::string> event_types,
        const std::string& reply_template)
    : event_types_(std::move(event_types))
    {
        load_settings(settings, reply_template);
        std::string dest_name = settings.get("destination", "");
        destination = melanobot::Melanobot::instance().connection(dest_name);
        if ( !destination )
            throw melanobot::ConfigurationError("Missing destination connection");
    }

    GitHubEventListener(const Settings& settings)
    {
        load_settings(settings, "");
        event_types_  = {settings.get("event_type", "")};

        if ( reply_template_.empty() || event_types_[0].empty() )
            throw melanobot::ConfigurationError();
    }

    virtual ~GitHubEventListener() {}

    const std::vector<std::string>& event_types() const
    {
        return event_types_;
    }

    virtual void handle_event(const PropertyTree& events)
    {
        int n_events = 0;
        for ( const auto& event : events )
        {
            if ( n_events++ >= limit )
                break;
            send_message(replacements(reply_template_.copy(), event.second));
        }
    }

protected:
    void send_message(string::FormattedString&& str) const
    {
        destination->say(network::OutputMessage(
            std::move(str),
            action,
            target,
            priority,
            from.copy()
        ));
    }

    virtual string::FormattedString&& replacements(
        string::FormattedString&& string,
        const PropertyTree& json)
    {
        return std::move(replace(std::move(string), json));
    }

    int event_limit() const
    {
        return limit;
    }

    const string::FormattedString& reply_template() const
    {
        return reply_template_;
    }

private:
    void load_settings(const Settings& settings, const std::string& default_reply)
    {
        std::string dest_name = settings.get("destination", "");
        destination = melanobot::Melanobot::instance().connection(dest_name);
        if ( !destination )
            throw melanobot::ConfigurationError("Missing destination connection");

        target = settings.get("target", "");
        priority = settings.get("priority", priority);
        action = settings.get("action", action);
        from = string::FormatterConfig().decode(settings.get("from", ""));
        reply_template_ = string::FormatterConfig().decode(settings.get("reply", default_reply));
        limit = settings.get("limit", limit);
    }

    std::vector<std::string> event_types_;
    network::Connection* destination;
    bool                 action = false;
    std::string          target;
    int                  priority = 0;
    string::FormattedString from;
    string::FormattedString reply_template_;
    int limit = 5;
};

/**
 * \todo Move to a separate file
 */
class ListenerFactory : public melanolib::Singleton<ListenerFactory>
{
public:
    using product_type = std::unique_ptr<GitHubEventListener>;
    using key_type = std::string;
    using args_type = const Settings&;
    using functor_type = std::function<product_type (args_type settings)>;

    template<class ListenerT>
        void register_listener(const std::string& name)
        {
            factory[name] = [](const Settings& settings)
            {
                return melanolib::New<ListenerT>(settings);
            };
        }

    product_type build(const key_type& name, args_type args) const
    {
        auto it = factory.find(name);
        if ( it != factory.end() )
            return it->second(args);
        throw melanobot::ConfigurationError("Unknown GitHub listener: "+name);
    }

private:
    ListenerFactory(){}
    friend ParentSingleton;

    std::map<std::string, functor_type> factory;
};

/**
 * \todo Shorten sha
 * \todo Shorten comment body(?)
 * \note the repo events don't show edits or deletions
 */
class CommitCommentEvent : public GitHubEventListener
{
public:
    CommitCommentEvent(const Settings& settings)
        : GitHubEventListener(settings, {"CommitCommentEvent"}, default_message())
    {
    }

private:
    static const char* default_message()
    {
        return "[$(dark_magenta)$repo.name$(-)] $(blue)$actor.login$(-) commented on commit $(-b)$(short_sha $payload.comment.commit_id)$(-): $(git_io $payload.comment.html_url)";
    }
};

class RefEvents : public GitHubEventListener
{
public:
    RefEvents(const Settings& settings)
        : GitHubEventListener(settings, {"CreateEvent", "DeleteEvent"}, default_message())
    {
    }

protected:
    string::FormattedString&& replacements(string::FormattedString&& string, const PropertyTree& json) override
    {
        replace(std::move(string), json);
        if ( json.get("type", "") == "DeleteEvent" )
        {
            string.replace("action", "deleted" );
            string.replace("color", string::FormattedString() << color::red );
        }
        else
        {
            string.replace("action", "created" );
            string.replace("color", string::FormattedString() << color::dark_green );
        }
        return std::move(string);
    }

private:
    static const char* default_message()
    {
        return "[$(dark_magenta)$repo.name$(-)] $(blue)$actor.login$(-) $color$action$(-) $payload.ref_type $(-b)$payload.ref$(-)";
    }
};

class ForkEvent : public GitHubEventListener
{
public:
    ForkEvent(const Settings& settings)
        : GitHubEventListener(settings, {"ForkEvent"}, default_message())
    {
    }

private:
    static const char* default_message()
    {
        return "[$(dark_magenta)$repo.name$(-)] $(blue)$actor.login$(-) created fork $(-b)$payload.forkee.full_name$(-): $(git_io $payload.forkee.html_url)";
    }
};

class GollumEvent : public GitHubEventListener
{
public:
    GollumEvent(const Settings& settings)
        : GitHubEventListener(settings, {"GollumEvent"}, default_message())
    {
    }

protected:
    void handle_event(const PropertyTree& event) override
    {
        int n_pages = 0;
        for ( const auto& gollum : event )
        {
            for ( const auto& page : gollum.second.get_child("payload.pages", {}) )
            {
                if ( n_pages++ >= event_limit() )
                    break;

                send_message(reply_template().replaced(
                    [&gollum, &page](const std::string& id)
                        -> melanolib::Optional<string::FormattedString>
                    {
                        if ( melanolib::string::starts_with(id, "page.") )
                            return string::FormattedString(page.second.get(id.substr(5), ""));
                        auto opt = gollum.second.get_optional<std::string>(id);
                        if ( opt )
                            return string::FormattedString(*opt);
                        return {};
                    }
                ));
            }
        }
    }

private:
    static const char* default_message()
    {
        return "[$(dark_magenta)$repo.name$(-)] $(blue)$actor.login$(-) $page.action $(-b)$page.title$(-): $(git_io https://github.com/$page.html_url)";
    }
};

class IssueCommentEvent : public GitHubEventListener
{
public:
    IssueCommentEvent(const Settings& settings)
        : GitHubEventListener(settings, {"IssueCommentEvent"}, default_message())
    {
    }

private:
    static const char* default_message()
    {
        return "[$(dark_magenta)$repo.name$(-)] $(blue)$actor.login$(-) commented on issue $(-b)#$payload.issue.number$(-) $(-i)$payload.issue.title$(-): $(git_io $payload.comment.html_url)";
    }
};

/**
 * \todo Split labeled/assigned to separate classes
 */
class IssuesEvent : public GitHubEventListener
{
public:
    IssuesEvent(const Settings& settings)
        : GitHubEventListener(settings, {"IssuesEvent"}, default_message())
    {
        detailed = settings.get("detailed", detailed);
    }

    void handle_event(const PropertyTree& event) override
    {
        int n_events = 0;
        for ( const auto& issue : event )
        {
            color::Color12 color;
            auto action = issue.second.get("payload.action", "");

            if ( action == "closed" )
                color = color::red;
            else if ( action == "opened"|| action == "reopened" )
                color = color::dark_green;
            else if ( !detailed )
                continue;
            else
                color = color::dark_cyan;

            if ( n_events++ >= event_limit() )
                break;

            auto reply = replacements(reply_template().copy(), issue.second);
            reply.replace("color", string::FormattedString() << color);

            send_message(std::move(reply));
        }
    }

private:
    static const char* default_message()
    {
        return "[$(dark_magenta)$repo.name$(-)] $(blue)$actor.login$(-) $color$payload.action$(-) issue $(-b)#$payload.issue.number$(-): $(-i)$payload.issue.title$(-) $(git_io $payload.issue.html_url)";
    }

    bool detailed = false;
};


class MemberEvent : public GitHubEventListener
{
public:
    MemberEvent(const Settings& settings)
        : GitHubEventListener(settings, {"MemberEvent"}, default_message())
    {
    }

protected:
    string::FormattedString&& replacements(string::FormattedString && string, const PropertyTree& json) override
    {
        replace(std::move(string), json);

        if ( json.get("payload.action", "") == "added" )
            string.replace("color", string::FormattedString() << color::dark_green);
        else
            string.replace("color", string::FormattedString() << color::red);

        return std::move(string);

    }

private:
    static const char* default_message()
    {
        return "[$(dark_magenta)$repo.name$(-)] $(blue)$actor.login$(-) $color$payload.action$(-) member $(-b)$payload.member.login$(-)";
    }
};


class PullRequestEvent : public GitHubEventListener
{
public:
    PullRequestEvent(const Settings& settings)
        : GitHubEventListener(settings, {"PullRequestEvent"}, default_message())
    {
    }

protected:
    string::FormattedString&& replacements(string::FormattedString&& string, const PropertyTree& json) override
    {
        replace(std::move(string), json);

        auto action = json.get("payload.action", "");
        color::Color12 color =
            action == "closed" ?
                color::red :
                color::dark_green;

        string.replace("color", string::FormattedString() << color);
        return std::move(string);
    }


private:
    static const char* default_message()
    {
        return "[$(dark_magenta)$repo.name$(-)] $(blue)$actor.login$(-) $color$payload.action$(-) pull request $(-b)#$payload.pull_request.number$(-) ($(-b)$(dark_yellow)$payload.pull_request.head.ref$(-) -> $(-b)$payload.pull_request.base.ref$(-)) $(-i)$payload.pull_request.title$(-): $(git_io $payload.pull_request.html_url)";
    }
};

class PullRequestReviewCommentEvent : public GitHubEventListener
{
public:
    PullRequestReviewCommentEvent(const Settings& settings)
        : GitHubEventListener(settings, {"PullRequestReviewCommentEvent"}, default_message())
    {
    }

private:
    static const char* default_message()
    {
        return "[$(dark_magenta)$repo.name$(-)] $(blue)$actor.login$(-) commented on issue $(-b)#$payload.pull_request.number$(-) ($(-i)$payload.pull_request.title$(-)): $(git_io $payload.comment.html_url)";
    }
};

/**
 * \todo should have a nice branch name (extracted from $payload.ref)
 * \todo should generate a url with the diff for the commits
 */
class PushEvent : public GitHubEventListener
{
public:
    PushEvent(const Settings& settings)
        : GitHubEventListener(settings, {"PushEvent"}, default_message())
    {
        commit_reply_template = string::FormatterConfig().decode(
            settings.get("commit_reply",
                " * [$(dark_magenta)$(short_sha $sha)$(-)] $(blue)$author.name$(-) $summary")
        );
        commit_limit = settings.get("commit_limit", commit_limit);
    }

protected:
    void handle_event(const PropertyTree& event)
    {
        /// \todo Merge consecutive events from the same person to the same branch
        int n_pushes = 0;
        for ( const auto& push : event )
        {
            if ( n_pushes++ >= event_limit() )
                break;

            auto commits = push.second.get_child("payload.commits", {});

            auto reply = replace(reply_template().copy(), push.second);
            reply.replace("commit_pluralized", melanolib::string::english.pluralize(commits.size(), "commit"));
            reply.replace("branch", ref_to_branch(push.second.get("payload.ref", "")));

            send_message(std::move(reply));

            int n_commits = 0;
            for ( const auto& commit : commits )
            {
                if ( n_commits++ >= commit_limit )
                    break;
                auto commit_reply = replace(commit_reply_template.copy(), commit.second);
                auto lines = melanolib::string::char_split(commit.second.get("message", ""), '\n');
                if ( !lines.empty() )
                    commit_reply.replace("summary", lines[0]);
                send_message(std::move(commit_reply));
            }
        }
    }

private:
    static const char* default_message()
    {
        return "[$(dark_magenta)$repo.name$(-)] $(blue)$actor.login$(-) pushed $(-b)$payload.size$(-) $(plural $payload.size commit) on $(magenta)$branch$(-): $(git_io 'https://github.com/$repo.name/compare/${payload.before}...${payload.head}')";
    }

    string::FormattedString commit_reply_template;
    int commit_limit = 3;
};

class ReleaseEvent : public GitHubEventListener
{
public:
    ReleaseEvent(const Settings& settings)
        : GitHubEventListener(settings, {"ReleaseEvent"}, default_message())
    {
    }

protected:
    string::FormattedString&& replacements(string::FormattedString&& string, const PropertyTree& json) override
    {
        replace(std::move(string), json);

        std::string release_type = json.get("payload.prerelease", false) ? "pre-release" : "release";
        if ( json.get("payload.draft", false) )
            release_type = "draft "+release_type;

        string.replace("release_type", release_type);

        return std::move(string);
    }

private:
    static const char* default_message()
    {
        return "[$(dark_magenta)$repo.name$(-)] $(blue)$actor.login$(-) $payload.action $release_type $(-b)$payload.release.name$(-): $(git_io $payload.release.html_url)";
    }
};

/*

Ignored:
PublicEvent
WatchEvent

*/


} // namespace github
#endif // MELANOBOT_MODULE_GITHUB_LISTENERS_HPP
