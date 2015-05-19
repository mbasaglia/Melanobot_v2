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
#ifndef INPUT_MESSAGE_HPP
#define INPUT_MESSAGE_HPP

#include "network/network.hpp"
#include "user/user.hpp"

class Melanobot;

namespace network {

/**
 * \brief A message originating from a connection
 */
struct Message
{
    /**
     * \brief Type of message
     */
    enum Type
    {
        /**
         * \brief Some unknown message, most likely protocol-specific stuff
         */
        UNKNOWN,
        /**
         * \brief Simple chat message
         *
         * These messages should give meaningful values to:
         *      * \c message
         *      * \c from
         *      * \c direct
         *      * \c channels
         */
        CHAT,
        /**
         * \brief Similar to \c CHAT, but used for actions/roleplay
         *
         * These messages should give meaningful values to:
         *      * \c message
         *      * \c from
         *      * \c channels
         */
        ACTION,
        /**
         * \brief User joined the connection/a channel
         *
         * These messages should give meaningful values to:
         *      * \c from
         *      * \c channels
         */
        JOIN,
        /**
         * \brief User parted or quit
         *
         * These messages should give meaningful values to:
         *      * \c message
         *      * \c from
         *      * \c channels
         */
        PART,
        /**
         * \brief User has been kicked
         *
         * These messages should give meaningful values to:
         *      * \c message
         *      * \c from
         *      * \c victim
         *      * \c channels
         */
        KICK,
        /**
         * \brief User changed name
         *
         * These messages should give meaningful values to:
         *      * \c from
         */
        RENAME,
        /**
         * \brief Server error
         *
         * These messages should give meaningful values to:
         *      * \c message
         */
        ERROR,
        /**
         * \brief Connection activated
         */
        CONNECTED,
        /**
         * \brief Connection deactivated
         */
        DISCONNECTED,
    };
// origin
    class Connection*        source {nullptr};     ///< Connection originating this message
// reply
    class Connection*        destination {nullptr};///< Connection which should receive replies
// low level properties
    std::string              raw;     ///< Raw contents
    std::string              command; ///< Protocol command name
    std::vector<std::string> params;  ///< Tokenized parameters
// high level properties (all optional)
    Type                     type{UNKNOWN};///< Message type
    std::string              message;      ///< Message contents
    std::vector<std::string> channels;     ///< Channels affected by the message
    bool                     direct{false};///< Message has been addessed to the bot directly
    user::User               from;         ///< User who created this message
    user::User               victim;       ///< User victim of this command

    /**
     * \brief Set \c source and \c destination and sends to the given bot
     */
    void send(Connection* from, Melanobot* to);

    /**
     * \brief Turns into a CONNECTED message
     */
    Message& connected()
    {
        type = CONNECTED;
        return *this;
    }
    /**
     * \brief Turns into a DISCONNECTED message
     */
    Message& disconnected()
    {
        type = DISCONNECTED;
        return *this;
    }

    /**
     * \brief Turns into a CHAT message
     */
    Message& chat(const std::string& message)
    {
        type = CHAT;
        this->message = message;
        return *this;
    }

    /**
     * \brief Turns into an ACTION message
     */
    Message& action(const std::string& message)
    {
        type = ACTION;
        this->message = message;
        return *this;
    }
};

} // namespace network
#endif // INPUT_MESSAGE_HPP
