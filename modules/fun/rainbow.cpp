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

#include "rainbow.hpp"
#include "string/encoding.hpp"

namespace fun {

std::string FormatterRainbow::name() const
{
    return "rainbow";
}

void FormatterRainbow::decode_ascii(string::DecodeEnvironment& env, uint8_t byte) const
{
    colors.push_back(std::make_shared<string::Color>(color::nocolor));
    env.output.append(colors.back());
    env.output.append<string::Character>(byte);
}
void FormatterRainbow::decode_unicode(string::DecodeEnvironment& env, const string::Unicode& unicode) const
{
    colors.push_back(std::make_shared<string::Color>(color::nocolor));
    env.output.append(colors.back());
    env.output.append<string::Unicode>(unicode);
}
void FormatterRainbow::decode_end(string::DecodeEnvironment& env) const
{
    for ( unsigned i = 0; i < colors.size(); i++ )
        *colors[i] = color::Color12::hsv(hue+double(i)/colors.size(),saturation,value);
    colors.clear();
}

} // namespace fun
