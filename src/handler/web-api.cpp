/**
 * \file
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
#include "web-api.hpp"

namespace handler {

class SearchVideo : public SimpleJson
{
public:
    SearchVideo(const Settings& settings, Melanobot* bot)
        : SimpleJson("video",settings,bot)
    {
        yt_api_url = settings.get("url",
            "https://gdata.youtube.com/feeds/api/videos?alt=json&max-results=1");
        not_found_reply = settings.get("not_found",
            "http://www.youtube.com/watch?v=oHg5SJYRHA0" );
    }

protected:
    bool on_handle(network::Message& msg) override
    {/// \todo strip prefix from msg
        request_json(msg,network::http::get(yt_api_url,{{"q",msg.message}}));
        return true;
    }

    void json_success(const network::Message& msg, const Settings& parsed) override
    {
        reply_to(msg,parsed.get("feed.entry.0.link.0.href",not_found_reply));
    }

private:
    std::string yt_api_url;
    std::string not_found_reply;
};
REGISTER_HANDLER(SearchVideo,SearchVideo);

} // namespace handler
