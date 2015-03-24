/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright  Mattia Basaglia
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
#ifndef HANDLER_BRIDGE_HPP
#define HANDLER_BRIDGE_HPP

#include "simple-group.hpp"

namespace handler {

/**
 * \brief Acts as bridge across connections
 * \todo attachable to other connections (maybe limiting the protocol)
 */
class Bridge : public SimpleGroup
{
public:
    Bridge(const Settings& settings, handler::HandlerContainer* parent);

    /**
     * \brief Attach to the given connection
     */
    void attach(network::Connection* connection);

protected:
    bool on_handle(network::Message& msg) override;

    network::Connection*         destination{nullptr};  ///< Message destination
    boost::optional<std::string> dst_channel;           ///< Message destination channel
};

/**
 * \brief Simply echoes chat messages (to be used in a Bridge group)
 */
class BridgeChat : public Handler
{
public:
    BridgeChat(const Settings& settings, handler::HandlerContainer* parent);
    bool can_handle(const network::Message& msg) const override;

protected:
    bool on_handle(network::Message& msg) override;

    std::string       prefix;                            ///< Output message prefix
    network::Duration timeout{network::Duration::zero()};///< Output message timeout
    bool              ignore_self{true};                 ///< Ignore bot messages
};

/**
 * \brief Attach the parent bridge to the provided connection
 */
class BridgeAttach : public SimpleAction
{
public:
    BridgeAttach(const Settings& settings, handler::HandlerContainer* parent);
    void initialize() override;

protected:
    bool on_handle(network::Message& msg) override;

    std::string protocol;    ///< Limit only to connections with this protocol
    bool    detach{true};    ///< Allow using this to detach the bridge
    Bridge* parent{nullptr}; ///< Bridge object to apply the attachment to
};

} // namespace handler
#endif // HANDLER_BRIDGE_HPP
