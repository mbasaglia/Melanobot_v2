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


#include "melanobot/handler.hpp"
#include "telegram-connection.hpp"

namespace telegram {

class PropertyBuilder : public PropertyTree
{
public:
    using PropertyTree::PropertyTree;

    void maybe_put(const std::string& name, const std::string& value)
    {
        if ( !value.empty() )
            put(name, value);
    }

    void maybe_put(const std::string& name, int value, int min = 0)
    {
        if ( value >= min )
            put(name, value);
    }

};

class InlineQueryResult
{
public:
    virtual ~InlineQueryResult(){}
    virtual PropertyBuilder to_properties() const = 0;
};

class InlineQueryResponse
{
public:
    using pointer = std::unique_ptr<InlineQueryResult>;

    explicit InlineQueryResponse(
            std::string inline_query_id,
            int cache_time = -1,
            bool is_personal = false,
            std::string next_offset = {},
            std::string switch_pm_text = {},
            std::string switch_pm_parameter = {}
    ) : inline_query_id(std::move(inline_query_id)),
        cache_time(cache_time),
        is_personal(is_personal),
        next_offset(std::move(next_offset)),
        switch_pm_text(std::move(switch_pm_text)),
        switch_pm_parameter(std::move(switch_pm_parameter))
    {
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
        return result(std::make_unique<ResultT>(std::forward<Args>(args)...));
    }

    PropertyBuilder to_properties() const
    {
        PropertyBuilder ptree;
        PropertyTree treeresults;
        for ( const auto& result : results )
            treeresults.push_back({"", result->to_properties()});
        ptree.put_child("results", treeresults);
        ptree.put("inline_query_id", inline_query_id);
        ptree.maybe_put("cache_time", cache_time);
        ptree.put("is_personal", is_personal);
        ptree.maybe_put("next_offset", next_offset);
        ptree.maybe_put("switch_pm_text", switch_pm_text);
        ptree.maybe_put("switch_pm_parameter", switch_pm_parameter);

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

/**
 * \brief Just to allow simpler initialization and attribute access
 *        while maintaining a virtual destructor
 */
template<class DataT>
    class SimpleDataInlineQueryResult : public InlineQueryResult
{
public:
    template<class... Args>
    SimpleDataInlineQueryResult(Args&&... args)
        : data({std::forward<Args>(args)...})
    {}

    PropertyBuilder to_properties() const override
    {
        return data.to_properties();
    }

    DataT data;
};

struct PhotoData
{
    std::string photo_url;
    std::string thumb_url;
    int photo_width = 0;
    int photo_height = 0;
    std::string title;
    std::string description;
    std::string parse_mode;
    // reply_markup
    // input_message_content

    PropertyBuilder to_properties() const
    {
        PropertyBuilder ptree;
        ptree.put("photo_url", photo_url);
        ptree.put("type", "photo");
        ptree.maybe_put("thumb_url", thumb_url);
        ptree.maybe_put("photo_width", photo_width, 1);
        ptree.maybe_put("photo_height", photo_height, 1);
        ptree.maybe_put("title", title);
        ptree.maybe_put("description", description);
        ptree.maybe_put("parse_mode", parse_mode);
        return ptree;
    }
};

using InlineQueryResultPhoto = SimpleDataInlineQueryResult<PhotoData>;


class InlineHandler : public melanobot::Handler
{
public:
    InlineHandler(const Settings& settings, MessageConsumer* parent)
        : Handler(settings, parent)
    {}

    bool on_handle(network::Message& msg) override
    {
        TelegramConnection* connection = dynamic_cast<TelegramConnection*>(msg.source);
        if ( !connection )
            throw melanobot::MelanobotError("Invalid telegram connection");
        auto response = on_handle_query(msg, connection, msg.message, msg.params[0], msg.params[1]);
        connection->post("answerInlineQuery", response.to_properties(), {});
        return true;
    }

    bool can_handle(const network::Message& msg) const override
    {
        return msg.type == network::Message::UNKNOWN &&
            msg.command == "inline_query" &&
            msg.source == msg.destination &&
            msg.source->protocol() == "telegram" &&
            msg.params.size() >= 2
        ;
    }

protected:
    virtual InlineQueryResponse on_handle_query(
        const network::Message& msg,
        TelegramConnection* connection,
        const std::string& query,
        const std::string& query_id,
        const std::string& offset
    ) const = 0;
};

/**
 * \brief Generates picture urls based on the queries
 */
class InlinePhotoUrl : public InlineHandler
{
private:
    class PhotoUriDescription
    {
    public:
        PhotoUriDescription(std::string base, std::string param)
            : base(std::move(base)),
              param(std::move(param)),
              has_query(this->base.find('?') != std::string::npos)
        {}

        std::string full_uri(const std::string& query) const
        {
            return base + (has_query ? '&' : '?') + param + "=" + httpony::urlencode(query);
        }

    private:
        std::string base;
        std::string param;
        bool has_query = false;
    };

public:
    InlinePhotoUrl(const Settings& settings, MessageConsumer* parent)
        : InlineHandler(settings, parent)
    {
        for ( const auto& photo : settings.get_child("photos", {}) )
        {
            photos.push_back({photo.first, photo.second.data()});
        }
    }

private:
    InlineQueryResponse on_handle_query(
        const network::Message& msg,
        TelegramConnection* connection,
        const std::string& query,
        const std::string& query_id,
        const std::string& offset
    ) const override
    {
        InlineQueryResponse resp(query_id);
        for ( const auto& photo : photos )
        {
            resp.result<InlineQueryResultPhoto>(photo.full_uri(query));
        }
        return resp;
    }

    std::vector<PhotoUriDescription> photos;
};

} // namespace telegram

#endif // MELANOBOT_MODULES_TELEGRAM_INLINE_HPP

