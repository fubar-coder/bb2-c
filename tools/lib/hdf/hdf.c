/*
 * hdf.c - AmigaDOS hardfiles on the host (see hdf.h).
 *
 * The layout of the blocks (512 bytes, big-endian longs) as the Amiga
 * ROM Kernel Reference Manual: Devices ("Rigid Disk Block") and the
 * AmigaDOS Manual describe them:
 *   RDSK  0 id, 4 summed longs, 8 checksum, 16 block size, 28 partition list
 *   PART  0 id, 4 summed longs, 8 checksum, 16 next, 36 name (BSTR),
 *         128 DosEnvec (3 surfaces, 5 blocks per track, 6 reserved,
 *         9 low cylinder, 10 high cylinder, 16 DOS type; in longs)
 *   root / directory / file header (T_HEADER):
 *         0 type, 4 own block, 8 high_seq, 12 hash table size (root),
 *         16 first data block, 20 checksum, 24 hash table or data block
 *         table (72 longs, the table from the end), 312 bitmap flag and
 *         316 bitmap blocks (25) and 416 bitmap extension (root),
 *         320 protection, 324 byte size, 328 comment, 420 date (days,
 *         minutes, ticks), 432 name (BSTR), 496 hash chain, 500 parent,
 *         504 extension (first T_LIST block), 508 secondary type
 *   T_LIST (file extension): as the header for 0..20, the table and 500,
 *         504, 508
 *   T_DATA (OFS only): 4 header, 8 sequence number, 12 data bytes,
 *         16 next data block, 20 checksum, 24 the data (488 bytes)
 *   bitmap block: 0 checksum, then 127 longs; bit set = free, bit 0 of
 *         the first long = the first block after the reserved ones
 * A checksum makes the sum of all longs of the block 0.
 */
#include "hdf.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

enum { T_HEADER = 2, T_DATA = 8, T_LIST = 16 };
enum { ST_ROOT = 1, ST_USERDIR = 2, ST_SOFTLINK = 3, ST_LINKDIR = 4, ST_FILE = -3,
       ST_LINKFILE = -4 };
enum {
    O_TYPE = 0, O_KEY = 4, O_HIGHSEQ = 8, O_HTSIZE = 12, O_FIRSTDATA = 16, O_CHKSUM = 20,
    O_TABLE = 24, O_BMFLAG = 312, O_BMPAGES = 316, O_BMEXT = 416, O_ACCESS = 320,
    O_BYTESIZE = 324, O_DAYS = 420, O_MINS = 424, O_TICKS = 428, O_NAME = 432,
    O_DISKDAYS = 472, O_CREATEDAYS = 484, O_HASHCHAIN = 496, O_PARENT = 500, O_EXT = 504,
    O_SECTYPE = 508
};
enum { BM_LONGS = 127, BM_BITS = BM_LONGS * 32, BM_PAGES = 25 };

static int fail(char *err, size_t errlen, const char *fmt, ...)
{
    va_list ap;

    if (err && errlen) {
        va_start(ap, fmt);
        vsnprintf(err, errlen, fmt, ap);
        va_end(ap);
    }
    return -1;
}

static uint32_t rd32(const uint8_t *p)
{
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
}

static void wr32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

/* the value for the checksum long at off of a block of n longs */
static uint32_t sum_for(const uint8_t *b, size_t nlongs, size_t off)
{
    uint32_t s = 0;
    size_t i;

    for (i = 0; i < nlongs; i++)
        if (i * 4 != off)
            s += rd32(b + i * 4);
    return (uint32_t)-s;
}

static int sum_ok(const uint8_t *b, size_t off)
{
    return rd32(b + off) == sum_for(b, HDF_BLOCK / 4, off);
}

static void sum_set(uint8_t *b, size_t off)
{
    wr32(b + off, sum_for(b, HDF_BLOCK / 4, off));
}

static int intl(const struct hdf *h)
{
    return (h->dostype & 0xFF) >= 2;
}

static int is_ffs(const struct hdf *h)
{
    return h->dostype & 1;
}

static unsigned upper(unsigned c, int international)
{
    if (c >= 'a' && c <= 'z')
        return c - 32;
    if (international && c >= 224 && c <= 254 && c != 247)
        return c - 32;
    return c;
}

static unsigned name_hash(const uint8_t *name, size_t len, int international)
{
    uint32_t h = (uint32_t)len;
    size_t i;

    for (i = 0; i < len; i++)
        h = (h * 13 + upper(name[i], international)) & 0x7FF;
    return h % HDF_HT;
}

