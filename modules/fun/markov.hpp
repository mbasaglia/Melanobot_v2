/**
 * \file
 *
 * \author Mattia Basaglia
 *
 * \copyright Copyright 2016 Mattia Basaglia
 *
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
#ifndef MELANOBOT_FUN_MARKOV_HPP
#define MELANOBOT_FUN_MARKOV_HPP

#include "melanobot/handler.hpp"
#include "melanolib/string/text_generator.hpp"
#include "settings.hpp"
#include "web/server/base_pages.hpp"

namespace fun {

struct MarkovGeneratorWrapper
{
    using StorageFormat = melanolib::string::TextGenerator::StorageFormat;
    MarkovGeneratorWrapper(const std::string& markov_key = "")
    {
        set_key(markov_key);
    }

    ~MarkovGeneratorWrapper()
    {
        save();
    }

    void save(const std::string& suffix = "",
              StorageFormat format = StorageFormat::TextPlain) const
    {
        if ( read_only && suffix.empty() )
            return;

        std::string file_path = settings::data_file(file_name() + suffix, settings::FileCheck::CREATE);
        Log("sys", '!', 4) << "Storing Markov data to " << file_path;

        try {
            std::ofstream stream(file_path);
            generator.store(stream, format);
        } catch(const std::runtime_error& ) {
            ErrorLog("sys") << "Couldn't store Markov data to " << file_path;
        }
    }

    void load(const std::string& suffix = "",
              StorageFormat format = StorageFormat::TextPlain)
    {
        std::string file_path = settings::data_file(file_name(), settings::FileCheck::EXISTING);
        if ( !file_path.empty() )
        {
            Log("sys", '!', 4) << "Loading Markov data from " << file_path;
            std::ifstream file(file_path);
            try {
                generator.load(file);
            } catch(const std::runtime_error& ) {
                ErrorLog("sys") << "Couldn't load Markov data from " << file_path;
            }
        }
    }

    std::string file_name() const
    {
        return "markov/" + markov_key;
    }

    void set_key(const std::string& key)
    {
        markov_key = key.empty() ? "markov" : key;
    }

    void read_settings(const Settings& settings)
    {
        std::size_t max_size = settings.get("max_size", generator.max_size());
        if ( max_size == 0 )
            max_size = std::numeric_limits<std::size_t>::max();
        generator.set_max_size(max_size);
        generator.set_max_age(melanolib::time::days(
            settings.get("max_age", generator.max_age().count())
        ));
        read_only = settings.get("read_only", read_only);
    }

    static MarkovGeneratorWrapper& get_generator(const std::string& key)
    {
        static std::unordered_map<std::string, MarkovGeneratorWrapper> generators;
        auto iter = generators.find(key);
        if ( iter != generators.end() )
            return iter->second;

        auto& gen = generators[key];
        gen.set_key(key);
        gen.load();
        return gen;
    }

    static MarkovGeneratorWrapper& get_generator(const Settings& settings)
    {
        std::string markov_key = settings.get("markov_key", "");
        auto& gen = get_generator(markov_key);
        gen.read_settings(settings);
        return gen;
    }

    std::string markov_key;
    melanolib::string::TextGenerator generator;
    bool read_only = true;
};

class MarkovListener : public melanobot::Handler
{
public:
    MarkovListener(const Settings& settings, MessageConsumer* parent)
        : Handler(settings, parent)
    {
        direct = settings.get("direct", direct);
        auto& wrapper = MarkovGeneratorWrapper::get_generator(settings);
        wrapper.read_only = false;
        generator = &wrapper.generator;
    }

    static melanolib::string::TextGenerator& markov_generator(const std::string& key)
    {
        static std::map<std::string, melanolib::string::TextGenerator> generators;
        return generators[key];
    }

protected:
    bool can_handle(const network::Message& msg) const override
    {
        return msg.type == network::Message::CHAT && (msg.direct || !direct);
    }

    bool on_handle(network::Message& msg) override
    {
        generator->add_text(melanolib::string::trimmed(
            msg.source->encode_to(msg.message, string::FormatterUtf8())
        ));
        return true;
    }

private:
    bool direct = false; ///< Whether the message needs to be direct
    melanolib::string::TextGenerator* generator = nullptr;
};

class MarkovTextGenerator : public melanobot::SimpleAction
{
public:
    MarkovTextGenerator(const Settings& settings, MessageConsumer* parent)
        : SimpleAction("chat", settings, parent)
    {
        synopsis += " [about subject...]";
        help = "Generates a random chat message";
        pattern = std::regex(
            melanolib::string::regex_escape(trigger) + "( about\\s+)?",
            std::regex::ECMAScript|std::regex::optimize
        );
        std::string markov_key = settings.get("markov_key", "");
        generator = &MarkovGeneratorWrapper::get_generator(settings).generator;
        min_words = settings.get("min_words", min_words);
        enough_words = settings.get("enough_words", enough_words);
        max_words = settings.get("max_words", max_words);
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        std::string subject = melanolib::string::trimmed(
            msg.source->encode_to(msg.message, string::FormatterUtf8())
        );

        subject = generator->generate_string(subject, min_words, enough_words, max_words);
        reply_to(msg, subject);
        return true;
    }

private:
    melanolib::string::TextGenerator* generator = nullptr;
    std::size_t min_words = 5;
    std::size_t enough_words = 10;
    std::size_t max_words = 100;
};

class MarkovSave : public melanobot::SimpleAction
{
public:
    MarkovSave(const Settings& settings, MessageConsumer* parent)
        : SimpleAction("save markov", settings, parent)
    {
        synopsis += " [dot]";
        help = "Saves the text generator graph";
        std::string markov_key = settings.get("markov_key", "");
        generator = &MarkovGeneratorWrapper::get_generator(markov_key);
        generator->read_only = false;
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        try
        {
            if ( msg.message == "dot" )
                generator->save(".dot", MarkovGeneratorWrapper::StorageFormat::Dot);
            else
                generator->save();
            reply_to(msg, "Graph saved");
        }
        catch ( const std::runtime_error& error )
        {
            reply_to(msg, "Error saving the graph");
        }

        return true;
    }

private:
    MarkovGeneratorWrapper *generator;

};

class MarkovStatus : public melanobot::SimpleAction
{
public:
    MarkovStatus(const Settings& settings, MessageConsumer* parent)
        : SimpleAction("markov status", settings, parent)
    {
        help = "Shows info on the text generator";
        std::string markov_key = settings.get("markov_key", "");
        generator = &MarkovGeneratorWrapper::get_generator(markov_key).generator;
        reply = read_string(settings, "reply",
            "I know $(-b)$word_count$(-) words and a total of $(-b)$transitions$(-) transitions. "
            "The most common word I know is \"$(-i)$most_common$(-)\".");
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        auto stats = generator->stats();

        string::FormattedProperties props {
            {"most_common", stats.most_common},
            {"transitions", std::to_string(stats.transitions)},
            {"word_count", std::to_string(stats.word_count)},
        };

        reply_to(msg, reply.replaced(props));
        return true;
    }

private:
    melanolib::string::TextGenerator *generator;
    string::FormattedString reply;
};

/**
 * \brief Class to generate text from multiple markov chains
 */
