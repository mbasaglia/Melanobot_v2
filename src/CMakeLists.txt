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

set(SOURCES
message/input_message.cpp
user/user.cpp
user/user_group.cpp
user/user_manager.cpp
storage.cpp
melanolib/library.cpp
melanolib/time/time_string.cpp
melanolib/time/time.cpp
melanolib/math.cpp
melanolib/string/language.cpp
melanolib/string/stringutils.cpp
load_module.cpp
handler/handler.cpp
settings.cpp
main.cpp
melanobot.cpp
string/encoding.cpp
string/formatter.cpp
string/string.cpp
string/logger.cpp
string/color.cpp
network/connection.cpp
)

set(EXECUTABLE_NAME melanobot) # repeated otherwise kdevelop messes up

add_executable(${EXECUTABLE_NAME} ${SOURCES})
install(TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION bin)

# Boost
find_package (Boost COMPONENTS system thread filesystem program_options REQUIRED)
target_link_libraries(${EXECUTABLE_NAME} ${Boost_LIBRARIES})
include_directories (${Boost_INCLUDE_DIRS})

find_package(Iconv)
if(Iconv_FOUND)
    add_definitions(-DHAS_ICONV)
endif()

target_link_libraries(${EXECUTABLE_NAME} ${CMAKE_DL_LIBS})
