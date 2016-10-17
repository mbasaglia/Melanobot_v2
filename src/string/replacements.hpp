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

    void expand_into(FormattedString& output) const
    {
        replacement.expand_into(output);
    }

    const FormattedString& value() const
    {
        return replacement;
    }

    melanolib::scripting::Object to_object(
        const melanolib::scripting::TypeSystem& ts
    ) const
    {
        return replacement.to_object(ts);
    }

    melanolib::scripting::Object to_object() const
    {
        return replacement.to_object();
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

    void expand_into(FormattedString& output) const
    {
        filtered().expand_into(output);
    }

    melanolib::scripting::Object to_object(
        const melanolib::scripting::TypeSystem& ts
    ) const
    {
        return filtered().to_object(ts);
    }

    melanolib::scripting::Object to_object() const
    {
        return filtered().to_object();
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

class ListItem
{
public:
    ListItem(FormattedString item)
    : item(std::move(item))
    {}

    template<class T>
        explicit ListItem(T&& element)
    {
        item.append(element);
    }

    std::string to_string(const Formatter& formatter, Formatter::Context* context) const
    {
        return item.to_string(formatter, context);
    }

    void replace(const ReplacementFunctor& func)
    {
        item.replace(func);
    }

    const FormattedString& contents() const
    {
        return item;
    }

    melanolib::scripting::Object to_object(
        const melanolib::scripting::TypeSystem& ts
    ) const
    {
        return item.to_object(ts);
    }

    melanolib::scripting::Object to_object() const
    {
        return item.to_object();
    }

private:
    FormattedString item;
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
        for ( const Element& item : source.expanded() )
        {
            output += replace_item(item).encode(formatter, context);
        }
        return output;
    }

    void replace(const ReplacementFunctor& func)
    {
        source.replace(func);
        subject.replace(func);
    }

private:
    FormattedString replace_item(const Element& element) const
    {
        using namespace melanolib::scripting;
        if ( element.has_type<Object>() )
        {
            const Object& obj = element.reference<Object>();
            Object context = obj.type().type_system().object<SimpleType>();
            context.set(variable, obj);
            return subject.replaced(context);
        }
        else if ( element.has_type<ListItem>() )
        {
            const ListItem& item = element.reference<ListItem>();
            if ( item.contents().size() == 1 )
                return replace_item(item.contents()[0]);
            return subject.replaced(variable, item.contents());
        }
        else
        {
            FormattedString replace;
            replace.push_back(element);
            return subject.replaced(variable, replace);
        }
    }

    std::string variable;
    FormattedString source;
    FormattedString subject;
};

class MethodCall
{
public:
    MethodCall(std::string name, std::string method,
               std::vector<FormattedString> arguments)
        : object_identifier(std::move(name)),
          method(std::move(method)),
          arguments(std::move(arguments))
    {}

    void replace(const ReplacementFunctor& func)
    {
        auto rep = func(object_identifier);
        if ( rep )
            object = rep->to_object();

        for ( auto& arg : arguments )
            arg.replace(func);
    }

    std::string to_string(const Formatter& formatter, Formatter::Context* context) const
    {
        if ( object.has_value() )
        {
            try {
                return call().to_string();
            } catch (const std::exception&) {
            }
        }
        return "";
    }

    void expand_into(FormattedString& output) const
    {
        if ( object.has_value() )
        {
            try {
                detail::expand_into_dispatch(call(), output);
            } catch (const std::exception&) {
            }
        }
    }


    melanolib::scripting::Object to_object(
        const melanolib::scripting::TypeSystem& ts
    ) const
    {
        return call();
    }

    melanolib::scripting::Object to_object() const
    {
        return call();
    }

private:
    melanolib::scripting::Object call() const
    {
        melanolib::scripting::Object::Arguments args;
        args.reserve(arguments.size());
        for ( const auto& string_arg : arguments )
            args.push_back(string_arg.to_object(object.type().type_system()));
        return object.call(method, args);
    }

private:
    std::string object_identifier;
    std::string method;
    melanolib::scripting::Object object{{}};
    std::vector<FormattedString> arguments;
};

} // namespace string
#endif // MELANOBOT_STRING_REPLACEMENTS_HPP
