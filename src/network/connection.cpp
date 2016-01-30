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
#include "connection.hpp"

namespace network {

void ConnectionFactory::register_connection(const std::string& protocol_name,
                                            const Contructor& function)
{
    if ( factory.count(protocol_name) )
        CRITICAL_ERROR("Re-registering connection protocol "+protocol_name);
    factory[protocol_name] = function;
}

std::unique_ptr<Connection> ConnectionFactory::create(
    const Settings& settings,
    const std::string& name)
{
    try
    {
        std::string protocol = settings.get("protocol",std::string());
        auto it = factory.find(protocol);
        if ( it != factory.end() )
        {
            if ( !settings.get("enabled",true) )
            {
                Log("sys",'!') << "Skipping disabled connection " << color::red << name;
                return nullptr;
            }

            Log("sys",'!') << "Creating connection " << color::dark_green << name;
            return it->second(settings, name);
        }
        ErrorLog ("sys","Connection Error") << ": Unknown connection protocol "+protocol;
    }
    catch ( const melanobot::MelanobotError& exc )
    {
        ErrorLog ("sys","Connection Error") << exc.what();
    }

    return nullptr;
}

} // namespace network
