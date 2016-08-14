/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015-2016 Mattia Basaglia
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
#include "boost-python.hpp"
#include "melanobot/melanobot.hpp"

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
    void write(std::string msg)
    {
        auto newline = msg.find('\n');

        while ( newline != std::string::npos )
        {
            line += msg.substr(0, newline);
            flush();

            msg.erase(0,newline+1);
            newline = msg.find('\n');
        }

        line += msg;
    }

    void write_unicode(const boost::python::object& unicode)
    {
        using namespace boost::python;
        write(extract<std::string>(str(unicode).encode("utf-8")));
    }

    void flush()
    {
        if ( print && !line.empty() )
            print(line);
        line.clear();
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
 * \brief Environment used to execute python scripts
 */
class ScriptEnvironment
{
public:
    ScriptEnvironment(ScriptOutput& output, ScriptOutput::CaptureFlags flags):
        output_(output),
        stdout{[this, flags](const std::string& line){
            if ( flags & ScriptOutput::LogStdout )
                Log("py",'>',3) << line;
            if ( flags & ScriptOutput::CaptureStdout )
                output_.output.push_back(line);
        }},
        stderr{[this, flags](const std::string& line){
            if ( flags & ScriptOutput::LogStderr )
                Log("py",'>',3) << line;
            if ( flags & ScriptOutput::CaptureStderr )
                output_.output.push_back(line);
        }}
    {
        using namespace boost::python;
        static object class_OutputCapture = class_<OutputCapture>("OutputCapture", no_init)
            .def("write", &OutputCapture::write)
            .def("write", &OutputCapture::write_unicode)
            .def("flush", &OutputCapture::flush)
        ;
        object main_module = import("__main__");
        main_namespace_ = boost::python::dict();
        main_namespace_["__builtins__"] = main_module.attr("__dict__")["__builtins__"];
        object sys_module = import("sys");
        sys_module.attr("stdout") = ptr(&stdout);
        sys_module.attr("stderr") = ptr(&stderr);
        main_module.attr("raw_input") = make_function(&raw_input);

        object melanobot_module = import("melanobot");
        melanobot_module.attr("bot") = ptr(&melanobot::Melanobot::instance());
    }

    ~ScriptEnvironment()
    {
        stdout.flush();
        stderr.flush();
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
    ScriptOutput& output_;
    OutputCapture stdout;
    OutputCapture stderr;
};

} // namespace python
#endif // PYTHON_UTILS_HPP
