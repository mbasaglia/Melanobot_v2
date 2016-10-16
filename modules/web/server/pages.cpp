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
#include "pages.hpp"
#include "config.hpp"
#include "melanobot/melanobot.hpp"


namespace web {

class Home : public SubPage
{
public:
    Home() : SubPage("Home", "", "status/home.html") {}

    melanolib::Optional<Response> prepare(
        const RequestItem& request,
        Context& context
    ) const override
    {
        auto& ts = context.type().type_system();
        auto project = ts.object<melanolib::scripting::SimpleType>();
        project.set("name", ts.object<std::string>(PROJECT_NAME));
        project.set("version", ts.object<std::string>(PROJECT_DEV_VERSION));
        context.set("project", project);
        context.set("compile", ts.object(settings::SystemInfo::compile_system()));
        context.set("runtime", ts.object(settings::SystemInfo::runtime_system()));
        context.set("compiler", ts.object<std::string>(SYSTEM_COMPILER));

        return {};
    }
};

class ConnectionList : public SubPage
{
public:
    ConnectionList()
        : SubPage(
            "Connections",
            "connection",
            "status/connection_list.html",
            "status/connection_menu.html"
        )
    {}
};


class ConnectionDetails : public SubPage
{
public:
    ConnectionDetails()
        : SubPage(
            "Connections",
            "connection",
            "status/connection_details.html",
            "status/connection_menu.html",
            false
        )
    {}

    bool matches(const RequestItem& request) const override
    {
        return request.path.match_prefix(path()) && request.path.size() >= 2;
    }

    melanolib::Optional<Response> prepare(
        const RequestItem& request,
        Context& context
    ) const override
    {
        network::Connection* conn = melanobot::Melanobot::instance().connection(request.path[1]);

        if ( !conn )
            throw HttpError(StatusCode::NotFound);

        if ( request.path.size() == 3 && context.get({"page", "editable"}).cast<bool>() )
        {
            if ( request.path[2] == "stop" )
            {
                conn->stop();
            }
            else if ( request.path[2] == "start" )
            {
                conn->stop();
                conn->start();
            }
            else
            {
                throw HttpError(StatusCode::NotFound);
            }

            return Response::redirect(request.full_path().parent().url_encoded(true));
        }
        else if ( request.path.size() != 2 )
        {
            throw HttpError(StatusCode::NotFound);
        }

        context.set("connection", context.type().type_system().reference(*conn));

        return {};
    }
};

class ServiceList : public SubPage
{
public:
    ServiceList()
        : SubPage(
            "Services",
            "service",
            "status/service_list.html",
            "status/service_menu.html"
        )
    {}
};


class ServiceDetails : public SubPage
{
public:
    ServiceDetails()
        : SubPage(
            "Services",
            "service",
            "status/service_details.html",
            "status/service_menu.html",
            false
        )
    {}

    bool matches(const RequestItem& request) const override
    {
        return request.path.match_prefix(path()) && request.path.size() >= 2;
    }

    melanolib::Optional<Response> prepare(
        const RequestItem& request,
        Context& context
    ) const override
    {
        AsyncService* service = nullptr;
        for ( const auto& svc : melanobot::Melanobot::instance().service_list() )
        {
            if ( std::to_string(uintptr_t(svc.get())) == request.path[1] )
            {
                service = svc.get();
                break;
            }
        }

        if ( !service )
            throw HttpError(StatusCode::NotFound);

        if ( request.path.size() == 3 && context.get({"page", "editable"}).cast<bool>() )
        {
            if ( request.path[2] == "stop" )
            {
                service->stop();
            }
            else if ( request.path[2] == "start" )
            {
                service->stop();
                service->start();
            }
            else
            {
                throw HttpError(StatusCode::NotFound);
            }

            return Response::redirect(request.full_path().parent().url_encoded(true));
        }
        else if ( request.path.size() != 2 )
        {
            throw HttpError(StatusCode::NotFound);
        }

        context.set("service", context.type().type_system().reference(*service));

        return {};
    }
};

class GlobalSettings : public SubPage
{
public:
    GlobalSettings()
        : SubPage("Global Settings", "settings", "status/global_settings.html")
    {}

    melanolib::Optional<Response> prepare(
        const RequestItem& request,
        Context& context
    ) const override
    {
        context.set(
            "global_settings",
            context.type().type_system().reference(settings::global_settings)
        );
        return {};
    }
};

StatusPage::StatusPage(const Settings& settings)
{
    uri = read_uri(settings, "");
    css_file = settings.get("css", css_file);
    editable = settings.get("editable", editable);
    template_path = settings.get("template_path", template_path);

    sub_pages.push_back(melanolib::New<Home>());
    sub_pages.push_back(melanolib::New<GlobalSettings>());
    sub_pages.push_back(melanolib::New<ConnectionList>());
    sub_pages.push_back(melanolib::New<ConnectionDetails>());
    sub_pages.push_back(melanolib::New<ServiceList>());
    sub_pages.push_back(melanolib::New<ServiceDetails>());
}

StatusPage::~StatusPage() = default;

Response StatusPage::respond(const RequestItem& request) const
{
    RequestItem local_item = request.descend(uri);

    SubPage* current_page = nullptr;
    for ( const auto& page : sub_pages )
    {
        if ( !current_page && page->matches(local_item) )
            current_page = page.get();
    }

    if ( !current_page )
        throw HttpError(StatusCode::NotFound);

    auto& ts = scripting_typesystem();
    auto context = ts.object<melanolib::scripting::SimpleType>();
    context.set("editable", ts.object(editable));
    context.set("request", ts.object(local_item));
    context.set("sub_request", ts.object(local_item.descend(current_page->path())));
    context.set("bot", ts.reference(melanobot::Melanobot::instance()));
    context.set("context", ts.reference(context));

    auto context_page = ts.object<melanolib::scripting::SimpleType>();
    context_page.set("css_file", ts.object(css_file));
    context_page.set("editable", ts.object(editable));
    context_page.set("current", ts.reference(*current_page));
    context_page.set("children", ts.reference(sub_pages));
    context_page.set("template_path", ts.object(template_path));
    context.set("page", context_page);


    if ( auto response = current_page->prepare(local_item, context) )
        return std::move(*response);

    Response response("text/html", StatusCode::OK, request.request.protocol);
    response.body << SubPage::process_template(template_path, "status/wrapper.html", context);
    return response;
}

} // namespace web
