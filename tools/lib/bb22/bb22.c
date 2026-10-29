/*
 * bb22.c - reading the "BB22" source format, writing TED's format
 * (bb22.h).
 */
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "bb22.h"

static unsigned word(const uint8_t *p)
{
    return (unsigned)p[0] << 8 | p[1];
}

static int fail(struct bb22 *b, char *err, size_t errlen, const char *fmt, ...)
{
    va_list ap;

    bb22_free(b);
    if (errlen) {
        va_start(ap, fmt);
        vsnprintf(err, errlen, fmt, ap);
        va_end(ap);
    }
    return -1;
}

bool bb22_is(const uint8_t *buf, size_t size)
{
    return size >= 4 && !memcmp(buf, "BB22", 4);
}

/* the header: version, its size and the prefix size, the cursor */
static int header(const uint8_t *buf, size_t size, struct bb22 *b, char *err, size_t errlen)
{
    if (!bb22_is(buf, size))
        return fail(b, err, errlen, "offset 0: no \"BB22\", not a BB22 file");
    if (size < 6)
        return fail(b, err, errlen, "offset 4: header cut off at the end of the file");
    b->version = word(buf + 4);
    switch (b->version) {
    case 8:
        b->header_size = 14;
        b->prefix_size = 1;
        break;
    case 10:
        b->header_size = 16;
        b->prefix_size = 2;
        break;
    default:
        return fail(b, err, errlen, "offset 4: version %u unknown (known: 8, 10)", b->version);
    }
    if (size < b->header_size)
        return fail(b, err, errlen, "offset %zu: header of version %u (%zu bytes) cut off at the end of the file",
                    size, b->version, b->header_size);
    b->column = word(buf + 6);
    b->row = word(buf + 8);
    b->top = word(buf + 12);
    if (word(buf + 10))
        return fail(b, err, errlen, "offset 10: header word %u unknown (known: 0)", word(buf + 10));
    if (b->version == 10 && word(buf + 14) != 2)
        return fail(b, err, errlen, "offset 14: header word %u unknown (known: 2)", word(buf + 14));
    return 0;
}

int bb22_parse(const uint8_t *buf, size_t size, struct bb22 *out, char *err, size_t errlen)
{
    struct bb22 b = {0};
    size_t p, q, cap = 0, i;

    *out = b;
    if (errlen)
        err[0] = 0;
    if (header(buf, size, &b, err, errlen))
        return -1;
    for (p = b.header_size; p < size; p = q + 1) {
        unsigned long line = (unsigned long)b.nlines + 1;

        if (size - p < b.prefix_size)
            return fail(&b, err, errlen, "line %lu (offset %zu): prefix cut off at the end of the file",
                        line, p);
        for (i = 0; i < b.prefix_size; i++)
            if (buf[p + i])
                return fail(&b, err, errlen, "line %lu (offset %zu): prefix byte $%02X unknown (known: 0)",
                            line, p + i, buf[p + i]);
        p += b.prefix_size;
        for (q = p; q < size && buf[q]; q++) {
            if (buf[q] == '\n')
                return fail(&b, err, errlen, "line %lu (offset %zu): LF in the line (TED would end the line there)",
                            line, q);
            if (buf[q] & 0x80) {
                if (q + 1 == size)
                    return fail(&b, err, errlen, "line %lu (offset %zu): token byte $%02X without its second byte",
                                line, q, buf[q]);
                if (!buf[q + 1])
                    return fail(&b, err, errlen,
                                "line %lu (offset %zu): token byte $%02X followed by a 0 byte (end of the line or token $%02X00?)",
                                line, q, buf[q], buf[q]);
                q++;
            }
        }
        if (q == size)
            return fail(&b, err, errlen, "line %lu (offset %zu): no 0 byte before the end of the file", line, p);
        if (b.nlines == cap) {
            struct bb22_line *n;

            cap = cap ? 2 * cap : 256;
            if (!(n = realloc(b.lines, cap * sizeof *n)))
                return fail(&b, err, errlen, "out of memory");
            b.lines = n;
        }
        b.lines[b.nlines].offset = p;
        b.lines[b.nlines].len = q - p;
        if (q - p > b.max_len)
            b.max_len = q - p;
        b.nlines++;
    }
    *out = b;
    return 0;
}

void bb22_free(struct bb22 *b)
{
    free(b->lines);
    memset(b, 0, sizeof *b);
}

size_t bb22_ted_size(const struct bb22 *b)
{
    size_t i, n = 0;

    for (i = 0; i < b->nlines; i++)
        n += b->lines[i].len + 1;
    return n;
}

void bb22_to_ted(const struct bb22 *b, const uint8_t *buf, uint8_t *out)
{
    size_t i;

    for (i = 0; i < b->nlines; i++) {
        memcpy(out, buf + b->lines[i].offset, b->lines[i].len);
        out += b->lines[i].len;
        *out++ = 0;
    }
}

int bb22_write_ted(const struct bb22 *b, const uint8_t *buf, FILE *f)
{
    size_t i;

    for (i = 0; i < b->nlines; i++)
        if (fwrite(buf + b->lines[i].offset, 1, b->lines[i].len, f) != b->lines[i].len
            || fputc(0, f) == EOF)
            return -1;
    return 0;
}
