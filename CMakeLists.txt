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

cmake_minimum_required(VERSION 2.6)
project(MelanoBot)

string(TOLOWER MelanoBot EXECUTABLE_NAME)
set (VERSION_MAJOR 2)
set (VERSION_MINOR 0)
set (MAINTAINER "Mattia Basaglia <mattia.basaglia@gmail.com>")
set(PROJECT_WEBSITE "https://github.com/mbasaglia/Melanobot_v2")

set(ALL_SOURCES "")
add_subdirectory(src)

find_package(Doxygen)
if(DOXYGEN_FOUND)
    set(DOXYGEN_OUTPUT doc)
    foreach(source ${ALL_SOURCES})
	set(DOXYGEN_INPUT "${DOXYGEN_INPUT} ${source}")
    endforeach()
    configure_file(${CMAKE_SOURCE_DIR}/Doxyfile.in ${PROJECT_BINARY_DIR}/Doxyfile)
    add_custom_target(doc
	    ${DOXYGEN_EXECUTABLE} ${PROJECT_BINARY_DIR}/Doxyfile
	    DEPENDS ${PROJECT_BINARY_DIR}/Doxyfile ${ALL_SOURCES}
	    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
	    COMMENT "Generating Doxygen Documentation" VERBATIM
    )
    add_custom_target(doc-view
	    xdg-open ${DOXYGEN_OUTPUT}/html/index.html
	    COMMENT "Showing Doxygen documentation"
    )
endif(DOXYGEN_FOUND)
