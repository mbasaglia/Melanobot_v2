/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015-2016 Mattia Basaglia
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

#include "multibuf.hpp"
#include "string.hpp"
#include "settings.hpp"

/**
 * \brief Singleton class handling logs
 * \see Log for a nicer interface
 */
class Logger : public melanolib::Singleton<Logger>
{
public:

    /**
     * \brief Register a log "direction"
     *
     * A direction is a simple identifier showing what kind of message
     * has been logged
     */
    void register_direction(char name, color::Color12 color);

    /**
     * \brief Register a log type
     *
     * A log type is the name of the component which generates the log.
     * Better to keep it short, 3 letters should do.
     * The default verbosity is 2
     */
    void register_log_type(const std::string& name, color::Color12 color);

    /**
     * \brief Change verbosity level for a given log type
     *
     * Messages of that type with higher verbisity will be discarded
     */
    void set_log_verbosity(const std::string& name, int level);

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

        LogType(color::Color12 color = {},  int verbosity = 2)
            : color(color), verbosity(verbosity) {}

        color::Color12 color;
        int verbosity = 2;
    };

    Logger()
    {
        log_buffer.push_buffer(std::cout.rdbuf());
    }
    friend ParentSingleton;

    static string::FormatterAnsi default_formatter;
    Multibuf     log_buffer;
    std::ostream log_destination {&log_buffer};
    std::unordered_map<std::string, LogType> log_types;
    std::unordered_map<char, color::Color12> log_directions;
    unsigned log_type_length = 0;
    string::Formatter* formatter = &default_formatter;
    std::string timestamp = "[Y-m-d H:i:s]";
    std::recursive_mutex mutex;
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
        : Log(type, direction, verbosity)
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
    ErrorLog(const std::string& type,
             const std::string& error = "Error",
             int verbosity = 0 )
        : Log(type, '!', verbosity)
    {
        stream << string::FormatFlags::BOLD << color::red << error
            << string::ClearFormatting() << ": ";
    }
};

#endif // LOGGER_HPP
