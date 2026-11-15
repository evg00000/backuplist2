/* sha256.h — FIPS 180-4. Own implementation: CryptoAPI on Windows XP SP2 has no SHA-256. */
#ifndef BL_SHA256_H
#define BL_SHA256_H

#include <stddef.h>
#include <stdint.h>
#include <wchar.h>

typedef struct {
    uint32_t state[8];
    uint64_t length;        /* in bytes */
    uint8_t  buf[64];
    size_t   used;
} Sha256;

void sha256_init(Sha256 *c);
void sha256_update(Sha256 *c, const void *data, size_t len);
void sha256_final(Sha256 *c, uint8_t out[32]);
void sha256_hex(const uint8_t digest[32], wchar_t out[65]);   /* lowercase */

#endif
