/*
 * teddump.c - screen dumps of teddrive, see teddump.h.
 */
#define _XOPEN_SOURCE 700       /* strndup */
#include <stdlib.h>
#include <string.h>
#include "sha256.h"
#include "teddump.h"
#include "util.h"

#define MAGIC "TEDSCR1"

void teddump_free(struct teddump *d)
{
    size_t i;

    for (i = 0; i < d->nwindows; i++)
        free(d->windows[i].title);
    free(d->windows);
    for (i = 0; d->planes && i < (size_t)d->depth; i++)
        free(d->planes[i]);
    free(d->planes);
    free(d->title);
    memset(d, 0, sizeof *d);
}

static int fail(struct teddump *d, char *err, size_t errlen, const char *msg)
{
    snprintf(err, errlen, "%s", msg);
    teddump_free(d);
    return -1;
}

/* a whole field as a number: decimal or $hex; 0 = no number */
static int number(const char *s, size_t n, long *v)
{
    char buf[32], *e;

    while (n && *s == ' ')
        s++, n--;
    while (n && s[n - 1] == ' ')
        n--;
    if (!n || n >= sizeof buf)
        return 0;
    memcpy(buf, s, n);
    buf[n] = 0;
    *v = buf[0] == '$' ? strtol(buf + 1, &e, 16) : strtol(buf, &e, 10);
    return *e == 0 && e != buf + (buf[0] == '$');
}

int teddump_parse(const uint8_t *data, size_t len, struct teddump *d, char *err, size_t errlen)
{
    const char *p = (const char *)data, *end = p + len;
    long *fields[] = { &d->width, &d->height, &d->depth, &d->bytesperrow, &d->rows,
                       &d->rowbytes, &d->interleaved, &d->modeid, &d->mousex, &d->mousey,
                       &d->memtype };
    static const char *const names[] = { "width", "height", "depth", "bytesperrow", "rows",
                                         "rowbytes", "interleaved", "modeid", "mousex",
                                         "mousey", "memtype" };
    unsigned have = 0;                  /* bits: width height depth rows rowbytes */
    int magic = 0;
    size_t size, k;
    long i;

    memset(d, 0, sizeof *d);
    for (;;) {
        const char *eol = memchr(p, '\n', (size_t)(end - p)), *sp, *v;
        size_t n, kl;

        if (!eol)
            return fail(d, err, errlen, "no \"data\" line");
        n = (size_t)(eol - p);
        if (n == 4 && !memcmp(p, "data", 4)) {
            p = eol + 1;
            break;
        }
        sp = memchr(p, ' ', n);
        kl = sp ? (size_t)(sp - p) : n;
        v = sp ? sp + 1 : eol;
        if (kl == 6 && !memcmp(p, "window", 6)) {
            /* L T W H FLAGS TITLE (the title may contain blanks) */
            struct teddump_window w = { 0 }, *nw;
            long *wf[] = { &w.left, &w.top, &w.width, &w.height, &w.flags };
            const char *q = v;

            for (k = 0; k < 5; k++) {
                const char *b = memchr(q, ' ', (size_t)(eol - q));
                const char *fe = b ? b : eol;

                number(q, (size_t)(fe - q), wf[k]);
                q = b ? b + 1 : eol;
            }
            if (!(w.title = strndup(q, (size_t)(eol - q)))
                || !(nw = realloc(d->windows, (d->nwindows + 1) * sizeof *nw))) {
                free(w.title);
                return fail(d, err, errlen, "out of memory");
            }
            d->windows = nw;
            d->windows[d->nwindows++] = w;
        } else if (kl == 5 && !memcmp(p, "title", 5)) {
            free(d->title);
            if (!(d->title = strndup(v, (size_t)(eol - v))))
                return fail(d, err, errlen, "out of memory");
        } else if (kl == 5 && !memcmp(p, "magic", 5)) {
            magic = (size_t)(eol - v) == strlen(MAGIC) && !memcmp(v, MAGIC, strlen(MAGIC));
        } else {
            for (k = 0; k < sizeof names / sizeof names[0]; k++)
                if (strlen(names[k]) == kl && !memcmp(p, names[k], kl)) {
                    if (number(v, (size_t)(eol - v), fields[k]))
                        have |= 1u << k;
                    break;
                }
        }
        p = eol + 1;
    }
    if (!magic)
        return fail(d, err, errlen, "bad magic");
    if ((have & 0x37) != 0x37)          /* width height depth rows rowbytes */
        return fail(d, err, errlen, "header field width/height/depth/rows/rowbytes missing");
    if (d->depth < 1 || d->depth > 8 || d->rows < 1 || d->rowbytes < 1)
        return fail(d, err, errlen, "bad geometry");
    size = (size_t)d->rows * (size_t)d->rowbytes;
    if ((size_t)(end - p) != (size_t)d->depth * size)
        return fail(d, err, errlen, "data size does not match depth x rows x rowbytes");
    if (!d->title && !(d->title = strdup("")))
        return fail(d, err, errlen, "out of memory");
    if (!(d->planes = calloc((size_t)d->depth, sizeof *d->planes)))
        return fail(d, err, errlen, "out of memory");
    for (i = 0; i < d->depth; i++) {
        if (!(d->planes[i] = malloc(size)))
            return fail(d, err, errlen, "out of memory");
        memcpy(d->planes[i], p + (size_t)i * size, size);
    }
    return 0;
}

