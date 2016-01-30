/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015-2016 Mattia Basaglia
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
#ifndef MELANOBOT_HPP
#define MELANOBOT_HPP

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>

#include "concurrency/container.hpp"
#include "settings.hpp"
#include "message/message_consumer.hpp"

namespace melanobot {

/**
 * \brief Main bot class
 */
class Melanobot : public MessageConsumer, public melanolib::Singleton<Melanobot>
{
public:
    ~Melanobot();

    /**
     * \brief Runs the bot
     * \thread main \lock messages(not continuous)
     */
    void run();

    /**
     * \brief Stops the bot
     * \thread external \lock none
     */
    void stop();

    /**
     * \brief Inform the bot there's an incoming message
     * \thread main \lock messages
     */
    void message(const network::Message& msg);

    /**
     * \brief Get connection by name
     * \return \b nullptr if not found
     * \note The returned pointer will become invalid afther
     *       the Melanobot object is destructed
     */
    network::Connection* connection(const std::string& name) const;

    /**
     * \brief Adds a connection from settings
     */
    void add_connection(std::string suggested_name, const Settings& settings);

    void add_handler(std::unique_ptr<melanobot::Handler>&& handler) override;

    void populate_properties(const std::vector<std::string>& property, PropertyTree& output) const override;

private:
    explicit Melanobot();
    friend ParentSingleton;
    
    bool handle(network::Message& msg) override;

    /// \todo allow dynamic connection/handler creation (requires locking)
    std::unordered_map<std::string, std::unique_ptr<network::Connection>> connections;
    std::vector<std::unique_ptr<melanobot::Handler>> handlers;

    /// Message Queue
    ConcurrentQueue<network::Message> messages;
};

} // namespace melanobot
#endif // MELANOBOT_HPP
