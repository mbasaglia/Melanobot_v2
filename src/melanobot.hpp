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
#ifndef MELANOBOT_HPP
#define MELANOBOT_HPP

#include <atomic>
#include <condition_variable>
#include <list>
#include <mutex>
#include <queue>
#include <thread>

#include "settings.hpp"
#include "network/connection.hpp"

/**
 * \brief Main bot class
 */
class Melanobot
{
public:
    explicit Melanobot(const Settings& settings);
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

private:
    /**
     * \brief Runs a network::Connection in its own thread
     */
    class Connection
    {
    public:
        explicit Connection(network::Connection* connection)
            : connection(connection)
        {}

        void start()
        {
            thread = std::move(std::thread([this]{connection->run();}));
        }

        void stop()
        {
            connection->quit();
            if ( thread.joinable() )
                thread.join();
        }

        network::Connection* connection;
        std::thread thread;
    };

    std::list<Connection> connections;

    std::queue<network::Message> messages;

    std::atomic<bool> keep_running {true};
    std::mutex mutex;
    std::condition_variable condition;

    /**
     * \brief Extract a message from the queue
     * \thread main \lock messages
     */
    void get_message(network::Message& msg);
};


#endif // MELANOBOT_HPP
