/*
 * test_blitzres.c - resident files built in memory: the bytes blitzres
 * writes (header, nodes, relocations as blitz2's menu5), reading them
 * back, the dump of a resident with a type, a macro and constants, the
 * constants of FD files and EQU includes, and broken inputs.
 */
#include "testutil.h"
#include "blitzres.h"

static char err[300];

/* the dump of buf as a string (malloc'd) */
static char *dump(const uint8_t *buf, size_t len, int *rc)
{
    FILE *f = tmpfile();
    char *s;
    long n;

    if (!f)
        abort();
    *rc = blitzres_dump(buf, len, f, err, sizeof err);
    n = ftell(f);
    rewind(f);
    s = calloc(1, (size_t)n + 1);
    if (!s || fread(s, 1, (size_t)n, f) != (size_t)n)
        abort();
    fclose(f);
    return s;
}

/* a constant node as blitz2 writes it, with junk in data/procnum/padding */
static void cnode(struct buf *b, uint32_t next, int32_t value, const char *name, uint8_t junk)
{
    size_t len = 19 + strlen(name) + 1;

    put32(b, next);
    put32(b, 1);
    put32(b, (uint32_t)value);
    put32(b, junk ? 0x12345678 : 0);
    put16(b, junk ? 0xABCD : 0);
    put8(b, (uint8_t)len);
    putbytes(b, name, strlen(name) + 1);
    if (len & 1)
        put8(b, junk);
}

static void test_write(void)
{
    struct blitzres_list l = { 0 }, back = { 0 };
    struct buf body = { 0 }, want;
    uint8_t *res;
    size_t len;
    uint32_t relocs[2];
    int rc;
    char *d;

    /* nodes: "ab" 22 bytes at 6, "custom" 26 at 28, "x" 21+1 at 54 */
    CHECK(blitzres_add(&l, "ab", 1, err, sizeof err) == 0);
    CHECK(blitzres_add(&l, "custom", (int32_t)0xdff000, err, sizeof err) == 0);
    CHECK(blitzres_add(&l, "x", -6, err, sizeof err) == 0);
    CHECK(blitzres_write(&l, &res, &len, err, sizeof err) == 0);
    put16(&body, 0);
    put16(&body, 0);
    put16(&body, 3);
    cnode(&body, 28, 1, "ab", 0);
    cnode(&body, 54, (int32_t)0xdff000, "custom", 0);
    cnode(&body, 0, -6, "x", 0);
    /* the offsets in the order blitz2 writes them: the last one first */
    relocs[0] = 28;
    relocs[1] = 6;
    want = loadfile1(1, HUNK_CODE, &body, relocs, 2, 0);
    CHECK(len == want.n && !memcmp(res, want.p, len));
    /* read back (strict), dumped */
    CHECK(blitzres_read_consts(res, len, 1, &back, err, sizeof err) == 0);
    CHECK(back.n == 3 && !strcmp(back.c[1].name, "custom") && back.c[1].value == 0xdff000
          && back.c[2].value == -6);
    d = dump(res, len, &rc);
    CHECK(rc == 0 && !strcmp(d, "const ab=1\nconst custom=14675968\nconst x=-6\n"));
    free(d);
    /* broken: a junk byte (as in a file of blitz2) is not "as written",
       but read leniently it is the same */
    res[32 + 6 + 12] = 0x55;
    blitzres_free(&back);
    CHECK(blitzres_read_consts(res, len, 1, &back, err, sizeof err) == -1);
    blitzres_free(&back);
    CHECK(blitzres_read_consts(res, len, 0, &back, err, sizeof err) == 0 && back.n == 3);
    blitzres_free(&back);
    /* broken: cut off */
    CHECK(blitzres_read_consts(res, len - 20, 0, &back, err, sizeof err) == -1);
    d = dump(res, 40, &rc);
    CHECK(rc == -1);
    free(d);
    free(res);
    free(want.p);
    free(body.p);
    blitzres_free(&back);
    blitzres_free(&l);

    /* one constant: no relocations; none: an empty resident */
    CHECK(blitzres_add(&l, "one", 1, err, sizeof err) == 0);
    CHECK(blitzres_write(&l, &res, &len, err, sizeof err) == 0);
    body = (struct buf){ 0 };
    put16(&body, 0);
    put16(&body, 0);
    put16(&body, 1);
    cnode(&body, 0, 1, "one", 0);
    want = loadfile1(1, HUNK_CODE, &body, NULL, 0, 0);
    CHECK(len == want.n && !memcmp(res, want.p, len));
    CHECK(blitzres_read_consts(res, len, 1, &back, err, sizeof err) == 0 && back.n == 1);
    free(res);
    free(want.p);
    free(body.p);
    blitzres_free(&back);
    blitzres_free(&l);
}

