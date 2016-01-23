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
#ifndef FUN_HANDLERS_HPP
#define FUN_HANDLERS_HPP
#include <unordered_map>

#include <boost/filesystem.hpp>

#include "core/handler/bridge.hpp"
#include "web/web-api.hpp"
#include "melanolib/math.hpp"
#include "melanolib/time/time_string.hpp"

namespace fun {

/**
 * \brief Handler translating between morse and latin
 */
class Morse : public handler::SimpleAction
{
public:
    Morse(const Settings& settings, MessageConsumer* parent)
        : SimpleAction("morse",settings,parent)
    {
        synopsis += " text|morse";
        help = "Converts between ASCII and Morse code";
    }

protected:
    bool on_handle(network::Message& msg) override;

private:
    static std::unordered_map<char,std::string> morse;
};

/**
 * \brief Turns ASCII characters upside-down
 */
class ReverseText : public handler::SimpleAction
{
public:
    ReverseText(const Settings& settings, MessageConsumer* parent)
        : SimpleAction("reverse",settings,parent)
    {
        synopsis += " text";
        help = "Turns ASCII upside-down";
    }

protected:
    bool on_handle(network::Message& msg) override;

private:
    static std::unordered_map<char,std::string> reverse_ascii;
};

/**
 * \brief Searches for a Chuck Norris joke
 */
class ChuckNorris : public web::SimpleJson
{
public:
    ChuckNorris(const Settings& settings, MessageConsumer* parent)
        : SimpleJson("norris",settings,parent)
    {
        synopsis += " [name]";
        help = "Shows a Chuck Norris joke from http://icndb.com";
    }

protected:
    bool on_handle(network::Message& msg) override;

    void json_success(const network::Message& msg, const Settings& parsed) override
    {
        /// \todo convert html entities
        reply_to(msg,parsed.get("value.joke",""));
    }

private:
    std::string api_url = "http://api.icndb.com/jokes/random";
};

/**
 * \brief Draws a pretty My Little Pony ASCII art
 * \note Very useful to see how the bot handles flooding
 * \note Even more useful to see pretty ponies ;-)
 * \see https://github.com/mbasaglia/ASCII-Pony
 */
class RenderPony : public handler::SimpleAction
{
public:
    RenderPony(const Settings& settings, MessageConsumer* parent)
        : SimpleAction("render_pony",settings,parent)
    {
        synopsis += " pony";
        help = "Draws a pretty pony /)^3^(\\";
        pony_path = settings.get("path",pony_path);
    }

protected:
    bool on_handle(network::Message& msg) override;

private:
    std::string pony_path;
};

/**
 * \brief Shows a countdown to the next My Little Pony episode
 */
class PonyCountDown : public web::SimpleJson
{
public:
    PonyCountDown(const Settings& settings, MessageConsumer* parent)
        : SimpleJson("nextpony",settings,parent)
    {
        api_url = settings.get("url",api_url);
        reply = settings.get("found",reply);
        not_found = settings.get("not_found",not_found);
        help = "Pony countdown ("+api_url+")";
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        request_json(msg, web::Request("GET", api_url));
        return true;
    }

    void json_success(const network::Message& msg, const Settings& parsed) override
    {
        if ( parsed.empty() )
            return json_failure(msg);

        Properties map;
        map["title"] = parsed.get("name","");
        map["season"] = melanolib::string::to_string(parsed.get("season",0),2);
        map["episode"] = melanolib::string::to_string(parsed.get("episode",0),2);
        map["duration"] = parsed.get("duration","");
        melanolib::time::DateTime time = melanolib::time::parse_time(parsed.get("time",""));
        melanolib::time::DateTime now;
        auto delta = time > now ? time - now : now - time;
        map["time_delta"] = melanolib::time::duration_string(delta);

        string::FormatterConfig fmt;
        reply_to(msg,fmt.decode(melanolib::string::replace(reply,map,"%")));
    }

