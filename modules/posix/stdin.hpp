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
#include <boost/asio.hpp>

#ifndef BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR
#       error "The posix module requires a POSIX-conformant system"
#endif

#include <cstdio>
#include <memory>
#include <thread>

#include "melanobot.hpp"
#include "network/connection.hpp"

/**
 * \brief Acts as a network connection to handle standard input
 */
class StdinConnection : public network::Connection
{
public:
    static std::unique_ptr<StdinConnection> create(
        Melanobot* bot, const Settings& settings, const std::string& name)
    {
        return std::make_unique<StdinConnection>(bot,settings, name);
    }

    StdinConnection(Melanobot* bot, const Settings& settings, const std::string& name)
        : Connection(name), bot(bot)
    {
        formatter_ = string::Formatter::formatter(settings.get("string_format",std::string("utf8")));
    }

    ~StdinConnection()
    {
        stop();
    }


    void stop() override
    {
        io_service.stop();
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
    std::string name() const override { return "stdin"; }
    network::Server server() const override { return {"stdin",0}; }
    std::string description() const override { return  "stdin"; }

    void say ( const network::OutputMessage& msg ) override
    {
        Log("std",'>',1) << msg.message;
    }

    // dummy overrides:
    Status status() const override { return CONNECTED; }
    void connect() override {}
    void disconnect(const std::string&) override {}
    void reconnect(const std::string&) override {}
    bool channel_mask(const std::vector<std::string>&,  const std::string&) const override { return true; }
    bool user_auth(const std::string&, const std::string&) const override { return true; }
    void update_user(const std::string&, const Properties& ) override {}
    user::User get_user(const std::string&) const override { return {}; }
    std::vector<user::User> get_users( const std::string& ) const override  { return {}; }
    bool add_to_group(const std::string&, const std::string&) override { return false; }
    bool remove_from_group(const std::string&, const std::string& ) override { return false; }
    std::vector<user::User> users_in_group(const std::string&) const override { return {}; }
    // maybe could be given some actual functionality:
    std::string get_property(const std::string&) const override { return {}; }
    bool set_property(const std::string& , const std::string& ) override { return false; }
    void command (network::Command) override {}
    user::UserCounter count_users(const std::string& channel = {}) const override { return {}; }
    Properties message_properties() const override { return Properties{}; }

private:
    Melanobot*                            bot;
    string::Formatter*                    formatter_;
    boost::asio::streambuf                buffer_read;
    boost::asio::io_service               io_service;
    boost::asio::posix::stream_descriptor input {io_service, STDIN_FILENO};
    std::thread                           thread;

    void run()
    {
        schedule_read();
        boost::system::error_code err;
        io_service.run(err);
        if ( err )
        {
            ErrorLog("std","Network Error") << err.message();
            bot->stop();
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
                ErrorLog("std","Network Error") << error.message();
            return;
        }

        std::istream buffer_stream(&buffer_read);
        network::Message msg;
        std::getline(buffer_stream,msg.raw);
        Log("std",'<',1) << formatter_->decode(msg.raw);
        std::istringstream socket_stream(msg.raw);

        msg.message = msg.raw;
        msg.from = name();
        msg.source = this;
        msg.type = network::Message::CHAT;

        bot->message(msg);

        schedule_read();
    }

};
