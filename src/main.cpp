#include "melanobot.hpp"
#include "string/logger.hpp"
#include "settings.hpp"

int main(int argc, char **argv)
{
    Logger::instance().register_direction('<',color::dark_green);
    Logger::instance().register_direction('>',color::dark_yellow);
    Logger::instance().register_direction('!',color::dark_blue);

    /// \todo register these in the proper classes
    REGISTER_LOG_TYPE(dp,color::dark_cyan);
    REGISTER_LOG_TYPE(sys,color::dark_red);

    try {
        Settings settings = Settings::initialize(argc,argv);

        Logger::instance().load_settings(settings.get_child("log",{}));

        if ( !settings.empty() )
        {
            Log("sys",'!',0) << "Executing from " << Settings::global_settings.get("config","");
            Melanobot bot(settings);
            bot.run();
            /// \todo some way to reload the config and restart the bot
        }
        return Settings::global_settings.get("exit_code",0);

    } catch ( const CriticalException& exc ) {
        /// \todo policy on how to handle exceptions
        ErrorLog errlog("sys","Critical Error");
        if ( Settings::global_settings.get("debug",0) )
            errlog << exc.file << ':' << exc.line << ": in " << exc.function << "(): ";
        errlog  << exc.what();
        return 1;
    } catch ( const std::exception& exc ) {
        ErrorLog ("sys","Critical Error") << exc.what();
        return 1;
    }

}
