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

include_directories("${CMAKE_SOURCE_DIR}/modules")

#modules     ID         Desctiption                             Default
melanomodule(irc        "Internet Relay Chat support"           ON)
melanomodule(xonotic    "Xonotic integration"                   ON)
melanomodule(posix      "Extra features for POSIX systems"      OFF)
melanomodule(web        "Web stuff (HTTP and friends)"          ON)
melanomodule(fun        "Possibly useless yet silly stuff"      ON)
