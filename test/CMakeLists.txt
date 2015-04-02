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

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address --coverage -O0 -fno-inline -fno-inline-small-functions -fno-default-inline")

# Boost
find_package (Boost COMPONENTS unit_test_framework REQUIRED)
include_directories (${Boost_INCLUDE_DIRS})
add_definitions (-DBOOST_TEST_DYN_LINK)

#Tests
enable_testing()

function(melanotest test_name)
    target_link_libraries( ${test_name} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
    add_test (NAME ${test_name} COMMAND ${test_name})
endfunction(melanotest)

add_executable(test_string test_string.cpp
    ${CMAKE_SOURCE_DIR}/src/string/string_functions.cpp
    ${CMAKE_SOURCE_DIR}/src/string/language.cpp
)
melanotest(test_string)


add_executable(test_time test_time.cpp
    ${CMAKE_SOURCE_DIR}/src/time/time_string.cpp
    ${CMAKE_SOURCE_DIR}/src/string/language.cpp
)
melanotest(test_time)


# Coverage
set(TRACEFILE "${CMAKE_BINARY_DIR}/test/coverage.info")
add_custom_target(coverage
    COMMAND cd "${CMAKE_BINARY_DIR}/test"
    COMMAND ctest
    COMMAND lcov -c -d "${CMAKE_BINARY_DIR}/test" -b "${CMAKE_SOURCE_DIR}" -o ${TRACEFILE} --no-external
    COMMAND genhtml ${TRACEFILE} -o "${CMAKE_BINARY_DIR}/lconv" -p "${CMAKE_SOURCE_DIR}" --demangle-cpp
    COMMAND xdg-open "${CMAKE_BINARY_DIR}/lconv/index.html"
)
