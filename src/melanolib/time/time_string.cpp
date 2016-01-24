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
#include "time_string.hpp"

#include "melanolib/string/stringutils.hpp"
#include "melanolib/string/language.hpp"

namespace melanolib {
namespace time {

static std::vector<std::string> month_names =
{
    "January",
    "February",
    "March",
    "April",
    "May",
    "June",
    "July",
    "August",
    "September",
    "October",
    "November",
    "December",
};

static std::vector<std::string> month_shortnames =
{
    "Jan",
    "Feb",
    "Mar",
    "Apr",
    "May",
    "June",
    "July",
    "Aug",
    "Sept",
    "Oct",
    "Nov",
    "Dec",
};

static std::vector<std::string> weekday_names =
{
    "Monday",
    "Tuesday",
    "Wednesday",
    "Thursday",
    "Friday",
    "Saturday",
    "Sunday"
};

static std::vector<std::string> weekday_shortnames =
{
    "Mon",
    "Tue",
    "Wed",
    "Thu",
    "Fri",
    "Sat",
    "Sun"
};


std::string month_name(Month month)
{
    return int(month) >= 1 && int(month) <= 12 ?
        month_names[int(month)-1] : "";
}

std::string month_shortname(Month month)
{
    return int(month) >= 1 && int(month) <= 12 ?
        month_shortnames[int(month)-1] : "";
}

/**
 * \brief Month from English name
 */
melanolib::Optional<Month> month_from_name(const std::string& name)
{
    for ( unsigned i = 0; i < month_names.size(); i++ )
        if ( string::icase_equal(name,month_names[i]) )
            return Month(i+1);

    for ( unsigned i = 0; i < month_shortnames.size(); i++ )
        if ( string::icase_equal(name,month_shortnames[i]) )
            return Month(i+1);
    return {};
}

/**
 * \brief Full weekday name in English
 */
std::string weekday_name(WeekDay day)
{
    return int(day) >= 1 && int(day) <= 7 ?
        weekday_names[int(day)-1] : "";
}

/**
 * \brief Short weekday name in English
 */
std::string weekday_shortname(WeekDay day)
{
    return int(day) >= 1 && int(day) <= 7 ?
        weekday_shortnames[int(day)-1] : "";
}

/**
 * \brief Weekday from English name
 */
melanolib::Optional<WeekDay> weekday_from_name(const std::string& name)
{
    for ( unsigned i = 0; i < weekday_names.size(); i++ )
        if ( string::icase_equal(name,weekday_names[i]) )
            return WeekDay(i+1);

    for ( unsigned i = 0; i < weekday_shortnames.size(); i++ )
        if ( string::icase_equal(name,weekday_shortnames[i]) )
            return WeekDay(i+1);

    return {};
}
/**
 * \todo A version with the same format as POSIX date command
 */
std::string format_char(const DateTime& date_time, char c)
{
    switch (c)
    {
    // Day
        case 'd':
            return string::to_string(date_time.day(),2);
        case 'D':
            return weekday_shortname(date_time.week_day());
        case 'j':
            return std::to_string(date_time.day());
        case 'l':
            return weekday_name(date_time.week_day());
        case 'N':
            return std::to_string(int(date_time.week_day()));
        case 'S':
            return string::english.ordinal_suffix(date_time.day());
        case 'w':
            return std::to_string(int(date_time.week_day()) % 7);
        case 'z':
            return std::to_string(date_time.year_day());
    // Week
        case 'W':
            return "(unimplemented)"; /// \todo week number
    // Month
        case 'F':
            return month_name(date_time.month());
        case 'm':
            return string::to_string(date_time.month_int(),2);
        case 'M':
            return month_shortname(date_time.month());
        case 'n':
            return std::to_string(date_time.month_int());
        case 't':
            return std::to_string(date_time.month_days(date_time.month()));
    // Year
        case 'L':
            return date_time.leap_year() ? "1" : "0";
        case 'o':
            return "(unimplemented)"; /// \todo week number
        case 'Y':
            return std::to_string(date_time.year()); /// \todo get only last 4 digits? Handle BC?
        case 'y':
            return string::to_string(date_time.year() % 100,2);
    // Time
        case 'a':
            return date_time.am() ? "am" : "pm";
        case 'A':
            return date_time.am() ? "AM" : "PM";
        case 'B':
            return "(unimplemented)"; /// \todo Swatch Internet time
        case 'g':
            return std::to_string(date_time.hour12());
        case 'G':
            return std::to_string(date_time.hour());
        case 'h':
            return string::to_string(date_time.hour12(),2);
        case 'H':
            return string::to_string(date_time.hour(),2);
        case 'i':
            return string::to_string(date_time.minute(),2);
        case 's':
            return string::to_string(date_time.second(),2);
        case 'u':
            return string::to_string(date_time.millisecond(),3)+"000";
    // Timezone
        /// \todo Timezone(?)
        case 'e':
            return "UTC";
        case 'I':
            return "0";
        case 'O':
            return "+0000";
        case 'P':
            return "+00:00";
        case 'T':
            return "UTC";
        case 'Z':
            return "0";
    // Full Date/Time
        case 'c':
            return format(date_time,"Y-m-d\\TH:i:s"); /// \todo timezone %P
        case 'r':
            return format(date_time,"D, d M Y H:i:s"); /// \todo timezone %O
        case 'U':
            return std::to_string(date_time.unix());
    // Default
        default:
            return std::string(1,c);
    }
}

std::string format(const DateTime& date_time, const std::string& fmt)
{
    std::string result;

    for ( std::string::size_type i = 0; i < fmt.size(); i++ )
    {
        if ( fmt[i] == '\\' && i+1 < fmt.size() )
            result += fmt[++i];
        else
            result += format_char(date_time,fmt[i]);
    }

    return result;
}

/**
 * \todo Looks like at least in PHP, the format characters
 * for date() and strftime() are different meanings.
 * Check which are the most common ones and use them uniformly
 */
std::string strftime(const DateTime& date_time, const std::string& fmt)
{
    std::string result;

    for ( std::string::size_type i = 0; i < fmt.size(); i++ )
    {
        if ( fmt[i] == '%' && i+1 < fmt.size() )
            result += format_char(date_time, fmt[++i]);
        else
            result += fmt[i];
    }

    return result;
}

} // namespace time
} // namespace melanolib
