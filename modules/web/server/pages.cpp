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
#include "status_page_impl.hpp"

using namespace httpony::quick_xml;
using namespace httpony::quick_xml::html;

namespace web {

static  melanobot::Melanobot& bot()
{
    return melanobot::Melanobot::instance();
}

static std::shared_ptr<BlockElement> page_link(
    const Request& request,
    const UriPath& path,
    const Text& text)
{
    if ( UriPathSlice(request.uri.path).match_exactly(path) )
        return std::make_shared<Element>("span", text, Attribute("class", "current_page"));
    else
        return std::make_shared<Link>(path.url_encoded(true), text);
}

static List connection_list(const WebPage::RequestItem& request)
{
    List connections;
    for ( const auto& conn : bot().connection_names() )
    {
        auto link = page_link(request.request, request.base_path()/"connection"/conn, conn);
        link->append(ServiceStatus(bot().connection(conn)->status()).short_element());
        connections.add_item(link);
    }
    return connections;
}

static List service_list(const WebPage::RequestItem& request)
{
    List services;
    for ( const auto& svc : bot().service_list() )
    {
        auto link = page_link(
            request.request,
            request.base_path()/"service"/std::to_string(uintptr_t(svc.get())),
            svc->name()
        );
        link->append(ServiceStatus(svc->running()).short_element());
        services.add_item(link);
    }
    return services;
}

class StatusPage::SubPage
{
public:
    using BlockElement = httpony::quick_xml::BlockElement;
    using PathSuffix = UriPathSlice;
    using RequestItem = WebPage::RequestItem;
    using ParentPage = StatusPage;

    SubPage(std::string name, UriPath path)
        : _name(std::move(name)), _path(std::move(path))
    {}

    virtual ~SubPage(){}

    virtual bool match_path(const PathSuffix& path) const
    {
        return path.match_prefix(_path);
    }

    const std::string& name() const
    {
        return _name;
    }

    const UriPath& path() const
    {
        return _path;
    }

    virtual melanolib::Optional<Response> render(
        const RequestItem& request,
        BlockElement& parent,
        const ParentPage& parent_page,
        melanolib::scripting::Object& context
    ) const = 0;

    virtual bool has_submenu() const
    {
        return false;
    }

    virtual List submenu(const RequestItem& request) const
    {
        return List();
    }

private:
    std::string _name;
    UriPath _path;
};

class Home : public StatusPage::SubPage
{
public:
    Home() : SubPage("Home", "") {}

    bool match_path(const PathSuffix& path) const
    {
        return path.empty();
    }

    melanolib::Optional<Response> render(
        const RequestItem& request,
        BlockElement& parent,
        const ParentPage& parent_page,
        melanolib::scripting::Object& context
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

        parent.append(httpony::quick_xml::Raw(
            parent_page.process_template("status/home.html", context)
        ));

        return {};
    }
};

class Connections : public StatusPage::SubPage
{
public:
    Connections() : SubPage("Connections", "connection") {}

    melanolib::Optional<Response> render(
        const RequestItem& request,
        BlockElement& parent,
        const ParentPage& parent_page,
        melanolib::scripting::Object& context
    ) const override
    {
        if ( request.path.size() == 0 )
        {
            parent.append(httpony::quick_xml::Raw(
                parent_page.process_template("status/connections.html", context)
            ));
            return {};
        }

        network::Connection* conn = bot().connection(request.path[0]);
        if ( !conn )
            throw HttpError(StatusCode::NotFound);

        if ( request.path.size() == 2 && parent_page.is_editable() )
        {
            if ( request.path[1] == "stop" )
            {
                conn->stop();
            }
            else if ( request.path[1] == "start" )
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
        else if ( request.path.size() != 1 )
        {
            throw HttpError(StatusCode::NotFound);
        }

        context.set("connection", context.type().type_system().reference(*conn));
        parent.append(httpony::quick_xml::Raw(
            parent_page.process_template("status/connection.html", context)
        ));

        return {};
    }

    virtual bool has_submenu() const override
    {
        return true;
    }

    virtual List submenu(const RequestItem& request) const override
    {
        return connection_list(request);
    }
};

class Services : public StatusPage::SubPage
{
public:
    Services() : SubPage("Services", "service") {}

