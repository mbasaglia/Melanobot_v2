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
#include "rainbow.hpp"
#include <set>

namespace fun {


bool Morse::on_handle(network::Message& msg)
{
    static std::regex regex_morse ("^[-. ]+$",
            std::regex_constants::optimize |
            std::regex_constants::ECMAScript);

    std::string result;
    if ( std::regex_match(msg.message, regex_morse) )
    {
        auto morse_array = melanolib::string::regex_split(msg.message, " ", false);
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
        std::string param_string = melanolib::string::strtolower(msg.message);
        for ( unsigned i = 0; i < param_string.size(); i++ )
        {
            auto it = morse.find(param_string[i]);
            if ( it != morse.end() )
                morse_string.push_back(it->second);
        }
        result = melanolib::string::implode(" ", morse_string);
    }

    if ( !result.empty() )
        reply_to(msg, result);

    return true;
}

std::unordered_map<char, std::string> Morse::morse {
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

bool ReverseText::on_handle(network::Message& msg)
{
    std::string ascii = msg.source->encode_to(msg.message, string::FormatterAscii());
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

    reply_to(msg, result);

    return true;
}

std::unordered_map<char, std::string> ReverseText::reverse_ascii {
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
    { '2', "ح" }, //ᄅ
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
    { 'P', "d" }, // meh
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

bool ChuckNorris::on_handle(network::Message& msg)
{
    static std::regex regex_name("(?:([^ ]+)\\s+)?(.*)",
            std::regex_constants::optimize |
            std::regex_constants::ECMAScript);

    web::DataMap params;
    std::smatch match;

    if ( std::regex_match(msg.message, match, regex_name) && match.length() > 0 )
    {
        params["firstName"] = match[1];
        params["lastName"]  = match[2];
    }

    request_json(msg, web::Request("GET", web::Uri(api_url, params)));
    return true;
}

bool RenderPony::on_handle(network::Message& msg)
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
                    int local_score = melanolib::string::similarity(
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
            std::fstream file(files[melanolib::math::random(files.size()-1)]);
            // I guess if not is_open and we have other possible ponies
            // we could select a different one (random_shuffle and iterate)
            // but whatever
            if ( file.is_open() )
            {
                // print the file line by line
                std::string line;
                while(true)
                {
                    std::getline(file, line);
                    if ( !file ) break;
                    reply_to(msg, line);
                }
                // done, return
                return true;
            }
        }
    }

    // didn't find any suitable file
    reply_to(msg, "Didn't find anypony D:");
    return true;
}

bool AnswerQuestions::on_handle(network::Message& msg)
{
    static std::regex regex_question(
        "^(?:((?:when(?: (?:will|did))?)|(?:who(?:se|m)?)|what|how)\\b)?\\s*(.*)\\?",
        std::regex::ECMAScript|std::regex::optimize|std::regex::icase
    );
    std::smatch match;

    std::regex_match(msg.message, match, regex_question);
    std::string question = melanolib::string::strtolower(match[1]);
        std::vector<std::vector<std::string>*> answers;

    if ( melanolib::string::starts_with(question, "when") )
    {
        answers.push_back(&category_when);
        if ( melanolib::string::ends_with(question, "did") )
            answers.push_back(&category_when_did);
        else if ( melanolib::string::ends_with(question, "will") )
            answers.push_back(&category_when_will);
    }
    else if ( melanolib::string::starts_with(question, "who") && !msg.channels.empty() && msg.source )
    {
        auto users = msg.source->get_users(msg.channels[0]);
        if ( !users.empty() )
        {
            const auto& name = users[melanolib::math::random(users.size()-1)].name;
            reply_to(msg, name == msg.source->name() ? "Not me!" : name);
            return true;
        }
        else
        {
            answers.push_back(&category_dunno);
        }
    }
    else if ( question == "what" || question == "how" || question == "why" || question == "where" )
    {
        answers.push_back(&category_dunno);
    }
    else
    {
        answers.push_back(&category_yesno);
        answers.push_back(&category_dunno);
    }

    random_answer(msg, answers);

    return true;
}

void AnswerQuestions::random_answer(network::Message& msg,
                    const std::vector<std::vector<std::string>*>& categories) const
{
    unsigned n = 0;
    for ( const auto& cat : categories )
        n += cat->size();

    n = melanolib::math::random(n-1);

    for ( const auto& cat : categories )
    {
        if ( n >= cat->size() )
        {
            n -= cat->size();
        }
        else
        {
            reply_to(msg, (*cat)[n]);
            break;
        }
    }
}

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

bool RainbowBridgeChat::on_handle(network::Message& msg)
{
    FormatterRainbow formatter(melanolib::math::random_real(), 0.6, 1);

    auto from = formatter.decode(
        msg.source->encode_to(msg.from.name, formatter));

    auto message = formatter.decode(
        msg.source->encode_to(msg.message, formatter));

    reply_to(msg, network::OutputMessage(
        std::move(message),
        msg.type == network::Message::ACTION,
        {},
        priority,
        std::move(from),
        {},
        timeout == network::Duration::zero() ?
            network::Time::max() :
            network::Clock::now() + timeout
    ));
    return true;
}


using WordList = std::vector<std::string>;

static WordList adjectives {
    "anal",
    "annoying",
    "atrocious",
    "awful",
    "bad",
    "boring",
    "clumsy",
    "corrupt",
    "craptacular",
    "crazy",
    "decaying",
    "dastardly",
    "deplorable",
    "deformed",
    "despicable",
    "detrimental",
    "dirty",
    "diseased",
    "disgusting",
    "dishonorable",
    "dreadful",
    "faulty",
    "filthy",
    "foul",
    "ghastly",
    "gross",
    "grotesque",
    "gruesome",
    "hideous",
    "horrible",
    "hostile",
    "inferior",
    "ignorant",
    "ill",
    "infernal",
    "moldy",
    "monstrous",
    "nasty",
    "naughty",
    "noxious",
    "obnoxious",
    "odious",
    "petty",
    "questionable",
    "repellent",
    "repulsive",
    "repugnant",
    "revolting",
    "rotten",
    "rude",
    "sad",
    "savage",
    "sick",
    "sickening",
    "slimy",
    "smelly",
    "sorry",
    "spiteful",
    "sticky",
    "stinky",
    "stupid",
    "terrible",
    "terrifying",
    "toxic",
    "ugly",
    "unpleasant",
    "unsatisfactory",
    "unwanted",
    "vicious",
    "vile",
    "wicked",
    "worthless",
    "yucky",
};

static WordList amounts {
    "accumulation",
    "ass-full",
    "assload",
    "bag",
    "bucket",
    "bucketful",
    "bunch",
    "bundle",
    "buttload",
    "cloud",
    "crapload",
    "dozen",
    "fuckload",
    "fuckton",
    "heap",
    "horde",
    "horseload",
    "legion",
    "load",
    "mass",
    "mound",
    "multitude",
    "myriad",
    "pile",
    "plate",
    "puddle",
    "shitload",
    "stack",
    "ton",
    "zillion",
};

static WordList animal {
    "anglerfish",
    "bat",
    "bug",
    "cat",
    "chicken",
    "cockroach",
    "dog",
    "donkey",
    "eel",
    "horse",
    "leech",
    "lizard",
    "maggot",
    "monkey",
    "pig",
    "pony",
    "rat",
    "skunk",
    "slug",
    "snake",
    "toad",
};

static WordList animal_part {
    "assholes",
    "balls",
    "dicks",
    "droppings",
    "dung",
    "excretions",
    "farts",
    "goo",
    "guts",
    "intestines",
    "ooze",
    "orifices",
    "piss",
    "poop",
    "puke",
    "pus",
    "skins",
    "shit",
    "slime",
    "snot",
    "spit",
    "stench",
    "toenails",
    "urine",
    "vomit",
};

static std::string random_word(const WordList& words)
{
    if ( words.empty() )
        return {};

    return words[melanolib::math::random(words.size()-1)];
}

static WordList random_words(const WordList& words, WordList::size_type count)
{
    if ( words.empty() || count == 0 )
        return {};

    std::set<WordList::size_type> used;
    WordList out_words;
    while ( out_words.size() < count )
    {
        WordList::size_type n;
        do
            n = melanolib::math::random(words.size()-1);
        while ( used.count(n) );
        out_words.push_back(words[n]);
    }
    return out_words;
}

std::string Insult::random_adjectives() const
{
    using melanolib::math::random;
    using namespace melanolib::string;

    return implode(" ",
        random_words(adjectives, random(min_adjectives, max_adjectives)));
}

Insult::Insult(const Settings& settings, MessageConsumer* parent)
    : SimpleAction("insult", settings, parent)
{
    synopsis += " [something]";
    help = "Gives a true statement about the subject";

    min_adjectives = settings.get("min_adjectives", min_adjectives);
    max_adjectives = settings.get("min_adjectives", max_adjectives);
    // Todo read list of words from storage
}

bool Insult::on_handle(network::Message& msg)
{
    using namespace melanolib::string;

    std::string subject = english.pronoun_1stto3rd(msg.message, msg.from.name);

    if ( icase_equal(subject, msg.source->name()) )
        subject = msg.from.name;

    if ( subject.empty() )
        subject = "You are";
    else
        subject += " is";

    std::string insult =
        random_adjectives() + ' ' + random_word(amounts) + " of "
        + random_adjectives() + ' ' + random_word(animal) + ' '
        + random_word(animal_part)
    ;
    reply_to(msg, subject + " as " + random_word(adjectives) + " as " +
        english.indefinite_article(insult) + insult);

    return true;
}

} // namespace fun
