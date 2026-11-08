/*
 * test_blitzlib_compare.c - blitzlib_compare: what must be equal
 * (header, names, structure, set code offsets, relocations of the
 * declarations), what may differ (code offsets, run time fields, code).
 */
#include "testutil.h"
#include "blitzlib.h"

struct variant {
    uint16_t libnum;
    uint32_t sub;               /* code offset of the form */
    uint32_t next;              /* next.l of the name block (run time) */
    const char *name;           /* 4 characters */
    int entries;                /* 1 or 2 */
    uint16_t code;              /* the code behind the table */
};

static const struct variant base = { 100, 0x80, 0, "Test", 1, 0x4E75 };

static struct buf build(const struct variant *v)
{
    struct buf b = { 0 };
    int i;

    put32(&b, 0x70004E75);
    put16(&b, v->libnum);
    while (b.n < 0x30)
        put8(&b, 0);
    for (i = 0; i < v->entries; i++) {
        put16(&b, 1);                   /* statement */
        put32(&b, 0);
        put16(&b, 0x0000);              /* form: 0 arguments */
        put16(&b, 0);                   /* no libs */
        put32(&b, v->sub); put32(&b, 0); put32(&b, 0);
        put16(&b, 0xFFFF);
        put32(&b, v->next);
        put16(&b, 0);                   /* nummer */
        putbytes(&b, v->name, 4);
        putbytes(&b, "\0X\0", 4);       /* help, even */
    }
    put16(&b, 0xFFFF);
    put32(&b, 0);                       /* no !libfin */
    put16(&b, v->code);
    return b;
}

/* compares orig with v; relocs: one at the form's sub (if set) plus
   extra ones; messages into msg */
static int cmp(const struct variant *v, uint32_t extra_reloc, char *msg, size_t len)
{
    struct buf a = build(&base), b = build(v);
    uint32_t ra[1] = { 0x30 + 10 }, rb[2] = { 0x30 + 10, extra_reloc };
    struct hunk ha = { HUNK_CODE, a.p, (uint32_t)a.n, 1, ra, NULL, 0, NULL, 0, 0 };
    struct hunk hb = { HUNK_CODE, b.p, (uint32_t)b.n, extra_reloc ? 2 : (v->sub ? 1 : 0), rb, NULL, 0, NULL, 0, 0 };
    FILE *out = tmpfile();
    int r;

    r = blitzlib_compare(&ha, &hb, "lib", out);
    rewind(out);
    msg[fread(msg, 1, len - 1, out)] = 0;
    fclose(out);
    free(a.p);
    free(b.p);
    return r;
}

