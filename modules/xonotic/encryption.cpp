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


#if OPENSSL_VERSION_NUMBER >= 0x10100000L
#define POINTER_INIT ; //HMAC_CTX_new allocates from heap
#define HMAC_CTX_init(ctx) ctx = HMAC_CTX_new()
#define HMAC_CTX_cleanup HMAC_CTX_free
#else // Version 1.0
#define POINTER_INIT HMAC_CTX c; ctx = &c;
#endif



namespace xonotic {
namespace crypto {

std::string hmac_md4(const std::string& input, const std::string& key)
{
    HMAC_CTX *ctx;
    POINTER_INIT;
    HMAC_CTX_init(ctx);

    HMAC_Init_ex(ctx, key.data(), key.size(), EVP_md4(), nullptr);

    HMAC_Update(ctx, reinterpret_cast<const unsigned char*>(input.data()), input.size());

    unsigned char out[16];
    unsigned int out_size = 0;
    HMAC_Final(ctx, out, &out_size);

    HMAC_CTX_cleanup(ctx);

    return std::string(out, out+out_size);
}



} // namespace crypto
} // namespace xonotic
