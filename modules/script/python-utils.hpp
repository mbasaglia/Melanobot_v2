/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015 Mattia Basaglia
 * \brief Low-level utilities interfacing directly to boost python
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
#ifndef PYTHON_UTILS_HPP
#define PYTHON_UTILS_HPP

#include <functional>

#include "string/logger.hpp"
#include "python.hpp"
#include "settings.hpp"
#include "python-modules.hpp"

namespace python {

/**
 * \brief Class to capture output from Python scripts
 */
class OutputCapture
{
public:
    OutputCapture(){}
    OutputCapture(const std::function<void(const std::string&)>& print)
        : print(print) {}

    /**
     * \brief Function visible from python
     */
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

/**
 * \brief Dummy function which prevents raw_input from blocking the script
 */
inline std::string raw_input(const std::string&) { return {}; }

/**
 * \brief Convert C++ strings to Python strings
 */
inline boost::python::str py_str(const std::string& str)
{
    return { str.data(), str.size() };
}

/**
 * \brief Recursevily converts a property tree to a Python dict object
 */
inline void ptree_to_dict(boost::python::object& output, const PropertyTree& tree)
{
    for ( const auto& child : tree )
    {
        if ( child.second.empty() )
            output[py_str(child.first)] = py_str(child.second.data());
        else
        {
            boost::python::dict child_object;
            ptree_to_dict(child_object, child.second);
            output[py_str(child.first)] = child_object;
        }
    }
}

/**
 * \brief Environment used to execute python scripts
 */
class ScriptEnvironment
{
public:
    ScriptEnvironment(ScriptOutput& output, const PropertyTree& dict) :
        stdout{[this](const std::string& line){output_.output.push_back(line);}},
        stderr{[](const std::string& line){Log("py",'>',3) << line;}},
        output_(output)
    {
        using namespace boost::python;
        object main_module = import("__main__");
        main_namespace_ = main_module.attr("__dict__");
        object class_PythonOutputCapture = class_<OutputCapture>("OutputCapture",no_init)
            .def("write", &OutputCapture::write);
        object sys_module = import("sys");
        sys_module.attr("stdout") = ptr(&stdout);
        sys_module.attr("stderr") = ptr(&stderr);
        main_module.attr("raw_input") = make_function(&raw_input);
        ptree_to_dict(main_namespace_, dict);
    }

    boost::python::object& main_namespace()
    {
        return main_namespace_;
    }

    ScriptOutput& output()
    {
        return output_;
    }

private:
    boost::python::object main_namespace_;
    OutputCapture stdout;
    OutputCapture stderr;
    ScriptOutput& output_;
};

} // namespace python
#endif // PYTHON_UTILS_HPP
