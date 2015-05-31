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
#include "python.hpp"

#include "python-utils.hpp"
#include "python-modules.hpp"

namespace python {

ScriptOutput PythonEngine::exec(const std::string& python_code, const Converter& vars)
{
    if ( !initialize() )
        return ScriptOutput{};

    ScriptOutput output;

    try {
        ScriptEnvironment env(output);
        vars.convert(env.main_namespace());
        boost::python::exec(py_str(python_code), env.main_namespace());
        output.success = true;
    } catch (const boost::python::error_already_set&) {
        ErrorLog("py") << "Exception from python script";
        PyErr_Print();
    }

    return output;
}

ScriptOutput PythonEngine::exec_file(const std::string& file, const Converter& vars)
{
    if ( !initialize() )
        return ScriptOutput{};

    ScriptOutput output;

    try {
        ScriptEnvironment env(output);
        vars.convert(env.main_namespace());
        boost::python::exec_file(py_str(file), env.main_namespace());
        output.success = true;
    } catch (const boost::python::error_already_set&) {
        ErrorLog("py") << "Exception from python script";
        PyErr_Print();
    }

    return output;
}

bool PythonEngine::initialize()
{
    if ( !Py_IsInitialized() )
    {
        /// \todo custom module factory
        PyImport_AppendInittab( "melanobot", &python::melanobot::initmelanobot );
        Py_Initialize();
    }

    if ( !Py_IsInitialized() )
    {
        ErrorLog("py") << "Could not initialize the interpreter";
        return false;
    }

    return true;
}

} // namespace python
