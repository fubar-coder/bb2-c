/*
 * test_amiga_emulation_cov.c - collecting the coverage files of a session
 * (amiga_emulation.h, "Coverage"): the merge of two files, broken files,
 * and the move from TEST:cov into the test's directory (no emulator).
 */
#define _XOPEN_SOURCE 700       /* mkdtemp */
#include <sys/stat.h>
#include <unistd.h>
#include "testutil.h"
#include "amiga_emulation.h"
#include "covfmt.h"

/* a coverage file for a code hunk of `size` bytes, bits from `bits` (one
   byte repeated) */
static size_t make_cov(unsigned char *p, uint32_t size, unsigned char bits)
{
    uint32_t len = COVFMT_BITMAP_LEN(size);
    const uint32_t v[4] = { COVFMT_MAGIC, COVFMT_VERSION, size, len };

    for (int i = 0; i < 4; i++) {
        p[4 * i] = (unsigned char)(v[i] >> 24);
        p[1 + 4 * i] = (unsigned char)(v[i] >> 16);
        p[2 + 4 * i] = (unsigned char)(v[i] >> 8);
        p[3 + 4 * i] = (unsigned char)v[i];
    }
    memset(p + COVFMT_HEADER_SIZE, bits, len);
    return COVFMT_HEADER_SIZE + len;
}

static void write_bytes(const char *path, const void *p, size_t n)
{
    FILE *f = fopen(path, "wb");

    CHECK(f != NULL);
    if (f) {
        CHECK(fwrite(p, 1, n, f) == n);
        fclose(f);
    }
}

/* the file's bytes into p (at most cap), its size; -1 if missing */
static long read_bytes(const char *path, unsigned char *p, size_t cap)
{
    FILE *f = fopen(path, "rb");
    size_t n;

    if (!f)
        return -1;
    n = fread(p, 1, cap, f);
    fclose(f);
    return (long)n;
}

static void test_merge(void)
{
    unsigned char a[64], b[64], c[64];
    size_t na = make_cov(a, 64, 0x01), nb = make_cov(b, 64, 0x80), nc;

    /* 64 bytes of code: 32 bits, 4 bytes of bitmap */
    CHECK(na == COVFMT_HEADER_SIZE + 4);
    /* no old file, then the same build: OR-ed */
    CHECK(amiga_emulation_cov_merge(a, na, NULL, 0) == 0);
    CHECK(amiga_emulation_cov_merge(a, na, b, nb) == 1);
    CHECK(a[COVFMT_HEADER_SIZE] == 0x81 && a[na - 1] == 0x81);
    /* another build (code size): the new file stays as it is */
    nc = make_cov(c, 80, 0xff);
    na = make_cov(a, 64, 0x01);
    CHECK(amiga_emulation_cov_merge(a, na, c, nc) == 0);
    CHECK(a[COVFMT_HEADER_SIZE] == 0x01);
    /* an old file that is none: ignored */
    memcpy(b, "XCOV", 4);
    CHECK(amiga_emulation_cov_merge(a, na, b, nb) == 0);
    CHECK(amiga_emulation_cov_merge(a, na, b, 3) == 0);

    /* broken new files */
    na = make_cov(a, 64, 0x01);
    CHECK(amiga_emulation_cov_merge(a, na - 1, NULL, 0) == -1);    /* short */
    CHECK(amiga_emulation_cov_merge(a, COVFMT_HEADER_SIZE - 1, NULL, 0) == -1);
    a[7] = COVFMT_VERSION + 1;                                      /* version */
    CHECK(amiga_emulation_cov_merge(a, na, NULL, 0) == -1);
    na = make_cov(a, 64, 0x01);
    a[15] = 5;                                                      /* length */
    CHECK(amiga_emulation_cov_merge(a, na, NULL, 0) == -1);
    na = make_cov(a, 64, 0x01);
    a[0] = 0x58;                                                    /* magic */
    CHECK(amiga_emulation_cov_merge(a, na, NULL, 0) == -1);
}

