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

#include "repository.hpp"
#include "melanobot/melanobot.hpp"

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

class GitHubEventListener
{
public:
    GitHubEventListener(const Settings& settings)
    {
        std::string dest_name = settings.get("destination", "");
        destination = melanobot::Melanobot::instance().connection(dest_name);
        if ( !destination )
            throw melanobot::ConfigurationError("Missing destination connection");

        target = settings.get("target", "");
        priority = settings.get("priority", priority);
        action = settings.get("action", action);
        from = string::FormatterConfig().decode(settings.get("from", ""));
    }

    virtual ~GitHubEventListener() {}

    virtual Repository::EventType event_type() const = 0;

    virtual void handle_event(const PropertyTree& event) = 0;

protected:
    void send_message(const string::FormattedString& str) const
    {
        destination->say(network::OutputMessage(
            str,
            action,
            target,
            priority,
            from
        ));
    }

    void send_message(const std::string& reply_template, const PropertyTree& json) const
    {
        send_message(string::FormatterConfig().decode(
            melanolib::string::replace(reply_template, tree_to_trie(json))
        ));
    }

private:
    network::Connection* destination;
    bool                 action = false;
    std::string          target;
    int                  priority = 0;
    string::FormattedString from;
};

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

class GitHubIssues : public GitHubEventListener
{
public:
    GitHubIssues(const Settings& settings)
        : GitHubEventListener(settings)
    {
        reply_template = settings.get("reply", reply_template);
        limit = settings.get("limit", limit);
    }

    Repository::EventType event_type() const override
    {
        return Repository::Issues;
    }

    void handle_event(const PropertyTree& event) override
    {
        int i = 0;
        for ( const auto& issue : event )
        {
            if ( ++i >= limit )
                break;
            if ( issue.second.get("state", "") == "open" )
                send_message(reply_template, issue.second);
        }
    }

private:
    std::string reply_template = "#blue#%user.login#-# opened issue #-b###%number#-#: #-i#%title#-# %html_url";
    int limit = 5;
};



} // namespace github
#endif // MELANOBOT_MODULE_GITHUB_HANDLERS_HPP