int teddump_load(const char *path, struct teddump *d, char *err, size_t errlen)
{
    size_t len;
    uint8_t *data = read_file(path, &len);
    int r;

    if (!data) {
        memset(d, 0, sizeof *d);
        snprintf(err, errlen, "cannot read %s", path);
        return -1;
    }
    r = teddump_parse(data, len, d, err, errlen);
    free(data);
    return r;
}

int teddump_load_fonts(const char *dir, struct teddump_fonts *f)
{
    char *p8 = str_printf("%s/ted_font8.b", dir), *p6 = str_printf("%s/ted_font6.b", dir);
    size_t l8 = 0, l6 = 0;
    uint8_t *d8 = p8 ? read_file(p8, &l8) : NULL, *d6 = p6 ? read_file(p6, &l6) : NULL;
    int r = d8 && d6 && l8 == sizeof f->f8 && l6 == sizeof f->f6 ? 0 : -1;

    if (!r) {
        memcpy(f->f8, d8, sizeof f->f8);
        memcpy(f->f6, d6, sizeof f->f6);
    }
    free(d8);
    free(d6);
    free(p8);
    free(p6);
    return r;
}

unsigned teddump_pixel(const struct teddump *d, long x, long y)
{
    unsigned c = 0, bit = 7 - (unsigned)(x & 7);
    long p;

    for (p = 0; p < d->depth; p++)
        c |= ((d->planes[p][y * d->rowbytes + (x >> 3)] >> bit) & 1u) << p;
    return c;
}

void teddump_digest(const struct teddump *d, char hex[65])
{
    struct sha256 s;
    long p;

    sha256_init(&s);
    for (p = 0; p < d->depth; p++)
        sha256_update(&s, d->planes[p], (size_t)(d->rows * d->rowbytes));
    sha256_hex(&s, hex);
}

void teddump_geometry(const struct teddump *d, int fh, struct teddump_geometry *g)
{
    long free_rows = d->height - TEDDUMP_TOPSKIP - TEDDUMP_BOTSKIP - 1 - fh;

    /* Python's // rounds down, also for negative numbers */
    g->edrows = free_rows >= 0 ? free_rows / fh : -((-free_rows + fh - 1) / fh);
    g->cols = d->rowbytes;
    g->top = TEDDUMP_TOPSKIP;
    g->status = TEDDUMP_TOPSKIP + g->edrows * fh + TEDDUMP_BOTSKIP + 1;
}

/* ------------------------------------------------------------ decoding */

static const uint8_t *font_of(const struct teddump_fonts *f, int fh)
{
    return fh == 6 ? f->f6 : f->f8;
}

/* rows of character c: fontcalc + r * 16 */
static void glyph(const uint8_t *font, int fh, int c, uint8_t *g)
{
    int base = (c & 15) + (c >> 4) * 16 * fh, r;

    for (r = 0; r < fh && r < 8; r++)      /* fh is 6 or 8 */
        g[r] = font[base + r * 16];
}

/* the table of print4 (and qprint) cells; a later entry wins: control
   codes first, then the box of $80-$9F and the Latin-1 characters from
   the top, the umlauts 26-31 (the same glyphs as $C4 $D6 $DC $E4 $F6
   $FC), then the printable ones from the top, so that e.g. the blank
   glyph decodes as a space */
struct entry {
    uint8_t p0[8], p1[8];
    int c;
    char attr;
};

struct decoder {
    int fh;
    struct entry cells[TEDDUMP_GLYPHS * 6];
    int ncells;
    uint8_t raw[TEDDUMP_GLYPHS][8];
    int rawc[TEDDUMP_GLYPHS];
    int nraw;
};

