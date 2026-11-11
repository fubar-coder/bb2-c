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

        fprintf(out, "entry\t%u\t%s\t", k, e->name && *e->name ? e->name : "-");
        tokens(&lib, e->offset, to, out);
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
        const char *eol = memchr(p, '\n', (size_t)(end - p)), *hex;
        size_t hl, k;
        uint8_t *data;

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
        /* the tokens: the last field */
        for (hex = eol; hex > p && hex[-1] != '\t'; hex--)
            ;
        hl = (size_t)(eol - hex);
        if (hl % 2)
            goto syntax;
        if (!(data = realloc((void *)h->data, h->size + hl / 2 + 1)))
            goto nomem;
        h->data = data;
        for (k = 0; k < hl; k += 2) {
            int hi = hexval(hex[k]), lo = hexval(hex[k + 1]);
            uint8_t b;

            if (hex[k] == '.' && hex[k + 1] == '.')
                b = 0;
            else if (hex[k] == '-' && hex[k + 1] == '-')
                b = 0;
            else if (hex[k] == '+' && hex[k + 1] == '+')
                b = 1;
            else if (hi >= 0 && lo >= 0)
                b = (uint8_t)(hi << 4 | lo);
            else {
                snprintf(err, errlen, "line %u: byte %zu: expected two hex digits, .., ++ or --",
                         line, k / 2);
                goto fail;
            }
            data[h->size++] = b;
        }
        p = eol + 1;
    }
    if (h && finish(h, libline, err, errlen))
        goto fail;
    return 0;
syntax:
    snprintf(err, errlen, "line %u: expected library NUM RELOCS, header HEX, entry K NAME HEX "
             "or end HEX", line);
    goto fail;
nomem:
    snprintf(err, errlen, "out of memory");
fail:
    blitzlib_decls_free(d);
    return -1;
}
