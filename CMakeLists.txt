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
set(PROJECT_WEBSITE "https://wiki.evil-ant-colony.org/doku.php?id=melanobot:melanobot")

# CMake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

# Flags
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -pedantic -Werror")
include_directories("${CMAKE_SOURCE_DIR}/src")

# Check C++14
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(${CMAKE_VERSION} LESS 3.2)
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR CMAKE_COMPILER_IS_GNUCXX)
        include(CheckCXXCompilerFlag)
        check_cxx_compiler_flag(--std=c++${CMAKE_CXX_STANDARD} STD_CXX)
        if(STD_CXX)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++${CMAKE_CXX_STANDARD}")
        else()
            message(SEND_ERROR "Requires C++${CMAKE_CXX_STANDARD} or better")
        endif()
    else()
        message(WARNING "Unrecognized compiler: ${CMAKE_CXX_COMPILER_ID}, make sure it supports C++${CMAKE_CXX_STANDARD}")
    endif()
endif()

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
set_property(GLOBAL PROPERTY MELANOMODULES_INIT "")
set_property(GLOBAL PROPERTY MELANOMODULES_DECLARE "")
macro(melanomodule name description default)
    string(TOUPPER "MODULE_${name}" OPTION_NAME)
    option (${OPTION_NAME} ${description} ${default})
    set(MODULE_NAME "melanomodule_${name}")
    if (${${OPTION_NAME}})
        set_property(GLOBAL APPEND PROPERTY MELANOMODULES ${MODULE_NAME})
        set_property(GLOBAL APPEND_STRING PROPERTY MELANOMODULES_DECLARE "
Melanomodule ${MODULE_NAME}(const Settings&);")
        set_property(GLOBAL APPEND_STRING PROPERTY MELANOMODULES_INIT "\\
    modules.push_back(${MODULE_NAME}(settings));")
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
        set(DOXYGEN_OUTPUT "\"${CMAKE_BINARY_DIR}/doc-out\"")
    else()
        set(DOXYGEN_OUTPUT "\"${CMAKE_BINARY_DIR}/doc\"")
    endif()

    set(DOXYGEN_INPUT "\"${CONFIG_HEADER}\" \"${MELANOMODULE_HEADER}\"")

    file(GLOB_RECURSE DOXYGEN_INPUT_FILES "doc/*.dox" "src/*.cpp" "src/*.hpp" "modules/*.cpp" "modules/*.hpp" )
    foreach(source ${DOXYGEN_INPUT_FILES})
        set(DOXYGEN_INPUT "${DOXYGEN_INPUT} \"${source}\"")
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

