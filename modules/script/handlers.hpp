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
#ifndef SCRIPT_HANDLERS_HPP
#define SCRIPT_HANDLERS_HPP

#include "handler/handler.hpp"
#include "python.hpp"

namespace python {

/**
 * \brief Runs a python script
 */
class SimplePython : public handler::SimpleAction
{
protected:
    /**
     * \brief Exposes data members
     * \note A bit ugly, convert is defined in script_variables.cpp
     */
    struct Variables : public MessageVariables
    {
        Variables(SimplePython* obj, network::Message& msg)
            : MessageVariables(msg), obj(obj) {}

        void convert(boost::python::object& target_namespace) const override;

        SimplePython* obj;
    };
public:
    SimplePython(const Settings& settings, MessageConsumer* parent)
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
        discard_error = settings.get("discard_error", discard_error);

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
        if ( output.success || !discard_error )
            for ( const auto& line : output.output )
                reply_to(msg,format(line));
        /// \todo Update from/victim

        return true;
    }

    /**
     * \brief Build the environment on which the script is run
     */
    virtual std::unique_ptr<MessageVariables> environment(network::Message& msg)
    {
        return std::make_unique<Variables>(this,msg);
    }

private:
    string::FormattedString format(const std::string& line)
    {
        if ( formatter )
            return formatter->decode(line);
        return string::FormattedString(line);
    }

    std::string script;         ///< Script file path
    bool discard_error = true;  ///< If \b true, only prints output of scripts that didn't fail
    string::Formatter* formatter{nullptr};      ///< Formatter used to parse the output
};

/**
 * \brief Reads a json file describing the handler
 */
class StructuredScript : public SimplePython
{
private:
    /**
     * \brief Exposes \c settings
     * \note A bit ugly, convert is defined in script_variables.cpp
     */
    struct Variables : public SimplePython::Variables
    {
        Variables(StructuredScript* obj, network::Message& msg)
            : SimplePython::Variables(obj,msg) {}

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
        : SimplePython(read_settings,parent)
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

    Settings settings;
};

} // namespace python
#endif // SCRIPT_HANDLERS_HPP
