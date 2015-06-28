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
#ifndef ERROR_HPP
#define ERROR_HPP

#include <stdexcept>

/**
 * \brief Generic Melanobot-related errors
 */
class MelanobotError : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

/**
 * \brief Class representing an error occurring during configuration
 */
class ConfigurationError : public MelanobotError
{
public:
    ConfigurationError(const std::string& msg = "Invalid configuration parameters")
        : MelanobotError(msg)
    {}
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

#endif // ERROR_HPP
