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
#ifndef MELANOBOT_STRING_REPLACEMENTS_HPP
#define MELANOBOT_STRING_REPLACEMENTS_HPP
#include "string.hpp"
#include "melanolib/string/stringutils.hpp"

namespace string {

/**
 * \brief Element that is used for replacements
 */
class Placeholder
{
public:
    Placeholder(std::string identifier, FormattedString replacement = {})
        : identifier(std::move(identifier)),
          replacement(std::move(replacement))
    {}

    std::string to_string(const Formatter& formatter, Formatter::Context* context) const
    {
        return replacement.encode(formatter, context);
    }

    void replace(const ReplacementFunctor& func)
    {
        auto rep = func(identifier);
        if ( rep )
            replacement = std::move(*rep);
    }

private:
    std::string identifier;
    FormattedString replacement;
};

class FilterRegistry : public melanolib::Singleton<FilterRegistry>
{
public:
    using Filter = std::function<FormattedString (const std::vector<FormattedString>& arguments)>;

    void register_filter(const std::string& name, const Filter& filter)
    {
        filters[name] = filter;
    }

    void unregister_filter(const std::string& name)
    {
        filters.erase(name);
    }

    FormattedString apply_filter(
        const std::string& name,
        const std::vector<FormattedString>& arguments
    ) const
    {
        auto it = filters.find(name);
        if ( it == filters.end() )
        {
            if ( arguments.empty() )
                return {};
            return arguments[0];
        }
        return it->second(arguments);
    }

private:
    FilterRegistry(){}
    friend ParentSingleton;

    std::unordered_map<std::string, Filter> filters;
};

class FilterCall
{
public:
    FilterCall(std::string filter, std::vector<FormattedString> arguments)
        : filter(std::move(filter)),
          arguments(std::move(arguments))
    {}

    void replace(const ReplacementFunctor& func)
    {
        for ( auto& arg : arguments )
            arg.replace(func);
    }

    FormattedString filtered() const
    {
        return FilterRegistry::instance().apply_filter(filter, arguments);
    }

    std::string to_string(const Formatter& formatter, Formatter::Context* context) const
    {
        return filtered().encode(formatter, context);
    }

private:
    std::string filter;
    std::vector<FormattedString> arguments;
};

class Padding
{
public:
    Padding(FormattedString string, int target_size, double align = 1, char fill=' ')
        : string(std::move(string)),
          target_size(target_size),
          align(align),
          fill(fill)
    {}

    std::string to_string(const Formatter& formatter, Formatter::Context* context) const
    {
        auto str = string.encode(formatter, context);

        int count = target_size - str.size();
        if ( count > 0 )
        {
            int before = count * align;

            if ( before )
                str = std::string(before, fill) + str;

            if ( count-before )
                str += std::string(count-before, fill);
        }

        return str;

    }

    void replace(const ReplacementFunctor& func)
    {
        string.replace(func);
    }

private:
    FormattedString string;
    int target_size;
    double align;
    char fill;
};

class IfStatement
{
public:
    IfStatement(FormattedString condition,
                FormattedString if_true,
                FormattedString if_false
    ) : condition(std::move(condition)),
        if_true(std::move(if_true)),
        if_false(std::move(if_false))
    {}

    std::string to_string(const Formatter& formatter, Formatter::Context* context) const
    {
        if ( is_true(condition.encode(formatter, context)) )
            return if_true.encode(formatter, context);
        return if_false.encode(formatter, context);
    }

    void replace(const ReplacementFunctor& func)
    {
        condition.replace(func);
        if_true.replace(func);
        if_false.replace(func);
    }

    bool is_true(std::string value) const
    {
        value = melanolib::string::trimmed(value);
        if ( std::all_of(value.begin(), value.end(), melanolib::string::ascii::is_digit) )
            return melanolib::string::to_uint(value);
        return !value.empty();
    }

private:
    FormattedString condition; /// \todo AST for boolean operations and comparisons?
    FormattedString if_true;
    FormattedString if_false;
};

class ForStatement
{
public:
    ForStatement(std::string variable,
                 FormattedString source,
                 FormattedString subject
    ) : variable(std::move(variable)),
        source(std::move(source)),
        subject(std::move(subject))
    {}

    std::string to_string(const Formatter& formatter, Formatter::Context* context) const
    {
        std::string output;
        for ( const auto& item : source )
        {
            FormattedString replace;
            replace.push_back(item);
            const_cast<FormattedString&>(subject).replace(variable, replace);
            output += subject.encode(formatter, context);
        }
        return output;
    }

    void replace(const ReplacementFunctor& func)
    {
        source.replace(func);
        subject.replace(func);
    }

    bool is_true(std::string value) const
    {
        value = melanolib::string::trimmed(value);
        if ( std::all_of(value.begin(), value.end(), melanolib::string::ascii::is_digit) )
            return melanolib::string::to_uint(value);
        return !value.empty();
    }

private:
    std::string variable;
    FormattedString source;
    FormattedString subject;
};

} // namespace string
#endif // MELANOBOT_STRING_REPLACEMENTS_HPP
