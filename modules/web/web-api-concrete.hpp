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
#include "time/time_string.hpp"

namespace handler {

/**
 * \brief Handler searching a video on Youtube
 */
class SearchVideoYoutube : public SimpleJson
{
public:
    SearchVideoYoutube(const Settings& settings, MessageConsumer* parent)
        : SimpleJson("video",settings,parent)
    {
        yt_api_key = settings.get("yt_api_key", "");
        order = settings.get("order", order);
        api_url = settings.get("url", api_url);
        reply = settings.get("reply", reply);
        not_found_reply = settings.get("not_found", not_found_reply );
        if ( yt_api_key.empty() || api_url.empty() || reply.empty() )
            throw ConfigurationError();
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        request_json(msg,network::http::get(api_url,{
            {"part", "snippet"},
            {"type", "video" },
            {"maxResults","1"},
            {"order",order},
            {"key",yt_api_key},
            {"q",msg.message},
        }));
        return true;
    }

    void json_success(const network::Message& msg, const Settings& parsed) override
    {
        int found = parsed.get("pageInfo.totalResults",0);
        if ( !found )
        {
            reply_to(msg,not_found_reply);
            return;
        }
        string::FormatterConfig fmt;
        string::FormatterUtf8   f8;
        Properties prop {
            {"videoId",parsed.get("items.0.id.videoId","")},
            {"title",f8.decode(parsed.get("items.0.snippet.title","")).encode(fmt)},
            {"channelTitle",f8.decode(parsed.get("items.0.snippet.channelTitle","")).encode(fmt)},
            {"description",f8.decode(parsed.get("items.0.snippet.description","")).encode(fmt)},
        };
        reply_to(msg,fmt.decode(string::replace(reply,prop,"%")));
    }

private:
    /**
     * \brief API soring order
     *
     * Acceptable values are:
     *  * date          : reverse chronological order
     *  * rating        : from highest to lowest rating
     *  * relevance     : relevance to the search query
     *  * title         : sorted alphabetically
     *  * viewCount     : from highest to lowest number of views
     */
    std::string order = "relevance";
    /**
     * \brief API key
     */
    std::string yt_api_key;
    /**
     * \brief API URL
     */
    std::string api_url = "https://www.googleapis.com/youtube/v3/search";
    /**
     * \brief Reply to give on found
     */
    std::string reply = "https://www.youtube.com/watch?v=%videoId";
    /**
     * \brief Fixed reply to give on not found
     */
    std::string not_found_reply = "http://www.youtube.com/watch?v=oHg5SJYRHA0";
};

/**
 * \brief Shows info on video links
 */
class VideoInfo : public Handler
{
public:
    VideoInfo(const Settings& settings, MessageConsumer* parent)
        : Handler(settings,parent)
    {
        yt_api_key = settings.get("yt_api_key", "");
        reply = settings.get("reply", reply);
        network::require_service("web");
    }

    bool can_handle(const network::Message& msg) const override
    {
        return msg.type == network::Message::CHAT;
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        std::smatch match;
        if ( std::regex_search(msg.message,match,regex) )
        {
            network::Request request;
            void (VideoInfo::*found_func)(const network::Message&,const Settings&) = nullptr;

            if ( match[1].matched )
            {
                found_func = &VideoInfo::yt_found;
                request = network::http::get(yt_api_url,{
                    {"part", "snippet,contentDetails"},
                    {"maxResults","1"},
                    {"key",yt_api_key},
                    {"id",match[1]},
                });
            }
            else if ( match[2].matched )
            {
                found_func = &VideoInfo::vimeo_found;
                request = network::http::get(vimeo_api_url+std::string(match[2])+".json");
            }
            else if ( match[3].matched )
            {
                found_func = &VideoInfo::dm_found;
                request = network::http::get(dm_api_url+std::string(match[3]),{
                    {"fields", "id,title,channel,duration,description"},
                });
            }

            network::service("web")->async_query(request,
                [this,msg,found_func](const network::Response& response)
                {
                    if ( response.error_message.empty() )
                    {
                        Settings ptree;
                        JsonParser parser;
                        parser.throws(false);
                        ptree = parser.parse_string(response.contents,response.origin);
                        if ( !parser.error() )
                            (this->*found_func)(msg,ptree);
                    }
                });

            return true;
        }
        return false;
    }

    /**
     * \brief Found youtube video
     */
    void yt_found(const network::Message& msg, const Settings& parsed)
    {
        if ( !parsed.get("pageInfo.totalResults",0) )
            return;

        string::FormatterConfig fmt;
        string::FormatterUtf8   f8;
        send_message(msg,{
            {"videoId",parsed.get("items.0.id","")},
            {"title",f8.decode(parsed.get("items.0.snippet.title","")).encode(fmt)},
            {"channelTitle",f8.decode(parsed.get("items.0.snippet.channelTitle","")).encode(fmt)},
            {"description",f8.decode(parsed.get("items.0.snippet.description","")).encode(fmt)},
            {"duration",
                timer::duration_string_short(
                    timer::parse_duration(
                        parsed.get("items.0.contentDetails.duration","")
                ))}
        });
    }

