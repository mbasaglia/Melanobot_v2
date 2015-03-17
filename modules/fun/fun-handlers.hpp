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

#include "handler/handler.hpp"
#include "web/web-api.hpp"
#include "math.hpp"

namespace fun {

/**
 * \brief Handler translating between morse and latin
 */
class Morse : public handler::SimpleAction
{
public:
    Morse(const Settings& settings, Melanobot* bot)
        : SimpleAction("morse",settings,bot)
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
    ReverseText(const Settings& settings, Melanobot* bot)
        : SimpleAction("reverse",settings,bot)
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
class ChuckNorris : public handler::SimpleJson
{
public:
    ChuckNorris(const Settings& settings, Melanobot* bot)
        : SimpleJson("norris",settings,bot)
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
    RenderPony(const Settings& settings, Melanobot* bot)
        : SimpleAction("render_pony",settings,bot)
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
 * \brief Answers direct questions
 */
class AnswerQuestions : public handler::Handler
{
public:
    AnswerQuestions(const Settings& settings, Melanobot* bot)
        : Handler(settings, bot)
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
    Slap(const Settings& settings, Melanobot* bot)
        : SimpleAction("slap",settings,bot)
    {
        synopsis += " victim";
        help = "Slap the victim";
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        reply_to(msg,network::OutputMessage(
            (string::FormattedStream() << "slaps " << msg.message),
            true
        ));
        return true;
    }
};

} // namespace fun
#endif // FUN_HANDLERS_HPP
