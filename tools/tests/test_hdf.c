/*
 * test_hdf.c - AmigaDOS hardfiles (tools/lib/hdf): images formatted in
 * memory (raw and inside a Rigid Disk Block), files written, replaced,
 * copied and read back in all four DOS types, the checks, and broken or
 * unsupported images that must be refused.
 */
#include "testutil.h"
#include "hdf.h"

#define DOS(n) (0x444F5300u | (n))

static uint32_t rd(const uint8_t *p)
{
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
}

static void wr(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

/* the checksum long at off over n longs */
static void sum(uint8_t *b, size_t nlongs, size_t off)
{
    uint32_t s = 0;
    size_t i;

    wr(b + off, 0);
    for (i = 0; i < nlongs; i++)
        s += rd(b + i * 4);
    wr(b + off, (uint32_t)-s);
}

static uint8_t *formatted(size_t blocks, uint32_t dostype)
{
    uint8_t *img = malloc(blocks * HDF_BLOCK);
    char err[200];

    CHECK(img && hdf_format(img, blocks * HDF_BLOCK, dostype, "Test", 17000, err, sizeof err) == 0);
    return img;
}

static uint8_t *pattern(size_t n, unsigned seed)
{
    uint8_t *p = malloc(n ? n : 1);
    size_t i;

    for (i = 0; i < n; i++)
        p[i] = (uint8_t)(i * 7 + seed + i / 251);
    return p;
}

static int reads(struct hdf *h, const char *path, const uint8_t *want, size_t n)
{
    uint8_t *got;
    size_t len;
    char err[200];
    int ok;

    if (hdf_read(h, path, &got, &len, err, sizeof err)) {
        fprintf(stderr, "read %s: %s\n", path, err);
        return 0;
    }
    ok = len == n && !memcmp(got, want, n) && got[n] == 0;
    free(got);
    return ok;
}

/* writing, replacing, reading in one DOS type */
static void files(uint32_t dostype)
{
    size_t blocks = 400, sizes[] = { 0, 1, 488, 489, 512, 513, 72 * 512 + 1, 150 * 512 + 3, 10 };
    uint8_t *img = formatted(blocks, dostype), *data;
    struct hdf h;
    char err[200];
    uint32_t free0;
    size_t i;

    CHECK(hdf_open(&h, img, blocks * HDF_BLOCK, err, sizeof err) == 0);
    CHECK(!h.rdb && h.reserved == 2 && h.nblocks == blocks && h.root == 200);
    CHECK(hdf_check(&h, err, sizeof err) == 0);
    free0 = hdf_free_blocks(&h);
    CHECK(free0 == blocks - 2 - 2);
    for (i = 0; i < sizeof sizes / sizeof sizes[0]; i++) {
        data = pattern(sizes[i], (unsigned)i);
        CHECK(hdf_write(&h, "Data", data, sizes[i], err, sizeof err) == 0);
        CHECK(reads(&h, "data", data, sizes[i]));
        CHECK(reads(&h, "/DATA", data, sizes[i]));
        if (hdf_check(&h, err, sizeof err)) {
            fprintf(stderr, "DOS\\%u, %zu bytes: %s\n", dostype & 0xFF, sizes[i], err);
            failures++;
        }
        free(data);
    }
    /* 10 bytes left: one header and one data block */
    CHECK(hdf_free_blocks(&h) == free0 - 2);
    /* a second file, a copy of it, replacing text */
    CHECK(hdf_write(&h, "Text", "one two three", 13, err, sizeof err) == 0);
    CHECK(hdf_copy(&h, "Text", "Copy", err, sizeof err) == 0);
    CHECK(hdf_replace(&h, "Copy", "two", "2", err, sizeof err) == 0);
    CHECK(reads(&h, "Copy", (const uint8_t *)"one 2 three", 11));
    CHECK(reads(&h, "Text", (const uint8_t *)"one two three", 13));
    CHECK(hdf_replace(&h, "Copy", "two", "x", err, sizeof err) == -1 && strstr(err, "not"));
    CHECK(hdf_write(&h, "Copy", "aa aa", 5, err, sizeof err) == 0);
    CHECK(hdf_replace(&h, "Copy", "aa", "b", err, sizeof err) == -1
          && strstr(err, "more than once"));
    CHECK(hdf_replace(&h, "Copy", "", "b", err, sizeof err) == -1);
    CHECK(hdf_check(&h, err, sizeof err) == 0);
    /* paths that do not work */
    CHECK(hdf_read(&h, "None", &data, &i, err, sizeof err) == -1 && strstr(err, "not found"));
    CHECK(hdf_write(&h, "None/x", "x", 1, err, sizeof err) == -1 && strstr(err, "not found"));
    CHECK(hdf_write(&h, "Text/x", "x", 1, err, sizeof err) == -1 && strstr(err, "no directory"));
    CHECK(hdf_read(&h, "", &data, &i, err, sizeof err) == -1 && strstr(err, "not a file"));
    CHECK(hdf_write(&h, "a234567890123456789012345678901", "x", 1, err, sizeof err) == -1
          && strstr(err, "bad name"));
    CHECK(hdf_write(&h, "a:b", "x", 1, err, sizeof err) == -1 && strstr(err, "bad name"));
    /* no room: refused before anything changes */
    free0 = hdf_free_blocks(&h);
    data = pattern(blocks * HDF_BLOCK, 1);
    CHECK(hdf_write(&h, "Data", data, blocks * HDF_BLOCK, err, sizeof err) == -1
          && strstr(err, "no room"));
    CHECK(hdf_free_blocks(&h) == free0);
    free(data);
    data = pattern(10, 8);                              /* still the 10 bytes */
    CHECK(reads(&h, "Data", data, 10));
    free(data);
    CHECK(hdf_check(&h, err, sizeof err) == 0);
    hdf_close(&h);
    free(img);
}

/* names: case-insensitive; the international DOS types also for Latin-1 */
static void names(void)
{
    uint8_t *img;
    struct hdf h;
    char err[200], vol[HDF_NAMEMAX + 2];
    uint8_t *got;
    size_t len;
    int international;

    for (international = 0; international < 2; international++) {
        img = formatted(100, DOS(international ? 3 : 1));
        CHECK(hdf_open(&h, img, 100 * HDF_BLOCK, err, sizeof err) == 0);
        hdf_volume_name(&h, vol);
        CHECK(!strcmp(vol, "Test"));
        CHECK(hdf_write(&h, "caf\xe9", "x", 1, err, sizeof err) == 0);
        CHECK(hdf_read(&h, "CAF\xc9", &got, &len, err, sizeof err) == (international ? 0 : -1));
        if (international)
            free(got);
        CHECK(hdf_read(&h, "CAF\xe9", &got, &len, err, sizeof err) == 0);
        free(got);
        hdf_close(&h);
        free(img);
    }
}

/* a copy keeps the date and the protection bits; a new file has its
   directory's date; the same changes give the same bytes */
static void dates(void)
{
    uint8_t *a = formatted(100, DOS(1)), *b = formatted(100, DOS(1)), *hb;
    struct hdf h;
    char err[200];
    uint32_t n;
    int k;

    for (k = 0; k < 2; k++) {
        CHECK(hdf_open(&h, k ? b : a, 100 * HDF_BLOCK, err, sizeof err) == 0);
        CHECK(hdf_write(&h, "S", "first", 5, err, sizeof err) == 0);
        CHECK(hdf_copy(&h, "S", "T", err, sizeof err) == 0);
        CHECK(hdf_write(&h, "S", "second", 6, err, sizeof err) == 0);
        hdf_close(&h);
    }
    CHECK(!memcmp(a, b, 100 * HDF_BLOCK));
    /* the headers: after the root (50) and its bitmap (51), each followed
       by its data block (S's new one takes the freed block again) */
    for (n = 52; n < 56; n += 2) {
        hb = a + n * HDF_BLOCK;
        CHECK(rd(hb) == 2 && rd(hb + 420) == 17000);
    }
    /* change S's date and protection by hand: T copies them */
    hb = a + 52 * HDF_BLOCK;
    wr(hb + 420, 12345);
    wr(hb + 320, 0x0F);
    sum(hb, 128, 20);
    CHECK(hdf_open(&h, a, 100 * HDF_BLOCK, err, sizeof err) == 0);
    CHECK(hdf_copy(&h, "S", "U", err, sizeof err) == 0);
    CHECK(hdf_check(&h, err, sizeof err) == 0);
    hdf_close(&h);
    hb = a + 56 * HDF_BLOCK;
    CHECK(rd(hb) == 2 && hb[433] == 'U' && rd(hb + 420) == 12345 && rd(hb + 320) == 0x0F);
    free(a);
    free(b);
}

/* an RDB image: RDSK in block 0, PART in block 1, the partition from
   cylinder 2 (cylinders of 4 blocks) */
static uint8_t *rdb_image(size_t *size, uint32_t dostype)
{
    size_t cyls = 40, blocks = cyls * 4;
    uint8_t *img = calloc(blocks, HDF_BLOCK), *r = img, *p = img + HDF_BLOCK, *env = p + 128;
    char err[200];

    memcpy(r, "RDSK", 4);
    wr(r + 4, 64);
    wr(r + 16, 512);
    wr(r + 24, 0xFFFFFFFF);
    wr(r + 28, 1);
    wr(r + 32, 0xFFFFFFFF);
    sum(r, 64, 8);
    memcpy(p, "PART", 4);
    wr(p + 4, 64);
    wr(p + 16, 0xFFFFFFFF);
    p[36] = 3;
    memcpy(p + 37, "DH0", 3);
    wr(env, 16);
    wr(env + 4, 128);
    wr(env + 12, 1);            /* surfaces */
    wr(env + 16, 1);            /* sectors per block */
    wr(env + 20, 4);            /* blocks per track */
    wr(env + 24, 2);            /* reserved */
    wr(env + 36, 2);            /* low cylinder */
    wr(env + 40, (uint32_t)cyls - 1);
    wr(env + 64, dostype);
    sum(p, 64, 8);
    CHECK(hdf_format(img + 8 * HDF_BLOCK, (blocks - 8) * HDF_BLOCK, dostype, "WB", 1, err,
                     sizeof err) == 0);
    *size = blocks * HDF_BLOCK;
    return img;
}

static void rdb(void)
{
    size_t size;
    uint8_t *img = rdb_image(&size, DOS(1)), *got = NULL;
    struct hdf h;
    char err[200];
    size_t len;

    CHECK(hdf_open(&h, img, size, err, sizeof err) == 0);
    CHECK(h.rdb && !strcmp(h.partname, "DH0") && h.offset == 8 * HDF_BLOCK
          && h.nblocks == 152 && h.root == 76);
    CHECK(hdf_write(&h, "File", "contents", 8, err, sizeof err) == 0);
    CHECK(hdf_check(&h, err, sizeof err) == 0);
    hdf_close(&h);
    /* the partition alone is a raw image with the same file */
    CHECK(hdf_open(&h, img + 8 * HDF_BLOCK, 152 * HDF_BLOCK, err, sizeof err) == 0);
    CHECK(!h.rdb && hdf_read(&h, "file", &got, &len, err, sizeof err) == 0 && len == 8
          && !memcmp(got, "contents", 8));
    free(got);
    hdf_close(&h);
    /* broken: the RDSK checksum, a second partition, too short an image */
    img[100] ^= 1;
    CHECK(hdf_open(&h, img, size, err, sizeof err) == -1 && strstr(err, "RDSK"));
    img[100] ^= 1;
    wr(img + HDF_BLOCK + 16, 2);
    sum(img + HDF_BLOCK, 64, 8);
    CHECK(hdf_open(&h, img, size, err, sizeof err) == -1 && strstr(err, "more than one"));
    wr(img + HDF_BLOCK + 16, 0xFFFFFFFF);
    sum(img + HDF_BLOCK, 64, 8);
    CHECK(hdf_open(&h, img, size - 4 * HDF_BLOCK, err, sizeof err) == -1
          && strstr(err, "ends after"));
    free(img);
    /* the boot block must agree with the partition's DOS type */
    img = rdb_image(&size, DOS(3));
    wr(img + 8 * HDF_BLOCK, DOS(1));
    CHECK(hdf_open(&h, img, size, err, sizeof err) == -1 && strstr(err, "boot block"));
    free(img);
}

/* images that are refused or found damaged */
static void broken(void)
{
    uint8_t *img = formatted(100, DOS(1)), *got;
    struct hdf h;
    char err[200];
    uint32_t hdr, data;
    size_t len;

    /* no file system, dircache, wrong size */
    CHECK(hdf_open(&h, img, 100 * HDF_BLOCK - 1, err, sizeof err) == -1);
    wr(img, DOS(5));
    CHECK(hdf_open(&h, img, 100 * HDF_BLOCK, err, sizeof err) == -1 && strstr(err, "DOS\\5"));
    wr(img, 0x4B49434B);
    CHECK(hdf_open(&h, img, 100 * HDF_BLOCK, err, sizeof err) == -1
          && strstr(err, "no AmigaDOS"));
    wr(img, DOS(1));
    /* the root's checksum, the bitmap flag, a bitmap checksum */
    img[50 * HDF_BLOCK + 300] ^= 1;
    CHECK(hdf_open(&h, img, 100 * HDF_BLOCK, err, sizeof err) == -1 && strstr(err, "root"));
    img[50 * HDF_BLOCK + 300] ^= 1;
    wr(img + 50 * HDF_BLOCK + 312, 0);
    sum(img + 50 * HDF_BLOCK, 128, 20);
    CHECK(hdf_open(&h, img, 100 * HDF_BLOCK, err, sizeof err) == -1 && strstr(err, "not marked"));
    wr(img + 50 * HDF_BLOCK + 312, 0xFFFFFFFF);
    sum(img + 50 * HDF_BLOCK, 128, 20);
    img[51 * HDF_BLOCK + 100] ^= 1;
    CHECK(hdf_open(&h, img, 100 * HDF_BLOCK, err, sizeof err) == -1 && strstr(err, "bitmap"));
    img[51 * HDF_BLOCK + 100] ^= 1;
    /* the check: a used block marked free, an allocated block unused */
    CHECK(hdf_open(&h, img, 100 * HDF_BLOCK, err, sizeof err) == 0);
    CHECK(hdf_write(&h, "F", "x", 1, err, sizeof err) == 0);
    hdr = 52;
    data = rd(img + hdr * HDF_BLOCK + 16);
    CHECK(data == 53);
    img[51 * HDF_BLOCK + 4 + (data - 2) / 32 * 4 + 3 - (data - 2) % 32 / 8] ^= (uint8_t)(1u << ((data - 2) % 8));
    sum(img + 51 * HDF_BLOCK, 128, 0);
    CHECK(hdf_check(&h, err, sizeof err) == -1 && strstr(err, "used but free"));
    img[51 * HDF_BLOCK + 4 + (data - 2) / 32 * 4 + 3 - (data - 2) % 32 / 8] ^= (uint8_t)(1u << ((data - 2) % 8));
    sum(img + 51 * HDF_BLOCK, 128, 0);
    CHECK(hdf_check(&h, err, sizeof err) == 0);
    img[51 * HDF_BLOCK + 4 + (90 - 2) / 32 * 4 + 3 - (90 - 2) % 32 / 8] ^= (uint8_t)(1u << ((90 - 2) % 8));
    sum(img + 51 * HDF_BLOCK, 128, 0);
    CHECK(hdf_check(&h, err, sizeof err) == -1 && strstr(err, "allocated but not used"));
    img[51 * HDF_BLOCK + 4 + (90 - 2) / 32 * 4 + 3 - (90 - 2) % 32 / 8] ^= (uint8_t)(1u << ((90 - 2) % 8));
    sum(img + 51 * HDF_BLOCK, 128, 0);
    /* a header with a wrong checksum, a size that does not fit the table */
    img[hdr * HDF_BLOCK + 200] ^= 1;
    CHECK(hdf_read(&h, "F", &got, &len, err, sizeof err) == -1 && strstr(err, "no valid header"));
    CHECK(hdf_check(&h, err, sizeof err) == -1);
    img[hdr * HDF_BLOCK + 200] ^= 1;
    wr(img + hdr * HDF_BLOCK + 324, 600);
    sum(img + hdr * HDF_BLOCK, 128, 20);
    CHECK(hdf_read(&h, "F", &got, &len, err, sizeof err) == -1 && strstr(err, "does not fit"));
    hdf_close(&h);
    free(img);
    /* OFS: a damaged data block */
    img = formatted(100, DOS(0));
    CHECK(hdf_open(&h, img, 100 * HDF_BLOCK, err, sizeof err) == 0);
    CHECK(hdf_write(&h, "F", "abc", 3, err, sizeof err) == 0);
    CHECK(reads(&h, "F", (const uint8_t *)"abc", 3));
    img[53 * HDF_BLOCK + 25] ^= 1;
    CHECK(hdf_read(&h, "F", &got, &len, err, sizeof err) == -1 && strstr(err, "damaged"));
    CHECK(hdf_check(&h, err, sizeof err) == -1 && strstr(err, "damaged"));
    hdf_close(&h);
    free(img);
}

static void listing(void)
{
    uint8_t *img = formatted(100, DOS(3));
    struct hdf h;
    char err[200], out[200] = { 0 };
    FILE *f = tmpfile();

    CHECK(hdf_open(&h, img, 100 * HDF_BLOCK, err, sizeof err) == 0);
    CHECK(hdf_write(&h, "Readme", "hello", 5, err, sizeof err) == 0);
    CHECK(hdf_list(&h, "", f, err, sizeof err) == 0);
    CHECK(f && fseek(f, 0, SEEK_SET) == 0 && fread(out, 1, sizeof out - 1, f) == 9);
    fclose(f);
    CHECK(!strcmp(out, "Readme 5\n"));
    CHECK(hdf_list(&h, "Readme", stderr, err, sizeof err) == -1 && strstr(err, "not a directory"));
    hdf_close(&h);
    free(img);
}

int main(void)
{
    uint32_t t;
    char err[200];
    uint8_t img[8 * HDF_BLOCK];

    for (t = 0; t < 4; t++)
        files(DOS(t));
    names();
    dates();
    rdb();
    broken();
    listing();
    CHECK(hdf_format(img, sizeof img, DOS(4), "x", 0, err, sizeof err) == -1);
    CHECK(hdf_format(img, sizeof img, DOS(1), "", 0, err, sizeof err) == -1);
    if (failures)
        fprintf(stderr, "test_hdf: %d failures\n", failures);
    return failures != 0;
}