int main(void)
{
    struct variant v;
    char msg[1000];

    /* the same */
    CHECK(cmp(&base, 0, msg, sizeof msg) == 0 && !*msg);
    /* allowed: another code offset, run time field, other code */
    v = base;
    v.sub = 0x200;
    v.next = 0x12345678;
    v.code = 0x4E71;
    CHECK(cmp(&v, 0, msg, sizeof msg) == 0 && !*msg);

    /* broken: code offset only in one of them (and its relocation) */
    v = base;
    v.sub = 0;
    CHECK(cmp(&v, 0, msg, sizeof msg) == 2);
    CHECK(strstr(msg, "lib: entry 0 Test +$A: code offset set in only one of them\n") != NULL);
    CHECK(strstr(msg, "relocations in the declaration part 1 != 0") != NULL);
    /* broken: the name */
    v = base;
    v.name = "Tast";
    CHECK(cmp(&v, 0, msg, sizeof msg) == 1);
    CHECK(strstr(msg, "lib: entry 0 Test +$1F: $65 != $61\n") != NULL);
    /* broken: the header */
    v = base;
    v.libnum = 101;
    CHECK(cmp(&v, 0, msg, sizeof msg) == 1);
    CHECK(strstr(msg, "lib: header +$05: $64 != $65\n") != NULL);
    /* broken: one entry more */
    v = base;
    v.entries = 2;
    CHECK(cmp(&v, 0, msg, sizeof msg) >= 2);
    CHECK(strstr(msg, "1 entries != 2") != NULL);
    CHECK(strstr(msg, "declaration part") != NULL);
    /* broken: an extra relocation in the declarations (the help text) */
    CHECK(cmp(&base, 0x30 + 0x1C, msg, sizeof msg) == 1);
    CHECK(strstr(msg, "relocations in the declaration part 1 != 2") != NULL);
    /* a relocation in the code does not count */
    CHECK(cmp(&base, 0x30 + 0x2C, msg, sizeof msg) == 0);
    /* a library that does not read */
    {
        struct buf a = build(&base);
        uint8_t shorty[8] = { 0 };
        struct hunk ha = { HUNK_CODE, a.p, (uint32_t)a.n, 0, NULL, NULL, 0, NULL, 0, 0 };
        struct hunk hb = { HUNK_CODE, shorty, 8, 0, NULL, NULL, 0, NULL, 0, 0 };
        FILE *out = tmpfile();

        CHECK(blitzlib_compare(&ha, &hb, "lib", out) == -1);
        fclose(out);
        free(a.p);
    }

    /* the list of intended differences: comments and empty lines skipped */
    {
        char text[] = "# fixed (#1)\nlib: header +$05: $64 != $65\n\n#\nlib: x\n";
        char text2[] = "a\nb\nc";
        const char *lines[2];

        CHECK(blitzlib_intended_list(text, lines, 2) == 2);
        CHECK(!strcmp(lines[0], "lib: header +$05: $64 != $65") && !strcmp(lines[1], "lib: x"));
        CHECK(blitzlib_intended_list(text2, lines, 2) == -1);       /* too many */
    }
    /* intended differences: not counted, marked as seen */
    {
        static const char *const intended[2] = {
            "lib: header +$05: $64 != $65", "lib: entry 0 Test +$1F: $65 != $61" };
        unsigned char seen[2] = { 0 };
        FILE *out = tmpfile();

        v = base;
        v.libnum = 101;
        CHECK(cmp(&v, 0, msg, sizeof msg) == 1);
        CHECK(blitzlib_intended(msg, intended, 2, seen, out) == 0);
        CHECK(seen[0] == 1 && seen[1] == 0);            /* the second one is stale */
        rewind(out);
        msg[fread(msg, 1, sizeof msg - 1, out)] = 0;
        CHECK(!strcmp(msg, "intended: lib: header +$05: $64 != $65\n"));
        fclose(out);
        /* broken: a difference that is not in the list is counted */
        v.name = "Tast";
        CHECK(cmp(&v, 0, msg, sizeof msg) == 2);
        seen[0] = seen[1] = 0;
        out = tmpfile();
        CHECK(blitzlib_intended(msg, intended + 1, 1, seen, out) == 1);
        CHECK(seen[0] == 1);
        fclose(out);
        /* a line must match as a whole, not as a prefix */
        seen[0] = 0;
        out = tmpfile();
        CHECK(blitzlib_intended("lib: header +$05: $64 != $650\n", intended, 1, seen, out) == 1);
        CHECK(seen[0] == 0);
        fclose(out);
    }
    /* a whole list as blitzlib compare uses it (libs/GROUP/declarations-
       intended.txt, libs/userlibs/rwe/NAME.declarations-intended.txt):
       correct, with a stale entry, with an entry missing */
    {
        char correct[] = "# #959: d0, not d1\n"
                         "lib: header +$05: $64 != $65\n"
                         "lib: entry 0 Test +$1F: $65 != $61\n";
        char stale[] = "lib: header +$05: $64 != $65\n"
                       "lib: entry 0 Test +$1F: $65 != $61\n"
                       "lib: entry 1 Gone +$0C: $01 != $00\n";
        char missing[] = "lib: header +$05: $64 != $65\n";
        const char *lines[4];
        unsigned char seen[4];
        long n;
        FILE *out;

        v = base;
        v.libnum = 101;
        v.name = "Tast";
        CHECK(cmp(&v, 0, msg, sizeof msg) == 2);
        n = blitzlib_intended_list(correct, lines, 4);
        CHECK(n == 2);
        memset(seen, 0, sizeof seen);
        out = tmpfile();
        CHECK(blitzlib_intended(msg, lines, (size_t)n, seen, out) == 0);
        CHECK(blitzlib_intended_stale(lines, (size_t)n, seen, out) == 0);
        fclose(out);
        /* broken: an entry that does not occur */
        n = blitzlib_intended_list(stale, lines, 4);
        CHECK(n == 3);
        memset(seen, 0, sizeof seen);
        out = tmpfile();
        CHECK(blitzlib_intended(msg, lines, (size_t)n, seen, out) == 0);
        CHECK(blitzlib_intended_stale(lines, (size_t)n, seen, out) == 1);
        rewind(out);
        msg[fread(msg, 1, sizeof msg - 1, out)] = 0;
        CHECK(strstr(msg, "intended difference not found: lib: entry 1 Gone +$0C: $01 != $00\n")
              != NULL);
        fclose(out);
        /* broken: a difference the list does not name */
        CHECK(cmp(&v, 0, msg, sizeof msg) == 2);
        n = blitzlib_intended_list(missing, lines, 4);
        CHECK(n == 1);
        memset(seen, 0, sizeof seen);
        out = tmpfile();
        CHECK(blitzlib_intended(msg, lines, (size_t)n, seen, out) == 1);
        CHECK(blitzlib_intended_stale(lines, (size_t)n, seen, out) == 0);
        fclose(out);
    }
    return failures != 0;
}
