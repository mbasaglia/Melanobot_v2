/**
 * \file
 * \brief This files defines some simple Handlers using web APIs
 *
 * \author Mattia Basaglia
 * \copyright Copyright 2015-2016 Mattia Basaglia
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

#include <regex>
#include "web-api.hpp"
#include "melanolib/time/time_string.hpp"
#include "melanolib/utils/type_utils.hpp"
#include "melanolib/math/random.hpp"

namespace web {

/**
 * \brief Handler searching a video on Youtube
 */
class SearchVideoYoutube : public SimpleJson
{
public:
    SearchVideoYoutube(const Settings& settings, MessageConsumer* parent)
        : SimpleJson("video", settings, parent)
    {
        synopsis += " Term...";
        help = "Search a video on YouTube";
        yt_api_key = settings.get("yt_api_key", "");
        order = settings.get("order", order);
        api_url = settings.get("url", api_url);
        reply = read_string(settings, "reply",
            "https://www.youtube.com/watch?v=$videoId");
        not_found_reply = read_string(settings, "not_found",
            "http://www.youtube.com/watch?v=oHg5SJYRHA0");
        if ( yt_api_key.empty() || api_url.empty() || reply.empty() )
            throw melanobot::ConfigurationError();
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        request_json(msg, web::Request("GET", Uri(api_url, {
            {"part", "snippet"},
            {"type", "video" },
            {"maxResults", "1"},
            {"order", order},
            {"key", yt_api_key},
            {"q", msg.message},
        })));
        return true;
    }

    void json_success(const network::Message& msg, const Settings& parsed) override
    {
        int found = parsed.get("pageInfo.totalResults", 0);
        if ( !found )
        {
            reply_to(msg, not_found_reply);
            return;
        }
        string::FormatterUtf8   f8;
        string::FormattedProperties prop {
            {"videoId", parsed.get("items.0.id.videoId", "")},
            {"title", f8.decode(parsed.get("items.0.snippet.title", ""))},
            {"channelTitle", f8.decode(parsed.get("items.0.snippet.channelTitle", ""))},
            {"description", f8.decode(parsed.get("items.0.snippet.description", ""))},
        };
        reply_to(msg, reply.replaced(prop));
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
    string::FormattedString reply;
    /**
     * \brief Fixed reply to give on not found
     */
    string::FormattedString not_found_reply;
};

/**
 * \brief Shows info on video links
 */
class VideoInfo : public melanobot::Handler
{
public:
    VideoInfo(const Settings& settings, MessageConsumer* parent)
        : Handler(settings, parent)
    {
        yt_api_key = settings.get("yt_api_key", "");
        reply = read_string(settings, "reply",
            "Ha Ha! Nice vid $name! $title ($(-b)$duration$(-))");
    }

    bool can_handle(const network::Message& msg) const override
    {
        return msg.type == network::Message::CHAT;
    }

protected:
    bool on_handle(network::Message& msg) override;

    using FoundFunction = void (VideoInfo::*)(const network::Message&, const Settings&);
    std::pair<FoundFunction, web::Request> request_from_match(const std::smatch& match) const;

    /**
     * \brief Found youtube video
     */
    void yt_found(const network::Message& msg, const Settings& parsed)
    {
        if ( !parsed.get("pageInfo.totalResults", 0) )
            return;

        string::FormatterUtf8 f8;
        send_message(msg, {
            {"videoId", parsed.get("items.0.id", "")},
            {"title", f8.decode(parsed.get("items.0.snippet.title", ""))},
            {"channelTitle", f8.decode(parsed.get("items.0.snippet.channelTitle", ""))},
            {"description", f8.decode(parsed.get("items.0.snippet.description", ""))},
            {"duration",
                melanolib::time::duration_string_short(
                    melanolib::time::parse_duration(
                        parsed.get("items.0.contentDetails.duration", "")
                ))}
        });
    }

