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
#ifndef TIME_PARSER_HPP
#define TIME_PARSER_HPP

#include "melanolib/string/stringutils.hpp"
#include "melanolib/time/time.hpp"
#include "melanolib/time/time_string.hpp"

namespace melanolib {
namespace time {
/**
 * \brief Parses a time description
 */
class TimeParser
{
public:
    using Clock    = DateTime::Clock;
    using Time     = typename Clock::time_point;
    using Duration = typename Clock::duration;

    TimeParser(std::istream& input) : input(input)
    {
        scan();
    }

    /**
     * \brief Parses a time point
     * \code
     *  TIME_POINT      ::= NOW_TIME | DATE_TIME
     *  NOW_TIME        ::= now
     *                  |   now + DURATION
     *                  |   now - DURATION
     *                  |   in DURATION
     * \endcode
     */
    DateTime parse_time_point()
    {
        if ( lookahead.type == Token::NOW )
        {
            DateTime now;
            scan();
            if ( lookahead.type == Token::OPERATOR )
            {
                char oper = lookahead.lexeme[0];
                scan();
                if ( oper == '+' )
                    now += parse_duration();
                else if ( oper == '-' )
                    now -= parse_duration();
            }
            return now;
        }
        else if ( lookahead.type == Token::IN )
        {
            scan();
            return DateTime() + parse_duration();
        }

        return parse_date_time();
    }

    /**
     * \brief Parses a duration
     * \code
     *  DURATION        ::= HOUR_OPT DURATION_SEQ
     *  HOUR_OPT        ::= (eps) | time | time "h" | time "min"
     *  DURATION_SEQ    ::= ATOMIC_DURATION | ATOMIC_DURATION DURATION_SEQ | (eps)
     * \endcode
     */
    Duration parse_duration()
    {
        Duration duration = Duration::zero();

        // check of the optional time
        if ( lookahead.type == Token::TIME )
        {
            // get the time
            duration = token_val<Duration>();
            // 12:34 is a valid minute, 12:34:56 is not
            bool can_be_minutes = lookahead.lexeme.size() == 5;
            // advance lookahead
            scan();
            // check for the optional unit (hours or minutes)
            if ( lookahead.type == Token::IDENTIFIER )
            {
                if ( is_unit<hours>(lookahead.lexeme) )
                {
                    // hours, nothing to do
                    scan();
                }
                else if ( can_be_minutes && is_unit<minutes>(lookahead.lexeme) )
                {
                    // hours become minutes
                    duration /= 60;
                    scan();
                }
            }
        }
        else if ( lookahead.type == Token::IDENTIFIER && lookahead.lexeme == "PT" )
        {
            // Note: between P and T there could be years/months/weeks/days in duration
            scan();
        }

        // Read multiple number/unit pairs
        while ( lookahead.type == Token::NUMBER )
            duration += parse_atomic_duration();

        return duration;
    }

    /**
     * \brief Returns everything after the last parsed token
     * \note After reading it once, the last token will be cleared
     */
    std::string get_remainder()
    {
        unget_last_token();
        lookahead = {};
        return std::string(std::istreambuf_iterator<char>(input), {});
    }

private:
    /**
     * \brief Parsed token
     */
    struct Token
    {
        /**
         * \brief Token type
         */
        enum Type {     //   Dscription         pattern         value type
            INVALID,    ///< EOF or similar                     void
            NUMBER,     ///< A number           [0-9]+          uint32_t
            IDENTIFIER, ///< An identifier      [a-z]+          void
            TIME,       ///< A time string      12:30[:00[.0]]  Duration
            OPERATOR,   ///< An operator        [+-]            void
            NOW,        ///< Now                now             void
            REL_DAY,    ///< Relative day       yesterday...    int (day offset)
            DATE,       ///< An ISO date        1234-02-27      DateTime
            MONTH,      ///< Month name         January...      Month
            WEEK_DAY,   ///< Week day name      Saturday...     WeekDay
            AMPM,       ///< am/pm              am|pm           bool (pm)
            IN,         ///< "in"               in              void
            AT,         ///< "at"               at              void
        };
        Type            type{INVALID};  ///< Token type
        std::string     lexeme;         ///< Corresponding string
        boost::any      value;          ///< Associated value

        Token(){};

        Token(Type type, std::string lexeme)
            : type(type), lexeme(std::move(lexeme)) {}

        template<class T>
            Token(Type type, std::string lexeme, T&& val)
                : type(type), lexeme(std::move(lexeme)), value(std::forward<T>(val)) {}

        bool is_identifier(const char* str) const
        {
            return type == IDENTIFIER && lexeme == str;
        }
    };

