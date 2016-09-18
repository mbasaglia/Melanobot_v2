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
        std::ofstream file(
            settings::data_file(file_name() + suffix, settings::FileCheck::CREATE)
        );
        generator.store(file, format);
    }

    void load(const std::string& suffix = "",
              StorageFormat format = StorageFormat::TextPlain)
    {
        std::string file_path = settings::data_file(file_name(), settings::FileCheck::EXISTING);
        if ( !file_path.empty() )
        {
            std::ifstream file(file_path);
            generator.load(file);
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

    std::string markov_key;
    melanolib::string::TextGenerator generator;
};

class MarkovListener : public melanobot::Handler
{
public:
    MarkovListener(const Settings& settings, MessageConsumer* parent)
        : Handler(settings, parent)
    {
        direct = settings.get("direct", direct);
        markov_key = settings.get("markov_key", markov_key);
        generator = &MarkovGeneratorWrapper::get_generator(markov_key).generator;
        generator->set_max_age(melanolib::time::days(
            settings.get("max_age", generator->max_age().count())
        ));
        generator->set_max_size(settings.get("max_size", generator->max_size()));
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
    std::string markov_key;
    melanolib::string::TextGenerator* generator = nullptr;
};

class MarkovTextGenerator : public melanobot::SimpleAction
{
public:
    MarkovTextGenerator(const Settings& settings, MessageConsumer* parent)
        : SimpleAction("chat", "chat( about)?", settings, parent)
    {
        synopsis += " [about subject...]";
        help = "Generates a random chat message";
        std::string markov_key = settings.get("markov_key", "");
        generator = &MarkovGeneratorWrapper::get_generator(markov_key).generator;
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

} // namespace fun
#endif // MELANOBOT_FUN_MARKOV_HPP
