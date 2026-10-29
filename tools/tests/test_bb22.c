/*
 * test_bb22.c - the BB22 reader with files built in memory: both versions,
 * a file without lines, tokens, the TED format written to a buffer and a
 * file, and broken files (each must be refused with its message).
 */
#include "testutil.h"
#include "bb22.h"

static struct bb22 b;
static char err[200];

/* an empty buffer for the next file */
static void reset(struct buf *f)
{
    free(f->p);
    f->p = NULL;
    f->n = f->cap = 0;
}

/* a header: "BB22", the version, column, row, w10, top (and w14 for 10) */
static void header(struct buf *f, unsigned version, unsigned column, unsigned row, unsigned w10,
                   unsigned top, unsigned w14)
{
    putbytes(f, "BB22", 4);
    put16(f, version);
    put16(f, column);
    put16(f, row);
    put16(f, w10);
    put16(f, top);
    if (version != 8)
        put16(f, w14);
}

/* a line: the prefix (prefix_size zero bytes), text, 0 */
static void line(struct buf *f, size_t prefix_size, const void *text, size_t len)
{
    size_t i;

    for (i = 0; i < prefix_size; i++)
        put8(f, 0);
    putbytes(f, text, len);
    put8(f, 0);
}

static int parse_raw(const uint8_t *p, size_t n)
{
    bb22_free(&b);
    err[0] = 0;
    return bb22_parse(p, n, &b, err, sizeof err);
}

static int parse(const struct buf *f)
{
    return parse_raw(f->p, f->n);
}

/* p[0..n) refused, the message begins with msg, nothing left in b */
static int refused_raw(const void *p, size_t n, const char *msg)
{
    if (parse_raw(p, n) != -1 || b.lines || b.nlines)
        return 0;
    if (strncmp(err, msg, strlen(msg))) {
        fprintf(stderr, "message: %s\n", err);
        return 0;
    }
    return 1;
}

static int refused(const struct buf *f, const char *msg)
{
    return refused_raw(f->p, f->n, msg);
}

/* the TED format of the parsed file equals want */
static int ted(const struct buf *f, const void *want, size_t len)
{
    uint8_t *out;
    int ok;

    if (bb22_ted_size(&b) != len)
        return 0;
    out = malloc(len + 1);
    bb22_to_ted(&b, f->p, out);
    ok = !memcmp(out, want, len);
    free(out);
    return ok;
}

static int ted_file(const struct buf *f, const void *want, size_t len)
{
    FILE *t = tmpfile();
    uint8_t got[64];
    size_t n;

    if (!t || bb22_write_ted(&b, f->p, t))
        return 0;
    rewind(t);
    n = fread(got, 1, sizeof got, t);
    fclose(t);
    return n == len && !memcmp(got, want, len);
}

