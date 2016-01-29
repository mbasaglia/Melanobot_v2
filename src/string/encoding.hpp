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
#ifndef STRING_ENCODING_HPP
#define STRING_ENCODING_HPP

#include "melanolib/string/quickstream.hpp"

namespace string {

/**
 * \brief Class used to parse and convert UTF-8
 */
class Utf8Parser
{
public:
    using Byte = uint8_t;

    std::function<void(uint8_t)>                     callback_ascii;
    std::function<void(uint32_t,const std::string&)> callback_utf8;
    std::function<void(const std::string&)>          callback_invalid;
    std::function<void()>                            callback_end;

    melanolib::string::QuickStream input;

    /**
     * \brief Parse the string
     */
    void parse(const std::string& string);

    /**
     * \brief Whether the end of the string has been reached
     */
    bool finished() const { return !input; }

    /**
     * \brief Encode a unicode value to UTF-8
     */
    static std::string encode(uint32_t value);

    /**
     * \brief Whether a byte is a valid ascii character
     */
    static bool is_ascii(Byte b) { return b < 128; }

    /**
     * \brief Transliterate a single character to ascii
     */
    static char to_ascii(uint32_t unicode);
    /**
     * \brief Transliterate a single character to ascii
     */
    static char to_ascii(const std::string& utf8_char);


private:
    std::string           utf8;         ///< Multibyte string
    uint32_t              unicode;      ///< Multibyte value
    unsigned              length = 0;   ///< Multibyte length

    /**
     * \brief Handles an invalid/incomplete sequence
     */
    void check_valid();
};


} // namespace string
#endif // STRING_ENCODING_HPP
