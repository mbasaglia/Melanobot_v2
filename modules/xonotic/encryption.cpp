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

#include "encryption.hpp"

#include <openssl/hmac.h>


namespace xonotic {
namespace crypto {

std::string hmac_md4(const std::string& input, const std::string& key)
{
    HMAC_CTX *ctx;
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
    ctx = HMAC_CTX_new();
#else // OpenSSL 1.0
    HMAC_CTX on_the_stack;
    ctx = &on_the_stack;
    HMAC_CTX_init(ctx);
#endif

    HMAC_Init_ex(ctx, key.data(), key.size(), EVP_md4(), nullptr);

    HMAC_Update(ctx, reinterpret_cast<const unsigned char*>(input.data()), input.size());

    unsigned char out[16];
    unsigned int out_size = 0;
    HMAC_Final(ctx, out, &out_size);

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
    HMAC_CTX_free(ctx);
#else // OpenSSL 1.0
    HMAC_CTX_cleanup(ctx);
#endif

    return std::string(out, out+out_size);
}



} // namespace crypto
} // namespace xonotic
