/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015 Mattia Basaglia
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

/**
 * \brief Main bot class
 */
class Melanobot : public MessageConsumer
{
public:
    ~Melanobot();

    /**
     * \brief Returns the singleton instance
     * \pre initialize() has been called
     */
    static Melanobot& instance();

    /**
     * \brief Initializes the singleton instance
     * \pre initialize() has never been called
     * \post instance() can be called
     */
    static Melanobot& initialize(const Settings& settings);

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

    void populate_properties(const std::vector<std::string>& property, PropertyTree& output) const override;

    /**
     * \brief Returns handler template from name
     */
    Settings get_template(const std::string& name) const;

private:
    explicit Melanobot(const Settings& settings);
    Melanobot(const Melanobot&) = delete;
    Melanobot(Melanobot&&) = delete;
    Melanobot& operator=(const Melanobot&) = delete;
    Melanobot& operator=(Melanobot&&) = delete;
    
    bool handle(network::Message& msg) override;

    friend std::unique_ptr<Melanobot> std::make_unique<Melanobot, const Settings&>(const Settings& settings);
    /// Singleton instance
    static std::unique_ptr<Melanobot> singleton;

    /// \todo allow dynamic connection/handler creation (requires locking)
    std::unordered_map<std::string,std::unique_ptr<network::Connection>> connections;
    std::vector<std::unique_ptr<handler::Handler>> handlers;

    /// Message Queue
    ConcurrentQueue<network::Message> messages;

    /// Settings containing configuration template definitions
    Settings templates;
};


#endif // MELANOBOT_HPP
