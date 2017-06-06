/**
 * \file
 *
 * \author Mattia Basaglia
 *
 * \copyright Copyright (C) 2017 Mattia Basaglia
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include "telegram-connection.hpp"

#include <boost/property_tree/json_parser.hpp>

#include "httpony/formats/json.hpp"
#include "web/client/http.hpp"
#include "melanobot/storage.hpp"

namespace telegram {

std::unique_ptr<TelegramConnection> TelegramConnection::create(
    const Settings& settings,
    const std::string& name
)
{
    if ( settings.get("protocol", std::string()) != "telegram" )
    {
        throw melanobot::ConfigurationError("Wrong protocol for a Telegram connection");
    }

    std::string api_base = settings.get("endpoint", "https://api.telegram.org/");
    std::string token = settings.get("token", "");
    if ( token.empty() )
        throw melanobot::ConfigurationError("Missing Telegram bot token");

    return melanolib::New<TelegramConnection>(api_base, token, settings, name);
}

TelegramConnection::TelegramConnection(const std::string& api_endpoint,
                                       const std::string& token,
                                       const Settings& settings,
                                       const std::string& name)
    : AuthConnection(name), PushReceiver(name, settings, name + token)
{
    log_errors_callback = [this](const PropertyTree& tree){ log_errors(tree); };

    api_base = api_endpoint + "/bot" + token;
    properties_.put("api.endpoint", api_endpoint);
    properties_.put("api.token", token);

    formatter_ = string::Formatter::formatter(
        settings.get("string_format", std::string("telegram-md"))
    );
    connection_status = DISCONNECTED;

    webhook = settings.get("webhook", webhook);
    webhook_url = settings.get("webhook_url", webhook_url);
    webhook_max_connections = settings.get("webhook_max_connections", webhook_max_connections);

    if ( !webhook )
    {
        polling_timer = melanolib::time::Timer(
            [this]{poll();},
            melanolib::time::seconds(settings.get("polling_time", 15))
        );
    }

    setup_auth(settings);
}

TelegramConnection::~TelegramConnection()
{
    stop();
}

network::Server TelegramConnection::server() const
{
    uint16_t port;

    if ( api_base.authority.port )
        port = *api_base.authority.port;
    else if ( api_base.scheme == "https" )
        port = 443;
    else
        port = 80;

    return network::Server(api_base.authority.host, port);
}

std::string TelegramConnection::description() const
{
    auto lock = make_lock(mutex);
    return properties_.get("api.endpoint", "");
}

void TelegramConnection::command(network::Command cmd)
{
    if ( cmd.command.empty() )
        return;

    if ( cmd.command == "getMe" )
    {
        get("getMe", [this](const PropertyTree& response){
            if ( response.get("ok", false) )
            {
                auto lock = make_lock(mutex);
                properties_.put_child("user", response.get_child("result"));
            }
        });
        return;
    }

    PropertyTree payload;

    if ( cmd.command == "forwardMessage" && cmd.parameters.size() == 3 )
    {
        payload.put("chat_id", cmd.parameters[0]);
        payload.put("from_chat_id", cmd.parameters[1]);
        payload.put("message_id", cmd.parameters[2]);
    }
    else if ( cmd.parameters.size() >= 2 &&
        melanolib::string::is_one_of(cmd.command,
            {"sendPhoto", "sendAudio", "sendDocument", "sendSticker",
             "sendVideo", "sendVoice",
            }
        )
    )
    {
        auto what = melanolib::string::strtolower(cmd.command.substr(4));
        payload.put("chat_id", cmd.parameters[0]);
        payload.put(what, cmd.parameters[1]);
        if ( cmd.parameters.size() > 2 )
            payload.put("caption", cmd.parameters[2]);
        if ( cmd.parameters.size() > 3 )
            payload.put("reply_to_message_id", cmd.parameters[3]);
    }
    else if ( cmd.parameters.size() >= 3 &&
        melanolib::string::starts_with(cmd.command,
            {"sendLocation", "sendVenue"}
        )
    )
    {
        payload.put("chat_id", cmd.parameters[0]);
        payload.put("latitude", cmd.parameters[1]);
        payload.put("longitude", cmd.parameters[2]);
        if ( cmd.parameters.size() > 3 )
            payload.put("caption", cmd.parameters[3]);
        if ( cmd.parameters.size() > 4 )
            payload.put("reply_to_message_id", cmd.parameters[4]);
    }
    else if ( cmd.parameters.size() >= 3 && cmd.command == "sendContact" )
    {
        payload.put("chat_id", cmd.parameters[0]);
        payload.put("phone_number", cmd.parameters[1]);
        payload.put("first_name", cmd.parameters[2]);
        if ( cmd.parameters.size() > 3 )
            payload.put("caption", cmd.parameters[3]);
        if ( cmd.parameters.size() > 4 )
            payload.put("reply_to_message_id", cmd.parameters[4]);
    }
    else if ( cmd.command == "sendChatAction" && cmd.parameters.size() == 2 )
    {
        payload.put("chat_id", cmd.parameters[0]);
        payload.put("action", cmd.parameters[1]);
    }
    else if ( cmd.command == "kickChatMember" && cmd.parameters.size() == 2 )
    {
        payload.put("chat_id", cmd.parameters[0]);
        payload.put("user_id", cmd.parameters[1]);
    }
    else if ( cmd.command == "leaveChat" && cmd.parameters.size() == 1 )
    {
        payload.put("chat_id", cmd.parameters[0]);
    }
    else if ( cmd.command == "unbanChatMember" && cmd.parameters.size() == 2 )
    {
        payload.put("chat_id", cmd.parameters[0]);
        payload.put("user_id", cmd.parameters[1]);
    }
    else
    {
        ErrorLog("telegram") << "Command not supported: " << cmd.command;
    }

    post(cmd.command, payload, log_errors_callback);
}

void TelegramConnection::say(const network::OutputMessage& message)
{
    string::FormattedString str;
    if ( !message.prefix.empty() )
        str << message.prefix << ' ' << color::nocolor;
    if ( !message.from.empty() )
    {
        if ( message.action )
            str << "* " << message.from << ' ';
        else
            str << '<' << message.from << color::nocolor << "> ";
    }
    str << message.message;
    PropertyTree payload;
    /// \todo somehow encode reply_to on target
    payload.put("chat_id", message.target);
    payload.put("text", str.encode(*formatter_));
    payload.put("parse_mode", "Markdown");
    Log("telegram", '>') << color::magenta << message.target
        << color::nocolor << ' ' << str;
    post("sendMessage", payload, log_errors_callback);
}

void TelegramConnection::get(const std::string& method,
                             const ApiCallback& callback,
                             const ErrorCallback& on_error)
{
    request(web::Request("GET", api_uri(method)), callback, on_error);
}

void TelegramConnection::post(const std::string& method,
                              const PropertyTree& payload,
                              const ApiCallback& callback,
                              const ErrorCallback& on_error)
{
    web::Request web_request("POST", api_uri(method));
    web_request.body.start_output("application/json");
    boost::property_tree::write_json(web_request.body, payload, false);
    request(std::move(web_request), callback, on_error);
}

void TelegramConnection::log_errors(const PropertyTree& response) const
{
    if ( !response.get("ok", false) )
    {
        ErrorLog("telegram") << response.get("description", "Unknown error");
    }
}

web::Uri TelegramConnection::api_uri(const std::string& method) const
{
    auto uri = api_base;
    uri.path /= method;
    return uri;
}

void TelegramConnection::request(web::Request&& request,
                                 const ApiCallback& callback,
                                 const ErrorCallback& on_error)
{
    web::HttpClient::instance().async_query(
        std::move(request),
        [callback](web::Request& request, web::Response& response){
            try
            {
                auto content = httpony::json::JsonParser().parse(response.body);
                callback(content);
            }
            catch(httpony::json::JsonError&)
            {
                ErrorLog("telegram") << "Malformed response";
            }
        },
        [on_error](const web::Request&, const web::OperationStatus&){
            on_error();
        }
    );
}

network::Connection::Status TelegramConnection::status() const
{
    return connection_status;
}

std::string TelegramConnection::protocol() const
{
    return "telegram";
}


std::string TelegramConnection::storage_key() const
{
    auto lock = make_lock(mutex);
    return "telegram." + properties_.get("api.token", "unknown");
}

void TelegramConnection::connect()
{
    if ( melanobot::has_storage() )
    {
        event_id = melanolib::string::to_uint(
            melanobot::storage().maybe_get_value(storage_key() + ".event_id", "0")
        );
    }

    connection_status = CONNECTING;

    std::function<void (const PropertyTree&)> on_success;


    if ( webhook )
    {
        on_success = [this](const PropertyTree&){
            command(network::Command("getMe"));
        };
    }
    else
    {
        on_success = [this](const PropertyTree& response)
        {
            polling_timer.start();
            auto lock = make_lock(mutex);
            properties_.put_child("user", response.get_child("result"));
        };
    }

    auto on_error = [this]()
    {
        polling_timer.stop();
        connection_status = DISCONNECTED;
        ErrorLog("telegram") << "Could not connect: Network error";
    };

    auto on_connect = [this, on_success](const PropertyTree& response)
    {
        if ( response.get("ok", false) )
        {
            connection_status = CONNECTED;
            if ( on_success )
                on_success(response);
        }
        else
        {
            polling_timer.stop();
            connection_status = DISCONNECTED;
            ErrorLog("telegram") << "Could not connect:"
                << response.get("description", "Unknown error");
        }
    };

    if ( webhook )
    {
        PropertyTree props;
        props.put("url", webhook_url);
        props.put("max_connections", webhook_max_connections);
        post("setWebhook", props, on_connect, on_error);
    }
    else
    {
        get("getMe", on_connect, on_error);
    }

}

void TelegramConnection::disconnect(const string::FormattedString&)
{
    connection_status = DISCONNECTED;
    polling_timer.stop();
    post("deleteWebhook", {}, log_errors_callback);
}

void TelegramConnection::reconnect(const string::FormattedString&)
{
    connect();
}

std::string TelegramConnection::name() const
{
    auto lock = make_lock(mutex);
    return properties_.get("user.username", "");
}

LockedProperties TelegramConnection::properties()
{
    return LockedProperties(&mutex, &properties_);
}

string::FormattedProperties TelegramConnection::pretty_properties() const
{
    auto lock = make_lock(mutex);
    return string::FormattedProperties{
        {"bot_username",    properties_.get("user.username", "")},
        {"bot_first_name",  properties_.get("user.first_name", "")},
        {"bot_id",          properties_.get("user.id", "")},
    };
}

void TelegramConnection::poll()
{
    auto& web_client = web::HttpClient::instance();
    web::Uri uri = api_uri("getUpdates");

    uri.query["offset"] = std::to_string(event_id);

    auto timeout = std::chrono::duration_cast<std::chrono::seconds>(
        polling_timer.timeout_duration()
    ).count();

    if ( web_client.timeout() )
        timeout = std::min(timeout, web_client.timeout()->count() - 1);

    uri.query["timeout"] = std::to_string(timeout);

    web::Response resp;
    auto status = web::HttpClient::instance().query(web::Request("GET", uri), resp);

    if ( status.error() )
    {
        ErrorLog("telegram") << "Could not fetch updates: Network error";
        connect();
        return;
    }

    process_events(resp.body.input());
}

user::User TelegramConnection::user_attributes(const PropertyTree& user) const
{
    std::string first_name = user.get("first_name", "");
    std::string last_name = user.get("last_name", "");
    std::string full_name = first_name;
    if ( !first_name.empty() && !last_name.empty() )
        full_name += " ";
    full_name += last_name;

    std::string userid = user.get("id", "");
    std::string username = user.get("username", "");

    if ( username.empty() )
    {
        username = 'u' + user.get("id", "");
        if ( full_name.empty() )
            full_name = userid;
    }
    else
    {
        if ( full_name.empty() )
            full_name = username;
        username = "@" + username;
    }

    return user::User(full_name, "", username, userid, const_cast<TelegramConnection*>(this));

}

web::Response TelegramConnection::receive_push(const RequestItem& request)
{
    process_events(request.request.body.input());
    return web::Response();
}

void TelegramConnection::process_event(PropertyTree& event)
{
    static const std::regex regex_command(
        R"((/.+)@(\w+)(.*))",
        std::regex::ECMAScript|std::regex::optimize
    );
    event_id = event.get("update_id", event_id.load());
    if ( auto message = event.get_child_optional("message") )
    {
        network::Message msg;
        std::istringstream socket_stream(msg.raw);

        msg.chat(message->get<std::string>("text"));
        msg.direct = false;

        std::smatch match;
        if ( std::regex_match(msg.message, match, regex_command) )
        {
            if ( match[2] == properties_.get("user.username", "") )
            {
                msg.message = match[1];
                msg.message += match[3];
                msg.direct = true;
            }
        }

        msg.from = user_attributes(message->get_child("from"));
        msg.channels = {message->get("chat.id", "")};
        Log("telegram", '<', 1) << color::magenta << msg.from.name
            << color::nocolor << ' ' << msg.message;
        msg.direct = message->get("chat.type", "") == "private";
        msg.send(this);
    }
    ++event_id;
}

void TelegramConnection::process_events(httpony::io::InputContentStream& body)
{
    PropertyTree content;
    try
    {
        content = httpony::json::JsonParser().parse(body);
    }
    catch(httpony::json::JsonError&)
    {
        ErrorLog("telegram") << "Malformed event data";
        return;
    }

    try
    {
        if ( content.count("update_id") )
        {
            process_event(content);
        }
        else
        {
            if ( !content.get("ok", false) )
            {
                log_errors(content);
                return;
            }

            for ( auto pt : content.get_child("result") )
            {
                process_event(pt.second);
            }
        }
    }
    catch(const std::exception& e)
    {
        ErrorLog("telegram") << "Error processing event " << event_id.load();
    }

    melanobot::storage().put(storage_key() + ".event_id", std::to_string(event_id));
}

user::User TelegramConnection::build_user(const std::string& local_id) const
{
    if ( local_id.empty() )
        return {""};

    if ( local_id[0] == '@' )
        return get_user(local_id);

    std::string global_id = local_id;
    if ( local_id[0] == 'u' )
        global_id.erase(0, 1);
    else if ( !melanolib::string::ascii::is_digit(local_id[0]) )
        return get_user('@' + local_id);

    auto lock = make_lock(mutex);
    if ( const user::User* user = user_manager.global_user(global_id) )
        return *user;

    return {local_id, "", local_id, global_id};
}

} // namespace telegram
