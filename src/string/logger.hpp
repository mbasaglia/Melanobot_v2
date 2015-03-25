/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015 Mattia Basaglia
 * \license
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
#include <mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include <boost/chrono/io/time_point_io.hpp>

#include "multibuf.hpp"
#include "string.hpp"
#include "settings.hpp"

/**
 * \brief Singleton class handling logs
 * \see Log for a nicer interface
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
     * \thread any \lock mutex
     */
    void log (const std::string& type, char direction,
        const string::FormattedString& message, int verbosity);

    void load_settings(const Settings& settings);

private:
    /**
     * \brief Data associated with a log type
     */
    struct LogType
    {
        color::Color12 color;
        int verbosity;
    };

    Logger()
    {
        log_buffer.push_buffer(std::cout.rdbuf());
    }
    Logger(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger& operator=(Logger&&) = delete;

    Multibuf     log_buffer;
    std::ostream log_destination {&log_buffer};
    std::unordered_map<std::string, LogType> log_types;
    std::unordered_map<char, color::Color12> log_directions;
    unsigned log_type_length = 0;
    string::Formatter* formatter = nullptr;
    std::string timestamp = "%Y-%m-%d %T";
    std::mutex mutex;
};

/**
 * \brief Simple log stream-like interface
 */
class Log
{
public:
    Log(std::string type, char direction, int verbosity = 2)
        : type(std::move(type)), direction(direction), verbosity(verbosity)
    {}

    Log(const std::string& type, char direction, const std::string& message, int verbosity = 2)
        : Log(type,direction,verbosity)
    {
        stream << message;
    }
    Log(const Log&) = delete;
    Log(Log&&) = delete;
    Log& operator=(const Log&) = delete;
    Log& operator=(Log&&) = delete;

    ~Log()
    {
        stream << string::ClearFormatting();
        Logger::instance().log(type, direction, stream, verbosity);
    }

    template<class T>
        Log& operator<< ( const T& t )
        {
            stream << t;
            return *this;
        }

public:
    std::string type;
    char direction;
    int verbosity;
    string::FormattedString stream;
};

/**
 * \brief Utility for error messages
 */
class ErrorLog : public Log
{
public:
    ErrorLog(const std::string& type, const std::string& error = "Error" )
        : Log(type,'!',0)
    {
        stream << string::FormatFlags::BOLD << color::red << error
            << string::ClearFormatting() << ": ";
    }
};

/**
 * \brief Exception that can be traced to a line in a file
 */
struct LocatableException : public std::logic_error
{
    std::string file;     ///< Source file name originating the error
    int         line = 0; ///< Source line number originating the error

    LocatableException( std::string file,
                        int line, const std::string& msg )
        : std::logic_error(msg), file(std::move(file)), line(line) {}
};


/**
 * \brief Critical error exception
 *
 * Class representing an error that cannot be recovered from or that
 * doesn't allow any meaningful continuation of the program
 */
struct CriticalException : public LocatableException
{

    CriticalException( std::string file,     int line,
                       std::string function, std::string msg )
        : LocatableException(file, line, msg), function(std::move(function)) {}

    std::string function; ///< Source function name originating the error
};


/**
 * \brief Throws an exception with a standardized format
 * \throws CriticalException
 */
inline void error [[noreturn]] (const std::string& file, int line,
                         const std::string& function, const std::string& msg )
{
    throw CriticalException(file,line,function,msg);
}
/**
 * \brief Throws an exception pointing to the call line
 * \throws CriticalException
 */
#define CRITICAL_ERROR(msg) \
        error(__FILE__,__LINE__,__func__,msg)

#endif // LOGGER_HPP
