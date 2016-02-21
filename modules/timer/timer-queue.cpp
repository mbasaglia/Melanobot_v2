/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015-2016 Mattia Basaglia
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
#include "timer-queue.hpp"

namespace timer {

/**
 * \brief RAII object for editing the items in the queue
 */
struct TimerQueue::EditLock
{
    TimerQueue* subject;
    std::unique_lock<std::mutex> lock;

    explicit EditLock(TimerQueue* subject)
        : subject(subject)
    {
        if ( subject->thread.joinable() )
        {
            subject->timer_action = TimerAction::Noop;
            subject->condition.notify_one();
            lock = std::unique_lock<std::mutex>(subject->events_mutex);
        }
    }

    ~EditLock()
    {
        if ( subject->thread.joinable() )
        {
            lock.unlock();
            subject->timer_action = TimerAction::Tick;
            subject->condition.notify_one();
        }
    }
};

void TimerQueue::push(TimerItem&& item)
{
    EditLock lock(this);
    items.emplace_back(std::move(item));
    std::push_heap(items.begin(), items.end());
}

void TimerQueue::remove(const void* owner)
{
    EditLock lock(this);
    items.erase(
        std::remove_if(items.begin(), items.end(),
            [owner](const TimerItem& item) { return item.owner == owner; }
        ),
        items.end()
    );
    std::make_heap(items.begin(), items.end());
}

void TimerQueue::stop()
{
    if ( thread.joinable() )
    {
        timer_action = TimerAction::Die;
        condition.notify_one();
        thread.join();
    }
}

void TimerQueue::start()
{
    if ( !thread.joinable() )
    {
        timer_action = TimerAction::Tick;
        thread = std::thread([this]{run();});
    }
}

void TimerQueue::run()
{
    std::mutex condition_mutex;
    while ( true )
    {
        auto lock = make_lock(events_mutex);

        if ( !items.empty() )
        {
            if ( tick(lock) )
                continue;
            condition.wait_until(lock, items[0].timeout);
        }
        else
        {
            condition.wait(lock);
        }

        switch( timer_action )
        {
            case TimerAction::Tick:
                tick(lock);
                break;
            case TimerAction::Die:
                return;
            case TimerAction::Noop:
                lock.unlock();
                std::this_thread::yield();
                break;
        }
    }
}

bool TimerQueue::tick(std::unique_lock<std::mutex>& lock)
{
    if ( items.empty() )
        return false;

    // Handle spurious wake ups
    if ( items[0].timeout > network::Clock::now() )
        return false;

    std::pop_heap(items.begin(), items.end());

    TimerItem item = std::move(items.back());
    items.pop_back();
    lock.unlock();

    item.action();
    return true;
}

} // namespace timer

