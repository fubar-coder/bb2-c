/*
 * sha256.h - SHA-256 (FIPS 180-4) for the host side: hashes compared with
 * generated test data (tests/data/ted-oracle.sha256).
 */
#ifndef SHA256_H
#define SHA256_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct sha256 {
    uint32_t h[8];
    uint64_t len;               /* bytes so far */
    uint8_t block[64];
    size_t n;                   /* bytes in block */
};

void sha256_init(struct sha256 *s);
void sha256_update(struct sha256 *s, const void *data, size_t len);
void sha256_final(struct sha256 *s, uint8_t digest[32]);
/* the digest as 64 lower-case hex digits and a 0 */
void sha256_hex(struct sha256 *s, char hex[65]);
void sha256_hex_of(const void *data, size_t len, char hex[65]);

#ifdef __cplusplus
}
#endif

#endif