static void decoder_init(struct decoder *dec, const struct teddump_fonts *f, int fh)
{
    static const char normal[] = "0123", inverse[] = " abc";
    const uint8_t *font = font_of(f, fh);
    int order[TEDDUMP_GLYPHS], n = 0, i, c;

    for (c = 0; c < 26; c++)
        order[n++] = c;
    order[n++] = 127;
    order[n++] = 0x80;                  /* the box of $80-$9F: one entry */
    for (c = 0xFF; c >= 0xA0; c--)
        order[n++] = c;
    for (c = 26; c < 32; c++)
        order[n++] = c;
    for (c = 126; c >= 32; c--)
        order[n++] = c;
    dec->fh = fh;
    dec->ncells = 0;
    dec->nraw = n;
    for (i = 0; i < n; i++) {
        static const int colours[] = { 3, 2, 1 };
        uint8_t g[8];
        int k, inv, r;

        c = order[i];
        glyph(font, fh, c, g);
        memcpy(dec->raw[i], g, (size_t)fh);
        dec->rawc[i] = c;
        for (k = 0; k < 3; k++)
            for (inv = 0; inv < 2; inv++) {
                struct entry *e = &dec->cells[dec->ncells++];
                int colour = colours[k];

                for (r = 0; r < fh && r < 8; r++) {
                    e->p0[r] = (uint8_t)(((colour & 1) ? g[r] : 0) ^ (inv ? 0xFF : 0));
                    e->p1[r] = (uint8_t)(((colour & 2) ? g[r] : 0) ^ (inv ? 0xFF : 0));
                }
                e->c = c;
                e->attr = (inv ? inverse : normal)[colour];
            }
    }
}

/* the umlauts of codes 26-31 (TED's germs table) */
static const uint8_t umlauts[6] = { 0xE4, 0xF6, 0xFC, 0xC4, 0xD6, 0xDC };

static void put_char(struct text *t, int c)
{
    if (c < 0)
        text_add(t, "\xc2\xb7");        /* unknown: middle dot */
    else if (c >= 26 && c < 32)
        c = umlauts[c - 26];
    else if (c >= 0x80 && c < 0xA0)
        text_add(t, "\xe2\x96\xa1");    /* $80-$9F: the box, a white square */
    if (c >= 0xA0)                      /* Latin-1 as UTF-8 */
        text_add(t, "%c%c", 0xC0 | (c >> 6), 0x80 | (c & 0x3F));
    else if (c >= 0 && c < 0x80)
        text_add(t, "%c", c >= 32 && c < 127 ? c : '~');
}

/* one editor cell: character (-1 unknown, 32 blank) and attribute */
static void cell(const struct decoder *dec, const struct teddump *d, long y, long x, int *c,
                 char *attr)
{
    uint8_t p0[8], p1[8];
    int r, zero = 1, ones = 1, i;

    for (r = 0; r < dec->fh; r++) {
        p0[r] = d->planes[0][(y + r) * d->rowbytes + x];
        p1[r] = d->planes[1][(y + r) * d->rowbytes + x];
        zero &= !p0[r] && !p1[r];
        ones &= p0[r] == 0xFF && p1[r] == 0xFF;
    }
    if (zero) {
        *c = ' ';
        *attr = ' ';
        return;
    }
    if (ones) {
        *c = ' ';
        *attr = '#';
        return;
    }
    for (i = dec->ncells - 1; i >= 0; i--)
        if (!memcmp(dec->cells[i].p0, p0, (size_t)dec->fh)
            && !memcmp(dec->cells[i].p1, p1, (size_t)dec->fh)) {
            *c = dec->cells[i].c;
            *attr = dec->cells[i].attr;
            return;
        }
    *c = -1;
    *attr = '?';
}

/* a status line cell (qprint: raw glyph in one plane) */
static int rawcell(const struct decoder *dec, const struct teddump *d, int plane, long y, long x)
{
    uint8_t rows[8];
    int r, i;

    for (r = 0; r < dec->fh; r++)
        rows[r] = d->planes[plane][(y + r) * d->rowbytes + x];
    for (i = dec->nraw - 1; i >= 0; i--)
        if (!memcmp(dec->raw[i], rows, (size_t)dec->fh))
            return dec->rawc[i];
    return -1;
}

/* text without trailing blanks: chars / attrs of one row */
static void rstrip(struct text *t, size_t from)
{
    while (t->n > from && t->p[t->n - 1] == ' ')
        t->p[--t->n] = 0;
}

