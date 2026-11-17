// test_blitztokens - the token list from a compiler and libraries built
// in memory: the compiler's list, the numbering of user and system
// libraries with a nullsub, BigLib files, the output, and broken inputs.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <snitch/snitch.hpp>
#include "testutil.hpp"
extern "C" {
#include "blitztokens.h"
}

/* a token node: next.l num.w "name",0 "help",0 even */
static void node(buf &b, uint32_t next, uint16_t num, const char *name)
{
    put32(b, next);
    put16(b, num);
    putbytes(b, name, strlen(name) + 1);
    put8(b, 0);                         /* empty help */
    if (b.size() & 1)
        put8(b, 0);
}

/* a compiler: some code, then three nodes; the next of the last one is
   `last` (0 = end), reloc says whether that long is relocated */
static buf compiler(uint32_t last, int reloc_last, const char *first)
{
    buf code, f;
    uint32_t relocs[3], n0, n1, n2;

    put32(code, 0x70004E75);           /* "code" before the list */
    n0 = (uint32_t)code.size();
    n1 = n0 + 4 + 2 + 8 + 1 + 1;        /* NEWTYPE: 4+2+"NEWTYPE\0"+"\0", even */
    n2 = n1 + 4 + 2 + 4 + 1 + 1;        /* End */
    node(code, n1, 1, first);
    node(code, n2, 2, "End");
    node(code, last, 3, "Let");
    relocs[0] = n0;
    relocs[1] = n1;
    relocs[2] = n2;
    f = loadfile1(1, HUNK_CODE, code, relocs, reloc_last ? 3 : 2, 0);
    return f;
}

/* a compiler with two lists (as blitz2-ab3): NEWTYPE End, then NEWTYPE
   End Let */
static buf compiler2(void)
{
    buf code, f;
    uint32_t relocs[3], a0, a1, b0, b1, b2;

    put32(code, 0x70004E75);
    a0 = (uint32_t)code.size();
    a1 = a0 + 16;                       /* 4+2+"NEWTYPE\0"+"\0" */
    b0 = a1 + 12;                       /* 4+2+"End\0"+"\0", even */
    b1 = b0 + 16;
    b2 = b1 + 12;
    node(code, a1, 1, "NEWTYPE");
    node(code, 0, 2, "End");
    node(code, b1, 1, "NEWTYPE");
    node(code, b2, 2, "End");
    node(code, 0, 3, "Let");
    relocs[0] = a0;
    relocs[1] = b0;
    relocs[2] = b1;
    f = loadfile1(1, HUNK_CODE, code, relocs, 3, 0);
    return f;
}

/* a compiler as AmiBlitz 2's: the nodes not word-aligned, the first one at
   an odd offset */
static buf compiler_odd(void)
{
    buf code, f;
    uint32_t relocs[2], n0, n1, n2;

    putbytes(code, "\x70\x00\x4E\x75\x00", 5);
    n0 = (uint32_t)code.size();              /* 5 */
    n1 = n0 + 4 + 2 + 8 + 3;            /* NEWTYPE, help "Ab\0": 22 */
    n2 = n1 + 4 + 2 + 4 + 1;            /* End: 33 */
    put32(code, n1);
    put16(code, 1);
    putbytes(code, "NEWTYPE\0Ab\0", 11);
    put32(code, n2);
    put16(code, 2);
    putbytes(code, "End\0\0", 5);
    put32(code, 0);
    put16(code, 3);
    putbytes(code, "Let\0\0", 5);
    relocs[0] = n0;
    relocs[1] = n1;
    f = loadfile1(1, HUNK_CODE, code, relocs, 2, 0);
    return f;
}

/* a library: header with libnum, then statement A, a nullsub, dumtoke D,
   statement B */
static void stmt(buf &b, const char *name)
{
    put16(b, 1);
    put32(b, 0);
    put16(b, 0x0000);                   /* form without arguments */
    put16(b, 0);                        /* no libs */
    put32(b, 0x80); put32(b, 0); put32(b, 0);
    put16(b, 0xFFFF);
    put32(b, 0);
    put16(b, 0);
    putbytes(b, name, strlen(name) + 1);
    put8(b, 0);
    if (b.size() & 1)
        put8(b, 0);
}