static int name_eq(const uint8_t *a, size_t alen, const uint8_t *b, size_t blen, int international)
{
    size_t i;

    if (alen != blen)
        return 0;
    for (i = 0; i < alen; i++)
        if (upper(a[i], international) != upper(b[i], international))
            return 0;
    return 1;
}

/* block n of the partition, NULL if outside */
static uint8_t *blk(const struct hdf *h, uint32_t n)
{
    if (n >= h->nblocks)
        return NULL;
    return h->data + h->offset + (size_t)n * HDF_BLOCK;
}

/* a header block (root, directory, file, link) with a valid checksum */
static uint8_t *header(const struct hdf *h, uint32_t n, char *err, size_t errlen)
{
    uint8_t *b = n >= h->reserved ? blk(h, n) : NULL;

    if (!b) {
        fail(err, errlen, "block %u is outside the partition", n);
        return NULL;
    }
    if (rd32(b + O_TYPE) != T_HEADER || !sum_ok(b, O_CHKSUM)) {
        fail(err, errlen, "block %u is no valid header block", n);
        return NULL;
    }
    return b;
}

static int32_t sectype(const uint8_t *b)
{
    return (int32_t)rd32(b + O_SECTYPE);
}

/* ---- the bitmap ---- */

static uint8_t *bm_word(const struct hdf *h, uint32_t n, uint32_t *bit)
{
    uint32_t i = n - h->reserved;
    uint8_t *b = blk(h, h->bm[i / BM_BITS]);

    *bit = 1u << (i % 32);
    return b + 4 + (size_t)(i % BM_BITS / 32) * 4;
}

static int bm_free(const struct hdf *h, uint32_t n)
{
    uint32_t bit;
    uint8_t *w = bm_word(h, n, &bit);

    return (rd32(w) & bit) != 0;
}

static void bm_set(struct hdf *h, uint32_t n, int free)
{
    uint32_t bit;
    uint8_t *w = bm_word(h, n, &bit);
    uint32_t v = rd32(w);

    wr32(w, free ? v | bit : v & ~bit);
    sum_set(blk(h, h->bm[(n - h->reserved) / BM_BITS]), 0);
}

uint32_t hdf_free_blocks(const struct hdf *h)
{
    uint32_t n, c = 0;

    for (n = h->reserved; n < h->nblocks; n++)
        c += bm_free(h, n);
    return c;
}

/* the next free block, from the root upwards, then from the start */
static int bm_alloc(struct hdf *h, uint32_t *out)
{
    uint32_t i, n;

    for (i = 0; i < h->nblocks - h->reserved; i++) {
        n = h->root + i;
        if (n >= h->nblocks)
            n -= h->nblocks - h->reserved;
        if (bm_free(h, n)) {
            bm_set(h, n, 0);
            *out = n;
            return 0;
        }
    }
    return -1;
}

static int load_bitmap(struct hdf *h, char *err, size_t errlen)
{
    uint8_t *root = blk(h, h->root);
    size_t need = (h->nblocks - h->reserved + BM_BITS - 1) / BM_BITS, i;
    uint32_t ext = rd32(root + O_BMEXT), n;
    uint8_t *e;

    if ((int32_t)rd32(root + O_BMFLAG) != -1)
        return fail(err, errlen, "the bitmap is not marked valid");
    h->bm = calloc(need, sizeof *h->bm);
    if (!h->bm)
        return fail(err, errlen, "out of memory");
    for (i = 0; i < need && i < BM_PAGES; i++)
        h->bm[h->nbm++] = rd32(root + O_BMPAGES + i * 4);
    while (h->nbm < need) {
        e = ext >= h->reserved ? blk(h, ext) : NULL;
        if (!e)
            return fail(err, errlen, "bitmap extension block %u is outside the partition", ext);
        for (i = 0; i < BM_LONGS && h->nbm < need; i++)
            h->bm[h->nbm++] = rd32(e + i * 4);
        ext = rd32(e + 508);
    }
    for (i = 0; i < h->nbm; i++) {
        n = h->bm[i];
        if (n < h->reserved || !blk(h, n))
            return fail(err, errlen, "bitmap block %u is outside the partition", n);
        if (!sum_ok(blk(h, n), 0))
            return fail(err, errlen, "bitmap block %u has a wrong checksum", n);
    }
    return 0;
}

/* ---- opening ---- */

