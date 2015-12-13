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
 *
 * \todo Split classes into separate files
 */
#ifndef MELANOLIB_TIME_HPP
#define MELANOLIB_TIME_HPP

#include <chrono>
#include <functional>
#include <cstdint>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <thread>

#include "functional.hpp"
#include "melanolib/c++-compat.hpp"
#include "melanolib/math.hpp"

namespace melanolib {
/**
 * \brief Namespace for time utilities
 */
namespace time {

/**
 * \brief A timer which performs a task after some time on a separate thread
 * \tparam Clock A clock type
 */
template<class Clock>
    class basic_timer
    {
    public:
        using clock_type    = Clock;                    ///< Clock
        using duration_type = typename Clock::duration; ///< Duration
        using function_type = std::function<void()>;    ///< Callback type

        explicit basic_timer( function_type action = {},
                              duration_type timeout = duration_type::zero(),
                              bool repeating = true
                            )
            :
            timeout(std::move(timeout)),
            repeating(repeating),
            action(std::move(action))
        {}

        basic_timer(const basic_timer& rhs)
            : timeout(rhs.timeout), repeating(rhs.repeating), action(rhs.action)
        {}
        basic_timer& operator=(const basic_timer& rhs)
        {
            timeout = rhs.timeout;
            repeating = rhs.repeating.load();
            action = rhs.action;
            return *this;
        }

        // note: mutex not move constructible
        basic_timer(basic_timer&& rhs) :
            timeout     (std::move(rhs.timeout)),
            repeating   (std::move(rhs.repeating)),
            action      (std::move(rhs.action))
        {
            rhs.stop();
        }

        basic_timer& operator=(basic_timer&& rhs)
        {
            rhs.stop();
            timeout     = std::move(rhs.timeout);
            repeating   = rhs.repeating.load();
            action      = std::move(rhs.action);
            return *this;
        }

        ~basic_timer() { stop(); }

        /**
         * \brief Start the timer with the already-set duration
         * \note If \c timeout is zero, \c repeating will be ignored
         * \return \b true on success
         */
        bool start()
        {
            if ( running() || !action )
                return false;
            if ( timeout == duration_type::zero() )
                callback(action);
            thread = std::move(std::thread([this]{run();}));
            return running();
        }

        /**
         * \brief Whether the timer has been started and is still running
         */
        bool running() const
        {
            return thread.joinable();
        }

        /**
         * \brief Stop the timer and set a new timeout
         * \return \b true on success
         */
        bool reset(duration_type timeout)
        {
            stop();
            this->timeout = timeout;
            return start();
        }

        /**
         * \brief Stop the timer
         */
        void stop()
        {
            if ( running() )
            {
                active = false;
                condition.notify_all();
                try {
                    thread.join();
                // catch in the case the thread joined early
                }catch(const std::invalid_argument&) {}
            }
        }

    private:
        duration_type           timeout;         ///< Timer duration
        std::atomic<bool>       repeating{false};///< Whether it should start over once finished
        std::atomic<bool>       active{false};   ///< Whether it's currently running
        std::condition_variable condition;       ///< Wait condition
        function_type           action;          ///< Executed on timeout
        std::thread             thread;          ///< Thread where the callback is executed

