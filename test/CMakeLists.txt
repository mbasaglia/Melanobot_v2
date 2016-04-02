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
include(CTest)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address --coverage -O0 -fno-inline")
include_directories("${CMAKE_SOURCE_DIR}/modules")


# Boost
find_package (Boost COMPONENTS unit_test_framework)
if(Boost_FOUND)
    include_directories (${Boost_INCLUDE_DIRS})
    add_definitions (-DBOOST_TEST_DYN_LINK)
    set(BUILD_TESTING ON)
else()
    set(BUILD_TESTING OFF)
endif()

# Iconv
find_package(Iconv)
if(Iconv_FOUND)
    add_definitions(-DHAS_ICONV)
endif()

#Tests
if (BUILD_TESTING)
message(STATUS "Test targets enabled")
enable_testing()

function(melanotest test_name)
    add_executable(${test_name} EXCLUDE_FROM_ALL ${test_name}.cpp ${ARGN})
    target_link_libraries(${test_name} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
    add_test (NAME ${test_name} COMMAND ${test_name})
    add_dependencies(test_all ${test_name})
endfunction(melanotest)


melanotest(test_formatter
    ${CMAKE_SOURCE_DIR}/src/string/string.cpp
    ${CMAKE_SOURCE_DIR}/src/string/format_flags.cpp
    ${CMAKE_SOURCE_DIR}/modules/xonotic/xonotic-formatter.cpp
    ${CMAKE_SOURCE_DIR}/modules/irc/irc_formatter.cpp
    ${CMAKE_SOURCE_DIR}/modules/fun/rainbow.cpp
    ${CMAKE_SOURCE_DIR}/src/string/color.cpp
    ${CMAKE_SOURCE_DIR}/src/string/logger.cpp
    ${CMAKE_SOURCE_DIR}/src/string/encoding.cpp
    ${CMAKE_SOURCE_DIR}/src/string/formatter.cpp
)
target_link_libraries(test_formatter melano_time melano_stringutils)

melanotest(test_json
    ${CMAKE_SOURCE_DIR}/src/string/encoding.cpp
)
target_link_libraries(test_json melano_stringutils)

endif()
