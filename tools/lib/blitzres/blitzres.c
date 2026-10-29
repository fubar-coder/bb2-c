/*
 * blitzres.c - Blitz Basic 2 resident files, see blitzres.h.
 */
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "blitzres.h"
#include "hunk.h"

static int fail(char *err, size_t errlen, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(err, errlen, fmt, ap);
    va_end(ap);
    return -1;
}

/* ------------------------------------------------------------ the list */

int blitzres_valid_name(const char *name)
{
    size_t i;

    if (!isalpha((unsigned char)name[0]) && name[0] != '_')
        return 0;
    for (i = 1; name[i]; i++)
        if (!isalnum((unsigned char)name[i]) && name[i] != '_')
            return 0;
    return i <= BLITZRES_MAXNAME;
}

int blitzres_find(const struct blitzres_list *l, const char *name, int32_t *value)
{
    size_t i;

    for (i = 0; i < l->n; i++)
        if (!strcmp(l->c[i].name, name)) {
            *value = l->c[i].value;
            return 0;
        }
    return -1;
}

int blitzres_skip(struct blitzres_list *l, const char *name, char *err, size_t errlen)
{
    char **s;

    if (!blitzres_valid_name(name))
        return fail(err, errlen, "\"%s\" is not a constant name", name);
    if (!(s = realloc(l->skip, (l->nskip + 1) * sizeof *s)) || !(s[l->nskip] = strdup(name))) {
        if (s)
            l->skip = s;
        return fail(err, errlen, "out of memory");
    }
    l->skip = s;
    l->nskip++;
    return 0;
}

int blitzres_add(struct blitzres_list *l, const char *name, int32_t value, char *err,
                 size_t errlen)
{
    int32_t old;
    size_t i;

    if (!blitzres_valid_name(name))
        return fail(err, errlen, "\"%s\" is not a constant name", name);
    for (i = 0; i < l->nskip; i++)
        if (!strcmp(l->skip[i], name))
            return 1;
    if (!blitzres_find(l, name, &old)) {
        if (old != value)
            return fail(err, errlen, "%s defined as %ld and as %ld", name, (long)old,
                        (long)value);
        return 2;
    }
    if (l->n == l->cap) {
        size_t cap = l->cap ? 2 * l->cap : 64;
        struct blitzres_const *c = realloc(l->c, cap * sizeof *c);

        if (!c)
            return fail(err, errlen, "out of memory");
        l->c = c;
        l->cap = cap;
    }
    if (!(l->c[l->n].name = strdup(name)))
        return fail(err, errlen, "out of memory");
    l->c[l->n++].value = value;
    return 0;
}

void blitzres_free(struct blitzres_list *l)
{
    size_t i;

    for (i = 0; i < l->n; i++)
        free(l->c[i].name);
    for (i = 0; i < l->nskip; i++)
        free(l->skip[i]);
    free(l->c);
    free(l->skip);
    memset(l, 0, sizeof *l);
}

/* ------------------------------------------------------------ writing */

struct out {
    uint8_t *p;
    size_t n, cap;
    int bad;
};

static void put(struct out *o, const void *d, size_t n)
{
    if (o->bad)
        return;
    if (o->n + n > o->cap) {
        size_t cap = o->cap ? o->cap : 1024;
        uint8_t *p;

        while (cap < o->n + n)
            cap *= 2;
        if (!(p = realloc(o->p, cap))) {
            o->bad = 1;
            return;
        }
        o->p = p;
        o->cap = cap;
    }
    memcpy(o->p + o->n, d, n);
    o->n += n;
}

static void put8(struct out *o, uint8_t v)
{
    put(o, &v, 1);
}

static void put16(struct out *o, uint16_t v)
{
    put8(o, (uint8_t)(v >> 8));
    put8(o, (uint8_t)v);
}

static void put32(struct out *o, uint32_t v)
{
    put16(o, (uint16_t)(v >> 16));
    put16(o, (uint16_t)v);
}

static void set32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

/* the node size of a constant: 19 bytes, the name, its 0 */
static size_t const_len(const char *name)
{
    return 19 + strlen(name) + 1;
}

