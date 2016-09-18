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

#include "async_service.hpp"


void ServiceRegistry::register_service(const std::string& name, AsyncService* object)
{
    if ( services.count(name) )
        ErrorLog("sys") << "Overwriting service " << name;
    services.insert({name, Entry{object}});
}

void ServiceRegistry::initialize(const Settings& settings)
{
    for ( const auto& p : settings )
    {
        auto it = services.find(p.first);
        if ( it == services.end() )
        {
            ErrorLog("sys") << "Trying to load an unknown service: " << p.first;
        }
        else
        {
            load_service(*it, p.second);
        }
    }

    for ( auto& p : services )
        load_service(p, {});
}

void ServiceRegistry::start()
{
    for ( auto& p : services )
        if ( p.second.loaded )
        {
            try
            {
                p.second.service->start();
                p.second.started = true;
            }
            catch (const melanobot::MelanobotError& err)
            {
                ErrorLog("sys") << "Failed starting service " << p.first
                    << ": " << err.what();
            }
        }
}

void ServiceRegistry::stop()
{
    for ( auto& p : services )
        if ( p.second.started )
        {
            p.second.service->stop();
            p.second.started = false;
        }
}

AsyncService* ServiceRegistry::service(const std::string& name) const
{
    auto it = services.find(name);
    if ( it == services.end() )
    {
        ErrorLog("sys") << "Trying to access unknown service: " << name;
        return nullptr;
    }

    if ( !it->second.loaded )
    {
        ErrorLog("sys") << "Trying to access an unloaded service: " << name;
        return nullptr;
    }

    return it->second.service;
}

void ServiceRegistry::load_service(EntryMap::value_type& entry, const Settings& settings)
{
    Log("sys", '!') << "Loading service: " << entry.first;
    try
    {
        entry.second.service->initialize(settings);
        entry.second.loaded = true;
    }
    catch ( const melanobot::MelanobotError& err )
    {
        ErrorLog("sys") << "Service " << entry.first << " failed: " << err.what();
    }
}
