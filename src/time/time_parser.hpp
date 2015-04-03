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
#ifndef TIME_PARSER_HPP
#define TIME_PARSER_HPP

#include "string/string_functions.hpp"
#include "time/time.hpp"

namespace timer {
/**
 * \brief Parses a time description
 */
class TimeParser
{
public:
    using Clock    = DateTime::Clock;
    using Time     = typename Clock::time_point;
    using Duration = typename Clock::duration;

    TimeParser(std::istream& input) : input(input.rdbuf())
    {
        scan();
    }

    /**
     * \brief Parses a time point
     * \code
     *  TIME_POINT      ::= NOW_TIME | DAY_TIME
     *  NOW_TIME        ::= now
     *                  |   now + DURATION
     *                  |   now - DURATION
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
                scan();
                if ( lookahead.lexeme == "+" )
                    now += parse_duration();
                else if ( lookahead.lexeme == "-" )
                    now -= parse_duration();
            }
            return now;
        }
        return DateTime();
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

        // Read multiple number/unit pairs
        while ( lookahead.type == Token::NUMBER )
            duration += parse_atomic_duration();

        return duration;
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
        };
        Type            type{INVALID};  ///< Token type
        std::string     lexeme;         ///< Corresponding string
        boost::any      value;          ///< Associated value

        Token(){};
        Token(Type type, std::string lexeme)
            : type(type), lexeme(std::move(lexeme)) {}
        template<class T>
            Token(Type type, std::string lexeme, T val)
                : type(type), lexeme(std::move(lexeme)), value(std::move(val)) {}
    };

    Token        lookahead;     ///< Last read token
    std::istream input;         ///< Input stream


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
        else if ( lower == "today" )
            return {Token::REL_DAY, id, 0};
        else if ( lower == "tomorrow" )
            return {Token::REL_DAY, id, 1};
        else if ( lower == "yesterday" )
            return {Token::REL_DAY, id, -1};
        else
            return {Token::IDENTIFIER, id, lower};
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
        if ( c != '-' )
            return {};

        if ( month > Month::DECEMBER || month < Month::JANUARY ||
            day < 1 || day > DateTime::month_days(year,month) )
            return {};


        input.unget();
        return {Token::DATE, lexed, DateTime(year,month,days(day))};
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
        {
            return lex_time(lexed);
        }
        input.unget();
        return {Token::NUMBER, lexed, uint32_t(string::to_uint(lexed))};
    }

    /**
     * \brief Reads the next token
     */
    Token lex()
    {
        char c = ' ';
        while ( std::isspace(c) && input )
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

        return {};
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
            }
        }
        // fail
        return Duration::zero();
    }
};
} // namespace timer
#endif // TIME_PARSER_HPP
