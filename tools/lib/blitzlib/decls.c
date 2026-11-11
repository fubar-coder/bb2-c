/*
 * decls.c - the declaration part of a library as text and back, see
 * blitzlib.h (blitzlib_decls_write, blitzlib_decls_parse).
 */
#define _XOPEN_SOURCE 700       /* strndup */
#include <stdlib.h>
#include <string.h>
#include "blitzlib.h"

/* the bytes [from, to) of lib as tokens */
static void tokens(const struct blitzlib *lib, uint32_t from, uint32_t to, FILE *out)
{
    uint32_t i;

    for (i = from; i < to; i++)
        if (lib->mask[i] == BLITZLIB_IGNORE)
            fputs("..", out);
        else if (lib->mask[i] == BLITZLIB_CODE) {
            /* a code offset (a long): only set or not */
            const char *t = i + 4 <= to && be32(lib->code + i) ? "++" : "--";

            fprintf(out, "%s%s%s%s", t, t, t, t);
            i += 3;
        } else
            fprintf(out, "%02x", lib->code[i]);
}

/* the bytes [from, to) of lib are all declaration bytes */
static int decl_bytes(const struct blitzlib *lib, uint32_t from, uint32_t to)
{
    for (; from < to; from++)
        if (lib->mask[from] != BLITZLIB_DECL)
            return 0;
    return 1;
}

/* a 0-terminated text: printable ASCII as it is, a backslash as \\, any
   other byte as \xHH */
static void text(const char *s, FILE *out)
{
    for (; *s; s++) {
        unsigned char c = (unsigned char)*s;

        if (c == '\\')
            fputs("\\\\", out);
        else if (c >= 0x20 && c < 0x7F)
            fputc(c, out);
        else
            fprintf(out, "\\x%02x", c);
    }
}

int blitzlib_decls_write(const struct hunk *h, FILE *out, char *err, size_t errlen)
{
    struct blitzlib lib;
    uint32_t end, i;
    unsigned relocs = 0;
    unsigned k;

    if (blitzlib_read(h, &lib, err, errlen))
        return -1;
    for (i = 0; i < h->nrelocs; i++)
        relocs += h->reloc_offset[i] < lib.codestart;
    end = lib.libfin ? lib.libfin - 2 : lib.codestart - 6;
    fprintf(out, "library\t%u\t%u\n", (unsigned)lib.libnum, relocs);
    fputs("header\t", out);
    tokens(&lib, 0, 0x30, out);
    fputc('\n', out);
    for (k = 0; k < lib.nentries; k++) {
        const struct blitzlib_entry *e = &lib.entries[k];
        uint32_t to = k + 1 < lib.nentries ? lib.entries[k + 1].offset : end;

        uint32_t name = e->name ? (uint32_t)((const uint8_t *)e->name - lib.code) : 0;
        uint32_t help = e->name ? (uint32_t)((const uint8_t *)e->help - lib.code) : 0;
        uint32_t tail = e->name ? help + (uint32_t)strlen(e->help) + 1 : 0;

        if (e->name && name >= e->offset && tail <= to && decl_bytes(&lib, name, tail)) {
            /* name and help text as text, the bytes before and behind them
               as tokens */
            fprintf(out, "entry\t%u\t", k);
            text(e->name, out);
            fputc('\t', out);
            text(e->help, out);
            fputc('\t', out);
            tokens(&lib, e->offset, name, out);
            fputc('\t', out);
            tokens(&lib, tail, to, out);
        } else {
            fprintf(out, "entry\t%u\t-\t", k);
            tokens(&lib, e->offset, to, out);
        }
        fputc('\n', out);
    }
    fputs("end\t", out);
    tokens(&lib, lib.nentries ? end : 0x30, lib.codestart, out);
    fputc('\n', out);
    blitzlib_free(&lib);
    return 0;
}

void blitzlib_decls_free(struct blitzlib_decls *d)
{
    size_t i;

    for (i = 0; i < d->n; i++) {
        free((void *)d->hunks[i].data);
        free(d->hunks[i].reloc_offset);
        free(d->hunks[i].reloc_target);
    }
    free(d->hunks);
    d->hunks = NULL;
    d->n = 0;
}

