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
        { ' ', "" },
        { '!', "-.-.--" },
        { '"', ".-..-." },
        { '#', ".....-......." }, // compressed .... .- ... .... (hash)
        { '$', "...-..-" },
        { '%', ".--.-.-." }, // compressed .--. -.-. (pc)
        { '&', ".-..." },
        { '\'', ".----." },
        { '(', "-.--." },
        { ')', "-.--.-" },
        { '*', "...-.-.-."}, // compressed ... - .- .-. (star)
        { '+', ".-.-." },
        { ',', "--..--" },
        { '-', "-....-" },
        { '.', ".-.-.-" },
        { '/', "-..-." },
        { ':', "---..." },
        { ';', "-.-.-." },
        { '<', ".-........." },
        { '=', "-...-" },
        { '>', "--..-...--..-." },
        { '?', "..--.." },
        { '@', ".--.-." },
        { '[', "-.--." },  // actually (
        { '\\', "-..-." }, // actually /
        { ']', "-.--.-" }, // actually )
        { '^', "-.-..-.-..-"}, // compressed -.-. .- .-. . - (caret)
        { '_', "..--.-" },
        { '`', ".----." }, // actually '
        { '{', "-.--." },  // actually (
        { '|', "-..-." },  // actually /
        { '}', "-.--.-" }, // actually )
        { '~', "-...-..-..."} // compressed - .. .-.. -.. . (tilde)
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

/**
 * \brief Draws a pretty My Little Pony ASCII art
 * \note Very useful to see how the bot handles flooding
 * \note Even more useful to see pretty ponies ;-)
 * \see https://github.com/mbasaglia/ASCII-Pony
 */
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

            // For all of the files in the given directory
            for( directory_iterator dir_iter(pony_path) ;
                dir_iter != directory_iterator() ; ++dir_iter)
            {
                // which are regular files
                if ( is_regular_file(dir_iter->status()) )
                {
                    if ( pony_search.empty() )
                    {
                        // no search query? any pony will do
                        files.push_back(dir_iter->path().string());
                    }
                    else
                    {
                        // get how similar the query is to the file name
                        int local_score = string::similarity(
                            dir_iter->path().filename().string(), pony_search );

                        if ( local_score > score )
                        {
                            // found a better match, use that
                            score = local_score;
                            files = { dir_iter->path().string() };
                        }
                        else if ( local_score == score )
                        {
                            // found an equivalent match, add it to the list
                            files.push_back( dir_iter->path().string() );
                        }
                    }
                }
            }

            // found at least one pony
            if ( !files.empty() )
            {
                // open a random one
                std::fstream file(files[math::random(files.size()-1)]);
                // I guess if not is_open and we have other possible ponies
                // we could select a different one (random_shuffle and iterate)
                // but whatever
                if ( file.is_open() )
                {
                    // print the file line by line
                    std::string line;
                    while(true)
                    {
                        std::getline(file,line);
                        if ( !file ) break;
                        reply_to(msg,line);
                    }
                    // done, return
                    return true;
                }
            }
        }

        // didn't find any suitable file
        reply_to(msg,"Didn't find anypony D:");
        return true;
    }

private:
    std::string pony_path;
};
REGISTER_HANDLER(RenderPony,RenderPony);

/**
 * \brief Answers direct questions
 */
class AnswerQuestions : public Handler
{
public:
    AnswerQuestions(const Settings& settings, Melanobot* bot)
        : Handler(settings, bot)
    {
        direct    = settings.get("direct",direct);
    }

