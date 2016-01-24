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
#ifndef SCRIPT_HANDLERS_HPP
#define SCRIPT_HANDLERS_HPP

#include "handler/handler.hpp"
#include "python.hpp"

namespace python {

/**
 * \brief Runs a python script
 */
class SimpleScript : public handler::SimpleAction
{
protected:
    /**
     * \brief Exposes data members
     * \note A bit ugly, convert is defined in script_variables.cpp
     */
    struct Variables : public MessageVariables
    {
        Variables(SimpleScript* obj, network::Message& msg)
            : MessageVariables(msg), obj(obj) {}

        void convert(boost::python::object& target_namespace) const override;

        SimpleScript* obj;
    };

    /**
     * \brief What kind of action to take when the script generates an error
     */
    enum class OnError {
        DISCARD_INPUT,  ///< Discard input message
        DISCARD_OUTPUT, ///< Discard script output, but still mark the message as handled
        IGNORE,         ///< Ignore the error, show script output
    };

public:
    SimpleScript(const Settings& settings, MessageConsumer* parent)
        : SimpleAction(settings.get("trigger",settings.get("script","")),settings,parent)
    {
        std::string script_rel = settings.get("script","");
        if ( script_rel.empty() )
            throw ConfigurationError("Missing script file");

        /// \todo check for absolute path
        script = settings::data_file("scripts/"+script_rel);
        if ( script.empty() )
            throw ConfigurationError("Script file not found: "+script_rel);

        synopsis += settings.get("synopsis","");
        help = settings.get("help", "Runs "+script_rel);
        on_error = onerror_from_string(settings.get("error", onerror_to_string(on_error)));

        auto formatter_name = settings.get_optional<std::string>("formatter");
        if ( formatter_name )
            formatter = string::Formatter::formatter(*formatter_name);
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        /// \todo Timeout
        auto env = environment(msg);
        auto output = python::PythonEngine::instance().exec_file(script,*env);
        if ( output.success || on_error == OnError::IGNORE )
            for ( const auto& line : output.output )
                reply_to(msg,format(line));

        return output.success || on_error != OnError::DISCARD_INPUT;
    }

    /**
     * \brief Build the environment on which the script is run
     */
    virtual std::unique_ptr<MessageVariables> environment(network::Message& msg)
    {
        return std::make_unique<Variables>(this,msg);
    }

    /**
     * \brief Converts an OnError to a string
     */
    static std::string onerror_to_string(OnError err)
    {
        switch(err)
        {
            case OnError::DISCARD_INPUT: return "discard_input";
            case OnError::DISCARD_OUTPUT:return "discard_output";
            case OnError::IGNORE:        return "ignore";
            default:                     return "";
        }
    }

    /**
     * \brief Converts astring to an OnError
     */
    static OnError onerror_from_string(const std::string& err)
    {
        if ( err == "discard_input" )
            return OnError::DISCARD_INPUT;
        if ( err == "discard_output" )
            return OnError::DISCARD_OUTPUT;
        if ( err == "ignore" )
            return OnError::IGNORE;
        return OnError::DISCARD_OUTPUT;
    }

private:
    string::FormattedString format(const std::string& line)
    {
        if ( formatter )
            return formatter->decode(line);
        return string::FormattedString(line);
    }

    std::string script;                         ///< Script file path
    OnError on_error = OnError::DISCARD_OUTPUT; ///< Script error policy
    string::Formatter* formatter{nullptr};      ///< Formatter used to parse the output
};

/**
 * \brief Reads a json file describing the handler
 */
class StructuredScript : public SimpleScript
{
private:
    /**
     * \brief Exposes \c settings
     * \note A bit ugly, convert is defined in script_variables.cpp
     */
    struct Variables : public SimpleScript::Variables
    {
        Variables(StructuredScript* obj, network::Message& msg)
            : SimpleScript::Variables(obj,msg) {}

        void convert(boost::python::object& target_namespace) const override;
    };

public:
    StructuredScript(const Settings& in_settings, MessageConsumer* parent)
        : StructuredScript(load_settings(in_settings),parent,true)
    {}

protected:
    std::unique_ptr<MessageVariables> environment(network::Message& msg) override
    {
        return std::make_unique<Variables>(this,msg);
    }

private:
    /**
     * \brief Called by the public constructor to ensure settings are read only once
     */
    StructuredScript(const Settings& read_settings, MessageConsumer* parent, bool)
        : SimpleScript(read_settings,parent)
    {
        settings = read_settings.get_child("settings",{});
    }

    /**
     * \brief Load settings from a file descibing the handler
     * \param input Settings passed to the constructor
     * \returns Settings containing merged keys of \c input and what has been
     *          read from the file
     */
    static Settings load_settings(const Settings& input)
    {
        std::string relfile = input.get("id","");
        if ( relfile.empty() )
            throw ConfigurationError("Missing id file");

        std::string descriptor = settings::data_file("scripts/"+relfile+"/"+relfile+".json");
        if ( descriptor.empty() )
            throw ConfigurationError("Id file not found: "+relfile);

        Settings description;
        try {
            description = settings::load(descriptor,settings::FileFormat::JSON);
        } catch ( const CriticalException& exc ) {
            throw ConfigurationError(exc.what());
        }

        settings::merge(description, input, true);
        return description;
    }

    Settings settings; ///< Script settings, read from the script description and the bot configuration
};

} // namespace python
#endif // SCRIPT_HANDLERS_HPP
