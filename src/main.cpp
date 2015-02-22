#include "melanobot.hpp"
#include "logger.hpp"
#include "network/async_service.hpp"

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

    network::HttpRequest ht;
    auto http_stuff = [](const network::Response& r)
    {
        std::cout << r.error_message << '\n';
        std::cout << r.contents << '\n';
    };
    ht.async_get("http://example.com",http_stuff);

    log("dp",'>',"Hello world!!",2);

    return bot.run();
}
