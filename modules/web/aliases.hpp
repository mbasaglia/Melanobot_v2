/**
 * \file
 *
 * \author Mattia Basaglia
 *
 * \copyright Copyright 2016 Mattia Basaglia
 *
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
#ifndef MELANOBOT_MODULE_WEB_ALIASES_HPP
#define MELANOBOT_MODULE_WEB_ALIASES_HPP

#include "httpony/ssl/ssl.hpp"

namespace web {

using httpony::DataMap;
using httpony::OperationStatus;
using httpony::Request;
using httpony::Response;
using httpony::Uri;
using httpony::build_query_string;
using httpony::urlencode;
using httpony::Status;
using httpony::StatusCode;
using httpony::MimeType;
using UriPath = httpony::Path;

} // namespace web
#endif // MELANOBOT_MODULE_WEB_ALIASES_HPP