int blitzres_write(const struct blitzres_list *l, uint8_t **outp, size_t *len, char *err,
                   size_t errlen)
{
    struct out o = { 0 };
    size_t i, longs, off;

    *outp = NULL;
    *len = 0;
    if (l->n > 0xFFFF)
        return fail(err, errlen, "%zu constants: a resident holds at most 65535", l->n);
    for (i = 0; i < l->n; i++)
        if (!blitzres_valid_name(l->c[i].name))
            return fail(err, errlen, "\"%s\" is not a constant name", l->c[i].name);
    /* menu5: header (sizes patched below), restypes, resmacs, resconsts */
    put32(&o, HUNK_HEADER);
    put32(&o, 0);
    put32(&o, 1);
    put32(&o, 0);
    put32(&o, 0);
    put32(&o, 0);                       /* tstsize1 */
    put32(&o, HUNK_CODE);
    put32(&o, 0);                       /* tstsize2 */
    put16(&o, 0);                       /* no types */
    put16(&o, 0);                       /* no macros */
    put16(&o, (uint16_t)l->n);
    off = 6;                            /* offset in the hunk */
    for (i = 0; i < l->n; i++) {
        size_t n = (const_len(l->c[i].name) + 1) & ~(size_t)1;

        put32(&o, i + 1 < l->n ? (uint32_t)(off + n) : 0);     /* next */
        put32(&o, 1);                   /* refs: a constant */
        put32(&o, (uint32_t)l->c[i].value);
        put32(&o, 0);                   /* data */
        put16(&o, 0);                   /* procnum */
        put8(&o, (uint8_t)const_len(l->c[i].name));
        put(&o, l->c[i].name, strlen(l->c[i].name) + 1);
        if (n != const_len(l->c[i].name))
            put8(&o, 0);
        off += n;
    }
    while (off & 3) {
        put8(&o, 0);
        off++;
    }
    longs = off / 4;
    if (l->n > 1) {
        /* the offsets of the next pointers, the last one written first
           (addoff2 puts each in front of the list) */
        size_t k, *at = malloc(l->n * sizeof *at), pos = 6;

        if (!at) {
            free(o.p);
            return fail(err, errlen, "out of memory");
        }
        for (k = 0; k < l->n; k++) {
            at[k] = pos;
            pos += (const_len(l->c[k].name) + 1) & ~(size_t)1;
        }
        put32(&o, HUNK_RELOC32);
        put32(&o, (uint32_t)(l->n - 1));
        put32(&o, 0);
        for (k = l->n - 1; k-- > 0;)
            put32(&o, (uint32_t)at[k]);
        put32(&o, 0);
        free(at);
    }
    put32(&o, HUNK_END);
    if (o.bad) {
        free(o.p);
        return fail(err, errlen, "out of memory");
    }
    set32(o.p + 20, (uint32_t)longs);
    set32(o.p + 28, (uint32_t)longs);
    *outp = o.p;
    *len = o.n;
    return 0;
}

/* ------------------------------------------------------------ reading */

struct res {
    const struct hunk *h;
    char *err;
    size_t errlen;
};

static int in(const struct res *r, uint32_t off, uint32_t n)
{
    return off <= r->h->size && r->h->size - off >= n;
}

/* the name at off (0-terminated within the node of len bytes at node) */
static int name_at(const struct res *r, uint32_t node, uint32_t nameoff, uint32_t len,
                   const char **name)
{
    const uint8_t *d = r->h->data;

    if (len <= nameoff || !in(r, node, len) || !memchr(d + node + nameoff, 0, len - nameoff))
        return fail(r->err, r->errlen, "hunk+%u: a node of %u bytes without its name", node,
                    len);
    *name = (const char *)d + node + nameoff;
    return 0;
}

static uint32_t even(uint32_t n)
{
    return (n + 1) & ~1u;
}

static int open_res(const uint8_t *buf, size_t size, struct loadfile *lf, struct res *r,
                    char *err, size_t errlen)
{
    r->err = err;
    r->errlen = errlen;
    if (load_parse(buf, size, lf, err, errlen))
        return -1;
    if (lf->nhunks != 1 || lf->hunks[0].type != HUNK_CODE) {
        load_free(lf);
        return fail(err, errlen, "not a resident: %u hunks, expected one code hunk",
                    lf->nhunks);
    }
    r->h = &lf->hunks[0];
    return 0;
}

