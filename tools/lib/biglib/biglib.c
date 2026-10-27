/*
 * biglib.c - writing and reading BigLib files, see biglib.h.
 */
#include <stdlib.h>
#include "biglib.h"
#include "hunk.h"

static int putlong(FILE *f, uint32_t v)
{
    unsigned char b[4] = { v >> 24 & 0xFF, v >> 16 & 0xFF, v >> 8 & 0xFF, v & 0xFF };

    return fwrite(b, 1, 4, f) == 4 ? 0 : -1;
}

int biglib_write(FILE *out, const struct biglib_entry *entries, size_t n)
{
    size_t i;

    for (i = 0; i < n; i++)
        if (putlong(out, entries[i].size)
            || fwrite(entries[i].data, 1, entries[i].size, out) != entries[i].size)
            return -1;
    return putlong(out, 0);
}

int biglib_read(const uint8_t *buf, size_t size, struct biglib_entry **entries, size_t *n,
                char *err, size_t errlen)
{
    size_t p = 0;

    *entries = NULL;
    *n = 0;
    for (;;) {
        uint32_t len;

        if (size - p < 4) {
            snprintf(err, errlen, "no end mark (long 0) at %zu", p);
            break;
        }
        len = be32(buf + p);
        p += 4;
        if (!len)
            return 0;
        if (size - p < len) {
            snprintf(err, errlen, "entry %zu at %zu: %u bytes, only %zu left",
                     *n, p - 4, (unsigned)len, size - p);
            break;
        }
        if (*n % 32 == 0) {
            struct biglib_entry *e = realloc(*entries, (*n + 32) * sizeof *e);

            if (!e) {
                snprintf(err, errlen, "out of memory");
                break;
            }
            *entries = e;
        }
        (*entries)[*n].data = buf + p;
        (*entries)[(*n)++].size = len;
        p += len;
    }
    free(*entries);
    *entries = NULL;
    *n = 0;
    return -1;
}
