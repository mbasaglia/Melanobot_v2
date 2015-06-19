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
#ifndef SCRIPT_VARIABLES_HPP
#define SCRIPT_VARIABLES_HPP

#include "settings.hpp"
#include "message/input_message.hpp"
#include <boost/python/object_fwd.hpp>

namespace python {

/**
 * \brief Class that knows how to convert C++ Objects to Python objects
 */
class Converter
{
public:
    virtual ~Converter() {}

    /**
     * \brief Converts the data members to python
     */
    virtual void convert(boost::python::object& target_namespace) const {};

    /**
     * \brief Recursively converts a PropertyTree to a dict
     */
    static void convert(const PropertyTree& input, boost::python::object& output);

    /**
     * \brief Converts a string
     */
    static void convert(const std::string& input, boost::python::object& output);

    /**
     * \brief Converts a vector of strings
     */
    static void convert(const std::vector<std::string>& input, boost::python::object& output);

    /**
     * \brief Converts a vector of strings
     */
    static void convert(const boost::python::object& input, std::vector<std::string>& output);
};

/**
 * \brief Handles variables needed by a message environment
 */
class MessageVariables : public Converter
{
public:
    explicit MessageVariables(network::Message& message) : message(message) {}

    void convert(boost::python::object& target_namespace) const override;

private:
    network::Message& message;
};

} // namespace python
#endif // SCRIPT_VARIABLES_HPP
