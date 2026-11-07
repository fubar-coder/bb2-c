/*
 * compare.c - comparing the declaration parts of two builds of a library,
 * see blitzlib.h (blitzlib_compare).
 */
#include <string.h>
#include "blitzlib.h"

/* region of an offset: -1 header, k entry k, nentries the end of the
   table (the $FFFF, !libfin) */
static long region(const struct blitzlib *lib, uint32_t p)
{
    uint32_t end = lib->libfin ? lib->libfin - 2 : lib->codestart - 6;
    long k;

    if (p < 0x30)
        return -1;
    if (p >= end)
        return lib->nentries;
    for (k = 1; k < (long)lib->nentries; k++)
        if (p < lib->entries[k].offset)
            break;
    return k - 1;
}

static void where(const struct blitzlib *lib, long r, uint32_t p, FILE *out)
{
    if (r < 0)
        fprintf(out, "header +$%02X", (unsigned)p);
    else if (r < (long)lib->nentries) {
        const struct blitzlib_entry *e = &lib->entries[r];

        fprintf(out, "entry %ld", r);
        if (lib->libnum & 0x8000)
            fprintf(out, " ($%04X)", (unsigned)(be16(lib->code + 0x0E) + r) & 0xFFFF);
        fprintf(out, " %s +$%X", e->name ? e->name : "(nullsub)", (unsigned)(p - e->offset));
    } else
        fprintf(out, "end of the table +$%X", (unsigned)p);
}

static unsigned decl_relocs(const struct hunk *h, uint32_t codestart)
{
    unsigned n = 0;
    uint32_t i;

    for (i = 0; i < h->nrelocs; i++)
        n += h->reloc_offset[i] < codestart;
    return n;
}

int blitzlib_compare(const struct hunk *orig, const struct hunk *new, const char *name, FILE *out)
{
    struct blitzlib a, b;
    char err[200];
    uint32_t n, i;
    long last = -2;
    int diffs = 0;
    unsigned ra, rb;

    if (blitzlib_read(orig, &a, err, sizeof err)) {
        fprintf(out, "%s: original: %s\n", name, err);
        return -1;
    }
    if (blitzlib_read(new, &b, err, sizeof err)) {
        fprintf(out, "%s: %s\n", name, err);
        blitzlib_free(&a);
        return -1;
    }
    n = a.codestart < b.codestart ? a.codestart : b.codestart;
    for (i = 0; i < n; i++) {
        uint8_t m = a.mask[i];
        uint32_t at = i;
        int differs;
        long r;

        if (m != b.mask[i])
            differs = 1;
        else if (m == BLITZLIB_CODE) {
            differs = i + 4 > n || memcmp(a.mask + i, b.mask + i, 4)
                      || !be32(a.code + i) != !be32(b.code + i);
            i += 3;
        } else
            differs = m == BLITZLIB_DECL && a.code[i] != b.code[i];
        if (!differs)
            continue;
        /* the first difference of each region */
        r = region(&a, at);
        if (r == last)
            continue;
        last = r;
        diffs++;
        fprintf(out, "%s: ", name);
        where(&a, r, at, out);
        if (m == BLITZLIB_CODE && a.mask[at] == b.mask[at])
            fprintf(out, ": code offset set in only one of them\n");
        else
            fprintf(out, ": $%02X != $%02X\n", a.code[at], b.code[at]);
    }
    if (a.nentries != b.nentries) {
        diffs++;
        fprintf(out, "%s: %u entries != %u\n", name, a.nentries, b.nentries);
    }
    if (a.codestart != b.codestart) {
        diffs++;
        fprintf(out, "%s: declaration part $%X != $%X bytes\n", name, (unsigned)a.codestart,
                (unsigned)b.codestart);
    }
    ra = decl_relocs(orig, a.codestart);
    rb = decl_relocs(new, b.codestart);
    if (ra != rb) {
        diffs++;
        fprintf(out, "%s: relocations in the declaration part %u != %u\n", name, ra, rb);
    }
    blitzlib_free(&a);
    blitzlib_free(&b);
    return diffs;
}

long blitzlib_intended_list(char *text, const char **lines, size_t max)
{
    size_t n = 0;
    char *p = text, *e;

    while (*p) {
        e = strchr(p, '\n');
        if (e)
            *e = 0;
        if (*p && *p != '#') {
            if (n == max)
                return -1;
            lines[n++] = p;
        }
        if (!e)
            break;
        p = e + 1;
    }
    return (long)n;
}

int blitzlib_intended(const char *msgs, const char *const *intended, size_t n,
                      unsigned char *seen, FILE *out)
{
    const char *p = msgs, *e;
    size_t len, k;
    int other = 0;

    while (*p) {
        e = strchr(p, '\n');
        len = e ? (size_t)(e - p) : strlen(p);
        for (k = 0; k < n; k++)
            if (strlen(intended[k]) == len && !memcmp(intended[k], p, len))
                break;
        if (k < n) {
            seen[k] = 1;
            fprintf(out, "intended: %.*s\n", (int)len, p);
        } else {
            other++;
            fprintf(out, "%.*s\n", (int)len, p);
        }
        if (!e)
            break;
        p = e + 1;
    }
    return other;
}

int blitzlib_intended_stale(const char *const *intended, size_t n, const unsigned char *seen,
                            FILE *out)
{
    size_t k;
    int stale = 0;

    for (k = 0; k < n; k++)
        if (!seen[k]) {
            fprintf(out, "intended difference not found: %s\n", intended[k]);
            stale++;
        }
    return stale;
}
