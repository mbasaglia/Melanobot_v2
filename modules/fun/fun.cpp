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
#include "fun-handlers.hpp"
#include "module/melanomodule.hpp"
#include "string/replacements.hpp"
#include "rainbow.hpp"
#include "markov.hpp"

/**
 * \brief Registers the fun handlers
 */
MELANOMODULE_ENTRY_POINT module::Melanomodule melanomodule_fun_metadata()
{
    return {"fun", "Fun handlers", 0, {{"web"}, {"core"}}};
}

MELANOMODULE_ENTRY_POINT void melanomodule_fun_initialize(const Settings&)
{
    module::register_handler<fun::AnswerQuestions>("AnswerQuestions");
    module::register_handler<fun::ChuckNorris>("ChuckNorris");
    module::register_handler<fun::ReverseText>("ReverseText");
    module::register_handler<fun::Morse>("Morse");
    module::register_handler<fun::RainbowBridgeChat>("RainbowBridgeChat");
    module::register_handler<fun::Slap>("Slap");
    module::register_handler<fun::Discord>("Discord");
    module::register_handler<fun::Stardate>("Stardate");
    module::register_handler<fun::Insult>("Insult");

    module::register_handler<fun::RenderPony>("RenderPony");
    module::register_handler<fun::PonyCountDown>("PonyCountDown");
    module::register_handler<fun::PonyFace>("PonyFace");

    module::register_handler<fun::MarkovTextGenerator>("MarkovTextGenerator");
    module::register_handler<fun::MarkovListener>("MarkovListener");
    module::register_handler<fun::MarkovSave>("MarkovSave");
    module::register_handler<fun::MarkovStatus>("MarkovStatus");
    module::register_handler<fun::MultiMarkovTextGenerator>("MultiMarkovTextGenerator");

    string::FilterRegistry::instance().register_filter("rainbow",
        [](const std::vector<string::FormattedString>& args) -> string::FormattedString
        {
            fun::FormatterRainbow rainbow;
            std::string str;
            for ( const auto& arg : args )
                str += arg.encode(rainbow);
            return rainbow.decode(str);
        }
    );
}
