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
#include "color.hpp"

#include <algorithm>
namespace color {

Color12 nocolor;
Color12 black           = Color12(0x0, 0x0, 0x0);
Color12 red             = Color12(0xf, 0x0, 0x0);
Color12 green           = Color12(0x0, 0xf, 0x0);
Color12 yellow          = Color12(0xf, 0xf, 0x0);
Color12 blue            = Color12(0x0, 0x0, 0xf);
Color12 magenta         = Color12(0xf, 0x0, 0xf);
Color12 cyan            = Color12(0x0, 0xf, 0xf);
Color12 white           = Color12(0xf, 0xf, 0xf);
Color12 silver          = Color12(0xc, 0xc, 0xc);
Color12 gray            = Color12(0x8, 0x8, 0x8);
Color12 dark_red        = Color12(0x8, 0x0, 0x0);
Color12 dark_green      = Color12(0x0, 0x8, 0x0);
Color12 dark_yellow     = Color12(0x8, 0x8, 0x0);
Color12 dark_blue       = Color12(0x0, 0x0, 0x8);
Color12 dark_magenta    = Color12(0x8, 0x0, 0x8);
Color12 dark_cyan       = Color12(0x0, 0x8, 0x8);

Color12 Color12::from_dp(const std::string& color)
{
    if ( color.size() < 2 || color[0] != '^' )
        return Color12();
    if ( color[1] == 'x' && color.size() == 5 )
        return Color12(color.substr(2));

    switch (color[1])
    {
        case '0': return black;
        case '1': return red;
        case '2': return green;
        case '3': return yellow;
        case '4': return blue;
        case '5': return cyan;
        case '6': return magenta;
        case '7': return white;
        case '8': return gray;
        case '9': return silver;
    }
    return Color12();
}


Color12 Color12::from_irc(const std::string& color)
{
    if ( color.size() < 2 || color[0] != '\3' || !std::isdigit(color[1]) )
        return Color12();

    int n = std::strtol(color.c_str()+1,nullptr,10);
    switch ( n )
    {
        case  0: return white;
        case  1: return black;
        case  2: return dark_blue;
        case  3: return dark_green;
        case  4: return red;
        case  5: return dark_red;
        case  6: return dark_magenta;
        case  7: return dark_yellow;
        case  8: return yellow;
        case  9: return green;
        case 10: return dark_cyan;
        case 11: return cyan;
        case 12: return blue;
        case 13: return magenta;
        case 14: return gray;
        case 15: return silver;
        default: return Color12();
    }
}


std::string Color12::to_dp() const
{
    switch ( to_bit_mask() )
    {
        case 0x000: return "^0";
        case 0xf00: return "^1";
        case 0x0f0: return "^2";
        case 0xff0: return "^3";
        case 0x00f: return "^4";
        case 0xf0f: return "^6";
        case 0x0ff: return "^5";
        case 0xfff: return "^7";
        case 0x888: return "^8";
        case 0xccc: return "^9";
    }
    return std::string("^x")+component_to_hex(r)+component_to_hex(g)+component_to_hex(b);
}

Color12::Component Color12::to_4bit() const
{
    if ( !valid )
        return 0xf0;

    Component color = 0;

    Component cmax = std::max({r,g,b});
    Component cmin = std::min({r,g,b});
    Component delta = cmax-cmin;

    if ( delta > 0 )
    {
        float hue = 0;
        if ( r == cmax )
            hue = float(g-b)/delta;
        else if ( g == cmax )
            hue = float(b-r)/delta + 2;
        else if ( b == cmax )
            hue = float(r-g)/delta + 4;

        float sat = float(delta)/cmax;
        if ( sat >= 0.3 )
        {
            if ( hue < 0 )
                hue += 6;

            if ( hue <= 0.5 )      color = 1; // red
            else if ( hue <= 1.5 ) color = 3; // yellow
            else if ( hue <= 2.5 ) color = 2; // green
            else if ( hue <= 3.5 ) color = 6; // cyan
            else if ( hue <= 4.5 ) color = 4; // blue
            else if ( hue <= 5.5 ) color = 5; // magenta
            else                   color = 1; // red
        }
        else if ( cmax > 7 )
            color = 7;
    }

    if ( cmax > 9 )
        color |= 8; // bright

    return color;
}

std::string Color12::to_irc() const
{
    int ircn = 1;

    if ( !valid )
        return "\xf"; // no color


    switch ( to_4bit() )
    {
        case 0b0000: ircn =  1; break; // black
        case 0b0001: ircn =  5; break; // dark red
        case 0b0010: ircn =  3; break; // dark green
        case 0b0011: ircn =  7; break; // dark yellow
        case 0b0100: ircn =  2; break; // dark blue
        case 0b0101: ircn =  6; break; // dark magenta
        case 0b0110: ircn = 10; break; // dark cyan
        case 0b0111: ircn = 15; break; // silver
        case 0b1000: ircn = 14; break; // grey
        case 0b1001: ircn =  4; break; // red
        case 0b1010: ircn =  9; break; // green
        case 0b1011: ircn =  8; break; // yellow
        case 0b1100: ircn = 12; break; // blue
        case 0b1101: ircn = 13; break; // magenta
        case 0b1110: ircn = 11; break; // cyan
        case 0b1111: ircn =  0; break; // white
    }
    return '\3'+std::to_string(ircn);
}

std::string Color12::to_ansi() const
{
    if ( !valid )
        return "\x1b[0m";

    Component c4b = to_4bit();
    return "\x1b[3"+std::to_string(c4b&~8)+";"+(c4b&8 ? "1" : "22")+"m";
}


std::string Color12::to_html() const
{
    return std::string()+'#'+component_to_hex(r)+component_to_hex(g)+component_to_hex(b);
}

} // namespace color
