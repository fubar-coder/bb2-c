/*
 * blitztokens.c - the token list of the compiler and its libraries, see
 * blitztokens.h.
 */
#include <stdlib.h>
#include <string.h>
#include "biglib.h"
#include "blitzlib.h"
#include "blitztokens.h"
#include "hunk.h"

static int add(struct blitztokens *list, uint16_t num, const char *name, char *err, size_t errlen)
{
    if (list->n == list->cap) {
        size_t cap = list->cap ? 2 * list->cap : 512;
        struct blitztoken *t = realloc(list->t, cap * sizeof *t);

        if (!t) {
            snprintf(err, errlen, "out of memory");
            return -1;
        }
        list->t = t;
        list->cap = cap;
    }
    list->t[list->n].num = num | 0x8000;
    list->t[list->n].name = name;
    list->n++;
    return 0;
}

/* the relocation of the long at off in hunk h: 0 = found (*target) */
static int reloc_at(const struct hunk *h, uint32_t off, uint32_t *target)
{
    uint32_t i;

    for (i = 0; i < h->nrelocs; i++)
        if (h->reloc_offset[i] == off) {
            *target = h->reloc_target[i];
            return 0;
        }
    return -1;
}

/* a 0-terminated string from off inside h */
static int string_at(const struct hunk *h, uint32_t off)
{
    return off < h->size && memchr(h->data + off, 0, h->size - off) != NULL;
}

/* a first node: next relocated, num 1, "NEWTYPE"; at any offset (the
   nodes of AmiBlitz 2's compilers are not word-aligned) */
static int is_first(const struct hunk *h, uint32_t p)
{
    static const char first[] = "NEWTYPE";
    uint32_t t;

    return p + 6 + sizeof first <= h->size && be16(h->data + p + 4) == 1
           && !memcmp(h->data + p + 6, first, sizeof first) && !reloc_at(h, p, &t);
}

/* the list from the node at hunk/off on, appended to out */
static int walk(const struct loadfile *lf, size_t size, uint32_t hunk, uint32_t off,
                struct blitztokens *out, char *err, size_t errlen)
{
    size_t count = 0, limit = size / 8;         /* a node has at least 8 bytes */

    for (;;) {
        const struct hunk *h = &lf->hunks[hunk];
        uint32_t next, target;

        if (off > h->size || h->size - off < 7 || !string_at(h, off + 6)) {
            snprintf(err, errlen, "token node at hunk %u $%X outside its hunk",
                     (unsigned)hunk, (unsigned)off);
            return -1;
        }
        if (++count > limit) {
            snprintf(err, errlen, "token list does not end (a loop?)");
            return -1;
        }
        if (add(out, be16(h->data + off + 4), (const char *)h->data + off + 6, err, errlen))
            return -1;
        next = be32(h->data + off);
        if (reloc_at(h, off, &target)) {
            if (next) {
                snprintf(err, errlen, "token node at hunk %u $%X: next $%X without relocation",
                         (unsigned)hunk, (unsigned)off, (unsigned)next);
                return -1;
            }
            return 0;                   /* the last node: next 0 */
        }
        if (target >= lf->nhunks) {
            snprintf(err, errlen, "token node at hunk %u $%X: next into hunk %u",
                     (unsigned)hunk, (unsigned)off, (unsigned)target);
            return -1;
        }
        hunk = target;
        off = next;
    }
}

int blitztokens_compiler_last(const uint8_t *buf, size_t size, uint16_t last,
                              struct blitztokens *out, char *err, size_t errlen)
{
    struct loadfile lf;
    struct blitztokens found = { 0 };
    uint32_t i, p, fhunk = 0, foff = 0;
    unsigned nfirst = 0, nfound = 0;
    int r = -1;

    if (load_parse(buf, size, &lf, err, errlen))
        return -1;
    for (i = 0; i < lf.nhunks; i++) {
        const struct hunk *h = &lf.hunks[i];

        if (!h->data)
            continue;
        for (p = 0; p + 6 <= h->size; p++) {
            struct blitztokens l = { 0 };

            if (!is_first(h, p))
                continue;
            nfirst++;
            if (!last) {                /* the only list */
                if (nfirst > 1) {
                    snprintf(err, errlen, "token 1 \"NEWTYPE\" found twice (hunk %u $%X, hunk %u $%X)",
                             (unsigned)fhunk, (unsigned)foff, (unsigned)i, (unsigned)p);
                    goto done;
                }
                fhunk = i;
                foff = p;
                continue;
            }
            /* the list that ends with token `last` */
            if (walk(&lf, size, i, p, &l, err, errlen)) {
                blitztokens_free(&l);
                goto done;
            }
            if (l.n && l.t[l.n - 1].num == (last | 0x8000)) {
                if (nfound++) {
                    snprintf(err, errlen, "two token lists end with $%04X (hunk %u $%X, hunk %u $%X)",
                             (unsigned)(last | 0x8000), (unsigned)fhunk, (unsigned)foff,
                             (unsigned)i, (unsigned)p);
                    blitztokens_free(&l);
                    goto done;
                }
                blitztokens_free(&found);
                found = l;
                fhunk = i;
                foff = p;
            } else
                blitztokens_free(&l);
        }
    }
    if (!nfirst) {
        snprintf(err, errlen, "no token list (no node of token 1 \"NEWTYPE\")");
        goto done;
    }
    if (!last) {
        r = walk(&lf, size, fhunk, foff, out, err, errlen);
        goto done;
    }
    if (!nfound) {
        snprintf(err, errlen, "no token list ends with $%04X", (unsigned)(last | 0x8000));
        goto done;
    }
    for (i = 0; i < found.n; i++)
        if (add(out, found.t[i].num, found.t[i].name, err, errlen))
            goto done;
    r = 0;
done:
    blitztokens_free(&found);
    load_free(&lf);
    return r;
}

