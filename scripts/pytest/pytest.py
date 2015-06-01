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

# This scripts runs other python scripts installed in the bot data

import melanobot
import __main__
import sys, re

match = re.match(r"^\s*((?:(?:\S+)|(?:\"[^\"]+\")).py)\s*(.*)$",message.message)
if not match:
    print "Script name please"
    sys.exit(1)

script_name = match.group(1);
script_file = melanobot.data_file("scripts/"+script_name)
if not script_file:
    print script_name+" not found"
    sys.exit(0);

sys.stderr.write("Running "+script_file+"\n")
message.message = match.group(2)
execfile(script_file,__main__.__dict__)
