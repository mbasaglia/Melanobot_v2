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
#include "web-api-concrete.hpp"
namespace web {

std::pair<VideoInfo::FoundFunction, web::Request>
    VideoInfo::request_from_match(const std::smatch& match) const
{
    if ( match[1].matched )
    {
        return {
            &VideoInfo::yt_found,
            web::Request("GET", {yt_api_url,
            {
                {"part", "snippet, contentDetails"},
                {"maxResults", "1"},
                {"key", yt_api_key},
                {"id", match[1]},
            }})
        };
    }
    else if ( match[2].matched )
    {
        return {
            &VideoInfo::vimeo_found,
            web::Request("GET", vimeo_api_url+std::string(match[2])+".json")
        };
    }
    else if ( match[3].matched )
    {
        return {
            &VideoInfo::dm_found,
            web::Request("GET", dm_api_url+std::string(match[3]),
            {
                {"fields", "id, title, channel, duration, description"},
            })
        };
    }
    else if ( match[4].matched )
    {
        return {
            &VideoInfo::vidme_found,
            web::Request("GET", vidme_api_url+std::string(match[4]))
        };
    }

    return {nullptr, web::Request()};
}

bool VideoInfo::on_handle(network::Message& msg)
{
    std::smatch match;
    if ( std::regex_search(msg.message, match, regex) )
    {
        web::Request request;
        FoundFunction found_func;
        std::tie(found_func, request) = request_from_match(match);

        web::HttpClient::instance().async_query(
            std::move(request),
            [this, msg, found_func](Request& request, Response& response)
            {
                if ( !response.status.is_error() )
                {
                    Settings ptree;
                    JsonParser parser;
                    try {
                        ptree = parser.parse(response.body, request.uri.full());
                        (this->*found_func)(msg, ptree);
                    } catch ( const JsonError& err ) {
                        ErrorLog errlog("web", "JSON Error");
                        if ( settings::global_settings.get("debug", 0) )
                            errlog << err.file << ':' << err.line << ": ";
                        errlog << err.what();
                    }
                }
            }
        );

        return true;
    }
    return false;
}


SearchWebSearx::SearchWebSearx(const Settings& settings, MessageConsumer* parent)
    : SimpleJson("search", settings, parent)
{
    synopsis += " Term...";
    api_url = settings.get("url", api_url);
    not_found_reply = read_string(settings, "not_found",
                                  "Didn't find anything about $search");
    found_reply = read_string(settings, "reply", "$(-b)$title$(-): $url" );
    category = settings.get("category", category );
    description_maxlen = settings.get("description", description_maxlen );


    std::string what = category;
    if ( what == "general" || what.empty() )
        what = "the web";
    help = "Search "+what+" using Searx";
}

void SearchWebSearx::json_success(const network::Message& msg, const Settings& parsed)
{
    using namespace melanolib::string;
    if ( settings::has_child(parsed, "results.0.title") )
    {
        Properties props = {
            {"title",  parsed.get("results.0.title", "")},
            {"url", parsed.get("results.0.url", "")},
            {"image", parsed.get("results.0.img_src", "")},
            {"longitude", parsed.get("results.0.longitude", "")},
            {"latitude", parsed.get("results.0.latitude", "")},
        };

        reply_to(msg, found_reply.replaced(props));

        if ( description_maxlen )
        {
            std::string result = parsed.get("results.0.content", "");
            if ( description_maxlen > 0 )
                result = elide( collapse_spaces(result), description_maxlen );
            reply_to(msg, result);
        }
    }
    else
    {
        json_failure(msg);
    }

}

void SearchWebSearx::json_failure(const network::Message& msg)
{
    Properties props = {
        {"search", msg.message},
        {"user", msg.from.name}
    };

    reply_to(msg, not_found_reply.replaced(props));
}

} // namespace web
