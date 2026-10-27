/*
 * testutil.h - minimal checks and a load file builder for the tests of the
 * host tools (no test framework: plain C, exit status = failures).
 */
#ifndef TESTUTIL_H
#define TESTUTIL_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hunk.h"

static int failures;

#define CHECK(cond) do { \
        if (!(cond)) { \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); \
            failures++; \
        } \
    } while (0)

/* growing byte buffer */
struct buf {
    uint8_t *p;
    size_t n, cap;
};

static void put8(struct buf *b, uint8_t v)
{
    if (b->n == b->cap) {
        b->cap = b->cap ? 2 * b->cap : 256;
        b->p = realloc(b->p, b->cap);
        if (!b->p)
            abort();
    }
    b->p[b->n++] = v;
}

static void put16(struct buf *b, uint16_t v)
{
    put8(b, v >> 8);
    put8(b, v & 0xFF);
}

static void put32(struct buf *b, uint32_t v)
{
    put16(b, v >> 16);
    put16(b, v & 0xFFFF);
}

static void putbytes(struct buf *b, const void *p, size_t n)
{
    size_t i;

    for (i = 0; i < n; i++)
        put8(b, ((const uint8_t *)p)[i]);
}

/* a load file: table_size in the header, one hunk of type `type` with the
   (long padded) contents and RELOC32 longs at `relocs` pointing into hunk
   `target` (nrelocs may be 0: no reloc hunk), then HUNK_END */
static struct buf loadfile1(uint32_t table_size, uint32_t type, const struct buf *contents,
                            const uint32_t *relocs, uint32_t nrelocs, uint32_t target)
{
    struct buf b = { 0 };
    uint32_t longs = (uint32_t)((contents->n + 3) / 4), i;

    put32(&b, HUNK_HEADER);
    put32(&b, 0);                       /* no resident names */
    put32(&b, table_size);
    put32(&b, 0);
    put32(&b, table_size - 1);
    for (i = 0; i < table_size; i++)
        put32(&b, longs);
    put32(&b, type);
    put32(&b, longs);
    putbytes(&b, contents->p, contents->n);
    while (b.n % 4)
        put8(&b, 0);
    if (nrelocs) {
        put32(&b, HUNK_RELOC32);
        put32(&b, nrelocs);
        put32(&b, target);
        for (i = 0; i < nrelocs; i++)
            put32(&b, relocs[i]);
        put32(&b, 0);
    }
    put32(&b, HUNK_END);
    return b;
}

#endif