static void test_collect(void)
{
    char base[] = "/tmp/test_amiga_emulation_cov-XXXXXX";
    char test[96], cov[128], dest[128], path[192];
    unsigned char a[64], b[64], got[64];
    size_t na, nb;
    char err[256];

    CHECK(mkdtemp(base) != NULL);
    snprintf(test, sizeof test, "%s/test", base);
    snprintf(cov, sizeof cov, "%s/cov", test);
    snprintf(dest, sizeof dest, "%s/coverage/some_test", base);
    CHECK(mkdir(test, 0777) == 0);

    /* nothing prepared: nothing to collect */
    CHECK(amiga_emulation_cov_collect(test, dest, err, sizeof err) == 0);
    /* a session without coverage files: TEST:cov removed again */
    CHECK(amiga_emulation_cov_prepare(test) == 0);
    CHECK(amiga_emulation_cov_prepare(test) == 0);
    CHECK(amiga_emulation_cov_collect(test, dest, err, sizeof err) == 0);
    CHECK(access(cov, F_OK) != 0);

    /* first session: moved */
    CHECK(amiga_emulation_cov_prepare(test) == 0);
    na = make_cov(a, 64, 0x01);
    snprintf(path, sizeof path, "%s/blitz2.cov", cov);
    write_bytes(path, a, na);
    CHECK(amiga_emulation_cov_collect(test, dest, err, sizeof err) == 1);
    CHECK(access(path, F_OK) != 0 && access(cov, F_OK) != 0);
    snprintf(path, sizeof path, "%s/blitz2.cov", dest);
    CHECK(read_bytes(path, got, sizeof got) == (long)na && !memcmp(got, a, na));

    /* second session of the same test: merged; a broken file and other
       names stay in TEST:cov */
    CHECK(amiga_emulation_cov_prepare(test) == 0);
    nb = make_cov(b, 64, 0x10);
    snprintf(path, sizeof path, "%s/blitz2.cov", cov);
    write_bytes(path, b, nb);
    snprintf(path, sizeof path, "%s/broken.cov", cov);
    write_bytes(path, b, nb - 1);
    snprintf(path, sizeof path, "%s/notes.txt", cov);
    write_bytes(path, "x", 1);
    CHECK(amiga_emulation_cov_collect(test, dest, err, sizeof err) == 1);
    snprintf(path, sizeof path, "%s/blitz2.cov", dest);
    CHECK(read_bytes(path, got, sizeof got) == (long)na);
    CHECK(got[COVFMT_HEADER_SIZE] == 0x11 && got[na - 1] == 0x11);
    snprintf(path, sizeof path, "%s/broken.cov", cov);
    CHECK(access(path, F_OK) == 0);
    snprintf(path, sizeof path, "%s/broken.cov", dest);
    CHECK(access(path, F_OK) != 0);
    snprintf(path, sizeof path, "%s/notes.txt", cov);
    CHECK(access(path, F_OK) == 0);

    /* another build: replaces */
    snprintf(path, sizeof path, "%s/broken.cov", cov);
    unlink(path);
    nb = make_cov(b, 80, 0x02);
    snprintf(path, sizeof path, "%s/blitz2.cov", cov);
    write_bytes(path, b, nb);
    CHECK(amiga_emulation_cov_collect(test, dest, err, sizeof err) == 1);
    snprintf(path, sizeof path, "%s/blitz2.cov", dest);
    CHECK(read_bytes(path, got, sizeof got) == (long)nb && !memcmp(got, b, nb));

    /* a destination that cannot be made */
    write_bytes(path, a, na);
    snprintf(path, sizeof path, "%s/blitz2.cov", cov);
    write_bytes(path, a, na);
    snprintf(path, sizeof path, "%s/blitz2.cov/sub", dest);
    CHECK(amiga_emulation_cov_collect(test, path, err, sizeof err) == -1);
    CHECK(strstr(err, "cannot create") != NULL);

    snprintf(path, sizeof path, "rm -rf %s", base);
    CHECK(system(path) == 0);
}

int main(void)
{
    test_merge();
    test_collect();
    if (failures)
        fprintf(stderr, "%d failure(s)\n", failures);
    return failures != 0;
}