/* the types from off: written to out (may be NULL); *end = behind them */
static int read_types(const struct res *r, uint32_t off, FILE *out, uint32_t *end)
{
    const uint8_t *d = r->h->data;
    unsigned n, i;

    if (!in(r, off, 2))
        return fail(r->err, r->errlen, "no type count");
    n = be16(d + off);
    off += 2;
    for (i = 0; i < n; i++) {
        uint32_t f = off, guard = 0, len;
        const char *name;

        /* fixtypes: the fields up to the one without next, the type
           behind it */
        if (out)
            fprintf(out, "type");
        for (;;) {
            if (!in(r, f, 15) || ++guard > r->h->size)
                return fail(r->err, r->errlen, "hunk+%u: field out of bounds", f);
            len = d[f + 14];
            if (name_at(r, f, 15, len, &name))
                return -1;
            if (!be32(d + f))
                break;
            if (be32(d + f) <= f)
                return fail(r->err, r->errlen, "hunk+%u: field list goes back", f);
            f = be32(d + f);
        }
        f += even(len);
        if (!in(r, f, 11))
            return fail(r->err, r->errlen, "hunk+%u: type out of bounds", f);
        len = d[f + 10];
        if (name_at(r, f, 11, len, &name))
            return -1;
        if (out) {
            uint32_t g = be32(d + f + 4);
            const char *fn;

            fprintf(out, " %s size=%d\n", name, (int16_t)be16(d + f + 8));
            for (;;) {
                if (!in(r, g, 15) || name_at(r, g, 15, d[g + 14], &fn))
                    return fail(r->err, r->errlen, "type %s: bad field list", name);
                /* ndims only counts for an array field (flags bit 1) */
                fprintf(out, "  field %s offset=%d flags=%u", fn, (int16_t)be16(d + g + 4),
                        be16(d + g + 6));
                if (be16(d + g + 6) & 2)
                    fprintf(out, " ndims=%d", (int16_t)be16(d + g + 8));
                fprintf(out, " type=%u\n", be16(d + g + 10));
                if (!be32(d + g))
                    break;
                if (be32(d + g) <= g)
                    return fail(r->err, r->errlen, "type %s: field list goes back", name);
                g = be32(d + g);
            }
        }
        off = f + even(len);
    }
    *end = off;
    return 0;
}

static int read_macros(const struct res *r, uint32_t off, FILE *out, uint32_t *end)
{
    const uint8_t *d = r->h->data;
    unsigned n, i;

    if (!in(r, off, 2))
        return fail(r->err, r->errlen, "no macro count");
    n = be16(d + off);
    off += 2;
    for (i = 0; i < n; i++) {
        uint32_t len, tl, k;
        const char *name;

        if (!in(r, off, 13))
            return fail(r->err, r->errlen, "hunk+%u: macro out of bounds", off);
        len = d[off + 12];
        tl = be16(d + off + 8);
        if (name_at(r, off, 13, len, &name) || !in(r, off + even(len), tl))
            return fail(r->err, r->errlen, "hunk+%u: macro out of bounds", off);
        if (out) {
            fprintf(out, "macro %s \"", name);
            for (k = 0; k < tl; k++) {
                uint8_t c = d[off + even(len) + k];

                if (c < 32 || c > 126 || c == '"' || c == '\\')
                    fprintf(out, "\\x%02X", c);
                else
                    fputc(c, out);
            }
            fprintf(out, "\"\n");
        }
        off += even(len) + even(tl);
    }
    *end = off;
    return 0;
}

/* the constants from off: to out and/or l */
static int read_consts(const struct res *r, uint32_t off, FILE *out, struct blitzres_list *l,
                       int strict, uint32_t *end)
{
    const uint8_t *d = r->h->data;
    unsigned n, i;

    if (!in(r, off, 2))
        return fail(r->err, r->errlen, "no constant count");
    n = be16(d + off);
    off += 2;
    for (i = 0; i < n; i++) {
        uint32_t len, k;
        const char *name;

        if (!in(r, off, 19))
            return fail(r->err, r->errlen, "hunk+%u: constant out of bounds", off);
        len = d[off + 18];
        if (name_at(r, off, 19, len, &name))
            return -1;
        if (strict) {
            uint32_t next = i + 1 < n ? off + even(len) : 0;

            if (be32(d + off) != next || be32(d + off + 4) != 1 || be32(d + off + 12)
                || be16(d + off + 16) || len != const_len(name))
                return fail(r->err, r->errlen, "constant %s: node not as written", name);
            for (k = len; k < even(len); k++)
                if (d[off + k])
                    return fail(r->err, r->errlen, "constant %s: padding not 0", name);
        }
        if (out)
            fprintf(out, "const %s=%ld\n", name, (long)(int32_t)be32(d + off + 8));
        if (l) {
            char e[200];
            int rc = blitzres_add(l, name, (int32_t)be32(d + off + 8), e, sizeof e);

            if (rc < 0)
                return fail(r->err, r->errlen, "%s", e);
            if (rc == 2 && strict)
                return fail(r->err, r->errlen, "constant %s twice", name);
        }
        off += even(len);
    }
    *end = off;
    return 0;
}

