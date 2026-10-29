/*
 * test_blitztokens.c - the token list from a compiler and libraries built
 * in memory: the compiler's list, the numbering of user and system
 * libraries with a nullsub, BigLib files, the output, and broken inputs.
 */
#include "testutil.h"
#include "blitztokens.h"

/* a token node: next.l num.w "name",0 "help",0 even */
static void node(struct buf *b, uint32_t next, uint16_t num, const char *name)
{
    put32(b, next);
    put16(b, num);
    putbytes(b, name, strlen(name) + 1);
    put8(b, 0);                         /* empty help */
    if (b->n & 1)
        put8(b, 0);
}

/* a compiler: some code, then three nodes; the next of the last one is
   `last` (0 = end), reloc says whether that long is relocated */
static struct buf compiler(uint32_t last, int reloc_last, const char *first)
{
    struct buf code = { 0 }, f;
    uint32_t relocs[3], n0, n1, n2;

    put32(&code, 0x70004E75);           /* "code" before the list */
    n0 = (uint32_t)code.n;
    n1 = n0 + 4 + 2 + 8 + 1 + 1;        /* NEWTYPE: 4+2+"NEWTYPE\0"+"\0", even */
    n2 = n1 + 4 + 2 + 4 + 1 + 1;        /* End */
    node(&code, n1, 1, first);
    node(&code, n2, 2, "End");
    node(&code, last, 3, "Let");
    relocs[0] = n0;
    relocs[1] = n1;
    relocs[2] = n2;
    f = loadfile1(1, HUNK_CODE, &code, relocs, reloc_last ? 3 : 2, 0);
    free(code.p);
    return f;
}

/* a library: header with libnum, then statement A, a nullsub, dumtoke D,
   statement B */
static void stmt(struct buf *b, const char *name)
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
    if (b->n & 1)
        put8(b, 0);
}

static struct buf library(uint16_t libnum)
{
    struct buf code = { 0 }, f;

    put32(&code, 0x70004E75);
    put16(&code, libnum);
    while (code.n < 0x30)
        put8(&code, 0);
    stmt(&code, "A");
    put16(&code, 0);                    /* nullsub */
    put32(&code, 0);
    put16(&code, 0);                    /* no libs */
    put32(&code, 0x80); put32(&code, 0); put32(&code, 0);
    put16(&code, 8);                    /* dumtoke */
    put32(&code, 0);
    put32(&code, 0);
    put16(&code, 0);
    putbytes(&code, "D\0\0", 4);
    stmt(&code, "B");
    put16(&code, 0xFFFF);
    put32(&code, 0);                    /* no !libfin */
    put16(&code, 0x4E75);
    f = loadfile1(1, HUNK_CODE, &code, NULL, 0, 0);
    free(code.p);
    return f;
}

static int has(const struct blitztokens *l, size_t i, uint16_t num, const char *name)
{
    return i < l->n && l->t[i].num == num && !strcmp(l->t[i].name, name);
}

int main(void)
{
    struct blitztokens l = { 0 };
    struct buf c, lib, sys, big = { 0 };
    char err[300];

    /* the compiler's list, ended by next 0 */
    c = compiler(0, 0, "NEWTYPE");
    CHECK(blitztokens_compiler(c.p, c.n, &l, err, sizeof err) == 0);
    CHECK(l.n == 3 && has(&l, 0, 0x8001, "NEWTYPE") && has(&l, 1, 0x8002, "End")
          && has(&l, 2, 0x8003, "Let"));

    /* a user library behind it: libnum*128+1, the nullsub counts */
    lib = library(100);
    CHECK(blitztokens_libraries(lib.p, lib.n, &l, err, sizeof err) == 0);
    CHECK(l.n == 6 && has(&l, 3, 0xB201, "A") && has(&l, 4, 0xB203, "D")
          && has(&l, 5, 0xB204, "B"));

    /* the output */
    {
        FILE *f = tmpfile();
        char text[200];
        size_t n;

        CHECK(f && blitztokens_write(f, &l) == 0);
        rewind(f);
        n = fread(text, 1, sizeof text - 1, f);
        text[n] = 0;
        fclose(f);
        CHECK(!strcmp(text, "8001 NEWTYPE\n8002 End\n8003 Let\nB201 A\nB203 D\nB204 B\n"));
    }
    blitztokens_free(&l);

    /* a system library (bit 15): counted from its number; both in a BigLib */
    sys = library(0xFF9A);
    put32(&big, (uint32_t)lib.n);
    putbytes(&big, lib.p, lib.n);
    put32(&big, (uint32_t)sys.n);
    putbytes(&big, sys.p, sys.n);
    put32(&big, 0);
    CHECK(blitztokens_libraries(big.p, big.n, &l, err, sizeof err) == 0);
    CHECK(l.n == 6 && has(&l, 0, 0xB201, "A") && has(&l, 3, 0xFF9A, "A")
          && has(&l, 4, 0xFF9C, "D") && has(&l, 5, 0xFF9D, "B"));
    blitztokens_free(&l);

    /* broken: BigLib without the end mark, a library without header (code
       shorter than $30 bytes), inside a BigLib with the library's index */
    big.n -= 4;
    CHECK(blitztokens_libraries(big.p, big.n, &l, err, sizeof err) == -1);
    blitztokens_free(&l);
    {
        struct buf code = { 0 }, bad, big2 = { 0 };

        put32(&code, 0x70004E75);
        put32(&code, 0);
        bad = loadfile1(1, HUNK_CODE, &code, NULL, 0, 0);
        CHECK(blitztokens_libraries(bad.p, bad.n, &l, err, sizeof err) == -1
              && strstr(err, "header"));
        put32(&big2, (uint32_t)lib.n);
        putbytes(&big2, lib.p, lib.n);
        put32(&big2, (uint32_t)bad.n);
        putbytes(&big2, bad.p, bad.n);
        put32(&big2, 0);
        CHECK(blitztokens_libraries(big2.p, big2.n, &l, err, sizeof err) == -1
              && strstr(err, "library 2"));
        free(code.p);
        free(bad.p);
        free(big2.p);
    }
    blitztokens_free(&l);

    /* broken compilers: no NEWTYPE, next without relocation, a loop */
    free(c.p);
    c = compiler(0, 0, "NEWTYPX");
    CHECK(blitztokens_compiler(c.p, c.n, &l, err, sizeof err) == -1 && strstr(err, "NEWTYPE"));
    free(c.p);
    blitztokens_free(&l);
    c = compiler(4, 0, "NEWTYPE");
    CHECK(blitztokens_compiler(c.p, c.n, &l, err, sizeof err) == -1
          && strstr(err, "without relocation"));
    free(c.p);
    blitztokens_free(&l);
    c = compiler(4, 1, "NEWTYPE");      /* the last node points to the first */
    CHECK(blitztokens_compiler(c.p, c.n, &l, err, sizeof err) == -1 && strstr(err, "loop"));
    blitztokens_free(&l);

    free(c.p);
    free(lib.p);
    free(sys.p);
    free(big.p);
    return failures != 0;
}
