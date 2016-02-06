/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015-2016 Mattia Basaglia
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
#include <boost/python/exception_translator.hpp>

#include "settings.hpp"
#include "message/input_message.hpp"
#include "script_variables.hpp"
#include "melanobot/melanobot.hpp"
#include "network/connection.hpp"
#include "network/async_service.hpp"
#include "melanobot/storage.hpp"

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
 * \brief Return a functor converting a pointer to member using Converter::convert
 */
template<class Class, class Member>
    auto convert_member_setter(Member Class::*member)
    {
        return [member](Class& obj, const boost::python::object& input) {
            Converter::convert(input, obj.*member);
        };
    }

/**
 * \brief Shothand to make functions that return pointer managed by C++
 */
template<class Func>
    auto return_pointer(Func&& func)
    {
        using namespace boost::python;
        return make_function(std::forward<Func>(func),
            return_value_policy<reference_existing_object>());
    }

/**
 * \brief Shothand to make functions that return a reference that should be copied
 */
template<class Func>
    auto return_copy(Func&& func)
    {
        using namespace boost::python;
        return make_function(std::forward<Func>(func),
            return_value_policy<return_by_value>());
    }

/**
 * \brief Creates a shared_ptr to a user which will update it upon destruction
 */
inline std::shared_ptr<user::User> make_shared_user(const user::User& user)
{
    std::string starting_id = user.local_id;
    return std::shared_ptr<user::User>(new user::User(user),
        [starting_id](user::User* ptr) {
            if ( ptr->origin && !starting_id.empty() )
                ptr->origin->update_user(starting_id, *ptr);
            delete ptr;
        });
}

/**
 * \brief Namespace corresponding to the python module \c melanobot
 */
