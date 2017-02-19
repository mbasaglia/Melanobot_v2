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

#include "user_group.hpp"

namespace user {

bool UserGroup::add_user ( const User& user )
{
    for ( const User& u : users )
        if ( u.matches(user) )
            return false;
    users.push_back(user);
    return true;
}

void UserGroup::remove_user ( const User& user )
{
    auto iter = users.begin();
    while ( iter != users.end() )
    {
        if ( user.matches(*iter) )
            iter = users.erase(iter);
        else
            ++iter;
    }
}

bool UserGroup::contains ( const User& user, bool recursive ) const
{
    for ( const User& u : users )
        if ( u.matches(user) )
            return true;
    if ( recursive )
    {
        for ( UserGroup* g : children )
            if ( g->contains(user, true) )
                return true;
    }
    return false;
}

void UserGroup::add_child ( UserGroup* child )
{
    for ( UserGroup* g : children )
        if ( g == child )
            return;
    children.push_back(child);
}

std::vector<User> UserGroup::direct_users() const
{
    return users;
}

std::vector<User> UserGroup::all_users() const
{
    std::vector<User> ret = direct_users();

    for ( UserGroup* g : children )
    {
        std::vector<User> tmp = g->all_users();
        ret.insert(ret.end(), tmp.begin(), tmp.end());
    }

    return ret;
}

} // namespace user
