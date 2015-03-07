/**
 * \file
 * \brief This file contains definitions for handlers which are pretty useless
 *
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

#include <unordered_map>

#include "web-api.hpp"

namespace handler {

/**
 * \brief Handler translating between morse and latin
 */
class Morse : public SimpleAction
{
public:
    Morse(const Settings& settings, Melanobot* bot)
        : SimpleAction("morse",settings,bot) {}

protected:
    bool on_handle(network::Message& msg) override
    {
        static std::regex regex_morse ("^[-. ]+$",
                std::regex_constants::syntax_option_type::optimize |
                std::regex_constants::syntax_option_type::ECMAScript);

        if ( std::regex_match(msg.message,regex_morse) )
        {
            std::string result;
            auto morse_array = string::regex_split(msg.message," ",false);
            for ( const auto& mc : morse_array )
            {
                if ( mc.empty() )
                {
                    result += ' ';
                }
                else
                {
                    for ( const auto& p : morse )
                        if ( p.second == mc )
                        {
                            result += p.first;
                            break;
                        }
                }

            }
            reply_to(msg,result);
        }
        else
        {
            std::vector<std::string> morse_string;
            std::string param_string = string::strtolower(msg.message);
            for ( unsigned i = 0; i < param_string.size(); i++ )
            {
                auto it = morse.find(param_string[i]);
                if ( it != morse.end() )
                    morse_string.push_back(it->second);
            }
            reply_to(msg,string::implode(" ",morse_string));
        }
        return true;
    }

private:
    std::unordered_map<char,std::string> morse {
        { 'a', ".-" },
        { 'b', "-..." },
        { 'c', "-.-." },
        { 'd', "-.." },
        { 'e', "." },
        { 'f', "..-." },
        { 'g', "--." },
        { 'h', "...." },
        { 'i', ".." },
        { 'j', ".---" },
        { 'k', "-.-" },
        { 'l', ".-.." },
        { 'm', "--" },
        { 'n', "-." },
        { 'o', "---" },
        { 'p', ".--." },
        { 'q', "--.-" },
        { 'r', ".-." },
        { 's', "..." },
        { 't', "-" },
        { 'u', "..-" },
        { 'v', "...-" },
        { 'w', ".--" },
        { 'x', "-..-" },
        { 'y', "-.--" },
        { 'z', "--.." },
        { '0', "-----" },
        { '1', ".----" },
        { '2', "..---" },
        { '3', "...--" },
        { '4', "....-" },
        { '5', "....." },
        { '6', "-...." },
        { '7', "--..." },
        { '8', "---.." },
        { '9', "----." },
        { '.', ".-.-.-" },
        { ',', "--..--" },
        { '?', "..--.." },
        { '\'', ".----." },
        { '!', "-.-.--" },
        { '/', "-..-." },
        { '(', "-.--." },
        { ')', "-.--.-" },
        { '&', ".-..." },
        { ':', "---..." },
        { ';', "-.-.-." },
        { '=', "-...-" },
        { '+', ".-.-." },
        { '-', "-....-" },
        { '_', "..--.-" },
        { '"', ".-..-." },
        { '$', "...-..-" },
        { '@', ".--.-." },
        { ' ', "" },
        { '<', ".-........." },
        { '>', "--..-...--..-." },
    };
};
REGISTER_HANDLER(Morse,Morse);

} // namespace handler