    void json_failure(const network::Message& msg) override
    {
        reply_to(msg,not_found);
    }

private:
    std::string api_url = "http://api.ponycountdown.com/next";
    std::string reply = "%time_delta until #-b#%title#-# (S%seasonE%episode)";
    std::string not_found = "Next episode: not soon enough D:";
};

/**
 * \brief Shows a pony face
 */
class PonyFace : public web::SimpleJson
{
public:
    PonyFace(const Settings& settings, MessageConsumer* parent)
        : SimpleJson("ponyface",settings,parent)
    {
        api_url = settings.get("url",api_url);
        not_found = settings.get("not_found",not_found);
        help = "Pony face ("+api_url+")";
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        request_json(msg, web::Request("GET",
            api_url+"tag:"+web::urlencode(msg.message)
        ));
        return true;
    }

    void json_success(const network::Message& msg, const Settings& parsed) override
    {
        if ( parsed.empty() )
            return json_failure(msg);
        auto faces = parsed.get_child("faces",{});
        if ( faces.empty() )
            return json_failure(msg);

        reply_to(msg,faces.get(std::to_string(melanolib::math::random(faces.size()-1))+".image",not_found));
    }

    void json_failure(const network::Message& msg) override
    {
        reply_to(msg,not_found);
    }

private:
    std::string api_url = "http://ponyfac.es/api.json/";
    std::string not_found = "Pony not found http://ponyfac.es/138/full";
};

/**
 * \brief Answers direct questions
 */
class AnswerQuestions : public handler::Handler
{
public:
    AnswerQuestions(const Settings& settings, MessageConsumer* parent)
        : Handler(settings, parent)
    {
        direct    = settings.get("direct",direct);
    }

    bool can_handle(const network::Message& msg) const override
    {
        return msg.direct && !msg.message.empty() && msg.message.back() == '?';
    }

protected:
    bool on_handle(network::Message& msg) override;

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
                       const std::vector<std::vector<std::string>*>& categories) const;
};

/**
 * \brief Slaps someone
 */
class Slap : public handler::SimpleAction
{
public:
    Slap(const Settings& settings, MessageConsumer* parent)
        : SimpleAction("slap",settings,parent)
    {
        synopsis += " victim";
        help = "Slap the victim";
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        reply_to(msg,network::OutputMessage(
            string::FormattedString() << "slaps " << msg.message,
            true
        ));
        return true;
    }
};

/**
 * \brief Like BridgeChat but more colorful
 */
class RainbowBridgeChat : public core::BridgeChat
{
public:
    using core::BridgeChat::BridgeChat;

protected:
    bool on_handle(network::Message& msg) override;
};

/**
 * \brief Discordian Calendar
 */
class Discord : public handler::SimpleAction
{
public:
    Discord(const Settings& settings, MessageConsumer* parent)
        : SimpleAction("discord",settings,parent)
    {
        synopsis += " [time]";
        help = "Show the Discordian date";
        format = settings.get("format",format);
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        static std::vector<std::string> day_names {
            "Sweetmorn", "Boomtime", "Pungenday", "Prickle-Prickle", "Setting Orange"};
        static std::vector<std::string> season_names {
            "Chaos", "Discord", "Confusion", "Bureaucracy", "The Aftermath"};

        melanolib::time::DateTime dt = melanolib::time::parse_time(msg.message);
        int day = dt.year_day();
        int year = dt.year() + 1166;
        if ( year % 4 == 2 && day >= 59 )
            day--;

        Properties discord {
            {"day_name", day_names[day%day_names.size()]},
            {"season_day", std::to_string(day%73+1)+melanolib::string::english.ordinal_suffix(day%73+1)},
            {"season", season_names[(day/73)%season_names.size()]},
            {"yold", std::to_string(year)}
        };

        reply_to(msg,string::FormatterConfig().decode(
            melanolib::string::replace(format,discord,"%") ));
        return true;
    }

private:
    std::string format = "%day_name, the %season_day day of %season in the YOLD %yold";
};

/**
 * \brief Very polite remarks
 */
class Insult : public handler::SimpleAction
{
public:
    Insult(const Settings& settings, MessageConsumer* parent);

protected:
    bool on_handle(network::Message& msg) override;

private:
    std::string random_adjectives() const;

    int min_adjectives = 1;
    int max_adjectives = 3;
};


} // namespace fun
#endif // FUN_HANDLERS_HPP
