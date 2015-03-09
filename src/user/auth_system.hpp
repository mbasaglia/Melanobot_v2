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
#ifndef AUTH_SYSTEM_HPP
#define AUTH_SYSTEM_HPP

#include <unordered_map>

#include "user_group.hpp"

namespace user {

/**
 * \brief User authorization system
 */
class AuthSystem
{
public:
    /**
     * \brief Adds an user to a set of groups
     *
     * If any of the groups doesn't exists, it will be created
     */
    void add_user(const User& user, const std::vector<std::string>& groups)
    {
        for ( const auto& g : groups )
            add_user(user,g);
    }

    void add_user(const User& user, const std::string& group)
    {
        user_groups[group].add_user(user);
    }

    /**
     * \brief Remove a user from the given group
     */
    void remove_user(const User& user, const std::string& group)
    {
        user_groups[group].remove_user(user);
    }


    /**
     * \brief Creates a group with the given name (if it doesn't exists)
     */
    void add_group(const std::string& group)
    {
        user_groups[group];
    }

    /**
     * \brief Grants access from group \c from to group \c to
     */
    void grant_access(const std::string& from, const std::string& to)
    {
        UserGroup* child_group = &user_groups[to];
        user_groups[from].add_child(child_group);
    }

    /**
     * \brief Check if \c user is in \c group
     */
    bool in_group(const User& user, const std::string& group, bool recursive = true)
    {
        return user_groups[group].contains(user, recursive);
    }

    bool in_group(const User& user, const std::string& group,
                  bool recursive = true) const
    {
        auto it = user_groups.find(group);
        if ( it != user_groups.end() )
            return it->second.contains(user, recursive);
        return false;
    }

    /**
     * \brief Gets the list of users with the given authorization level
     */
    std::vector<User> users_with_auth(const std::string& group) const
    {
        auto it = user_groups.find(group);
        if ( it == user_groups.end() )
            return {};
        return it->second.all_users();
    }


private:
    std::unordered_map<std::string,UserGroup> user_groups;
};


} // namespace user
#endif // AUTH_SYSTEM_HPP
