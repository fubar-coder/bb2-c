/*
 * hunkcheck.c - check the start of the first hunk, see hunkcheck.h.
 */
#include <stdlib.h>
#include <string.h>
#include "hunkcheck.h"

static int hexval(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

int hunk_check(const struct loadfile *lf, int nspecs, char **specs, const char *name, FILE *out)
{
    const struct hunk *h;
    int i, bad = 0;

    if (lf->nhunks == 0 || lf->hunks[0].type != HUNK_CODE) {
        fprintf(out, "%s: the first hunk is not code\n", name);
        return 1;
    }
    h = &lf->hunks[0];
    for (i = 0; i < nspecs; i++) {
        char *hex;
        unsigned long off = strtoul(specs[i], &hex, 0);
        size_t n, k;

        n = *hex == ':' ? strlen(++hex) : 0;
        if (!n || n % 2) {
            fprintf(out, "%s: bad check \"%s\" (OFFSET:HEXBYTES)\n", name, specs[i]);
            return 1;
        }
        if (off > h->size || h->size - off < n / 2) {
            fprintf(out, "%s: hunk+%lu: the first hunk has only %u bytes\n",
                    name, off, (unsigned)h->size);
            bad = 1;
            continue;
        }
        for (k = 0; k < n / 2; k++) {
            int hi = hexval(hex[2 * k]), lo = hexval(hex[2 * k + 1]);

            if (hi < 0 || lo < 0) {
                fprintf(out, "%s: bad check \"%s\" (OFFSET:HEXBYTES)\n", name, specs[i]);
                return 1;
            }
            if (h->data[off + k] != (hi << 4 | lo))
                break;
        }
        if (k < n / 2) {
            fprintf(out, "%s: hunk+%lu is ", name, off);
            for (k = 0; k < n / 2; k++)
                fprintf(out, "%02x", h->data[off + k]);
            fprintf(out, ", expected %s\n", hex);
            bad = 1;
        }
    }
    return bad;
}
