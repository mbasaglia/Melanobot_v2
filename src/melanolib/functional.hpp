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
#ifndef FUNCTIONAL_HPP
#define FUNCTIONAL_HPP

#include <functional>
#include <utility>

namespace melanolib {

/**
 * \brief Call a std::function when it is properly initialized
 * \tparam Functor  Callable which can be converted to bool
 * \tparam CallArgs Argument types used at the call point
 * \param  function Function object
 * \param  args     Function arguments
 */
template<class Functor, class... CallArgs>
    void callback(const Functor& function, CallArgs&&... args)
    {
        if ( function )
            function(std::forward<CallArgs>(args)...);
    }

} // namespace melanolib
#endif // FUNCTIONAL_HPP
