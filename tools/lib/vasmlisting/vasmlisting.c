/*
 * vasmlisting.c - reading the listing of vasm, see vasmlisting.h.
 */
#define _XOPEN_SOURCE 700       /* getline, strdup */
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "vasmlisting.h"

static int is_hex(int c)
{
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F');
}

static int hexval(int c)
{
    return c <= '9' ? c - '0' : c - 'A' + 10;
}

static int is_space(int c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\v' || c == '\f';
}

/* 8 upper-case hex digits at p */
static int hex8(const char *p, uint32_t *v)
{
    int i;

    *v = 0;
    for (i = 0; i < 8; i++) {
        if (!is_hex((unsigned char)p[i]))
            return 0;
        *v = *v << 4 | (uint32_t)hexval((unsigned char)p[i]);
    }
    return 1;
}

static void *grow(void *p, size_t n, size_t size)
{
    return n % 256 == 0 ? realloc(p, (n + 256) * size) : p;
}

/* 00:AAAAAAAA HEX<ws with a tab>LINE: TEXT */
static int parse_row(const char *s, struct listing_row *r)
{
    const char *p = s + 12, *hex, *ws;
    uint32_t addr;
    size_t nhex, i;
    int tab = 0;

    if (strncmp(s, "00:", 3) || !hex8(s + 3, &addr) || s[11] != ' ')
        return 0;
    hex = p;
    while (is_hex((unsigned char)*p))
        p++;
    nhex = (size_t)(p - hex);
    if (nhex % 2)
        return 0;
    ws = p;
    while (is_space((unsigned char)*p))
        tab |= *p++ == '\t';
    if (!tab || p == ws || !isdigit((unsigned char)*p))
        return 0;
    r->line = (unsigned)strtoul(p, (char **)&p, 10);
    if (*p != ':')
        return 0;
    p++;
    if (*p == ' ')
        p++;
    r->addr = addr;
    r->nbytes = nhex / 2;
    r->bytes = malloc(r->nbytes ? r->nbytes : 1);
    r->text = strdup(p);
    if (!r->bytes || !r->text) {
        free(r->bytes);
        free(r->text);
        return -1;
    }
    for (i = 0; i < r->nbytes; i++)
        r->bytes[i] = (uint8_t)(hexval((unsigned char)hex[2 * i]) << 4
                                | hexval((unsigned char)hex[2 * i + 1]));
    return 1;
}

/* NAME<ws>00:AAAAAAAA or NAME<ws>A:AAAAAAAA, optional white space behind */
static int parse_symbol(const char *s, struct listing_symbol *sym)
{
    const char *p = s, *name = s;
    size_t n;

    if (!(isalpha((unsigned char)*p) || *p == '_' || *p == '.'))
        return 0;
    while (isalnum((unsigned char)*p) || *p == '_' || *p == '.')
        p++;
    n = (size_t)(p - name);
    if (!is_space((unsigned char)*p))
        return 0;
    while (is_space((unsigned char)*p))
        p++;
    if (!strncmp(p, "00:", 3))
        p += 3;
    else if (!strncmp(p, "A:", 2))
        p += 2;
    else
        return 0;
    if (!hex8(p, &sym->value))
        return 0;
    p += 8;
    while (is_space((unsigned char)*p))
        p++;
    if (*p)
        return 0;
    if (!(sym->name = strndup(name, n)))
        return -1;
    return 1;
}

static int fail(struct listing *l, char *err, size_t errlen, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(err, errlen, fmt, ap);
    va_end(ap);
    listing_free(l);
    return -1;
}

int listing_read(const char *path, struct listing *l, char *err, size_t errlen)
{
    FILE *f = fopen(path, "r");
    int r;

    if (!f) {
        memset(l, 0, sizeof *l);
        return fail(l, err, errlen, "cannot open %s", path);
    }
    r = listing_read_stream(f, l, err, errlen);
    fclose(f);
    return r;
}

int listing_read_stream(FILE *f, struct listing *l, char *err, size_t errlen)
{
    char *line = NULL;
    size_t cap = 0;
    ssize_t n;
    const char *cur = "?";
    int in_syms = 0;

    memset(l, 0, sizeof *l);
    while ((n = getline(&line, &cap, f)) >= 0) {
        int r;

        if (n && line[n - 1] == '\n')
            line[--n] = 0;
        if (!strncmp(line, "Symbols by name", 15)) {
            in_syms = 1;
            continue;
        }
        if (!strncmp(line, "Symbols by value", 16))
            break;
        if (in_syms) {
            struct listing_symbol sym;

            if (!(l->syms = grow(l->syms, l->nsyms, sizeof *l->syms)))
                goto nomem;
            r = parse_symbol(line, &sym);
            if (r < 0)
                goto nomem;
            if (r)
                l->syms[l->nsyms++] = sym;
            continue;
        }
        if (!strncmp(line, "Source: \"", 9)) {
            char *e = strchr(line + 9, '"');

            if (e) {
                if (!(l->files = grow(l->files, l->nfiles, sizeof *l->files))
                    || !(l->files[l->nfiles] = strndup(line + 9, (size_t)(e - line - 9))))
                    goto nomem;
                cur = l->files[l->nfiles++];
                continue;
            }
        }
        if (!(l->rows = grow(l->rows, l->nrows, sizeof *l->rows)))
            goto nomem;
        r = parse_row(line, &l->rows[l->nrows]);
        if (r < 0)
            goto nomem;
        if (r)
            l->rows[l->nrows++].file = cur;
    }
    free(line);
    return 0;
nomem:
    free(line);
    return fail(l, err, errlen, "out of memory");
}

