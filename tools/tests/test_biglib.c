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

    /* library numbers (word at offset 4 of the code hunk) and finding one */
    {
        struct buf c1 = { 0 }, c2 = { 0 }, l1, l2, short_code = { 0 }, l3;
        struct biglib_entry libs[3];
        unsigned num = 0;

        put32(&c1, 0x70004E75);
        put16(&c1, 100);
        put16(&c1, 0);
        put32(&c2, 0x70004E75);
        put16(&c2, 0xFFDC);                 /* 65500 */
        put16(&c2, 0);
        put16(&short_code, 0x4E75);         /* too short for a number */
        l1 = loadfile1(1, HUNK_CODE, &c1, NULL, 0, 0);
        l2 = loadfile1(1, HUNK_CODE, &c2, NULL, 0, 0);
        l3 = loadfile1(1, HUNK_CODE, &short_code, NULL, 0, 0);
        CHECK(biglib_number(l2.p, l2.n, &num) == 0 && num == 65500);
        CHECK(biglib_number(l3.p, l3.n, &num) == -1);
        CHECK(biglib_number(a, sizeof a, &num) == -1);  /* no load file */
        libs[0].data = l3.p;
        libs[0].size = (uint32_t)l3.n;
        libs[1].data = l1.p;
        libs[1].size = (uint32_t)l1.n;
        libs[2].data = l2.p;
        libs[2].size = (uint32_t)l2.n;
        CHECK(biglib_find(libs, 3, 100) == 1);
        CHECK(biglib_find(libs, 3, 65500) == 2);
        CHECK(biglib_find(libs, 3, 0x1FFDC) == 2);      /* 16 bits count */
        CHECK(biglib_find(libs, 3, 7) == -1);
        /* clashes: all numbers differ (the one without a number does not
           count), then the third has the number of the second */
        {
            size_t e = 99;

            CHECK(biglib_clash(libs, 3, &e) == -1 && e == 99);
            libs[2] = libs[1];
            CHECK(biglib_clash(libs, 3, &e) == 2 && e == 1);
            CHECK(biglib_clash(libs, 2, &e) == -1);
        }
        free(c1.p);
        free(c2.p);
        free(short_code.p);
        free(l1.p);
        free(l2.p);
        free(l3.p);
    }

    if (failures)
        fprintf(stderr, "test_biglib: %d failures\n", failures);
    return failures != 0;
}
