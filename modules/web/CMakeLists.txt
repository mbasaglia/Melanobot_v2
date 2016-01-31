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
melanomodule(web "Web stuff (HTTP and friends)" ON)

set(SOURCES
http.cpp
web.cpp
web-api-concrete.cpp
)

if(${MODULE_ENABLED})
    # CPR can be either downloaded automatically or these variables must be set:
    # CPR_CUSTOM=ON
    # CPR_INCLUDE=/path/to/include (contains cpr/cpr.h)
    # CPR_LIB=/path/to/lib (contains libcpr.a)

    if(NOT CPR_CUSTOM)
        # set(CPR_VERSION 1.2.0)
        include(ExternalProject)
        ExternalProject_Add(cpr_sources
            PREFIX lib
            GIT_REPOSITORY https://github.com/whoshuu/cpr.git
            # GIT_TAG ${CPR_VERSION}
            GIT_SUBMODULES opt/mongoose
            CMAKE_ARGS -DBUILD_CPR_TESTS=OFF -DUSE_SYSTEM_CURL=ON -DBUILD_CURL_TESTS=OFF -DCMAKE_CXX_FLAGS=-fPIC -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            INSTALL_COMMAND ""
        )
        set(CPR_ROOT ${CMAKE_CURRENT_BINARY_DIR}/lib/src/cpr_sources)
        set(CPR_INCLUDE ${CPR_ROOT}/include)
        set(CPR_LIB ${CPR_ROOT}-build/lib)
    endif()

    include_directories(${CPR_INCLUDE})
    link_directories(${CPR_LIB})


    add_library(${MODULE_NAME} SHARED ${SOURCES})


    # cpr [cont]
    # target_link_libraries(${MODULE_NAME} cpr)
    target_link_libraries(${MODULE_NAME} ${CPR_LIB}/libcpr.a)
    if(NOT CPR_CUSTOM)
        add_dependencies(${MODULE_NAME} cpr_sources)
    endif()

    # cURL
    find_package (cURL REQUIRED)
    target_link_libraries(${MODULE_NAME} ${cURL_LIBRARIES})
    include_directories (${cURL_INCLUDE_DIRS})
endif()
