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
#ifndef XONOTIC_FORMATTER_HPP
#define XONOTIC_FORMATTER_HPP

#include <regex>

#include "string/string.hpp"

/**
 * \brief Namespace for Xonotic integration
 */
namespace xonotic {

/**
 * \brief Darkplaces UTF-8
 */
class XonoticFormatter : public string::FormatterUtf8
{
public:
    using FormatterUtf8::to_string;
    std::string to_string(char input) const override;
    std::string to_string(const string::AsciiString& s) const override;
    std::string to_string(const color::Color12& color) const override;
    std::string to_string(string::FormatFlags flags) const override;
    std::string to_string(string::ClearFormatting clear) const override;

    string::FormattedString decode(const std::string& source) const override;
    std::string name() const override;
    /**
     * \brief Creates a color from a DP color string ^. or ^x...
     */
    static color::Color12 color_from_string(std::string color);

private:
    static std::regex regex_xoncolor; ///< Regex matching color sequences

    /**
     * \brief Creates a color from a regex match
     * \pre \c match has been passed to a successful call to regex_match
     *      which used \c regex_xoncolor
     */
    static color::Color12 color_from_match(const std::smatch& match);
};

/**
 * \brief qfont character
 */
class QFont
{
public:
    explicit QFont(unsigned index) : index_(index) {}

    /**
     * \brief Returns the qfont index
     */
    unsigned index() const { return index_; }

    /**
     * \brief Gets an alternative representation of the character
     * \return An ASCII string aproximating the qfont character
     */
    std::string alternative() const;

    /**
     * \brief The qfont character as a custom unicode character
     */
    uint32_t unicode_point() const
    {
        return 0xE000 | index_;
    }

    std::string to_string(const string::Formatter& fmt) const;

private:
    unsigned index_; ///< Index in the qfont_table
};

} // namespace xonotic
#endif // XONOTIC_FORMATTER_HPP