static int open_rdb(struct hdf *h, uint32_t rdsk, char *err, size_t errlen)
{
    const uint8_t *r = h->data + (size_t)rdsk * HDF_BLOCK, *p, *env;
    uint32_t part, summed, n, surfaces, bpt, lo, hi;
    size_t len;

    summed = rd32(r + 4);
    if (summed < 64 || summed > HDF_BLOCK / 4 || rd32(r + 8) != sum_for(r, summed, 8))
        return fail(err, errlen, "RDSK block %u: wrong checksum", rdsk);
    if (rd32(r + 16) != HDF_BLOCK)
        return fail(err, errlen, "RDB: block size %u, not 512", rd32(r + 16));
    part = rd32(r + 28);
    if (part == 0xFFFFFFFF)
        return fail(err, errlen, "RDB without partition");
    if ((size_t)part * HDF_BLOCK + HDF_BLOCK > h->size)
        return fail(err, errlen, "RDB: partition block %u outside the image", part);
    p = h->data + (size_t)part * HDF_BLOCK;
    summed = rd32(p + 4);
    if (memcmp(p, "PART", 4) || summed < 64 || summed > HDF_BLOCK / 4
        || rd32(p + 8) != sum_for(p, summed, 8))
        return fail(err, errlen, "RDB: block %u is no valid PART block", part);
    if (rd32(p + 16) != 0xFFFFFFFF)
        return fail(err, errlen, "RDB with more than one partition");
    n = p[36];
    if (n > 31)
        return fail(err, errlen, "RDB: partition name too long");
    memcpy(h->partname, p + 37, n);
    h->partname[n] = 0;
    env = p + 128;
    if (rd32(env + 1 * 4) != HDF_BLOCK / 4 || rd32(env + 4 * 4) != 1)
        return fail(err, errlen, "partition %s: blocks of %u longs, not 128", h->partname,
                    rd32(env + 4));
    surfaces = rd32(env + 3 * 4);
    bpt = rd32(env + 5 * 4);
    h->reserved = rd32(env + 6 * 4);
    lo = rd32(env + 9 * 4);
    hi = rd32(env + 10 * 4);
    h->dostype = rd32(env + 16 * 4);
    if (!surfaces || !bpt || hi < lo || (uint64_t)(hi - lo + 1) * surfaces * bpt > 0xFFFFFFFFu)
        return fail(err, errlen, "partition %s: bad geometry", h->partname);
    h->offset = (size_t)lo * surfaces * bpt * HDF_BLOCK;
    h->nblocks = (hi - lo + 1) * surfaces * bpt;
    len = (size_t)h->nblocks * HDF_BLOCK;
    if (h->offset > h->size || len > h->size - h->offset)
        return fail(err, errlen, "partition %s ends after the image", h->partname);
    return 0;
}

int hdf_open(struct hdf *h, uint8_t *data, size_t size, char *err, size_t errlen)
{
    uint32_t i, boot;
    uint8_t *root;

    memset(h, 0, sizeof *h);
    h->data = data;
    h->size = size;
    if (size % HDF_BLOCK || size < 8 * HDF_BLOCK)
        return fail(err, errlen, "size %zu is no multiple of 512 or too small", size);
    for (i = 0; i < 16 && (i + 1) * (size_t)HDF_BLOCK <= size; i++)
        if (!memcmp(data + (size_t)i * HDF_BLOCK, "RDSK", 4))
            break;
    if (i < 16 && (i + 1) * (size_t)HDF_BLOCK <= size) {
        h->rdb = 1;
        if (open_rdb(h, i, err, errlen))
            return -1;
    } else {
        h->reserved = 2;
        h->nblocks = (uint32_t)(size / HDF_BLOCK);
        h->dostype = rd32(data);
    }
    if ((h->dostype & 0xFFFFFF00) != 0x444F5300)
        return fail(err, errlen, "no AmigaDOS file system (DOS type %08x)", h->dostype);
    if ((h->dostype & 0xFF) > 3)
        return fail(err, errlen, "DOS\\%u is not supported (only DOS\\0 to DOS\\3)",
                    h->dostype & 0xFF);
    if (h->reserved < 1 || h->nblocks < h->reserved + 4)
        return fail(err, errlen, "partition too small");
    boot = rd32(h->data + h->offset);
    if (boot != h->dostype)
        return fail(err, errlen, "boot block DOS type %08x, partition %08x", boot, h->dostype);
    h->root = (h->nblocks - 1 + h->reserved) / 2;
    root = blk(h, h->root);
    if (rd32(root + O_TYPE) != T_HEADER || sectype(root) != ST_ROOT || !sum_ok(root, O_CHKSUM)
        || rd32(root + O_HTSIZE) != HDF_HT)
        return fail(err, errlen, "block %u is no valid root block", h->root);
    if (load_bitmap(h, err, errlen)) {
        hdf_close(h);
        return -1;
    }
    return 0;
}

void hdf_close(struct hdf *h)
{
    free(h->bm);
    h->bm = NULL;
    h->nbm = 0;
}

