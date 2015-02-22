#include "melanobot.hpp"
#include "logger.hpp"

int main(int argc, char **argv)
{
    melanobot::Melanobot bot;

    Logger::singleton().register_direction('<',color::dark_green);
    Logger::singleton().register_direction('>',color::dark_yellow);
    Logger::singleton().register_direction('!',color::dark_blue);

    /// \todo register these in the proper classes
    Logger::singleton().register_log_type("irc",color::dark_magenta);
    Logger::singleton().register_log_type("dp",color::dark_cyan);
    Logger::singleton().register_log_type("std",color::white);

    log("irc",'<',"Hello world!!",2);
    log("dp",'>',"Hello world!!",2);

    return bot.run();
}
