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
#include "base_pages.hpp"
#include "melanobot/melanobot.hpp"
#include "formatter_html.hpp"

class ServiceStatus
{
public:
    ServiceStatus(network::Connection::Status status)
    {
        if ( status > network::Connection::CHECKING )
        {
            status_name = "Connected";
            short_name = "OK";
        }
        else if ( status >= network::Connection::CONNECTING )
        {
            status_name = "Connecting";
            short_name = "...";
        }
        else
        {
            status_name = "Disconnected";
            short_name = "(!)";
        }
    }

    ServiceStatus(bool status)
        : ServiceStatus(status ? network::Connection::CONNECTED : network::Connection::DISCONNECTED)
        {}

    const std::string& name() const
    {
        return status_name;
    }

    std::string css_class() const
    {
        return "status_" + melanolib::string::strtolower(status_name);
    }

    std::string element() const
    {
        return generic_element(status_name, "span");
    }

    std::string short_element() const
    {
        return generic_element(short_name, "span");
    }


private:
    std::string generic_element(const std::string& text, const std::string& tag) const
    {
        return "<" + tag + " class='" + css_class() + "'>" + text + "</" + tag + ">";
    }

    std::string status_name;
    std::string short_name;
};

static std::string page_link(
    const web::Request& request,
    const web::UriPath& path,
    const std::string& text,
    bool is_current_parent = false)
{
    if ( web::UriPathSlice(request.uri.path).match_exactly(path) )
        return "<span class='current_page'>" + text + "</span>";

    std::string extra;
    if ( is_current_parent )
        extra = " class='current_page'";

    return "<a href='" + path.url_encoded(true) + "'" + extra + ">" + text + "</a>";
}

