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

#include <map>
#include "melanobot/melanobot.hpp"
#include "melanolib/string/language.hpp"

namespace github {

inline void tree_to_trie_impl(const std::string& prefix,
                         const PropertyTree& tree,
                         const std::string& separator,
                         melanolib::string::StringTrie &output)
{
    for ( const auto& pair : tree )
    {
        if ( !pair.first.empty() )
        {
            if ( !pair.second.data().empty() )
                output.insert(prefix+pair.first, pair.second.data());

            tree_to_trie_impl(prefix+pair.first+separator, pair.second, separator, output);
        }
    }
}

inline melanolib::string::StringTrie tree_to_trie(const PropertyTree& tree,
                                                  const std::string& prefix = "%",
                                                  const std::string& separator = ".")
{
    melanolib::string::StringTrie trie;
    tree_to_trie_impl("", tree, separator, trie);
    trie.prepend(prefix);
    return trie;
}

/**
 * \brief Makes gir refs identifiers more human-readable
 */
inline std::string ref_to_branch(const std::string& ref)
{
    static melanolib::string::StringTrie unref{
        "refs/",
        "refs/head/",
        "refs/remotes/",
    };

    return melanolib::string::replace(ref, unref);
}

/**
 * \brief Shortens a commit hash
 */
inline std::string short_sha(const std::string& sha)
{
    return sha.substr(0, 7);
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
        std::string reply_template)
    : event_types_(std::move(event_types)),
      reply_template_(std::move(reply_template))
    {
        load_settings(settings);
        std::string dest_name = settings.get("destination", "");
        destination = melanobot::Melanobot::instance().connection(dest_name);
        if ( !destination )
            throw melanobot::ConfigurationError("Missing destination connection");
    }

    GitHubEventListener(const Settings& settings)
    {
        load_settings(settings);
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
            send_message(reply_template_, replacements(event.second));
        }
    }

protected:
    void send_message(const string::FormattedString& str) const
    {
        destination->say(network::OutputMessage(
            str.copy(),
            action,
            target,
            priority,
            from.copy()
        ));
    }

    void send_message(const std::string& reply_template,
                      const melanolib::string::StringTrie& trie) const
    {
        send_message(string::FormatterConfig().decode(
            melanolib::string::replace(reply_template, trie)
        ));
    }

    virtual melanolib::string::StringTrie replacements(const PropertyTree& json)
    {
        return tree_to_trie(json);
    }

    int event_limit() const
    {
        return limit;
    }

    const std::string& reply_template() const
    {
        return reply_template_;
    }

private:
    void load_settings(const Settings& settings)
    {
        std::string dest_name = settings.get("destination", "");
        destination = melanobot::Melanobot::instance().connection(dest_name);
        if ( !destination )
            throw melanobot::ConfigurationError("Missing destination connection");

        target = settings.get("target", "");
        priority = settings.get("priority", priority);
        action = settings.get("action", action);
        from = string::FormatterConfig().decode(settings.get("from", ""));
        reply_template_ = settings.get("reply", reply_template_);
        limit = settings.get("limit", limit);
    }

    std::vector<std::string> event_types_;
    network::Connection* destination;
    bool                 action = false;
    std::string          target;
    int                  priority = 0;
    string::FormattedString from;
    std::string         reply_template_;
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

protected:
    melanolib::string::StringTrie replacements(const PropertyTree& json) override
    {
        auto trie = tree_to_trie(json);
        trie.insert("%short_sha", short_sha(json.get("payload.comment.commit_id", "")));
        return trie;
    }

private:
    static const char* default_message()
    {
        return "[#dark_magenta#%repo.name#-#] #blue#%actor.login#-# commented on commit #-b#%short_sha#-#: %payload.comment.html_url";
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
    melanolib::string::StringTrie replacements(const PropertyTree& json) override
    {
        auto trie = tree_to_trie(json);
        std::string action;
        if ( json.get("type", "") == "DeleteEvent" )
        {
            trie.insert("%action", "deleted" );
            trie.insert("%color", "#red#" );
        }
        else
        {
            trie.insert("%action", "created" );
            trie.insert("%color", "#dark_green#" );
        }
        return trie;
    }

private:
    static const char* default_message()
    {
        return "[#dark_magenta#%repo.name#-#] #blue#%actor.login#-# %color%action#-# %payload.ref_type #-b#%payload.ref#-#";
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
        return "[#dark_magenta#%repo.name#-#] #blue#%actor.login#-# created fork #-b#%payload.forkee.full_name#-#: %payload.forkee.html_url";
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
            auto event_nopages = gollum.second;
            event_nopages.get_child("payload").erase("pages");
            auto trie_nopages = replacements(event_nopages);

            for ( const auto& page : gollum.second.get_child("payload.pages", {}) )
            {
                if ( n_pages++ >= event_limit() )
                    break;
                auto trie = tree_to_trie(page.second, "%page.");
                trie += trie_nopages;
                send_message(reply_template(), trie);
            }
        }
    }