        /**
         * \brief Runs the timer
         */
        void run()
        {
            std::mutex mutex;
            active = true;
            while ( active && repeating )
            {
                std::unique_lock<std::mutex> lock(mutex);
                condition.wait_until(lock,clock_type::now()+timeout,[this]{return !active;});
                if ( active )
                    action();
            }
            active = false;
        }

    };

/**
 * \brief Timer using the wall clock
 */
using Timer = basic_timer<std::chrono::system_clock>;

using std::chrono::milliseconds;
using std::chrono::seconds;
using std::chrono::minutes;
using std::chrono::hours;
using days =  std::chrono::duration<int64_t, std::ratio<(24*3600)>>;
using weeks =  std::chrono::duration<int64_t, std::ratio<(24*3600*7)>>;

/**
 * \brief Month enum
 */
enum class Month : uint8_t {
    JANUARY  = 1,
    FEBRUARY = 2,
    MARCH    = 3,
    APRIL    = 4,
    MAY      = 5,
    JUNE     = 6,
    JULY     = 7,
    AUGUST   = 8,
    SEPTEMBER= 9,
    OCTOBER  =10,
    NOVEMBER =11,
    DECEMBER =12
};

constexpr Month operator- (Month m, int i) noexcept;
inline constexpr Month operator+ (Month m, int i) noexcept
{
    return i > 0 ? Month((((int(m)-1)+i)%12)+1) : m - -i;
}

inline SUPER_CONSTEXPR Month& operator+= (Month& m, int i) noexcept
{
    m = m + i;
    return m;
}
inline constexpr Month operator- (Month m, int i) noexcept
{
    return i < 0 ? m + -i : ( i < int(m) ? Month(int(m)-i) : m - (i-12) );
}

inline SUPER_CONSTEXPR Month& operator-= (Month& m, int i) noexcept
{
    m = m - i;
    return m;
}

inline SUPER_CONSTEXPR Month& operator++ (Month& m) noexcept
{
    return m += 1;
}

inline SUPER_CONSTEXPR Month operator++ (Month& m, int) noexcept
{
    auto c = m;
    ++m;
    return c;
}

inline SUPER_CONSTEXPR Month& operator-- (Month& m) noexcept
{
    return m -= 1;
}

inline SUPER_CONSTEXPR Month operator-- (Month& m, int) noexcept
{
    auto c = m;
    --m;
    return c;
}

/**
 * \brief Week day enum
 */
enum class WeekDay : uint8_t {
    MONDAY      = 1,
    TUESDAY     = 2,
    WEDNESDAY   = 3,
    THURSDAY    = 4,
    FRIDAY      = 5,
    SATURDAY    = 6,
    SUNDAY      = 7
};

constexpr WeekDay operator- (WeekDay m, int i) noexcept;
inline constexpr WeekDay operator+ (WeekDay m, int i) noexcept
{
    return i > 0 ? WeekDay((((int(m)-1)+i)%7)+1) : m - -i;
}

inline SUPER_CONSTEXPR WeekDay& operator+= (WeekDay& m, int i) noexcept
{
    m = m + i;
    return m;
}
inline constexpr WeekDay operator- (WeekDay m, int i) noexcept
{
    return i < 0 ? m + -i : ( i < int(m) ? WeekDay(int(m)-i) : m - (i-7) );
}

inline SUPER_CONSTEXPR WeekDay& operator-= (WeekDay& m, int i) noexcept
{
    m = m - i;
    return m;
}

inline SUPER_CONSTEXPR WeekDay& operator++ (WeekDay& m) noexcept
{
    return m += 1;
}

inline SUPER_CONSTEXPR WeekDay operator++ (WeekDay& m, int) noexcept
{
    auto c = m;
    ++m;
    return c;
}

inline SUPER_CONSTEXPR WeekDay& operator-- (WeekDay& m) noexcept
{
    return m -= 1;
}

inline SUPER_CONSTEXPR WeekDay operator-- (WeekDay& m, int) noexcept
{
    auto c = m;
    --m;
    return c;
}
/**
 * \brief A class representing a date and time
 * \note Some functions assume std::chrono::system_clock starts on the unix epoch
 */
class DateTime
{
public:
    using Clock    = std::chrono::system_clock; ///< Wall clock time
    using Time     = Clock::time_point;         ///< Time point
    using Duration = Clock::duration;

// ctors
    DateTime() : DateTime(Clock::now()) {}

    DateTime(const Time& time)
        : DateTime(1970,Month::JANUARY,days(1),hours(0),minutes(0))
    {
        *this += time.time_since_epoch();
    }

    constexpr DateTime(int32_t year, Month month, days day)
    : DateTime(year, month, day, hours(0), minutes(0)) {}

