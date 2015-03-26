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
#ifndef XONOTIC_HANDLER_RCON_HPP
#define XONOTIC_HANDLER_RCON_HPP

#include "handler/handler.hpp"

namespace xonotic {

/**
 * \brief Sends a rcon command
 * \param destination Xonotic connection
 * \param command Rcon command and arguments
 */
void rcon(network::Connection* destination,
          std::vector<std::string> command,
          int priority = 0
         )
{
    destination->command({"rcon",std::move(command),priority});
}

/**
 * \brief Send a fixed rcon command to a Xonotic connection
 */
class RconCommand : public handler::SimpleAction
{
public:
    RconCommand(const Settings& settings, handler::HandlerContainer* parent)
        : SimpleAction(settings.get("command",settings.data()),settings,parent)
    {
        /// \note it allows the command to be specified in the top-level data
        command = settings.get("command",settings.data());
        if ( command.empty() )
            throw ConfigurationError{};
        arguments = settings.get("arguments",arguments);
        if ( arguments )
            synopsis += " argument...";
        /// \todo would be cool to gather help from the server
        ///       or at least from settings
        help = "Performs the Rcon command \""+command+"\"";
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        std::vector<std::string> args = { command };
        if ( arguments && !msg.message.empty() )
            args.push_back(msg.message);
        rcon(msg.destination,std::move(args),priority);
        return true;
    }

private:
    std::string command;        ///< Rcon command to be sent
    bool        arguments{true};///< Whether to allow command arguments
};

} // namespace xonotic
#endif // XONOTIC_HANDLER_RCON_HPP
