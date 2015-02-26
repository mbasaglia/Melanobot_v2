/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015 Mattia Basaglia
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
#ifndef COLOR12_HPP
#define COLOR12_HPP

#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <string>
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
    typedef uint8_t  Component;

    /**
     * \brief 12 bit integer containing all 3 components
     *
     * 0xfff = white, 0xf00 = red, 0x0f0 = green, 0x00f = blue
     */
    typedef uint16_t BitMask;


    Color12() = default;

    Color12(BitMask mask)
        : valid(true), r((mask>>4)&0xf), g((mask>>4)&0xf), b(mask&0xf) {}

    /**
     * \brief Creates a color from its rgb components
     */
    Color12(Component r, Component g, Component b)
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
     * \brief Creates a color from a DP color string ^. or ^x...
     */
    static Color12 from_dp(const std::string& color);

    /**
     * \brief Creates a color from an IRC color string \3..
     */
    static Color12 from_irc(const std::string& color);

    bool is_valid() const { return valid; }

    /**
     * \brief Get the 12 bit mask
     */
    BitMask to_bit_mask() const
    {
        return (r << 8) | (g << 4) | b;
    }

    /**
     * \brief Covert to a dp color string
     */
    std::string to_dp() const;

    /**
     * \brief Covert to an irc color string
     */
    std::string to_irc() const;

    /**
     * \brief Compress to 4 bits
     *
     * least to most significant: red green blue bright
     */
    Component to_4bit() const;

    /**
     * \brief Convert to an ansi color string
     */
    std::string to_ansi() const;

    /**
     * \brief Convert to a html color string
     */
    std::string to_html() const;

private:
    static Component validate(Component c) { return c > 0xf ? 0xf : c; }
    static Component component_from_hex(char c) { return std::strtol(&c, nullptr, 16); }
    static char component_to_hex(Component c) { return c < 0xa ? c+'0' : c-0xa+'a'; }

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
