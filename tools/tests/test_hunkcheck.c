/*
 * test_hunkcheck.c - hunk_check: match, mismatch, first hunk not code,
 * check behind the hunk, bad spec; hunk_layout: match, another type, a
 * missing chip flag, too few and too many hunks, bad layout; hunk_stored:
 * hunks stored whole, a data and a code hunk with an unstored tail.
 */
#include "testutil.h"
#include "hunkcheck.h"

static int check(uint32_t type, int n, char **specs, char *msg, size_t len)
{
    struct buf code = { 0 }, f;
    struct loadfile lf;
    char err[200];
    FILE *out = tmpfile();
    int r;

    put32(&code, 0x61000010);           /* bsr.w */
    put32(&code, 0x48E7FFFE);           /* movem.l d0-d7/a0-a6,-(sp) */
    f = loadfile1(1, type, &code, NULL, 0, 0);
    if (load_parse(f.p, f.n, &lf, err, sizeof err))
        abort();
    r = hunk_check(&lf, n, specs, "ted", out);
    rewind(out);
    msg[fread(msg, 1, len - 1, out)] = 0;
    fclose(out);
    load_free(&lf);
    free(f.p);
    free(code.p);
    return r;
}

/* a load file of three hunks: code, bss, data in chip memory */
static int layout(const char *want, char *msg, size_t len)
{
    struct buf f = { 0 };
    struct loadfile lf;
    char err[200];
    FILE *out = tmpfile();
    int r;

    put32(&f, HUNK_HEADER);
    put32(&f, 0);
    put32(&f, 3);
    put32(&f, 0);
    put32(&f, 2);
    put32(&f, 1);
    put32(&f, 2);
    put32(&f, 1 | HUNKF_CHIP);
    put32(&f, HUNK_CODE);
    put32(&f, 1);
    put32(&f, 0x4e754e75);
    put32(&f, HUNK_END);
    put32(&f, HUNK_BSS);
    put32(&f, 2);
    put32(&f, HUNK_END);
    put32(&f, HUNK_DATA);
    put32(&f, 1);
    put32(&f, 0x00010000);
    put32(&f, HUNK_END);
    if (load_parse(f.p, f.n, &lf, err, sizeof err))
        abort();
    r = hunk_layout(&lf, want, "exe", out);
    rewind(out);
    msg[fread(msg, 1, len - 1, out)] = 0;
    fclose(out);
    load_free(&lf);
    free(f.p);
    return r;
}

/* hunk_stored: code (1 long stored) and data with header sizes
   code_size/data_size in longs and 1 stored long; a bss hunk of 4 longs
   (never stored) after them */
static int stored(uint32_t code_size, uint32_t data_size, char *msg, size_t len)
{
    struct buf f = { 0 };
    struct loadfile lf;
    char err[200];
    FILE *out = tmpfile();
    int r;

    put32(&f, HUNK_HEADER);
    put32(&f, 0);
    put32(&f, 3);
    put32(&f, 0);
    put32(&f, 2);
    put32(&f, code_size);
    put32(&f, data_size);
    put32(&f, 4);
    put32(&f, HUNK_CODE);
    put32(&f, 1);
    put32(&f, 0x4e754e75);
    put32(&f, HUNK_END);
    put32(&f, HUNK_DATA);
    put32(&f, 1);
    put32(&f, 0x00010000);
    put32(&f, HUNK_END);
    put32(&f, HUNK_BSS);
    put32(&f, 4);
    put32(&f, HUNK_END);
    if (load_parse(f.p, f.n, &lf, err, sizeof err))
        abort();
    r = hunk_stored(&lf, "ted", out);
    rewind(out);
    msg[fread(msg, 1, len - 1, out)] = 0;
    fclose(out);
    load_free(&lf);
    free(f.p);
    return r;
}

int main(void)
{
    char msg[500];
    char *ok[] = { "0:6100", "4:48e7", "0x4:48E7FFFE" };
    char *wrong[] = { "4:4e75" };
    char *behind[] = { "6:48e7fffe" };
    char *bad1[] = { "4:48e" };
    char *bad2[] = { "4:zz" };

    CHECK(check(HUNK_CODE, 3, ok, msg, sizeof msg) == 0 && !*msg);
    CHECK(check(HUNK_CODE, 1, wrong, msg, sizeof msg) == 1 && strstr(msg, "hunk+4 is 48e7, expected 4e75"));
    CHECK(check(HUNK_DATA, 3, ok, msg, sizeof msg) == 1 && strstr(msg, "not code"));
    CHECK(check(HUNK_CODE, 1, behind, msg, sizeof msg) == 1 && strstr(msg, "only 8 bytes"));
    CHECK(check(HUNK_CODE, 1, bad1, msg, sizeof msg) == 1 && strstr(msg, "bad check"));
    CHECK(check(HUNK_CODE, 1, bad2, msg, sizeof msg) == 1 && strstr(msg, "bad check"));

    CHECK(layout("code,bss,data+chip", msg, sizeof msg) == 0 && !*msg);
    CHECK(layout("code,data,data+chip", msg, sizeof msg) == 1
          && strstr(msg, "hunk 1 is bss, expected data"));
    CHECK(layout("code,bss,data", msg, sizeof msg) == 1
          && strstr(msg, "hunk 2 is data+chip, expected data"));
    CHECK(layout("code,bss", msg, sizeof msg) == 1 && strstr(msg, "3 hunks, the layout \"code,bss\" has 2"));
    CHECK(layout("code,bss,data+chip,bss", msg, sizeof msg) == 1 && strstr(msg, "has more"));
    CHECK(layout("code,text", msg, sizeof msg) == 1 && strstr(msg, "bad layout"));
    CHECK(layout("code,bss+fast,data+chip", msg, sizeof msg) == 1 && strstr(msg, "bad layout"));

    /* stored whole; the bss hunk's 4 longs are no tail */
    CHECK(stored(1, 1, msg, sizeof msg) == 0 && !*msg);
    /* a data hunk of $10 bytes with 4 stored: the .bss of -fbaserel */
    CHECK(stored(1, 4, msg, sizeof msg) == 1
          && strstr(msg, "ted: hunk 1 (data) has $10 bytes, only $4 stored")
          && !strstr(msg, "hunk 0"));
    /* a code hunk with a tail, too */
    CHECK(stored(2, 1, msg, sizeof msg) == 1
          && strstr(msg, "ted: hunk 0 (code) has $8 bytes, only $4 stored")
          && !strstr(msg, "hunk 1"));

    if (failures)
        fprintf(stderr, "test_hunkcheck: %d failures\n", failures);
    return failures != 0;
}
