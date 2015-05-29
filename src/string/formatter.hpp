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
#ifndef STRING_FORMATTER_HPP
#define STRING_FORMATTER_HPP

#include <unordered_map>

#include "format_flags.hpp"
#include "color.hpp"
#include "encoding.hpp"

namespace string {

class Unicode;
class QFont;
class FormattedString;

/**
 * \brief Abstract formatting visitor (and factory)
 * \note Formatters should register here
 */
class Formatter
{
public:
    /**
     * \brief Internal factory
     *
     * Factory needs a singleton, Formatter is abstract so it needs a different
     * class, but Formatter is the place to go to access the factory.
     *
     * Also note that this doesn't really create objects since they don't need
     * any data, so the objects are created only once.
     */
    class Registry
    {
    public:
        static Registry& instance()
        {
            static Registry factory;
            return factory;
        }

        ~Registry()
        {
            for ( const auto& f : formatters )
                delete f.second;
        }

        /**
         * \brief Get a registered formatter
         * \throws CriticalException If the formatter doesn't exist
         */
        Formatter* formatter(const std::string& name);

        /**
         * \brief Register a formatter
         */
        void add_formatter(Formatter* instance);

        std::unordered_map<std::string,Formatter*> formatters;
        Formatter* default_formatter = nullptr;
    private:
        Registry();
        Registry(const Registry&) = delete;
        Registry(Registry&&) = delete;
        Registry& operator=(const Registry&) = delete;
        Registry& operator=(Registry&&) = delete;
    };

    Formatter() = default;
    Formatter(const Formatter&) = delete;
    Formatter(Formatter&&) = delete;
    Formatter& operator=(const Formatter&) = delete;
    Formatter& operator=(Formatter&&) = delete;

    virtual ~Formatter() {}
    /**
     * \brief Encode a single ASCII character
     */
    virtual std::string ascii(char c) const = 0;
    /**
     * \brief Encode a simple ASCII string
     */
    virtual std::string ascii(const std::string& s) const = 0;
    /**
     * \brief Encode a color code
     * \todo Background colors?
     */
    virtual std::string color(const color::Color12& color) const = 0;
    /**
     * \brief Encode format flags
     */
    virtual std::string format_flags(FormatFlags flags) const = 0;
    /**
     * \brief Encode a unicode (non-ASCII) character
     */
    virtual std::string unicode(const Unicode& c) const = 0;
    /**
     * \brief Encode a Darkpaces weird character
     */
    virtual std::string qfont(const QFont& c) const = 0;

    /**
     * \brief Clear all formatting
     */
    virtual std::string clear() const = 0;

    /**
     * \brief Decode a string
     */
    virtual FormattedString decode(const std::string& source) const = 0;

    /**
     * \brief Name of the format
     */
    virtual std::string name() const = 0;

    /**
     * \brief Get a string formatter from name
     */
    static Formatter* formatter(const std::string& name)
    {
        return Registry::instance().formatter(name);
    }

    /**
     * \brief Get the formatter registry singleton
     */
    static Registry& registry() { return Registry::instance(); }
};

} // namespace string
#endif // STRING_FORMATTER_HPP