static void test_list(void)
{
    struct blitzres_list l = { 0 };
    uint8_t *res;
    size_t len, i;
    int32_t v;

    CHECK(blitzres_valid_name("_Open") && blitzres_valid_name("color00"));
    CHECK(!blitzres_valid_name("0x") && !blitzres_valid_name("a-b") && !blitzres_valid_name(""));
    CHECK(blitzres_add(&l, "bad name", 1, err, sizeof err) == -1);
    CHECK(blitzres_skip(&l, "bpl1mod", err, sizeof err) == 0);
    CHECK(blitzres_skip(&l, "1x", err, sizeof err) == -1);
    CHECK(blitzres_add(&l, "bpl1mod", 1, err, sizeof err) == 1 && l.n == 0);
    CHECK(blitzres_add(&l, "a", 1, err, sizeof err) == 0);
    CHECK(blitzres_add(&l, "a", 1, err, sizeof err) == 2 && l.n == 1);
    /* broken: a name again with another value */
    CHECK(blitzres_add(&l, "a", 2, err, sizeof err) == -1 && strstr(err, "a defined as 1"));
    CHECK(blitzres_find(&l, "a", &v) == 0 && v == 1);
    CHECK(blitzres_find(&l, "b", &v) == -1);
    blitzres_free(&l);
    /* broken: more constants than the count word holds */
    l.c = calloc(65536, sizeof *l.c);
    if (!l.c)
        abort();
    for (i = 0; i < 65536; i++) {
        char n[16];

        snprintf(n, sizeof n, "c%zu", i);
        if (!(l.c[i].name = strdup(n)))
            abort();
    }
    l.n = l.cap = 65536;
    CHECK(blitzres_write(&l, &res, &len, err, sizeof err) == -1 && strstr(err, "65535"));
    blitzres_free(&l);
}

/* a resident with a NewType (two fields, one an array), a macro and a
   constant, as blitz2 writes it (the pointers are offsets) */
static void test_dump(void)
{
    struct buf body = { 0 }, f;
    struct blitzres_list l = { 0 };
    uint32_t relocs[3];
    char *d;
    int rc;

    put16(&body, 1);                    /* one type */
    /* fields: next.l offset.w flags.w ndims.w type.l len.b name (15)
       field "_a" at 2: next -> 20, offset 0, flags 0, ndims junk, type 2 */
    put32(&body, 20);
    put16(&body, 0);
    put16(&body, 0);
    put16(&body, 0x70F);
    put16(&body, 2);                    /* type: the index in the high word */
    put16(&body, 0x1234);
    put8(&body, 18);
    putbytes(&body, "_a", 3);           /* 15+3 = 18 */
    /* field "_b" at 20: last, offset 2, array of 4 bytes, type 1 */
    put32(&body, 0);
    put16(&body, 2);
    put16(&body, 2);
    put16(&body, 4);
    put16(&body, 1);
    put16(&body, 0);
    put8(&body, 18);
    putbytes(&body, "_b", 3);
    /* type "pt" at 38: fields -> 2, size 6 */
    put32(&body, 0);
    put32(&body, 2);
    put16(&body, 6);
    put8(&body, 14);
    putbytes(&body, "pt", 3);           /* 11+3 = 14 */
    put16(&body, 1);                    /* one macro, at 54 */
    put32(&body, 0);
    put32(&body, 72);                   /* text */
    put16(&body, 3);
    put16(&body, 0);
    put8(&body, 17);
    putbytes(&body, "mac", 4);          /* 13+4 = 17, padded */
    put8(&body, 0);
    putbytes(&body, "a\"\x80", 3);      /* text, padded */
    put8(&body, 0);
    put16(&body, 1);                    /* one constant */
    cnode(&body, 0, 7, "seven", 1);
    relocs[0] = 2;
    relocs[1] = 42;
    relocs[2] = 58;
    f = loadfile1(1, HUNK_CODE, &body, relocs, 3, 0);
    d = dump(f.p, f.n, &rc);
    if (rc)
        fprintf(stderr, "dump: %s\n", err);
    CHECK(rc == 0);
    CHECK(!strcmp(d, "type pt size=6\n"
                     "  field _a offset=0 flags=0 type=2\n"
                     "  field _b offset=2 flags=2 ndims=4 type=1\n"
                     "macro mac \"a\\x22\\x80\"\n"
                     "const seven=7\n"));
    free(d);
    /* only constants: a resident with types is an error */
    CHECK(blitzres_read_consts(f.p, f.n, 0, &l, err, sizeof err) == -1);
    free(f.p);
    /* broken: the field list points back */
    body.p[2 + 3] = 2;
    f = loadfile1(1, HUNK_CODE, &body, relocs, 3, 0);
    d = dump(f.p, f.n, &rc);
    CHECK(rc == -1 && strstr(err, "goes back"));
    free(d);
    free(f.p);
    free(body.p);
    /* broken: not a load file, two hunks */
    {
        uint8_t junk[8] = { 1, 2, 3, 4 };

        d = dump(junk, sizeof junk, &rc);
        CHECK(rc == -1);
        free(d);
    }
    blitzres_free(&l);
}

