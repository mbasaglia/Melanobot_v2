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
#include "logger.hpp"


void Logger::log (const std::string& type, char direction,
    const string::FormattedString& message, int verbosity)
{
    if ( !formatter )
        formatter = new string::FormatterAnsi(true); // This happens when everything else has failed and will leak

    auto type_it = log_types.find(type);
    if ( type_it != log_types.end() && type_it->second.verbosity < verbosity )
        return;

    mutex.lock();
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
    mutex.unlock();
}

void Logger::load_settings(const Settings& settings)
{
    std::string format = settings.get("string_format","ansi-utf8");
    formatter = string::Formatter::formatter(format);
    timestamp = settings.get("timestamp",timestamp);
    for ( const auto&p  : settings.get_child("verbosity",{}) )
    {
        auto type_it = log_types.find(p.first);
        if ( type_it != log_types.end() )
            type_it->second.verbosity = p.second.get_value(type_it->second.verbosity);
    }
    /// \todo maybe should use different a formatter (ie: plain utf8) for log files
    std::string output = settings.get("logfile","");
    if ( !output.empty() )
    {
        if ( !log_buffer.push_file(output) )
            ErrorLog("sys") << "Cannot open log file: " << output;
    }
}
