/*
 * tedfont.c - TED-C's 256-glyph screen fonts (see tedfont.h).
 */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "tedfont.h"

static int fail(char *err, size_t errlen, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(err, errlen, fmt, ap);
    va_end(ap);
    return -1;
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

/* the line p..e without trailing blanks (and a CR) */
static const char *trim(const char *p, const char *e)
{
    while (e > p && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\r'))
        e--;
    return e;
}

int tedfont_parse(const char *text, size_t len, const char *name, struct tedfont_src *src,
                  char *err, size_t errlen)
{
    const char *p = text, *end = text + len;
    int lineno = 0, code = -1, rows = 0, codeline = 0;
    unsigned c;

    memset(src, 0, sizeof *src);
    while (p < end) {
        const char *nl = memchr(p, '\n', (size_t)(end - p));
        const char *e = nl ? nl : end, *t = trim(p, e);
        size_t n = (size_t)(t - p);

        lineno++;
        if (n == 0 || *p == ';')
            goto next;
        if (!src->height) {
            if (n == 3 && p[0] == 'h' && p[1] == '=' && (p[2] == '8' || p[2] == '6')) {
                src->height = p[2] - '0';
                goto next;
            }
            return fail(err, errlen, "%s:%d: expected h=8 or h=6 before the glyphs", name, lineno);
        }
        if (*p == '@') {
            int hi, lo;

            if (code >= 0 && rows < src->height)
                return fail(err, errlen, "%s:%d: @%02X has %d rows, %d expected", name, codeline,
                            code, rows, src->height);
            if (n < 3 || (hi = hexdigit(p[1])) < 0 || (lo = hexdigit(p[2])) < 0
                || (n > 3 && p[3] != ' ' && p[3] != '\t'))
                return fail(err, errlen, "%s:%d: expected @XX, a code of two hexadecimal digits",
                            name, lineno);
            if (n > 3) {
                const char *q = p + 3;

                while (q < t && (*q == ' ' || *q == '\t'))
                    q++;
                if (q < t && *q != ';')
                    return fail(err, errlen, "%s:%d: text after @%c%c (a comment starts with ';')",
                                name, lineno, p[1], p[2]);
            }
            code = hi * 16 + lo;
            if (code < 0x80 && !(code == TEDFONT_REDRAWN && src->height == 6))
                return fail(err, errlen,
                            "%s:%d: @%02X: glyph $%02X is TED's own, only $80-$FF%s may be given",
                            name, lineno, code, code, src->height == 6 ? " and $1A" : "");
            if (src->line[code])
                return fail(err, errlen, "%s:%d: @%02X given twice (first at line %d)", name,
                            lineno, code, src->line[code]);
            src->line[code] = lineno;
            codeline = lineno;
            rows = 0;
            goto next;
        }
        if (code < 0)
            return fail(err, errlen, "%s:%d: a row before the first @XX", name, lineno);
        if (rows >= src->height)
            return fail(err, errlen, "%s:%d: a row too many: @%02X has %d rows", name, lineno,
                        code, src->height);
        if (n != 8)
            return fail(err, errlen, "%s:%d: a row of %zu characters, 8 expected", name, lineno, n);
        {
            uint8_t b = 0;
            unsigned i;

            for (i = 0; i < 8; i++) {
                if (p[i] != '#' && p[i] != '.')
                    return fail(err, errlen, "%s:%d: '%c' in a row, only '#' and '.'", name,
                                lineno, p[i]);
                b = (uint8_t)(b << 1 | (p[i] == '#'));
            }
            src->glyph[code][rows++] = b;
        }
next:
        p = nl ? nl + 1 : end;
    }
    if (!src->height)
        return fail(err, errlen, "%s: no line h=8 or h=6", name);
    if (code >= 0 && rows < src->height)
        return fail(err, errlen, "%s:%d: @%02X has %d rows, %d expected", name, codeline, code,
                    rows, src->height);
    for (c = 0x80; c < 0x100; c++)
        if (!src->line[c])
            return fail(err, errlen, "%s: no block @%02X (every code $80-$FF is needed)", name, c);
    return 0;
}

static int archive_ok(const struct tedfont_src *src, size_t archlen, char *err, size_t errlen)
{
    if (archlen != tedfont_size(src->height) / 2)
        return fail(err, errlen, "the archive font has %zu bytes, %zu expected for h=%d", archlen,
                    tedfont_size(src->height) / 2, src->height);
    return 0;
}

/* glyph c of the font: from the source or the archive file */
static int from_source(const struct tedfont_src *src, unsigned c)
{
    return c >= 0x80 || (c == TEDFONT_REDRAWN && src->line[c]);
}

int tedfont_build(const struct tedfont_src *src, const uint8_t *archive, size_t archlen,
                  uint8_t *out, char *err, size_t errlen)
{
    int h = src->height;
    unsigned c, r;

    if (archive_ok(src, archlen, err, errlen))
        return -1;
    memset(out, 0, tedfont_size(h));
    memcpy(out, archive, archlen);              /* strips 0-7 */
    for (c = 0; c < 256; c++)
        if (from_source(src, c))
            for (r = 0; r < (unsigned)h; r++)
                out[tedfont_at(h, c, r)] = src->glyph[c][r];
    return 0;
}

int tedfont_check(const struct tedfont_src *src, const uint8_t *archive, size_t archlen,
                  const uint8_t *font, size_t fontlen, char *err, size_t errlen)
{
    int h = src->height;
    unsigned c, r;

    if (archive_ok(src, archlen, err, errlen))
        return -1;
    if (fontlen != tedfont_size(h))
        return fail(err, errlen, "the font has %zu bytes, %zu expected for h=%d", fontlen,
                    tedfont_size(h), h);
    for (c = 0; c < 256; c++)
        for (r = 0; r < (unsigned)h; r++) {
            size_t at = tedfont_at(h, c, r);
            int own = from_source(src, c);
            uint8_t want = own ? src->glyph[c][r] : archive[at];

            if (font[at] != want)
                return fail(err, errlen, "glyph $%02X row %u is $%02X, %s has $%02X", c, r,
                            font[at], own ? "the source" : "the archive font", want);
        }
    return 0;
}
