/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015-2017 Mattia Basaglia
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
#ifndef DARKPLACES_HPP
#define DARKPLACES_HPP

#include "engine.hpp"

namespace xonotic {

class Darkplaces : public Engine
{
public:
    enum class Secure {
        NO        = 0,  ///< Plaintext rcon
        TIME      = 1,  ///< Time-based digest
        CHALLENGE = 2   ///< Challeng-based security
    };

    explicit Darkplaces(network::Server server,
        std::string rcon_password, Secure rcon_secure = Secure::NO);

    void rcon_command(std::string command) override;

    Secure rcon_secure() const
    {
        return rcon_secure_;
    }

private:
    std::pair<melanolib::cstring_view, melanolib::cstring_view>
        split_command(melanolib::cstring_view message) const override;

    melanolib::cstring_view filter_challenge(melanolib::cstring_view message) const override;

    void challenged_command(const std::string& challenge, const std::string& command) override;

    bool is_log(melanolib::cstring_view command) const override;


    std::atomic<Secure> rcon_secure_;
};

} // namespace xonotic
#endif // DARKPLACES_HPP
