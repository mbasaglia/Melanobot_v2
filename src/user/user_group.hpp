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
#ifndef USER_GROUP_HPP
#define USER_GROUP_HPP

#include <vector>

#include "user.hpp"

namespace user {

/**
 * \brief A group of users
 */
class UserGroup
{
public:
    /**
     * \brief Add a user to the group
     *
     * The way users are matched depends on which attributes \c user has set.
     *
     * In order of priority, it will look for: \c global_id, \c host, \c name
     *
     * If a previously added user matches \c user, it won't be inserted
     */
    void add_user ( const User& user );

    /**
     * \brief Remove the user from the group
     *
     * Removes all matching users.
     */
    void remove_user ( const User& user );

    /**
     * \brief Whether the user is in the group or any of its children
     * \param user      User to be checked
     * \param recursive Whether to check inherited groups
     */
    bool contains ( const User& user, bool recursive ) const;

    /**
     * \brief Adds a child group
     *
     * The child group will inherit the same access as this group
     *
     * \note Always keep the structure as a tree, not doing so will mess up
     */
    void add_child ( UserGroup* child );

    /**
     * \brief Users directly in this group
     */
    std::vector<User> direct_users() const;

    /**
     * \brief Users in this group or in its children
     */
    std::vector<User> all_users() const;


private:
    std::vector<User>           users;   ///< List of users
    std::vector<UserGroup*>     children;///< Groups which inherit this group
};

} // namespace user
#endif // USER_GROUP_HPP
