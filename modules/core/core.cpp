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

#include "melanomodule.hpp"
#include "handler/admin.hpp"
#include "handler/bridge.hpp"
#include "handler/misc.hpp"
#include "handler/group.hpp"
#include "handler/connection_monitor.hpp"

/**
 * \brief Defines the core module
 */
MELANOMODULE_ENTRY_POINT module::Melanomodule melanomodule_core_metadata()
{
    return {"core", "Core module"};
}


MELANOMODULE_ENTRY_POINT void melanomodule_core_initialize(const Settings&)
{
    module::register_log_type("sys",color::dark_red);

    module::register_handler<core::AdminQuit>("Quit");
    module::register_handler<core::AdminGroup>("AdminGroup");
    module::register_handler<core::FilterGroup>("FilterGroup");
    module::register_handler<core::AdminReconnect>("Reconnect");
    module::register_handler<core::AdminConnect>("Connect");
    module::register_handler<core::AdminDisconnect>("Disconnect");
    module::register_handler<core::Chanhax>("Chanhax");

    module::register_handler<core::Bridge>("Bridge");
    module::register_handler<core::BridgeChat>("BridgeChat");
    module::register_handler<core::BridgeAttach>("BridgeAttach");
    module::register_handler<core::BridgeAttachChannel>("BridgeAttachChannel");

    module::register_handler<core::MonitorServerStatus>("MonitorServerStatus");
    module::register_handler<core::MonitorReply>("MonitorReply");

    module::register_handler<core::JoinMessage>("JoinMessage");
    module::register_handler<core::PartMessage>("PartMessage");
    module::register_handler<core::KickMessage>("KickMessage");
    module::register_handler<core::RenameMessage>("RenameMessage");

    module::register_handler<core::License>("License");
    module::register_handler<core::Help>("Help");
    module::register_handler<core::Echo>("Echo");
    module::register_handler<core::ServerHost>("ServerHost");
    module::register_handler<core::Cointoss>("Cointoss");
    module::register_handler<core::Reply>("Reply");
    module::register_handler<core::Action>("Action");
    module::register_handler<core::Command>("Command");
    module::register_handler<core::Time>("Time");

    module::register_handler<core::Group>("Group");
    module::register_handler<core::Multi>("Multi");
    module::register_handler<core::IfSet>("IfSet");
}
