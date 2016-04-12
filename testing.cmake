# Copyright 2015-2016 Mattia Basaglia
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

enable_testing()
add_custom_target(tests_compile
    COMMENT "Building all tests"
)

add_custom_target(tests_run
    COMMAND ctest -V
    DEPENDS tests_compile
    COMMENT "Running all tests"
)

set(TRACEFILE "${CMAKE_BINARY_DIR}/coverage.info")
set(COVERAGE_DIR "${CMAKE_BINARY_DIR}/lcov")
add_custom_target(tests_coverage
    COMMAND cd ${CMAKE_BINARY_DIR}
    COMMAND lcov -c -d "${CMAKE_CURRENT_BINARY_DIR}" -b "${CMAKE_SOURCE_DIR}" -o ${TRACEFILE} --no-external --rc lcov_branch_coverage=1
    COMMAND genhtml ${TRACEFILE} -o ${COVERAGE_DIR} -p "${CMAKE_SOURCE_DIR}" --demangle-cpp --branch-coverage
    DEPENDS tests_run
)

add_custom_target(tests_coverage_view
    COMMAND xdg-open "${COVERAGE_DIR}/index.html"
)
