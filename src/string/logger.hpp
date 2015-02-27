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
#include <stdexcept>
#include <string>
#include <unordered_map>

#include <boost/chrono/io/time_point_io.hpp>

#include "string.hpp"
#include "settings.hpp"

/**
 * \brief Singleton class handling logs
 * \see Log for a nicer interface
 * \todo read settings
 */
class Logger
{
public:

    /**
     * \brief Singleton instance
     */
    static Logger& instance()
    {
        static Logger singleton;
        return singleton;
    }

    /**
     * \brief Register a log "direction"
     *
     * A direction is a simple identifier showing what kind of message
     * has been logged
     */
    void register_direction(char name, color::Color12 color)
    {
        log_directions[name] = color;
    }

    /**
     * \brief Register a log type
     *
     * A log type is the name of the component which generates the log.
     * Better to keep it short, 3 letters should do.
     * The default verbosity is 2
     */
    void register_log_type(const std::string& name, color::Color12 color)
    {
        if ( log_type_length < name.size() )
            log_type_length = name.size();
        log_types[name] = {color,2};
    }

    /**
     * \brief Change verbosity level for a given log type
     *
     * Messages of that type with higher verbisity will be discarded
     */
    void set_log_verbosity(const std::string& name, int level)
    {
        log_types[name].verbosity = level;
    }

    /**
     * \brief Log a message
     */
    void log (const std::string& type, char direction,
        const string::FormattedString& message, int verbosity)
    {
        /// \todo lock mutex for log_destination
        auto type_it = log_types.find(type);
        if ( type_it != log_types.end() && type_it->second.verbosity < verbosity )
            return;

        if ( !timestamp.empty() )
        {
            log_destination <<  formatter->color(color::yellow) << '['
                << boost::chrono::time_fmt(boost::chrono::timezone::local, timestamp)
                << boost::chrono::system_clock::now()
                << ']' << formatter->clear();
        }

        if ( type_it != log_types.end() )
            log_destination <<  formatter->color(type_it->second.color);
        log_destination << std::setw(log_type_length) << std::left << type;

        log_destination << formatter->color(log_directions[direction]) << direction;

        log_destination << formatter->clear() << message.encode(formatter)
            << formatter->clear() << std::endl;
    }

    void load_settings(const settings::Settings& settings)
    {
        std::string format = settings.get("format",std::string("ansi-utf8"));
        formatter = string::Formatter::formatter(format);
        timestamp = settings.get("timestamp",std::string("%Y-%m-%d %T"));
        /// \todo read verbosity levels
    }

private:
    /**
     * \brief Data associated with a log type
     */
    struct LogType
    {
        color::Color12 color;
        int verbosity;
    };

    Logger() {}
    Logger(const Logger&) = delete;

    std::ostream log_destination {std::cout.rdbuf()};
    std::unordered_map<std::string, LogType> log_types;
    std::unordered_map<char, color::Color12> log_directions;
    unsigned log_type_length = 0;
    string::Formatter* formatter = nullptr;
    std::string timestamp;
};

/**
 * \brief Simple log stream-like interface
 */
class Log
{
public:
    Log(const std::string& type, char direction, int verbosity = 2)
        : type(type), direction(direction), verbosity(direction)
    {}

    Log(const std::string& type, char direction, const std::string& message, int verbosity = 2)
        : Log(type,direction,verbosity)
    {
        stream << message;
    }

    Log(const Log&) = delete;
    Log& operator= (const Log&) = delete;

    ~Log()
    {
        if ( color )
            stream << color::nocolor;
        Logger::instance().log(type, direction, stream.str(), verbosity);
    }

    template<class T>
        const Log& operator<< ( const T& t ) const
        {
            stream << t;
            return *this;
        }

    const Log& operator<< ( const color::Color12& t ) const
    {
        color = true;
        stream << t;
        return *this;
    }

public:
    std::string type;
    char direction;
    int verbosity;
    string::FormattedStream stream;
    mutable bool color = false;
};

/**
 * \brief Throws an exception with a standardized format
 */
inline void error [[noreturn]] (const std::string& file, int line,
                         const std::string& function, const std::string& msg )
{
    throw std::logic_error(function+"@"+file+":"+std::to_string(line)+": error: "+msg);
}
/**
 * \brief Throws an exception pointing to the call line
 */
#define CRITICAL_ERROR(msg) \
        error(__FILE__,__LINE__,__func__,msg)

#endif // LOGGER_HPP
