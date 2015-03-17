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
#include "fun-handlers.hpp"
#include "melanomodule.hpp"

/**
 * \brief Registers the fun handlers
 */
Melanomodule melanomodule_fun()
{
    Melanomodule module{"fun","Fun handlers"};
    module.register_handler<fun::AnswerQuestions>("AnswerQuestions");
    module.register_handler<fun::RenderPony>("RenderPony");
    module.register_handler<fun::ChuckNorris>("ChuckNorris");
    module.register_handler<fun::ReverseText>("ReverseText");
    module.register_handler<fun::Morse>("Morse");
    return module;
}
