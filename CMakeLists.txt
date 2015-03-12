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
project(MelanoBot CXX)

set(PROJECT_VERSION 2.0)
string(TOLOWER MelanoBot EXECUTABLE_NAME)
set(MAINTAINER "Mattia Basaglia <mattia.basaglia@gmail.com>")
set(PROJECT_WEBSITE "https://github.com/mbasaglia/Melanobot_v2")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

set(ALL_SOURCES "")

add_subdirectory(src bin)

find_package(Doxygen)
if(DOXYGEN_FOUND)

    if ( ${CMAKE_BINARY_DIR} STREQUAL ${CMAKE_SOURCE_DIR} )
        set(DOXYGEN_OUTPUT ${CMAKE_BINARY_DIR}/doc-out)
    else()
        set(DOXYGEN_OUTPUT ${CMAKE_BINARY_DIR}/doc)
    endif()

    file(GLOB DOX_FILES doc/*.dox)
    set(DOXYGEN_INPUT_FILES ${ALL_SOURCES} ${DOX_FILES})
    foreach(source ${DOXYGEN_INPUT_FILES})
        set(DOXYGEN_INPUT "${DOXYGEN_INPUT} ${source}")
    endforeach()

    configure_file(${CMAKE_SOURCE_DIR}/Doxyfile.in ${PROJECT_BINARY_DIR}/Doxyfile)
    add_custom_target(doc
        ${DOXYGEN_EXECUTABLE} ${PROJECT_BINARY_DIR}/Doxyfile
        DEPENDS ${PROJECT_BINARY_DIR}/Doxyfile ${DOXYGEN_INPUT_FILES}
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
        COMMENT "Generating Doxygen Documentation" VERBATIM
    )
    add_custom_target(doc-view
        xdg-open ${DOXYGEN_OUTPUT}/html/index.html
        COMMENT "Showing Doxygen documentation"
    )
    add_custom_target(doc-commit
        COMMAND "${CMAKE_SOURCE_DIR}/commit-doc.sh" "${DOXYGEN_OUTPUT}/html"
        COMMENT "Committing Doxygen documentation"
    )
endif(DOXYGEN_FOUND)
