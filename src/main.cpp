#include "melanobot.hpp"
#include "logger.hpp"
#include "network/async_service.hpp"

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

    return bot.run();
}


