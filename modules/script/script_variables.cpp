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
#include "script_variables.hpp"
#include "python-utils.hpp"
#include "handlers.hpp"

namespace python {

void Converter::convert(const PropertyTree& input, boost::python::object& output)
{
    for ( const auto& child : input )
    {
        if ( child.second.empty() )
            output[py_str(child.first)] = py_str(child.second.data());
        else
        {
            boost::python::dict child_object;
            convert(child.second, child_object);
            output[py_str(child.first)] = child_object;
        }
    }
}

void Converter::convert(const std::string& input, boost::python::object& output)
{
    output = py_str(input);
}

void Converter::convert(const std::vector<std::string>& input, boost::python::object& output)
{
    boost::python::list result;
    for (const auto& s : input)
        result.append(s);
    output = result;
}

void MessageVariables::convert(boost::python::object& target_namespace) const
{
    boost::python::import("melanobot");
    target_namespace["message"] = boost::ref(message);
    /// \todo message.source (with pretty_properties)
}

void SimpleScript::Variables::convert(boost::python::object& target_namespace) const
{
    using namespace boost::python;
    MessageVariables::convert(target_namespace);
    import("melanobot");
    target_namespace["formatter"] = ptr(obj->formatter);
}

void StructuredScript::Variables::convert(boost::python::object& target_namespace) const
{
    SimpleScript::Variables::convert(target_namespace);
    boost::python::dict sett;
    Converter::convert(static_cast<StructuredScript*>(obj)->settings, sett);
    target_namespace["settings"] = sett;
}

} // namespace python
