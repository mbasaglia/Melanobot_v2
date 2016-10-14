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
#include "pages.hpp"
#include "config.hpp"
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

    httpony::quick_xml::Attribute css_class() const
    {
        return httpony::quick_xml::Attribute("class", "status_" + melanolib::string::strtolower(status_name));
    }

    httpony::quick_xml::BlockElement element(const std::string& tag = "span") const
    {
        return httpony::quick_xml::BlockElement{tag, css_class(), httpony::quick_xml::Text(status_name)};
    }

    httpony::quick_xml::BlockElement short_element(const std::string& tag = "span") const
    {
        return httpony::quick_xml::BlockElement{tag, css_class(), httpony::quick_xml::Text(short_name)};
    }


private:
    std::string status_name;
    std::string short_name;
};


namespace web {

    std::string page_link(
        const Request& request,
        const UriPath& path,
        const std::string& text,
        bool is_current_parent = false
    );

    melanolib::scripting::TypeSystem& scripting_typesystem();

} // namespace web
