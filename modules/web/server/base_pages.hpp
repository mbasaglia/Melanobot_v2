/**
 * \file
 *
 * \author Mattia Basaglia
 *
 * \copyright Copyright  Mattia Basaglia
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
#ifndef MELANOBOT_MODULE_WEB_BASE_PAGES_HPP
#define MELANOBOT_MODULE_WEB_BASE_PAGES_HPP

#include "../aliases.hpp"
#include "settings.hpp"
#include "string/logger.hpp"

namespace web {

class HttpServer;

/**
 * \brief Exception you can throw to make the server generate a proper
 *        error response
 */
class HttpError : public std::runtime_error
{
public:
    HttpError(Status status)
        : runtime_error(status.message),
          _status(std::move(status))
    {}

    const Status& status() const
    {
        return _status;
    }

private:
    Status _status;
};

/**
 * \brief Base class for HTTP server error handlers
 */
class ErrorPage
{
public:
    virtual ~ErrorPage() {}

    virtual bool matches(const Status& status, const Request& request) const
    {
        return status.is_error();
    }

    virtual Response respond(const Status& status, Request& request, const HttpServer& sv) const = 0;

    static Response canned_response(const Status& status, const httpony::Protocol& protocol)
    {
        httpony::Response response("text/plain", status, protocol);
        response.body << response.status.message << '\n';
        return response;
    }
};

/**
 * \brief Base class for HTTP server page handlers
 */
class WebPage
{
public:
    struct RequestItem
    {
        RequestItem(Request& request, const HttpServer& server)
            : request(request), path(request.uri.path), server(server)
        {}

        RequestItem(Request& request, UriPathSlice slice, const HttpServer& server)
            : request(request), path(slice), server(server)
        {}

        RequestItem descend(const UriPath& prefix) const
        {
            return RequestItem(request, path.left_stripped(prefix.size()), server);
        }

        UriPath base_path() const
        {
            return path.strip_path_suffix(request.uri.path).to_path();
        }

        Request& request;
        UriPathSlice path;
        const HttpServer& server;
    };


    virtual ~WebPage() {}

    virtual bool matches(const RequestItem& request) const
    {
        return true;
    }

    virtual Response respond(const RequestItem& request) const = 0;

protected:
    UriPath read_uri(const Settings& settings, const std::string& default_value = "") const
    {
        return read_uri("uri", settings, default_value);
    }

    UriPath read_uri(const std::string& name, const Settings& settings, const std::string& default_value = "") const
    {
        return melanolib::string::char_split(settings.get(name, default_value), '/');
    }
};

/**
 * \brief Singleton to register page types for the config
 */
class PageRegistry : public melanolib::Singleton<PageRegistry>
{
public:
    template<class ErrorPageT>
        std::enable_if_t<std::is_base_of<ErrorPage, ErrorPageT>::value>
        register_page(const std::string& name)
        {
            error_page_types[name] = [](const Settings& settings) {
                return melanolib::New<ErrorPageT>(settings);
            };
        }

    template<class WebPageT>
        std::enable_if_t<std::is_base_of<WebPage, WebPageT>::value>
        register_page(const std::string& name)
        {
            web_page_types[name] = [](const Settings& settings) {
                return melanolib::New<WebPageT>(settings);
            };
        }

    std::unique_ptr<ErrorPage> build_error_page(const std::string& name, const Settings& settings) const
    {
        auto iter = error_page_types.find(name);
        if ( iter != error_page_types.end() )
            return iter->second(settings);
        return {};
    }

    std::unique_ptr<WebPage> build_web_page(const std::string& name, const Settings& settings) const
    {
        auto iter = web_page_types.find(name);
        if ( iter != web_page_types.end() )
            return iter->second(settings);
        return {};
    }

private:
    PageRegistry(){}
    friend ParentSingleton;
    std::map<std::string, std::function<std::unique_ptr<ErrorPage>(const Settings&)>> error_page_types;
    std::map<std::string, std::function<std::unique_ptr<WebPage>(const Settings&)>> web_page_types;
};

/**
 * \brief Base class for nested http request handlers
 */
class HttpRequestHandler
{
public:
    /**
     * \brief Finds a response for the given request and suggested status
     */
    Response respond(const WebPage::RequestItem& request,
                     const Status& status) const
    {
        Response response;
        if ( status.is_error() )
            response = handle_error(request, status, 0);
        else
            response = get_response(request);
        return response;
    }

protected:
    void load_pages(const Settings& settings)
    {
        for ( const auto& page : settings )
        {
            if ( page.first.empty() || !melanolib::string::ascii::is_upper(page.first[0]) )
                continue;
            else if ( auto page_ptr = PageRegistry::instance().build_web_page(page.first, page.second) )
                web_pages.push_back(std::move(page_ptr));
            else if ( auto err_ptr = PageRegistry::instance().build_error_page(page.first, page.second) )
                error_pages.push_back(std::move(err_ptr));
            else
                ErrorLog("wsv") << "Unknown page type: " << page.first;
        }
    }

private:
    /**
     * \brief Finds the right handler for the given error condition.
     *
     * If more than \p max_error_depth errors happen during error handling,
     * a built-in response is generated
     */
    Response handle_error(const WebPage::RequestItem& request,
                          const Status& status,
                          int depth) const
    {
        if ( depth < max_error_depth )
        {
            try
            {
                for ( const auto& ep : error_pages )
                {
                    if ( ep->matches(status, request.request) )
                        return ep->respond(status, request.request, request.server);
                }
                return ErrorPage::canned_response(status, request.request.protocol);
            }
            catch ( const HttpError& error )
            {
                if ( error.status() != status )
                    return handle_error(request, error.status(), depth + 1);
            }
            catch ( const std::exception& error )
            {
                if ( status != httpony::StatusCode::InternalServerError )
                    return handle_error(request, httpony::StatusCode::InternalServerError, depth + 1);
            }
        }
        return unhandled_error(request, status);
    }

    /**
     * \brief Finds a response for the given request
     */
    Response get_response(const WebPage::RequestItem& request) const
    {
        try
        {
            for ( const auto& page : web_pages )
            {
                if ( page->matches(request) )
                    return page->respond(request);
            }
            return handle_error(request, httpony::StatusCode::NotFound, 0);
        }
        catch ( const HttpError& error )
        {
            return handle_error(request, error.status(), 0);
        }
        catch ( const std::exception& error )
        {
            return handle_error(request, httpony::StatusCode::InternalServerError, 0);
        }
    }

    virtual Response unhandled_error(
        const WebPage::RequestItem& request,
        const Status& status) const
    {
        throw HttpError(status);
    }

    std::vector<std::unique_ptr<WebPage>> web_pages;
    std::vector<std::unique_ptr<ErrorPage>> error_pages;
    int max_error_depth = 2;
};

} // namespace web
#endif // MELANOBOT_MODULE_WEB_BASE_PAGES_HPP
