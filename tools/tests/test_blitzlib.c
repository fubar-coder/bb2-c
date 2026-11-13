/*
 * test_blitzlib.c - blitzlib_read and blitzlib_check on a minimal library
 * and on broken variants.
 */
#include "testutil.h"
#include "blitzlib.h"

/* header ($30 bytes): MOVEQ #0,d0:RTS, libnum, ..., error at $2C */
static void header(struct buf *b, uint32_t error)
{
    put32(b, 0x70004E75);
    put16(b, 100);                      /* libnum */
    while (b->n < 0x2C)
        put8(b, 0);
    put32(b, error);
}

/* one statement entry: kind 1, one form without arguments, name block */
static void statement(struct buf *b, int lead)
{
    put16(b, 1);
    put32(b, 0);
    put16(b, 0x0000);                   /* form: 0 arguments */
    put16(b, 0);                        /* no libs */
    put32(b, 0x80); put32(b, 0); put32(b, 0);   /* subs */
    if (lead)
        put16(b, 0xFFFF);               /* end of forms = name block */
    put32(b, 0);                        /* next */
    put16(b, 0);                        /* nummer */
    putbytes(b, "Test\0X\0", 8);        /* name, help (even) */
}

/* the library; returns blitzlib_check's result, messages into msg */
static int check(const struct buf *code, uint32_t table_size, uint32_t type,
                 const uint32_t *relocs, uint32_t nrelocs, uint32_t target, char *msg, size_t len)
{
    struct buf f = loadfile1(table_size, type, code, relocs, nrelocs, target);
    struct loadfile lf;
    char err[200];
    FILE *out = tmpfile();
    int r;

    if (load_parse(f.p, f.n, &lf, err, sizeof err))
        abort();
    r = blitzlib_check(&lf, "lib", out);
    rewind(out);
    msg[fread(msg, 1, len - 1, out)] = 0;
    fclose(out);
    load_free(&lf);
    free(f.p);
    return r;
}

/* the reservation of libraries 32 and 33 (#1018): libraries built in
   memory as blitzlib_read leaves them */
static void test_reserved(void)
{
    static struct blitzlib_entry e[127];
    static char names[127][2];
    struct blitzlib lib = { 0 };
    unsigned i;

    for (i = 0; i < 127; i++) {
        names[i][0] = (char)(0x81 + i);
        e[i].kind = 8;
        e[i].name = names[i];
    }
    lib.entries = e;
    lib.nentries = 127;
    /* the literal library: 33, 127 dummy tokens $81-$FF */
    lib.libnum = 33;
    CHECK(blitzlib_is_literallib(&lib));
    CHECK(blitzlib_reserved(&lib, 0) == BLITZLIB_RESERVE_OK);
    /* another library 33 (RIDosLib's statement), with and without the
       exception */
    e[5].kind = 1;
    CHECK(!blitzlib_is_literallib(&lib));
    CHECK(blitzlib_reserved(&lib, 0) == BLITZLIB_RESERVE_33);
    CHECK(blitzlib_reserved(&lib, 1) == BLITZLIB_RESERVE_ALLOWED);
    e[5].kind = 8;
    /* a wrong name, one entry too few: not the literal library */
    names[10][0] = 'x';
    CHECK(blitzlib_reserved(&lib, 0) == BLITZLIB_RESERVE_33);
    names[10][0] = (char)(0x81 + 10);
    lib.nentries = 126;
    CHECK(blitzlib_reserved(&lib, 0) == BLITZLIB_RESERVE_33);
    lib.nentries = 127;
    /* library 32: refused, also with the exception and literallib's entries */
    lib.libnum = 32;
    CHECK(blitzlib_reserved(&lib, 0) == BLITZLIB_RESERVE_32);
    CHECK(blitzlib_reserved(&lib, 1) == BLITZLIB_RESERVE_32);
    /* any other number: ok */
    lib.libnum = 34;
    CHECK(blitzlib_reserved(&lib, 0) == BLITZLIB_RESERVE_OK);
}

