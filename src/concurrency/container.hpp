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
#ifndef CONCURRENT_CONTAINER_HPP
#define CONCURRENT_CONTAINER_HPP

#include <algorithm>
#include <functional>
#include <queue>
#include <vector>

#include "concurrency.hpp"

/**
 * \brief Container wrapper which makes it fit for concurrent use
 *
 * To be used with one consumer and multiple producers.
 * It only provides utilities for insertion and extraction.
 */
template <class Container>
class ConcurrentContainer
{
public:
    using container_type = Container;
    using value_type     = typename Container::value_type;
    using reference      = typename Container::reference;
    using const_reference= typename Container::const_reference;
    using method_push    = void(Container::*)(const_reference);
    using method_pop     = void(Container::*)();
    using method_get     = const_reference(Container::*)() const;

    ConcurrentContainer(method_push container_push,
                        method_pop  container_pop,
                        method_get  container_get
    ) : container_push(container_push), container_pop(container_pop), container_get(container_get)
    {}
    ConcurrentContainer(const ConcurrentContainer&) = delete;
    ConcurrentContainer(ConcurrentContainer&&) = delete;
    ConcurrentContainer& operator=(const ConcurrentContainer&) = delete;
    ConcurrentContainer& operator=(ConcurrentContainer&&) = delete;

    /**
     * \brief Add an element to the container
     *
     * Acquires a lock, inserts the item and notifies the consumer.
     *
     * If active() is not true, it will discard any input
     */
    void push (const_reference item)
    {
        if ( !run ) return;
        std::lock_guard<std::mutex> lock(mutex);
        (container.*container_push)(item);
        condition.notify_one();
    }

    /**
     * \brief Retrieve an element from the container
     *
     * Waits until there are elements to get or active() is false
     */
    void pop(reference item)
    {
        std::unique_lock<std::mutex> lock(mutex);
        condition.wait(lock, [this]{return !wait_condition();});
        if ( !run )
            return;
        item = (container.*container_get)();
        (container.*container_pop)();
    }

    /**
     * \brief Whether the container is allowed to process data
     */
    bool active() const { return run; }

    /**
     * \brief Starts the container
     * \note This is the state after construction
     * \post active() is true
     */
    void start()
    {
        run = true;
    }

    /**
     * \brief Stops the container
     * \post active() is false
     */
    void stop()
    {
        run = false;
        condition.notify_all();
    }

    /**
     * \brief Returns a reference to the container without any locks
     */
    Container& get_container() { return container; }

    /**
     * \brief Removes all elements matching \c pred from the container
     * \tparam Predicate Functor taking \c value_type and returning \b bool
     */
    template<class Predicate>
    void remove_if(const Predicate& pred)
    {
        auto lock = make_lock(mutex);

        Container swapped;
        std::swap(swapped, container);

        value_type value;
        while ( !swapped.empty() )
        {
            value = (swapped.*container_get)();
            (swapped.*container_pop)();
            if ( !pred(value) )
                (container.*container_push)(value);
        }
    }

private:
    method_push container_push;
    method_pop  container_pop;
    method_get  container_get;
    Container   container;

    std::atomic<bool> run {true};
    std::mutex mutex;
    std::condition_variable condition;

    bool wait_condition() const
    {
        return container.empty() && run;
    }
};

/**
 * \brief Makes a std::queue suitable for concurrency
 */
template<class T, class Container = std::deque<T>>
class ConcurrentQueue : public ConcurrentContainer<std::queue<T, Container>>
{
    using parent_type = ConcurrentContainer<std::queue<T, Container>>;

public:
    using container_type = typename parent_type::container_type;

    ConcurrentQueue() : parent_type( &container_type::push, &container_type::pop, &container_type::front )
    {}
};

/**
 * \brief Makes a std::priority_queue suitable for concurrency
 */
template<class T,
         class Container = std::vector<T>,
         class Compare = std::less<typename Container::value_type> >
class ConcurrentPriorityQueue : public ConcurrentContainer<std::priority_queue<T, Container, Compare>>
{
    using parent_type = ConcurrentContainer<std::priority_queue<T, Container, Compare>>;

public:
    using container_type = typename parent_type::container_type;

    ConcurrentPriorityQueue() : parent_type( &container_type::push, &container_type::pop, &container_type::top )
    {}
};

#endif // CONCURRENT_CONTAINER_HPP
