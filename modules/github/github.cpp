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
#include "module/melanomodule.hpp"
#include "github-source.hpp"
#include "github-listeners.hpp"
#include "github-handlers.hpp"

/**
 * \brief Registers the fun handlers
 */
MELANOMODULE_ENTRY_POINT module::Melanomodule melanomodule_github_metadata()
{
    return {"github", "GitHub integration", 0, {{"web"}}};
}


MELANOMODULE_ENTRY_POINT void melanomodule_github_initialize(const Settings&)
{
    github::SourceRegistry::instance();
    module::register_instantiable_service<github::GitHubEventSource>("GitHub");

    github::ListenerFactory::instance().register_listener<github::GitHubEventListener>("Event");
    github::ListenerFactory::instance().register_listener<github::CommitCommentEvent>("CommitCommentEvent");
    github::ListenerFactory::instance().register_listener<github::RefEvents>("RefEvents");
    github::ListenerFactory::instance().register_listener<github::ForkEvent>("ForkEvent");
    github::ListenerFactory::instance().register_listener<github::GollumEvent>("GollumEvent");
    github::ListenerFactory::instance().register_listener<github::IssueCommentEvent>("IssueCommentEvent");
    github::ListenerFactory::instance().register_listener<github::IssuesEvent>("IssuesEvent");
    github::ListenerFactory::instance().register_listener<github::MemberEvent>("MemberEvent");
    github::ListenerFactory::instance().register_listener<github::PullRequestEvent>("PullRequestEvent");
    github::ListenerFactory::instance().register_listener<github::PullRequestReviewCommentEvent>("PullRequestReviewCommentEvent");
    github::ListenerFactory::instance().register_listener<github::PushEvent>("PushEvent");
    github::ListenerFactory::instance().register_listener<github::ReleaseEvent>("ReleaseEvent");

    module::register_handler<github::GitHubIssue>("GitHubIssue");
    module::register_handler<github::GitHubRelease>("GitHubRelease");
    module::register_handler<github::GitHubSearch>("GitHubSearch");
}