void hdf_volume_name(const struct hdf *h, char name[HDF_NAMEMAX + 2])
{
    const uint8_t *r = blk(h, h->root);
    size_t n = r[O_NAME] > HDF_NAMEMAX ? HDF_NAMEMAX : r[O_NAME];

    memcpy(name, r + O_NAME + 1, n);
    name[n] = 0;
}

/* ---- names and paths ---- */

/* the entry called name[0..len) in directory dir: its block, 0 if none */
static int find_entry(const struct hdf *h, uint32_t dir, const uint8_t *name, size_t len,
                      uint32_t *out, char *err, size_t errlen)
{
    uint8_t *d = header(h, dir, err, errlen), *e;
    uint32_t n;
    unsigned guard = 0;

    if (!d)
        return -1;
    n = rd32(d + O_TABLE + name_hash(name, len, intl(h)) * 4);
    while (n) {
        if (!(e = header(h, n, err, errlen)))
            return -1;
        if (name_eq(e + O_NAME + 1, e[O_NAME], name, len, intl(h))) {
            *out = n;
            return 0;
        }
        n = rd32(e + O_HASHCHAIN);
        if (++guard > h->nblocks)
            return fail(err, errlen, "hash chain loop in directory %u", dir);
    }
    *out = 0;
    return 0;
}

/* the entry of path; with parent: also the directory of the last
   component, which itself need not exist (*out = 0) */
static int lookup(const struct hdf *h, const char *path, uint32_t *out, uint32_t *parent,
                  const char **last, char *err, size_t errlen)
{
    uint32_t cur = h->root, dir = h->root;
    const char *p = path, *s;
    uint8_t *b;

    while (*p == '/')
        p++;
    if (last)
        *last = p;
    while (*p) {
        s = strchr(p, '/');
        if (!s)
            s = p + strlen(p);
        if ((size_t)(s - p) > HDF_NAMEMAX || s == p)
            return fail(err, errlen, "%s: bad name", path);
        if (!(b = header(h, cur, err, errlen)))
            return -1;
        if (sectype(b) != ST_ROOT && sectype(b) != ST_USERDIR)
            return fail(err, errlen, "%s: %.*s is no directory", path, (int)(p - path), path);
        dir = cur;
        if (last)
            *last = p;
        if (find_entry(h, cur, (const uint8_t *)p, (size_t)(s - p), &cur, err, errlen))
            return -1;
        while (*s == '/')
            s++;
        if (!cur) {
            if (*s || !parent)
                return fail(err, errlen, "%s: not found", path);
            break;
        }
        p = s;
    }
    *out = cur;
    if (parent)
        *parent = dir;
    return 0;
}

/* ---- file contents ---- */

static size_t data_bytes(const struct hdf *h)
{
    return is_ffs(h) ? HDF_BLOCK : HDF_BLOCK - 24;
}

/* the data blocks and the extension blocks of a file header (malloc'd
   lists), checked */
static int file_blocks(const struct hdf *h, uint32_t hdr, uint32_t **data, size_t *ndata,
                       uint32_t **ext, size_t *next, char *err, size_t errlen)
{
    const uint8_t *b = blk(h, hdr), *x;
    uint32_t size = rd32(b + O_BYTESIZE), cnt, i, n;
    size_t want = (size + data_bytes(h) - 1) / data_bytes(h), got = 0, ne = 0;
    uint32_t *d = calloc(want + 1, sizeof *d), *e = calloc(want / HDF_HT + 2, sizeof *e);

    if (!d || !e) {
        free(d);
        free(e);
        return fail(err, errlen, "out of memory");
    }
    x = b;
    for (;;) {
        cnt = rd32(x + O_HIGHSEQ);
        if (cnt > HDF_HT || got + cnt > want)
            goto bad;
        for (i = 0; i < cnt; i++) {
            n = rd32(x + O_TABLE + (HDF_HT - 1 - i) * 4);
            if (n < h->reserved || n >= h->nblocks)
                goto bad;
            d[got++] = n;
        }
        n = rd32(x + O_EXT);
        if (!n)
            break;
        if (cnt != HDF_HT || ne > want / HDF_HT || n < h->reserved || n >= h->nblocks)
            goto bad;
        x = blk(h, n);
        if (rd32(x + O_TYPE) != T_LIST || rd32(x + O_KEY) != n || rd32(x + O_PARENT) != hdr
            || !sum_ok(x, O_CHKSUM))
            goto bad;
        e[ne++] = n;
    }
    if (got != want)
        goto bad;
    *data = d;
    *ndata = got;
    *ext = e;
    *next = ne;
    return 0;
bad:
    free(d);
    free(e);
    return fail(err, errlen, "file header %u: its data block table does not fit its size", hdr);
}

