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
#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <iomanip>
#include <iostream>
#include <string>
#include <unordered_map>

//#include <boost/chrono.hpp>
#include <boost/chrono/io/time_point_io.hpp>
//#include <boost/chrono/io/timezone.hpp>

#include "color.hpp"

class Logger
{
public:
    static const int colors    = 0x1;
    static const int timestamp = 0x2;

    static Logger& singleton()
    {
        static Logger instance;
        return instance;
    }

    void register_direction(char name, color::Color12 color)
    {
        log_directions[name] = color;
    }

    void register_log_type(const std::string& name, color::Color12 color)
    {
        if ( log_type_length < name.size() )
            log_type_length = name.size();
        log_types[name] = {color,2};
    }

    void set_log_verbosity(const std::string& name, int level)
    {
        log_types[name].verbosity = level;
    }

    void log (const std::string& type, char direction, const std::string& message, int verbosity)
    {
        auto type_it = log_types.find(type);
        if ( type_it != log_types.end() && type_it->second.verbosity < verbosity )
            return;

        if ( log_flags & timestamp )
        {
            put_color(color::yellow);
            log_destination << '['
                << boost::chrono::time_fmt(boost::chrono::timezone::local, "%Y-%m-%d %T")
                << boost::chrono::system_clock::now()
                << ']';
            put_color(color::nocolor);
        }

        if ( type_it != log_types.end() )
            put_color(type_it->second.color);
        log_destination << std::setw(log_type_length) << std::left << type;

        put_color(log_directions[direction]);
        log_destination << direction;
        put_color(color::nocolor);
        log_destination << message << std::endl;
    }

private:
    struct LogType
    {
        color::Color12 color;
        int verbosity;
    };

    Logger() {}
    Logger(const Logger&) = delete;

    void put_color( const color::Color12& color )
    {
        if ( log_flags & colors )
            log_destination << color.to_ansi();
    }

    int log_flags = colors|timestamp;
    std::ostream log_destination {std::cout.rdbuf()};
    std::unordered_map<std::string, LogType> log_types;
    std::unordered_map<char, color::Color12> log_directions;
    unsigned log_type_length = 0;
};

void log (const std::string& type, char direction, const std::string& message, int verbosity = 2)
{
    Logger::singleton().log(type, direction, message, verbosity);
}

#endif // LOGGER_HPP
