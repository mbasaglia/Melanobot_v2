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

std::string page_link(
    const web::Request& request,
    const web::UriPath& path,
    const std::string& text,
    bool is_current_parent)
{
    if ( web::UriPathSlice(request.uri.path).match_exactly(path) )
        return "<span class='current_page'>" + text + "</span>";

    std::string extra;
    if ( is_current_parent )
        extra = " class='current_page'";

    return "<a href='" + path.url_encoded(true) + "'" + extra + ">" + text + "</a>";
}

class StatusPage::SubPage
{
public:
    using BlockElement = httpony::quick_xml::BlockElement;
    using PathSuffix = UriPathSlice;
    using RequestItem = WebPage::RequestItem;
    using ParentPage = StatusPage;
    using RenderResult = melanolib::Variant<std::string, Response>;

    SubPage(ParentPage& parent, std::string name, UriPath path)
        : _parent(parent), _name(std::move(name)), _path(std::move(path))
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

    virtual RenderResult render(
        const RequestItem& request,
        melanolib::scripting::Object& context
    ) const = 0;

    virtual std::string submenu(melanolib::scripting::Object& context) const
    {
        return {};
    }

    ParentPage& parent() const
    {
        return _parent;
    }

protected:
    std::string process_template(
        const std::string& name,
        const melanolib::scripting::Object& context) const
    {
        return _parent.process_template(name, context);
    }

private:
    ParentPage& _parent;
    std::string _name;
    UriPath _path;
};

class Home : public StatusPage::SubPage
{
public:
    Home(ParentPage& parent) : SubPage(parent, "Home", "") {}

    bool match_path(const PathSuffix& path) const
    {
        return path.empty();
    }

    RenderResult render(
        const RequestItem& request,
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

        return process_template("status/home.html", context);
    }
};

class Connections : public StatusPage::SubPage
{
public:
    Connections(ParentPage& parent)
        : SubPage(parent, "Connections", "connection")
    {}

    RenderResult render(
        const RequestItem& request,
        melanolib::scripting::Object& context
    ) const override
    {
        if ( request.path.size() == 0 )
            return process_template("status/connections.html", context);

        network::Connection* conn = melanobot::Melanobot::instance().connection(request.path[0]);
        if ( !conn )
            throw HttpError(StatusCode::NotFound);

        if ( request.path.size() == 2 && parent().is_editable() )
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
        return process_template("status/connection.html", context);
    }

    virtual std::string submenu(melanolib::scripting::Object& context) const override
    {
        return process_template("status/connections_menu.html", context);
    }
};

class Services : public StatusPage::SubPage
{
public:
    Services(ParentPage& parent)
        : SubPage(parent, "Services", "service")
    {}

    RenderResult render(
        const RequestItem& request,
        melanolib::scripting::Object& context
    ) const override
    {
        if ( request.path.size() == 0 )
            return process_template("status/services.html", context);

        AsyncService* service = nullptr;
        for ( const auto& svc : melanobot::Melanobot::instance().service_list() )
            if ( std::to_string(uintptr_t(svc.get())) == request.path[0] )
            {
                service = svc.get();
                break;
            }
        if ( !service )
            throw HttpError(StatusCode::NotFound);


        if ( request.path.size() == 2 && parent().is_editable() )
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
        return process_template("status/service.html", context);

        context.set("service", context.type().type_system().reference(*service));
    }

    virtual std::string submenu(melanolib::scripting::Object& context) const override
    {
        return process_template("status/services_menu.html", context);
    }
};

class GlobalSettings : public StatusPage::SubPage
{
public:
    GlobalSettings(ParentPage& parent)
        : SubPage(parent, "Global Settings", "settings")
    {}

    RenderResult render(
        const RequestItem& request,
        melanolib::scripting::Object& context
    ) const override
    {
        if ( !request.path.empty() )
            throw HttpError(StatusCode::NotFound);

        context.set(
            "global_settings",
            context.type().type_system().reference(settings::global_settings)
        );
        return process_template("status/global_settings.html", context);
    }
};

StatusPage::StatusPage(const Settings& settings)
{
    uri = read_uri(settings, "");
    css_file = settings.get("css", css_file);
    editable = settings.get("editable", editable);
    template_path = settings.get("template_path", template_path);

    sub_pages.push_back(melanolib::New<Home>(*this));
    sub_pages.push_back(melanolib::New<GlobalSettings>(*this));
    sub_pages.push_back(melanolib::New<Connections>(*this));
    sub_pages.push_back(melanolib::New<Services>(*this));
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

        menu.add_item(Raw(page_link(
            request.request,
            local_item.base_path() / page->path(),
            page->name(),
            current_page == page.get()
        )));
    }
    if ( !current_page )
        throw HttpError(StatusCode::NotFound);


    auto& ts = scripting_typesystem();
    auto context = ts.object<melanolib::scripting::SimpleType>();
    context.set("editable", ts.object(editable));
    context.set("request", ts.object(local_item));
    context.set("sub_request", ts.object(local_item.descend(current_page->path())));
    context.set("bot", ts.reference(melanobot::Melanobot::instance()));

    auto result = current_page->render(local_item.descend(current_page->path()), context);
    if ( result.which() == 1 )
        return std::move(melanolib::get<Response>(result));

    Element nav("nav");
    nav.append(menu);
    std::string submenu = current_page->submenu(context);
    if ( !submenu.empty() )
    {
        nav.append(Element(
            "nav",
            Attribute("class", "submenu"),
            Raw(std::move(submenu))
        ));
    }
    html.body().append(nav);

    BlockElement contents("div", Attribute("class", "contents"));
    contents.append(Raw(melanolib::get<std::string>(result)));
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