    constexpr DateTime(int32_t year, Month month, days day, hours hour, minutes minute,
             seconds second = seconds(0), milliseconds millisecond = milliseconds(0))
    : year_(year),
      month_(melanolib::math::bound(Month::JANUARY,month,Month::DECEMBER)),
      day_(melanolib::math::bound(1,day.count(),month_days(year,melanolib::math::bound(Month::JANUARY,month,Month::DECEMBER)))),
      hour_(hour.count() % 24),
      minute_(minute.count() % 60),
      second_(second.count() % 60),
      milliseconds_(millisecond.count() % 1000)
      {}

// conversions
    /**
     * \brief Build a time point from the DateTime
     */
    SUPER_CONSTEXPR Time time_point() const noexcept
    {
        return std::chrono::time_point_cast<Duration>(
            std::chrono::time_point<Clock, milliseconds>(
                milliseconds(unix()*1000+milliseconds_)));
    }

    /**
     * \brief Return as a unix timestamp
     */
    SUPER_CONSTEXPR int64_t unix() const noexcept
    {
        int64_t timestamp = 0;
        const int64_t day_seconds = 60 * 60 * 24;
        if ( year_ > 1970 )
        {
            for ( int32_t y = 1970; y < year_; y++ )
                timestamp += year_days(y) * day_seconds;
        }
        else if ( year_ <= 1970 )
        {
            for ( int32_t y = 1969; y >= year_; y-- )
                timestamp -= year_days(y) * day_seconds;
        }
        timestamp += year_day() * day_seconds;
        timestamp += hour_*60*60;
        timestamp += minute_*60;
        timestamp += second_;
        return timestamp;
    }

// getters
    /**
     * \brief Milliseconds
     */
    constexpr auto millisecond() const noexcept { return milliseconds_; }
    /**
     * \brief Seconds
     * \note No leap seconds
     */
    constexpr auto second() const noexcept { return second_; }
    /**
     * \brief Minutes
     */
    constexpr auto minute() const noexcept { return minute_; }
    /**
     * \brief Hour of the day [0,23]
     */
    constexpr auto hour() const noexcept { return hour_; }
    /**
     * \brief Hour of the day [1,12]
     */
    constexpr auto hour12() const noexcept { return hour_ % 12 ? hour_ % 12 : 12; }
    /**
     * \brief Whether it's a.m. (12:00 am = midnight, 12:00pm = noon)
     */
    constexpr bool am() const noexcept { return hour_ < 12; }
    /**
     * \brief Whether it's a.m. (12:00 am = midnight, 12:00pm = noon)
     */
    constexpr bool pm() const noexcept { return hour_ >= 12; }
    /**
     * \brief Day of the month [1,31]
     */
    constexpr auto day() const noexcept { return day_; }
    /**
     * \brief Day of the year [0,365]
     * \note Zero based
     */
    SUPER_CONSTEXPR int year_day() const noexcept
    {
        int r = 0;
        for ( Month m = Month::JANUARY; m < month_; m++ )
            r += month_days(m);
        return r + day() - 1 ;
    }
    /**
     * \brief Number of days in a year
     */
    static constexpr int year_days(int32_t year) noexcept
    {
        return leap_year(year) ? 366 : 365;
    }
    /**
     * \brief Month
     */
    constexpr Month month() const noexcept { return month_; }
    /**
     * \brief Month (as an int)
     */
    constexpr int month_int() const noexcept { return int(month_); }
    /**
     * \brief Year
     */
    constexpr auto year() const noexcept { return year_; }
    /**
     * \brief Whether the year is leap
     * \note This is true for the Gregorian calendar and disregards oddities
     *       of dates before 15 October 1582
     */
    constexpr bool leap_year() const noexcept
    {
        return leap_year(year_);
    }
    /**
     * \brief Whether the year is leap
     * \note This is true for the Gregorian calendar and disregards oddities
     *       of dates before 15 October 1582
     */
    static constexpr bool leap_year(int32_t year) noexcept
    {
        // Leap if divisible by 4
        // but not if divisible by 100
        // unless it's divisible by 400
        return !(year % 400) || ( !(year%4) && (year%100) );
    }
    /**
     * \brief Number of days in the given month
     */
    constexpr int month_days(Month m) const noexcept
    {
        return month_days(year_,m);
    }
    /**
     * \brief Number of days in the given month
     */
    static constexpr int month_days(int32_t year, Month m) noexcept
    {
#if !defined(__cpp_constexpr) || __cpp_constexpr >= 201304
        if ( m == Month::FEBRUARY )
            return leap_year(year) ? 29 : 28;
        if ( m <= Month::JULY )
            return int(m) % 2 ? 31 : 30;
        return int(m) % 2 ? 30 : 31;
#else
        return m == Month::FEBRUARY ?
            ( leap_year(year) ? 29 : 28 ) :
            ( ( m <= Month::JULY ) ?
                ( int(m) % 2 ? 31 : 30 ) :
                ( int(m) % 2 ? 30 : 31 )
            );
#endif
    }

