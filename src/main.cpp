#include "melanobot.hpp"
#include "string/logger.hpp"
#include "network/async_service.hpp"
#include "network/irc.hpp"
#include "string/string.hpp"

int main(int argc, char **argv)
{
    settings::initialize(argc,argv);

    Logger::instance().register_direction('<',color::dark_green);
    Logger::instance().register_direction('>',color::dark_yellow);
    Logger::instance().register_direction('!',color::dark_blue);

    /// \todo register these in the proper classes
    Logger::instance().register_log_type("irc",color::dark_magenta);
    Logger::instance().register_log_type("dp",color::dark_cyan);
    Logger::instance().register_log_type("std",color::white);
    Logger::instance().register_log_type("web",color::dark_blue);
    Logger::instance().register_log_type("sys",color::red);

    std::string settings_file;

    /// \todo Read arguments

    settings_file = settings::find_config();

    if ( settings_file.empty() )
        CRITICAL_ERROR("Cannot start without a config file");
    settings::Settings settings = settings::load(settings_file);

    Logger::instance().load_settings(settings.get_child("log",{}));
    
    Log("sys",'!',0) << "Executing from " << settings_file;

    Melanobot bot(settings);

    Logger::instance().set_log_verbosity("irc",100);
    network::irc::IrcConnection irc(&bot,network::Server{"irc.quakenet.org",6667});
    irc.run();

    return bot.run();
}
