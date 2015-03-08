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

#include <boost/filesystem.hpp>

#include "web-api.hpp"
#include "math.hpp"

namespace handler {

/**
 * \brief Handler translating between morse and latin
 */
class Morse : public SimpleAction
{
public:
    Morse(const Settings& settings, Melanobot* bot)
        : SimpleAction("morse",settings,bot)
    {
        synopsis += " text|morse";
        help = "Converts between ASCII and Morse code";
    }

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
 * \brief Turns ASCII characters upside-down
 */
class ReverseText : public SimpleAction
{
public:
    ReverseText(const Settings& settings, Melanobot* bot)
        : SimpleAction("reverse",settings,bot)
    {
        synopsis += " text";
        help = "Turns ASCII upside-down";
    }

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

/**
 * \brief Searches for a Chuck Norris joke
 */
class ChuckNorris : public SimpleJson
{
public:
    ChuckNorris(const Settings& settings, Melanobot* bot)
        : SimpleJson("norris",settings,bot)
    {
        synopsis += " [name]";
        help = "Shows a Chuck Norris joke from http://icndb.com";
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        static std::regex regex_name("(?:([^ ]+) )?\\s*(.*)",
                std::regex_constants::syntax_option_type::optimize |
                std::regex_constants::syntax_option_type::ECMAScript);

        network::http::Parameters params;
        std::smatch match;

        if ( std::regex_match(msg.message, match, regex_name) )
        {
            params["firstName"] = match[1];
            params["lastName"]  = match[2];
        }

        request_json(msg,network::http::get(api_url,params));
        return true;
    }

    void json_success(const network::Message& msg, const Settings& parsed) override
    {
        /// \todo convert html entities
        reply_to(msg,parsed.get("value.joke",""));
    }

private:
    std::string api_url = "http://api.icndb.com/jokes/random";
};
REGISTER_HANDLER(ChuckNorris,ChuckNorris);

class RenderPony : public SimpleAction
{
public:
    RenderPony(const Settings& settings, Melanobot* bot)
        : SimpleAction("render_pony",settings,bot)
    {
        synopsis += " pony";
        help = "Draws a pretty pony /)^3^(\\";
        pony_path = settings.get("path",pony_path);
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        using namespace boost::filesystem;

        if ( exists(pony_path) && is_directory(pony_path))
        {
            std::string pony_search = msg.message;
            int score = -1;
            std::vector<std::string> files;

            for( directory_iterator dir_iter(pony_path) ;
                dir_iter != directory_iterator() ; ++dir_iter)
            {
                if ( is_regular_file(dir_iter->status()) )
                {
                    if ( pony_search.empty() )
                    {
                        files.push_back(dir_iter->path().string());
                    }
                    else
                    {
                        int local_score = string::similarity(
                            dir_iter->path().filename().string(), pony_search );
                        if ( local_score > score )
                        {
                            score = local_score;
                            files = { dir_iter->path().string() };
                        }
                        else if ( local_score > score )
                        {
                            files.push_back( dir_iter->path().string() );
                        }
                    }
                }
            }

            if ( !files.empty() )
            {
                std::fstream file(files[math::random(files.size()-1)]);
                if ( file.is_open() )
                {
                    std::string line;
                    while(true)
                    {
                        std::getline(file,line);
                        if ( !file ) break;
                        reply_to(msg,line);
                    }
                    return true;
                }
            }
        }

        reply_to(msg,"Didn't find anypony D:");
        return true;
    }

private:
    std::string pony_path;
};
REGISTER_HANDLER(RenderPony,RenderPony);

} // namespace handler
