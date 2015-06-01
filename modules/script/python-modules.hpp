/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015 Mattia Basaglia
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
#ifndef PYTHON_MODULES_HPP
#define PYTHON_MODULES_HPP

#include "boost-python.hpp"
#include "settings.hpp"
#include "message/input_message.hpp"
#include "script_variables.hpp"
#include "melanobot.hpp"

namespace python {

/**
 * \brief Return a functor converting a pointer to member using Converter::convert
 */
template<class Class, class Member>
    auto convert_member(Member Class::*member)
    {
        return [member](const Class& obj) {
            boost::python::object out;
            Converter::convert(obj.*member, out);
            return out;
        };
    }

/**
 * \brief Namespace corresponding to the python module \c melanobot
 */
namespace melanobot {

BOOST_PYTHON_MODULE(melanobot)
{
    using namespace boost::python;

    def("data_file", &settings::data_file);
    def("data_file", [](const std::string& path) { return settings::data_file(path); } );

    class_<user::User>("User",no_init)
        .def_readwrite("name",&user::User::name)
        .def_readwrite("host",&user::User::host)
        .def_readonly("local_id",&user::User::local_id)
        .def_readwrite("global_id",&user::User::global_id)
        .add_property("channels",convert_member(&user::User::channels))
        .def("__getattr__",&user::User::property)
        .def("__setattr__",[](user::User& user, const std::string& property, object val) {
            user.properties[property] = extract<std::string>(val);
        })
    ;

    /// \todo readonly or readwrite?
    class_<network::Message>("Message",no_init)
        .def_readonly("raw",&network::Message::raw)
        .def_readonly("params",convert_member(&network::Message::params))

        .def_readonly("message",&network::Message::message)
        .def_readonly("channels",convert_member(&network::Message::channels))
        .def_readonly("direct",&network::Message::direct)
        .def_readonly("user",&network::Message::from)
        .def_readonly("victim",&network::Message::victim)
    ;

    class_<Melanobot,Melanobot*,boost::noncopyable>("Melanobot",no_init)
        .def("stop",&Melanobot::stop)
    ;

    class_<color::Color12>("Color")
        .def(init<std::string>())
        .def(init<color::Color12::BitMask>())
        .def(init<color::Color12::Component,color::Color12::Component,color::Color12::Component>())
        .add_property("valid",&color::Color12::is_valid)
        .add_property("red",&color::Color12::red)
        .add_property("green",&color::Color12::green)
        .add_property("blue",&color::Color12::blue)
        .def("hsv",&color::Color12::hsv).staticmethod("hsv")
        .def("blend",&color::Color12::blend).staticmethod("blend")
    ;

    class_<string::Formatter,string::Formatter*,boost::noncopyable>("Formatter",no_init)
        .def("__init__", make_constructor([](const std::string& name) {
            return string::Formatter::formatter(name);
        }))
        .add_property("name",[](string::Formatter* fmt) {
            return fmt ? fmt->name() : "";
        })
        .def("convert",[](string::Formatter* fmt, const color::Color12& col) {
            return fmt ? fmt->color(col) : "";
        })
        .def("convert",[](string::Formatter* fmt, const std::string& str) {
            return str;
        })

    ;

}

} // namespace melanobot
} // namespace python
#endif // PYTHON_MODULES_HPP