static int hexval(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    return -1;
}

/* make room in h for n more bytes; 0 = ok, -1 out of memory */
static int room(struct hunk *h, size_t n)
{
    uint8_t *data = realloc((void *)h->data, h->size + n + 1);

    if (!data)
        return -1;
    h->data = data;
    return 0;
}

/* append the tokens [s, e) to h; 0 = ok, -1 out of memory, 1 broken (err) */
static int add_tokens(struct hunk *h, const char *s, const char *e, unsigned line, char *err,
                      size_t errlen)
{
    size_t hl = (size_t)(e - s), k;
    uint8_t *data;

    if (hl % 2) {
        snprintf(err, errlen, "line %u: an odd number of token characters", line);
        return 1;
    }
    if (room(h, hl / 2))
        return -1;
    data = (uint8_t *)h->data;
    for (k = 0; k < hl; k += 2) {
        int hi = hexval(s[k]), lo = hexval(s[k + 1]);
        uint8_t b;

        if (s[k] == '.' && s[k + 1] == '.')
            b = 0;
        else if (s[k] == '-' && s[k + 1] == '-')
            b = 0;
        else if (s[k] == '+' && s[k + 1] == '+')
            b = 1;
        else if (hi >= 0 && lo >= 0)
            b = (uint8_t)(hi << 4 | lo);
        else {
            snprintf(err, errlen, "line %u: byte %zu: expected two hex digits, .., ++ or --",
                     line, k / 2);
            return 1;
        }
        data[h->size++] = b;
    }
    return 0;
}

/* append the text [s, e) (with the escapes of text()) and a 0 to h;
   0 = ok, -1 out of memory, 1 broken (err) */
static int add_text(struct hunk *h, const char *s, const char *e, unsigned line, char *err,
                    size_t errlen)
{
    const char *p;
    uint8_t *data;

    if (room(h, (size_t)(e - s) + 1))
        return -1;
    data = (uint8_t *)h->data;
    for (p = s; p < e; p++) {
        unsigned char c = (unsigned char)*p;

        if (c == '\\') {
            int hi = e - p >= 4 && p[1] == 'x' ? hexval(p[2]) : -1;
            int lo = hi >= 0 ? hexval(p[3]) : -1;

            if (p + 1 < e && p[1] == '\\')
                p++;
            else if (lo >= 0 && (hi || lo) && (hi << 4 | lo) != '\\' &&
                     ((hi << 4 | lo) < 0x20 || (hi << 4 | lo) >= 0x7F)) {
                c = (unsigned char)(hi << 4 | lo);
                p += 3;
            } else {
                snprintf(err, errlen, "line %u: text column %zu: expected \\\\ or \\xHH (a byte "
                         "$01-$1F or $7F-$FF)", line, (size_t)(p - s) + 1);
                return 1;
            }
        } else if (c < 0x20 || c >= 0x7F) {
            snprintf(err, errlen, "line %u: text column %zu: a byte that is not printable "
                     "ASCII (written \\xHH)", line, (size_t)(p - s) + 1);
            return 1;
        }
        data[h->size++] = c;
    }
    data[h->size++] = 0;
    return 0;
}

/* the library being read: check it reads and ends where its text ends */
static int finish(struct hunk *h, unsigned line, char *err, size_t errlen)
{
    struct blitzlib lib;
    char e[160];
    uint32_t codestart;

    if (blitzlib_read(h, &lib, e, sizeof e)) {
        snprintf(err, errlen, "library of line %u: %s", line, e);
        return -1;
    }
    codestart = lib.codestart;
    blitzlib_free(&lib);
    if (codestart != h->size) {
        snprintf(err, errlen, "library of line %u: the declaration part is $%X bytes, the text "
                 "has $%X", line, (unsigned)codestart, (unsigned)h->size);
        return -1;
    }
    return 0;
}