int blitztokens_compiler(const uint8_t *buf, size_t size, struct blitztokens *out,
                         char *err, size_t errlen)
{
    return blitztokens_compiler_last(buf, size, 0, out, err, errlen);
}

/* the tokens of one library (blitz2.s fixlibs) */
static int library(const uint8_t *data, size_t size, struct blitztokens *out,
                   char *err, size_t errlen)
{
    struct loadfile lf;
    struct blitzlib lib;
    uint16_t num;
    unsigned i;
    int r = -1;

    if (load_parse(data, size, &lf, err, errlen))
        return -1;
    if (!lf.nhunks || lf.hunks[0].type != HUNK_CODE) {
        snprintf(err, errlen, "no code hunk");
        load_free(&lf);
        return -1;
    }
    if (blitzlib_read(&lf.hunks[0], &lib, err, errlen)) {
        load_free(&lf);
        return -1;
    }
    /* user library: libnum*128+1; system library: the number itself */
    num = lib.libnum & 0x8000 ? lib.libnum : (uint16_t)((lib.libnum << 7) + 1);
    for (i = 0; i < lib.nentries; i++, num++) {
        const struct blitzlib_entry *e = &lib.entries[i];

        if (e->kind != 8 && !(e->kind & 4) && (e->kind & 0xF) == 0)
            continue;                   /* nullsub: counted, no token */
        if (add(out, num, e->name, err, errlen))
            goto done;
    }
    r = 0;
done:
    blitzlib_free(&lib);
    load_free(&lf);
    return r;
}

int blitztokens_libraries(const uint8_t *buf, size_t size, struct blitztokens *out,
                          char *err, size_t errlen)
{
    struct biglib_entry *e;
    size_t n, i;
    char why[200];

    if (size >= 4 && be32(buf) == HUNK_HEADER)
        return library(buf, size, out, err, errlen);
    if (biglib_read(buf, size, &e, &n, err, errlen))
        return -1;
    for (i = 0; i < n; i++)
        if (library(e[i].data, e[i].size, out, why, sizeof why)) {
            snprintf(err, errlen, "library %zu: %s", i + 1, why);
            free(e);
            return -1;
        }
    free(e);
    return 0;
}

int blitztokens_write(FILE *f, const struct blitztokens *list)
{
    size_t i;

    for (i = 0; i < list->n; i++)
        if (fprintf(f, "%04X %s\n", list->t[i].num, list->t[i].name) < 0)
            return -1;
    return 0;
}

static int hexdigit(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    return -1;
}

int blitztokens_read(char *buf, size_t size, struct blitztokens *out, char *err,
                     size_t errlen)
{
    size_t pos = 0, line = 0;

    while (pos < size) {
        char *s = buf + pos, *nl = memchr(s, '\n', size - pos);
        size_t len = nl ? (size_t)(nl - s) : size - pos;
        unsigned num = 0;
        int i, d;

        line++;
        pos += len + (nl != NULL);
        if (len && s[len - 1] == '\r')
            len--;
        if (!len)
            continue;
        for (i = 0; i < 4 && (size_t)i < len && (d = hexdigit(s[i])) >= 0; i++)
            num = num << 4 | (unsigned)d;
        if (i < 4 || len < 6 || s[4] != ' ' || memchr(s, 0, len)) {
            snprintf(err, errlen, "line %zu: not \"XXXX name\"", line);
            return -1;
        }
        if (!(num & 0x8000)) {
            snprintf(err, errlen, "line %zu: token $%04X without bit 15", line, num);
            return -1;
        }
        s[len] = 0;
        if (add(out, (uint16_t)num, s + 5, err, errlen))
            return -1;
    }
    return 0;
}

void blitztokens_free(struct blitztokens *list)
{
    free(list->t);
    memset(list, 0, sizeof *list);
}