    Token           lookahead;  ///< Last read token
    std::istream&   input;      ///< Input stream

    /**
     * \brief Step back if the last token wasn't read
     */
    void unget_last_token()
    {
        if ( !lookahead.lexeme.empty() )
        {
            input.clear();
            for ( auto it = lookahead.lexeme.rbegin(); it != lookahead.lexeme.rend(); ++it )
            {
                input.unget();
            }
        }
    }

    /**
     * \brief Lex an identifier
     * \param c Lookahead character
     */
    Token lex_identifier(char c)
    {
        std::string id;
        while(std::isalpha(c) && input)
        {
            id += c;
            c = input.get();
        }
        input.unget();

        auto lower = string::strtolower(id);

        if ( lower == "now" )
            return {Token::NOW, id};
        if ( lower == "today" )
            return {Token::REL_DAY, id, 0};
        if ( lower == "tomorrow" )
            return {Token::REL_DAY, id, 1};
        if ( lower == "yesterday" )
            return {Token::REL_DAY, id, -1};
        if ( lower == "am" )
            return {Token::AMPM, id, false};
        if ( lower == "pm" )
            return {Token::AMPM, id, true};
        if ( lower == "in" )
            return {Token::IN, id};
        if ( lower == "at" )
            return {Token::AT, id};

        auto maybe_month = month_from_name(lower);
        if ( maybe_month )
            return {Token::MONTH, id, *maybe_month};

        auto maybe_weekday = weekday_from_name(lower);
        if ( maybe_weekday )
            return {Token::WEEK_DAY, id, *maybe_weekday};

        return {Token::IDENTIFIER, id, std::move(lower)};
    }

    /**
     * \brief Lexes a time (12:34[:56[.789]])
     * \param lexed Lexed first two digits
     * \pre \c lexed contains two decimal digits
     * \pre A ':' has been lexed
     */
    Token lex_time(std::string& lexed)
    {
        Duration dur = Duration::zero();
        dur += hours(string::to_uint(lexed));

        char c = input.get();
        std::string part2_lex = lex_raw_number(c);
        dur += minutes(string::to_uint(part2_lex));
        lexed += ':'+part2_lex;

        // has seconds
        if ( c == ':' )
        {
            c = input.get();
            part2_lex = lex_raw_number(c);
            dur += seconds(string::to_uint(part2_lex));
            lexed += ':'+part2_lex;
            // milliseconds
            if ( c == '.' )
            {
                c = input.get();
                part2_lex = lex_raw_number(c);
                dur += milliseconds(string::to_uint(part2_lex));
                lexed += '.'+part2_lex;
            }
        }
        input.unget();
        return {Token::TIME, lexed, dur};
    }

    /**
     * \brief Lexes a date 1234-02-23
     * \param lexed Lexed year
     * \pre \c lexed contains only decimal digits
     * \pre A '-' has been lexed
     */
    Token lex_date(std::string& lexed)
    {
        int year = string::to_uint(lexed);

        char c = input.get();
        std::string part2_lex = lex_raw_number(c);
        Month month = Month(string::to_uint(part2_lex));
        lexed += '-'+part2_lex;
        if ( c != '-' )
            return {};

        c = input.get();
        part2_lex = lex_raw_number(c);
        int day = string::to_uint(part2_lex);
        lexed += '-'+part2_lex;

        if ( month > Month::DECEMBER || month < Month::JANUARY ||
            day < 1 || day > DateTime::month_days(year,month) )
            return {};


        input.unget();
        DateTime date;
        date.set_date(year,month,days(day));
        return {Token::DATE, lexed, std::move(date)};
    }

    /**
     * \brief Lex a number without any other processing
     * \param c Lookahead character
     */
    std::string lex_raw_number(char& c)
    {
        std::string lexed;
        while(std::isdigit(c) && input)
        {
            lexed += c;
            c = input.get();
        }
        return lexed;
    }

    /**
     * \brief Lex a number
     * \param c Lookahead character
     */
    Token lex_number(char c)
    {
        std::string lexed = lex_raw_number(c);
        if ( c == ':' )
            return lex_time(lexed);
        else if ( c == '-' )
            return lex_date(lexed);
        input.unget();
        return {Token::NUMBER, lexed, uint32_t(string::to_uint(lexed))};
    }

