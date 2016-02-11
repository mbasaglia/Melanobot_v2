/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015-2016 Mattia Basaglia
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
#include <sstream>

#include "format_flags.hpp"
#include "color.hpp"
#include "melanolib/singleton.hpp"
#include "melanolib/utils.hpp"

namespace string {

class Unicode;
class FormattedString;


/**
 * \brief Dummy class that represents a complete reset of all flags and colors
 */
class ClearFormatting{};

/**
 * \brief Simple ASCII string
 */
using AsciiString = std::string;

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
    class Registry : public melanolib::Singleton<Registry>
    {
    public:

        ~Registry()
        {
            for ( const auto& f : formatters )
                delete f.second;
        }

        /**
         * \brief Get a registered formatter
         */
        Formatter* formatter(const std::string& name);

        /**
         * \brief Register a formatter
         */
        void add_formatter(Formatter* instance);

    private:
        Registry();
        friend ParentSingleton;

        std::unordered_map<std::string,Formatter*> formatters;
        Formatter* default_formatter = nullptr;
    };

    Formatter() = default;
    Formatter(const Formatter&) = delete;
    Formatter(Formatter&&) = delete;
    Formatter& operator=(const Formatter&) = delete;
    Formatter& operator=(Formatter&&) = delete;

    virtual ~Formatter() {}

    /**
     * \brief Encode a simple ASCII string
     */
    virtual std::string to_string(const AsciiString& s) const
    {
        return s;
    }

    /**
     * \brief Encode a single ASCII character
     */
    virtual std::string to_string(char c) const
    {
        return to_string(std::string(1, c));
    }

    /**
     * \brief Encode a unicode (non-ASCII) character
     */
    virtual std::string to_string(const Unicode& c) const
    {
        return {};
    }

    /**
     * \brief Encode a color code
     * \todo Background colors?
     */
    virtual std::string to_string(const color::Color12& color) const
    {
        return {};
    }

    /**
     * \brief Encode format flags
     */
    virtual std::string to_string(FormatFlags flags) const
    {
        return {};
    }

    /**
     * \brief Clear all formatting
     */
    virtual std::string to_string(ClearFormatting) const
    {
        return {};
    }

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