int main(void)
{
    struct buf f = {0};
    static const char want[] = "a\x80\x94 b\0\0;c\xC1\x02\0";

    /* version 10: header, three lines (a token, an empty line, a library token) */
    header(&f, 10, 13, 25, 0, 441, 2);
    line(&f, 2, "a\x80\x94 b", 5);
    line(&f, 2, "", 0);
    line(&f, 2, ";c\xC1\x02", 4);
    CHECK(bb22_is(f.p, f.n));
    CHECK(!parse(&f));
    CHECK(b.version == 10 && b.header_size == 16 && b.prefix_size == 2);
    CHECK(b.column == 13 && b.row == 25 && b.top == 441);
    CHECK(b.nlines == 3 && b.max_len == 5);
    CHECK(b.lines && b.lines[0].offset == 18 && b.lines[0].len == 5);
    CHECK(b.lines[1].len == 0 && b.lines[2].offset == 29 && b.lines[2].len == 4);
    CHECK(ted(&f, want, sizeof want - 1));
    CHECK(ted_file(&f, want, sizeof want - 1));

    /* version 8: 14-byte header, 1 prefix byte */
    reset(&f);
    header(&f, 8, 19, 1, 0, 96, 0);
    line(&f, 1, "a\x80\x94 b", 5);
    line(&f, 1, "", 0);
    line(&f, 1, ";c\xC1\x02", 4);
    CHECK(!parse(&f));
    CHECK(b.version == 8 && b.header_size == 14 && b.prefix_size == 1);
    CHECK(b.column == 19 && b.row == 1 && b.top == 96);
    CHECK(b.nlines == 3 && b.max_len == 5 && b.lines[0].offset == 15);
    CHECK(ted(&f, want, sizeof want - 1));

    /* a token whose second byte has bit 7 too, and 0 in the low half of a
       word is fine as long as it is not a token's second byte */
    reset(&f);
    header(&f, 10, 0, 0, 0, 0, 2);
    line(&f, 2, "\x80\x80\xff\xff", 4);
    CHECK(!parse(&f));
    CHECK(b.nlines == 1 && b.lines[0].len == 4);
    CHECK(ted(&f, "\x80\x80\xff\xff", 5));

    /* no lines: the header alone, the TED format is empty */
    reset(&f);
    header(&f, 10, 0, 0, 0, 0, 2);
    CHECK(!parse(&f));
    CHECK(b.nlines == 0 && !b.lines && b.max_len == 0);
    CHECK(bb22_ted_size(&b) == 0);
    CHECK(ted_file(&f, "", 0));
    reset(&f);
    header(&f, 8, 0, 0, 0, 0, 0);
    CHECK(!parse(&f) && b.nlines == 0);

    /* the probe */
    CHECK(!bb22_is((const uint8_t *)"BB2", 3));
    CHECK(!bb22_is((const uint8_t *)";BB22", 5));
    CHECK(bb22_is((const uint8_t *)"BB22", 4));

    /* broken: not BB22, header cut off, unknown version or header words */
    CHECK(refused_raw("", 0, "offset 0: no \"BB22\""));
    CHECK(refused_raw("; text\0", 7, "offset 0: no \"BB22\""));
    reset(&f);
    putbytes(&f, "BB22", 4);
    CHECK(refused(&f, "offset 4: header cut off"));
    put16(&f, 10);
    put16(&f, 0);
    CHECK(refused(&f, "offset 8: header of version 10 (16 bytes) cut off"));
    reset(&f);
    header(&f, 9, 0, 0, 0, 0, 2);
    CHECK(refused(&f, "offset 4: version 9 unknown"));
    reset(&f);
    header(&f, 10, 0, 0, 1, 0, 2);
    CHECK(refused(&f, "offset 10: header word 1 unknown"));
    reset(&f);
    header(&f, 10, 0, 0, 0, 0, 3);
    CHECK(refused(&f, "offset 14: header word 3 unknown"));

    /* broken lines: prefix not 0, prefix cut off, no 0 byte at the end,
       token byte at the end, token byte before the 0 byte, LF */
    reset(&f);
    header(&f, 10, 0, 0, 0, 0, 2);
    line(&f, 2, "ok", 2);
    put8(&f, 0);
    put8(&f, 1);
    putbytes(&f, "x\0", 2);
    CHECK(refused(&f, "line 2 (offset 22): prefix byte $01 unknown"));
    reset(&f);
    header(&f, 8, 0, 0, 0, 0, 0);
    put8(&f, 0x80);
    putbytes(&f, "x\0", 2);
    CHECK(refused(&f, "line 1 (offset 14): prefix byte $80 unknown"));
    reset(&f);
    header(&f, 10, 0, 0, 0, 0, 2);
    line(&f, 2, "ok", 2);
    put8(&f, 0);
    CHECK(refused(&f, "line 2 (offset 21): prefix cut off"));
    reset(&f);
    header(&f, 10, 0, 0, 0, 0, 2);
    line(&f, 2, "ok", 2);
    putbytes(&f, "\0\0abc", 5);
    CHECK(refused(&f, "line 2 (offset 23): no 0 byte before the end of the file"));
    reset(&f);
    header(&f, 10, 0, 0, 0, 0, 2);
    putbytes(&f, "\0\0", 2);
    CHECK(refused(&f, "line 1 (offset 18): no 0 byte before the end of the file"));
    reset(&f);
    header(&f, 10, 0, 0, 0, 0, 2);
    putbytes(&f, "\0\0a\x80", 4);
    CHECK(refused(&f, "line 1 (offset 19): token byte $80 without its second byte"));
    reset(&f);
    header(&f, 10, 0, 0, 0, 0, 2);
    line(&f, 2, "a\xC1", 2);
    CHECK(refused(&f, "line 1 (offset 19): token byte $C1 followed by a 0 byte"));
    reset(&f);
    header(&f, 8, 0, 0, 0, 0, 0);
    line(&f, 1, "a\nb", 3);
    CHECK(refused(&f, "line 1 (offset 16): LF in the line"));

    bb22_free(&b);
    free(f.p);
    return failures;
}
