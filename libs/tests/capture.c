/*
 * capture - the output of a library test program as cases (capture.h; the
 * format: openspec change libs-tests-capture, design D4 and D6; the raw
 * block observations: change libs-capture-host-blocks, design H1/H2).
 *
 * Portable C: the system is behind capture_env.h (capture_amiga.c in
 * libs_tests, memory in the host test libs_capture_format).  The output
 * goes through static buffers, so that the memory balance checks of the
 * test programs (AvailMem) stay as they are; so does the table of the
 * occurrence numbers.
 */
#include <stdio.h>
#include <string.h>

#include "capture.h"
#include "capture_env.h"

#define OUT_MAX (64 * 1024)
#define BIN_MAX (64 * 1024)             /* the raw block file (H1: more gains nothing) */
#define DUMP_MAX (8 * 1024)
#define KEY_MAX 1024
#define KEYS (1UL << 17)                /* distinct keys of a program */
#define PTRBYTES_MAX (16 * 1024)

struct stream {
    void *f;
    char *buf;
    uint32_t len, size;
    char path[300];
};

static char obuf[OUT_MAX], bbuf[BIN_MAX], dbuf[DUMP_MAX];
static struct stream out = { NULL, obuf, 0, OUT_MAX, "" };
static struct stream bin = { NULL, bbuf, 0, BIN_MAX, "" };
static struct stream dump = { NULL, dbuf, 0, DUMP_MAX, "" };
static int begun, faults, full_reported, long_reported, write_reported;
static char pname[64];

static char rpart[KEY_MAX];             /* the escaped routine */
static char kbuf[KEY_MAX];              /* the key of the current case */
static uint32_t klen;

/* the occurrence numbers: the number of cases of each key part
   ROUTINE|CHECK|VECTOR, by two hashes of it (n = 0: free) */
static struct { uint32_t h1, h2, n; } keys[KEYS];
static uint32_t nkeys;

/* block mode (D6, H1/H2): the observations go raw to NAME.bin, NAME.txt
   gets the place of each block; blkno blocks closed, blkcount
   observations in the open one */
static int blocks, bin_failed, bin_done;
static uint32_t blkno, blkcount;

/* LIBS_CAP_DUMP (a plain bytes case; the blocks are dumped on the host) */
static char dumpkey[KEY_MAX];

static void fault(const char *why)
{
    char m[200];
    snprintf(m, sizeof m, "capture %s: %s", pname, why);
    faults++;
    bl_test_check(m, 0);
}

static uint32_t fnv(uint32_t h, const char *p, uint32_t n)
{
    while (n--) {
        h ^= (unsigned char)*p++;
        h = (h * 0x01000193UL) & 0xffffffffUL;
    }
    return h;
}

#define FNV_BASIS 0x811c9dc5UL

/* ------------------------------------------------------------- files */

static void flush(struct stream *s)
{
    if (s->len && s->f && cap_env_write(s->f, s->buf, s->len) && !write_reported) {
        write_reported = 1;
        fault("cannot write the output");
    }
    s->len = 0;
}

static void sput(struct stream *s, const char *p, uint32_t n)
{
    if (s == &dump && !dump.f) {
        if (!(dump.f = cap_env_create(dump.path))) {
            if (!write_reported) {
                write_reported = 1;
                fault("cannot open the dump file");
            }
            return;
        }
    }
    while (n) {
        uint32_t k = s->size - s->len;
        if (k > n)
            k = n;
        memcpy(s->buf + s->len, p, k);
        s->len += k;
        p += k;
        n -= k;
        if (s->len == s->size)
            flush(s);
    }
}

/* a part of the current line */
static void emit(const char *p, uint32_t n)
{
    sput(&out, p, n);
}

static void emits(const char *s)
{
    emit(s, (uint32_t)strlen(s));
}

/* append the N bytes at S to DST (length *len, size SIZE > 0), cut at
 * SIZE - 1 bytes like snprintf */
static void put_part(char *dst, uint32_t size, uint32_t *len, const char *s, uint32_t n)
{
    if (n > size - 1 - *len)
        n = size - 1 - *len;
    memcpy(dst + *len, s, n);
    *len += n;
}

/* DIR, a "/" unless DIR is empty or ends in ':' or '/', and FILE into DST
 * (size SIZE > 0), cut like snprintf.  Copied with the lengths already
 * taken: with -fsanitize=undefined, gcc keeps a path on which strlen(dir)
 * was handed NULL (the check reports and goes on) and warns about a null
 * %s argument of snprintf there (-Wformat-truncation). */
static void join(char *dst, uint32_t size, const char *dir, const char *file)
{
    uint32_t n = (uint32_t)strlen(dir), len = 0;

    put_part(dst, size, &len, dir, n);
    if (n && dir[n - 1] != ':' && dir[n - 1] != '/')
        put_part(dst, size, &len, "/", 1);
    put_part(dst, size, &len, file, (uint32_t)strlen(file));
    dst[len] = 0;
}

