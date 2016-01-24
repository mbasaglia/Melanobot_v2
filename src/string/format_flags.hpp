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
#ifndef FORMAT_FLAGS_HPP
#define FORMAT_FLAGS_HPP

namespace string {

/**
 * \brief String formatting flags
 */
class FormatFlags
{
public:
    enum FormatFlagsEnum {
        NO_FORMAT = 0,
        BOLD      = 1,
        UNDERLINE = 2,
        ITALIC    = 4,
    };
    constexpr FormatFlags(FormatFlagsEnum flags) noexcept : flags(flags) {}

    constexpr FormatFlags() noexcept = default;

    explicit constexpr operator bool() const noexcept { return flags; }

    constexpr FormatFlags operator| ( const FormatFlags& o ) const noexcept
    {
        return flags|o.flags;
    }

    constexpr FormatFlags operator& ( const FormatFlags& o ) const noexcept
    {
        return flags&o.flags;
    }
    constexpr FormatFlags operator& ( int o ) const noexcept
    {
        return flags&o;
    }

    constexpr FormatFlags operator^ ( const FormatFlags& o ) const noexcept
    {
        return flags^o.flags;
    }

    constexpr FormatFlags operator~ () const noexcept
    {
        return ~flags;
    }

    constexpr bool operator== ( const FormatFlags& o ) const noexcept
    {
        return flags == o.flags;
    }

    FormatFlags& operator|= ( const FormatFlags& o ) noexcept
    {
        flags |= o.flags;
        return *this;
    }

    FormatFlags& operator&= ( const FormatFlags& o ) noexcept
    {
        flags &= o.flags;
        return *this;
    }

    FormatFlags& operator&= ( int o ) noexcept
    {
        flags &= o;
        return *this;
    }

    FormatFlags& operator^= ( const FormatFlags& o ) noexcept
    {
        flags ^= o.flags;
        return *this;
    }

    friend constexpr FormatFlags operator| (
        FormatFlags::FormatFlagsEnum a, FormatFlags::FormatFlagsEnum b ) noexcept
    {
        return FormatFlags(int(a)|int(b));
    }

private:
    constexpr FormatFlags(int flags) noexcept : flags(flags) {}

    int flags = 0;
};

} // namespace string
#endif // FORMAT_FLAGS_HPP
