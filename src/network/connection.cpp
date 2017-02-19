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
#include "connection.hpp"

namespace network {

void ConnectionFactory::register_connection(const std::string& protocol_name,
                                            const Contructor& function)
{
    if ( factory.count(protocol_name) )
        throw melanobot::ConfigurationError("Re-registering connection protocol "+protocol_name);
    factory[protocol_name] = function;
}

std::unique_ptr<Connection> ConnectionFactory::create(
    const Settings& settings,
    const std::string& name)
{
    try
    {
        std::string protocol = settings.get("protocol", std::string());
        auto it = factory.find(protocol);
        if ( it != factory.end() )
        {
            if ( !settings.get("enabled", true) )
            {
                Log("sys", '!') << "Skipping disabled connection " << color::red << name;
                return nullptr;
            }

            Log("sys", '!') << "Creating connection " << color::dark_green << name;
            return it->second(settings, name);
        }
        ErrorLog ("sys", "Connection Error") << ": Unknown connection protocol "+protocol;
    }
    catch ( const melanobot::MelanobotError& exc )
    {
        ErrorLog ("sys", "Connection Error") << exc.what();
    }

    return nullptr;
}


void AuthConnection::setup_auth(const Settings& settings)
{
    for ( const auto pt: settings.get_child("users", {}) )
    {
        add_to_group(pt.first, pt.second.data());
    }

    auto groups = settings.get_child_optional("groups");
    if ( groups )
        for ( const auto pt: *groups )
        {
            auth_system.add_group(pt.first);
            auto inherits = melanolib::string::comma_split(pt.second.data());
            for ( const auto& inh : inherits )
                auth_system.grant_access(inh, pt.first);
        }
}


/// \todo change this to accept a reference to user::User instead of local_id?
bool AuthConnection::user_auth(const std::string& local_id,
                              const std::string& auth_group) const
{
    if ( auth_group.empty() )
        return true;

    auto lock = make_lock(mutex);
    const user::User* user = user_manager.user(local_id);
    if ( user )
        return auth_system.in_group(*user, auth_group);

    return auth_system.in_group(build_user(local_id), auth_group);
}

void AuthConnection::update_user(const std::string& local_id,
                                const Properties& properties)
{
    auto lock = make_lock(mutex);
    user::User* user = user_manager.user(local_id);
    if ( user )
    {
        user->update(properties);

        auto it = properties.find("global_id");
        if ( it != properties.end() )
            Log(protocol(), '!', 3)
                << "User " << color::dark_cyan << user->local_id
                << color::nocolor << " is authed as "
                << color::cyan << it->second;
    }
}

void AuthConnection::update_user(const std::string& local_id,
                                const user::User& updated)
{
    auto lock = make_lock(mutex);
    user::User* user = user_manager.user(local_id);
    if ( user )
    {
        if ( !updated.global_id.empty() && updated.global_id != user->global_id )
            Log(protocol(), '!', 3)
                << "User " << color::dark_cyan << updated.local_id
                << color::nocolor << " is authed as "
                << color::cyan << updated.global_id;

        *user = updated;
    }
}

user::User AuthConnection::get_user(const std::string& local_id) const
{
    auto lock = make_lock(mutex);
    const user::User* user = user_manager.user(local_id);
    if ( user )
        return *user;
    return {};
}

std::vector<user::User> AuthConnection::get_users(const std::string& channel) const
{
    auto lock = make_lock(mutex);
    auto normalized_channel = normalize_channel(channel);

    std::list<user::User> list;
    if ( channel.empty() )
        list = user_manager.users();
    else if ( is_private_channel(channel) )
        list = { *user_manager.user(normalized_channel) };
    else
        list = user_manager.channel_users(normalized_channel);

    return std::vector<user::User>(list.begin(), list.end());
}

bool AuthConnection::add_to_group(const std::string& username,
                                  const std::string& group)
{
    std::vector<std::string> groups = melanolib::string::comma_split(group);

    if ( groups.empty() || username.empty() )
        return false;

    user::User user = build_user(username);

    if ( !groups.empty() )
    {
        auto lock = make_lock(mutex);
        groups.erase(std::remove_if(groups.begin(), groups.end(),
                        [this, user](const std::string& str)
                        { return auth_system.in_group(user, str); }),
                    groups.end()
        );
        if ( !groups.empty() && auth_system.add_user(user, groups) )
        {
            Log(protocol(), '!', 3) << "Registered user " << color::cyan
                << username << color::nocolor << " in "
                << melanolib::string::implode(", ", groups);
            return true;
        }
    }
    return false;
}

bool AuthConnection::remove_from_group(const std::string& username, const std::string& group)
{
    if ( group.empty() || username.empty() )
        return false;

    user::User user = build_user(username);

    auto lock = make_lock(mutex);
    if ( auth_system.in_group(user, group, false) )
    {
        auth_system.remove_user(user, group);
        return true;
    }

    return false;
}

std::vector<user::User> AuthConnection::users_in_group(const std::string& group) const
{
    auto lock = make_lock(mutex);
    return auth_system.users_with_auth(group);
}
std::vector<user::User> AuthConnection::real_users_in_group(const std::string& group) const
{
    auto lock = make_lock(mutex);

    auto user_group = auth_system.group(group);
    if ( !user_group )
        return {};

    std::vector<user::User> users;
    for ( const user::User& user : user_manager )
        if ( user_group->contains(user, true) )
            users.push_back(user);
    return users;
}

bool AuthConnection::channel_mask(const std::vector<std::string>& channels,
                                 const std::string& mask) const
{
    std::vector<std::string> masks = melanolib::string::comma_split(mask);
    for ( const auto& m : masks )
    {
        for ( const auto& chan : channels )
            if ( normalize_channel(m) == normalize_channel(chan) )
                return true;
    }
    return false;
}


} // namespace network