/* ------------------------------------------------------------- the key */

static const char hexdig[] = "0123456789ABCDEF";

/* append the escaped part S (NULL: empty) to D (length *n, size KEY_MAX) */
static void esc_part(char *d, uint32_t *n, const char *s)
{
    char e[4];
    uint32_t k;

    for (; s && *s; s++) {
        unsigned char c = (unsigned char)*s;
        k = 0;
        if (c == '\t') {
            e[k++] = '\\';
            e[k++] = 't';
        } else if (c == '|' || c == '\\') {
            e[k++] = '\\';
            e[k++] = (char)c;
        } else if (c < 0x20 || c > 0x7e) {
            e[k++] = '\\';
            e[k++] = 'x';
            e[k++] = hexdig[c >> 4];
            e[k++] = hexdig[c & 15];
        } else
            e[k++] = (char)c;
        if (*n + k >= KEY_MAX) {
            if (!long_reported) {
                long_reported = 1;
                fault("a key is too long");
            }
            break;
        }
        memcpy(d + *n, e, k);
        *n += k;
    }
    d[*n] = 0;
}

static void key_add(const char *s)
{
    uint32_t n = (uint32_t)strlen(s);
    if (klen + n >= KEY_MAX) {
        if (!long_reported) {
            long_reported = 1;
            fault("a key is too long");
        }
        n = KEY_MAX - 1 - klen;
    }
    memcpy(kbuf + klen, s, n);
    klen += n;
    kbuf[klen] = 0;
}

/* the occurrence number of the key part in kbuf: 1 for its first case */
static uint32_t occurrence(void)
{
    uint32_t h1 = fnv(FNV_BASIS, kbuf, klen), h2 = 5381, i, k;

    for (k = 0; k < klen; k++)
        h2 = (h2 * 33 + (unsigned char)kbuf[k]) & 0xffffffffUL;
    i = h1 & (KEYS - 1);
    while (keys[i].n && (keys[i].h1 != h1 || keys[i].h2 != h2))
        i = (i + 1) & (KEYS - 1);
    if (!keys[i].n) {
        if (nkeys >= KEYS / 8 * 7) {
            if (!full_reported) {
                full_reported = 1;
                fault("too many distinct keys");
            }
            return 0;
        }
        nkeys++;
        keys[i].h1 = h1;
        keys[i].h2 = h2;
    }
    return ++keys[i].n;
}

/* ------------------------------------------------------------- block mode */

/* the raw records of NAME.bin (libscmp.h, design H1): opened with the
   first block of the program */
static void bput(const void *p, uint32_t n)
{
    if (!bin.f && !bin_failed) {
        char h[100];

        if (!(bin.f = cap_env_create(bin.path))) {
            bin_failed = 1;
            if (!write_reported) {
                write_reported = 1;
                fault("cannot open the block file");
            }
        } else {
            snprintf(h, sizeof h, "# capture-bin %s 1\n", pname);
            sput(&bin, h, (uint32_t)strlen(h));
        }
    }
    if (bin.f)
        sput(&bin, p, n);
}

static void b1(uint32_t v)
{
    char c = (char)v;
    bput(&c, 1);
}

static void b4(uint32_t v)
{
    char c[4] = { (char)(v >> 24), (char)(v >> 16), (char)(v >> 8), (char)v };
    bput(c, 4);
}

static void bstr(const char *s)
{
    if (!s)
        s = "";
    bput(s, (uint32_t)strlen(s) + 1);
}

/* the start of an observation record of type t */
static void bobs(uint32_t t, const char *check, const char *vec)
{
    b1(t);
    bstr(check);
    bstr(vec);
}

/* the open block closes: its place in NAME.txt; end: block mode ends
   (record $02 in NAME.bin) */
static void close_block(int end)
{
    char l[120];

    if (!blkcount)
        return;
    if (end)
        b1(0x02);
    blkno++;
    snprintf(l, sizeof l, "BLOCK:%lu\t%lu in %s.bin\n", (unsigned long)blkno,
             (unsigned long)blkcount, pname);
    sput(&out, l, (uint32_t)strlen(l));
    blkcount = 0;
}

/* an observation written: the place of its block once it is full */
static void bobs_end(void)
{
    if (++blkcount == CAP_BLOCK)
        close_block(0);
}

/* "|N" (N decimal) into n (16 bytes): what snprintf "|%lu" writes */
static const char *occ_text(char *n, uint32_t v)
{
    char *p = n + 15;

    *p = 0;
    do {
        *--p = (char)('0' + v % 10);
        v /= 10;
    } while (v);
    *--p = '|';
    return p;
}

