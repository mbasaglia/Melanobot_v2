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
import sys, socket, string
try:
    import geoip2.database
except ImportError:
    print "You need the python module geoip2"
    raise

try:
    database = settings["database"];
    reader = geoip2.database.Reader(database)
except Exception:
    print "Error reading the GeoIP Database"
    raise

try:
    address = message.message.strip()
    resolved_address = socket.gethostbyaddr(address)[2][0]

    properties = {};

    if "City.mmdb" in database:

        response = reader.city(resolved_address)
        properties["location.longitude"] = response.location.longitude
        properties["location.latitude"] = response.location.latitude
        properties["city.name"]         = response.city.name

    else:
        response = reader.country(resolved_address)

    properties["address"]               = address
    properties["resolved"]              = resolved_address
    properties["continent.code"]        = response.continent.code
    properties["continent.name"]        = response.continent.name
    properties["country.iso_code"]      = response.country.iso_code
    properties["country.name"]          = response.country.name

    class Template(string.Template):
        delimiter = '%'
        idpattern = r'[_a-z0-9.]*'

    print Template(settings["format"]).safe_substitute(properties)

except Exception:
    print "Unable to locate the given address ("+address+")"
    raise