void listing_free(struct listing *l)
{
    size_t i;

    for (i = 0; i < l->nrows; i++) {
        free(l->rows[i].bytes);
        free(l->rows[i].text);
    }
    for (i = 0; i < l->nsyms; i++)
        free(l->syms[i].name);
    for (i = 0; i < l->nfiles; i++)
        free(l->files[i]);
    free(l->rows);
    free(l->syms);
    free(l->files);
    memset(l, 0, sizeof *l);
}

const struct listing_row *listing_defrow(const struct listing *l, const char *name)
{
    size_t i, n = strlen(name);

    for (i = 0; i < l->nrows; i++) {
        const char *t = l->rows[i].text;
        size_t k = 0;

        if (!(isalpha((unsigned char)t[0]) || t[0] == '_'))
            continue;
        while (isalnum((unsigned char)t[k]) || t[k] == '_')
            k++;
        if (k == n && !strncasecmp(t, name, n))
            return &l->rows[i];
    }
    return NULL;
}

/* ---------------------------------------------------------- views */

/* the text without leading and trailing white space */
static void print_stripped(const char *s, FILE *out)
{
    size_t n;

    while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\v' || *s == '\f')
        s++;
    n = strlen(s);
    while (n && (s[n - 1] == ' ' || s[n - 1] == '\t' || s[n - 1] == '\r'
                 || s[n - 1] == '\v' || s[n - 1] == '\f'))
        n--;
    fwrite(s, 1, n, out);
}

void listing_print_map(const struct listing *l, FILE *out)
{
    size_t i, k;

    for (i = 0; i < l->nrows; i++) {
        const struct listing_row *r = &l->rows[i];

        if (!r->nbytes)
            continue;
        fprintf(out, "%06X\t%s:%u\t", (unsigned)r->addr, r->file, r->line);
        for (k = 0; k < r->nbytes; k++)
            fprintf(out, "%02x", r->bytes[k]);
        fputc('\t', out);
        print_stripped(r->text, out);
        fputc('\n', out);
    }
}

/* symbol index with its value: sorted by value, equal values keep the
   order of the listing (stable) */
struct symref {
    uint32_t value;
    size_t index;
};

static int by_value(const void *a, const void *b)
{
    const struct symref *x = a, *y = b;

    if (x->value != y->value)
        return x->value < y->value ? -1 : 1;
    return x->index < y->index ? -1 : x->index > y->index;
}

int listing_print_labels(const struct listing *l, FILE *out)
{
    struct symref *refs = malloc((l->nsyms ? l->nsyms : 1) * sizeof *refs);
    size_t i;

    if (!refs)
        return -1;
    for (i = 0; i < l->nsyms; i++) {
        refs[i].value = l->syms[i].value;
        refs[i].index = i;
    }
    qsort(refs, l->nsyms, sizeof *refs, by_value);
    for (i = 0; i < l->nsyms; i++) {
        const struct listing_symbol *s = &l->syms[refs[i].index];
        const struct listing_row *r = listing_defrow(l, s->name);

        fprintf(out, "%06X\t%s\t%s:%u\n", (unsigned)s->value, s->name, r ? r->file : "?",
                r ? r->line : 0);
    }
    free(refs);
    return 0;
}

size_t listing_verify(const struct listing *l, const struct hunk *h, FILE *out)
{
    unsigned char *skip = calloc((size_t)h->size + 4, 1);
    size_t i, k, bad = 0;

    if (!skip)
        return l->nrows ? l->nrows : 1;
    for (i = 0; i < h->nrelocs; i++)
        for (k = 0; k < 4 && h->reloc_offset[i] + k < h->size; k++)
            skip[h->reloc_offset[i] + k] = 1;
    for (i = 0; i < l->nrows; i++) {
        const struct listing_row *r = &l->rows[i];

        for (k = 0; k < r->nbytes; k++) {
            uint32_t a = r->addr + (uint32_t)k;

            if (a < h->size && skip[a])
                continue;
            if (a >= h->size || h->data[a] != r->bytes[k]) {
                if (++bad <= 20) {
                    fprintf(out, "diff %06X %s:%u ", (unsigned)a, r->file, r->line);
                    print_stripped(r->text, out);
                    fputc('\n', out);
                }
                break;
            }
        }
    }
    free(skip);
    return bad;
}
