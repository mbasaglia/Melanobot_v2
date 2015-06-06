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
include(CTest)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address --coverage -O0 -fno-inline")
include_directories("${CMAKE_SOURCE_DIR}/modules")

set(ENABLE_TESTS ON)

# Boost
find_package (Boost COMPONENTS unit_test_framework)
if(Boost_FOUND)
    include_directories (${Boost_INCLUDE_DIRS})
    add_definitions (-DBOOST_TEST_DYN_LINK)
else()
    set(ENABLE_TESTS OFF)
endif()

# Iconv
find_package(Iconv)
if(Iconv_FOUND)
    add_definitions(-DHAS_ICONV)
endif()

#Tests
if (ENABLE_TESTS)
message(STATUS "Test targets enabled")
enable_testing()
set(TEST_TARGETS "")
function(melanotest test_name)
    target_link_libraries( ${test_name} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
    add_test (NAME ${test_name} COMMAND ${test_name})
    set(TEST_TARGETS ${TEST_TARGETS} ${test_name} PARENT_SCOPE)
endfunction(melanotest)

add_executable(test_string EXCLUDE_FROM_ALL test_string.cpp
    ${CMAKE_SOURCE_DIR}/src/string/string_functions.cpp
    ${CMAKE_SOURCE_DIR}/src/string/language.cpp
)
melanotest(test_string)

add_executable(test_time EXCLUDE_FROM_ALL test_time.cpp
    ${CMAKE_SOURCE_DIR}/src/string/language.cpp
    ${CMAKE_SOURCE_DIR}/src/string/string_functions.cpp
    ${CMAKE_SOURCE_DIR}/src/time/time.cpp
    ${CMAKE_SOURCE_DIR}/src/time/time_string.cpp
)
melanotest(test_time)

add_executable(test_formatter EXCLUDE_FROM_ALL test_formatter.cpp
    ${CMAKE_SOURCE_DIR}/src/string/string.cpp
    ${CMAKE_SOURCE_DIR}/modules/xonotic/formatter.cpp
    ${CMAKE_SOURCE_DIR}/modules/irc/irc_formatter.cpp
    ${CMAKE_SOURCE_DIR}/modules/fun/rainbow.cpp
    ${CMAKE_SOURCE_DIR}/src/string/color.cpp
    ${CMAKE_SOURCE_DIR}/src/string/string_functions.cpp
    ${CMAKE_SOURCE_DIR}/src/string/logger.cpp
    ${CMAKE_SOURCE_DIR}/src/time/time.cpp
    ${CMAKE_SOURCE_DIR}/src/time/time_string.cpp
    ${CMAKE_SOURCE_DIR}/src/string/language.cpp
    ${CMAKE_SOURCE_DIR}/src/string/encoding.cpp
    ${CMAKE_SOURCE_DIR}/src/string/formatter.cpp
)
melanotest(test_formatter)

# Coverage
add_custom_target(test_all
    DEPENDS ${TEST_TARGETS}
    COMMENT "Building all tests"
)

set(TRACEFILE "${CMAKE_BINARY_DIR}/test/coverage.info")
add_custom_target(coverage
    COMMAND cd "${CMAKE_BINARY_DIR}/test"
    COMMAND ctest -V
    COMMAND lcov -c -d "${CMAKE_BINARY_DIR}/test" -b "${CMAKE_SOURCE_DIR}" -o ${TRACEFILE} --no-external
    COMMAND genhtml ${TRACEFILE} -o "${CMAKE_BINARY_DIR}/lconv" -p "${CMAKE_SOURCE_DIR}" --demangle-cpp
    DEPENDS ${TEST_TARGETS}
)

add_custom_target(coverage_view
    COMMAND xdg-open "${CMAKE_BINARY_DIR}/lconv/index.html"
)

endif()