    /**
     * \brief Reads the next token
     */
    Token lex()
    {
        char c = ' ';
        while ( (std::isspace(c) || c == ',') && input )
            c = input.get();
        if ( !input )
            return {};

        if ( std::isalpha(c) )
            return lex_identifier(c);
        else if ( std::isdigit(c) )
            return lex_number(c);
        else if ( c == '+' || c == '-' )
            return {Token::OPERATOR, std::string(1,c)};
        else if ( c == '\'' || c == '"' ) // minute/second unit
            return {Token::IDENTIFIER, std::string(1,c), std::string(1,c)};

        return {Token::INVALID, std::string(1,c)};
    }
    /**
     * \brief Reads the next token into \c lookahead
     */
    void scan()
    {
        lookahead = lex();
    }

    /**
     * \brief Extract value from lookahead
     */
    template<class T>
        T token_val()
        {
            return boost::any_cast<T>(lookahead.value);
        }
    /**
     * \brief Convert any duration type into \c Duration
     */
    template<class DurationT>
        Duration duration_cast(DurationT&& dur)
        {
            return std::chrono::duration_cast<Duration>(std::forward<DurationT>(dur));
        }

    /**
     * \brief Checks if a string is a unit description for the given duration type
     * \tparam DurationT one of the type aliases like \c seconds and friends
     */
    template <class DurationT>
        static bool is_unit(const std::string& str)
        {
            if ( std::is_same<DurationT,weeks>::value )
                return string::is_one_of(str,{"week","weeks","w"});
            if ( std::is_same<DurationT,days>::value )
                return string::is_one_of(str,{"day","days","d"});
            if ( std::is_same<DurationT,hours>::value )
                return string::is_one_of(str,{"hours","hour","h"});
            if ( std::is_same<DurationT,minutes>::value )
                return string::is_one_of(str,{"minutes","minute","m","min","'"});
            if ( std::is_same<DurationT,seconds>::value )
                return string::is_one_of(str,{"seconds","second","s","\""});
            if ( std::is_same<DurationT,milliseconds>::value )
                return string::is_one_of(str,{"milliseconds","millisecond","ms"});
            return false;
        }

    /**
     * \brief Parses an atomic duration
     * \code
     *  ATOMIC_DURATION         ::= number unit
     * \endcode
     */
    Duration parse_atomic_duration()
    {
        // check lookahead is a number
        if ( lookahead.type == Token::NUMBER )
        {
            // get the value
            Duration::rep ticks = token_val<uint32_t>();
            // advance lookahead to the next token
            scan();
            // now we should have a unit identifier
            if ( lookahead.type == Token::IDENTIFIER )
            {
                // check the known units
                if ( is_unit<milliseconds>(token_val<std::string>()) )
                {
                    scan();
                    return duration_cast(milliseconds(ticks));
                }
                else if ( is_unit<seconds>(token_val<std::string>()) )
                {
                    scan();
                    return duration_cast(seconds(ticks));
                }
                else if ( is_unit<minutes>(token_val<std::string>()) )
                {
                    scan();
                    return duration_cast(minutes(ticks));
                }
                else if ( is_unit<hours>(token_val<std::string>()) )
                {
                    scan();
                    return duration_cast(hours(ticks));
                }
                else if ( is_unit<days>(token_val<std::string>()) )
                {
                    scan();
                    return duration_cast(days(ticks));
                }
                else if ( is_unit<weeks>(token_val<std::string>()) )
                {
                    scan();
                    return duration_cast(weeks(ticks));
                }
            }
        }
        // fail
        return Duration::zero();
    }

    /**
     * \brief Parses a date and (optionally) a time
     * \code
     *  DATE_TIME       ::= DAY OPT_TIME | AT_TIME
     *  OPT_TIME        ::= (eps) | at HOUR | TIME | "T" TIME
     * \endcode
     */
    DateTime parse_date_time()
    {
        if ( lookahead.type == Token::AT )
        {
            return parse_at_time();
        }

        DateTime day = parse_day();
        if ( lookahead.type == Token::AT )
        {
            scan();
            parse_hour(day);
            return day;
        }
        else if ( lookahead.is_identifier("T") )
        {
            scan();
        }
        parse_time(day);
        return day;
    }

    /**
     * \code
     * AT_TIME  ::= at TIME [DAY]
     * \endcode
     */
    DateTime parse_at_time()
    {
        scan();

        DateTime time;
        parse_hour(time);

        if ( lookahead.type != Token::INVALID )
        {
            time.set_date(parse_day());
        }

        return time;

    }

    /**
     * \brief Parses a day
     * \code
     *  DAY     ::= rel_date | date | DATE_DESC
     * \endcode
     */
    DateTime parse_day()
    {
        if ( lookahead.type == Token::REL_DAY )
        {
            auto offset = days(token_val<int>());
            scan();
            return DateTime() + offset;
        }
        if ( lookahead.type == Token::DATE )
        {
            auto date = token_val<DateTime>();
            scan();
            return date;
        }

        return parse_date_desc();
    }