static void test_fd(void)
{
    static const char fd[] =
        "* \"datatypes.library\"\n"
        "##base _DataTypesBase\n"
        "##bias 30\n"
        "##private\n"
        "Reserved()()\n"
        "##public\n"
        "ObtainDataTypeA(type,handle,attrs)(d0/a0/a1)\n"
        "  SetDTAttrsA(o,win,req,attrs)(a0/a1/a2/a3)\n"
        "GetDTAttrsA(o,attrs)(a0/a2)\n"
        "GetDTAttrs(o,attrs)(a0/a2)\n"
        "##end\n";
    struct blitzres_list l = { 0 };

    CHECK(blitzres_fd(fd, strlen(fd), 0, &l, err, sizeof err) == 0);
    CHECK(l.n == 4);
    CHECK(!strcmp(l.c[0].name, "_ObtainDataTypeA") && l.c[0].value == -36);
    CHECK(!strcmp(l.c[1].name, "_SetDTAttrsA") && l.c[1].value == -42);
    CHECK(l.c[3].value == -54);
    blitzres_free(&l);
    /* varargs: every NAMEA without NAME in the file, behind them:
       _ObtainDataType, _SetDTAttrs, not _GetDTAttrs again */
    CHECK(blitzres_fd(fd, strlen(fd), 1, &l, err, sizeof err) == 0);
    CHECK(l.n == 6 && !strcmp(l.c[4].name, "_ObtainDataType") && l.c[4].value == -36);
    CHECK(!strcmp(l.c[5].name, "_SetDTAttrs") && l.c[5].value == -42);
    /* a second file with the same name and offset: counted once */
    CHECK(blitzres_fd(fd, strlen(fd), 0, &l, err, sizeof err) == 0 && l.n == 6);
    blitzres_free(&l);
    /* skipped names still count for the bias */
    CHECK(blitzres_skip(&l, "_ObtainDataTypeA", err, sizeof err) == 0);
    CHECK(blitzres_fd(fd, strlen(fd), 0, &l, err, sizeof err) == 0);
    CHECK(l.n == 3 && l.c[0].value == -42);
    blitzres_free(&l);
    /* broken: a function before ##bias, a name with two offsets */
    CHECK(blitzres_fd("Open(a)(d0)\n", 12, 0, &l, err, sizeof err) == -1
          && strstr(err, "before ##bias"));
    blitzres_free(&l);
    {
        static const char two[] = "##bias 30\nOpen()()\nOpen()()\n";

        CHECK(blitzres_fd(two, strlen(two), 0, &l, err, sizeof err) == -1);
        blitzres_free(&l);
    }
}