    bool can_handle(const network::Message& msg) override
    {
        return msg.direct && !msg.message.empty() && msg.message.back() == '?';
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        static std::regex regex_question(
            "^(?:(where(?: (?:is|are))?|(?:when(?: (?:will|did))?)|(?:who(?:se|m)?)|what|how)\\b)?\\s*(.*)\\?",
            std::regex::ECMAScript|std::regex::optimize|std::regex::icase
        );
        std::smatch match;

        std::regex_match(msg.message,match,regex_question);
        std::string question = string::strtolower(match[1]);
            std::vector<std::vector<std::string>*> answers;

        if ( string::starts_with(question,"when") )
        {
            answers.push_back(&category_when);
            if ( string::ends_with(question,"did") )
                answers.push_back(&category_when_did);
            else if ( string::ends_with(question,"will") )
                answers.push_back(&category_when_will);
        }
        else if ( string::starts_with(question,"where") )
        {
            static std::string url = "http://maps.googleapis.com/maps/api/geocode/json?sensor=false";
            network::Response response =  network::service("web")->query(
                network::http::get(url,{{"address",match[2]}}) );

            JsonParser parser;
            parser.throws(false);
            Settings ptree = parser.parse_string(response.contents,response.origin);

            std::string address = ptree.get("results.0.formatted_address","I don't know");
            network::http::Parameters params {{"q",match[2]}};
            auto location = ptree.get_child_optional("results.0.geometry.location");
            if ( location )
                params["ll"] = location->get("lat","")+","+location->get("lng","");

            reply_to(msg,address+": https://maps.google.com/?"+
                network::http::build_query(params));

            return true;
        }
        else if ( string::starts_with(question,"who") && !msg.channels.empty() && msg.source )
        {
            auto users = msg.source->get_users(msg.channels[0]);
            auto user = users[math::random(users.size()-1)];
            reply_to(msg,user.name == msg.source->name() ? "Not me!" : user.name);
            return true;
        }
        else if ( question == "what" || question == "how" )
        {
            answers.push_back(&category_dunno);
        }
        else
        {
            answers.push_back(&category_yesno);
            answers.push_back(&category_dunno);
        }

        random_answer(msg,answers);

        return true;
    }

private:
    bool direct = true;

    /// Answers corresponding to yes or no
    static std::vector<std::string> category_yesno;
    /// Generic and unsatisfying answers
    static std::vector<std::string> category_dunno;

    /// Answers to some time in the past
    static std::vector<std::string> category_when_did;
    /// Generic answers to when
    static std::vector<std::string> category_when;
    /// Answers to some time in the future
    static std::vector<std::string> category_when_will;

    /**
     * \brief Selects a random answer from a set of categories
     */
    void random_answer(network::Message& msg,
                       const std::vector<std::vector<std::string>*>& categories) const
    {
        unsigned n = 0;
        for ( const auto& cat : categories )
            n += cat->size();

        n = math::random(n-1);

        for ( const auto& cat : categories )
        {
            if ( n >= cat->size() )
            {
                n -= cat->size();
            }
            else
            {
                reply_to(msg,(*cat)[n]);
                break;
            }
        }
    }
};
REGISTER_HANDLER(AnswerQuestions,AnswerQuestions);

std::vector<std::string> AnswerQuestions::category_yesno = {
    "Signs point to yes",
    "Yes",
    "Without a doubt",
    "As I see it, yes",
    "It is decidedly so",
    "Of course",
    "Most likely",
    "Sure!",
    "Eeyup!",
    "Maybe",

    "Maybe not",
    "My reply is no",
    "My sources say no",
    "I doubt it",
    "Very doubtful",
    "Don't count on it",
    "I don't think so",
    "Nope",
    "No way!",
    "No",
};

std::vector<std::string> AnswerQuestions::category_dunno = {
    "Better not tell you now",
    "Ask again later",
    "I don't know",
    "I know the answer but won't tell you",
    "Please don't ask stupid questions",
};

std::vector<std::string> AnswerQuestions::category_when_did {
    "42 years ago",
    "Yesterday",
    "Some time in the past",
};
std::vector<std::string> AnswerQuestions::category_when {
    "Right now",
    "Never",
    "When you stop asking stupid questions",
    "The same day you'll decide to shut up",
};
std::vector<std::string> AnswerQuestions::category_when_will {
    "Some time in the future",
    "Tomorrow",
    "42 years from now",
};

} // namespace handler
