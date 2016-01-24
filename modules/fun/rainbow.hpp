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
#ifndef FUN_RAINBOW_HPP
#define FUN_RAINBOW_HPP

#include "string/string.hpp"

namespace fun {

/**
 * \brief Rainbow UTF-8
 *
 * Encodes like FormatterUtf8 and decodes adding colors between each character
 */
class FormatterRainbow : public string::FormatterUtf8
{
public:
    explicit FormatterRainbow(double hue = 0, double saturation = 1, double value = 1)
        : hue(hue), saturation(saturation), value(value) {}
    string::FormattedString decode(const std::string& source) const override;
    std::string name() const override;

    double hue;         ///< Starting hue [0,1]
    double saturation;  ///< Saturation [0,1]
    double value;       ///< Value [0,1]
};

} // namespace fun
#endif // FUN_RAINBOW_HPP