int blitzlib_decls_parse(const char *text, size_t len, struct blitzlib_decls *d, char *err,
                         size_t errlen)
{
    const char *p = text, *end = text + len;
    unsigned line = 0, libline = 0;
    struct hunk *h = NULL;
    size_t cap = 0;

    d->hunks = NULL;
    d->n = 0;
    while (p < end) {
        const char *eol = memchr(p, '\n', (size_t)(end - p)), *q, *fs[4], *f[4];
        size_t k;
        int nf, ent, r;

        if (!eol)
            eol = end;
        line++;
        if (p == eol || *p == '#') {
            p = eol + 1;
            continue;
        }
        if (!strncmp(p, "library\t", 8)) {
            unsigned long num, relocs;
            char *q;

            if (h && finish(h, libline, err, errlen))
                goto fail;
            num = strtoul(p + 8, &q, 10);
            if (*q != '\t' || q == p + 8 || num > 0xFFFF)
                goto syntax;
            relocs = strtoul(q + 1, &q, 10);
            if (q != eol || relocs > 100000)
                goto syntax;
            if (d->n == cap) {
                struct hunk *n = realloc(d->hunks, (cap + 16) * sizeof *n);

                if (!n)
                    goto nomem;
                d->hunks = n;
                cap += 16;
            }
            h = &d->hunks[d->n++];
            memset(h, 0, sizeof *h);
            h->type = HUNK_CODE;
            h->nrelocs = (uint32_t)relocs;
            /* only their number counts (in the declaration part) */
            h->reloc_offset = calloc(relocs + 1, sizeof *h->reloc_offset);
            h->reloc_target = calloc(relocs + 1, sizeof *h->reloc_target);
            if (!h->reloc_offset || !h->reloc_target)
                goto nomem;
            libline = line;
            p = eol + 1;
            continue;
        }
        if (!h || (strncmp(p, "header\t", 7) && strncmp(p, "entry\t", 6) && strncmp(p, "end\t", 4)))
            goto syntax;
        /* the fields behind the kind (and behind the number of an entry) */
        q = (const char *)memchr(p, '\t', (size_t)(eol - p)) + 1;
        ent = p[1] == 'n' && p[2] == 't';
        if (ent) {
            k = strspn(q, "0123456789");
            if (!k || q + k >= eol || q[k] != '\t')
                goto syntax;
            q += k + 1;
        }
        for (nf = 0;; q = f[nf - 1] + 1) {
            const char *t = memchr(q, '\t', (size_t)(eol - q));

            if (nf == 4)
                goto syntax;
            fs[nf] = q;
            f[nf++] = t ? t : eol;
            if (!t)
                break;
        }
        if (!ent) {
            if (nf != 1)
                goto syntax;
            r = add_tokens(h, fs[0], f[0], line, err, errlen);
        } else if (nf == 2) {
            /* - TOKENS: a nullsub (or an entry without a name block as text) */
            if (f[0] - fs[0] != 1 || *fs[0] != '-')
                goto syntax;
            r = add_tokens(h, fs[1], f[1], line, err, errlen);
        } else if (nf == 4) {
            /* NAME HELP TOKENS TOKENS: the bytes before and behind the texts */
            r = add_tokens(h, fs[2], f[2], line, err, errlen);
            if (!r)
                r = add_text(h, fs[0], f[0], line, err, errlen);
            if (!r)
                r = add_text(h, fs[1], f[1], line, err, errlen);
            if (!r)
                r = add_tokens(h, fs[3], f[3], line, err, errlen);
        } else
            goto syntax;
        if (r < 0)
            goto nomem;
        if (r)
            goto fail;
        p = eol + 1;
    }
    if (h && finish(h, libline, err, errlen))
        goto fail;
    return 0;
syntax:
    snprintf(err, errlen, "line %u: expected library NUM RELOCS, header TOKENS, entry K NAME "
             "HELP TOKENS TOKENS, entry K - TOKENS or end TOKENS", line);
    goto fail;
nomem:
    snprintf(err, errlen, "out of memory");
fail:
    blitzlib_decls_free(d);
    return -1;
}
