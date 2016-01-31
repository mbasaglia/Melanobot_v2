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

/**
 * \brief Registers the fun handlers
 */
MELANOMODULE_ENTRY_POINT module::Melanomodule melanomodule_github_metadata()
{
    return {"github", "GitHub integration", 0, {{"web"}}};
}


MELANOMODULE_ENTRY_POINT void melanomodule_github_initialize(const Settings&)
{
    module::register_instantiable_service<github::GitHubEventSource>("GitHub");
    github::ListenerFactory::instance().register_listener<github::GitHubIssues>("Issues");
}