    /**
     * \brief Evaluates the week day
     */
    SUPER_CONSTEXPR WeekDay week_day() const noexcept
    {
        int month2 = int(month_);
        int year2 = year_;
        if ( month2 < 3 )
        {
            year2--;
            month2+=12;
        }
        year2 = positive_year(year2);

        int d = ( day_ + (month2+1)*26/10 + year2 + year2/4 + 6*(year2/100) + year2/400 ) % 7;
        return WeekDay((d+5) % 7 + 1);
    }

// setters
    /**
     * \brief Set the time without changing the date
     */
    SUPER_CONSTEXPR void set_time(hours hour, minutes minute,
             seconds second = seconds(0), milliseconds millisecond = milliseconds(0)) noexcept
    {
        set_hour(hour.count());
        set_minute(minute.count());
        set_second(second.count());
        set_millisecond(millisecond.count());
    }

    /**
     * \brief Set the date without changing the time
     */
    SUPER_CONSTEXPR void set_date(int32_t year, Month month, days day) noexcept
    {
        set_year(year);
        month_ = melanolib::math::bound(Month::JANUARY, month, Month::DECEMBER);
        set_day(day.count());
    }
    /**
     * \brief Set the date without changing the year or the time
     */
    SUPER_CONSTEXPR void set_date(Month month, days day) noexcept
    {
        month_ = melanolib::math::bound(Month::JANUARY, month, Month::DECEMBER);
        set_day(day.count());
    }

    /**
     * \brief Sets the year
     */
    SUPER_CONSTEXPR void set_year(int32_t year) noexcept
    {
        year_ = year;
    }
    /**
     * \brief Sets the month
     * \note It might change the day if its greater than month_days(month)
     */
    SUPER_CONSTEXPR void set_month(Month month) noexcept
    {
        month_ = melanolib::math::bound(Month::JANUARY, month, Month::DECEMBER);
        set_day(day_);
    }
    /**
     * \brief Sets the day of the month
     */
    SUPER_CONSTEXPR void set_day(int day) noexcept
    {
        day_ = melanolib::math::bound(1, day, month_days(month_));
    }
    /**
     * \brief Sets the hour of the day [0,23]
     */
    SUPER_CONSTEXPR void set_hour(uint8_t hour) noexcept
    {
        hour_ = hour % 24;
    }
    /**
     * \brief Sets the minute of the hour [0,59]
     */
    SUPER_CONSTEXPR void set_minute(uint8_t minute) noexcept
    {
        minute_ = minute % 60;
    }
    /**
     * \brief Sets the second of the minute [0,59]
     * \note no leap seconds
     */
    SUPER_CONSTEXPR void set_second(uint8_t second) noexcept
    {
        second_ = second % 60;
    }
    /**
     * \brief Sets the milliseconds of the second [0,999]
     */
    SUPER_CONSTEXPR void set_millisecond(uint8_t milliseconds) noexcept
    {
        milliseconds_ = milliseconds % 1000;
    }

// Operations

