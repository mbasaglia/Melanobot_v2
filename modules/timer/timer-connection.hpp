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
#ifndef MELANOBOT_MODULES_TIMER_CONNECTION
#define MELANOBOT_MODULES_TIMER_CONNECTION

#include "network/connection.hpp"
#include "network/time.hpp"
#include "functional.hpp"
#include "concurrency/concurrency.hpp"

namespace timer {
using namespace network;

class TimerItem
{
public:
    Time        timeout;
    std::string message;

    /**
     * \brief Comparator used to create the heap
     */
    bool operator<(const TimerItem& rhs) const
    {
        return timeout > rhs.timeout;
    }
};

class TimerConnection : public network::SingleUnitConnection
{
public:

    static std::unique_ptr<TimerConnection> create(
        const Settings& settings, const std::string& name)
    {
        return melanolib::New<TimerConnection>(name);
    }

    explicit TimerConnection(const std::string& name)
        : SingleUnitConnection(name), timer{[this]{tick();}}
    {}

    ~TimerConnection()
    {
        stop();
    }

    Server server() const override { return {}; }

    std::string description() const override
    {
        return protocol();
    }

    void command ( Command cmd ) override
    {
        if ( cmd.parameters.empty() )
            return;
        push({cmd.timeout, cmd.parameters[0]});
    }

    void say ( const OutputMessage& message ) override
    {
        push({message.timeout, message.message.encode(formatter())});
    }

    Status status() const override { return CONNECTED; }

    std::string protocol() const override { return "timer"; }

    void connect() override {}

    void disconnect(const std::string& message = {}) override {}

    void reconnect(const std::string& quit_message = {}) {}

    void stop() override
    {
        if ( thread.joinable() )
        {
            timer_status = Die;
            condition.notify_one();
            thread.join();
        }
    }

    void start() override
    {
        if ( !thread.joinable() )
        {
            timer_status = Tick;
            thread = std::thread([this]{run();});
        }
    }

    string::Formatter* formatter() const override
    {
        return string::Formatter::formatter("config");
    }

    LockedProperties properties() override
    {
        static PropertyTree props;
        return {nullptr, &props};
    }

    Properties pretty_properties() const override
    {
        return {};
    }

private:
    void run()
    {
        std::mutex condition_mutex;
        while ( true )
        {
            bool empty = true;
            Time timeout;

            {
                Lock lock_events(events_mutex);
                empty = events.empty();
                if ( !empty )
                    timeout = events[0].timeout;
            }

            std::unique_lock<std::mutex> lock(condition_mutex);
            if ( !empty )
                condition.wait_until(lock, timeout);
            else
                condition.wait(lock);

            switch( timer_status )
            {
                case Noop:
                    condition.wait(lock);
                case Tick:
                    tick();
                case Die:
                    return;
            }
        }
    }

    void tick()
    {
        Lock lock(events_mutex);
        if ( events.empty() )
            return;
        // Handle spurious wake ups
        if ( events[0].timeout > Clock::now() )
            return;

        std::pop_heap(events.begin(), events.end());

        TimerItem cmd = std::move(events.back());
        events.pop_back();
        lock.unlock();

        /// \todo send message
    }

    void push(TimerItem&& item)
    {
        timer_status = Noop;
        condition.notify_one();
        Lock lock(events_mutex);
        events.emplace_back(std::move(item));
        std::push_heap(events.begin(), events.end());
        lock.unlock();
        timer_status = Tick;
        condition.notify_one();
    }

    std::vector<TimerItem> events;
    Timer timer;
    std::condition_variable condition;       ///< Wait condition
    std::mutex events_mutex;
    std::thread thread;

    enum TimerStatus
    {
        Noop, Tick, Die
    };

    std::atomic<TimerStatus> timer_status;
};

} // namespace timer
#endif // MELANOBOT_MODULES_TIMER_CONNECTION
