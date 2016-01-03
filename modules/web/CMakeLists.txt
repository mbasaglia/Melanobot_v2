# Copyright (C) 2015 Mattia Basaglia
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
melanomodule(web "Web stuff (HTTP and friends)" ON)

set(SOURCES
http.cpp
web.cpp
web-api-concrete.cpp
)

# cpr
# set(CPR_VERSION 1.2.0)
include(ExternalProject)
ExternalProject_Add(cpr_sources
    PREFIX lib
    GIT_REPOSITORY https://github.com/whoshuu/cpr.git
#    GIT_TAG ${CPR_VERSION}
    GIT_SUBMODULES opt/mongoose
    CMAKE_ARGS -DBUILD_CPR_TESTS=OFF -DUSE_SYSTEM_CURL=ON -DBUILD_CURL_TESTS=OFF -DCMAKE_CXX_FLAGS=-fPIC
    INSTALL_COMMAND ""
)
set(CPR_ROOT ${CMAKE_CURRENT_BINARY_DIR}/lib/src/cpr_sources)
include_directories(${CPR_ROOT}/include)
link_directories(${CPR_ROOT}-build/lib)


add_library(${MODULE_NAME} MODULE ${SOURCES})


# cpr [cont]
# target_link_libraries(${MODULE_NAME} cpr)
target_link_libraries(${MODULE_NAME} ${CPR_ROOT}-build/lib/libcpr.a)
add_dependencies(${MODULE_NAME} cpr_sources)

# cURL
find_package (cURL REQUIRED)
target_link_libraries(${MODULE_NAME} ${cURL_LIBRARIES})
include_directories (${cURL_INCLUDE_DIRS})