    /**
     * \brief Add a duration
     */
    template<class Rep, class Period>
        DateTime& operator+= (const std::chrono::duration<Rep,Period>& dur) noexcept
        {
            if ( dur < std::chrono::duration<Rep,Period>::zero() )
                return *this -= -dur;
            if ( dur == std::chrono::duration<Rep,Period>::zero() )
                return *this;

            // adding a positive duration

            auto ms = std::chrono::duration_cast<milliseconds>(dur).count();
            decltype(ms) mask = 1;

            // add time
            add_helper(1000,mask,ms,milliseconds_);
            add_helper(60,mask,ms,second_);
            add_helper(60,mask,ms,minute_);
            add_helper(24,mask,ms,hour_);

            // now the time has been added correctly, ms is rounded to the day
            // and the date needs to be adjusted
            auto d = ms / mask;

            // round to the next year
            if ( d >= year_days(year_) - year_day() )
            {
                d -= year_days(year_) - year_day();
                set_date(year_+1,Month::JANUARY,days(1));
            }

            // advance whole years
            while ( d >= year_days(year_) )
            {
                d -= year_days(year_);
                year_++;
            }

            // advance whole months
            while ( d >= month_days(month_) )
            {
                d -= month_days(month_);
                month_++;
            }

            // advance whole days
            day_ += d;

            // handle day overflow
            if ( day_ > month_days(month_) )
            {
                day_ -= month_days(month_);
                month_++;
            }

            return *this;
        }

    template<class Rep, class Period>
        SUPER_CONSTEXPR DateTime operator+ (const std::chrono::duration<Rep,Period>& dur) const noexcept
        {
            auto t = *this;
            return t += dur;
        }

    /**
     * \brief Subtract a duration
     * \tparam Duration a chrono duration
     */
    template<class Rep, class Period>
        DateTime& operator-= (const std::chrono::duration<Rep,Period>& dur) noexcept
        {
            if ( dur < std::chrono::duration<Rep,Period>::zero() )
                return *this += -dur;
            if ( dur == std::chrono::duration<Rep,Period>::zero() )
                return *this;

            // subtracting a positive duration
            auto ms = std::chrono::duration_cast<milliseconds>(dur).count();
            decltype(ms) mask = 1;

            // subtract time
            subtract_helper(1000,mask,ms,milliseconds_);
            subtract_helper(60,mask,ms,second_);
            subtract_helper(60,mask,ms,minute_);
            subtract_helper(24,mask,ms,hour_);

            // now the time has been subtracted correctly,
            // ms is rounded to the day and the date needs to be adjusted
            auto d = ms / mask;

            // move to dec 12 of the previous year
            if ( d > year_day() )
            {
                d -= year_day() + 1;
                set_date(year_-1,Month::DECEMBER,days(31));
            }

            // subtract whole years
            while ( d >= year_days(year_) )
            {
                d -= year_days(year_);
                year_--;
            }

            // subtract whole months
            while ( d >= month_days(month_) )
            {
                d -= month_days(month_);
                month_--;
            }

            // advance whole days
            day_ -= d;

            // handle day underflow
            if ( day_ <= 0 )
            {
                month_--;
                day_ += month_days(month_);
            }

            return *this;
        }

    template<class Rep, class Period>
        SUPER_CONSTEXPR DateTime operator- (const std::chrono::duration<Rep,Period>& dur) const noexcept
        {
            auto t = *this;
            return t -= dur;
        }

    SUPER_CONSTEXPR Duration operator- (const DateTime& rhs) const noexcept
    {
        return std::chrono::duration_cast<Duration>(milliseconds(unix()*1000+milliseconds_)) -
            std::chrono::duration_cast<Duration>(milliseconds(rhs.unix()*1000+rhs.milliseconds_));
    }

    SUPER_CONSTEXPR bool operator< (const DateTime& rhs) const noexcept
    {
        if ( year_ < rhs.year_ ) return true;
        if ( year_ > rhs.year_ ) return false;

        if ( month_ < rhs.month_ ) return true;
        if ( month_ > rhs.month_ ) return false;

        if ( day_ < rhs.day_ ) return true;
        if ( day_ > rhs.day_ ) return false;

        if ( hour_ < rhs.hour_ ) return true;
        if ( hour_ > rhs.hour_ ) return false;

        if ( minute_ < rhs.minute_ ) return true;
        if ( minute_ > rhs.minute_ ) return false;

        if ( second_ < rhs.second_ ) return true;
        if ( second_ > rhs.second_ ) return false;

        if ( milliseconds_ < rhs.milliseconds_ ) return true;
        return false;
    }