    melanolib::Optional<Response> render(
        const RequestItem& request,
        BlockElement& parent,
        const ParentPage& parent_page,
        melanolib::scripting::Object& context
    ) const override
    {
        if ( request.path.size() == 0 )
        {
            parent.append(httpony::quick_xml::Raw(
                parent_page.process_template("status/services.html", context)
            ));
            return {};
        }

        AsyncService* service = nullptr;
        for ( const auto& svc : bot().service_list() )
            if ( std::to_string(uintptr_t(svc.get())) == request.path[0] )
            {
                service = svc.get();
                break;
            }
        if ( !service )
            throw HttpError(StatusCode::NotFound);


        if ( request.path.size() == 2 && parent_page.is_editable() )
        {
            if ( request.path[1] == "stop" )
            {
                service->stop();
            }
            else if ( request.path[1] == "start" )
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
        else if ( request.path.size() != 1 )
        {
            throw HttpError(StatusCode::NotFound);
        }

        context.set("service", context.type().type_system().reference(*service));
        parent.append(httpony::quick_xml::Raw(
            parent_page.process_template("status/service.html", context)
        ));
        return {};
    }

    virtual bool has_submenu() const override
    {
        return true;
    }

    virtual List submenu(const RequestItem& request) const override
    {
        return service_list(request);
    }
};

class GlobalSettings : public StatusPage::SubPage
{
public:
    GlobalSettings() : SubPage("Global Settings", "settings") {}

    melanolib::Optional<Response> render(
        const RequestItem& request,
        BlockElement& parent,
        const ParentPage& parent_page,
        melanolib::scripting::Object& context
    ) const override
    {
        if ( !request.path.empty() )
            throw HttpError(StatusCode::NotFound);

        context.set(
            "global_settings",
            context.type().type_system().reference(settings::global_settings)
        );
        parent.append(httpony::quick_xml::Raw(
            parent_page.process_template("status/global_settings.html", context)
        ));
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
    sub_pages.push_back(melanolib::New<Connections>());
    sub_pages.push_back(melanolib::New<Services>());
}

StatusPage::~StatusPage() = default;

Response StatusPage::respond(const RequestItem& request) const
{
    RequestItem local_item = request.descend(uri);
    HtmlDocument html("Bot status");

    if ( !css_file.empty() )
    {
        html.head().append(Element("link", Attributes{
            {"rel", "stylesheet"}, {"type", "text/css"}, {"href", css_file}
        }));
    }

    List menu;
    menu.append(Attribute("class", "menu"));
    SubPage* current_page = nullptr;
    for ( const auto& page : sub_pages )
    {
        if ( !current_page && page->match_path(local_item.path) )
            current_page = page.get();

        auto link = page_link(request.request, local_item.base_path() / page->path(), page->name());
        if ( current_page == page.get() && link->tag_name() == "a" )
            link->append(Attribute("class", "current_page"));
        menu.add_item(link);
    }
    if ( !current_page )
        throw HttpError(StatusCode::NotFound);

    Element nav("nav");
    nav.append(menu);

    if ( current_page->has_submenu() )
    {
        List submenu = current_page->submenu(local_item);
        submenu.append(Attribute("class", "submenu"));
        nav.append(submenu);
    }

    html.body().append(nav);

    BlockElement contents("div", Attribute("class", "contents"));
    auto& ts = scripting_typesystem();
    auto context = ts.object<melanolib::scripting::SimpleType>();
    context.set("editable", ts.object(editable));
    context.set("request", ts.object(local_item));
    context.set("sub_request", ts.object(local_item.descend(current_page->path())));
    context.set("bot", ts.reference(melanobot::Melanobot::instance()));

    auto resp = current_page->render(local_item.descend(current_page->path()), contents, *this, context);
    if ( resp )
        return std::move(*resp);
    html.body().append(contents);

    Response response("text/html", StatusCode::OK, request.request.protocol);
    html.print(response.body, true);
    return response;
}

std::string StatusPage::process_template(
    const std::string& template_name,
    const melanolib::scripting::Object& context) const
{
    std::ifstream template_file(template_path + '/' + template_name);
    template_file.unsetf(std::ios::skipws);
    auto template_str = string::FormatterConfig{}.decode(
        std::string(std::istream_iterator<char>(template_file), {})
    );
    template_str.replace(context);
    return template_str.encode(string::FormatterUtf8{});
}

} // namespace web