char *teddump_text(const struct teddump *d, const struct teddump_fonts *f, int fh)
{
    struct decoder *dec = malloc(sizeof *dec);
    struct teddump_geometry g;
    struct text t = { 0 };
    long row, x;
    size_t i;

    if (!dec || d->depth < 2) {
        free(dec);
        return NULL;
    }
    decoder_init(dec, f, fh);
    teddump_geometry(d, fh, &g);
    text_add(&t, "# %ldx%ldx%ld mode $%08lX title \"%s\" edrows %ld font %d\n", d->width,
             d->height, d->depth, (unsigned long)d->modeid & 0xFFFFFFFFul, d->title, g.edrows, fh);
    for (i = 0; i < d->nwindows; i++) {
        const struct teddump_window *w = &d->windows[i];

        text_add(&t, "# window %ld,%ld %ldx%ld flags $%08lX \"%s\"\n", w->left, w->top, w->width,
                 w->height, (unsigned long)w->flags & 0xFFFFFFFFul, w->title);
    }
    for (row = 0; row < g.edrows; row++) {
        struct text a = { 0 };
        long y = g.top + row * fh;
        size_t start;
        int plain = 1;

        text_add(&t, "%3ld |", row);
        start = t.n;
        text_add(&a, "%s", "");
        for (x = 0; x < g.cols && y + fh <= d->rows; x++) {
            int c;
            char attr;

            cell(dec, d, y, x, &c, &attr);
            put_char(&t, c);
            text_add(&a, "%c", attr);
        }
        rstrip(&t, start);
        rstrip(&a, 0);
        text_add(&t, "\n");
        for (i = 0; i < a.n; i++)
            plain &= a.p[i] == ' ' || a.p[i] == '1';
        if (!plain)
            text_add(&t, "    @%s\n", a.p);
        free(a.p);
    }
    text_add(&t, "status |");
    if (g.status + fh <= d->rows) {
        size_t start = t.n;

        for (x = 0; x < g.cols; x++)
            put_char(&t, rawcell(dec, d, 1, g.status, x));
        rstrip(&t, start);
    }
    text_add(&t, "\n");
    free(dec);
    if (t.bad) {
        free(t.p);
        return NULL;
    }
    return t.p;
}

/* ----------------------------------------------------------- comparing */

/* the planes with the status field "Free" cleared (malloc'd copy) */
static uint8_t *masked(const struct teddump *d, int fh, int no_mask)
{
    size_t size = (size_t)(d->rows * d->rowbytes);
    uint8_t *m = malloc(size * (size_t)d->depth);
    struct teddump_geometry g;
    long p, r;

    if (!m)
        return NULL;
    for (p = 0; p < d->depth; p++)
        memcpy(m + (size_t)p * size, d->planes[p], size);
    teddump_geometry(d, fh, &g);
    if (!no_mask && g.status >= 0 && g.status + fh <= d->rows
        && d->rowbytes >= TEDDUMP_FREE_COL + TEDDUMP_FREE_LEN)
        for (p = 0; p < d->depth; p++)
            for (r = 0; r < fh; r++)
                memset(m + (size_t)p * size + (size_t)((g.status + r) * d->rowbytes)
                       + TEDDUMP_FREE_COL, 0, TEDDUMP_FREE_LEN);
    return m;
}

/* the lines of a text, 0-terminated in place (*n lines) */
static char **lines_of(char *t, size_t *n)
{
    char **v = NULL, *s;
    size_t k = 0;

    for (s = t; s && *s; ) {
        char *e = strchr(s, '\n');
        char **nv = realloc(v, (k + 1) * sizeof *v);

        if (!nv)
            break;
        v = nv;
        v[k++] = s;
        if (!e)
            break;
        *e = 0;
        s = e + 1;
    }
    *n = k;
    return v;
}

int teddump_compare(const struct teddump *a, const struct teddump *b,
                    const struct teddump_fonts *f, int fh, int no_mask, FILE *out)
{
    size_t size, i, diff = 0;
    uint8_t *ma, *mb;

    if (a->width != b->width || a->height != b->height || a->depth != b->depth
        || a->rows != b->rows || a->rowbytes != b->rowbytes) {
        fprintf(out, "geometry %ldx%ldx%ld (%ld rows of %ld) != %ldx%ldx%ld (%ld rows of %ld)"
                     " (not comparable)\n", a->width, a->height, a->depth, a->rows,
                a->rowbytes, b->width, b->height, b->depth, b->rows, b->rowbytes);
        return 1;
    }
    size = (size_t)(a->rows * a->rowbytes) * (size_t)a->depth;
    ma = masked(a, fh, no_mask);
    mb = masked(b, fh, no_mask);
    if (!ma || !mb) {
        free(ma);
        free(mb);
        fprintf(out, "out of memory\n");
        return 1;
    }
    for (i = 0; i < size; i++)
        diff += ma[i] != mb[i];
    free(ma);
    free(mb);
    if (!diff)
        return 0;
    fprintf(out, "%zu bytes differ\n", diff);
    if (f && a->depth >= 2) {
        char *ta = teddump_text(a, f, fh), *tb = teddump_text(b, f, fh);
        size_t na = 0, nb = 0;
        char **la = ta ? lines_of(ta, &na) : NULL, **lb = tb ? lines_of(tb, &nb) : NULL;

        /* the same geometry: the same lines, row by row */
        for (i = 0; i < na || i < nb; i++) {
            const char *x = i < na ? la[i] : "", *y = i < nb ? lb[i] : "";

            if (strcmp(x, y))
                fprintf(out, "-%s\n+%s\n", x, y);
        }
        free(la);
        free(lb);
        free(ta);
        free(tb);
    }
    return 1;
}
