/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015-2016 Mattia Basaglia
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
#include <boost/asio.hpp>

#ifndef POSIX_STDIN_HPP
#define POSIX_STDIN_HPP

#ifndef BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR
#       error "The posix module requires a POSIX-conformant system"
#endif

#include <cstdio>
#include <memory>
#include <thread>

#include <fcntl.h>
#include <unistd.h>

#include "melanobot/melanobot.hpp"
#include "network/connection.hpp"

namespace posix {
/**
 * \brief Acts as a network connection to handle standard input (or any file)
 */
class StdinConnection : public network::SingleUnitConnection
{
public:
    static std::unique_ptr<StdinConnection> create(
        const Settings& settings, const std::string& name)
    {
        return melanolib::New<StdinConnection>(settings, name);
    }

    StdinConnection(const Settings& settings, const std::string& name)
        : SingleUnitConnection(name), input{io_service}
    {
        std::string filename = settings.get("file", "");
        int file = open_file(filename);
        if ( file < 0 )
            throw melanobot::ConfigurationError("Cannot open "+filename);
        input = boost::asio::posix::stream_descriptor(io_service, file);
        formatter_ = string::Formatter::formatter(settings.get("string_format", std::string("utf8")));
    }

    ~StdinConnection()
    {
        stop();
    }


    void stop() override
    {
        io_service.stop();
        if ( input.native_handle() > 0 )
            close(input.native_handle());
        if ( thread.joinable() )
            thread.join();
    }

    void start() override
    {
        if ( io_service.stopped() )
            io_service.reset();
        if ( !thread.joinable() )
            thread = std::move(std::thread{[this]{run();}});
    }

    string::Formatter* formatter() const override
    {
        return formatter_;
    }


    std::string protocol() const override { return "stdin"; }
    network::Server server() const override { return {"stdin", 0}; }
    std::string description() const override { return  "stdin"; }

    void say ( const network::OutputMessage& msg ) override
    {
        Log("std", '>', 1) << msg.message;
    }

    LockedProperties properties() override
    {
        static PropertyTree props;
        return {{}, &props};
    }

    // dummy overrides:
    Status status() const override { return CONNECTED; }
    void connect() override {}
    void disconnect(const string::FormattedString&) override {}
    void reconnect(const string::FormattedString&) override {}

    void command (network::Command) override {}
    // maybe could be given some actual functionality:
    string::FormattedProperties pretty_properties() const override { return string::FormattedProperties{}; }

private:
    string::Formatter*                    formatter_;
    boost::asio::streambuf                buffer_read;
    boost::asio::io_service               io_service;
    boost::asio::posix::stream_descriptor input;
    std::thread                           thread;

    void run()
    {
        schedule_read();
        boost::system::error_code err;
        io_service.run(err);
        if ( err )
        {
            ErrorLog("std", "Network Error") << err.message();
            melanobot::Melanobot::instance().stop(); /// \todo move this in error handler
        }
    }

    void schedule_read()
    {
        boost::asio::async_read_until(input, buffer_read, '\n',
            [this](const boost::system::error_code& error, std::size_t)
            { return on_read_line(error); });
    }

    void on_read_line(const boost::system::error_code &error)
    {
        if (error)
        {
            if ( error != boost::asio::error::eof )
                ErrorLog("std", "Network Error") << error.message();
            return;
        }

        std::istream buffer_stream(&buffer_read);
        network::Message msg;
        std::getline(buffer_stream, msg.raw);
        Log("std", '<', 1) << formatter_->decode(msg.raw);
        std::istringstream socket_stream(msg.raw);

        msg.chat(msg.raw);
        msg.from = name();
        msg.direct = true;

        msg.send(this);

        schedule_read();
    }

    /**
     * \brief Opens a file descriptor
     */
    static int open_file(const std::string& name)
    {
        if ( name.empty() || name == "stdin" || name == "/dev/stdin" )
            return STDIN_FILENO;
        // O_RDWR so that named pipes don't have to block until a writer arrives
        return open(name.c_str(), O_RDWR);
    }
};
} // namespace posix

#endif // POSIX_STDIN_HPP
