/*
 * hunk.c - reading AmigaDOS load files (hunk format) on the host, see
 * hunk.h.
 */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hunk.h"

uint16_t be16(const uint8_t *p)
{
    return (uint16_t)(p[0] << 8 | p[1]);
}

uint32_t be32(const uint8_t *p)
{
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
}

struct reader {
    const uint8_t *buf;
    size_t size, pos;
    int bad;                    /* read past the end */
};

static uint32_t rd32(struct reader *r)
{
    uint32_t v;

    if (r->bad || r->size - r->pos < 4) {
        r->bad = 1;
        return 0;
    }
    v = be32(r->buf + r->pos);
    r->pos += 4;
    return v;
}

static uint16_t rd16(struct reader *r)
{
    uint16_t v;

    if (r->bad || r->size - r->pos < 2) {
        r->bad = 1;
        return 0;
    }
    v = be16(r->buf + r->pos);
    r->pos += 2;
    return v;
}

/* skip n longs */
static void skip(struct reader *r, uint32_t n)
{
    if (r->bad || (r->size - r->pos) / 4 < n) {
        r->bad = 1;
        return;
    }
    r->pos += (size_t)n * 4;
}

static int fail(struct loadfile *lf, char *err, size_t errlen, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(err, errlen, fmt, ap);
    va_end(ap);
    load_free(lf);
    return -1;
}

static int add_symbol(struct hunk *h, const char *name, uint32_t max, uint32_t value)
{
    uint32_t len = 0;

    if (h->nsyms % 64 == 0) {
        struct hunk_symbol *s = realloc(h->syms, (h->nsyms + 64) * sizeof *s);

        if (!s)
            return -1;
        h->syms = s;
    }
    while (len < max && name[len])
        len++;
    h->syms[h->nsyms].name = name;
    h->syms[h->nsyms].len = len;
    h->syms[h->nsyms].value = value;
    h->nsyms++;
    return 0;
}

static int add_reloc(struct hunk *h, uint32_t offset, uint32_t target)
{
    if (h->nrelocs % 64 == 0) {
        uint32_t *o = realloc(h->reloc_offset, (h->nrelocs + 64) * sizeof *o);
        uint32_t *t;

        if (!o)
            return -1;
        h->reloc_offset = o;
        t = realloc(h->reloc_target, (h->nrelocs + 64) * sizeof *t);
        if (!t)
            return -1;
        h->reloc_target = t;
    }
    h->reloc_offset[h->nrelocs] = offset;
    h->reloc_target[h->nrelocs] = target;
    h->nrelocs++;
    return 0;
}

