/**
 * \file
 * \brief This files defines some simple Handlers using web APIs
 *
 * \author Mattia Basaglia
 * \copyright Copyright 2015 Mattia Basaglia
 * \license
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
#ifndef WEB_API_CONCRETE
#define WEB_API_CONCRETE

#include "web-api.hpp"

namespace handler {

/**
 * \brief Handler searching a video on Youtube
 */
class SearchVideoYoutube : public SimpleJson
{
public:
    SearchVideoYoutube(const Settings& settings, handler::HandlerContainer* parent)
        : SimpleJson("video",settings,parent)
    {
        api_url = settings.get("url",
            "https://gdata.youtube.com/feeds/api/videos?alt=json&max-results=1");
        not_found_reply = settings.get("not_found",
            "http://www.youtube.com/watch?v=oHg5SJYRHA0" );
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        request_json(msg,network::http::get(api_url,{{"q",msg.message}}));
        return true;
    }

    void json_success(const network::Message& msg, const Settings& parsed) override
    {
        reply_to(msg,parsed.get("feed.entry.0.link.0.href",not_found_reply));
    }

private:
    std::string api_url;
    std::string not_found_reply;
};

/**
 * \brief Handler searching images with Google
 */
class SearchImageGoogle : public SimpleJson
{
public:
    SearchImageGoogle(const Settings& settings, handler::HandlerContainer* parent)
        : SimpleJson("image",settings,parent)
    {}

protected:
    bool on_handle(network::Message& msg) override
    {
        std::string url = "https://ajax.googleapis.com/ajax/services/search/images?v=1.0&rsz=1";
        request_json(msg,network::http::get(url,{{"q",msg.message}}));
        return true;
    }

    void json_success(const network::Message& msg, const Settings& parsed) override
    {
        std::string not_found_reply = "Didn't find any image of "+msg.message;
        reply_to(msg,parsed.get("responseData.results.0.unescapedUrl",not_found_reply));
    }
};



/**
 * \brief Handler searching a definition on Urban Dictionary
 */
class UrbanDictionary : public SimpleJson
{
public:
    UrbanDictionary(const Settings& settings, handler::HandlerContainer* parent)
        : SimpleJson("define",settings,parent)
    {}

protected:
    bool on_handle(network::Message& msg) override
    {
        std::string url = "http://api.urbandictionary.com/v0/define";
        request_json(msg,network::http::get(url,{{"term",msg.message}}));
        return true;
    }

    void json_success(const network::Message& msg, const Settings& parsed) override
    {
        std::string not_found_reply = "I don't know what "+msg.message+" means";
        std::string result = parsed.get("list.0.definition",not_found_reply);
        result = string::elide( string::collapse_spaces(result), 400 );
        reply_to(msg,result);
    }
};


/**
 * \brief Handler searching a web page using Searx
 */
class SearchWebSearx : public SimpleJson
{
public:
    SearchWebSearx(const Settings& settings, handler::HandlerContainer* parent)
        : SimpleJson("search",settings,parent)
    {
        api_url = settings.get("url","https://searx.me/");
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        request_json(msg,network::http::get(api_url,{{"format","json"},{"q",msg.message}}));
        return true;
    }

    void json_success(const network::Message& msg, const Settings& parsed) override
    {
        if ( parsed.has_child("results.0.title") )
        {
            string::FormattedStream title("utf8");
            title << string::FormatFlags::BOLD << parsed.get("results.0.title","")
                  << string::FormatFlags::NO_FORMAT << ": "
                  << parsed.get("results.0.url","");
            reply_to(msg,title.str());

            std::string result = parsed.get("results.0.content","");
            result = string::elide( string::collapse_spaces(result), 400 );
            reply_to(msg,result);
        }
        else
        {
            reply_to(msg,"Didn't find anything about "+msg.message);
        }

    }
private:

    std::string api_url;
};


} // namespace handler

#endif // WEB_API_CONCRETE
