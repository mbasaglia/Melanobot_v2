# Copyright 2015-2017 Mattia Basaglia
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
melanomodule(github "GitHub integration" ON)

set(SOURCES
github.cpp
github-controller.cpp
event_source.cpp
)

if(${MODULE_ENABLED})
    add_library(${MODULE_NAME} MODULE ${SOURCES})
    target_link_libraries(${MODULE_NAME} melanomodule_web melano_stringutils melano_time)
endif()

