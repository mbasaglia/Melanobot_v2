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
#include "logger.hpp"
#include "melanolib/time/time_string.hpp"
#include "concurrency/concurrency.hpp"
#include "string/replacements.hpp"

string::FormatterAnsi Logger::default_formatter{true};

void Logger::log(const std::string& type, char direction,
    const string::FormattedString& message, int verbosity)
{

    std::unique_lock<std::mutex> lock(mutex);

    auto type_it = log_types.find(type);
    if ( type_it != log_types.end() && type_it->second.verbosity < verbosity )
        return;

    auto log_type_length = this->log_type_length;
    auto type_color = type_it != log_types.end() ? type_it->second.color : color::Color12();
    auto direction_color = log_directions[direction];
    auto timestamp = this->timestamp;

    lock.unlock();

    string::FormattedString line;
    if ( !timestamp.empty() )
    {
        line << color::yellow << melanolib::time::format(timestamp) << string::ClearFormatting();
    }

    line << type_color << string::Padding(type, log_type_length, 0)
         << direction_color << direction << string::ClearFormatting()
         << message << string::ClearFormatting();

    std::string buffer = line.encode(*formatter) + '\n';

    lock.lock();
    log_destination.write(buffer.data(), buffer.size());
    log_destination.flush();
}


void Logger::register_log_type(const std::string& name, color::Color12 color)
{
    std::lock_guard<std::mutex> lock(mutex);
    if ( log_type_length < name.size() )
        log_type_length = name.size();
    log_types[name].color = color;
}

void Logger::register_direction(char name, color::Color12 color)
{
    std::lock_guard<std::mutex> lock(mutex);
    log_directions[name] = color;
}

void Logger::set_log_verbosity(const std::string& name, int level)
{
    std::lock_guard<std::mutex> lock(mutex);
    log_types[name].verbosity = level;
}

void Logger::load_settings(const Settings& settings)
{
    std::unique_lock<std::mutex> lock(mutex);

    std::string format = settings.get("string_format", "ansi-utf8");
    formatter = string::Formatter::formatter(format);
    timestamp = settings.get("timestamp", timestamp);
    for ( const auto&p  : settings.get_child("verbosity", {}) )
    {
        auto type_it = log_types.find(p.first);
        if ( type_it != log_types.end() )
        {
            type_it->second.verbosity = p.second.get_value(type_it->second.verbosity);
        }
        else
        {
            if ( log_type_length < p.first.size() )
                log_type_length = p.first.size();
            log_types.insert({p.first,
                LogType(color::nocolor, p.second.get_value(2))});
        }
    }

    /// \todo maybe should use different a formatter (ie: plain utf8) for log files
    std::string output = settings.get("logfile", "");
    if ( !output.empty() )
    {
        if ( !log_buffer.push_file(output) )
        {
            lock.unlock();
            ErrorLog("sys") << "Cannot open log file: " << output;
        }
    }
}