int blitzres_dump(const uint8_t *buf, size_t size, FILE *out, char *err, size_t errlen)
{
    struct loadfile lf;
    struct res r;
    uint32_t off;
    int rc;

    if (open_res(buf, size, &lf, &r, err, errlen))
        return -1;
    rc = read_types(&r, 0, out, &off) || read_macros(&r, off, out, &off)
         || read_consts(&r, off, out, NULL, 0, &off) ? -1 : 0;
    load_free(&lf);
    return rc;
}

int blitzres_read_consts(const uint8_t *buf, size_t size, int strict, struct blitzres_list *l,
                         char *err, size_t errlen)
{
    struct loadfile lf;
    struct res r;
    uint32_t off;
    int rc = -1;

    if (open_res(buf, size, &lf, &r, err, errlen))
        return -1;
    if (!in(&r, 0, 4) || be16(r.h->data) || be16(r.h->data + 2)) {
        fail(err, errlen, "the resident has types or macros");
        goto out;
    }
    if (read_consts(&r, 4, NULL, l, strict, &off))
        goto out;
    if (strict) {
        /* the file is exactly what blitzres_write makes of the constants
           (padding, relocations, header) */
        struct blitzres_list again = { 0 };
        uint8_t *w = NULL;
        size_t wl;
        char e[200];
        int same = !read_consts(&r, 4, NULL, &again, 0, &off)
                   && !blitzres_write(&again, &w, &wl, e, sizeof e) && wl == size
                   && !memcmp(w, buf, size);

        free(w);
        blitzres_free(&again);
        if (!same) {
            fail(err, errlen, "the file is not as blitzres writes these constants");
            goto out;
        }
    }
    rc = 0;
out:
    load_free(&lf);
    return rc;
}

/* ------------------------------------------------------------ FD files */

/* the next line of text (without the line end, trimmed): 0 at the end */
static int next_line(const char **p, const char *end, char *line, size_t cap)
{
    const char *s = *p, *e;
    size_t n;

    if (s >= end)
        return 0;
    e = memchr(s, '\n', (size_t)(end - s));
    if (!e)
        e = end;
    *p = e < end ? e + 1 : e;
    while (s < e && isspace((unsigned char)*s))
        s++;
    while (e > s && isspace((unsigned char)e[-1]))
        e--;
    n = (size_t)(e - s);
    if (n >= cap)
        n = cap - 1;
    memcpy(line, s, n);
    line[n] = 0;
    return 1;
}

static int is_word(int c)
{
    return isalnum(c) || c == '_';
}

int blitzres_fd(const char *text, size_t len, int varargs, struct blitzres_list *l, char *err,
                size_t errlen)
{
    const char *p = text, *end = text + len;
    char line[512];
    long bias = 0;
    int have_bias = 0, pub = 1;
    struct blitzres_list fns = { 0 };   /* this file's functions */
    size_t i;
    int rc = -1;

    while (next_line(&p, end, line, sizeof line)) {
        size_t n = 0;

        if (!strncmp(line, "##bias", 6)) {
            char *e;

            bias = strtol(line + 6, &e, 10);
            if (e == line + 6) {
                fail(err, errlen, "\"%s\": no number", line);
                goto out;
            }
            have_bias = 1;
            continue;
        }
        if (!strcmp(line, "##private")) {
            pub = 0;
            continue;
        }
        if (!strcmp(line, "##public")) {
            pub = 1;
            continue;
        }
        if (line[0] == '#' && line[1] == '#')
            continue;
        if (!line[0] || line[0] == '*')
            continue;
        while (is_word((unsigned char)line[n]))
            n++;
        if (!n || line[n] != '(')
            continue;
        if (!have_bias) {
            fail(err, errlen, "%.*s before ##bias", (int)n, line);
            goto out;
        }
        if (pub) {
            char name[BLITZRES_MAXNAME + 2];

            if (n > BLITZRES_MAXNAME - 1) {
                fail(err, errlen, "%.*s: name too long", (int)n, line);
                goto out;
            }
            name[0] = '_';
            memcpy(name + 1, line, n);
            name[n + 1] = 0;
            if (blitzres_add(&fns, name, (int32_t)-bias, err, errlen) < 0
                || blitzres_add(l, name, (int32_t)-bias, err, errlen) < 0)
                goto out;
        }
        bias += 6;
    }
    if (varargs)
        for (i = 0; i < fns.n; i++) {
            const char *nm = fns.c[i].name;
            size_t k = strlen(nm);
            char shortname[BLITZRES_MAXNAME + 2];
            int32_t v;

            if (k < 3 || nm[k - 1] != 'A')
                continue;
            memcpy(shortname, nm, k - 1);
            shortname[k - 1] = 0;
            if (!blitzres_find(&fns, shortname, &v))
                continue;
            if (blitzres_add(l, shortname, fns.c[i].value, err, errlen) < 0)
                goto out;
        }
    rc = 0;
out:
    blitzres_free(&fns);
    return rc;
}

