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
#ifndef USER_MANAGER_HPP
#define USER_MANAGER_HPP

#include "user.hpp"

#include <list>

namespace user {

/**
 * \brief A class to keep track of users
 */
class UserManager
{
public:
    /**
     * \brief Adds a user
     * \note If a different user with the same \c user.local_id is found, it
     *       will be overwritten
     */
    User* add_user(const User& user);

    /**
     * \brief Get a user by local id
     * \return A pointer to the user or null if not found
     */
    User* user(const std::string& local_id);
    const User* user(const std::string& local_id) const;

    /**
     * \brief Get the first user with the matching property
     * \return A pointer to the user or null if not found
     */
    User* user_by_property(const std::string& property_name, const std::string& property_value);
    const User* user_by_property(const std::string& property_name, const std::string& property_value) const;

    /**
     * \brief Get all users
     */
    std::list<User> users() const { return users_; }

    /**
     * \brief Get all users matching the given condition
     */
    template<class UnaryPredicate>
        std::list<User> users(const UnaryPredicate& predicate) const
        {
            std::list<User> ret;
            for ( const User& u : users_ )
                if ( predicate(u) )
                    ret.push_back(u);
            return ret;
        }

    /**
     * \brief Get all users on the given channel
     */
    std::list<User> channel_users(const std::string& channel) const;

    /**
     * \brief Get all users on the given channel
     */
    std::list<User*> channel_user_pointers(const std::string& channel);

    /**
     * \brief Remove a user by local_id
     * \return \b true on success
     */
    bool remove_user(const std::string& local_id);

    /**
     * \brief Change user id
     * \return \b true on success
     */
    bool change_id(const std::string& old_local_id, const std::string& new_local_id);

    /**
     * \brief Update a user
     *
     * If there's no user matching \c local_id, \c user is inserted
     */
    void update_user(const std::string& local_id, const User& user);

    /**
     * \brief Removes all users
     */
    void clear() { users_.clear(); }

private:
    std::list<User> users_;
};


} // namespace user
#endif // USER_MANAGER_HPP
