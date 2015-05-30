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
#ifndef SCRIPT_PYTHON_HPP
#define SCRIPT_PYTHON_HPP

#include <vector>
#include <string>
#include "settings.hpp"

/**
 * \brief Namespace for Python-related functionality
 */
namespace python {

/**
 * \brief Object containing the result of a script
 */
struct ScriptOutput
{
    std::vector<std::string>    output;    ///< Lines written to stdout
    bool                        success{0};///< Whether the script has run successfully
};

/**
 * \brief Scripting engine for python
 */
class PythonEngine
{
public:
    /**
     * \brief Executes some python code
     * \param python_code Python sources to execute
     * \param dict        Properties to be added to the main dict
     */
    ScriptOutput exec(const std::string& python_code, const PropertyTree& dict = {});

    /**
     * \brief Executes some python code from a file
     * \param file File containing python sources
     * \param dict Properties to be added to the main dict
     */
    ScriptOutput exec_file(const std::string& file, const PropertyTree& dict = {});

    /**
     * \brief Singleton instance
     */
    static PythonEngine& instance()
    {
        static PythonEngine singleton;
        return singleton;
    }

private:
    PythonEngine() {}
    PythonEngine(const PythonEngine&) = delete;

    /**
     * \brief Initializes the interpreter
     * \return Whether the interpreter has been initialized
     */
    bool initialize();
};

} // namespace python
#endif // SCRIPT_PYTHON_HPP
