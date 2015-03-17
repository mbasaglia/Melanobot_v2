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
    REGISTER_HANDLER(fun::AnswerQuestions,AnswerQuestions);
    REGISTER_HANDLER(fun::RenderPony,RenderPony);
    REGISTER_HANDLER(fun::ChuckNorris,ChuckNorris);
    REGISTER_HANDLER(fun::ReverseText,ReverseText);
    REGISTER_HANDLER(fun::Morse,Morse);
    return module;
}
