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

include_directories("${CMAKE_SOURCE_DIR}/modules")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
link_directories(${CMAKE_CURRENT_BINARY_DIR})

file(GLOB SUBDIRS RELATIVE "${CMAKE_SOURCE_DIR}/modules" ${CMAKE_SOURCE_DIR}/modules/*)
foreach(subdir ${SUBDIRS})
    if(IS_DIRECTORY "${CMAKE_SOURCE_DIR}/modules/${subdir}")
        add_subdirectory(${subdir})
    endif()
endforeach()

target_link_libraries(${MODULE_NAME} melanomodule_core)
