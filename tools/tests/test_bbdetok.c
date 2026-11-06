/*
 * test_bbdetok.c - tokenised sources built in memory as text: names,
 * unknown tokens, AmiBlitz's strings and comments against TED's rules,
 * the line ends, and broken input.
 */
#include "testutil.h"
#include "bbdetok.h"

static struct bbdetok d;
static struct bbdetok_result r;

/* the text of src[0..len) by rules; NULL on an error (err) */
static char *text(int rules, const char *src, size_t len, char *err)
{
    char *out;
    size_t n;
    static char buf[1000];

    if (bbdetok_text(&d, rules, (const uint8_t *)src, len, &out, &n, &r, err, 200))
        return NULL;
    if (n >= sizeof buf)
        abort();
    memcpy(buf, out, n);
    buf[n] = 0;
    free(out);
    return buf;
}

#define TEXT(rules, lit, err) text(rules, lit, sizeof lit - 1, err)

int main(void)
{
    struct blitztokens l = { 0 };
    char list[] = "8001 NEWTYPE\n8021 NPrint\n8021 Other\nB201 Print\n8100 Zero\n";
    char err[200], *t;

    CHECK(blitztokens_read(list, sizeof list - 1, &l, err, sizeof err) == 0);
    CHECK(bbdetok_init(&d, &l) == 0);

    /* names (the first entry of a number), plain bytes, TAB, an empty line */
    t = TEXT(BBDETOK_AB, "\x80\x21 \"a\"\0\0\tx=1:\xB2\x01 x\0", err);
    CHECK(t && !strcmp(t, "NPrint \"a\"\n\n\tx=1:Print x\n"));
    CHECK(r.lines == 3 && r.tokens == 2 && r.unknown == 0 && r.kinds == 0 && !r.open);

    /* unknown tokens: {L:N}, counted per number */
    t = TEXT(BBDETOK_AB, "\xEC\x82 \xFC\x6C\xEC\x82\0\x80\x7F\0", err);
    CHECK(t && !strcmp(t, "{217:2} {248:108}{217:2}\n{0:127}\n"));
    CHECK(r.tokens == 4 && r.unknown == 4 && r.kinds == 3 && r.count[0x6C82] == 2
          && r.count[0x7C6C] == 1 && r.count[0x007F] == 1);

    /* AmiBlitz: umlauts in strings and comments are characters, a token
       after the string again a token; ; in a string starts no comment,
       " in a comment no string; a string open at the end of the line */
    t = TEXT(BBDETOK_AB, "\x80\x21 \"f\xFCr ;\"\xB2\x01 ;gr\xF6\xDF \"x\0\"\xE4\0\xB2\x01\0", err);
    CHECK(t && !strcmp(t, "NPrint \"f\xFCr ;\"Print ;gr\xF6\xDF \"x\n\"\xE4\nPrint\n"));
    CHECK(r.tokens == 3 && r.unknown == 0);
    /* TED: every byte from $80 a token, also there */
    t = TEXT(BBDETOK_TED, "\"\xB2\x01\" ;\x80\x21\0", err);
    CHECK(t && !strcmp(t, "\"Print\" ;NPrint\n"));
    t = TEXT(BBDETOK_AB, "\"\xB2\x01\" ;\x80\x21\0", err);
    CHECK(t && !strcmp(t, "\"\xB2\x01\" ;NPrint\n") && r.tokens == 1 && r.comment == 1);
    /* AmiBlitz: in a comment only a known number is a token (a line
       commented out), an unknown one and one with a 0 are characters */
    t = TEXT(BBDETOK_AB, ";\xB2\x01 \xFC\x6C \x81\0\x81\0", err);
    CHECK(t && !strcmp(t, ";Print \xFCl \x81\nZero\n") && r.tokens == 2 && r.comment == 1
          && r.unknown == 0);

    /* a token's second byte 0 does not end the line; a last line without
       its 0 is written (open); an empty file */
    t = TEXT(BBDETOK_AB, "\x82\x00x\0y", err);
    CHECK(t && !strcmp(t, "{4:0}x\ny\n") && r.lines == 2 && r.open);
    t = TEXT(BBDETOK_AB, "\x82\x00", err);
    CHECK(t && !strcmp(t, "{4:0}\n") && r.lines == 1 && r.open);
    t = TEXT(BBDETOK_AB, "", err);
    CHECK(t && !strcmp(t, "") && r.lines == 0 && !r.open);

    /* broken: a token byte as the last byte, unknown rules */
    CHECK(!TEXT(BBDETOK_AB, "x\0\x80\x21\xB2", err)
          && !strcmp(err, "line 2: token byte $B2 at the end of the file"));
    CHECK(!TEXT(BBDETOK_TED, "\"\xB2", err) && strstr(err, "line 1: token byte $B2"));
    CHECK(!TEXT(7, "x\0", err) && strstr(err, "unknown rules"));
    /* in AmiBlitz's rules the same bytes in a string are characters */
    t = TEXT(BBDETOK_AB, "\"\xB2", err);
    CHECK(t && !strcmp(t, "\"\xB2\n") && r.open);

    bbdetok_free(&d);
    blitztokens_free(&l);
    return failures != 0;
}