class MultiMarkov
{
public:
    explicit MultiMarkov(const Settings& settings)
    {
        min_words = settings.get("min_words", min_words);
        enough_words = settings.get("enough_words", enough_words);
        max_words = settings.get("max_words", max_words);

        std::string markov_prefix = settings.get("markov_prefix", "");
        auto chains = settings.get_child("Chains", {});
        for ( const auto& chain : chains )
        {
            melanolib::string::TextGenerator* generator =
                &MarkovGeneratorWrapper::get_generator(markov_prefix + chain.first).generator;
            auto& output = output_prefixes[generator];
            if ( chain.second.empty() )
            {
                generators[chain.first] = generator;
            }
            else
            {
                for ( const auto& prefix : chain.second )
                {
                    if ( prefix.second.data().empty() || prefix.second.data() == "output" )
                        output.push_back(prefix.first);
                    if ( prefix.second.data().empty() || prefix.second.data() == "input" )
                        generators[prefix.first] = generator;
                }
            }
        }
    }

    bool contains(const std::string& generator) const
    {
        return generators.count(generator);
    }

    bool generate(const std::string& generator, const std::string& prompt, std::string& output) const
    {
        return generate(generator, prompt, min_words, enough_words, output);
    }

    bool generate(const std::string& generator, const std::string& prompt,
                  std::size_t min_words, std::size_t enough_words,
                  std::string& output) const
    {
        auto iter = generators.begin();

        if ( generator.empty() )
            std::advance(iter, melanolib::math::random(0, generators.size() - 1));
        else
            iter = generators.find(generator);

        if ( iter != generators.end() )
        {
            min_words = melanolib::math::min(min_words, max_words);
            enough_words = melanolib::math::min(enough_words, max_words);
            melanolib::string::TextGenerator* generator = iter->second;
            const auto& prefixes = output_prefixes.at(generator);
            if ( !prefixes.empty() )
                output = prefixes[melanolib::math::random(0, prefixes.size() - 1)];
            output += generator->generate_string(prompt, min_words, enough_words, max_words);
            return true;
        }
        return false;
    }

protected:
    std::size_t min_words = 5;
    std::size_t enough_words = 10;
    std::size_t max_words = 100;
    std::unordered_map<melanolib::string::TextGenerator*, std::vector<std::string>> output_prefixes;
    std::unordered_map<std::string, melanolib::string::TextGenerator*> generators;
};

/**
 * \brief MultiMarkov chat handler
 */