    SUPER_CONSTEXPR bool operator> (const DateTime& rhs) const noexcept
    {
        if ( year_ > rhs.year_ ) return true;
        if ( year_ < rhs.year_ ) return false;

        if ( month_ > rhs.month_ ) return true;
        if ( month_ < rhs.month_ ) return false;

        if ( day_ > rhs.day_ ) return true;
        if ( day_ < rhs.day_ ) return false;

        if ( hour_ > rhs.hour_ ) return true;
        if ( hour_ < rhs.hour_ ) return false;

        if ( minute_ > rhs.minute_ ) return true;
        if ( minute_ < rhs.minute_ ) return false;

        if ( second_ > rhs.second_ ) return true;
        if ( second_ < rhs.second_ ) return false;

        if ( milliseconds_ > rhs.milliseconds_ ) return true;
        return false;
    }

    constexpr bool operator== (const DateTime& rhs) const noexcept
    {
        return  milliseconds_ == rhs.milliseconds_ &&
                second_ == rhs.second_ &&
                minute_ == rhs.minute_ &&
                hour_ == rhs.hour_ &&
                day_ == rhs.day_ &&
                month_ == rhs.month_ &&
                year_ == rhs.year_;
    }

    constexpr bool operator!= (const DateTime& rhs) const noexcept
    {
        return  ! (*this == rhs);
    }

    SUPER_CONSTEXPR bool operator<= (const DateTime& rhs) const noexcept
    {
        return  ! (*this > rhs);
    }

    SUPER_CONSTEXPR bool operator>= (const DateTime& rhs) const noexcept
    {
        return  ! (*this < rhs);
    }


private:
    int32_t     year_;          ///< Year
    Month       month_;         ///< Month
    int8_t     day_;            ///< Day of the month [1,31]
    int8_t     hour_;           ///< Hour [0,23]
    int8_t     minute_;         ///< Minute [0,59]
    int8_t     second_;         ///< Second [0,59] (no leap seconds)
    int16_t    milliseconds_;   ///< Milli seconds [0,10^3)

    /**
     * \brief Helper function for operator-=()
     */
    template<class T1, class T2>
        static void subtract_helper(int nextunit, T1& mask, T1& ms, T2& member) noexcept
        {
            const auto next_mask = mask * nextunit;
            if ( ms % next_mask )
            {
                auto delta = (ms % next_mask) / mask;
                member -= delta;
                ms -= delta * mask;
                if ( member < 0 )
                {
                    member += nextunit;
                    ms += next_mask;
                }
            }
            mask = next_mask;
        }

    template<class T1, class T2>
        static void add_helper(int nextunit, T1& mask, T1& ms, T2& member) noexcept
        {
            const auto next_mask = mask * nextunit;
            if ( ms % next_mask )
            {
                auto delta = (ms % next_mask) / mask;
                member += delta;
                ms -= delta * mask;
                // all members have large enough types to handle overflows
                if ( member >= nextunit )
                {
                    member %= nextunit;
                    ms += next_mask;
                }
            }
            mask = next_mask;
        }

    // converts negative years into positive ones and add 1
    // (-1 BC becomes 0 AD, which is equivalent to 400 AD)
    static constexpr int32_t positive_year(int32_t year) noexcept
    {
        return year < 0 ? year+400*std::ceil(-year/400.0)+1 : year;
    }
};

/**
 * \brief Parses the text description of a time point
 */
DateTime parse_time(const std::string& text);
/**
 * \brief Parses the text description of a duration
 */
DateTime::Duration parse_duration(const std::string& text);

} // namespace time
} // namespace melanolib

#endif // MELANOLIB_TIME_HPP
