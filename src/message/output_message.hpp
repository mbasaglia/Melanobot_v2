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
#ifndef OUTPUT_MESSAGE_HPP
#define OUTPUT_MESSAGE_HPP

#include "network/network.hpp"

namespace network {

/**
 * \brief A command to send to a connection
 */
struct Command
{
    std::string                 command;        ///< Command name
    std::vector<std::string>    parameters;     ///< Optional parameters
    int                         priority;       ///< Priority, higher = handled sooner
    Time                        timein;         ///< Time of creation
    Time                        timeout;        ///< Time it becomes obsolete

    Command ( std::string         command = {},
        std::vector<std::string>  parameters = {},
        int                       priority = 0,
        Time                      timeout = Time::max() )
    :   command(std::move(command)),
        parameters(std::move(parameters)),
        priority(priority),
        timein(Clock::now()),
        timeout(std::move(timeout))
    {}

    Command ( std::string         command,
        std::vector<std::string>  parameters,
        int                             priority,
        const Duration&                 duration )
    :   command(std::move(command)),
        parameters(std::move(parameters)),
        priority(priority),
        timein(Clock::now()),
        timeout(Clock::now()+duration)
    {}

    Command(const Command&) = default;
    Command(Command&&) noexcept = default;
    Command& operator=(const Command&) = default;
    Command& operator=(Command&&) /*noexcept(std::is_nothrow_move_assignable<std::string>::value)*/ = default;

    bool operator< ( const Command& other ) const
    {
        return priority < other.priority ||
            ( priority == other.priority && timein > other.timein );
    }
};

/**
 * \brief A message given to a connection
 *
 * This is similar to \c Command but at a higher level
 * (doesn't require knowledge of the protocol used by the connection)
 */
struct OutputMessage
{
    OutputMessage(string::FormattedString message,
                  bool action = false,
                  std::string target = {},
                  int priority = 0,
                  string::FormattedString  from = {},
                  string::FormattedString  prefix = {},
                  Time  timeout = Clock::time_point::max()
    ) : target(std::move(target)), message(std::move(message)), priority(priority),
        from(std::move(from)), prefix(std::move(prefix)),
        action(action), timeout(std::move(timeout))
    {}

    OutputMessage(const OutputMessage&) = default;
    OutputMessage(OutputMessage&&) noexcept = default;
    OutputMessage& operator=(const OutputMessage&) = default;
    OutputMessage& operator=(OutputMessage&&) /*noexcept(std::is_nothrow_move_assignable<std::string>::value)*/ = default;


    /**
     * \brief Channel or user id to which the message shall be delivered to
     */
    std::string target;
    /**
     * \brief Message contents
     */
    string::FormattedString message;
    /**
     * \brief Priority, higher = handled sooner
     */
    int priority = 0;
    /**
     * \brief If not empty, the bot will make it look like the message comes from this user
     */
    string::FormattedString from;
    /**
     * \brief Prefix to prepend to the message
     */
    string::FormattedString prefix;
    /**
     * \brief Whether the message is an action
     */
    bool action = false;
    /**
     * \brief Time at which this message becomes obsolete
     */
    Time timeout;
};

} // namespace network
#endif // OUTPUT_MESSAGE_HPP
