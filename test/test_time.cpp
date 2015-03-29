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
#define BOOST_TEST_MODULE Test_Time

#include <boost/test/unit_test.hpp>

#include "time.hpp"

using namespace timer;

BOOST_AUTO_TEST_CASE( test_Month )
{
    Month m = Month::JANUARY;
    BOOST_CHECK( int(m) == 1 );
    ++m;
    BOOST_CHECK( int(m) == 2 );
    m++;
    BOOST_CHECK( int(m) == 3 );
    m += 4;
    BOOST_CHECK( int(m) == 7 );
    m += 13;
    BOOST_CHECK( int(m) == 8 );
    --m;
    BOOST_CHECK( int(m) == 7 );
    m--;
    BOOST_CHECK( int(m) == 6 );
    m -= 4;
    BOOST_CHECK( int(m) == 2 );
    m -= 13;
    BOOST_CHECK( int(m) == 1 );

    BOOST_CHECK( Month::MAY - -2 == Month::JULY );
    BOOST_CHECK( Month::MAY + -2 ==  Month::MARCH );

}

BOOST_AUTO_TEST_CASE( test_DateTime )
{
    // explicit ctor
    DateTime mlps5utc(2015,Month::APRIL,days(4),hours(15),minutes(0));
    BOOST_CHECK( mlps5utc.year() == 2015 );
    BOOST_CHECK( mlps5utc.month() == Month::APRIL );
    BOOST_CHECK( mlps5utc.month_int() == 4 );
    BOOST_CHECK( mlps5utc.day() == 4 );
    BOOST_CHECK( mlps5utc.hour() == 15 );
    BOOST_CHECK( mlps5utc.minute() == 0 );
    BOOST_CHECK( mlps5utc.second() == 0 );
    BOOST_CHECK( mlps5utc.millisecond() == 0 );

    // explicit ctor bounding overflows
    DateTime overflowctor(2015,Month(15),days(34),hours(25),minutes(70),seconds(346),milliseconds(7777));
    BOOST_CHECK( overflowctor.year() == 2015 );
    BOOST_CHECK( overflowctor.month() <= Month::DECEMBER );
    BOOST_CHECK( overflowctor.day() <= 31 );
    BOOST_CHECK( overflowctor.hour() < 24 );
    BOOST_CHECK( overflowctor.minute() < 60 );
    BOOST_CHECK( overflowctor.second() < 60 );
    BOOST_CHECK( overflowctor.millisecond() < 1000 );

    // month days
    BOOST_CHECK( DateTime::month_days(2015,Month::JANUARY)      == 31 );
    BOOST_CHECK( DateTime::month_days(2015,Month::FEBRUARY)     == 28 );
    BOOST_CHECK( DateTime::month_days(2015,Month::MARCH)        == 31 );
    BOOST_CHECK( DateTime::month_days(2015,Month::APRIL)        == 30 );
    BOOST_CHECK( DateTime::month_days(2015,Month::MAY)          == 31 );
    BOOST_CHECK( DateTime::month_days(2015,Month::JUNE)         == 30 );
    BOOST_CHECK( DateTime::month_days(2015,Month::JULY)         == 31 );
    BOOST_CHECK( DateTime::month_days(2015,Month::AUGUST)       == 31 );
    BOOST_CHECK( DateTime::month_days(2015,Month::SEPTEMBER)    == 30 );
    BOOST_CHECK( DateTime::month_days(2015,Month::OCTOBER)      == 31 );
    BOOST_CHECK( DateTime::month_days(2015,Month::NOVEMBER)     == 30 );
    BOOST_CHECK( DateTime::month_days(2015,Month::DECEMBER)     == 31 );

    // leap years
    BOOST_CHECK( !DateTime::leap_year(2015) );
    BOOST_CHECK( DateTime::month_days(2015,Month::FEBRUARY) == 28 );
    BOOST_CHECK( DateTime::leap_year(2012) );
    BOOST_CHECK( DateTime::month_days(2012,Month::FEBRUARY) == 29 );
    BOOST_CHECK( DateTime::leap_year(2000) );
    BOOST_CHECK( DateTime::month_days(2000,Month::FEBRUARY) == 29 );
    BOOST_CHECK( DateTime::leap_year(2004) );
    BOOST_CHECK( DateTime::month_days(2004,Month::FEBRUARY) == 29 );
    BOOST_CHECK( !DateTime::leap_year(2100) );
    BOOST_CHECK( DateTime::month_days(2100,Month::FEBRUARY) == 28 );
    BOOST_CHECK( DateTime::leap_year(2400) );
    BOOST_CHECK( DateTime::month_days(2400,Month::FEBRUARY) == 29 );


    // operations
    DateTime time(2015,Month::DECEMBER,days(31),hours(23),minutes(59));
    // \todo simple setters

    // + millisecond (no overflow)
    time += milliseconds(500);
    BOOST_CHECK( time.millisecond() == 500 );
    // + millisecond (overflow)
    time += milliseconds(500);
    BOOST_CHECK( time.millisecond() == 0 );
    BOOST_CHECK( time.second() == 1 );

    // + seconds (no overflow)
    time += seconds(58);
    // + seconds (full overflow)
    BOOST_CHECK( time.second() == 59 );
    time += seconds(1);
    BOOST_CHECK( time.year() == 2016 );
    BOOST_CHECK( time.month() == Month::JANUARY );
    BOOST_CHECK( time.day() == 1 );
    BOOST_CHECK( time.hour() == 0 );
    BOOST_CHECK( time.minute() == 0 );
    BOOST_CHECK( time.second() == 0 );
    BOOST_CHECK( time.millisecond() == 0 );
    BOOST_CHECK( time == DateTime(2016,Month::JANUARY,days(1),hours(0),minutes(0),seconds(0),milliseconds(0)) );

    // + hours (no overflow)
    time += hours(4);
    BOOST_CHECK( time == DateTime(2016,Month::JANUARY,days(1),hours(4),minutes(0),seconds(0),milliseconds(0)) );
    // + hours (overflow)
    time += hours(25);
    BOOST_CHECK( time == DateTime(2016,Month::JANUARY,days(2),hours(5),minutes(0),seconds(0),milliseconds(0)) );

    // + days (no overflow)
    time += days(29);
    BOOST_CHECK( time == DateTime(2016,Month::JANUARY,days(31),hours(5),minutes(0),seconds(0),milliseconds(0)) );
    // + days (overflow)
    time += days(7);
    BOOST_CHECK( time == DateTime(2016,Month::FEBRUARY,days(7),hours(5),minutes(0),seconds(0),milliseconds(0)) );
    // + days (overflow)
    time += days(375+(time.leap_year()?1:0));
    BOOST_CHECK( time == DateTime(2017,Month::FEBRUARY,days(17),hours(5),minutes(0),seconds(0),milliseconds(0)) );
    // + months (no overflow)
    time += days(28+31+30);
    BOOST_CHECK( time == DateTime(2017,Month::MAY,days(17),hours(5),minutes(0),seconds(0),milliseconds(0)) );

    // compound setters
    time.set_time(hours(12),minutes(34),seconds(56),milliseconds(78));
    BOOST_CHECK( time == DateTime(2017,Month::MAY,days(17),hours(12),minutes(34),seconds(56),milliseconds(78)) );
    time.set_date(1974,Month::JULY,days(25));
    BOOST_CHECK( time == DateTime(1974,Month::JULY,days(25),hours(12),minutes(34),seconds(56),milliseconds(78)) );


    time = DateTime(2015,Month::JANUARY,days(1),hours(0),minutes(0));
    // - milliseconds (full overflow)
    time -= milliseconds(500);
    BOOST_CHECK( time == DateTime(2014,Month::DECEMBER,days(31),hours(23),minutes(59),seconds(59),milliseconds(500)) );
    // - milliseconds (no overflow)
    time -= milliseconds(500);
    BOOST_CHECK( time == DateTime(2014,Month::DECEMBER,days(31),hours(23),minutes(59),seconds(59),milliseconds(0)) );
    // - seconds (no overflow)
    time -= seconds(50);
    BOOST_CHECK( time == DateTime(2014,Month::DECEMBER,days(31),hours(23),minutes(59),seconds(9)) );
    // - seconds (overflow)
    time -= seconds(69);
    BOOST_CHECK( time == DateTime(2014,Month::DECEMBER,days(31),hours(23),minutes(58)) );
    // - minutes (no overflow)
    time -= minutes(50);
    BOOST_CHECK( time == DateTime(2014,Month::DECEMBER,days(31),hours(23),minutes(8)) );
    // - minutes (overflow)
    time -= minutes(8+60*2);
    BOOST_CHECK( time == DateTime(2014,Month::DECEMBER,days(31),hours(21),minutes(0)) );
    // - hours (no overflow)
    time -= hours(11);
    BOOST_CHECK( time == DateTime(2014,Month::DECEMBER,days(31),hours(10),minutes(0)) );
    // - hours (overflow)
    time -= hours(34);
    BOOST_CHECK( time == DateTime(2014,Month::DECEMBER,days(30),hours(0),minutes(0)) );
    // - days (no overflow)
    time -= days(20);
    BOOST_CHECK( time == DateTime(2014,Month::DECEMBER,days(10),hours(0),minutes(0)) );
    // - days (overflow)
    time -= days(20);
    BOOST_CHECK( time == DateTime(2014,Month::NOVEMBER,days(20),hours(0),minutes(0)) );
    // - months (no overflow)
    time -= days(30+31+30);
    BOOST_CHECK( time == DateTime(2014,Month::AUGUST,days(20),hours(0),minutes(0)) );
    // - days (overflow)
    time -= days(time.year_day());
    BOOST_CHECK( time == DateTime(2014,Month::JANUARY,days(1),hours(0),minutes(0)) );
    // - year
    time -= days(365);
    BOOST_CHECK( time == DateTime(2013,Month::JANUARY,days(1),hours(0),minutes(0)) );

    // comparison
    BOOST_CHECK( time+seconds(1) < time+milliseconds(1001) );
    BOOST_CHECK( time < time+milliseconds(1) );
    BOOST_CHECK( !(time+milliseconds(1) < time) );
    BOOST_CHECK( time <= time+milliseconds(1) );
    BOOST_CHECK( !(time+milliseconds(1) <= time) );

    BOOST_CHECK( !(time > time+milliseconds(1)) );
    BOOST_CHECK( time+milliseconds(1) > time );
    BOOST_CHECK( !(time >= time+milliseconds(1)) );
    BOOST_CHECK( time+milliseconds(1) >= time );

    BOOST_CHECK( time != time+milliseconds(1) );
    BOOST_CHECK( time+milliseconds(1) != time );
    BOOST_CHECK( !(time != time) );

    // year_day
    for ( int i = 0; i < 31; i++ )
        BOOST_CHECK( DateTime(2013,Month::JANUARY,days(1+i),hours(0),minutes(0)).year_day() == i );

    for ( int i = 0; i < 30; i++ )
        BOOST_CHECK( DateTime(2013,Month::MARCH,days(1+i),hours(0),minutes(0)).year_day() == i+28+31 );

    // unix
    DateTime unix(1970,Month::JANUARY,days(1),hours(0),minutes(0));
    BOOST_CHECK( unix.unix() == 0 );
    BOOST_CHECK( (unix+milliseconds(12)).unix() == 0 );
    BOOST_CHECK( (unix+seconds(1)).unix() == 1 );
    BOOST_CHECK( (unix+minutes(2)).unix() == 2*60 );
    BOOST_CHECK( (unix+hours(3)).unix() == 3*60*60 );
    BOOST_CHECK( (unix+days(4)).unix() == 4*24*60*60 );
    BOOST_CHECK( DateTime(2015,Month::MARCH,days(28),hours(18),minutes(53),seconds(30)).unix() == 1427568810  );
    BOOST_CHECK( DateTime(1915,Month::MARCH,days(28),hours(18),minutes(53),seconds(30)).unix() == -1728191190 );

    // time_point()
    BOOST_CHECK( unix.time_point().time_since_epoch().count() == 0 );
    auto time_point_check = [](const DateTime& dt, int64_t millistamp)
    {
        return std::chrono::duration_cast<milliseconds>(
            dt.time_point().time_since_epoch()).count() == millistamp;
    };
    BOOST_CHECK( time_point_check(
        {2015,Month::MARCH,days(28),hours(18),minutes(53),seconds(30),milliseconds(123)},
        1427568810123 ) );
    BOOST_CHECK( time_point_check(
        {1915,Month::MARCH,days(28),hours(18),minutes(53),seconds(30),milliseconds(1)},
        -1728191189999 ) );

    // default ctor (calls Clock::time_point ctor)
    BOOST_CHECK( std::abs( DateTime().unix() - std::time(nullptr) ) < 1);

}
