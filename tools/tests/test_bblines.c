/*
 * test_bblines.c - line edits of tokenised sources built in memory: the
 * INCDIR lines of a source commented out, the other lines unchanged
 * (also tokens whose second byte is 0 or ':'), and broken inputs.
 */
#include "testutil.h"
#include "bblines.h"

static char err[300];

#define INCDIR 0x80BF
/* a string literal as bytes and length (without the literal's own 0) */
#define S(x) (const uint8_t *)(x), sizeof(x) - 1

static void test_comment(void)
{
    /* INCDIR "BlitzLibs:" / XINCLUDE "libmacs.bb" / "  INCDIR "" ; c" /
       a token with 0 and ':' as second byte / INCDIR in the middle */
    static const uint8_t src[] =
        "\x80\xBF \"BlitzLibs:\"\0"
        "\x80\xC0 \"libmacs.bb\"\0"
        "  \x80\xBF \"\" ; INCDIR: comment\0"
        "\x81\x00 \x81\x3A a:b\0"
        "a=1:\x80\xBF \"x\"\0";
    static const uint8_t want[] =
        ";\0"
        "\x80\xC0 \"libmacs.bb\"\0"
        ";\0"
        "\x81\x00 \x81\x3A a:b\0"
        "a=1:\x80\xBF \"x\"\0";
    uint8_t *out;
    size_t n, count;

    CHECK(bblines_comment(src, sizeof src - 1, INCDIR, &out, &n, &count, err, sizeof err) == 0);
    CHECK(count == 2 && n == sizeof want - 1 && !memcmp(out, want, n));
    free(out);
    /* another token: nothing changes */
    CHECK(bblines_comment(src, sizeof src - 1, 0x80C1, &out, &n, &count, err, sizeof err) == 0);
    CHECK(count == 0 && n == sizeof src - 1 && !memcmp(out, src, n));
    free(out);
    /* empty */
    CHECK(bblines_comment(src, 0, INCDIR, &out, &n, &count, err, sizeof err) == 0 && n == 0);
    free(out);
}

static void test_broken(void)
{
    uint8_t *out;
    size_t n, count;

    /* INCDIR with another statement on its line */
    CHECK(bblines_comment(S("\x80\xBF \"a\":b=1\0"), INCDIR, &out, &n, &count,
                          err, sizeof err) == -1 && strstr(err, "more statements"));
    /* a ':' in quotes is no statement end */
    CHECK(bblines_comment(S("\x80\xBF \"a:\"\0"), INCDIR, &out, &n, &count,
                          err, sizeof err) == 0 && count == 1);
    free(out);
    /* text (no 0), the last line without its end, a token cut off */
    CHECK(bblines_comment(S("a\nb\n"), INCDIR, &out, &n, &count, err,
                          sizeof err) == -1 && strstr(err, "not tokenised"));
    CHECK(bblines_comment(S("a\0b"), INCDIR, &out, &n, &count, err,
                          sizeof err) == -1 && strstr(err, "line 2: no line end"));
    CHECK(bblines_comment(S("a\0\x80"), INCDIR, &out, &n, &count, err,
                          sizeof err) == -1 && strstr(err, "second byte"));
    /* not a token word */
    CHECK(bblines_comment(S("a\0"), 0x00BF, &out, &n, &count, err,
                          sizeof err) == -1);
}

int main(void)
{
    test_comment();
    test_broken();
    if (failures)
        fprintf(stderr, "%d check(s) failed\n", failures);
    return failures != 0;
}
