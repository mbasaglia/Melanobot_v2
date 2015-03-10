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

file(GLOB_RECURSE SOURCES *.cpp)
file(GLOB_RECURSE HEADERS *.hpp)

configure_file (
  "${PROJECT_SOURCE_DIR}/src/config.hpp.in"
  "${PROJECT_BINARY_DIR}/src/config.hpp"
)
set(HEADERS ${HEADERS} "${PROJECT_BINARY_DIR}/src/config.hpp")
include_directories("${PROJECT_BINARY_DIR}/src")

set(ALL_SOURCES ${ALL_SOURCES} ${SOURCES} ${HEADERS} PARENT_SCOPE)

set(EXECUTABLE_NAME melanobot) # repeated otherwise kdevelop messes up

# Flags
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -pedantic -Werror")
include_directories("${CMAKE_SOURCE_DIR}/src")

# Check C++14
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


add_executable(${EXECUTABLE_NAME} ${SOURCES})
install(TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION bin)

# Boost
find_package (Boost COMPONENTS system chrono thread filesystem program_options REQUIRED)
target_link_libraries(${EXECUTABLE_NAME} ${Boost_LIBRARIES})
include_directories (${Boost_INCLUDE_DIRS})

# cURLpp
target_link_libraries(${EXECUTABLE_NAME} curlpp curl)
