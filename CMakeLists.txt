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

# CMake stuff
cmake_minimum_required(VERSION 2.6)
cmake_policy(SET CMP0012 NEW)

# Metadata
project(MelanoBot CXX)
set(PROJECT_VERSION 2.0)
string(TOLOWER MelanoBot EXECUTABLE_NAME)
set(MAINTAINER "Mattia Basaglia <mattia.basaglia@gmail.com>")
set(PROJECT_WEBSITE "https://wiki.evil-ant-colony.org/doku.php?id=melanobot:melanobot")

# CMake modules
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(misc)

# Flags
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -pedantic -Werror")
include_directories("${CMAKE_SOURCE_DIR}/src")

# Check C++14
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
use_cxx_standard(14)

# Enable Debug by default, can be changed with -D CMAKE_BUILD_TYPE=Release
if(CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE Debug)
    set(DEBUG 1)
else()
    set(DEBUG 0)
endif()

# Git describe
set(PROJECT_DEV_VERSION ${PROJECT_VERSION})
if(NOT GIT_FOUND)
    find_package(Git QUIET)
    if(GIT_FOUND)
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" describe --always
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            OUTPUT_VARIABLE PROJECT_DEV_VERSION
            ERROR_QUIET
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    endif()
endif()

# Config header
set(CONFIG_HEADER "${PROJECT_BINARY_DIR}/src/config.hpp")
configure_file ("${PROJECT_SOURCE_DIR}/src/config.hpp.in" ${CONFIG_HEADER})
include_directories("${CMAKE_BINARY_DIR}/src")

#Melano modules
set_property(GLOBAL PROPERTY MELANOMODULES "")
option (MODULE_ALL "Enable all modules" OFF)
macro(melanomodule name description default)
    string(TOUPPER "MODULE_${name}" OPTION_NAME)
    option (${OPTION_NAME} ${description} ${default})
    set(MODULE_NAME "melanomodule_${name}")
    if (${${OPTION_NAME}} OR ${MODULE_ALL})
        set(MODULE_ENABLED ON)
        set_property(GLOBAL APPEND PROPERTY MELANOMODULES ${MODULE_NAME})
        message(STATUS "Enabled module ${name}: ${description}")
    else()
        set(MODULE_ENABLED OFF)
    endif()
endmacro()

# Libraries

include_directories(${CMAKE_SOURCE_DIR}/lib/Melanolib/include)
add_definitions(-DMELANOLIB_BOOST_OPTIONAL -DMELANOLIB_BOOST_ANY)
set(BUILD_SHARED_MELANO_LIBS ON)
add_subdirectory(lib/Melanolib/src EXCLUDE_FROM_ALL)

add_subdirectory(modules lib)

get_property(MELANOMODULES GLOBAL PROPERTY MELANOMODULES)
if (NOT MELANOMODULES)
    message(STATUS "No modules enabled")
else()
    message(STATUS "Loaded Modules: ${MELANOMODULES}")
endif()

# Binary
add_subdirectory(src bin)

# Tests 
include(testing)
add_subdirectory(lib/Melanolib/test EXCLUDE_FROM_ALL)
add_subdirectory(test)

# Doxygen target
# Find all sources for documentation and stuff
set(ALL_SOURCE_DIRECTORIES src modules doc)

find_sources(ALL_SOURCES *.cpp *.hpp)
set(DOXYGEN_FOUND ON)
find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
    create_doxygen_target(doc)
endif(DOXYGEN_FOUND)

# CPack
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A not-so-simple IRC bot")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER ${MAINTAINER})
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/COPYING")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
install(DIRECTORY examples/ DESTINATION "share/${EXECUTABLE_NAME}/examples/" FILES_MATCHING PATTERN "*.info")
install(DIRECTORY scripts/ DESTINATION "share/${EXECUTABLE_NAME}/scripts/" )

set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE ${PROJECT_WEBSITE})
install(FILES "${CMAKE_SOURCE_DIR}/COPYING" DESTINATION "share/doc/${EXECUTABLE_NAME}" RENAME copyright)

include(CPack)

# Custom dist
set(DIST_ROOT "${PROJECT_NAME}-${PROJECT_DEV_VERSION}")
set(DIST_PACKAGE "${DIST_ROOT}.tar.gz")
add_custom_command(OUTPUT "${DIST_PACKAGE}"
    COMMAND rm -f "${CMAKE_BINARY_DIR}/${DIST_PACKAGE}"
    COMMAND git ls-files | xargs tar --transform="s,^,${DIST_ROOT}/," -caf "${CMAKE_BINARY_DIR}/${DIST_PACKAGE}"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
add_custom_target(dist DEPENDS "${DIST_PACKAGE}")

