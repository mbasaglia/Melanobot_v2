/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015-2018 Mattia Basaglia
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
#ifndef MELANOBOT_MODULES_TELEGRAM_INLINE_HPP
#define MELANOBOT_MODULES_TELEGRAM_INLINE_HPP

#include "message/input_message.hpp"

namespace telegram {


class InlineQueryResult
{
public:
    virtual ~InlineQueryResult(){}
    virtual PropertyTree to_properties() const = 0;
};

class InlineQueryResponse
{
public:
    using pointer = std::unique_ptr<InlineQueryResult>;

    explicit InlineQueryResponse(
            const network::Message& msg,
            int cache_time = -1,
            bool is_personal = false,
            std::string next_offset = {},
            std::string switch_pm_text = {},
            std::string switch_pm_parameter = {}
    ) : cache_time(cache_time),
        is_personal(is_personal),
        next_offset(std::move(next_offset)),
        switch_pm_text(std::move(switch_pm_text)),
        switch_pm_parameter(std::move(switch_pm_parameter))
    {
        if ( msg.command != "inline_query" || msg.params.empty() )
            throw melanobot::MelanobotError("InlineQueryResponse created with the wrong kind of message");

        inline_query_id = msg.params[0];
    }


    InlineQueryResponse& result(pointer result)
    {
        if ( results.size() >= 50 )
            throw melanobot::MelanobotError("Too many inline results");
        results.push_back(std::move(result));
        return *this;
    }

    template<class ResultT, class... Args>
        InlineQueryResponse& result(Args&&... args)
    {
        return result(std::make_unique<ResultT>(std::forward(args)...));
    }

    PropertyTree to_properties() const
    {
        PropertyTree ptree;
        PropertyTree treeresults;
        for ( const auto& result : results )
            treeresults.push_back({"", result->to_properties()});
        ptree.put_child("results", treeresults);
        ptree.put("inline_query_id", inline_query_id);
        if ( cache_time >= 0 )
            ptree.put("cache_time", cache_time);
        ptree.put("is_personal", is_personal);
        ptree.put("next_offset", next_offset);
        ptree.put("switch_pm_text", switch_pm_text);
        ptree.put("switch_pm_parameter", switch_pm_parameter);

        return ptree;
    }

private:
    std::vector<pointer> results;
    std::string inline_query_id;
    int cache_time;
    bool is_personal;
    std::string next_offset;
    std::string switch_pm_text;
    std::string switch_pm_parameter;
};


class InlineQueryResultPhoto : public InlineQueryResult
{
public:
    explicit InlineQueryResultPhoto(
        std::string url
    ) : url(url)
    {
    }

    PropertyTree to_properties() const override
    {
        PropertyTree ptree;
        ptree.put("photo_url", url);
        ptree.put("type", "photo");
        return ptree;
    }

private:
    std::string url;
};

} // namespace telegram

#endif // MELANOBOT_MODULES_TELEGRAM_INLINE_HPP