static int read_header(const struct hdf *h, uint32_t hdr, uint8_t **out, size_t *len, char *err,
                       size_t errlen)
{
    const uint8_t *b = blk(h, hdr), *x;
    uint32_t size = rd32(b + O_BYTESIZE), *d, *e;
    size_t nd, ne, i, pos = 0, k;
    uint8_t *buf;

    if (file_blocks(h, hdr, &d, &nd, &e, &ne, err, errlen))
        return -1;
    free(e);
    buf = malloc((size_t)size + 1);
    if (!buf) {
        free(d);
        return fail(err, errlen, "out of memory");
    }
    for (i = 0; i < nd; i++) {
        x = blk(h, d[i]);
        k = size - pos < data_bytes(h) ? size - pos : data_bytes(h);
        if (!is_ffs(h)) {
            if (rd32(x + O_TYPE) != T_DATA || rd32(x + 4) != hdr || rd32(x + 8) != i + 1
                || rd32(x + 12) != k || rd32(x + 16) != (i + 1 < nd ? d[i + 1] : 0)
                || !sum_ok(x, O_CHKSUM)) {
                fail(err, errlen, "data block %u of file header %u is damaged", d[i], hdr);
                free(d);
                free(buf);
                return -1;
            }
            x += 24;
        }
        memcpy(buf + pos, x, k);
        pos += k;
    }
    free(d);
    buf[size] = 0;
    *out = buf;
    *len = size;
    return 0;
}

int hdf_read(struct hdf *h, const char *path, uint8_t **out, size_t *len, char *err, size_t errlen)
{
    uint32_t n;
    uint8_t *b;

    if (lookup(h, path, &n, NULL, NULL, err, errlen))
        return -1;
    if (!(b = header(h, n, err, errlen)))
        return -1;
    if (sectype(b) != ST_FILE)
        return fail(err, errlen, "%s: not a file", path);
    return read_header(h, n, out, len, err, errlen);
}

/* give the header's data the bytes data[0..len): the old data and
   extension blocks are freed, new ones taken */
static int set_contents(struct hdf *h, uint32_t hdr, const uint8_t *data, size_t len, char *err,
                        size_t errlen)
{
    uint8_t *b = blk(h, hdr), *x, *db;
    uint32_t *d, *e, n, prev = 0, need;
    size_t nd, ne, i, nblk, bsz = data_bytes(h), k, inx;

    if (len > 0xFFFFFFFFu)
        return fail(err, errlen, "file too large");
    if (file_blocks(h, hdr, &d, &nd, &e, &ne, err, errlen))
        return -1;
    nblk = (len + bsz - 1) / bsz;
    need = (uint32_t)(nblk + (nblk > HDF_HT ? (nblk - 1) / HDF_HT : 0));
    if (hdf_free_blocks(h) + nd + ne < need) {
        free(d);
        free(e);
        return fail(err, errlen, "no room for %zu bytes", len);
    }
    for (i = 0; i < nd; i++)
        bm_set(h, d[i], 1);
    for (i = 0; i < ne; i++)
        bm_set(h, e[i], 1);
    free(d);
    free(e);
    memset(b + O_TABLE, 0, HDF_HT * 4);
    wr32(b + O_HIGHSEQ, 0);
    wr32(b + O_FIRSTDATA, 0);
    wr32(b + O_EXT, 0);
    x = b;
    for (i = 0; i < nblk; i++) {
        inx = i % HDF_HT;
        if (i && !inx) {
            /* the table is full: an extension block */
            if (bm_alloc(h, &n))
                return fail(err, errlen, "no room");
            wr32(x + O_EXT, n);
            if (x != b)
                sum_set(x, O_CHKSUM);
            x = blk(h, n);
            memset(x, 0, HDF_BLOCK);
            wr32(x + O_TYPE, T_LIST);
            wr32(x + O_KEY, n);
            wr32(x + O_PARENT, hdr);
            wr32(x + O_SECTYPE, (uint32_t)ST_FILE);
        }
        if (bm_alloc(h, &n))
            return fail(err, errlen, "no room");
        wr32(x + O_TABLE + (HDF_HT - 1 - inx) * 4, n);
        wr32(x + O_HIGHSEQ, (uint32_t)inx + 1);
        if (!i)
            wr32(b + O_FIRSTDATA, n);
        k = len - i * bsz < bsz ? len - i * bsz : bsz;
        db = blk(h, n);
        memset(db, 0, HDF_BLOCK);
        if (is_ffs(h)) {
            memcpy(db, data + i * bsz, k);
        } else {
            wr32(db + O_TYPE, T_DATA);
            wr32(db + 4, hdr);
            wr32(db + 8, (uint32_t)i + 1);
            wr32(db + 12, (uint32_t)k);
            memcpy(db + 24, data + i * bsz, k);
            if (prev) {
                wr32(blk(h, prev) + 16, n);
                sum_set(blk(h, prev), O_CHKSUM);
            }
            sum_set(db, O_CHKSUM);
            prev = n;
        }
    }
    if (x != b)
        sum_set(x, O_CHKSUM);
    wr32(b + O_BYTESIZE, (uint32_t)len);
    sum_set(b, O_CHKSUM);
    return 0;
}

