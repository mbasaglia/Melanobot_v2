/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015 Mattia Basaglia
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
#include "handler.hpp"

namespace handler {

class License : public SimpleAction
{
public:
    static License* create(const Settings& settings, Melanobot* bot)
    {
        License* obj = new License;
        obj->sources_url = settings.get("url",Settings::global_settings.get("website",""));
        obj->trigger = "license";
        if ( obj->load_settings(settings,bot) )
            return obj;
        delete obj;
        ErrorLog("sys") << "Error creating handler";
        return nullptr;
    }

protected:
    bool on_handle(const network::Message& msg) override
    {
        reply_to(msg,"AGPLv3+ (http://www.gnu.org/licenses/agpl-3.0.html), Sources: "+sources_url);
        return true;
    }

private:
    License() {}
    std::string sources_url;
};
REGISTER_HANDLER(License,License);

} // namespace handler
