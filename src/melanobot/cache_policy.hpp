/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015-2017 Mattia Basaglia
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
#ifndef CACHE_POLICY_HPP
#define CACHE_POLICY_HPP

#include "settings.hpp"
#include "melanolib/string/stringutils.hpp"

namespace melanobot {

/**
 * \brief Policy to handle cached data
 * \todo Handle DISCARD correctly
 */
class CachePolicy
{
private:
    /**
     * \brief Represents the status of the system using the cache policy
     */
    enum class Status {
        INITIALIZING,
        INITIALIZED,
        LOADED,
        FINISHED
    };

public:
    /**
     * \brief Read policy
     */
    enum class Read {
        ONCE    = 0,    ///< Read once at startup
        LAZY    = 1,    ///< Read once the first time a request is made
        DYNAMIC = 2,    ///< Read each time a request is made
    };
    /**
     * \brief Write policy
     */
    enum class Write {
        ONCE    = 0,    ///< Write once at shutdown
        DYNAMIC = 2,    ///< Write each time a request is made
        DISCARD = 3,    ///< Don't write at all
    };

    /**
     * \brief Converts policy to string
     */
    static std::string to_string(Read read)
    {
        switch ( read )
        {
            default:
            case Read::ONCE:    return "once";
            case Read::LAZY:    return "lazy";
            case Read::DYNAMIC: return "dynamic";
        }
    }
    static std::string to_string(Write read)
    {
        switch ( read )
        {
            default:
            case Write::ONCE:    return "once";
            case Write::DISCARD: return "discard";
            case Write::DYNAMIC: return "dynamic";
        }
    }
    /**
     * \brief Converts a string to a read policy
     */
    static Read to_read_policy(std::string read_policy)
    {
        read_policy = melanolib::string::strtolower(read_policy);
        if ( read_policy == "once" )
            return Read::ONCE;
        else if ( read_policy == "lazy" )
            return Read::LAZY;
        else if ( read_policy == "dynamic" )
            return Read::DYNAMIC;
        return Read::ONCE;
    }
    /**
     * \brief Converts a string to a write policy
     */
    static Write to_write_policy(std::string write_policy)
    {
        write_policy = melanolib::string::strtolower(write_policy);
        if ( write_policy == "once" )
            return Write::ONCE;
        else if ( write_policy == "dynamic" )
            return Write::DYNAMIC;
        else if ( write_policy == "discard" )
            return Write::DISCARD;
        return Write::ONCE;
    }

    constexpr CachePolicy() noexcept = default;
    constexpr CachePolicy(Read read, Write write) noexcept : read_(read), write_(write) {}

    /**
     * \brief Set read/write policies from settings
     */
    void load_settings(const Settings& settings)
    {
        read_ = to_read_policy(settings.get("read", to_string(read_)));
        write_ = to_write_policy(settings.get("write", to_string(write_)));
    }

    /**
     * \brief Marks that the system is being initialized
     */
    SUPER_CONSTEXPR void mark_initializing() noexcept
    {
        status_ = Status::INITIALIZING;
    }

    /**
     * \brief Marks that the system has been fully initialized and following
     *        calls are to be considered dynamic
     */
    SUPER_CONSTEXPR void mark_initialized() noexcept
    {
        status_ = Status::INITIALIZED;
    }

    /**
     * \brief Marks that the system is being finalized and following
     *        calls are no longer to be considered dynamic
     */
    SUPER_CONSTEXPR void mark_finalizing() noexcept
    {
        status_ = Status::FINISHED;
    }

    /**
     * \brief Signals that the cache is in the same state
     *        as if it were just aquired
     */
    SUPER_CONSTEXPR void mark_clean() noexcept
    {
        dirty_ = false;
        if ( status_ < Status::LOADED )
            status_ = Status::LOADED;
    }

    /**
     * \brief Signals that the cached data has been modified
     */
    SUPER_CONSTEXPR void mark_dirty() noexcept
    {
        dirty_ = true;
    }

    /**
     * \brief Returns whether the cached data should be updated
     *
     * This is for dynamic calls
     */
    SUPER_CONSTEXPR bool should_read() const noexcept
    {
        switch ( read_ )
        {
            default:
            case Read::ONCE:    return status_ <= Status::INITIALIZED;
            case Read::LAZY:    return status_ == Status::INITIALIZED;
            case Read::DYNAMIC: return status_ >= Status::INITIALIZED;
        }
    }

    /**
     * \brief Returns whether the cached data should be writted to its destination
     *
     * This is for dyamic calls
     */
    SUPER_CONSTEXPR bool should_write() const noexcept
    {
        switch ( write_ )
        {
            default:
            case Write::ONCE:    return dirty_ && status_ == Status::FINISHED;
            case Write::DYNAMIC: return dirty_;
            case Write::DISCARD: return false;
        }
    }

    /**
     * \brief The policy used for reading data
     */
    constexpr Read read() const noexcept { return read_; }

    /**
     * \brief The policy used for writing data
     */
    constexpr Write write() const noexcept { return write_; }

private:
    Read   read_  = Read::ONCE;         ///< Policy used for reading data
    Write  write_ = Write::ONCE;        ///< Policy used for writing data
    bool   dirty_ = false;              ///< Whether cached data has been modified locally
    Status status_= Status::INITIALIZING;///< Whether cached data has been loaded
};

} // namespace melanobot
#endif // CACHE_POLICY_HPP
