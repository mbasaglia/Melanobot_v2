/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright  Mattia Basaglia
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
#ifndef MAYBE_PTR_HPP
#define MAYBE_PTR_HPP

#include <memory>
#include <type_traits>

/**
 * \brief A pointer which might have unique ownership or no ownership at all
 */
template<class T> class MaybePtr
{
public:
    using pointer = T*;
    using element_type = T;

    /**
     * \brief Creates an owning pointer
     */
    template <class... Args>
    static MaybePtr make(Args&&...args)
    {
        return MaybePtr(new T(std::forward<Args>(args)...),true);
    }

    MaybePtr(pointer p = nullptr, bool owns = false) noexcept
        : data(p), ownership(owns) {}

    MaybePtr(const MaybePtr&) = delete;

    /**
     * \brief Transfers ownership
     */
    template<class U>
    MaybePtr(MaybePtr<U>&& rhs) noexcept
        : data(rhs.data), ownership(rhs.ownership)
    {
        rhs.ownership = false;
    }

    MaybePtr& operator=(const MaybePtr&) = delete;

    /**
     * \brief Transfers ownership
     */
    template<class U>
    MaybePtr& operator=(MaybePtr<U>&& rhs) noexcept
    {
        call_deleter();
        data = rhs.data;
        ownership = rhs.ownership;
        rhs.ownership = false;
    }

    ~MaybePtr()
    {
        call_deleter();
    }

    /**
     * \brief Releases ownership
     */
    pointer release() noexcept
    {
        ownership = false;
        return data;
    }

    /**
     * \brief resets the managed object
     */
    void reset(pointer ptr = pointer(), bool owns = false) noexcept
    {
        call_deleter();
        data = ptr;
        ownership = owns;
    }

    /**
     * \brief Swap managed object and ownership
     */
    void swap(MaybePtr& oth) noexcept
    {
        std::swap(data,oth.data);
        std::swap(ownership,oth.ownership);
    }

    /**
     * \brief Get the managed object
     */
    pointer get() const noexcept
    {
        return data;
    }

    /**
     * \brief Returns \b true if it isn't null
     */
    explicit operator bool() const noexcept
    {
        return data;
    }

    /**
     * \brief Whether the pointer owns the object
     */
    bool owns() const noexcept
    {
        return ownership;
    }

    T& operator*() const noexcept
    {
        return *data;
    }

    pointer operator->() const noexcept
    {
        return data;
    }

    bool operator<  (const MaybePtr& rhs) const noexcept { return data < rhs.data; }
    bool operator<= (const MaybePtr& rhs) const noexcept { return data <= rhs.data; }
    bool operator== (const MaybePtr& rhs) const noexcept { return data == rhs.data; }
    bool operator>= (const MaybePtr& rhs) const noexcept { return data >= rhs.data; }
    bool operator>  (const MaybePtr& rhs) const noexcept { return data > rhs.data; }
    bool operator!= (const MaybePtr& rhs) const noexcept { return data != rhs.data; }

private:
    pointer     data{nullptr};
    bool        ownership{false};
    template<class U> friend class MaybePtr;

    /**
     * \brief Maybe deletes the managed object
     *
     * (It doesn't reset \c data to \b nullptr)
     */
    void call_deleter() const noexcept
    {
        if ( ownership )
            delete data;
    }
};



#endif // MAYBE_PTR_HPP
