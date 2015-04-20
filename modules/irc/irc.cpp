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
#include "irc/network/connection.hpp"
#include "irc/irc_formatter.hpp"
#include "irc/handler/ctcp.hpp"
#include "irc/handler/irc_actions.hpp"
#include "irc/handler/irc_admin.hpp"
#include "irc/handler/irc_whois.hpp"
#include "melanomodule.hpp"

/**
 * \brief Initializes the IRC module
 */
Melanomodule melanomodule_irc()
{
    Melanomodule module{"irc","IRC integration"};
    module.register_connection<irc::IrcConnection>("irc");
    module.register_log_type("irc",color::dark_magenta);
    module.register_formatter<irc::FormatterIrc>();

    module.register_handler<irc::handler::CtcpVersion>("CtcpVersion");
    module.register_handler<irc::handler::CtcpSource>("CtcpSource");
    module.register_handler<irc::handler::CtcpUserInfo>("CtcpUserInfo");
    module.register_handler<irc::handler::CtcpPing>("CtcpPing");
    module.register_handler<irc::handler::CtcpTime>("CtcpTime");
    module.register_handler<irc::handler::CtcpClientInfo>("CtcpClientInfo");

    module.register_handler<irc::handler::IrcKickRejoin>("IrcKickRejoin");

    module.register_handler<irc::handler::AdminNick>("Nick");
    module.register_handler<irc::handler::AdminJoin>("Join");
    module.register_handler<irc::handler::AdminPart>("Part");
    module.register_handler<irc::handler::AcceptInvite>("AcceptInvite");
    module.register_handler<irc::handler::AdminRaw>("Raw");

    module.register_handler<irc::handler::Whois330>("Whois330");
    module.register_handler<irc::handler::QSendWhois>("QSendWhois");
    module.register_handler<irc::handler::QGetWhois>("QGetWhois");
    module.register_handler<irc::handler::WhoisCheckMe>("WhoisCheckMe");

    return module;
}