private:
    static const char* default_message()
    {
        return "[#dark_magenta#%repo.name#-#] #blue#%actor.login#-# %page.action #-b#%page.title#-#: https://github.com/%page.html_url";
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
        return "[#dark_magenta#%repo.name#-#] #blue#%actor.login#-# commented on issue #-b###%payload.issue.number#-# #-i#%payload.issue.title#-#: %payload.comment.html_url";
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

    void handle_event(const PropertyTree& event)
    {
        int n_events = 0;
        for ( const auto& issue : event )
        {

            auto trie = replacements(issue.second);

            std::string color;
            auto action = event.get("%payload.action", "");

            if ( action == "closed" )
                color = "#red#";
            else if ( action == "opened"|| action == "reopened" )
                color = "#dark_green#";
            else if ( !detailed )
                continue;
            else
                color = "#dark_cyan#";

            if ( n_events++ >= event_limit() )
                break;

            trie.insert("%color", color);
            send_message(reply_template(), trie);
        }
    }

private:
    static const char* default_message()
    {
        return "[#dark_magenta#%repo.name#-#] #blue#%actor.login#-# %color%payload.action#-# issue #-b###%payload.issue.number#-#: #-i#%payload.issue.title#-# %payload.issue.html_url";
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

private:
    static const char* default_message()
    {
        return "[#dark_magenta#%repo.name#-#] #blue#%actor.login#-# %payload.action member #-b#%payload.member.id#-#";
    }
};


class PullRequestEvent : public GitHubEventListener
{
public:
    PullRequestEvent(const Settings& settings)
        : GitHubEventListener(settings, {"PullRequestEvent"}, default_message())
    {
    }

private:
    static const char* default_message()
    {
        return "[#dark_magenta#%repo.name#-#] #blue#%actor.login#-# %payload.action pull request #-b###%payload.pull_request.number#-#: (#-b##dark_yellow#%payload.pull_request.head.ref#-# -> #-b#%payload.pull_request.base.ref#-#) #-i#%payload.pull_request.title#-# %payload.pull_request.html_url";
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
        return "[#dark_magenta#%repo.name#-#] #blue#%actor.login#-# commented on issue #-b###%payload.pull_request.number#-# (#-i#%payload.pull_request.title#-#): %payload.comment.html_url";
    }
};

/**
 * \todo should have a nice branch name (extracted from %payload.ref)
 * \todo should generate a url with the diff for the commits
 */
class PushEvent : public GitHubEventListener
{
public:
    PushEvent(const Settings& settings)
        : GitHubEventListener(settings, {"PushEvent"}, default_message())
    {
        commit_reply_template = settings.get("commit_reply", commit_reply_template);
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

            auto trie = tree_to_trie(push.second);
            trie.insert("%commit_pluralized",
                melanolib::string::english.pluralize(commits.size(), "commit")
            );

            trie.insert("%branch", ref_to_branch(push.second.get("payload.ref", "")));
            trie.insert("%short_before", short_sha(push.second.get("payload.head", "")));
            trie.insert("%short_head", short_sha(push.second.get("payload.head", "")));


            send_message(reply_template(), trie);

            int n_commits = 0;
            for ( const auto& commit : commits )
            {
                if ( n_commits++ >= commit_limit )
                    break;
                auto commit_trie = tree_to_trie(commit.second);
                commit_trie.insert("%short_sha", short_sha(commit.second.get("sha", "")));
                send_message(commit_reply_template, commit_trie);
            }
        }
    }

private:
    static const char* default_message()
    {
        return "[#dark_magenta#%repo.name#-#] #blue#%actor.login#-# pushed #-b#%payload.size#-# %commit_pluralized on #magenta#%branch#-#: https://github.com/%repo.name/compare/%short_before...%short_head";
    }

    std::string commit_reply_template = " * [#dark_magenta#%short_sha#-#] #blue#%author.name#-# %message";
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
    melanolib::string::StringTrie replacements(const PropertyTree& json) override
    {
        auto trie = tree_to_trie(json);

        std::string release_type = json.get("payload.prerelease", false) ? "pre-release" : "release";
        if ( json.get("payload.draft", false) )
            release_type = "draft "+release_type;

        trie.insert("%release_type", release_type);

        return trie;
    }

private:
    static const char* default_message()
    {
        return "[#dark_magenta#%repo.name#-#] #blue#%actor.login#-# %payload.action %release_type #-b#%payload.release.name#-#: %payload.release.html_url";
    }
};

/*

Ignored:
PublicEvent
WatchEvent

*/


} // namespace github
#endif // MELANOBOT_MODULE_GITHUB_HANDLERS_HPP