/* ------------------------------------------------------- EQU includes */

/* expressions: | ^ & << >> + - * ~ - () over numbers and symbols, with
   C's (and Python's) precedence; 64 bit, the result cut to 32 */
struct expr {
    const char *p;
    const struct blitzres_list *syms;
    int bad;
};

static void skipws(struct expr *x)
{
    while (*x->p == ' ' || *x->p == '\t')
        x->p++;
}

static int64_t e_or(struct expr *x);

static int64_t e_primary(struct expr *x)
{
    int64_t v = 0;

    skipws(x);
    if (*x->p == '(') {
        x->p++;
        v = e_or(x);
        skipws(x);
        if (*x->p != ')')
            x->bad = 1;
        else
            x->p++;
        return v;
    }
    if (*x->p == '-') {
        x->p++;
        return -e_primary(x);
    }
    if (*x->p == '~') {
        x->p++;
        return ~e_primary(x);
    }
    if (*x->p == '+') {
        x->p++;
        return e_primary(x);
    }
    if (*x->p == '\'') {
        /* 'ABCD': four characters, big endian */
        int k;

        for (k = 1; k <= 4; k++)
            if (!x->p[k] || x->p[k] == '\'')
                break;
        if (k <= 4 || x->p[5] != '\'') {
            x->bad = 1;
            return 0;
        }
        for (k = 1; k <= 4; k++)
            v = v << 8 | (uint8_t)x->p[k];
        x->p += 6;
        return v;
    }
    if (*x->p == '$' || (x->p[0] == '0' && (x->p[1] == 'x' || x->p[1] == 'X'))) {
        const char *s = x->p + (*x->p == '$' ? 1 : 2);
        int digits = 0;

        while (isxdigit((unsigned char)*s)) {
            v = v * 16 + (isdigit((unsigned char)*s) ? *s - '0' : (tolower(*s) - 'a' + 10));
            s++;
            digits++;
        }
        if (!digits || is_word((unsigned char)*s) || digits > 8)
            x->bad = 1;
        x->p = s;
        return v;
    }
    if (isdigit((unsigned char)*x->p)) {
        const char *s = x->p;

        /* Python: a decimal with a leading 0 is only 0, 00 ... */
        while (isdigit((unsigned char)*s)) {
            v = v * 10 + (*s - '0');
            if (v > 0xFFFFFFFFll)
                x->bad = 1;
            s++;
        }
        if (is_word((unsigned char)*s) || (x->p[0] == '0' && v))
            x->bad = 1;
        x->p = s;
        return v;
    }
    if (isalpha((unsigned char)*x->p) || *x->p == '_') {
        char name[BLITZRES_MAXNAME + 1];
        size_t n = 0;
        int32_t sv = 0;

        while (is_word((unsigned char)x->p[n]))
            n++;
        if (n > BLITZRES_MAXNAME) {
            x->bad = 1;
            return 0;
        }
        memcpy(name, x->p, n);
        name[n] = 0;
        x->p += n;
        if (blitzres_find(x->syms, name, &sv))
            x->bad = 1;
        return sv;
    }
    x->bad = 1;
    return 0;
}