/* a new, empty file header called name[0..len) in dir, dated like the
   block date (12 bytes at O_DAYS) */
static int create(struct hdf *h, uint32_t dir, const char *name, size_t len, const uint8_t *date,
                  uint32_t *out, char *err, size_t errlen)
{
    uint8_t *d = blk(h, dir), *b, *p;
    uint32_t n, slot, cur, prevb = 0;
    unsigned guard = 0;
    size_t i;

    for (i = 0; i < len; i++)
        if (name[i] == ':' || name[i] == '/')
            return fail(err, errlen, "%.*s: bad name", (int)len, name);
    if (bm_alloc(h, &n))
        return fail(err, errlen, "no room");
    b = blk(h, n);
    memset(b, 0, HDF_BLOCK);
    wr32(b + O_TYPE, T_HEADER);
    wr32(b + O_KEY, n);
    memcpy(b + O_DAYS, date, 12);
    b[O_NAME] = (uint8_t)len;
    memcpy(b + O_NAME + 1, name, len);
    wr32(b + O_PARENT, dir);
    wr32(b + O_SECTYPE, (uint32_t)ST_FILE);
    /* into the hash chain, sorted by block number */
    slot = O_TABLE + name_hash((const uint8_t *)name, len, intl(h)) * 4;
    cur = rd32(d + slot);
    while (cur && cur < n) {
        prevb = cur;
        cur = rd32(blk(h, cur) + O_HASHCHAIN);
        if (++guard > h->nblocks)
            return fail(err, errlen, "hash chain loop in directory %u", dir);
    }
    wr32(b + O_HASHCHAIN, cur);
    sum_set(b, O_CHKSUM);
    p = prevb ? blk(h, prevb) : d;
    wr32(p + (prevb ? O_HASHCHAIN : slot), n);
    sum_set(p, O_CHKSUM);
    *out = n;
    return 0;
}

/* write data to path; date: the date for a new file (NULL: the
   directory's), access: its protection bits */
static int write_file(struct hdf *h, const char *path, const uint8_t *data, size_t len,
                      const uint8_t *date, uint32_t access, char *err, size_t errlen)
{
    uint32_t n, dir;
    const char *last;
    uint8_t *b;

    if (lookup(h, path, &n, &dir, &last, err, errlen))
        return -1;
    if (n) {
        if (!(b = header(h, n, err, errlen)))
            return -1;
        if (sectype(b) != ST_FILE)
            return fail(err, errlen, "%s: not a file", path);
    } else {
        if (!*last)
            return fail(err, errlen, "%s: no file name", path);
        if (create(h, dir, last, strlen(last), date ? date : blk(h, dir) + O_DAYS, &n, err,
                   errlen))
            return -1;
        b = blk(h, n);
        wr32(b + O_ACCESS, access);
    }
    return set_contents(h, n, data, len, err, errlen);
}

int hdf_write(struct hdf *h, const char *path, const void *data, size_t len, char *err,
              size_t errlen)
{
    return write_file(h, path, data, len, NULL, 0, err, errlen);
}