    /**
     * \brief Found a Vimeo video
     */
    void vimeo_found(const network::Message& msg, const Settings& parsed)
    {
        string::FormatterConfig fmt;
        string::FormatterUtf8   f8;
        send_message(msg,{
            {"videoId",parsed.get("0.id","")},
            {"title",f8.decode(parsed.get("0.title","")).encode(fmt)},
            {"channelTitle",f8.decode(parsed.get("0.user_name","")).encode(fmt)},
            {"description",f8.decode(parsed.get("0.description","")).encode(fmt)},
            {"duration",
                timer::duration_string_short(
                    timer::seconds(
                        parsed.get("0.duration",0)
                ))}
        });
    }

    /**
     * \brief Found a Dailymotion video
     */
    void dm_found(const network::Message& msg, const Settings& parsed)
    {
        if ( parsed.get_child_optional("error") )
            return;
        string::FormatterConfig fmt;
        string::FormatterUtf8   f8;
        send_message(msg,{
            {"videoId",parsed.get("id","")},
            {"title",f8.decode(parsed.get("title","")).encode(fmt)},
            {"channelTitle",f8.decode(parsed.get("cannel","")).encode(fmt)},
            {"description",f8.decode(parsed.get("description","")).encode(fmt)},
            {"duration",
                timer::duration_string_short(
                    timer::seconds(
                        parsed.get("duration",0)
                ))}
        });
    }

    /**
     * \brief Send message with the video info
     */
    void send_message(const network::Message& msg, Properties properties)
    {
        string::FormatterConfig fmt;
        properties.insert({
            {"channel",string::implode(", ",msg.channels)},
            {"name", msg.source->encode_to(msg.from.name,fmt)},
            {"host", msg.from.host},
            {"global_id", msg.from.global_id},
        });
        reply_to(msg,fmt.decode(string::replace(reply,properties,"%")));
    }

private:
    /**
     * \brief Message regex
     */
    std::regex regex{
        R"regex((?:(?:youtube\.com/watch\?v=|youtu\.be/)([-_0-9a-zA-Z]+)))regex"
        R"regex(|(?:vimeo\.com/([0-9]+)))regex"
        R"regex(|(?:dailymotion\.com/video/([0-9a-zA-Z]+)))regex",
        std::regex::ECMAScript|std::regex::optimize
    };
    /**
     * \brief Reply to give on found
     */
    std::string reply = "Ha Ha! Nice vid %name! %title (#-b#%duration#-#)";
    /**
     * \brief YouTube API URL
     */
    std::string yt_api_url = "https://www.googleapis.com/youtube/v3/videos";
    /**
     * \brief YouTube API key
     */
    std::string yt_api_key;
    /**
     * \brief Vimeo API URL
     */
    std::string vimeo_api_url = "https://vimeo.com/api/v2/video/";
    /**
     * \brief Dailymotion API URL
     */
    std::string dm_api_url = "https://api.dailymotion.com/video/";
};

/**
 * \brief Handler searching images with Google
 */
class SearchImageGoogle : public SimpleJson
{
public:
    SearchImageGoogle(const Settings& settings, MessageConsumer* parent)
        : SimpleJson("image",settings,parent)
    {
        not_found_reply = settings.get("not_found", not_found_reply );
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        std::string url = "https://ajax.googleapis.com/ajax/services/search/images?v=1.0&rsz=1";
        request_json(msg,network::http::get(url,{{"q",msg.message}}));
        return true;
    }

    void json_success(const network::Message& msg, const Settings& parsed) override
    {
        std::string result = parsed.get("responseData.results.0.unescapedUrl","");
        if ( result.empty() )
            result = string::replace(not_found_reply,{
                {"search", msg.message},
                {"user", msg.from.name}
            }, "%");
        reply_to(msg,result);
    }

private:
    std::string not_found_reply = "Didn't find any image of %search";
};

/**
 * \brief Handler searching a definition on Urban Dictionary
 */
class UrbanDictionary : public SimpleJson
{
public:
    UrbanDictionary(const Settings& settings, MessageConsumer* parent)
        : SimpleJson("define",settings,parent)
    {
        not_found_reply = settings.get("not_found", not_found_reply );
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        std::string url = "http://api.urbandictionary.com/v0/define";
        request_json(msg,network::http::get(url,{{"term",msg.message}}));
        return true;
    }

