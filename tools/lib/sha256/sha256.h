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

/** @brief The state of a SHA-256 computation. */
struct sha256 {
    uint32_t h[8];
    uint64_t len;               /* bytes so far */
    uint8_t block[64];
    size_t n;                   /* bytes in block */
};

/** @brief Start a computation. */
void sha256_init(struct sha256 *s);

/**
 * @brief Hash more data.
 * @param s     the state
 * @param data  the bytes
 * @param len   their number
 */
void sha256_update(struct sha256 *s, const void *data, size_t len);

/**
 * @brief End the computation (the padding) and give the digest.
 * @param s       the state (used up)
 * @param digest  out: the 32 bytes of the digest
 */
void sha256_final(struct sha256 *s, uint8_t digest[32]);

/**
 * @brief End the computation and give the digest as 64 lower-case hex
 *        digits and a 0.
 * @param s    the state (used up)
 * @param hex  out: the digest
 */
void sha256_hex(struct sha256 *s, char hex[65]);

/**
 * @brief The digest of data[0..len) as 64 lower-case hex digits and a 0.
 * @param data  the bytes
 * @param len   their number
 * @param hex   out: the digest
 */
void sha256_hex_of(const void *data, size_t len, char hex[65]);

#ifdef __cplusplus
}
#endif

#endif
