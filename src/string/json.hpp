/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright  Mattia Basaglia
 * \license
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
#ifndef JSON_HPP
#define JSON_HPP

#include <fstream>
#include <istream>
#include <stack>
#include <sstream>
#include <string>

#include <boost/property_tree/ptree.hpp>

#include "string/logger.hpp"
#include "string_functions.hpp"


/**
 * \brief Error encountered when parsing JSON
 */
struct JsonError : public LocatableException
{
    using LocatableException::LocatableException;
};

/**
 * \brief Class that populates a property tree from a JSON in a stream
 *
 * Unlike from boost::property_tree::json_parser::read_json
 * it reads arrays element with their numeric index, is a bit forgiving
 * about syntax errors and allows simple unquoted strings
 */
struct JsonParser
{
    /**
     * \brief Parse the stream
     * \param stream      Input stream
     * \param stream_name Used for error reporting
     * \throws JsonError On bad syntax
     * \returns The property tree populated from the stream
     */
    const Settings& parse(std::istream& stream, const std::string& stream_name="")
    {
        this->stream.rdbuf(stream.rdbuf());
        this->stream_name = stream_name;
        line = 1;
        parse_json_root();
        if ( !context.empty() )
            error("Abrupt ending");
        return ptree;
    }

    /**
     * \brief Parse the given file
     * \throws JsonError On bad syntax
     * \returns The property tree populated from the stream
     */
    const Settings& parse_file(const std::string& file_name )
    {
        std::filebuf buf;
        line = 1;
        stream_name = file_name;
        if ( !buf.open(file_name,std::ios::in) )
            error("Cannot open file");
        this->stream.rdbuf(&buf);
        parse_json_root();
        if ( !context.empty() )
            error("Abrupt ending");
        return ptree;
    }

    /**
     * \brief Parse the string
     * \throws JsonError On bad syntax
     * \returns The property tree populated from the stream
     */
    const Settings& parse_string(const std::string& json, const std::string& stream_name="" )
    {
        std::istringstream ss(json);
        return parse(ss,stream_name);
    }

    /**
     * \brief This can be used to get partial trees on errors
     */
    const Settings& tree() const
    {
        return ptree;
    }

    /**
     * \brief Whether there has been a parsing error
     */
    bool error() const
    {
        return error_flag;
    }

    /**
     * \brief Whether the parser can throw
     */
    bool throws() const
    {
        return !nothrow;
    }

    /**
     * \brief Sets whether the parser can throw
     */
    void throws(bool throws)
    {
        nothrow = !throws;
    }

private:
    /**
     * \brief Throws an exception pointing to the current line
     */
    void error [[noreturn]] (const std::string& message)
    {
        throw JsonError(stream_name,line,message);
    }

    /**
     * \brief Parses the entire file
     */
    void parse_json_root()
    {
        error_flag = false;
        ptree.clear();
        context = decltype(context)();

        if ( nothrow )
        {
            try {
                parse_json_object();
            } catch ( const JsonError& err ) {
                ErrorLog errlog("web","JSON Error");
                if ( Settings::global_settings.get("debug",0) )
                    errlog << err.file << ':' << err.line << ": ";
                errlog << err.what();
            }
        }
        else
        {
            parse_json_object();
        }
    }

    /**
     * \brief Parse a JSON object
     */
    void parse_json_object()
    {
        char c = get_skipws();
        if ( c != '{' )
            error("Expected object");

        if ( !context.empty() )
            ptree.put_child(context_pos(),{});

        parse_json_properties();
    }

    /**
     * \brief Parse object properties
     */
    void parse_json_properties()
    {
        char c = get_skipws();
        while ( true )
        {
            if ( !stream )
                error("Expected } or ,");

            if ( c == '}' )
                break;

            if ( c == '\"' )
            {
                stream.unget();
                context_push(parse_json_string());
            }
            else if ( std::isalpha(c) )
            {
                stream.unget();
                context_push(parse_json_identifier());
            }
            else
                error("Expected property name");


            c = get_skipws();
            if ( c != ':' )
                stream.unget();

            parse_json_value();
            context_pop();

            c = get_skipws();

            if ( c == ',' )
                c = get_skipws();
        }
    }

    /**
     * \brief Parse a JSON array
     */
    void parse_json_array()
    {
        char c = get_skipws();
        if ( c != '[' )
            error("Expected array");

        if ( !context.empty() )
            ptree.put_child(context_pos(),{});

        context_push_array();
        parse_json_array_elements();
        context_pop();
    }

    /**
     * \brief Parse array elements
     */
    void parse_json_array_elements()
    {
        char c = get_skipws();

        while ( true )
        {
            if ( !stream )
                error("Expected ]");

            if ( c == ']' )
                break;

            stream.unget();

            parse_json_value();

            c = get_skipws();

            if ( c == ',' )
                c = get_skipws();

            context.top().array_index++;
        }
    }

