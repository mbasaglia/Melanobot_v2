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


Melanomodule melanomodule_core()
{
    Melanomodule module {"core", "Core module"};

    module.register_log_type("sys",color::dark_red);

    REGISTER_HANDLER(handler::AdminQuit,Quit);
    REGISTER_HANDLER(handler::AdminGroup,AdminGroup);
    REGISTER_HANDLER(handler::FilterGroup,FilterGroup);
    REGISTER_HANDLER(handler::AdminReconnect,Reconnect);
    REGISTER_HANDLER(handler::AdminConnect,Connect);
    REGISTER_HANDLER(handler::AdminDisconnect,Disconnect);
    REGISTER_HANDLER(handler::Chanhax,Chanhax);

    REGISTER_HANDLER(handler::Bridge,Bridge);
    REGISTER_HANDLER(handler::BridgeChat,BridgeChat);

    REGISTER_HANDLER(handler::License,License);
    REGISTER_HANDLER(handler::Help,Help);
    REGISTER_HANDLER(handler::Echo,Echo);
    REGISTER_HANDLER(handler::ServerHost,ServerHost);
    REGISTER_HANDLER(handler::Cointoss,Cointoss);
    REGISTER_HANDLER(handler::Reply,Reply);

    REGISTER_HANDLER(handler::SimpleGroup, Group);

    return module;
}
