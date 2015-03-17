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
    /**
     * \todo Extract more stuff from src/ into modules/
     */

    Melanomodule module{"irc","IRC integration"};
    module.register_connection<irc::IrcConnection>("irc");
    module.register_log_type("irc",color::dark_magenta);
    module.register_formatter<irc::FormatterIrc>();

    REGISTER_HANDLER(irc::handler::CtcpVersion,CtcpVersion);
    REGISTER_HANDLER(irc::handler::CtcpSource,CtcpSource);
    REGISTER_HANDLER(irc::handler::CtcpUserInfo,CtcpUserInfo);
    REGISTER_HANDLER(irc::handler::CtcpPing,CtcpPing);
    REGISTER_HANDLER(irc::handler::CtcpTime,CtcpTime);
    REGISTER_HANDLER(irc::handler::CtcpClientInfo,CtcpClientInfo);

    REGISTER_HANDLER(irc::handler::IrcJoinMessage,IrcJoinMessage);
    REGISTER_HANDLER(irc::handler::IrcKickMessage,IrcKickMessage);
    REGISTER_HANDLER(irc::handler::IrcKickRejoin,IrcKickRejoin);

    REGISTER_HANDLER(irc::handler::AdminNick,Nick);
    REGISTER_HANDLER(irc::handler::AdminJoin,Join);
    REGISTER_HANDLER(irc::handler::AdminPart,Part);
    REGISTER_HANDLER(irc::handler::AcceptInvite,AcceptInvite);
    REGISTER_HANDLER(irc::handler::AdminRaw,Raw);

    REGISTER_HANDLER(irc::handler::Whois330,Whois330);
    REGISTER_HANDLER(irc::handler::QSendWhois,QSendWhois);
    REGISTER_HANDLER(irc::handler::QGetWhois,QGetWhois);
    REGISTER_HANDLER(irc::handler::WhoisCheckMe,WhoisCheckMe);

    return module;
}
