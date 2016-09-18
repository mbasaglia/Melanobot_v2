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
#include "web-api-concrete.hpp"
#include "http.hpp"
#include "module/melanomodule.hpp"

/**
 * \brief Initializes the web module
 */
MELANOMODULE_ENTRY_POINT module::Melanomodule melanomodule_web_metadata()
{
    return {"web", "Web services"};
}

MELANOMODULE_ENTRY_POINT void melanomodule_web_initialize(const Settings&)
{
    module::register_log_type("web", color::dark_blue);
    module::register_service<web::HttpClient>("http");

    module::register_handler<web::SearchVideoYoutube>("SearchVideoYoutube");
    module::register_handler<web::UrbanDictionary>("UrbanDictionary");
    module::register_handler<web::SearchWebSearx>("SearchWebSearx");
    module::register_handler<web::VideoInfo>("VideoInfo");
    module::register_handler<web::MediaWiki>("MediaWiki");
    module::register_handler<web::MediaWikiTitles>("MediaWikiTitles");
    module::register_handler<web::MediaWikiCategoryTitle>("MediaWikiCategoryTitle");
    module::register_handler<web::WhereIsGoogle>("WhereIsGoogle");
}
