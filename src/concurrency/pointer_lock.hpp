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

/**
 * \brief Move-only locking RAII class
 *        which does nothing if initialized with a null pointer
 */
template <class Lockable>
class PointerLock
{
public:
    PointerLock(Lockable* target)
        : target(target)
    {
        lock();
    }
    PointerLock(const PointerLock&) = delete;
    PointerLock& operator=(const PointerLock&) = delete;
    PointerLock(PointerLock&& o)
        : target(o.target)
    {
        o.target = nullptr;
    }
    PointerLock& operator=(PointerLock&& o)
    {
        std::swap(o.target,target);
    }

    ~PointerLock()
    {
        unlock();
    }

    void lock()
    {
        if ( target )
            target->lock();
    }

    void unlock()
    {
        if ( target )
            target->unlock();
    }

private:
    Lockable* target;
};

#endif // POINTER_LOCK_HPP
