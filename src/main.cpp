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
#include "melanobot.hpp"
#include "string/logger.hpp"
#include "settings.hpp"
#include "network/async_service.hpp"
#include "load_module.hpp"

int main(int argc, char **argv)
{
    Logger::instance().register_direction('<',color::dark_green);
    Logger::instance().register_direction('>',color::dark_yellow);
    Logger::instance().register_direction('!',color::dark_blue);

    string::Formatter::registry(); // ensures the default formatters get loaded

    try {
        Settings settings = settings::initialize(argc, argv);
        Logger::instance().load_settings(settings.get_child("log",{}));

        auto lib_path = settings::global_settings.get("path.library", "");

        auto modules = module::initialize_modules<const Settings&>(
            string::char_split(lib_path, ':'),
            settings
        );

        // Load log settings again to ensure log types defined by modules
        // are handled correctly
        Logger::instance().load_settings(settings.get_child("log",{}));

        if ( !settings.empty() )
        {
            Log("sys",'!',0) << "Executing from " << settings::global_settings.get("config","");
            network::ServiceRegistry::instance().initialize(settings.get_child("services",{}));
            network::ServiceRegistry::instance().start();
            Melanobot::load(settings).run();
            network::ServiceRegistry::instance().stop();
            /// \todo some way to reload the config and restart the bot
        }

        int exit_code = settings::global_settings.get("exit_code",0);
        Log("sys",'!',4) << "Exiting with status " << exit_code;
        return exit_code;

    } catch ( const CriticalException& exc ) {
        /// \todo policy on how to handle exceptions (quit/restart)
        ErrorLog errlog("sys","Critical Error");
        if ( settings::global_settings.get("debug",0) )
            errlog << exc.file << ':' << exc.line << ": in " << exc.function << "(): ";
        errlog  << exc.what();
        return 1;
    } catch ( const LocatableException& exc ) {
        ErrorLog errlog("sys","Critical Error");
        if ( settings::global_settings.get("debug",0) )
            errlog << exc.file << ':' << exc.line << ": ";
        errlog  << exc.what();
        return 1;
    } catch ( const std::exception& exc ) {
        ErrorLog ("sys","Critical Error") << exc.what();
        return 1;
    }

}
