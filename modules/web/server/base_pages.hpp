/**
 * \file
 *
 * \author Mattia Basaglia
 *
 * \copyright Copyright 2015-2017 Mattia Basaglia
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

struct Html {};

/**
 * \brief Returns a typesystem with useful types ready to expose bot data
 */
melanolib::scripting::TypeSystem& scripting_typesystem();

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

        RequestItem ascend(const UriPath& suffix) const
        {
            UriPathSlice upper_path(
                melanolib::math::max(
                    request.uri.path.cbegin(),
                    path.begin() - suffix.size()
                ),
                path.end()
            );
            return RequestItem(request, upper_path, server);
        }

        UriPath base_path() const
        {
            return path.strip_path_suffix(request.uri.path).to_path();
        }

        UriPath full_path() const
        {
            return request.uri.path;
        }

        Request& request;
        UriPathSlice path;
        const HttpServer& server;
        const httpony::DataMap& get = request.uri.query;
        const httpony::DataMap& post = request.post;
        const httpony::DataMap& cookies = request.cookies;
    };


    virtual ~WebPage() {}

    virtual bool matches(const RequestItem& request) const
    {
        return true;
    }

    virtual Response respond(const RequestItem& request) const = 0;

    static UriPath read_uri(const Settings& settings, const std::string& default_value = "")
    {
        return read_uri("uri", settings, default_value);
    }

    static UriPath read_uri(const std::string& name, const Settings& settings, const std::string& default_value = "")
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

    void add_web_page(std::unique_ptr<WebPage> page)
    {
        web_pages.push_back(std::move(page));
    }

protected:
    void load_pages(const Settings& settings)
    {
        for ( const auto& page : settings )
        {
            if ( page.first.empty() || !melanolib::string::ascii::is_upper(page.first[0]) )
                continue;
            else if ( auto page_ptr = PageRegistry::instance().build_web_page(page.first, page.second) )
                add_web_page(std::move(page_ptr));
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
                ErrorLog("wsv") << "Exception: " << error.what();
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
            ErrorLog("wsv") << "Exception: " << error.what();
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

/**
 * \brief A sub-page within a page handler
 */
class SubPage
{
public:
    using PathSuffix = UriPathSlice;
    using RequestItem = WebPage::RequestItem;
    using Context = melanolib::scripting::Object;

    SubPage(
        std::string name,               ///< Name to show in menus and titles
        UriPath path,                   ///< Sub-path
        std::string page_template,      ///< Template used to render the page
        std::string menu_template = "", ///< Template used to render a sub-menu
        bool show_on_menu = true        ///< Whether it should be visible on a menu
    ) : _name(std::move(name)),
        _path(std::move(path)),
        _page_template(std::move(page_template)),
        _menu_template(std::move(menu_template)),
        _menu(show_on_menu)
    {}

    virtual ~SubPage(){}

    /**
     * \brief Whether the page shall be selected for the request
     */
    virtual bool matches(const RequestItem& request) const
    {
        return request.path.match_exactly(_path);
    }

    /**
     * \brief Name to show in menus and titles
     */
    const std::string& name() const
    {
        return _name;
    }

    /**
     * \brief Sub-path
     */
    const UriPath& path() const
    {
        return _path;
    }

    /**
     * \brief Whether it should be visible on a menu
     */
    bool show_on_menu() const
    {
        return _menu;
    }

    /**
     * \brief Prepares the context for rendering
     * \pre matches(request)
     * \return Nothing if it should be rendered normally, a response if
     *         it has to return a different response
     */
    virtual melanolib::Optional<Response> prepare(
        const RequestItem& request,
        Context& context
    ) const
    {
        return {};
    }

    /**
     * \brief Renders the submenu
     * \pre \p context contains page.template_path
     */
    std::string submenu(const Context& context) const
    {
        if ( _menu_template.empty() )
            return "";
        std::string template_path = context.get({"page", "template_path"}).to_string();
        return process_template(template_path, _menu_template, context);
    }

    /**
     * \brief Renders the page contents
     * \pre \p context contains page.template_path
     */
    std::string render(const Context& context) const
    {
        std::string template_path = context.get({"page", "template_path"}).to_string();
        return process_template(template_path, _page_template, context);
    }

    /**
     * \brief Renders a template file based on the context
     * \param template_path Path used to search templates
     * \param template_name Relative name of the template file
     * \param context       Context used for variable expansion
     */
    static std::string process_template(
        const std::string& template_path,
        const std::string& template_name,
        const Context& context
    )
    {
        std::ifstream template_file(template_path + '/' + template_name);
        template_file.unsetf(std::ios::skipws);
        auto template_str = string::FormatterConfig{}.decode(
            std::string(std::istream_iterator<char>(template_file), {})
        );
        template_str.replace(context);
        return template_str.encode(string::FormatterUtf8{});
    }

private:
    std::string _name;
    UriPath _path;
    std::string _page_template;
    std::string _menu_template;
    bool _menu;
};

} // namespace web
#endif // MELANOBOT_MODULE_WEB_BASE_PAGES_HPP
