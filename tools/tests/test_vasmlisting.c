/*
 * test_vasmlisting.c - reading a vasm listing and its views, on a small
 * listing written here.
 */
#include "testutil.h"
#include "vasmlisting.h"

static const char listing_text[] =
    "Sections:\n"
    "00: \"CODE\" (0-C)\n"
    "\n"
    "Source: \"t.s\"\n"
    "                        \t     1: ; comment\n"
    "00:00000000 4E71        \t     2: start\tnop\n"
    "00:00000002 41F900000000\t     3: \tlea\tdata,a0   \n"
    "00:00000008 4E75        \t     4: Data\trts\n"
    "00:0000000A             \t     5: .loop\n"
    "\n"
    "Symbols by name:\n"
    "data                            00:00000008\n"
    "start                           00:00000000\n"
    ".loop                           00:0000000A\n"
    "Symbols by value:\n"
    "00000000 start\n";

/* runs a view into a string */
static char *view(const struct listing *l, int which, const struct hunk *h, size_t *bad)
{
    static char buf[2000];
    FILE *f = tmpfile();
    size_t n;

    if (which == 0)
        listing_print_map(l, f);
    else if (which == 1)
        listing_print_labels(l, f);
    else
        *bad = listing_verify(l, h, f);
    rewind(f);
    n = fread(buf, 1, sizeof buf - 1, f);
    buf[n] = 0;
    fclose(f);
    return buf;
}

int main(void)
{
    struct listing l;
    char err[200];
    FILE *f = tmpfile();
    uint8_t code[12] = { 0x4E, 0x71, 0x41, 0xF9, 0, 0, 0, 8, 0x4E, 0x75, 0, 0 };
    uint32_t reloc = 4, target = 0;
    struct hunk h = { HUNK_CODE, code, sizeof code, 1, &reloc, &target, 0, NULL, 0, 0 };
    size_t bad = 99;
    const char *s;

    fputs(listing_text, f);
    rewind(f);
    CHECK(listing_read_stream(f, &l, err, sizeof err) == 0);
    fclose(f);

    /* rows with an address, the file, the symbols in listing order */
    CHECK(l.nrows == 4);
    CHECK(l.nsyms == 3 && !strcmp(l.syms[0].name, "data") && l.syms[0].value == 8);
    CHECK(l.rows[1].addr == 2 && l.rows[1].nbytes == 6 && l.rows[1].bytes[1] == 0xF9);
    CHECK(!strcmp(l.rows[1].file, "t.s") && l.rows[1].line == 3);
    CHECK(l.rows[3].nbytes == 0);
    /* definition rows: case insensitive, local labels have none */
    CHECK(listing_defrow(&l, "data") == &l.rows[2]);
    CHECK(listing_defrow(&l, "start") == &l.rows[0]);
    CHECK(listing_defrow(&l, ".loop") == NULL);

    s = view(&l, 0, NULL, NULL);
    CHECK(!strcmp(s, "000000\tt.s:2\t4e71\tstart\tnop\n"
                     "000002\tt.s:3\t41f900000000\tlea\tdata,a0\n"
                     "000008\tt.s:4\t4e75\tData\trts\n"));
    s = view(&l, 1, NULL, NULL);
    CHECK(!strcmp(s, "000000\tstart\tt.s:2\n"
                     "000008\tdata\tt.s:4\n"
                     "00000A\t.loop\t?:0\n"));

    /* verify: the relocated long (0 in the listing, 8 in the code) is skipped */
    s = view(&l, 2, &h, &bad);
    CHECK(bad == 0 && !*s);
    code[9] = 0x71;                     /* rts -> nop: row 4 differs */
    s = view(&l, 2, &h, &bad);
    CHECK(bad == 1 && !strcmp(s, "diff 000009 t.s:4 Data\trts\n"));
    h.size = 8;                         /* code shorter than the listing */
    s = view(&l, 2, &h, &bad);
    CHECK(bad == 1);

    listing_free(&l);
    CHECK(listing_read("/nonexistent/x.lst", &l, err, sizeof err) == -1);
    if (failures)
        fprintf(stderr, "test_vasmlisting: %d failures\n", failures);
    return failures != 0;
}