namespace py_melanobot {

/**
 * \brief Defines symbols in the \c melanobot.network Python module
 * \param module_top Parent module object
 */
void module_melanobot_network(boost::python::scope& module_top)
{
    using namespace boost::python;

    // Name of the parent scope
    std::string module_parent_name = extract<std::string>(module_top.attr("__name__"));

    // Name of the current module
    std::string module_name = "network";
    // Fully qualified name for the current module
    std::string module_full_name = module_parent_name + '.' + module_name;
    // Create the module using the C API and get a boost wrapper
    object module_object(borrowed(PyImport_AddModule(module_full_name.c_str())));
    // Update the parent scope object
    module_top.attr(module_name.c_str()) = module_object;
    // Change scope for the next declarations
    scope module_scope = module_object;

    /// \todo readonly or readwrite?
    class_<network::Message>("Message",no_init)
        .def_readwrite("raw",&network::Message::raw)
        .def_readonly("params",convert_member(&network::Message::params))

        .def_readwrite("message",&network::Message::message)
        .def_readonly("channels",convert_member(&network::Message::channels))
        .def_readwrite("direct",&network::Message::direct)
        .add_property("user",[](const network::Message& msg){
            return make_shared_user(msg.from);
        })
        .def_readonly("victim",[](const network::Message& msg){
            return make_shared_user(msg.victim);
        })
        .def_readonly("source",&network::Message::source)
        .def_readonly("destination",&network::Message::destination)
    ;

    class_<network::Connection,network::Connection*,boost::noncopyable>("Connection",no_init)
        .add_property("name",return_copy(&network::Connection::config_name))
        .add_property("description",&network::Connection::description)
        .add_property("protocol",&network::Connection::protocol)
        .add_property("formatter",return_pointer(&network::Connection::formatter))
        .def("user",[](network::Connection* conn, const std::string& local_id){
            return make_shared_user(conn->get_user(local_id));
        })
        /// \todo Expose Command
        .def("command",[](network::Connection* conn,const std::string& command){
            conn->command({command});
        })
        /// \todo Expose OutputMessage and say()
        .def("connect",&network::Connection::connect)
        .def("disconnect",&network::Connection::disconnect)
        .def("reconnect",&network::Connection::reconnect)
        .def("__getattr__",[](network::Connection* conn, const std::string& name){
            auto props = conn->pretty_properties();
            auto it = props.find(name);
            if ( it != props.end() )
                return it->second.encode(*conn->formatter());
            return conn->properties().get(name);
        })
        .def("__setattr__",[](network::Connection* conn,
                              const std::string&   name,
                              const std::string&   value ){
            conn->properties().put(name, value);
        })
    ;

    /*class_<network::Request>("Request")
        .def("__init__",make_constructor(
            [](const std::string& command, const std::string& resource, list parameters) {
                std::vector<std::string> params;
                Converter::convert(parameters,params);
                return std::shared_ptr<network::Request>(
                    new network::Request(command, resource, params));
        }))
        .def_readwrite("command",&network::Request::command)
        .def_readwrite("resource",&network::Request::resource)
        .add_property("parameters",
                      convert_member(&network::Request::parameters),
                      convert_member_setter(&network::Request::parameters))
    ;

    class_<network::Response>("Response")
        .def_readwrite("error_message",&network::Response::error_message)
        .def_readwrite("contents",&network::Response::contents)
        .def_readwrite("resource",&network::Response::resource)
    ;

    class_<network::AsyncService,network::AsyncService*,boost::noncopyable>("Service",no_init)
        .def("query",[](network::AsyncService* serv, const network::Request& request)
            { return serv->query(request); })
    ;

    def("service",return_pointer(&network::require_service));*/
}

void module_melanobot_storage(boost::python::scope& module_top)
{
    using namespace boost::python;
    using sequence      = melanobot::StorageBase::sequence;
    using table         = melanobot::StorageBase::table;
    using key_type      = melanobot::StorageBase::key_type;
    using value_type    = melanobot::StorageBase::value_type;

    // Name of the parent scope
    std::string module_parent_name = extract<std::string>(module_top.attr("__name__"));

    // Name of the current module
    std::string module_name = "storage";
    // Fully qualified name for the current module
    std::string module_full_name = module_parent_name + '.' + module_name;
    // Create the module using the C API and get a boost wrapper
    object module_object(borrowed(PyImport_AddModule(module_full_name.c_str())));
    // Update the parent scope object
    module_top.attr(module_name.c_str()) = module_object;
    // Change scope for the next declarations
    scope module_scope = module_object;

    register_exception_translator<melanobot::StorageError>(
        [](const melanobot::StorageError& err){
            PyErr_SetString(PyExc_RuntimeError, err.what());
    });

    def("get_value",[](const key_type& path) {
        return melanobot::storage().get_value(path);
    });
    def("get_sequence",[](const key_type& path) {
        list out;
        Converter::convert(melanobot::storage().get_sequence(path), out);
        return out;
    });
    def("get_map",[](const key_type& path) {
        dict out;
        Converter::convert(melanobot::storage().get_map(path), out);
        return out;
    });

    def("maybe_get_value",[](const key_type& path, const value_type& def) {
        return melanobot::storage().maybe_get_value(path, def);
    });
    def("maybe_get_sequence",[](const key_type& path) {
        list out;
        Converter::convert(melanobot::storage().maybe_get_sequence(path), out);
        return out;
    });
    def("maybe_get_map",[](const key_type& path) {
        dict out;
        Converter::convert(melanobot::storage().maybe_get_map(path), out);
        return out;
    });

    def("put", [](const key_type& path, const value_type& value) {
        melanobot::storage().put(path, value);
    });
    def("put", [](const std::string& path, const list& value) {
        sequence seq;
        Converter::convert(value, seq);
        melanobot::storage().put(path, seq);
    });
    def("put", [](const std::string& path, const dict& value) {
        table map;
        Converter::convert(value, map);
        melanobot::storage().put(path, map);
    });
    def("put", [](const key_type& path, const key_type& key, const value_type& value) {
        melanobot::storage().put(path, key, value);
    });

    def("append", [](const key_type& path, const value_type& value) {
        melanobot::storage().append(path, value);
    });

    def("erase", [](const key_type& path) {
        return melanobot::storage().erase(path);
    });
    def("erase", [](const key_type& path, const key_type& key) {
        return melanobot::storage().erase(path, key);
    });

    def("maybe_put", [](const key_type& path, const value_type& value) {
        return melanobot::storage().maybe_put(path, value);
    });
    def("maybe_put", [](const std::string& path, const list& value) {
        sequence seq;
        Converter::convert(value, seq);
        list out;
        Converter::convert(melanobot::storage().maybe_put(path, seq), out);
        return out;
    });
    def("maybe_put", [](const std::string& path, const dict& value) {
        table map;
        Converter::convert(value, map);
        dict out;
        Converter::convert(melanobot::storage().maybe_put(path, map), out);
        return out;
    });
}

/**
 * \brief Defines symbols in the \c melanobot Python module
 */
BOOST_PYTHON_MODULE(melanobot)
{
    using namespace boost::python;

    // Scope representing the current module
    scope module_top;

    // Register exceptions
    register_exception_translator<melanobot::ConfigurationError>(
        [](const melanobot::ConfigurationError& err){
            PyErr_SetString(PyExc_RuntimeError, err.what());
    });

    // def data_file(path, check)
    def("data_file", &settings::data_file);
    // def data_file(path)
    def("data_file", [](const std::string& path) { return settings::data_file(path); } );

    class_<user::User, std::shared_ptr<user::User>>("User",no_init)
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

    class_<melanobot::Melanobot, melanobot::Melanobot*, boost::noncopyable>("Melanobot",no_init)
        .def("stop",&melanobot::Melanobot::stop)
        .def("connection",return_pointer(&melanobot::Melanobot::connection))
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
        .def("__str__",[](const color::Color12& col){
            object main = import("__main__");
            dict main_namespace = extract<dict>(main.attr("__dict__"));
            if ( !main_namespace.has_key("formatter") )
                return object(str());
            return main_namespace["formatter"].attr("convert")(col);
        })
        .def("__add__",[](const color::Color12& lhs, const str& rhs) {
            return str(lhs)+rhs;
        })
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

    module_melanobot_network(module_top);
    module_melanobot_storage(module_top);

}

#if PY_VERSION_HEX >= 0x03000000
    PyObject* initmelanobot()
    {
        PyInit_melanobot();
    }
#endif

} // namespace py_melanobot
} // namespace python
#endif // PYTHON_MODULES_HPP
