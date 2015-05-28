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

#include <boost/python.hpp>
#include <Python.h>

class PythonIO
{
public:
    void write(const std::string& msg)
    {
        auto newline = msg.find('\n');

        if ( newline != std::string::npos )
        {
            Log("py",'>') << line << msg.substr(0,newline);
            line = msg.substr(newline+1);
        }
        else
        {
            line += msg;
        }
    }
private:
    std::string line;
};

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
        object classPythonIO = class_<PythonIO>("PythonIO")
            .def("write", &PythonIO::write);
        object sys_module = import("sys");
        sys_module.attr("stdout") = classPythonIO();

        exec("print \"Hello\", \"World!\"",main_namespace);
        exec("print \"Python here!\"",main_namespace);
        exec("1/0",main_namespace);
    } catch (const error_already_set& exc) {
        ErrorLog("py") << "PyDerp!";
        PyErr_Print();
    }
    return module;
}