static int64_t e_mul(struct expr *x)
{
    int64_t v = e_primary(x);

    for (;;) {
        skipws(x);
        if (*x->p != '*')
            return v;
        x->p++;
        v *= e_primary(x);
        v = (int64_t)(int32_t)(uint32_t)v;
    }
}

static int64_t e_add(struct expr *x)
{
    int64_t v = e_mul(x);

    for (;;) {
        skipws(x);
        if (*x->p == '+') {
            x->p++;
            v += e_mul(x);
        } else if (*x->p == '-') {
            x->p++;
            v -= e_mul(x);
        } else
            return v;
    }
}

static int64_t e_shift(struct expr *x)
{
    int64_t v = e_add(x);

    for (;;) {
        int64_t s;

        skipws(x);
        if (x->p[0] == '<' && x->p[1] == '<') {
            x->p += 2;
            s = e_add(x);
            if (s < 0 || s > 31)
                x->bad = 1;
            else
                v = (int64_t)((uint64_t)v << s);
        } else if (x->p[0] == '>' && x->p[1] == '>') {
            x->p += 2;
            s = e_add(x);
            if (s < 0 || s > 31)
                x->bad = 1;
            else
                v >>= s;
        } else
            return v;
    }
}

static int64_t e_and(struct expr *x)
{
    int64_t v = e_shift(x);

    for (;;) {
        skipws(x);
        if (*x->p != '&')
            return v;
        x->p++;
        v &= e_shift(x);
    }
}

static int64_t e_xor(struct expr *x)
{
    int64_t v = e_and(x);

    for (;;) {
        skipws(x);
        if (*x->p != '^')
            return v;
        x->p++;
        v ^= e_and(x);
    }
}

static int64_t e_or(struct expr *x)
{
    int64_t v = e_xor(x);

    for (;;) {
        skipws(x);
        if (*x->p != '|')
            return v;
        x->p++;
        v |= e_xor(x);
    }
}

int blitzres_equ(const char *text, size_t len, struct blitzres_list *syms,
                 struct blitzres_list *l, char *err, size_t errlen)
{
    const char *p = text, *end = text + len;

    while (p < end) {
        const char *s = p, *e = memchr(p, '\n', (size_t)(end - p));
        char line[512], name[BLITZRES_MAXNAME + 1], *v, *c;
        size_t n = 0, ll;
        struct expr x;
        int64_t val;

        if (!e)
            e = end;
        p = e < end ? e + 1 : e;
        ll = (size_t)(e - s);
        if (ll >= sizeof line)
            continue;
        memcpy(line, s, ll);
        line[ll] = 0;
        /* NAME, white space, EQU|equ|SET, white space, the expression */
        while (is_word((unsigned char)line[n]))
            n++;
        if (!n || (line[n] != ' ' && line[n] != '\t') || n > BLITZRES_MAXNAME)
            continue;
        memcpy(name, line, n);
        name[n] = 0;
        v = line + n;
        while (*v == ' ' || *v == '\t')
            v++;
        if (strncmp(v, "EQU", 3) && strncmp(v, "equ", 3) && strncmp(v, "SET", 3))
            continue;
        v += 3;
        if (*v != ' ' && *v != '\t')
            continue;
        while (*v == ' ' || *v == '\t')
            v++;
        if ((c = strchr(v, ';')))
            *c = 0;
        for (c = v + strlen(v); c > v && isspace((unsigned char)c[-1]);)
            *--c = 0;
        if (!*v || (n > 2 && !strcmp(name + n - 2, "_I")) || !blitzres_valid_name(name))
            continue;
        x.p = v;
        x.syms = syms;
        x.bad = 0;
        val = e_or(&x);
        skipws(&x);
        if (x.bad || *x.p)
            continue;
        val = (int64_t)(int32_t)(uint32_t)val;
        {
            char e2[200];
            size_t i;

            /* a symbol defined again takes the new value (as the
               assembler's SET) */
            for (i = 0; i < syms->n && strcmp(syms->c[i].name, name); i++)
                ;
            if (i < syms->n)
                syms->c[i].value = (int32_t)val;
            else if (blitzres_add(syms, name, (int32_t)val, e2, sizeof e2) < 0)
                return fail(err, errlen, "%s", e2);
        }
        if (blitzres_add(l, name, (int32_t)val, err, errlen) < 0)
            return -1;
    }
    return 0;
}
