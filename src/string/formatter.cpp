/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015 Mattia Basaglia
 * \section License
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "formatter.hpp"
#include "logger.hpp"

namespace string {


Formatter::Registry::Registry()
{
    add_formatter(default_formatter = new FormatterUtf8);
    add_formatter(new FormatterAscii);
    add_formatter(new FormatterAnsi(true));
    add_formatter(new FormatterAnsi(false));
    add_formatter(new FormatterConfig);
    add_formatter(new FormatterAnsiBlack(true));
    add_formatter(new FormatterAnsiBlack(false));
}

Formatter* Formatter::Registry::formatter(const std::string& name)
{
    auto it = formatters.find(name);
    if ( it == formatters.end() )
    {
        ErrorLog("sys") << "Invalid formatter: " << name;
        if ( default_formatter )
            return default_formatter;
        CRITICAL_ERROR("Trying to access an invalid formatter");
    }
    return it->second;
}

void Formatter::Registry::add_formatter(Formatter* instance)
{
    if ( formatters.count(instance->name()) )
        ErrorLog("sys") << "Overwriting formatter: " << instance->name();
    formatters[instance->name()] = instance;
    if ( ! default_formatter )
        default_formatter = instance;
}

} // namespace string