static void init_type_system(melanolib::scripting::TypeSystem& ts)
{
    using namespace network;
    using namespace web;
    using namespace melanolib::scripting;

    ts.register_type<bool>("bool");
    ts.register_type<uint16_t>("uint16_t");
    ts.register_type<std::size_t>("size");

    ts.register_type<SimpleType>("SimpleType")
        .add_method("set", [](SimpleType& obj, const std::string& key, const Object& value){
            obj.set(key, value);
            return std::string{};
        })
    ;

    ts.register_type<std::string>("string")
        .add_readwrite("size", &std::string::size,
            [](std::string& str, std::size_t sz){ str.resize(sz, ' '); }
        )
        .add_readonly("empty", &std::string::empty)
    ;

    ts.register_type<string::FormattedString>("FormattedString")
        .string_conversion([](const string::FormattedString& value){
            return value.encode(web::FormatterHtml{});
        })
    ;

    ts.register_type<UriPath>("Path")
        .string_conversion([](const UriPath& value){
            return value.url_encoded(true);
        })
    ;

    ts.register_type<AsyncService>("AsyncService")
        .add_readonly("name", &AsyncService::name)
        .add_readonly("running", &AsyncService::running)
        .add_readonly("status", [](const AsyncService& service){
            return ServiceStatus(service.running());
        })
        .add_readonly("id", [](const AsyncService& svc) {
            return std::to_string(uintptr_t(&svc));
        })
    ;

    ts.register_type<ServiceStatus>("ServiceStatus")
        .add_readonly("element", &ServiceStatus::element)
        .add_readonly("short_element", &ServiceStatus::short_element)
    ;

    ts.register_type<Connection>("Connection")
        .add_readonly("protocol", &Connection::protocol)
        .add_readonly("config_name", &Connection::config_name)
        .add_readonly("name", &Connection::name)
        .add_readonly("server", &Connection::server)
        .add_readonly("status", [](const Connection& connection){
            return ServiceStatus(connection.status());
        })
        .add_readonly("formatter", [](const Connection& conn){
            return conn.formatter()->name();
        })
        .add_readonly("pretty_properties",
            (string::FormattedProperties (Connection::*) ()const)
            (&Connection::pretty_properties)
        )
        .add_readonly("properties", [](const Connection& conn){
            auto foo = const_cast<Connection&>(conn).properties().get("foo.bar", 0);
            (void)foo;
            return const_cast<Connection&>(conn).properties().copy();
        })
    ;

    auto settings_data = (const Settings::data_type& (Settings::*)() const) &Settings::data;
    ts.register_type<Settings>("Settings")
        .string_conversion(settings_data)
        .add_readonly("empty", &Settings::empty)
        .add_readonly("has_children", [](const Settings& tree){
            return !tree.empty();
        })
        .add_readonly("data", settings_data)
        .make_iterable(
            [](Settings& settings){
                return settings::SettingsDepthIterator(settings);
            },
            [](Settings&){
                return settings::SettingsDepthIterator();
            },
            [&ts](const settings::SettingsDepthIterator::value_type& value){
                Object object = ts.object<SimpleType>();
                object.set("key", ts.object(value.first));
                object.set("value", ts.reference(value.second));
                return object;
            }
        )
    ;

    ts.register_type<Server>("Server")
        .add_readonly("host", &Server::host)
        .add_readonly("port", &Server::port)
        .string_conversion(&Server::name)
    ;

    ts.register_type<WebPage::RequestItem>("Request")
        .add_readonly("base_path", &WebPage::RequestItem::base_path)
        .add_readonly("full_path", &WebPage::RequestItem::full_path)
    ;

    ts.register_type<settings::SystemInfo>("SystemInfo")
        .add_readonly("os", &settings::SystemInfo::os)
        .add_readonly("os_version", &settings::SystemInfo::os_version)
        .add_readonly("machine", &settings::SystemInfo::machine)
    ;

    using ConnectionList = std::vector<Ref<Connection>>;
    using ServiceList = std::vector<Ref<AsyncService>>;
    ts.register_type<melanobot::Melanobot>("Melanobot")
        .add_readonly("connections", [](const melanobot::Melanobot& bot){
            ConnectionList connections;
            for ( const auto& conn : bot.connection_names() )
                connections.emplace_back(*bot.connection(conn));
            return connections;
        })
        .add_readonly("services", [](const melanobot::Melanobot& bot){
            ServiceList services;
            for ( const auto& svc : bot.service_list() )
                services.emplace_back(*svc);
            return services;
        })
    ;

    ts.ensure_type<ConnectionList::size_type>();
    ts.register_type<ConnectionList>("ConnectionList")
        .make_iterable(WrapReferencePolicy{})
        .add_readonly("size", &ConnectionList::size)
    ;

    ts.ensure_type<ServiceList::size_type>();
    ts.register_type<ServiceList>("ServiceList")
        .make_iterable(WrapReferencePolicy{})
        .add_readonly("size", &ServiceList::size)
    ;

    using PrettyProps = string::FormattedProperties;
    ts.ensure_type<PrettyProps::size_type>();
    ts.register_type<PrettyProps>("FormattedProperties")
        .make_iterable()
        .add_readonly("size", &PrettyProps::size)
        .fallback_getter([](const PrettyProps& obj, const std::string& name){
            return obj.at(name);
        })
    ;
    ts.register_type<PrettyProps::value_type>()
        .add_readonly("key", &PrettyProps::value_type::first)
        .add_readonly("value", &PrettyProps::value_type::second)
    ;

    ts.register_type<SubPage>("SubPage")
        .add_readonly("name", &SubPage::name)
        .add_readonly("path", &SubPage::path)
        .add_readonly("show_on_menu", &SubPage::show_on_menu)
        .add_method("submenu", &SubPage::submenu)
        .add_method("render", &SubPage::render)
        .add_method("page_link",
            [](const SubPage& page, const WebPage::RequestItem& request, const SubPage& curr){
                return page_link(
                    request.request,
                    request.base_path()/page.path(),
                    page.name(),
                    &page == &curr
                );
        })
    ;

    using SubPageList = std::vector<std::unique_ptr<SubPage>>;
    ts.register_type<SubPageList>("SubPages")
        .add_readonly("size", &SubPageList::size)
        .make_iterable(melanolib::Begin<SubPageList>{}, melanolib::End<SubPageList>{},
            [](std::unique_ptr<SubPage>& subpage){
                return wrap_reference(*subpage);
            }
        )
    ;

    ts.register_type<Html>("Html")
        .add_readonly("escape", &httpony::quick_xml::amp_escape)
        .add_method("page_link", [](const WebPage::RequestItem& request,
                                    const Object& url, const Object& text) {
            return page_link(request.request, url.to_string(), text.to_string());
        })
        .add_method("url_encode", &httpony::urlencode) // string, bool
        .add_method("url_encode", [](const std::string& text) { // string
            return httpony::urlencode(text);
        })
    ;
}

namespace web {

melanolib::scripting::TypeSystem& scripting_typesystem()
{
    static thread_local melanolib::scripting::TypeSystem ts;
    static thread_local bool initalized = false;
    if ( !initalized )
    {
        init_type_system(ts);
        initalized = true;
    }
    return ts;
}

} // namespace web
