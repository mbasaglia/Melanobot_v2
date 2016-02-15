/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2016 Mattia Basaglia
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
#ifndef MELANOBOT_MODULE_XONOTIC_DAEMON_HPP
#define MELANOBOT_MODULE_XONOTIC_DAEMON_HPP

#include "engine.hpp"

namespace unvanquished {

class Daemon : public xonotic::Engine
{
public:
    enum class Secure
    {
        Unencrypted,        // Allow unencrypted rcon
        EncryptedPlain,     // Require encryption
        EncryptedChallenge, // Require encryption and challege check
        Invalid             // Invalid protocol
    };

    explicit Daemon(network::Server server, std::string rcon_password);

    Secure rcon_secure() const { return rcon_secure_; }

    void set_rcon_secure(Secure secure) { rcon_secure_ = secure; }

protected:
    void rcon_command(std::string command) override;

private:
    bool is_log(melanolib::cstring_view command) const override;

    void challenged_command(const std::string& challenge, const std::string& command) override;

    bool is_challenge_response(melanolib::cstring_view command) const override;

    /**
     * \brief Requests a rconinfo
     */
    void on_connect() override;

    /**
     * \brief Manages rconInfoResponse
     */
    void on_receive(const std::string& cmd, const std::string& msg) override;

    std::string challenge_request() const override;

private:
    std::atomic<Secure> rcon_secure_;
};

} // namespace unvanquished
#endif // MELANOBOT_MODULE_XONOTIC_DAEMON_HPP
