# Copyright (C) 2015 Mattia Basaglia
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
melanomodule(script "Scripting interface" ON)

file(GLOB_RECURSE SOURCES *.cpp)

add_library(${MODULE_NAME} ${SOURCES})

# Boost
find_package (Boost COMPONENTS python REQUIRED)
target_link_libraries(${MODULE_NAME} ${Boost_LIBRARIES})
include_directories (${Boost_INCLUDE_DIRS})

# CPython
find_package(PythonLibs)
target_link_libraries(${MODULE_NAME} ${PYTHON_LIBRARIES})
include_directories (${PYTHON_INCLUDE_DIRS})

