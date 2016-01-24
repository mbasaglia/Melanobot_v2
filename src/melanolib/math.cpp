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
#include "melanolib/math.hpp"

#include <random>

namespace melanolib {
namespace math {

thread_local static std::random_device random_device;  ///< PRNG device

long random()
{
    return std::uniform_int_distribution<long>()(random_device);
}

long random(long max)
{
    return random(0,max);
}

long random(long min, long max)
{
    return std::uniform_int_distribution<long>(min,max)(random_device);
}

double random_real()
{
    thread_local static std::uniform_real_distribution<double> dist;
    return dist(random_device);
}

} // namespace math

} // namespace melanolib
