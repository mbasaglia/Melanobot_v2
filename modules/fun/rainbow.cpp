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

#include "rainbow.hpp"

namespace fun {

string::FormattedString FormatterRainbow::decode(const std::string& source) const
{
    string::FormattedString str;

    string::Utf8Parser parser;
    std::vector<std::shared_ptr<string::Color>> colors;

    parser.callback_ascii = [&str,&colors](uint8_t byte)
    {
        colors.push_back(std::make_shared<string::Color>(color::nocolor));
        str.append(colors.back());
        str.append<string::Character>(byte);
    };
    parser.callback_utf8 = [&str,&colors](uint32_t unicode,const std::string& utf8)
    {
        colors.push_back(std::make_shared<string::Color>(color::nocolor));
        str.append(colors.back());
        str.append<string::Unicode>(utf8,unicode);
    };

    parser.parse(source);

    for ( unsigned i = 0; i < colors.size(); i++ )
        *colors[i] = color::Color12::hsv(hue+double(i)/colors.size(),saturation,value);

    return str;
}
std::string FormatterRainbow::name() const
{
    return "rainbow";
}

} // namespace fun