static int hexval(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

int hdf_write_hex(struct hdf *h, const char *path, const char *hex, char *err, size_t errlen)
{
    size_t n = strlen(hex), i;
    uint8_t *buf;
    int r;

    if (n % 2)
        return fail(err, errlen, "hex contents of %s: odd number of digits", path);
    buf = malloc(n / 2 + 1);
    if (!buf)
        return fail(err, errlen, "out of memory");
    for (i = 0; i < n; i += 2) {
        int a = hexval(hex[i]), b = hexval(hex[i + 1]);

        if (a < 0 || b < 0) {
            free(buf);
            return fail(err, errlen, "hex contents of %s: bad digit at %zu", path,
                        a < 0 ? i : i + 1);
        }
        buf[i / 2] = (uint8_t)(a << 4 | b);
    }
    r = hdf_write(h, path, buf, n / 2, err, errlen);
    free(buf);
    return r;
}

int hdf_copy(struct hdf *h, const char *from, const char *to, char *err, size_t errlen)
{
    uint32_t n;
    uint8_t *b, *buf, date[12];
    size_t len;
    int r;

    if (lookup(h, from, &n, NULL, NULL, err, errlen))
        return -1;
    if (!(b = header(h, n, err, errlen)))
        return -1;
    if (sectype(b) != ST_FILE)
        return fail(err, errlen, "%s: not a file", from);
    memcpy(date, b + O_DAYS, 12);
    if (read_header(h, n, &buf, &len, err, errlen))
        return -1;
    r = write_file(h, to, buf, len, date, rd32(b + O_ACCESS), err, errlen);
    free(buf);
    return r;
}

/* the first occurrence of n[0..nl) in s[0..sl), NULL if none */
static uint8_t *find(uint8_t *s, size_t sl, const char *n, size_t nl)
{
    size_t i;

    for (i = 0; nl <= sl && i <= sl - nl; i++)
        if (!memcmp(s + i, n, nl))
            return s + i;
    return NULL;
}

int hdf_replace(struct hdf *h, const char *path, const char *old, const char *repl, char *err,
                size_t errlen)
{
    uint8_t *buf, *nb, *at;
    size_t len, ol = strlen(old), rl = strlen(repl), pos;
    int r;

    if (!ol)
        return fail(err, errlen, "%s: empty text to replace", path);
    if (hdf_read(h, path, &buf, &len, err, errlen))
        return -1;
    at = find(buf, len, old, ol);
    if (!at || find(at + 1, len - (size_t)(at + 1 - buf), old, ol)) {
        free(buf);
        return fail(err, errlen, "%s: the text to replace occurs %s", path,
                    at ? "more than once" : "not");
    }
    pos = (size_t)(at - buf);
    nb = malloc(len - ol + rl + 1);
    if (!nb) {
        free(buf);
        return fail(err, errlen, "out of memory");
    }
    memcpy(nb, buf, pos);
    memcpy(nb + pos, repl, rl);
    memcpy(nb + pos + rl, at + ol, len - pos - ol);
    r = hdf_write(h, path, nb, len - ol + rl, err, errlen);
    free(nb);
    free(buf);
    return r;
}

/* ---- checking ---- */

struct check {
    struct hdf *h;
    uint8_t *used;
    char *err;
    size_t errlen;
};

static int use(struct check *c, uint32_t n, const char *what)
{
    if (n < c->h->reserved || n >= c->h->nblocks)
        return fail(c->err, c->errlen, "%s block %u is outside the partition", what, n);
    if (c->used[n])
        return fail(c->err, c->errlen, "%s block %u is used twice", what, n);
    c->used[n] = 1;
    return 0;
}

static int check_dir(struct check *c, uint32_t dir, unsigned depth)
{
    struct hdf *h = c->h;
    uint8_t *d = blk(h, dir), *e;
    uint32_t n, *db, *xb;
    size_t i, k, nd, nx;
    int32_t st;
    uint8_t *buf;

    if (depth > 100)
        return fail(c->err, c->errlen, "directories nested too deep");
    for (i = 0; i < HDF_HT; i++) {
        for (n = rd32(d + O_TABLE + i * 4); n; n = rd32(e + O_HASHCHAIN)) {
            if (use(c, n, "header"))
                return -1;
            if (!(e = header(h, n, c->err, c->errlen)))
                return -1;
            if (rd32(e + O_KEY) != n || rd32(e + O_PARENT) != dir || !e[O_NAME]
                || e[O_NAME] > HDF_NAMEMAX || name_hash(e + O_NAME + 1, e[O_NAME], intl(h)) != i)
                return fail(c->err, c->errlen, "header block %u does not fit directory %u", n, dir);
            st = sectype(e);
            if (st == ST_USERDIR) {
                if (check_dir(c, n, depth + 1))
                    return -1;
            } else if (st == ST_FILE) {
                if (file_blocks(h, n, &db, &nd, &xb, &nx, c->err, c->errlen))
                    return -1;
                for (k = 0; k < nd + nx; k++)
                    if (k < nd ? use(c, db[k], "data") : use(c, xb[k - nd], "extension"))
                        break;
                free(db);
                free(xb);
                if (k != nd + nx)
                    return -1;
                if (!is_ffs(h)) {
                    if (read_header(h, n, &buf, &k, c->err, c->errlen))
                        return -1;
                    free(buf);
                }
            } else if (st != ST_SOFTLINK && st != ST_LINKDIR && st != ST_LINKFILE) {
                return fail(c->err, c->errlen, "header block %u: unknown type %d", n, st);
            }
        }
    }
    return 0;
}

int hdf_check(struct hdf *h, char *err, size_t errlen)
{
    struct check c = { h, calloc(h->nblocks, 1), err, errlen };
    uint8_t *root = blk(h, h->root), *e;
    uint32_t n, ext;
    size_t i;
    int r = -1;

    if (!c.used)
        return fail(err, errlen, "out of memory");
    if (use(&c, h->root, "root"))
        goto out;
    for (i = 0; i < h->nbm; i++)
        if (use(&c, h->bm[i], "bitmap"))
            goto out;
    for (ext = rd32(root + O_BMEXT), i = BM_PAGES; ext && i < h->nbm; i += BM_LONGS) {
        if (use(&c, ext, "bitmap extension"))
            goto out;
        e = blk(h, ext);
        ext = rd32(e + 508);
    }
    if (check_dir(&c, h->root, 0))
        goto out;
    for (n = h->reserved; n < h->nblocks; n++) {
        if (c.used[n] && bm_free(h, n)) {
            fail(err, errlen, "block %u is used but free in the bitmap", n);
            goto out;
        }
        if (!c.used[n] && !bm_free(h, n)) {
            fail(err, errlen, "block %u is allocated but not used", n);
            goto out;
        }
    }
    r = 0;
out:
    free(c.used);
    return r;
}

int hdf_list(struct hdf *h, const char *path, FILE *out, char *err, size_t errlen)
{
    uint32_t dir, n;
    uint8_t *d, *e;
    size_t i;

    if (lookup(h, path, &dir, NULL, NULL, err, errlen))
        return -1;
    if (!(d = header(h, dir, err, errlen)))
        return -1;
    if (sectype(d) != ST_ROOT && sectype(d) != ST_USERDIR)
        return fail(err, errlen, "%s: not a directory", path);
    for (i = 0; i < HDF_HT; i++)
        for (n = rd32(d + O_TABLE + i * 4); n; n = rd32(e + O_HASHCHAIN)) {
            if (!(e = header(h, n, err, errlen)))
                return -1;
            if (sectype(e) == ST_USERDIR)
                fprintf(out, "%.*s/\n", e[O_NAME], (const char *)e + O_NAME + 1);
            else
                fprintf(out, "%.*s %u\n", e[O_NAME], (const char *)e + O_NAME + 1,
                        rd32(e + O_BYTESIZE));
        }
    return 0;
}

int hdf_format(uint8_t *data, size_t size, uint32_t dostype, const char *name, uint32_t days,
               char *err, size_t errlen)
{
    uint32_t nblocks, root, nbm, i, n;
    size_t len = strlen(name);
    uint8_t *r, *b;

    if (size % HDF_BLOCK || size < 8 * HDF_BLOCK || size / HDF_BLOCK > 0xFFFFFFFFu)
        return fail(err, errlen, "size %zu is no multiple of 512 or too small", size);
    if ((dostype & 0xFFFFFF00) != 0x444F5300 || (dostype & 0xFF) > 3)
        return fail(err, errlen, "DOS type %08x is not supported", dostype);
    if (!len || len > HDF_NAMEMAX)
        return fail(err, errlen, "bad volume name");
    nblocks = (uint32_t)(size / HDF_BLOCK);
    root = (nblocks - 1 + 2) / 2;
    nbm = (nblocks - 2 + BM_BITS - 1) / BM_BITS;
    if (nbm > BM_PAGES || root + nbm >= nblocks)
        return fail(err, errlen, "image too large for a bitmap without extension");
    memset(data, 0, size);
    wr32(data, dostype);
    r = data + (size_t)root * HDF_BLOCK;
    wr32(r + O_TYPE, T_HEADER);
    wr32(r + O_HTSIZE, HDF_HT);
    wr32(r + O_BMFLAG, 0xFFFFFFFF);
    for (i = 0; i < nbm; i++)
        wr32(r + O_BMPAGES + i * 4, root + 1 + i);
    wr32(r + O_DAYS, days);
    wr32(r + O_DISKDAYS, days);
    wr32(r + O_CREATEDAYS, days);
    r[O_NAME] = (uint8_t)len;
    memcpy(r + O_NAME + 1, name, len);
    wr32(r + O_SECTYPE, ST_ROOT);
    sum_set(r, O_CHKSUM);
    for (n = 2; n < nblocks; n++) {
        if (n >= root && n <= root + nbm)
            continue;
        i = n - 2;
        b = data + (size_t)(root + 1 + i / BM_BITS) * HDF_BLOCK + 4 + (size_t)(i % BM_BITS / 32) * 4;
        wr32(b, rd32(b) | 1u << (i % 32));
    }
    for (i = 0; i < nbm; i++)
        sum_set(data + (size_t)(root + 1 + i) * HDF_BLOCK, 0);
    return 0;
}