    /**
     * \brief Parse a value (number, string, array, object, or other literal)
     */
    void parse_json_value()
    {
        char c = get_skipws();
        stream.unget();
        if ( c == '{' )
            parse_json_object();
        else if ( c == '[' )
            parse_json_array();
        else
            parse_json_literal();

    }

    /**
     * \brief Parse numeric, string and boolean literals
     */
    void parse_json_literal()
    {
        char c = get_skipws();
        if ( std::isalpha(c) )
        {
            stream.unget();
            std::string val = parse_json_identifier();
            if ( val == "true" )
                put(true);
            else if ( val == "false" )
                put(false);
            else if ( val != "null" ) // Null => no value
                put(val);
        }
        else if ( c == '\"' )
        {
            stream.unget();
            put(parse_json_string());
        }
        else if ( std::isdigit(c) || c == '.' || c == '-' || c == '+' )
        {
            stream.unget();
            put(parse_json_number());
        }
        else
        {
            error("Expected value");
        }
    }

    /**
     * \brief Parse a string literal
     */
    std::string parse_json_string()
    {
        char c = get_skipws();
        std::string r;
        if ( c == '\"' )
        {
            while ( true )
            {
                c = stream.get();
                if ( !stream || c == '\"' )
                    break;
                if ( c == '\\' )
                {
                    c = escape(stream.get());
                    if ( !stream )
                        break;

                    if ( c == 'u' )
                    {
                        char hex[] = "0000";
                        stream.read(hex,4);
                        for ( int i = 0; i < 4; i++ )
                        {
                            if ( !std::isxdigit(hex[i]) )
                                hex[i] = '0';
                        }
                        r += string::Utf8Parser::encode(string::to_uint(hex,16));

                        continue;
                    }

                    if ( !escapeable(c) )
                        r += '\\';
                }

                r += c;

                if ( c == '\n' )
                    line++;
            }
        }

        return r;
    }

    /**
     * \brief Parses an identifier
     */
    std::string parse_json_identifier()
    {
        char c = get_skipws();
        if ( !std::isalpha(c) )
            return {};

        std::string r;
        while ( true )
        {
            r += c;
            c = stream.get();
            if ( !stream || ( !std::isalnum(c) && c != '_' && c != '-' ) )
                break;
        }
        stream.unget();

        return r;
    }

    /**
     * \brief Parse a numeric literal
     */
    double parse_json_number()
    {
        double d = 0;

        if ( !(stream >> d) )
        {
            stream.get();
            error("Expected numeric literal");
        }

        return d;
    }

    /**
     * \brief Get the next non-space character in the stream
     */
    char get_skipws()
    {
        /// \todo possibly skip comments
        char c;
        do
        {
            c = stream.get();
            if ( c == '\n' )
                line++;
        }
        while ( stream && std::isspace(c) );
        return c;
    }

    /**
     * \brief Whether \c c can be espaced on a string literal
     */
    bool escapeable(char c)
    {
        switch ( c )
        {
            case 'b': case 'f': case 'r': case 't': case 'n':
            case '\\': case '\"': case '/':
                return true;
        }
        return false;
    }

    /**
     * \brief Escape code for \\ c
     */
    char escape(char c)
    {
        switch ( c )
        {
            case 'b': return '\b';
            case 'f': return '\f';
            case 'r': return '\r';
            case 't': return '\t';
            case 'n': return '\n';
            default:  return c;
        }
    }

    /**
     * \brief Put a value in the tree
     */
    template<class T>
        void put(const T& val)
    {
        ptree.put(context_pos(),val);
    }

    /**
     * \brief Position in the tree as defined by the current context
     */
    std::string context_pos() const
    {
        if ( context.empty() )
            return {};
        const Context& ctx = context.top();
        std::string name = ctx.name;
        if ( ctx.array_index >= 0 )
        {
            if ( !name.empty() )
                name += '.';
            name += std::to_string(ctx.array_index);
        }
        return name;
    }

    /**
     * \brief Push a named context
     */
    void context_push(const std::string&name)
    {
        std::string current = context_pos();
        if ( !current.empty() )
            current += '.';
        current += name;
        context.push({current});
    }

    /**
     * \brief Push an array context
     */
    void context_push_array()
    {
        std::string current = context_pos();
        Context ctx(current);
        ctx.array_index = 0;
        context.push(ctx);
    }

    /**
     * \brief Pop a context
     */
    void context_pop()
    {
        context.pop();
    }

    /**
     * \brief Context information
     */
    struct Context
    {
        std::string name;
        int array_index = -1;
        Context(std::string name) : name(std::move(name)) {}
    };

    std::istream stream{nullptr};///< Input stream
    std::string stream_name;    ///< Name of the file
    int line = 0;               ///< Line number
    Settings ptree;             ///< Output tree
    std::stack<Context> context;///< Context stack
    bool nothrow = false;       ///< If true, don't throw
    bool error_flag = false;    ///< If true, there has been an error
};

#endif // JSON_HPP
