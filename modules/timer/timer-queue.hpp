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

class TimerItem
{
public:
    network::Time timeout;        ///< Time at which the item will be executed
    std::function<void()> action; ///< Action to be executed
    const void* owner = nullptr;  ///< Pointer to an object that owns the action

    TimerItem(network::Time timeout,
              std::function<void()> action,
              const void* owner = nullptr)
        : timeout(timeout), action(std::move(action)), owner(owner)
    {
    }

    /**
     * \brief Comparator used to create the heap
     */
    bool operator<(const TimerItem& rhs) const
    {
        return timeout > rhs.timeout;
    }
};

class TimerQueue : public melanolib::Singleton<TimerQueue>
{
public:
    void push(TimerItem&& item)
    {
        EditLock lock(this);
        items.emplace_back(std::move(item));
        std::push_heap(items.begin(), items.end());
    }

    template<class... Args>
        void push(Args&&... args)
        {
            push(TimerItem(std::forward<Args>(args)...));
        }

    /**
     * \brief Removes all items owned by the given object
     */
    void remove(const void* owner)
    {
        EditLock lock(this);
        items.erase(
            std::remove_if(items.begin(), items.end(),
                [owner](const TimerItem& item) { return item.owner == owner; }
            ),
            items.end()
        );
    }

private:
    friend ParentSingleton;

    TimerQueue()
    {
        start();
    }

    ~TimerQueue()
    {
        stop();
    }

    void stop()
    {
        if ( thread.joinable() )
        {
            timer_status = Die;
            condition.notify_one();
            thread.join();
        }
    }

    void start()
    {
        if ( !thread.joinable() )
        {
            timer_status = Tick;
            thread = std::thread([this]{run();});
        }
    }

    void run()
    {
        std::mutex condition_mutex;
        while ( true )
        {
            bool empty = true;
            network::Time timeout;

            {
                Lock lock_events(events_mutex);
                empty = items.empty();
                if ( !empty )
                    timeout = items[0].timeout;
            }

            std::unique_lock<std::mutex> lock(condition_mutex);
            if ( !empty )
                condition.wait_until(lock, timeout);
            else
                condition.wait(lock);

            switch( timer_status )
            {
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
        if ( items.empty() )
            return;
        // Handle spurious wake ups
        if ( items[0].timeout > network::Clock::now() )
            return;

        std::pop_heap(items.begin(), items.end());

        TimerItem item = std::move(items.back());
        items.pop_back();
        lock.unlock();

        item.action();
    }

    std::vector<TimerItem> items;
    network::Timer timer;
    std::condition_variable condition;       ///< Wait condition
    std::mutex events_mutex;
    std::thread thread;

    enum TimerStatus
    {
        //Noop,
        Tick,
        Die
    };

    std::atomic<TimerStatus> timer_status;

    /**
     * \brief RAII object for editing the items in the queue
     */
    struct EditLock
    {
        TimerQueue* subject;
        Lock lock;

        explicit EditLock(TimerQueue* subject)
            : subject(subject)
        {
            if ( subject->thread.joinable() )
            {
                lock = Lock(subject->events_mutex);
            }
        }

        ~EditLock()
        {
            if ( subject->thread.joinable() )
            {
                lock.unlock();
                subject->timer_status = Tick;
                subject->condition.notify_one();
            }
        }
    };
};

} // namespace timer
#endif // MELANOBOT_MODULES_TIMER_CONNECTION
