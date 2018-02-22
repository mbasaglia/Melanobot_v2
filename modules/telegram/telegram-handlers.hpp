/**
 * \file
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
#ifndef MELANOBOT_MODULES_TELEGRAM_HANDLERS_HPP
#define MELANOBOT_MODULES_TELEGRAM_HANDLERS_HPP

#include "core/handler/misc.hpp"
#include "inline.hpp"

namespace telegram {

/**
 * \brief Sends a sticker
 */
class SendSticker : public core::Reply
{
public:
    SendSticker(const Settings& settings, MessageConsumer* parent)
        : Reply(settings, parent)
    {
        sticker_id = settings.get("reply", "");
        if ( sticker_id.empty() )
            throw melanobot::ConfigurationError("Missing sticker_id for SendSticker");
    }

    void on_handle(const network::Message& msg, string::FormattedString&& reply) const override
    {
        msg.destination->command({
            "sendSticker",
            {reply_channel(msg), sticker_id}
        });
    }

private:
    std::string sticker_id;
};

} // namespace telegram

#endif // MELANOBOT_MODULES_TELEGRAM_HANDLERS_HPP