static void test_equ(void)
{
    static const char inc[] =
        "    IFND HARDWARE_CUSTOM_I\n"
        "HARDWARE_CUSTOM_I SET 1\n"
        "bltddat     EQU   $000\n"
        "dmaconr\tEQU\t$002 ; comment\n"
        "DTA_Dummy\tequ\t(TAG_USER+$1000)\n"
        "DTA_Name\tequ\t(DTA_Dummy + 2)\n"
        "POBAB_X equ 3\n"
        "POBAF_X EQU (1<<POBAB_X)\n"
        "ID_ILBM\tequ\t'ILBM'\n"
        "MASK\tequ\t0xff000000\n"
        "EXPR\tequ\t-2*3+(~0&$f0)|1^3\n"
        "TEXT\tequ\t'text.datatype'\n"
        "UNKNOWN\tequ\tNOSUCH+1\n"
        "OCTAL\tequ\t010\n"
        "EQUR\tEQUR\td0\n"
        "label: dc.l 0\n"
        "  indented EQU 5\n"
        "AGAIN\tSET 1\n"
        "AGAIN\tSET 1\n";
    struct blitzres_list syms = { 0 }, l = { 0 };
    int32_t v;

    CHECK(blitzres_add(&syms, "TAG_USER", INT32_MIN, err, sizeof err) == 0);
    CHECK(blitzres_equ(inc, strlen(inc), &syms, &l, err, sizeof err) == 0);
    CHECK(l.n == 10);
    CHECK(!strcmp(l.c[0].name, "bltddat") && l.c[0].value == 0);
    CHECK(!strcmp(l.c[1].name, "dmaconr") && l.c[1].value == 2);
    CHECK(blitzres_find(&l, "DTA_Dummy", &v) == 0 && v == INT32_MIN + 0x1000);
    CHECK(blitzres_find(&l, "DTA_Name", &v) == 0 && v == INT32_MIN + 0x1002);
    CHECK(blitzres_find(&l, "POBAF_X", &v) == 0 && v == 8);
    CHECK(blitzres_find(&l, "ID_ILBM", &v) == 0 && v == 0x494C424D);
    CHECK(blitzres_find(&l, "MASK", &v) == 0 && v == (int32_t)0xff000000);
    CHECK(blitzres_find(&l, "EXPR", &v) == 0 && v == ((-2 * 3 + (~0 & 0xf0)) | (1 ^ 3)));
    CHECK(blitzres_find(&l, "AGAIN", &v) == 0 && v == 1);
    /* left out: the include guard, strings, unknown symbols, a decimal
       with a leading 0 (no number in Python), EQUR, indented lines */
    CHECK(blitzres_find(&l, "HARDWARE_CUSTOM_I", &v) == -1);
    CHECK(blitzres_find(&l, "TEXT", &v) == -1 && blitzres_find(&l, "UNKNOWN", &v) == -1);
    CHECK(blitzres_find(&l, "OCTAL", &v) == -1 && blitzres_find(&l, "EQUR", &v) == -1);
    CHECK(blitzres_find(&l, "indented", &v) == -1);
    /* every value is a symbol for later lines */
    CHECK(blitzres_find(&syms, "POBAB_X", &v) == 0 && v == 3);
    blitzres_free(&l);
    /* broken: a constant again with another value */
    CHECK(blitzres_equ("A EQU 1\nA EQU 2\n", 16, &syms, &l, err, sizeof err) == -1
          && strstr(err, "A defined as 1"));
    blitzres_free(&l);
    blitzres_free(&syms);
}

static void test_lvo(void)
{
    static const char inc[] =
        "* Automatically generated header (sfdc 1.13)! Do not edit!\n"
        "     IFND  LVO_AHI_LIB_I\n"
        "LVO_AHI_LIB_I   SET   1\n"
        "\n"
        "_LVOAHI_AllocAudioA     EQU   -42\n"
        "_LVOAHI_FreeAudio     EQU   -48\n"
        "OTHER\tEQU\t5\n"
        "_LVOAHI_AudioRequestA     EQU   -126\n"
        "     ENDC  * LVO_AHI_LIB_I\n";
    struct blitzres_list l = { 0 };

    CHECK(blitzres_lvo(inc, strlen(inc), &l, err, sizeof err) == 0);
    CHECK(l.n == 3);
    CHECK(!strcmp(l.c[0].name, "AHI_AllocAudioA") && l.c[0].value == -42);
    CHECK(!strcmp(l.c[1].name, "AHI_FreeAudio") && l.c[1].value == -48);
    CHECK(!strcmp(l.c[2].name, "AHI_AudioRequestA") && l.c[2].value == -126);
    blitzres_free(&l);
    /* skipped names are left out */
    CHECK(blitzres_skip(&l, "AHI_FreeAudio", err, sizeof err) == 0);
    CHECK(blitzres_lvo(inc, strlen(inc), &l, err, sizeof err) == 0 && l.n == 2);
    blitzres_free(&l);
    /* broken: no _LVO line, an offset not below 0, a name with two offsets */
    CHECK(blitzres_lvo("A EQU 1\n", 8, &l, err, sizeof err) == -1 && strstr(err, "no _LVO"));
    blitzres_free(&l);
    CHECK(blitzres_lvo("_LVOX EQU 6\n", 12, &l, err, sizeof err) == -1
          && strstr(err, "not negative"));
    blitzres_free(&l);
    CHECK(blitzres_add(&l, "AHI_FreeAudio", -6, err, sizeof err) == 0);
    CHECK(blitzres_lvo(inc, strlen(inc), &l, err, sizeof err) == -1
          && strstr(err, "AHI_FreeAudio defined as -6"));
    blitzres_free(&l);
}

int main(void)
{
    test_write();
    test_list();
    test_dump();
    test_fd();
    test_equ();
    test_lvo();
    if (failures)
        fprintf(stderr, "%d check(s) failed\n", failures);
    return failures != 0;
}
