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

# CMake stuff
cmake_minimum_required(VERSION 2.6)
cmake_policy(SET CMP0012 NEW)

# Metadata
project(MelanoBot CXX)
set(PROJECT_VERSION 2.0)
string(TOLOWER MelanoBot EXECUTABLE_NAME)
set(MAINTAINER "Mattia Basaglia <mattia.basaglia@gmail.com>")
set(PROJECT_WEBSITE "https://github.com/mbasaglia/Melanobot_v2")

# CMake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

# Flags
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -pedantic -Werror")
include_directories("${CMAKE_SOURCE_DIR}/src")

# Check C++14
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR CMAKE_COMPILER_IS_GNUCXX)
    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag(--std=c++14 STD_CXX14)
    if(STD_CXX14)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++14")
    else()
        message(SEND_ERROR "Requires C++14 or better")
    endif()
else()
    message(WARNING "Unrecognized compiler: ${CMAKE_CXX_COMPILER_ID}, make sure it supports C++14")
endif()

# Enable Debug by default, can be changed with -D CMAKE_BUILD_TYPE=Release
if(CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE Debug)
    set(DEBUG 1)
else()
    set(DEBUG 0)
endif()

# Config header
set(CONFIG_HEADER "${PROJECT_BINARY_DIR}/src/config.hpp")
configure_file ("${PROJECT_SOURCE_DIR}/src/config.hpp.in" ${CONFIG_HEADER})
include_directories("${CMAKE_BINARY_DIR}/src")

#Melano modules
set_property(GLOBAL PROPERTY MELANOMODULES "")
set_property(GLOBAL PROPERTY MELANOMODULES_INIT "")
set_property(GLOBAL PROPERTY MELANOMODULES_DECLARE "")
macro(melanomodule name description default)
    string(TOUPPER "MODULE_${name}" OPTION_NAME)
    option (${OPTION_NAME} ${description} ${default})
    set(MODULE_NAME "melanomodule_${name}")
    if (${${OPTION_NAME}})
        set_property(GLOBAL APPEND PROPERTY MELANOMODULES ${MODULE_NAME})
        set_property(GLOBAL APPEND_STRING PROPERTY MELANOMODULES_DECLARE "
Melanomodule ${MODULE_NAME}();")
        set_property(GLOBAL APPEND_STRING PROPERTY MELANOMODULES_INIT "\\
    modules.push_back(${MODULE_NAME}());")
        message(STATUS "Enabled module ${name}: ${description}")
    endif()
endmacro()

add_subdirectory(modules lib)

get_property(MELANOMODULES GLOBAL PROPERTY MELANOMODULES)
get_property(MELANOMODULES_INIT GLOBAL PROPERTY MELANOMODULES_INIT)
get_property(MELANOMODULES_DECLARE GLOBAL PROPERTY MELANOMODULES_DECLARE)
if (NOT MELANOMODULES)
    message(STATUS "No modules enabled")
else()
    message(STATUS "Loaded Modules: ${MELANOMODULES}")
endif()

set(MELANOMODULE_HEADER "${PROJECT_BINARY_DIR}/src/melanomodules.hpp")
configure_file ("${PROJECT_SOURCE_DIR}/src/melanomodules.hpp.in" ${MELANOMODULE_HEADER})

# Binary
add_subdirectory(src bin)

# Tests 
add_subdirectory(test)

# Doxygen target
find_package(Doxygen)
if(DOXYGEN_FOUND)

    if ( ${CMAKE_BINARY_DIR} STREQUAL ${CMAKE_SOURCE_DIR} )
        set(DOXYGEN_OUTPUT ${CMAKE_BINARY_DIR}/doc-out)
    else()
        set(DOXYGEN_OUTPUT ${CMAKE_BINARY_DIR}/doc)
    endif()

    set(DOXYGEN_INPUT "${CONFIG_HEADER} ${MELANOMODULE_HEADER}")

    file(GLOB_RECURSE DOXYGEN_INPUT_FILES "doc/*.dox" "src/*.cpp" "src/*.hpp" "modules/*.cpp" "modules/*.hpp" )
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
