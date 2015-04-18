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
#include "handler/admin.hpp"
#include "handler/bridge.hpp"
#include "handler/misc.hpp"
#include "handler/simple-group.hpp"

/**
 * \brief Defines the core module
 */
Melanomodule melanomodule_core()
{
    Melanomodule module {"core", "Core module"};

    module.register_log_type("sys",color::dark_red);

    module.register_handler<handler::AdminQuit>("Quit");
    module.register_handler<handler::AdminGroup>("AdminGroup");
    module.register_handler<handler::FilterGroup>("FilterGroup");
    module.register_handler<handler::AdminReconnect>("Reconnect");
    module.register_handler<handler::AdminConnect>("Connect");
    module.register_handler<handler::AdminDisconnect>("Disconnect");
    module.register_handler<handler::Chanhax>("Chanhax");

    module.register_handler<handler::Bridge>("Bridge");
    module.register_handler<handler::BridgeChat>("BridgeChat");
    module.register_handler<handler::BridgeAttach>("BridgeAttach");
    module.register_handler<handler::BridgeAttachChannel>("BridgeAttachChannel");
    module.register_handler<handler::JoinMessage>("JoinMessage");

    module.register_handler<handler::License>("License");
    module.register_handler<handler::Help>("Help");
    module.register_handler<handler::Echo>("Echo");
    module.register_handler<handler::ServerHost>("ServerHost");
    module.register_handler<handler::Cointoss>("Cointoss");
    module.register_handler<handler::Reply>("Reply");
    module.register_handler<handler::Action>("Action");

    module.register_handler<handler::SimpleGroup>("Group");

    return module;
}
