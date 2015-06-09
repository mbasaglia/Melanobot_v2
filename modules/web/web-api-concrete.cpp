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
#include "web-api-concrete.hpp"
namespace web {

bool VideoInfo::on_handle(network::Message& msg)
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
                    try {
                        ptree = parser.parse_string(response.contents,response.origin);
                        (this->*found_func)(msg,ptree);
                    } catch ( const JsonError& err ) {
                        ErrorLog errlog("web","JSON Error");
                        if ( settings::global_settings.get("debug",0) )
                            errlog << err.file << ':' << err.line << ": ";
                        errlog << err.what();
                    }
                }
            });

        return true;
    }
    return false;
}

} // namespace web