class MultiMarkovTextGenerator : public MultiMarkov, public melanobot::SimpleAction
{
public:
    MultiMarkovTextGenerator(const Settings& settings, MessageConsumer* parent)
        : MultiMarkov(settings), SimpleAction("chat like", settings, parent)
    {
        synopsis += " character [about subject...]";
        help = "Generates a random chat message";

        prompt_separator = settings.get("prompt_separator", prompt_separator);
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        std::string subject = melanolib::string::trimmed(
            msg.source->encode_to(msg.message, string::FormatterUtf8())
        );

        auto split = subject.find(prompt_separator);
        std::string name = subject.substr(0, split);

        std::string prompt;
        if ( split != std::string::npos && split + prompt_separator.size() < subject.size() )
            prompt = subject.substr(split + prompt_separator.size());

        std::string result;
        if ( generate(name, prompt, result) )
        {
            reply_to(msg, result);
            return true;
        }
        return false;
    }

private:
    std::string prompt_separator = " about ";
};

/**
 * \brief MultiMarkov web page (html form)
 */
class MultiMarkovHtmlPage : public MultiMarkov, public web::WebPage
{
public:
    explicit MultiMarkovHtmlPage(const Settings& settings)
        : MultiMarkov(settings)
    {
        uri = read_uri(settings);
        title = settings.get("title", title);
        raw_link = settings.get("raw_link", raw_link);
    }

    bool matches(const web::Request& request, const web::UriPathSlice& path) const override
    {
        return path.match_exactly(uri);
    }

    web::Response respond(web::Request& request, const web::UriPathSlice& path, const web::HttpServer& sv) const override
    {
        web::Response response(request.protocol);

        using namespace httpony::quick_xml::html;
        using namespace httpony::quick_xml;

        std::string selected = request.uri.query["character"];
        Select select_pony("character");
        select_pony.append(Option("", selected == "", false, Text{"Random"}));
        for ( const auto& item : generators )
            select_pony.append(Option(item.first, selected == item.first));

        HtmlDocument html(title);

        Element submit_p{"p", Input("submit", "submit", "Chat!")};

        if ( !raw_link.empty() )
        {
            submit_p.append(
                Input("raw", "submit", "Raw result",
                    Attribute{"onclick", "this.form.action='" + raw_link + "';"})
            );
        }

        html.body().append(
            Element{"h1", Text{title}},
            Element{"form",
                Element{"p",
                    Label("character", "Character"),
                    std::move(select_pony),
                },
                Element{"p",
                    Label("prompt", "Prompt"),
                    Input("prompt", "text", request.uri.query["prompt"])},
                Element{"p",
                    Label("min-words", "Min words"),
                    Input("min-words", "number",
                          request.uri.query.get("min-words", "5"),
                          Attribute{"min", "0"},
                          Attribute{"max", std::to_string(max_words)}
                    )},
                Element{"p",
                    Label("enough-words", "Enough words"),
                    Input("enough-words", "number",
                          request.uri.query.get("enough-words", "10"),
                          Attribute{"min", "0"},
                          Attribute{"max", std::to_string(max_words)}
                    )},
                submit_p
            }
        );

        if ( request.uri.query.contains("submit") )
        {
            std::string result;
            generate(
                request.uri.query["character"],
                request.uri.query["prompt"],
                melanolib::string::to_uint(request.uri.query["min-words"]),
                melanolib::string::to_uint(request.uri.query["enough-words"]),
                result
            );
            html.body().append(
                Element{"h2", Text{"Output"}},
                Element{"div", Text{result}}
            );
        }

        response.body.start_output("text/html; charset=utf-8");
        html.print(response.body, true);
        response.body << "\r\n";
        return response;

        return response;
    }
private:
    web::UriPath uri;
    std::string title = "Chat generator";
    std::string raw_link;
};

/**
 * \brief MultiMarkov web page (plain text)
 */
class MultiMarkovPlainPage : public MultiMarkov, public web::WebPage
{
public:
    explicit MultiMarkovPlainPage(const Settings& settings)
        : MultiMarkov(settings)
    {
        uri = read_uri(settings);
    }

    bool matches(const web::Request& request, const web::UriPathSlice& path) const override
    {
        return path.match_prefix(uri);
    }

    web::Response respond(web::Request& request, const web::UriPathSlice& path, const web::HttpServer& sv) const override
    {

        std::string character;
        if ( path.size() > uri.size() )
            character = path[uri.size()];
        else
            character = request.uri.query["character"];
        std::string result;
        auto generated_ok = generate(
            character,
            request.uri.query["prompt"],
            melanolib::string::to_uint(request.uri.query["min-words"]),
            melanolib::string::to_uint(request.uri.query["enough-words"]),
            result
        );

        if ( !generated_ok )
            throw web::HttpError(web::StatusCode::NotFound);

        web::Response response("text/plain; charset=utf-8", {}, request.protocol);
        response.body << result << "\r\n";
        return response;
    }

private:
    web::UriPath uri;
};

} // namespace fun
#endif // MELANOBOT_FUN_MARKOV_HPP
