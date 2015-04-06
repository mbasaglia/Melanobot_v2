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
#ifndef COLOR12_HPP
#define COLOR12_HPP

#include <cstdint>
#include <string>

#include "math.hpp"
#include "c++-compat.hpp"

/**
 * \brief Namespace for color operations
 */
namespace color {
/**
 * \brief 12 bit color
 */
class Color12 {
public:
    /**
     * \brief Component type, will use 4 bits
     */
    using Component = uint8_t;

    /**
     * \brief 12 bit integer containing all 3 components
     *
     * 0xfff = white, 0xf00 = red, 0x0f0 = green, 0x00f = blue
     */
    using BitMask = uint16_t;


    Color12() = default;
    Color12(const Color12&) = default;
    Color12(Color12&&) noexcept = default;
    Color12& operator=(const Color12&) = default;
    Color12& operator=(Color12&&) noexcept = default;

    SUPER_CONSTEXPR Color12(BitMask mask)
        : valid(true), r((mask>>4)&0xf), g((mask>>4)&0xf), b(mask&0xf) {}

    /**
     * \brief Creates a color from its rgb components
     */
    SUPER_CONSTEXPR Color12(Component r, Component g, Component b)
        : valid(true), r(validate(r)), g(validate(g)), b(validate(b)) {}
    /**
     * \brief Creates a color from a hex string
     */
    Color12(const std::string& s)
    {
        if ( s.size() >= 3 )
        {
            valid = true;
            r = component_from_hex(s[0]);
            g = component_from_hex(s[1]);
            b = component_from_hex(s[2]);
        }
    }

    /**
     * \brief Whether the color is an actual color or invalid
     */
    SUPER_CONSTEXPR bool is_valid() const { return valid; }

    /**
     * \brief Get the 12 bit mask
     */
    SUPER_CONSTEXPR BitMask to_bit_mask() const
    {
        return (r << 8) | (g << 4) | b;
    }

    /**
     * \brief Compress to 4 bits
     *
     * least to most significant: red green blue bright
     */
    Component to_4bit() const;

    /**
     * \brief Map a 4 bit color
     *
     * least to most significant: red green blue bright
     */
    static Color12 from_4bit(Component color);

    /**
     * \brief Get a color from name
     */
    static Color12 from_name(const std::string& name);

    /**
     * \brief Convert to a html color string
     */
    std::string to_html() const;


    SUPER_CONSTEXPR Component red()      const { return r; };
    SUPER_CONSTEXPR Component green()    const { return g; };
    SUPER_CONSTEXPR Component blue()     const { return b; };
    SUPER_CONSTEXPR char      hex_red()  const { return component_to_hex(r); };
    SUPER_CONSTEXPR char      hex_green()const { return component_to_hex(g); };
    SUPER_CONSTEXPR char      hex_blue() const { return component_to_hex(b); };


    /**
        * \brief Blend two colors together
        * \param c1 First color
        * \param c2 Second color
        * \param factor Blend factor 0 => \c c1, 1 => \c c2
        */
    static SUPER_CONSTEXPR Color12 blend(Color12 c1, Color12 c2, double factor)
    {
        return Color12(math::round(c1.r*(1-factor) + c2.r*factor),
                       math::round(c1.g*(1-factor) + c2.g*factor),
                       math::round(c1.b*(1-factor) + c2.b*factor));
    }

    /**
     * \brief Get a color from HSV components in [0,1]
     */
    static Color12 hsv(double h, double s, double v);

private:
    static SUPER_CONSTEXPR Component validate(Component c) { return c > 0xf ? 0xf : c; }
    static SUPER_CONSTEXPR Component component_from_hex(char c)
    {
        if ( c <= '9' && c >= '0' )
            return c - '0';
        if ( c <= 'f' && c >= 'a' )
            return c - 'a' + 0xa;
        if ( c <= 'F' && c >= 'A' )
            return c - 'A' + 0xa;
        return 0;
    }
    static SUPER_CONSTEXPR char component_to_hex(Component c) { return c < 0xa ? c+'0' : c-0xa+'a'; }

    bool valid  = false;
    Component r = 0;
    Component g = 0;
    Component b = 0;

};

extern Color12 nocolor;
extern Color12 black;
extern Color12 red;
extern Color12 green;
extern Color12 yellow;
extern Color12 blue;
extern Color12 magenta;
extern Color12 cyan;
extern Color12 white;
extern Color12 silver;
extern Color12 gray;
extern Color12 dark_red;
extern Color12 dark_green;
extern Color12 dark_yellow;
extern Color12 dark_blue;
extern Color12 dark_magenta;
extern Color12 dark_cyan;

} // namespace color
#endif // COLOR12_HPP
