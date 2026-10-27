/*
 * test_hunkcheck.c - hunk_check: match, mismatch, first hunk not code,
 * check behind the hunk, bad spec.
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

    if (failures)
        fprintf(stderr, "test_hunkcheck: %d failures\n", failures);
    return failures != 0;
}
