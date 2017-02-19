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

#include "user_manager.hpp"

namespace user {

User* UserManager::add_user(const User& user)
{
    users_.push_back(user);
    return &users_.back();
}

User* UserManager::user(const std::string& local_id)
{
    for ( User& u : users_ )
        if ( u.local_id == local_id )
            return &u;
    return nullptr;
}

const User* UserManager::user(const std::string& local_id) const
{
    for ( const User& u : users_ )
        if ( u.local_id == local_id )
            return &u;
    return nullptr;
}

User* UserManager::global_user(const std::string& global_id)
{
    for ( User& u : users_ )
        if ( u.global_id == global_id )
            return &u;
    return nullptr;
}

const User* UserManager::global_user(const std::string& global_id) const
{
    for ( const User& u : users_ )
        if ( u.global_id == global_id )
            return &u;
    return nullptr;
}

User* UserManager::user_by_property(const std::string& property_name,
                                    const std::string& property_value)
{
    for ( User& u : users_ )
        if ( u.property(property_name) == property_value )
            return &u;
    return nullptr;
}

const User* UserManager::user_by_property(const std::string& property_name,
                                          const std::string& property_value) const
{
    for ( const User& u : users_ )
        if ( u.property(property_name) == property_value )
            return &u;
    return nullptr;
}

std::list<User> UserManager::channel_users(const std::string& channel) const
{
    std::list<User> ret;
    for ( const User& user : users_ )
    {
        for ( const std::string& chan : user.channels )
        {
            if ( chan == channel )
            {
                ret.push_back(user);
                break;
            }
        }
    }
    return ret;
}

std::list<User*> UserManager::channel_user_pointers(const std::string& channel)
{
    std::list<User*> ret;
    for ( User& user : users_ )
    {
        for ( const std::string& chan : user.channels )
        {
            if ( chan == channel )
            {
                ret.push_back(&user);
                break;
            }
        }
    }
    return ret;
}

bool UserManager::remove_user(const std::string& local_id)
{
    for ( auto iter = users_.begin(); iter != users_.end(); ++iter )
        if ( iter->local_id == local_id )
        {
            users_.erase(iter);
            return true;
        }
    return false;
}

bool UserManager::change_id(const std::string& old_local_id, const std::string& new_local_id)
{
    for ( auto iter = users_.begin(); iter != users_.end(); ++iter )
        if ( iter->local_id == old_local_id )
        {
            iter->local_id = new_local_id;
            return true;
        }
    return false;
}

void UserManager::update_user(const std::string& local_id, const User& user)
{
    for ( User& u : users_ )
    {
        if ( u.local_id == local_id )
        {
            u = user;
            return;
        }
    }
}

} // namespace user
