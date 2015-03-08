/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright  Mattia Basaglia
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
#ifndef HANDLER_CONTAINER_HPP
#define HANDLER_CONTAINER_HPP

#include "settings.hpp"

namespace handler {

class Handler;

/**
 * \brief Base class for classes which may contain handlers
 */
class HandlerContainer
{
public:
    virtual ~HandlerContainer() {}

    /**
     * \brief Populates \c output from properties of its children
     */
    virtual void populate_properties(const std::vector<std::string>& properties, PropertyTree& output) const = 0;

};


} // namespace handler
#endif // HANDLER_CONTAINER_HPP
