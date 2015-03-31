/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright  Mattia Basaglia
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
#ifndef TIME_STRING_HPP
#define TIME_STRING_HPP
#include "time.hpp"
namespace timer {

/**
 * \brief Full month name in English
 */
std::string month_name(Month month);

/**
 * \brief Short month name in English
 */
std::string month_shortname(Month month);

/**
 * \brief Month from English name
 */
Optional<Month> month_from_name(const std::string& name);

/**
 * \brief Full weekday name in English
 */
std::string weekday_name(WeekDay day);

/**
 * \brief Short weekday name in English
 */
std::string weekday_shortname(WeekDay day);

/**
 * \brief Weekday from English name
 */
Optional<WeekDay> weekday_from_name(const std::string& name);

/**
 * \brief Returns a string from a time format character
 * \see http://php.net/manual/en/function.date.php
 */
std::string format_char(const DateTime& date_time, char c);

/**
 * \brief Format a string from DateTime with the given format
 *
 * The character \\ escapes characters so they won't be expanded
 * \see http://php.net/manual/en/function.date.php
 */
std::string format(const DateTime& date_time, const std::string& fmt);

} // namespace timer
#endif // TIME_STRING_HPP