/* start a plain case: the key and the TAB */
static void line_begin(const char *check, const char *vec)
{
    char n[16];

    klen = 0;
    key_add(rpart);
    key_add("|");
    esc_part(kbuf, &klen, check);
    key_add("|");
    esc_part(kbuf, &klen, vec);
    key_add(occ_text(n, occurrence()));
    emit(kbuf, klen);
    emit("\t", 1);
}

static void line_end(void)
{
    emit("\n", 1);
}

/* ------------------------------------------------------------- the run */

void cap_begin(const char *dir, const char *name)
{
    char file[80];
    char l[100];

    out.f = bin.f = dump.f = NULL;
    out.len = bin.len = dump.len = 0;
    faults = full_reported = long_reported = write_reported = 0;
    snprintf(pname, sizeof pname, "%s", name);
    rpart[0] = kbuf[0] = 0;
    klen = 0;
    memset(keys, 0, sizeof keys);
    nkeys = 0;
    blocks = bin_failed = bin_done = 0;
    blkno = blkcount = 0;
    dumpkey[0] = 0;
    if (cap_env_getvar("LIBS_CAP_DUMP", dumpkey, sizeof dumpkey) <= 0)
        dumpkey[0] = 0;
    cap_env_find_prog();
    snprintf(file, sizeof file, "%s.txt", name);
    join(out.path, sizeof out.path, dir, file);
    snprintf(file, sizeof file, "%s.bin", name);
    join(bin.path, sizeof bin.path, dir, file);
    snprintf(file, sizeof file, "%s.dump", name);
    join(dump.path, sizeof dump.path, dir, file);
    begun = 1;
    if (!(out.f = cap_env_create(out.path))) {
        cap_env_mkdir(dir);
        out.f = cap_env_create(out.path);
    }
    if (!out.f) {
        write_reported = 1;
        fault("cannot open the output");
    }
    snprintf(l, sizeof l, "# capture %s 1\n", name);
    sput(&out, l, (uint32_t)strlen(l));
}

int cap_end(void)
{
    if (!begun)
        return 0;
    close_block(1);
    flush(&out);
    flush(&bin);
    flush(&dump);
    if (out.f)
        cap_env_close(out.f);
    if (bin.f)
        cap_env_close(bin.f);
    if (dump.f)
        cap_env_close(dump.f);
    out.f = bin.f = dump.f = NULL;
    begun = blocks = 0;
    return faults;
}

void cap_routine(const char *routine)
{
    uint32_t n = 0;
    esc_part(rpart, &n, routine);
    if (begun && blocks) {
        b1(0x01);
        bstr(rpart);
    }
}

void cap_blocks(int on)
{
    if (!begun)
        return;
    if (!on && blocks) {
        /* closed here, so that the file's memory is back before the
           programs' memory balance checks */
        close_block(1);
        flush(&bin);
        if (bin.f)
            cap_env_close(bin.f);
        bin.f = NULL;
        bin_done = 1;
    } else if (on && !blocks) {
        if (bin_done && !bin_failed) {
            bin_failed = 1;
            fault("block mode a second time (one block file per program)");
        }
        b1(0x01);
        bstr(rpart);
    }
    blocks = on;
}

/* ------------------------------------------------------------- the values */

static void hex8(char *d, uint32_t v)
{
    int i;
    for (i = 7; i >= 0; i--, v >>= 4)
        d[i] = hexdig[v & 15];
    d[8] = 0;
}

void cap_num(const char *check, const char *vec, uint32_t v)
{
    char s[12];

    if (!begun)
        return;
    if (blocks) {
        bobs(0x10, check, vec);
        b4(v);
        bobs_end();
        return;
    }
    s[0] = '$';
    hex8(s + 1, v);
    line_begin(check, vec);
    emits(s);
    line_end();
}

/* LIBS_CAP_DUMP names this case: its bytes to the dump file */
static void dump_bytes(const unsigned char *p, uint32_t n)
{
    char l[16];
    uint32_t o, k;

    if (!dumpkey[0] || strcmp(dumpkey, kbuf))
        return;
    for (o = 0; o < n; o += 32) {
        sput(&dump, kbuf, klen);
        snprintf(l, sizeof l, "\tat $%04lX ", (unsigned long)o);
        sput(&dump, l, (uint32_t)strlen(l));
        for (k = o; k < n && k < o + 32; k++) {
            l[0] = hexdig[p[k] >> 4];
            l[1] = hexdig[p[k] & 15];
            sput(&dump, l, 2);
        }
        sput(&dump, "\n", 1);
    }
}

