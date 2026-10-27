/*
 * test_biglib.c - biglib_write / biglib_read: round trip, format, broken
 * files.
 */
#include "testutil.h"
#include "biglib.h"

int main(void)
{
    static const uint8_t a[] = { 1, 2, 3, 4 }, b[] = { 5, 6, 7, 8, 9, 10, 11, 12 };
    struct biglib_entry in[2] = { { a, sizeof a }, { b, sizeof b } }, *out;
    static const uint8_t expect[] = { 0, 0, 0, 4, 1, 2, 3, 4,
                                      0, 0, 0, 8, 5, 6, 7, 8, 9, 10, 11, 12, 0, 0, 0, 0 };
    uint8_t buf[64];
    size_t size, n;
    char err[200];
    FILE *f = tmpfile();

    /* write: length, data, ..., long 0 */
    CHECK(biglib_write(f, in, 2) == 0);
    rewind(f);
    size = fread(buf, 1, sizeof buf, f);
    fclose(f);
    CHECK(size == sizeof expect && !memcmp(buf, expect, sizeof expect));

    /* read it back */
    CHECK(biglib_read(buf, size, &out, &n, err, sizeof err) == 0);
    CHECK(n == 2 && out[0].size == 4 && out[1].size == 8 && !memcmp(out[1].data, b, 8));
    free(out);

    /* no end mark */
    CHECK(biglib_read(buf, size - 4, &out, &n, err, sizeof err) == -1 && strstr(err, "end mark"));
    CHECK(out == NULL && n == 0);
    /* a length beyond the end */
    buf[11] = 0x40;
    CHECK(biglib_read(buf, size, &out, &n, err, sizeof err) == -1 && strstr(err, "only"));
    /* empty BigLib file: only the end mark */
    CHECK(biglib_read(expect + 20, 4, &out, &n, err, sizeof err) == 0 && n == 0);
    free(out);

    if (failures)
        fprintf(stderr, "test_biglib: %d failures\n", failures);
    return failures != 0;
}
