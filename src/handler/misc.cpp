/**
 * \file
 * \brief This file defines handlers which perform miscellaneous tasks
 *
 *
 * \author Mattia Basaglia
 * \copyright Copyright 2015 Mattia Basaglia
 * \license
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
#include "handler.hpp"

namespace handler {

/**
 * \brief Handler showing licensing information
 * \note Must be enabled to comply to the AGPL
 */
class License : public SimpleAction
{
public:
    License(const Settings& settings, Melanobot* bot)
        : SimpleAction("license",settings,bot)
    {
        sources_url = settings.get("url",Settings::global_settings.get("website",""));
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        reply_to(msg,"AGPLv3+ (http://www.gnu.org/licenses/agpl-3.0.html), Sources: "+sources_url);
        return true;
    }

private:
    std::string sources_url;
};
REGISTER_HANDLER(License,License);

} // namespace handler
