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

set(SOURCES
main.cpp
melanobot/config_factory.cpp
melanobot/handler.cpp
melanobot/melanobot.cpp
melanobot/storage.cpp
message/input_message.cpp
module/load_module.cpp
network/connection.cpp
network/async_service.cpp
settings.cpp
string/color.cpp
string/encoding.cpp
string/formatter.cpp
string/logger.cpp
string/string.cpp
string/format_flags.cpp
user/user.cpp
user/user_group.cpp
user/user_manager.cpp
)

set(EXECUTABLE_NAME melanobot) # repeated otherwise kdevelop messes up

add_executable(${EXECUTABLE_NAME} ${SOURCES})
install(TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION bin)

# Boost
find_package (Boost COMPONENTS system thread filesystem program_options REQUIRED)
target_link_libraries(${EXECUTABLE_NAME} ${Boost_LIBRARIES})
include_directories (${Boost_INCLUDE_DIRS})

find_package(Iconv QUIET)
if(Iconv_FOUND)
    add_definitions(-DHAS_ICONV)
    target_link_libraries(${EXECUTABLE_NAME} ${Iconv_LIBRARY})
include_directories (${Iconv_INCLUDE_DIR})
endif()

target_link_libraries(${EXECUTABLE_NAME} melano_stringutils melano_time melano_dynlib)
