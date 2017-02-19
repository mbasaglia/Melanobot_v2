/**
 * \file
 * \brief This files has handlers for CTCP messages
 * 
 * \author Mattia Basaglia
 * \copyright Copyright 2015-2017 Mattia Basaglia
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
#ifndef IRC_HANDLER_CTCP
#define IRC_HANDLER_CTCP

#include "core/handler/group.hpp"
#include "config.hpp"
#include "irc/network/functions.hpp"
#include "melanolib/time/time_string.hpp"
#include "melanobot/melanobot.hpp"

namespace irc {
/**
 * \brief IRC-Specific handlers
 */
namespace handler {

/**
 * \brief Base class for handling CTCP requests
 * \see http://www.irchelp.org/irchelp/rfc/ctcpspec.html
 */
class CtcpBase : public melanobot::Handler
{
public:

    CtcpBase(const std::string& ctcp, const Settings& settings, ::MessageConsumer* parent)
        : Handler(settings, parent), ctcp(irc::strtoupper(ctcp))
    {
        if ( ctcp.empty() )
            throw melanobot::ConfigurationError();
    }

    bool can_handle(const network::Message& msg) const override
    {
        return !msg.params.empty() &&
            msg.source->protocol() == "irc" && msg.source == msg.destination &&
            msg.channels.size() == 1 && msg.from.name == msg.channels[0] &&
            irc::strtoupper(msg.command) == "CTCP" &&
            irc::strtoupper(msg.params[0]) == ctcp;
    }

    std::string get_property(const std::string& name) const override
    {
        if ( name == "ctcp" )
            return ctcp;
        else if ( name == "clientinfo" )
            return clientinfo;
        return Handler::get_property(name);
    }

protected:
    /**
     * \brief Sends a properly formatted reply corresponding to \c ctctp
     */
    void reply_to(const network::Message& msg,
                  network::OutputMessage output) const override
    {
        string::FormattedString s;
        s << '\1' << ctcp << ' ' << output.message << '\1';
        msg.destination->command({"NOTICE",
            {msg.from.name, s.encode(*msg.destination->formatter())}, priority});
    }
    using Handler::reply_to;

   std::string clientinfo; ///< String to be shown on clientinfo

private:
   std::string ctcp; ///< Name of the ctcp command to reply to
};

/**
 * \brief CTCP VERSION reply, shows a pre-formatted version string
 * \note It is strongly recommended that this is enabled
 */
class CtcpVersion: public CtcpBase
{

public:
    CtcpVersion ( const Settings& settings, ::MessageConsumer* parent )
        : CtcpBase("VERSION", settings, parent)
    {
        version = settings.get("version", "");
        clientinfo = ": Shows the bot's version";

        if ( version.empty() )
            version = PROJECT_NAME ":" PROJECT_DEV_VERSION ":"
                SYSTEM_COMPILER " " SYSTEM_PROCESSOR " "
                SYSTEM_NAME " " SYSTEM_VERSION;
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        reply_to(msg, version);
        return true;
    }

private:
    std::string version; ///< Version string to be displayed
};

/**
 * \brief CTCP SOURCE reply, shows a URL with the sources
 * \note Must be enabled to comply to the AGPL
 * \note It just prints the URL, not the weird format the specification says
 */
class CtcpSource: public CtcpBase
{

public:
    CtcpSource ( const Settings& settings, ::MessageConsumer* parent )
        : CtcpBase("SOURCE", settings, parent)
    {
        sources_url = settings.get("url", settings::global_settings.get("website", ""));
        clientinfo = ": Shows the bot's source URL";
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        reply_to(msg, sources_url);
        return true;
    }

private:
    std::string sources_url; ///< URL with the sources
};


/**
 * \brief CTCP USERINFO reply, shows a user-defined string
 */
class CtcpUserInfo: public CtcpBase
{

public:
    CtcpUserInfo ( const Settings& settings, ::MessageConsumer* parent )
        : CtcpBase(settings.get("ctcp", "USERINFO"), settings, parent)
    {
        reply = settings.get("reply", "");
        clientinfo = settings.get("clientinfo", "");
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        reply_to(msg, reply);
        return true;
    }

private:
    std::string reply; ///< Fixed reply
};

/**
 * \brief CTCP PING reply, Used to measure round-trip message delays
 */
class CtcpPing: public CtcpBase
{

public:
    CtcpPing ( const Settings& settings, ::MessageConsumer* parent )
        : CtcpBase("PING", settings, parent)
    {
        clientinfo = "time_string : Replies the same as the input";
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        /// \note should return a timestamp in the same format as the one
        /// provided by \c msg, but that's kinda hard to detect...
        reply_to(msg, msg.params.size() < 2 ? "" : msg.params[1]);
        return true;
    }
};

/**
 * \brief CTCP TIME reply, Shows the local time
 */
class CtcpTime: public CtcpBase
{

public:
    CtcpTime ( const Settings& settings, ::MessageConsumer* parent )
        : CtcpBase("TIME", settings, parent)
    {
        format = settings.get("format", "r");
        clientinfo = ": Shows local time";
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        reply_to(msg, melanolib::time::format(format));
        return true;
    }

private:
    std::string format; ///< Timestamp format
};

/**
 * \brief Shows help about other CTCP handlers
 * \note Strongly recommended to be enabled
 */
class CtcpClientInfo: public CtcpBase
{

public:
    CtcpClientInfo ( const Settings& settings, ::MessageConsumer* parent )
        : CtcpBase("CLIENTINFO", settings, parent)
    {
        clientinfo = "[command] : Shows help on CTCP commands";
        help_group = settings.get("help_group", help_group);
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        PropertyTree props;
        get_parent<melanobot::Melanobot>()->populate_properties({"ctcp", "clientinfo", "help_group"}, props);

        Properties clientinfo;
        gather(props, clientinfo);

        if ( !clientinfo.empty() )
        {
            std::string query = msg.params.size() < 2 ? "" : msg.params[1];
            auto it = clientinfo.find(irc::strtoupper(query));
            if ( it != clientinfo.end() )
            {
                reply_to(msg, it->first+" "+it->second);
            }
            else
            {
                std::vector<std::string> ctcp;
                ctcp.reserve(clientinfo.size());
                for ( const auto& p : clientinfo )
                    ctcp.push_back(p.first);
                std::sort(ctcp.begin(), ctcp.end());

                reply_to(msg, melanolib::string::implode(" ", ctcp));
            }
        }

        return true;
    }

private:
    std::string help_group; ///< Only shows help for members of this group

    /**
     * \brief Gathers ctcp clientinfo
     */
    void gather(const PropertyTree& properties, Properties& out) const
    {
        if ( properties.get("help_group", help_group) != help_group )
            return;
        for ( const auto& p : properties )
        {
            auto name = p.second.get("ctcp", "");
            if ( !name.empty() )
                out[name] = p.second.get("clientinfo", "");
            gather(p.second, out);
        }
    }
};

/**
 * \brief Preset for basic CTCP actions
 */
class Ctcp : public core::PresetGroup
{
public:
    Ctcp(const Settings& settings, ::MessageConsumer* parent)
        : PresetGroup({
            "CtcpVersion",
            "CtcpSource",
            "CtcpPing",
            "CtcpTime",
            "CtcpClientInfo",
        }, settings, parent) {}
};


} // namespace handler
} // namespace irc
#endif // IRC_HANDLER_CTCP