int load_parse(const uint8_t *buf, size_t size, struct loadfile *lf, char *err, size_t errlen)
{
    struct reader r = { buf, size, 0, 0 };
    long cur = -1;              /* index of the open hunk */
    uint32_t n, i, first, last;
    uint32_t flags[64] = { 0 };                 /* of the first 64 hunks */
    uint32_t sizes[64] = { 0 };                 /* longs, of the first 64 hunks */

    memset(lf, 0, sizeof *lf);
    lf->data = buf;
    lf->size = size;
    if (rd32(&r) != HUNK_HEADER)
        return fail(lf, err, errlen, "not a load file (no HUNK_HEADER)");
    while ((n = rd32(&r)) != 0 && !r.bad)       /* resident library names */
        skip(&r, n);
    lf->table_size = rd32(&r);
    first = rd32(&r);
    last = rd32(&r);
    if (r.bad || last < first)
        return fail(lf, err, errlen, "broken HUNK_HEADER");
    for (i = first; i <= last && !r.bad; i++) {
        uint32_t sz = rd32(&r);

        if (i - first < sizeof flags / sizeof flags[0]) {
            flags[i - first] = sz & 0xC0000000;
            sizes[i - first] = sz & 0x3FFFFFFF;
        }
        if ((sz & 0xC0000000) == 0xC0000000)
            rd32(&r);                           /* extended memory flags */
    }

    while (!r.bad && r.pos < r.size) {
        size_t at = r.pos;
        uint32_t t = rd32(&r) & 0x3FFFFFFF;
        struct hunk *h;

        switch (t) {
        case HUNK_CODE:
        case HUNK_DATA:
        case HUNK_BSS:
            if (lf->nhunks % 16 == 0) {
                struct hunk *nh = realloc(lf->hunks, (lf->nhunks + 16) * sizeof *nh);

                if (!nh)
                    return fail(lf, err, errlen, "out of memory");
                lf->hunks = nh;
            }
            cur = lf->nhunks++;
            h = &lf->hunks[cur];
            memset(h, 0, sizeof *h);
            h->type = t;
            if (cur < (long)(sizeof flags / sizeof flags[0])) {
                h->memflags = flags[cur];
                h->memsize = sizes[cur] * 4;
            }
            n = rd32(&r) & 0x3FFFFFFF;
            h->size = n * 4;
            if (t != HUNK_BSS) {
                h->data = buf + r.pos;
                skip(&r, n);
            }
            break;
        case HUNK_RELOC32:
            if (cur < 0)
                return fail(lf, err, errlen, "HUNK_RELOC32 at %zu outside a hunk", at);
            while ((n = rd32(&r)) != 0 && !r.bad) {
                uint32_t target = rd32(&r);

                while (n-- && !r.bad)
                    if (add_reloc(&lf->hunks[cur], rd32(&r), target))
                        return fail(lf, err, errlen, "out of memory");
            }
            break;
        case HUNK_RELOC32SHORT:
        case HUNK_DREL32:
        case HUNK_RELRELOC32:
            if (cur < 0)
                return fail(lf, err, errlen, "relocation hunk $%X at %zu outside a hunk", t, at);
            while ((n = rd16(&r)) != 0 && !r.bad) {
                uint32_t target = rd16(&r);

                while (n-- && !r.bad)
                    if (add_reloc(&lf->hunks[cur], rd16(&r), target))
                        return fail(lf, err, errlen, "out of memory");
            }
            if (r.pos & 2)                      /* padded to a long */
                rd16(&r);
            break;
        case HUNK_SYMBOL:
            while ((n = rd32(&r)) != 0 && !r.bad) {
                const char *name = (const char *)buf + r.pos;
                uint32_t value;

                skip(&r, n);                    /* name, padded with 0 to longs */
                value = rd32(&r);
                if (!r.bad && cur >= 0 && add_symbol(&lf->hunks[cur], name, n * 4, value))
                    return fail(lf, err, errlen, "out of memory");
            }
            break;
        case HUNK_DEBUG:
        case HUNK_NAME:
            skip(&r, rd32(&r));
            break;
        case HUNK_END:
            cur = -1;
            break;
        default:
            return fail(lf, err, errlen, "unknown hunk type $%X at %zu", t, at);
        }
    }
    if (r.bad)
        return fail(lf, err, errlen, "file ends inside a hunk");
    return 0;
}

int load_read(const char *path, struct loadfile *lf, char *err, size_t errlen)
{
    FILE *f = fopen(path, "rb");
    uint8_t *buf;
    long size;

    memset(lf, 0, sizeof *lf);
    if (!f)
        return fail(lf, err, errlen, "cannot open");
    if (fseek(f, 0, SEEK_END) || (size = ftell(f)) < 0 || fseek(f, 0, SEEK_SET)) {
        fclose(f);
        return fail(lf, err, errlen, "cannot read");
    }
    buf = malloc(size ? (size_t)size : 1);
    if (!buf || fread(buf, 1, (size_t)size, f) != (size_t)size) {
        free(buf);
        fclose(f);
        return fail(lf, err, errlen, "cannot read");
    }
    fclose(f);
    if (load_parse(buf, (size_t)size, lf, err, errlen)) {
        free(buf);
        return -1;
    }
    lf->buf = buf;
    return 0;
}

void load_free(struct loadfile *lf)
{
    uint32_t i;

    for (i = 0; i < lf->nhunks; i++) {
        free(lf->hunks[i].reloc_offset);
        free(lf->hunks[i].reloc_target);
        free(lf->hunks[i].syms);
    }
    free(lf->hunks);
    free(lf->buf);
    memset(lf, 0, sizeof *lf);
}

int load_symbol(const struct loadfile *lf, const char *name, uint32_t *hunk, uint32_t *value)
{
    size_t n = strlen(name);
    uint32_t i, k;
    int found = -1;

    for (i = 0; i < lf->nhunks; i++)
        for (k = 0; k < lf->hunks[i].nsyms; k++) {
            const struct hunk_symbol *s = &lf->hunks[i].syms[k];

            if (s->len == n && !memcmp(s->name, name, n)) {
                *hunk = i;
                *value = s->value;
                found = 0;
            }
        }
    return found;
}