int main(void)
{
    struct buf ok = { 0 }, b;
    char msg[1000];
    uint32_t reloc[1];

    header(&ok, 0);
    statement(&ok, 1);
    put16(&ok, 0xFFFF);                 /* end of the entries */
    put32(&ok, 0);                      /* no !libfin */
    put16(&ok, 0x4E75);                 /* code */

    CHECK(check(&ok, 1, HUNK_CODE, NULL, 0, 0, msg, sizeof msg) == 0 && !*msg);

    /* the reader: header, one entry with its names, end of the table */
    {
        struct hunk h = { HUNK_CODE, ok.p, (uint32_t)ok.n, 0, NULL, NULL, 0, NULL, 0, 0 };
        struct blitzlib lib;
        char err[200];

        CHECK(blitzlib_read(&h, &lib, err, sizeof err) == 0);
        CHECK(lib.libnum == 100 && lib.error == 0 && lib.libfin == 0);
        CHECK(lib.nentries == 1 && lib.entries[0].offset == 0x30 && lib.entries[0].kind == 1);
        CHECK(lib.entries[0].name && !strcmp(lib.entries[0].name, "Test"));
        CHECK(lib.entries[0].help && !strcmp(lib.entries[0].help, "X"));
        CHECK(lib.codestart == ok.n - 2);
        blitzlib_free(&lib);
        h.size = 0x20;
        CHECK(blitzlib_read(&h, &lib, err, sizeof err) == -1 && strstr(err, "shorter"));
    }

    /* the table data: a function with two forms (the second with a
       repetition group), an AmigaOS call, an object name and !libfin */
    {
        struct buf t = { 0 };
        struct hunk h = { HUNK_CODE, NULL, 0, 0, NULL, NULL, 0, NULL, 0, 0 };
        struct blitzlib lib;
        const struct blitzlib_entry *e;
        char err[200];
        uint32_t objname;

        header(&t, 0);
        put16(&t, 0x0302); put32(&t, 0);   /* function, result long */
        put16(&t, 0x0002); put8(&t, 2); put8(&t, 7);    /* .w, $ */
        put16(&t, 0);
        put32(&t, 0x80); put32(&t, 0); put32(&t, 0);
        put16(&t, 0x1103); put8(&t, 3); put8(&t, 8); put8(&t, 0x83); put8(&t, 0);
        put16(&t, 0);
        put32(&t, 0x80); put32(&t, 0); put32(&t, 0);
        put16(&t, 0xFFFF); put32(&t, 0); put16(&t, 0);
        putbytes(&t, "Fn\0(a,b)\0", 9); put8(&t, 0);
        put16(&t, 6); put32(&t, 0);         /* AmigaOS call */
        put16(&t, 252); put16(&t, (uint16_t)-30);
        put8(&t, 0x01); put8(&t, 0x10); put8(&t, 0xFF); put8(&t, 0);
        put32(&t, 0); put16(&t, 0);
        putbytes(&t, "Open_\0(n)\0", 10);
        put16(&t, 8); put32(&t, 0);         /* dumtoke */
        put32(&t, 0); put16(&t, 0);
        objname = (uint32_t)t.n;
        putbytes(&t, "Obj\0x\0", 6);
        put16(&t, 0xFFFF);
        put32(&t, objname);                 /* !libfin: toke ... */
        put32(&t, 0); put32(&t, 0); put32(&t, 0); put32(&t, 0);
        put16(&t, 0); put16(&t, 20); put16(&t, 3);
        put16(&t, 0x4E75);
        h.data = t.p;
        h.size = (uint32_t)t.n;

        CHECK(blitzlib_read(&h, &lib, err, sizeof err) == 0);
        CHECK(lib.nentries == 3 && lib.nforms == 2);
        e = &lib.entries[0];
        CHECK(e->kind == 0x0302 && !strcmp(e->name, "Fn") && !strcmp(e->help, "(a,b)"));
        CHECK(e->form == 0 && e->nforms == 2);
        CHECK(lib.forms[0].head == 2 && lib.forms[0].types[0] == 2 && lib.forms[0].types[1] == 7);
        CHECK(lib.forms[1].head == 0x1103 && lib.forms[1].types[2] == 0x83);
        e = &lib.entries[1];
        CHECK(e->kind == 6 && !strcmp(e->name, "Open_") && e->nforms == 0);
        CHECK(e->oslib == 252 && e->lvo == -30 && e->nregs == 2);
        CHECK(e->regs[0] == 0x01 && e->regs[1] == 0x10);
        e = &lib.entries[2];
        CHECK(e->kind == 8 && !strcmp(e->name, "Obj") && !strcmp(e->help, "x"));
        CHECK(lib.object && !strcmp(lib.object, "Obj") && lib.defmax == 20);
        CHECK(lib.codestart == t.n - 2);
        blitzlib_free(&lib);

        /* a toke that is no string: no object */
        {
            uint32_t at = (uint32_t)t.n - 2 - 26;       /* the toke long */

            t.p[at] = 0xFF;
            CHECK(blitzlib_read(&h, &lib, err, sizeof err) == 0 && !lib.object);
            blitzlib_free(&lib);
        }
        free(t.p);
    }

    /* deflibs: 4 zero bytes instead of MOVEQ #0,d0:RTS */
    b = (struct buf){ 0 };
    putbytes(&b, ok.p, ok.n);
    memset(b.p, 0, 4);
    CHECK(check(&b, 1, HUNK_CODE, NULL, 0, 0, msg, sizeof msg) == 0);
    free(b.p);

    /* two hunks in the header */
    CHECK(check(&ok, 2, HUNK_CODE, NULL, 0, 0, msg, sizeof msg) == 1 && strstr(msg, "exactly one"));

    /* data instead of code */
    CHECK(check(&ok, 1, HUNK_DATA, NULL, 0, 0, msg, sizeof msg) == 1 && strstr(msg, "not code"));

    /* relocation into another hunk */
    reloc[0] = 0x16;
    CHECK(check(&ok, 1, HUNK_CODE, reloc, 1, 1, msg, sizeof msg) == 1 && strstr(msg, "into hunk 1"));

    /* wrong start */
    b = (struct buf){ 0 };
    putbytes(&b, ok.p, ok.n);
    b.p[0] = 0x4E;
    CHECK(check(&b, 1, HUNK_CODE, NULL, 0, 0, msg, sizeof msg) == 1 && strstr(msg, "library header"));
    free(b.p);

    /* token table without its end: runs past the code */
    b = (struct buf){ 0 };
    header(&b, 0);
    statement(&b, 1);
    CHECK(check(&b, 1, HUNK_CODE, NULL, 0, 0, msg, sizeof msg) == 1 && strstr(msg, "past the end"));
    free(b.p);

    /* name block without its $FFFF: the next word is read as a form head
       (0), its libs and subs, then the name block ends the code */
    b = (struct buf){ 0 };
    header(&b, 0);
    statement(&b, 0);
    put16(&b, 0xFFFF);
    put32(&b, 0);
    CHECK(check(&b, 1, HUNK_CODE, NULL, 0, 0, msg, sizeof msg) == 1 && strstr(msg, "does not parse"));
    free(b.p);

    /* reference from the code into the error check area */
    b = (struct buf){ 0 };
    header(&b, 0x50);
    put16(&b, 0xFFFF);
    put32(&b, 0);                       /* token table ends at $36 */
    put16(&b, 0x41F9);                  /* $36: lea abs.l,a0 */
    put32(&b, 0x52);                    /* $38: -> $52, error area from $50 */
    while (b.n < 0x54)
        put8(&b, 0);
    reloc[0] = 0x38;
    CHECK(check(&b, 1, HUNK_CODE, reloc, 1, 0, msg, sizeof msg) == 1
          && strstr(msg, "$38 refers to $52"));
    free(b.p);

    free(ok.p);
    test_reserved();
    if (failures)
        fprintf(stderr, "test_blitzlib: %d failures\n", failures);
    return failures != 0;
}
