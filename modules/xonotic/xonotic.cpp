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

#include "xonotic-formatter.hpp"
#include "module/melanomodule.hpp"
#include "xonotic-connection.hpp"
#include "handler/rcon.hpp"
#include "handler/status.hpp"
#include "handler/log.hpp"

#include "unvanquished-connection.hpp"
#include "handler/unvanquished.hpp"

/**
 * \brief Initializes the Xonotic module
 */
MELANOMODULE_ENTRY_POINT module::Melanomodule melanomodule_xonotic_metadata()
{
    return {"xonotic","Xonotic integration"};
}

MELANOMODULE_ENTRY_POINT void melanomodule_xonotic_initialize(const Settings&)
{
    module::register_formatter<xonotic::XonoticFormatter>();
    module::register_log_type("xon",color::dark_cyan);
    module::register_connection<xonotic::XonoticConnection>("xonotic");

    module::register_handler<xonotic::RconCommand>("RconCommand");
    module::register_handler<xonotic::XonoticVCall>("XonoticVCall");
    module::register_handler<xonotic::XonoticVStop>("XonoticVStop");

    module::register_handler<xonotic::ConnectionEvents>("ConnectionEvents");
    module::register_handler<xonotic::XonoticJoinPart>("XonoticJoinPart");

    module::register_handler<xonotic::XonoticMatchStart>("XonoticMatchStart");
    module::register_handler<xonotic::ShowVoteCall>("ShowVoteCall");
    module::register_handler<xonotic::ShowVoteLogin>("ShowVoteLogin");
    module::register_handler<xonotic::ShowVoteDo>("ShowVoteDo");
    module::register_handler<xonotic::ShowVoteResult>("ShowVoteResult");
    module::register_handler<xonotic::ShowVoteStop>("ShowVoteStop");
    module::register_handler<xonotic::ShowVotes>("ShowVotes");
    module::register_handler<xonotic::XonoticMatchScore>("XonoticMatchScore");
    module::register_handler<xonotic::XonoticHostError>("XonoticHostError");
    module::register_handler<xonotic::XonoticUpdateBans>("XonoticUpdateBans");

    module::register_handler<xonotic::ListPlayers>("ListPlayers");
    module::register_handler<xonotic::XonoticStatus>("XonoticStatus");
    module::register_handler<xonotic::XonoticMaps>("XonoticMaps");
    module::register_handler<xonotic::XonoticBan>("XonoticBan");
    module::register_handler<xonotic::XonoticKick>("XonoticKick");


    module::register_connection<unvanquished::UnvanquishedConnection>("unvanquished");
    module::register_log_type("unv",color::dark_cyan);
    module::register_handler<unvanquished::UnvanquishedStatus>("UnvanquishedStatus");
    module::register_handler<unvanquished::UnvanquishedMaps>("UnvanquishedMaps");
}

namespace xonotic {
static const std::unordered_map<std::string, std::string> gametype_names = {
    {"as",  "assault"},
    {"ca",  "clan arena"},
    {"cq",  "conquest"},
    {"ctf", "capture the flag"},
    {"cts", "race cts"},
    {"dom", "domination"},
    {"dm",  "deathmatch"},
    {"ft",  "freezetag"},
    {"inf", "infection"},
    {"inv", "invasion"},
    {"jb",  "jailbreak"},
    {"ka",  "keepaway"},
    {"kh",  "key hunt"},
    {"lms", "last man standing"},
    {"nb",  "nexball"},
    {"ons", "onslaught"},
    {"rc",  "race"},
    {"tdm", "team deathmatch"},
};
std::string gametype_name(const std::string& short_name)
{
    auto it = gametype_names.find(short_name);
    return it == gametype_names.end() ? short_name : it->second;
}
} // namespace xonotic