static buf library(uint16_t libnum)
{
    buf code, f;

    put32(code, 0x70004E75);
    put16(code, libnum);
    while (code.size() < 0x30)
        put8(code, 0);
    stmt(code, "A");
    put16(code, 0);                    /* nullsub */
    put32(code, 0);
    put16(code, 0);                    /* no libs */
    put32(code, 0x80); put32(code, 0); put32(code, 0);
    put16(code, 8);                    /* dumtoke */
    put32(code, 0);
    put32(code, 0);
    put16(code, 0);
    putbytes(code, "D\0\0", 4);
    stmt(code, "B");
    put16(code, 0xFFFF);
    put32(code, 0);                    /* no !libfin */
    put16(code, 0x4E75);
    f = loadfile1(1, HUNK_CODE, code, NULL, 0, 0);
    return f;
}

static int has(const struct blitztokens *l, size_t i, uint16_t num, const char *name)
{
    return i < l->n && l->t[i].num == num && !strcmp(l->t[i].name, name);
}

TEST_CASE("blitztokens: checks", "[blitztokens]")
{
    struct blitztokens l = {};
    buf c, lib, sys, big;
    char err[300];

    /* the compiler's list, ended by next 0 */
    c = compiler(0, 0, "NEWTYPE");
    CHECK(blitztokens_compiler(c.data(), c.size(), &l, err, sizeof err) == 0);
    CHECK(l.n == 3 && has(&l, 0, 0x8001, "NEWTYPE") && has(&l, 1, 0x8002, "End")
          && has(&l, 2, 0x8003, "Let"));

    /* a user library behind it: libnum*128+1, the nullsub counts */
    lib = library(100);
    CHECK(blitztokens_libraries(lib.data(), lib.size(), &l, err, sizeof err) == 0);
    CHECK(l.n == 6 && has(&l, 3, 0xB201, "A") && has(&l, 4, 0xB203, "D")
          && has(&l, 5, 0xB204, "B"));

    /* the output */
    {
        FILE *f = tmpfile();
        char text[200];
        size_t n;

        REQUIRE(f != NULL);
        CHECK(blitztokens_write(f, &l) == 0);
        rewind(f);
        n = fread(text, 1, sizeof text - 1, f);
        text[n] = 0;
        fclose(f);
        CHECK(!strcmp(text, "8001 NEWTYPE\n8002 End\n8003 Let\nB201 A\nB203 D\nB204 B\n"));
    }
    blitztokens_free(&l);

    /* a system library (bit 15): counted from its number; both in a BigLib */
    sys = library(0xFF9A);
    put32(big, (uint32_t)lib.size());
    putbytes(big, lib.data(), lib.size());
    put32(big, (uint32_t)sys.size());
    putbytes(big, sys.data(), sys.size());
    put32(big, 0);
    CHECK(blitztokens_libraries(big.data(), big.size(), &l, err, sizeof err) == 0);
    CHECK(l.n == 6 && has(&l, 0, 0xB201, "A") && has(&l, 3, 0xFF9A, "A")
          && has(&l, 4, 0xFF9C, "D") && has(&l, 5, 0xFF9D, "B"));
    blitztokens_free(&l);

    /* broken: BigLib without the end mark, a library without header (code
       shorter than $30 bytes), inside a BigLib with the library's index */
    big.resize(big.size() - 4);
    CHECK(blitztokens_libraries(big.data(), big.size(), &l, err, sizeof err) == -1);
    blitztokens_free(&l);
    {
        buf code, bad, big2;

        put32(code, 0x70004E75);
        put32(code, 0);
        bad = loadfile1(1, HUNK_CODE, code, NULL, 0, 0);
        CHECK(blitztokens_libraries(bad.data(), bad.size(), &l, err, sizeof err) == -1
              && strstr(err, "header"));
        put32(big2, (uint32_t)lib.size());
        putbytes(big2, lib.data(), lib.size());
        put32(big2, (uint32_t)bad.size());
        putbytes(big2, bad.data(), bad.size());
        put32(big2, 0);
        CHECK(blitztokens_libraries(big2.data(), big2.size(), &l, err, sizeof err) == -1
              && strstr(err, "library 2"));
    }
    blitztokens_free(&l);

    /* broken compilers: no NEWTYPE, next without relocation, a loop */
    c = compiler(0, 0, "NEWTYPX");
    CHECK(blitztokens_compiler(c.data(), c.size(), &l, err, sizeof err) == -1 && strstr(err, "NEWTYPE"));
    blitztokens_free(&l);
    c = compiler(4, 0, "NEWTYPE");
    CHECK(blitztokens_compiler(c.data(), c.size(), &l, err, sizeof err) == -1
          && strstr(err, "without relocation"));
    blitztokens_free(&l);
    c = compiler(4, 1, "NEWTYPE");      /* the last node points to the first */
    CHECK(blitztokens_compiler(c.data(), c.size(), &l, err, sizeof err) == -1 && strstr(err, "loop"));
    blitztokens_free(&l);
    CHECK(blitztokens_compiler_last(c.data(), c.size(), 0x8003, &l, err, sizeof err) == -1
          && strstr(err, "loop"));
    blitztokens_free(&l);

    /* a list file read back: LF, CR LF, an empty line, the last line
       without LF; broken: no name, no hex number, no bit 15 */
    {
        char text[] = "8001 NEWTYPE\r\n\nB201 Print Two\nFF9A x";
        char bad1[] = "8001 NEWTYPE\n8002\n", bad2[] = "80G1 End\n", bad3[] = "0001 End\n";

        REQUIRE(blitztokens_read(text, sizeof text - 1, &l, err, sizeof err) == 0);
        CHECK(l.n == 3 && has(&l, 0, 0x8001, "NEWTYPE") && has(&l, 1, 0xB201, "Print Two")
              && has(&l, 2, 0xFF9A, "x"));
        blitztokens_free(&l);
        CHECK(blitztokens_read(bad1, sizeof bad1 - 1, &l, err, sizeof err) == -1
              && !strcmp(err, "line 2: not \"XXXX name\"") && l.n == 1);
        blitztokens_free(&l);
        CHECK(blitztokens_read(bad2, sizeof bad2 - 1, &l, err, sizeof err) == -1
              && strstr(err, "line 1: not"));
        blitztokens_free(&l);
        CHECK(blitztokens_read(bad3, sizeof bad3 - 1, &l, err, sizeof err) == -1
              && strstr(err, "without bit 15"));
        blitztokens_free(&l);
    }

    /* AmiBlitz 2: nodes at odd offsets; broken: a next behind the hunk */
    c = compiler_odd();
    CHECK(blitztokens_compiler(c.data(), c.size(), &l, err, sizeof err) == 0);
    CHECK(l.n == 3 && has(&l, 0, 0x8001, "NEWTYPE") && has(&l, 1, 0x8002, "End")
          && has(&l, 2, 0x8003, "Let"));
    blitztokens_free(&l);
    c[0x20 + 5 + 3] = 0x7F;           /* NEWTYPE's next: $7F */
    CHECK(blitztokens_compiler(c.data(), c.size(), &l, err, sizeof err) == -1
          && strstr(err, "$7F outside its hunk"));
    blitztokens_free(&l);

    /* two lists: only one with -last, chosen by its last token */
    c = compiler2();
    CHECK(blitztokens_compiler(c.data(), c.size(), &l, err, sizeof err) == -1 && strstr(err, "twice"));
    blitztokens_free(&l);
    CHECK(blitztokens_compiler_last(c.data(), c.size(), 0x8003, &l, err, sizeof err) == 0);
    CHECK(l.n == 3 && has(&l, 0, 0x8001, "NEWTYPE") && has(&l, 2, 0x8003, "Let"));
    blitztokens_free(&l);
    CHECK(blitztokens_compiler_last(c.data(), c.size(), 2, &l, err, sizeof err) == 0);
    CHECK(l.n == 2 && has(&l, 1, 0x8002, "End"));
    blitztokens_free(&l);
    CHECK(blitztokens_compiler_last(c.data(), c.size(), 0x8004, &l, err, sizeof err) == -1
          && strstr(err, "no token list ends with $8004"));
    blitztokens_free(&l);
    /* one list, -last given: it must end with that token */
    c = compiler(0, 0, "NEWTYPE");
    CHECK(blitztokens_compiler_last(c.data(), c.size(), 0x8003, &l, err, sizeof err) == 0 && l.n == 3);
    blitztokens_free(&l);
    CHECK(blitztokens_compiler_last(c.data(), c.size(), 0x8002, &l, err, sizeof err) == -1);
    blitztokens_free(&l);

}
