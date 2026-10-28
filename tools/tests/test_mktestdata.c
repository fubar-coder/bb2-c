/*
 * test_mktestdata.c - mktestdata with descriptions from memory: values,
 * labels (also forward), IFF lengths and pad bytes, declared lengths, and
 * broken descriptions (each must fail with the line in the message).
 */
#include "testutil.h"
#include "mktestdata.h"

static uint8_t *out;
static size_t outlen;
static char err[200];

static int build(const char *spec)
{
    free(out);
    out = NULL;
    outlen = 0;
    err[0] = 0;
    return mktestdata_build(spec, &out, &outlen, err, sizeof err);
}

static int same(const char *spec, const void *want, size_t len)
{
    return !build(spec) && outlen == len && !memcmp(out, want, len);
}

/* broken: fails, and the message names the line */
static int broken(const char *spec, const char *line)
{
    return build(spec) == -1 && !strncmp(err, line, strlen(line));
}

int main(void)
{
    /* values: sizes, big-endian, number forms, characters, expressions */
    CHECK(same("b 1 $ff 0x10 -1\n", "\x01\xff\x10\xff", 4));
    CHECK(same("w $1234 -2\nl 'MMD0' 'AB'\n", "\x12\x34\xff\xfe" "MMD0\0\0AB", 12));
    CHECK(same("l 3+4-1 ; comment\n# a comment line\n\n", "\0\0\0\x06", 4));
    CHECK(same("s \"a;b#\\\"\\0\"\n", "a;b#\"\0", 6));
    CHECK(same("fill 3 $aa\nb ';'\n", "\xaa\xaa\xaa;", 4));
    CHECK(same("b 1\nalign 4\nb 2\n", "\x01\0\0\0\x02", 5));
    CHECK(same("", "", 0));

    /* labels: offsets, forward references, use in counts (backward only) */
    CHECK(same("l @there\nb 9\nthere:\nb @there\n", "\0\0\0\x05\x09\x05", 6));
    CHECK(same("a:\nb 1 2\nc:\nfill @c-@a 7\nw @end-@a\nend:\n",
               "\x01\x02\x07\x07\0\x06", 6));

    /* IFF: lengths, pad byte, nesting, declared length */
    CHECK(same("form 8SVX\nchunk VHDR\nb 1 2 3\nend\nend\n",
               "FORM\0\0\0\x10" "8SVX" "VHDR\0\0\0\x03\x01\x02\x03\0", 24));
    CHECK(same("chunk BODY 131072\nb 5\nend\n", "BODY\0\x02\0\0\x05\0", 10));
    CHECK(same("form ILBM\nend\n", "FORM\0\0\0\x04ILBM", 12));
    CHECK(same("chunk '(c) ' 2 ; c\nend\n", "(c) \0\0\0\x02", 8));
    CHECK(same("chunk BODY 0\nend\nchunk XMRK\ns \"ab\"\nend\n",
               "BODY\0\0\0\0XMRK\0\0\0\x02" "ab", 18));

    /* broken descriptions */
    CHECK(broken("b 1\nfoo 2\n", "line 2: unknown directive"));
    CHECK(broken("b 256\n", "line 1: value 256 does not fit"));
    CHECK(broken("w -32769\n", "line 1: value -32769 does not fit"));
    CHECK(broken("b\n", "line 1: b needs a value"));
    CHECK(broken("b 1x\n", "line 1: bad value"));
    CHECK(broken("l $123456789\n", "line 1: number too large"));
    CHECK(broken("b 'ABCDE'\n", "line 1: bad character constant"));
    CHECK(broken("l @nowhere\n", "line 1: unknown label nowhere"));
    CHECK(broken("x:\nx:\n", "line 2: label x defined twice"));
    CHECK(broken("fill @x 0\nx:\n", "line 1: a count must not refer to a later label"));
    CHECK(broken("align 0\n", "line 1: align N"));
    CHECK(broken("chunk BOD\nend\n", "line 1: an IFF id has 4 characters"));
    CHECK(broken("chunk 'AB C\nend\n", "line 1: an IFF id has 4 characters"));
    CHECK(broken("chunk 'AB '\nend\n", "line 1: an IFF id has 4 characters"));
    CHECK(broken("form ILBM\nchunk BMHD\nend\n", "line 3: 1 form(s)/chunk(s) without end"));
    CHECK(broken("end\n", "line 1: end without form or chunk"));
    CHECK(broken("s \"abc\n", "line 1: unterminated text"));
    CHECK(broken("s \"\\q\"\n", "line 1: unknown escape"));
    CHECK(broken("end 3\n", "line 1: text behind end"));

    free(out);
    return failures != 0;
}
