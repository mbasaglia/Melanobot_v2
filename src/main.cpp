/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015-2017 Mattia Basaglia
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
#include "melanobot/melanobot.hpp"
#include "string/logger.hpp"
#include "settings.hpp"
#include "network/async_service.hpp"
#include "module/load_module.hpp"
#include "melanobot/storage.hpp"
#include "melanobot/config_factory.hpp"

/**
 * \brief Initializes static components
 */
void initialize_static()
{
    Logger::instance().register_direction('<', color::dark_green);
    Logger::instance().register_direction('>', color::dark_yellow);
    Logger::instance().register_direction('!', color::dark_blue);

    Logger::instance().register_log_type("sys", color::dark_red);

    string::Formatter::registry(); // ensures the default formatters get loaded
}

/**
 * \brief Initializes global components
 */
Settings initialize_global(int argc, char **argv)
{
    initialize_static();

    // Load settings and environment
    Settings settings = settings::initialize(argc, argv);

    if ( settings.empty() )
        throw melanobot::ConfigurationError("Missing configuration");

    Log("sys", '!', 0) << "Executing from " << settings::global_settings.get("config", "");

    // Log configuration
    Logger::instance().load_settings(settings.get_child("log", {}));

    // Load modules
    auto lib_path = settings::global_settings.get("path.library", "");

    auto modules = module::initialize_modules<const Settings&>(
        melanolib::string::char_split(lib_path, ':'),
        settings
    );

    // Initialize storage
    melanobot::StorageFactory::instance().initilize_global_storage(
        settings.get_child("storage", {})
    );

    return settings;
}

void run_bot(const Settings& settings)
try
{
    auto& factory = melanobot::ConfigFactory::instance();
    factory.load_templates(settings.get_child("templates", {}));
    factory.build(settings.get_child("bot", {}), &melanobot::Melanobot::instance());

    melanobot::Melanobot::instance().start();
    melanobot::Melanobot::instance().run();
    melanobot::Melanobot::instance().stop("main", "end of execution");
    /// \todo some way to reload the config and restart the bot
}
catch ( const melanobot::MelanobotError& exc )
{
    /// \todo policy on how to handle exceptions (quit/restart)
    ErrorLog ("sys", "Unhandled Error") << exc.what();
    settings::global_settings.get("exit_code", 1);
}

void run_services(const Settings& settings)
try
{
    ServiceRegistry::instance().initialize(settings.get_child("services", {}));
    ServiceRegistry::instance().start();

    run_bot(settings);

    ServiceRegistry::instance().stop();
}
catch ( const melanobot::MelanobotError& exc )
{
    ErrorLog ("sys", "Service Initialization Error") << exc.what();
    settings::global_settings.get("exit_code", 1);
}

int main(int argc, char **argv)
{
    try
    {
        auto settings = initialize_global(argc, argv);
        run_services(settings);

        // Finalize for a clean exit
        int exit_code = settings::global_settings.get("exit_code", 0);
        Log("sys", '!', 4) << "Exiting with status " << exit_code;
        return exit_code;
    }
    catch ( const std::exception& exc )
    {
        ErrorLog ("sys", "Critical Error") << exc.what();
        return 1;
    }
    catch ( ... )
    {
        ErrorLog ("sys", "Unexpected Error");
        return 1;
    }
}
