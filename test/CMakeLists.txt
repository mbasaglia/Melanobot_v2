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

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address")

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

add_executable(test_string test_string.cpp ${CMAKE_SOURCE_DIR}/src/string/string_functions.cpp)
melanotest(test_string)


add_executable(test_time test_time.cpp)
melanotest(test_time)
