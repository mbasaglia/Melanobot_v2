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
#ifndef IRC_FORMATTER_HPP
#define IRC_FORMATTER_HPP

#include "string/string.hpp"

namespace irc {

/**
 * \brief UTF-8 with IRC colors
 */
class FormatterIrc : public string::FormatterPlain
{
public:
    std::string color(const color::Color12& color) const override;
    std::string format_flags(string::FormatFlags flags) const override;
    std::string clear() const override;
    string::FormattedString decode(const std::string& source) const override;
    std::string name() const override;
    /**
     * \brief Creates a color from an IRC color string \3..
     */
    static color::Color12 color_from_string(const std::string& color);
};

/**
 * \brief IRC formatter optimized for white baclgrounds
 */
class FormatterIrcWhite : public FormatterIrc
{
public:
    std::string color(const color::Color12& color) const override;
    std::string name() const override;
};

} // namespace irc
#endif // IRC_FORMATTER_HPP