void cap_bytes(const char *check, const char *vec, const void *p, uint32_t n)
{
    const unsigned char *b = p;
    char s[40];
    uint32_t k;

    if (!begun)
        return;
    if (blocks) {
        bobs(0x11, check, vec);
        b4(n);
        bput(p, n);
        bobs_end();
        return;
    }
    line_begin(check, vec);
    if (n <= CAP_BYTES) {
        snprintf(s, sizeof s, n ? "bytes %lu " : "bytes %lu", (unsigned long)n);
        emits(s);
        for (k = 0; k < n; k++) {
            s[0] = hexdig[b[k] >> 4];
            s[1] = hexdig[b[k] & 15];
            emit(s, 2);
        }
    } else {
        snprintf(s, sizeof s, "bytes %lu fnv %08lX", (unsigned long)n,
                 (unsigned long)fnv(FNV_BASIS, p, n));
        emits(s);
    }
    line_end();
    dump_bytes(b, n);
}

void cap_ptrbytes(const char *check, const char *vec, const void *p, uint32_t n)
{
    static unsigned char copy[PTRBYTES_MAX];
    const unsigned char *b = p;
    uint32_t i, v;

    if (!begun)
        return;
    if (n > PTRBYTES_MAX) {
        fault("a record is too long for the pointer normalisation");
        cap_bytes(check, vec, p, n);
        return;
    }
    memcpy(copy, p, n);
    for (i = 0; i + 4 <= n; i++) {
        v = (uint32_t)b[i] << 24 | (uint32_t)b[i + 1] << 16 | (uint32_t)b[i + 2] << 8 | b[i + 3];
        if (cap_env_is_prog(v))
            memcpy(copy + i, "PROG", 4);
    }
    cap_bytes(check, vec, copy, n);
}

void cap_str(const char *check, const char *vec, const char *s)
{
    char e[4];

    if (!begun)
        return;
    if (blocks) {
        bobs(0x13, check, vec);
        b1(s != NULL);
        if (s)
            bstr(s);
        bobs_end();
        return;
    }
    line_begin(check, vec);
    if (!s)
        emits("null");
    else {
        emit("\"", 1);
        for (; *s; s++) {
            unsigned char c = (unsigned char)*s;
            if (c == '"' || c == '\\') {
                e[0] = '\\';
                e[1] = (char)c;
                emit(e, 2);
            } else if (c < 0x20 || c > 0x7e) {
                e[0] = '\\';
                e[1] = 'x';
                e[2] = hexdig[c >> 4];
                e[3] = hexdig[c & 15];
                emit(e, 4);
            } else
                emit(s, 1);
        }
        emit("\"", 1);
    }
    line_end();
}

void cap_ptr(const char *check, const char *vec, uint32_t v, const char *name, uint32_t base)
{
    char s[120];

    if (!begun)
        return;
    if (blocks) {
        bobs(0x14, check, vec);
        if (v && cap_env_is_prog(v))
            b1(0);
        else if (!v || !name) {
            b1(1);
            b4(v);
        } else {
            b1(v >= base ? 2 : 3);
            b4(v >= base ? v - base : base - v);
            bstr(name);
        }
        bobs_end();
        return;
    }
    if (v && cap_env_is_prog(v))
        snprintf(s, sizeof s, "prog");
    else if (!v || !name)
        snprintf(s, sizeof s, "$%08lX", (unsigned long)v);
    else if (v >= base)
        snprintf(s, sizeof s, "%.100s+$%lX", name, (unsigned long)(v - base));
    else
        snprintf(s, sizeof s, "%.100s-$%lX", name, (unsigned long)(base - v));
    line_begin(check, vec);
    emits(s);
    line_end();
}

void cap_regs(const char *check, const char *vec, uint32_t lost)
{
    char s[64];
    uint32_t n = 0;
    int r;

    if (!begun)
        return;
    lost &= 0x7fff;
    if (blocks) {
        char c[2] = { (char)(lost >> 8), (char)lost };

        bobs(0x15, check, vec);
        bput(c, 2);
        bobs_end();
        return;
    }
    if (!lost)
        strcpy(s, "kept");
    else {
        strcpy(s, "lost");
        n = 4;
        for (r = 0; r < 15; r++)
            if (lost & (1UL << r)) {
                s[n++] = ' ';
                s[n++] = r < 8 ? 'd' : 'a';
                s[n++] = (char)('0' + (r & 7));
            }
        s[n] = 0;
    }
    line_begin(check, vec);
    emits(s);
    line_end();
}

void cap_not_captured(const char *check, const char *vec, const char *reason)
{
    char e[4];

    if (!begun)
        return;
    line_begin(check, vec);
    emits("not captured: ");
    for (; reason && *reason; reason++) {
        unsigned char c = (unsigned char)*reason;
        if (c == '\\') {
            emit("\\\\", 2);
        } else if (c < 0x20 || c > 0x7e) {
            e[0] = '\\';
            e[1] = 'x';
            e[2] = hexdig[c >> 4];
            e[3] = hexdig[c & 15];
            emit(e, 4);
        } else
            emit(reason, 1);
    }
    line_end();
}
