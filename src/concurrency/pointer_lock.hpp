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
#ifndef POINTER_LOCK_HPP
#define POINTER_LOCK_HPP

#include <algorithm>
#include "melanolib/utils/c++-compat.hpp"

/**
 * \brief Type-erased mutex reference holder
 */
class ErasedMutex
{
private:
    class HolderBase
    {
    public:
        virtual ~HolderBase(){}
        virtual void lock() = 0;
        virtual void unlock() = 0;
        virtual bool try_lock() = 0;
    };

    template<class Lockable>
        class Holder : public HolderBase
        {
        public:
            Holder(Lockable& lockable)
                : lockable(lockable)
            {}

            void lock() override
            {
                lockable.lock();
            }

            void unlock() override
            {
                lockable.unlock();
            }

            bool try_lock() override
            {
                return lockable.try_lock();
            }

        private:
            Lockable& lockable;
        };

public:
    template<class Lockable>
        ErasedMutex(Lockable& lockable)
            : holder(melanolib::New<Holder<Lockable>>(lockable))
        {}

    template<class Lockable>
        ErasedMutex(Lockable* lockable)
            : ErasedMutex(*lockable)
        {}

    ErasedMutex(std::nullptr_t)
    {}

    ErasedMutex()
    {}

    void lock()
    {
        if ( holder )
            holder->lock();
    }

    void unlock()
    {
        if ( holder )
            holder->unlock();
    }

    bool try_lock()
    {
        if ( holder )
            return holder->try_lock();
        return false;
    }

private:
    std::unique_ptr<HolderBase> holder;
};

#endif // POINTER_LOCK_HPP
