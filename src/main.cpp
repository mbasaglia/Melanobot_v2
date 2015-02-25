#include "melanobot.hpp"
#include "logger.hpp"
#include "network/async_service.hpp"
#include "network/irc.hpp"

int main(int argc, char **argv)
{
    Melanobot bot;

    Logger::singleton().register_direction('<',color::dark_green);
    Logger::singleton().register_direction('>',color::dark_yellow);
    Logger::singleton().register_direction('!',color::dark_blue);

    /// \todo register these in the proper classes
    Logger::singleton().register_log_type("irc",color::dark_magenta);
    Logger::singleton().register_log_type("dp",color::dark_cyan);
    Logger::singleton().register_log_type("std",color::white);
    Logger::singleton().register_log_type("web",color::dark_blue);

    Logger::singleton().set_log_verbosity("irc",100);
    network::irc::IrcConnection irc(&bot,network::Server{"irc.quakenet.org",6667});
    irc.run();

    return bot.run();
}


