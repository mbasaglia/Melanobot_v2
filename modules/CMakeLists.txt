# Copyright 2015-2016 Mattia Basaglia
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

include_directories("${CMAKE_SOURCE_DIR}/modules")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/melanobot")

add_subdirectory(core)
add_subdirectory(file_storage)
add_subdirectory(fun)
add_subdirectory(irc)
add_subdirectory(lists)
add_subdirectory(posix)
add_subdirectory(script)
add_subdirectory(timer)
add_subdirectory(web)
add_subdirectory(xonotic)
