/**
 * \file
 * \brief This files has handlers for CTCP messages
 * 
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
#include "handler.hpp"
#include "config.hpp"
#include "network/irc_functions.hpp"

namespace handler {

/**
 * \brief Base class for handling CTCP requests
 * \see http://www.irchelp.org/irchelp/rfc/ctcpspec.html
 */
class CtcpBase : public Handler
{
public:

    CtcpBase(const std::string& ctcp, const Settings& settings, Melanobot* bot)
        : Handler(settings,bot), ctcp(network::irc::strtoupper(ctcp))
    {
        if ( ctcp.empty() )
            throw ConfigurationError();
    }

    bool can_handle(const network::Message& msg) override
    {
        return authorized(msg) && !msg.params.empty() &&
            msg.source && msg.source->protocol() == "irc" &&
            msg.channels.size() == 1 && msg.from == msg.channels[0] &&
            network::irc::strtoupper(msg.command) == "CTCP" &&
            network::irc::strtoupper(msg.params[0]) == ctcp;
    }

protected:
    /**
     * \brief Sends a properly formatted reply corresponding to \c ctctp
     */
    void reply_to(const network::Message& msg,
                  const string::FormattedString& text) const override
    {
        string::FormattedStream s;
        s << '\1' << ctcp << ' ' << text << '\1';
        msg.source->command({"NOTICE",
            {msg.from,s.str().encode(msg.source->formatter())}, priority});
    }
    using Handler::reply_to;

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
    CtcpVersion ( const Settings& settings, Melanobot* bot )
        : CtcpBase("VERSION", settings, bot)
    {
        version = settings.get("version","");

        /// \todo more detailed system information
        if ( version.empty() )
            version = PROJECT_NAME ":" PROJECT_VERSION ":C++";
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        reply_to(msg,version);
        return true;
    }

private:
    std::string version; ///< Version string to be displayed
};
REGISTER_HANDLER(CtcpVersion,CtcpVersion);

/**
 * \brief CTCP SOURCE reply, shows a URL with the sources
 * \note Must be enabled to comply to the AGPL
 * \note It just prints the URL, not the weird format the specification says
 */
class CtcpSource: public CtcpBase
{

public:
    CtcpSource ( const Settings& settings, Melanobot* bot )
        : CtcpBase("SOURCE", settings, bot)
    {
        sources_url = settings.get("url",Settings::global_settings.get("website",""));
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        reply_to(msg,sources_url);
        return true;
    }

private:
    std::string sources_url; ///< URL with the sources
};
REGISTER_HANDLER(CtcpSource,CtcpSource);


/**
 * \brief CTCP USERINFO reply, shows a user-defined string
 */
class CtcpUserInfo: public CtcpBase
{

public:
    CtcpUserInfo ( const Settings& settings, Melanobot* bot )
        : CtcpBase(settings.get("ctcp","USERINFO"), settings, bot)
    {
        reply = settings.get("reply","");
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        reply_to(msg,":"+reply);
        return true;
    }

private:
    std::string reply; ///< Fixed reply
};
REGISTER_HANDLER(CtcpUserInfo,CtcpUserInfo);

/**
 * \file
 * \todo CtcpClientInfo
 */

/**
 * \brief CTCP PING reply, Used to measure round-trip message delays
 */
class CtcpPing: public CtcpBase
{

public:
    CtcpPing ( const Settings& settings, Melanobot* bot )
        : CtcpBase("PING", settings, bot)
    {}

protected:
    bool on_handle(network::Message& msg) override
    {
        /// \note should return a timestamp in the same format as the one
        /// provided by \c msg, but that's kinda hard to detect...
        reply_to(msg,msg.params.empty() ? "" : msg.params[1]);
        return true;
    }
};
REGISTER_HANDLER(CtcpPing,CtcpPing);

/**
 * \brief CTCP TIME reply, Shows the local time
 */
class CtcpTime: public CtcpBase
{

public:
    CtcpTime ( const Settings& settings, Melanobot* bot )
        : CtcpBase("TIME", settings, bot)
    {
        format = settings.get("format","%c %Z");
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        /// \note should return a timestamp in the same format as the one
        /// provided by \c msg, but that's kinda hard to detect...
        std::ostringstream ss;
        ss << boost::chrono::time_fmt(boost::chrono::timezone::local, format)
           << boost::chrono::system_clock::now();
        reply_to(msg,ss.str());
        return true;
    }

private:
    std::string format; ///< Timestamp format
};
REGISTER_HANDLER(CtcpTime,CtcpTime);

} // namespace handler