    void json_success(const network::Message& msg, const Settings& parsed) override
    {
        std::string result = parsed.get("list.0.definition","");

        if ( result.empty() )
            result = string::replace(not_found_reply,{
                {"search", msg.message},
                {"user", msg.from.name}
            }, "%");
        else
            result = string::elide( string::collapse_spaces(result), 400 );

        reply_to(msg,result);
    }
private:
    std::string not_found_reply = "I don't know what %search means";
};


/**
 * \brief Handler searching a web page using Searx
 */
class SearchWebSearx : public SimpleJson
{
public:
    SearchWebSearx(const Settings& settings, MessageConsumer* parent)
        : SimpleJson("search",settings,parent)
    {
        api_url = settings.get("url",api_url);
        not_found_reply = settings.get("not_found", not_found_reply );
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        request_json(msg,network::http::get(api_url,{{"format","json"},{"q",msg.message}}));
        return true;
    }

    void json_success(const network::Message& msg, const Settings& parsed) override
    {
        if ( settings::has_child(parsed,"results.0.title") )
        {
            string::FormatterUtf8 fmt;
            string::FormattedString title(&fmt);
            title << string::FormatFlags::BOLD << parsed.get("results.0.title","")
                  << string::FormatFlags::NO_FORMAT << ": "
                  << parsed.get("results.0.url","");
            reply_to(msg,title);

            std::string result = parsed.get("results.0.content","");
            result = string::elide( string::collapse_spaces(result), 400 );
            reply_to(msg,result);
        }
        else
        {
            std::string result = string::replace(not_found_reply,{
                {"search", msg.message},
                {"user", msg.from.name}
            }, "%");
            reply_to(msg,result);
        }

    }
private:

    std::string api_url = "https://searx.me/";
    std::string not_found_reply = "Didn't find anything about %search";
};

/**
 * \brief Handler searching on a Mediawiki (text search)
 */
class MediaWiki : public SimpleJson
{
public:
    MediaWiki(const Settings& settings, MessageConsumer* parent)
        : SimpleJson("wiki",settings,parent)
    {
        api_url = settings.get("url",api_url);
        reply = settings.get("reply",reply);
        not_found_reply = settings.get("not_found_reply",not_found_reply);
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        request_json(msg,network::http::get(api_url,{
            {"format",  "json"},
            {"action",  "query"},
            {"list",    "search"},
            {"srsearch",msg.message},
            {"srlimit", "1"},
        }));
        return true;
    }

    void json_success(const network::Message& msg, const Settings& parsed) override
    {
        auto result = parsed.get_child_optional("query.search.0");
        string::FormatterConfig fmt;
        Properties prop {
            {"search", msg.source->encode_to(msg.message,fmt)},
            {"user", msg.source->encode_to(msg.from.name,fmt)},
        };

        if ( !result )
        {
            reply_to(msg,fmt.decode(string::replace(not_found_reply,prop,"%")));
            return;
        }
        prop["title"] = result->get("title","");
        /// \todo Transform snippet to plaintext
        prop["snippet"] = result->get("snippet","");

        reply_to(msg,fmt.decode(string::replace(reply,prop,"%")));
    }

protected:
    /**
     * \brief API endpoint URL
     */
    std::string api_url = "http://en.wikipedia.org/w/api.php";
    /**
     * \brief Reply to give on found
     */
    std::string reply = "%snippet";
    /**
     * \brief Reply to give on not found
     */
    std::string not_found_reply = "I don't know anything about %search";
};

/**
 * \brief Handler searching on a Mediawiki (Title Search)
 */
class MediaWikiTitles : public MediaWiki
{
public:
    MediaWikiTitles(const Settings& settings, MessageConsumer* parent)
        : MediaWiki(settings,parent)
    {}

protected:
    bool on_handle(network::Message& msg) override
    {
        request_json(msg,network::http::get(api_url,{
            {"format",  "json"},
            {"action",  "query"},
            {"prop",    "revisions"},
            {"titles",  msg.message},
            {"rvprop",  "content"},
            {"rvsection","0"},
            {"redirects",""},
        }));
        return true;
    }

    void json_success(const network::Message& msg, const Settings& parsed) override
    {
        auto result = parsed.get_child_optional("query.pages");
        string::FormatterConfig fmt;
        Properties prop {
            {"search", msg.source->encode_to(msg.message,fmt)},
            {"user", msg.source->encode_to(msg.from.name,fmt)},
        };

        if ( result && !result->empty() )
            result = result->front().second;

        if ( !result || !settings::has_child(*result,"revisions.0.*") )
        {
            reply_to(msg,fmt.decode(string::replace(not_found_reply,prop,"%")));
            return;
        }

        prop["title"] = result->get("title","");
        /// \todo Transform snippet to plaintext
        prop["snippet"] = result->get("revisions.0.*","");

        reply_to(msg,fmt.decode(string::replace(reply,prop,"%")));
    }
};

} // namespace handler

#endif // WEB_API_CONCRETE
