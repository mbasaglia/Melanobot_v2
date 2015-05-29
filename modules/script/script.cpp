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

#include "melanomodule.hpp"

#include <functional>
#include <boost/python.hpp>
#include <Python.h>

/**
 * \brief Class to capture output from Python scripts
 */
class PythonOutputCapture
{
public:
    PythonOutputCapture(){}
    PythonOutputCapture(const std::function<void(const std::string&)>& print)
        : print(print) {}

    void write(const std::string& msg)
    {
        auto newline = msg.find('\n');

        if ( newline != std::string::npos )
        {
            if ( print )
                print(line+msg.substr(0,newline));
            line = msg.substr(newline+1);
        }
        else
        {
            line += msg;
        }
    }

private:
    std::string line; ///< Partial line buffer
    std::function<void(const std::string&)> print; ///< Functor called when a line has to be printed
};

std::string raw_input(const std::string&) { return ""; }

/**
 * \brief Scripting module initialization
 * \todo Have the Python version available somewhere
 */
Melanomodule melanomodule_script()
{
    Melanomodule module{"script","Scripting interface"};
    module.register_log_type("py",color::dark_yellow);
    using namespace boost::python;
    try {
        Py_Initialize();

        object main_module = import("__main__");
        object main_namespace = main_module.attr("__dict__");
        object class_PythonOutputCapture = class_<PythonOutputCapture>("PythonOutputCapture",no_init)
            .def("write", &PythonOutputCapture::write);
        object sys_module = import("sys");
        PythonOutputCapture stdout([](const std::string& line){
            Log("py",'>') << line;
        });
        PythonOutputCapture stderr([](const std::string& line){
            Log("py",'>',3) << line;
        });
        sys_module.attr("stdout") = ptr(&stdout);
        sys_module.attr("stderr") = ptr(&stderr);
        main_module.attr("raw_input") = make_function(&raw_input);

        exec("print \"Hello\", \"World!\"",main_namespace);
        exec("print \"Python here!\"",main_namespace);
        exec("raw_input(\"foo\")",main_namespace);
        exec("1/0",main_namespace);
    } catch (const error_already_set& exc) {
        ErrorLog("py") << "Exception from python script";
        PyErr_Print();
    }
    return module;
}
