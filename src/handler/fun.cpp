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

        std::string result;
        if ( std::regex_match(msg.message,regex_morse) )
        {
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
            result = string::implode(" ",morse_string);
        }

        if ( !result.empty() )
            reply_to(msg,result);

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

/**
 * \brief Turn ASCII characters upside-down
 */
class ReverseText : public SimpleAction
{
public:
    ReverseText(const Settings& settings, Melanobot* bot)
        : SimpleAction("reverse",settings,bot)
    {}

protected:
    bool on_handle(network::Message& msg) override
    {
        std::string ascii = msg.source->formatter()->decode(msg.message).encode("ascii");
        if ( ascii.empty() )
            return true;

        std::string result;
        for ( char c : ascii )
        {
            auto it = reverse_ascii.find(c);
            if ( it !=  reverse_ascii.end() )
                result = it->second + result;
            else
                result = c + result;
        }

        reply_to(msg,result);

        return true;
    }

private:
    std::unordered_map<char,std::string> reverse_ascii {
        { ' ', " " },
        { '!', "¡" },
        { '"', "„" },
        { '#', "#" },
        { '$', "$" },
        { '%', "%" }, // :-(
        { '&', "⅋" },
        { '\'', "ˌ" },
        { '(', ")" },
        { ')', "(" },
        { '*', "*" },
        { '+', "+" },
        { ',', "ʻ" },
        { '-', "-" },
        { '.', "˙" },
        { '/', "\\" },
        { '0', "0" },
        { '1', "⇂" }, // Ɩ
        { '2', "ح" },//ᄅ
        { '3', "Ꜫ" },
        { '4', "ᔭ" },
        { '5', "2" }, // meh
        { '6', "9" },
        { '7', "ㄥ" },
        { '8', "8" },
        { '9', "6" },
        { ':', ":" },
        { ';', "؛" },
        { '<', ">" },
        { '=', "=" },
        { '>', "<" },
        { '?', "¿" },
        { '@', "@" }, // :-(
        { 'A', "Ɐ" },
        { 'B', "ᗺ" },
        { 'C', "Ɔ" },
        { 'D', "ᗡ" },
        { 'E', "Ǝ" },
        { 'F', "Ⅎ" },
        { 'G', "⅁" },
        { 'H', "H" },
        { 'I', "I" },
        { 'J', "ſ" },
        { 'K', "ʞ" }, // :-/
        { 'L', "Ꞁ" },
        { 'M', "ꟽ" },
        { 'N', "N" },
        { 'O', "O" },
        { 'P', "d" },// meh
        { 'Q', "Ò" },
        { 'R', "ᴚ" },
        { 'S', "S" },
        { 'T', "⊥" },
        { 'U', "⋂" },
        { 'V', "Λ" },
        { 'W', "M" }, // meh
        { 'X', "X" },
        { 'Y', "⅄" },
        { 'Z', "Z" },
        { '[', "]" },
        { '\\', "/" },
        { ']', "[" },
        { '^', "˯" },
        { '_', "¯" },
        { '`', "ˎ" },
        { 'a', "ɐ" },
        { 'b', "q" },
        { 'c', "ɔ" },
        { 'd', "p" },
        { 'e', "ə" },
        { 'f', "ɟ" },
        { 'g', "δ" },
        { 'h', "ɥ" },
        { 'i', "ᴉ" },
        { 'j', "ɾ" },
        { 'k', "ʞ" },
        { 'l', "ꞁ" },
        { 'm', "ɯ" },
        { 'n', "u" },
        { 'o', "o" },
        { 'p', "d" },
        { 'q', "b" },
        { 'r', "ɹ" },
        { 's', "s" },
        { 't', "ʇ" },
        { 'u', "n" },
        { 'v', "ʌ" },
        { 'w', "ʍ" },
        { 'x', "x" },
        { 'y', "ʎ" },
        { 'z', "z" },
        { '{', "}" },
        { '|', "|" },
        { '}', "{" },
        { '~', "∽" },
    };
};
REGISTER_HANDLER(ReverseText,ReverseText);
} // namespace handler