    /**
     * \brief Found a Vimeo video
     */
    void vimeo_found(const network::Message& msg, const Settings& parsed)
    {
        string::FormatterUtf8 f8;
        send_message(msg, {
            {"videoId", parsed.get("0.id", "")},
            {"title", f8.decode(parsed.get("0.title", ""))},
            {"channelTitle", f8.decode(parsed.get("0.user_name", ""))},
            {"description", f8.decode(parsed.get("0.description", ""))},
            {"duration",
                melanolib::time::duration_string_short(
                    melanolib::time::seconds(
                        parsed.get("0.duration", 0)
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
        string::FormatterUtf8   f8;
        send_message(msg, {
            {"videoId", parsed.get("id", "")},
            {"title", f8.decode(parsed.get("title", ""))},
            {"channelTitle", f8.decode(parsed.get("cannel", ""))},
            {"description", f8.decode(parsed.get("description", ""))},
            {"duration",
                melanolib::time::duration_string_short(
                    melanolib::time::seconds(
                        parsed.get("duration", 0)
                ))}
        });
    }

    /**
     * \brief Found a Vid.me video
     */
    void vidme_found(const network::Message& msg, const Settings& parsed)
    {
        if ( parsed.get_child_optional("error") )
            return;
        string::FormatterUtf8   f8;
        send_message(msg, {
            {"videoId", parsed.get("video.video_id", "")},
            {"title", f8.decode(parsed.get("video.title", ""))},
            {"channelTitle", f8.decode(parsed.get("user.username", ""))},
            {"description", f8.decode(parsed.get("video.description", ""))},
            {"duration", melanolib::time::duration_string_short(
                melanolib::time::seconds(int(parsed.get("video.duration", 0.0)))
            )}
        });
    }

    /**
     * \brief Send message with the video info
     */
    void send_message(const network::Message& msg, const string::FormattedProperties& properties)
    {
        auto response = reply.replaced(properties);
        response.replace(string::FormattedProperties{
            {"channel", melanolib::string::implode(", ", msg.channels)},
            {"name", msg.source->decode(msg.from.name)},
            {"host", msg.from.host},
            {"global_id", msg.from.global_id},
        });
        reply_to(msg, std::move(response));
    }

private:
    /**
     * \brief Message regex
     */
    std::regex regex{
        R"regex((?:(?:youtube\.com/watch\?v=|youtu\.be/)([-_0-9a-zA-Z]+)))regex"
        R"regex(|(?:vimeo\.com/([0-9]+)))regex"
        R"regex(|(?:dailymotion\.com/video/([0-9a-zA-Z]+)))regex"
        R"regex(|(?:vid.me/(?:e/)?([0-9a-zA-Z]+)))regex",
        std::regex::ECMAScript|std::regex::optimize
    };
    /**
     * \brief Reply to give on found
     */
    string::FormattedString reply;
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
    /**
     * \brief Vid.me API URL
     */
    std::string vidme_api_url = "https://api.vid.me/videoByUrl/";
};

/**
 * \brief Handler searching a definition on Urban Dictionary
 */
class UrbanDictionary : public SimpleJson
{
public:
    UrbanDictionary(const Settings& settings, MessageConsumer* parent)
        : SimpleJson("define", settings, parent)
    {
        synopsis += " Term...";
        help = "Search a definition on Urban Dictionary";
        not_found_reply = read_string(settings, "not_found", "I don't know what $search means");
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        std::string url = "http://api.urbandictionary.com/v0/define";
        request_json(msg, web::Request("GET", {url, {{"term", msg.message}}}));
        return true;
    }

    void json_success(const network::Message& msg, const Settings& parsed) override
    {
        std::string result = parsed.get("list.0.definition", "");

        if ( result.empty() )
            reply_to(msg, not_found_reply.replaced( Properties{
                {"search", msg.message},
                {"user", msg.from.name}
            }));
        else
            reply_to(msg, melanolib::string::elide(
                melanolib::string::collapse_spaces(result),
                400
            ));
    }
private:
    string::FormattedString not_found_reply;
};

/**
 * \brief Handler searching a web page using Searx
 */
class SearchWebSearx : public SimpleJson
{
public:
    SearchWebSearx(const Settings& settings, MessageConsumer* parent);

protected:
    bool on_handle(network::Message& msg) override
    {
        request_json(msg, web::Request("GET", {api_url, {
            {"format", "json"},
            {"q", msg.message},
            {"categories", category}
        }}));
        return true;
    }

    void json_success(const network::Message& msg, const Settings& parsed) override;

    void json_failure(const network::Message& msg) override;

private:
    /// Base url for the searx installation
    std::string api_url = "https://searx.me/";
    /**
     * \brief Reply to give on a successful call.
     * Expands: title, url, image, longitude, latitude
     */
    string::FormattedString found_reply;
    /**
     * \brief Length of the content description
     *
     * Longer conent is elided, if 0 no description, if negative unelided
     */
    int description_maxlen = 400;
    /**
     * \brief Reply given when no results were found
     *
     * Expands: search, user
     */
    string::FormattedString not_found_reply;
    /**
     * \brief Search categry
     */
    std::string category = "general";
};

/**
 * \brief Handler searching on a Mediawiki (text search)
 */
class MediaWiki : public SimpleJson
{
public:
    MediaWiki(const Settings& settings, MessageConsumer* parent)
        : SimpleJson("wiki", settings, parent)
    {
        synopsis += " Term...";
        help = "Search a page on a wiki";
        api_url = settings.get("url", api_url);
        reply = read_string(settings, "reply", "$snippet");
        not_found_reply = read_string(settings, "not_found_reply",
                                      "I don't know anything about $search");
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        request_json(msg, web::Request("GET", {api_url, {
            {"format",   "json"},
            {"action",   "query"},
            {"list",     "search"},
            {"srsearch", msg.message},
            {"srlimit",  "1"},
        }}));
        return true;
    }

    void json_success(const network::Message& msg, const Settings& parsed) override
    {
        auto result = parsed.get_child_optional("query.search.0");

        string::FormattedProperties prop {
            {"search", msg.source->decode(msg.message)},
            {"user", msg.source->decode(msg.from.name)},
        };

        if ( !result )
        {
            reply_to(msg, not_found_reply.replaced(prop));
            return;
        }
        prop["title"] = result->get("title", "");
        /// \todo Transform snippet to plaintext
        prop["snippet"] = result->get("snippet", "");

        reply_to(msg, reply.replaced(prop));
    }

protected:
    /**
     * \brief API endpoint URL
     */
    std::string api_url = "http://en.wikipedia.org/w/api.php";
    /**
     * \brief Reply to give on found
     */
    string::FormattedString reply;
    /**
     * \brief Reply to give on not found
     */
    string::FormattedString not_found_reply;
};

/**
 * \brief Handler searching on a Mediawiki (Title Search)
 */
class MediaWikiTitles : public MediaWiki
{
public:
    MediaWikiTitles(const Settings& settings, MessageConsumer* parent)
        : MediaWiki(settings, parent)
    {}

protected:
    bool on_handle(network::Message& msg) override
    {
        request_json(msg, web::Request("GET", {api_url, {
            {"format",  "json"},
            {"action",  "query"},
            {"prop",    "revisions"},
            {"titles",  msg.message},
            {"rvprop",  "content"},
            {"rvsection", "0"},
            {"redirects", ""},
        }}));
        return true;
    }

    void json_success(const network::Message& msg, const Settings& parsed) override
    {
        auto result = parsed.get_child_optional("query.pages");

        string::FormattedProperties prop {
            {"search", msg.source->decode(msg.message)},
            {"user", msg.source->decode(msg.from.name)},
        };

        if ( result && !result->empty() )
            result = result->front().second;

        if ( !result || !settings::has_child(*result, "revisions.0.*") )
        {
            reply_to(msg, not_found_reply.replaced(prop));
            return;
        }

        prop["title"] = result->get("title", "");
        /// \todo Transform snippet to plaintext
        prop["snippet"] = result->get("revisions.0.*", "");

        reply_to(msg, reply.replaced(prop));
    }
};

/**
 * \brief Returns a random page title from a category
 */
class MediaWikiCategoryTitle : public melanobot::SimpleAction
{
private:
    template<class Callback>
    struct MultiCallback
    {
        MultiCallback(Callback on_loaded)
            : on_loaded(std::move(on_loaded))
        {}

        ~MultiCallback()
        {
            auto lock = make_lock(mutex);
            on_loaded(items);
        }

        void append(const std::vector<std::string>& chunk)
        {
            auto lock = make_lock(mutex);
           items.insert(items.end(), chunk.begin(), chunk.end());
        }

        std::mutex mutex;
        std::vector<std::string> items;
        Callback on_loaded;
    };

    template<class Callback>
        using MutiCallbackWrapper = std::shared_ptr<MultiCallback<Callback>>;

public:
    MediaWikiCategoryTitle(const Settings& settings, MessageConsumer* parent)
        : melanobot::SimpleAction("random_title", settings, parent)
    {

        std::string item_name = settings.get("item_name", "item");

        synopsis += " Term...";
        help = "Name a a random " + item_name;

        api_url = settings.get("url", api_url);
        reply = read_string(settings, "reply", "$item");
        not_found_reply = read_string(settings, "not_found_reply",
                                      "I didn't find any " + item_name);

        default_categories = comma_split(settings.get("default", ""));

        title_pattern = std::regex(
            settings.get("pattern", "[^:/]+"),
            std::regex::optimize
        );

        if ( !default_categories.empty() && settings.get("preload", false) )
            load_categories(default_categories, melanolib::Noop{});
    }

protected:
    std::vector<std::string> comma_split(const std::string& words)
    {
        static std::regex regex_comma ( "(,\\s*)",
            std::regex_constants::optimize |
            std::regex_constants::ECMAScript );
        return melanolib::string::regex_split(words, regex_comma);
    }

    bool on_handle(network::Message& msg) override
    {
        load_categories(
            msg.message.empty() ? default_categories : comma_split(msg.message),
            [this, msg](const std::vector<std::string>& pages)
            {
                if ( pages.empty() )
                    reply_to(msg, not_found_reply);
                else
                    reply_to(msg, pages[melanolib::math::random(pages.size()-1)]);
            }
        );
        return true;
    }

    template<class Callback>
        void load_categories(const std::vector<std::string>& categories, const Callback& on_loaded)
    {
        auto multi_callback = std::make_shared<MultiCallback<Callback>>(on_loaded);
        for ( const auto& category : categories )
            load_category(category, multi_callback);
    }

private:
    template<class Callback>
        void load_category(const std::string& category,
                           const MutiCallbackWrapper<Callback>& callback)
    {
        auto lock = make_lock(mutex);
        auto it = titles.find(category);
        if ( it != titles.end() )
        {
            callback->append(it->second);
            return;
        }
        titles.insert(it, {category, {}});
        lock.unlock();

        request(category, "", callback);
    }

    template<class Callback>
    void request(const std::string& category,
                 const std::string& cmcontinue,
                 const MutiCallbackWrapper<Callback>& callback)
    {
        auto category_id = "Category:" + melanolib::string::slug(category);
        web::HttpClient::instance().async_query(
            web::Request("GET", web::Uri(api_url, {
                {"format",      "json"},
                {"action",      "query"},
                {"list",        "categorymembers"},
                {"cmlimit",     "300"},
                {"cmtitle",     category_id},
                {"cmcontinue",  cmcontinue},
            })),
            [this, callback, category, cmcontinue](Request& req, web::Response& resp)
            {
                if ( !resp.status.is_error() )
                {
                    Settings ptree;
                    httpony::json::JsonParser parser;
                    try {
                        ptree = parser.parse(resp.body, req.uri.full());
                    } catch ( const httpony::json::JsonError& err ) {
                        ErrorLog errlog("web", "JSON Error");
                        if ( settings::global_settings.get("debug", 0) )
                            errlog << err.file << ':' << err.line << ": ";
                        errlog << err.what();
                        return;
                    }

                    auto next = ptree.get("query-continue.categorymembers.cmcontinue", "");
                    if ( !next.empty() )
                        request(category, cmcontinue, callback);

                    auto lock = make_lock(mutex);
                    auto& list = titles[category];
                    for ( const auto& page : ptree.get_child("query.categorymembers", {}) )
                    {
                        std::string title = page.second.get("title", "");
                        if ( std::regex_match(title, title_pattern) )
                            list.push_back(title);
                    }
                    auto list_copy = list;
                    lock.unlock();

                    if ( next.empty() )
                        callback->append(std::move(list_copy));
                }
            });
    }

    std::vector<std::string> default_categories;
    std::regex title_pattern;
    std::string item_name = "item";

    std::map<std::string, std::vector<std::string>> titles;
    std::mutex mutex;

    /**
     * \brief API endpoint URL
     */
    std::string api_url = "http://en.wikipedia.org/w/api.php";

    /**
     * \brief Reply to give on found
     */
    string::FormattedString reply;

    /**
     * \brief Reply to give on not found
     */
    string::FormattedString not_found_reply;
};


class WhereIsGoogle : public SimpleJson
{
public:
    WhereIsGoogle(const Settings& settings, MessageConsumer* parent)
        : SimpleJson("where", settings, parent)
    {
        synopsis += " Term...";
        help = "Search a plage on Google maps";
    }
protected:
    bool on_handle(network::Message& msg) override
    {
        static std::string url = "http://maps.googleapis.com/maps/api/geocode/json?sensor=false";
        auto subject = get_subject(msg);
        request_json(msg, web::Request("GET", {url, {{"address", subject}}}));
        // map types: satellite terrain hybrid roadmap
        reply_to(msg, "http://maps.google.com/maps/api/staticmap?size=400x400&maptype=hybrid&sensor=false&format=png&markers=" + urlencode(subject));
        return true;
    }

    void json_success(const network::Message& msg, const Settings& parsed) override
    {
        std::string address = parsed.get("results.0.formatted_address", "");
        auto subject = get_subject(msg);
        std::string near = address;
        if ( address.empty() )
        {
            address = "I don't know";
            near = subject;
        }
        std::string url = "https://maps.google.com/?" + build_query_string({{"q", subject}, {"hnear", near}});
        reply_to(msg, address + ": " + url);
    }

private:
    std::string get_subject(const network::Message& msg)
    {
        static std::regex regex_question(
            R"(^\s*(?:is|are)?\s*([^?]+)(?:\?.*)?)",
            std::regex::ECMAScript|std::regex::optimize|std::regex::icase
        );
        std::smatch match;
        if ( std::regex_match(msg.message, match, regex_question) )
            return match[1];
        return msg.from.name;
    }
};

} // namespace web

#endif // WEB_API_CONCRETE
