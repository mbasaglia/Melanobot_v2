/**
 * \file
 *
 * \author Mattia Basaglia
 *
 * \copyright Copyright 2016 Mattia Basaglia
 *
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
#ifndef MELANOBOT_MODULE_WEB_PAGES_HPP
#define MELANOBOT_MODULE_WEB_PAGES_HPP

#include <boost/filesystem.hpp>
#include "server.hpp"

namespace web {

/**
 * \brief Web page handler rendering files in a directory on disk
 */
class RenderStatic : public WebPage
{
public:
    explicit RenderStatic(const Settings& settings)
    {
        directory = settings.get("directory", "");
        if ( directory.empty() || !boost::filesystem::is_directory(directory) )
            throw melanobot::ConfigurationError("Invalid path: " + directory.string());

        uri = read_uri(settings, "static");

        default_mime_type = settings.get("default_mime_type", default_mime_type.string());

        for ( const auto& mimes : settings.get_child("Mime", {}) )
            extension_to_mime[mimes.first] = mimes.second.data();

        /// \todo blacklist patterns
    }

    bool matches(const RequestItem& request) const override
    {
        return request.path.match_prefix(uri) && boost::filesystem::is_regular(full_path(request.path));
    }

    Response respond(const RequestItem& request) const override
    {
        auto file_path = full_path(request.path);
        std::ifstream input(file_path.string());
        if ( !input.is_open() )
            throw HttpError(StatusCode::NotFound);
        Response response(mime(file_path), StatusCode::OK, request.request.protocol);
        std::array<char, 1024> chunk;
        while ( input.good() )
        {
            input.read(chunk.data(), chunk.size());
            response.body.write(chunk.data(), input.gcount());
        }

        return response;
    }

protected:
    const MimeType& mime(const boost::filesystem::path& path) const
    {
        auto it = extension_to_mime.find(path.extension().string());
        if ( it != extension_to_mime.end() )
            return it->second;
        return default_mime_type;
    }

    boost::filesystem::path full_path(const UriPathSlice& path) const
    {
        auto file_path = directory;
        for ( const auto& dir : path.left_stripped(uri.size()) )
            file_path /= dir;
        return file_path;
    }

private:
    boost::filesystem::path directory;
    UriPath uri;
    std::unordered_map<std::string, MimeType> extension_to_mime;
    MimeType default_mime_type = "application/octet-stream";
};

/**
 * \brief Renders a fixed file
 */
class RenderFile : public WebPage
{
public:
    explicit RenderFile(const Settings& settings)
    {
        file_path = settings.get("path", file_path);
        uri = read_uri(settings);

        mime_type = settings.get("mime_type", mime_type.string());
    }

    bool matches(const RequestItem& request) const override
    {
        return request.path.match_exactly(uri);
    }

    Response respond(const RequestItem& request) const override
    {
        std::ifstream input(file_path);
        if ( !input.is_open() )
            throw HttpError(StatusCode::InternalServerError);
        Response response(mime_type, StatusCode::OK, request.request.protocol);
        std::array<char, 1024> chunk;
        while ( input.good() )
        {
            input.read(chunk.data(), chunk.size());
            response.body.write(chunk.data(), input.gcount());
        }

        return response;
    }

private:
    std::string file_path;
    UriPath uri;
    MimeType mime_type = "application/octet-stream";
};

/**
 * \todo Allow regexes and parametrized expansions
 * \todo Add configurable headers to the response
 */
class Redirect : public WebPage
{
public:
    explicit Redirect(const Settings& settings)
    {
        destination = settings.get("destination", destination);
        uri = read_uri(settings);
        status = settings.get<Status>("status", StatusCode::Found);
    }

    bool matches(const RequestItem& request) const override
    {
        return request.path.match_exactly(uri);
    }

    Response respond(const RequestItem& request) const override
    {
        return Response::redirect(destination, status);
    }

private:
    std::string destination;
    UriPath uri;
    Status status;
};

/**
 * \brief Groups pages under a common prefix
 */
class PageDirectory : public HttpRequestHandler, public WebPage
{
public:
    explicit PageDirectory(const Settings& settings)
    {
        uri = read_uri(settings);
        load_pages(settings);
        auto range = settings.equal_range("verified_client");
        for ( ; range.first != range.second; ++range.first )
            verified_clients.push_back(range.first->second.data());
    }

    bool matches(const RequestItem& request) const override
    {
        return request.path.match_prefix(uri) && verified(request.request);
    }

    Response respond(const RequestItem& request) const override
    {
        return HttpRequestHandler::respond(request.descend(uri), StatusCode::OK);
    }

private:
    bool verified(const Request& request) const
    {
        return verified_clients.empty() ||
            std::find(verified_clients.begin(), verified_clients.end(),
                httpony::ssl::SslAgent::get_cert_common_name(request.connection.socket()))
            != verified_clients.end();
    }

    UriPath uri;
    std::vector<std::string> verified_clients;
};

/**
 * \brief Renders an error page using HTML
 */
class HtmlErrorPage : public ErrorPage
{
public:
    explicit HtmlErrorPage(const Settings& settings)
    {
        css_file = settings.get("css", css_file);
        extra_info = settings.get("extra_info", extra_info);
    }

    Response respond(const Status& status, Request& request, const HttpServer& sv) const
    {
        using namespace httpony::quick_xml;
        using namespace httpony::quick_xml::html;
        Response response("text/html", status, request.protocol);

        HtmlDocument document("Error " + melanolib::string::to_string(status.code));
        if ( !css_file.empty() )
        {
            document.head().append(Element("link", Attributes{
                {"rel", "stylesheet"}, {"type", "text/css"}, {"href", css_file}
            }));
        }

        document.body().append(Element("h1", Text(status.message)));

        std::string reply = "The URL " + request.uri.path.url_encoded(true) + " ";
        if ( status == StatusCode::NotFound )
            reply += "was not found.";
        else if ( status.type() == httpony::StatusType::ClientError )
            reply += "has not been accessed correctly.";
        else if ( status.type() == httpony::StatusType::ServerError )
            reply += "caused a server error.";
        else
            reply += "caused an unknown error";
        document.body().append(Element("p", Text(reply)));

        if ( !extra_info.empty() )
        {
            document.body().append(Element("p", Text(
                sv.format_info(extra_info, request, response)
            )));
        }

        response.body << document << '\n';
        return response;
    }

private:
    std::string css_file;
    std::string extra_info;
};

/**
 * \brief Web page showing an overview of the bot status
 */
class StatusPage : public WebPage
{
public:
    explicit StatusPage(const Settings& settings);
    ~StatusPage();

    bool matches(const RequestItem& request) const override
    {
        return request.path.match_prefix(uri);
    }

    Response respond(const RequestItem& request) const override;

private:
    UriPath uri;
    std::string css_file;
    std::string template_path;
    std::vector<std::unique_ptr<SubPage>> sub_pages;
    bool editable = false;
};

} // namespace web
#endif // MELANOBOT_MODULE_WEB_PAGES_HPP
