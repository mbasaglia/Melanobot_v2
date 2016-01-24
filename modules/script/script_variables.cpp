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


void Converter::convert(const Properties& input, boost::python::object& output)
{
    for ( const auto& child : input )
        output[py_str(child.first)] = py_str(child.second);
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

void Converter::convert(const boost::python::object& input, std::vector<std::string>& output)
{
    using namespace boost::python;
    output.resize(len(input));
    for ( int i = 0; i < len(input); i++ )
        output[i] = to_string(input[i]);
}

void Converter::convert(const boost::python::object& input, Properties& output)
{
    using namespace boost::python;
    output.clear();
    auto keys = input.attr("keys")();
    auto keylen = len(keys);
    for ( int i = 0; i < keylen; i++ )
    {
        auto key = keys[i];
        output[to_string(key)] = to_string(input[key]);
    }
}

void MessageVariables::convert(boost::python::object& target_namespace) const
{
    boost::python::import("melanobot");
    target_namespace["message"] = boost::ref(message);
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

std::string Converter::to_string(const boost::python::object& input)
{
    return boost::python::extract<std::string>(input.attr("__str__")());
}

} // namespace python