    /**
     * \brief Parses a date description
     * \code
     *  DATE_DESC       ::= OPT_WEEK_DAY month MONTH_DAY OPT_YEAR
     *                  | OPT_WEEK_DAY MONTH_DAY month OPT_YEAR
     *                  | week_day | "next" week_day
     *  OPT_WEEK_DAY    ::= (eps) | week_day
     *  ORDINAL_SUFFIX  ::= st | nd | rd | th
     *  OPT_YEAR        ::= (eps) | number
     * \endcode
     */
    DateTime parse_date_desc()
    {
        // skip "next"
        if ( lookahead.type == Token::IDENTIFIER &&
             token_val<std::string>() == "next" )
            scan();

        // skip week day
        if ( lookahead.type == Token::WEEK_DAY )
        {
            WeekDay wd = token_val<WeekDay>();
            scan();
            // Match next day with that week day
            if ( lookahead.type != Token::MONTH && lookahead.type != Token::NUMBER )
                return next_weekday(wd);
        }

        DateTime date;
        Month month = date.month();
        int day = date.day();
        int year = date.year();

        // month day
        if ( lookahead.type == Token::MONTH )
        {
            month = token_val<Month>();
            scan();
            parse_month_day(day);
        }
        // day month
        else if ( lookahead.type == Token::NUMBER )
        {
            parse_month_day(day);
            if ( lookahead.type == Token::MONTH )
            {
                month = token_val<Month>();
                scan();
            }
        }

        // year
        if ( lookahead.type == Token::NUMBER )
        {
            year = token_val<uint32_t>();
            scan();
        }

        date.set_date(year,month,days(day));
        return date;
    }

    /**
     * \brief Day matching the given week day
     */
    DateTime next_weekday(WeekDay wday)
    {
        DateTime day;
        
        if ( wday >= WeekDay::MONDAY && wday <= WeekDay::SUNDAY )
        {
            do
                day += days(1);
            while ( day.week_day() != wday );
        }

        return day;
    }

    /**
     * \brief Parse a day of the month into \c day
     * \code
     *  MONTH_DAY       ::= number | number ORDINAL_SUFFIX
     * \endcode
     */
    void parse_month_day(int& day)
    {
        if ( lookahead.type == Token::NUMBER )
        {
            uint32_t n = token_val<uint32_t>();
            if ( n < 1 || n > 31 )
                return;
            day = n;
            scan();
            if ( lookahead.type == Token::IDENTIFIER &&
                    string::is_one_of(token_val<std::string>(),{"th","st","nd","rd"}) )
                scan();
        }
    }

    /**
     * \brief Parses a time and writes it to \c out
     * \code
     *  HOUR ::= TIME | number | number AMPM
     * \endcode
     */
    void parse_hour(DateTime& out)
    {
        if ( lookahead.type == Token::NUMBER )
        {
            hours hour(token_val<uint32_t>());
            scan();
            apply_am_pm(hour);
            out.set_time(hour, minutes(0));
        }
        else if ( lookahead.type == Token::TIME )
        {
            parse_time(out);
        }
    }

    /**
     * \brief Parses a time and writes it to \c out
     * \code
     *  TIME    ::= time | time AMPM
     * \endcode
     */
    void parse_time(DateTime& out)
    {
        if ( lookahead.type != Token::TIME )
            return;
        Duration time = token_val<Duration>();

        hours hour = std::chrono::duration_cast<hours>(time);
        if ( hour.count() > 24 ) return; // invalid hour
        time -= hour;

        minutes mins = std::chrono::duration_cast<minutes>(time);
        if ( mins.count() >= 60 ) return; // invalid minute
        time -= mins;

        seconds second = std::chrono::duration_cast<seconds>(time);
        time -= second;

        milliseconds millisecond = std::chrono::duration_cast<milliseconds>(time);

        scan();
        apply_am_pm(hour);

        out.set_time(hour,mins,second,millisecond);
    }

    void apply_am_pm(hours& hour)
    {
        if ( hour.count() < 13 && lookahead.type == Token::AMPM )
        {
            bool pm = token_val<bool>();
            scan();
            if ( pm && hour.count() < 12 )
                hour += hours(12);
            else if ( !pm && hour.count() == 12 )
                hour = hours(0); // or 24? O_o
        }
    }
};

    
} // namespace time
} // namespace melanolib

#endif // TIME_PARSER_HPP
