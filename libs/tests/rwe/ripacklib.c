/*
 * rwe ripacklib - the C build of ripacklib (userlib 73) against the CD
 * binary (BUM10A RIPACKLIB.OBJ), in the emulator (snitch test case
 * "rwe ripacklib", argv[1] = the directory with the library files).
 *
 * Both libraries are loaded like the compiler does (sysload.h) with
 * memlib (ILBMPalette's globalloc/globfree), and every routine is called
 * on both with the same inputs (other registers = markers).  Compared are
 * d0, the output buffers (filled the same before every call), the palette
 * and bitmap objects, and every register the original keeps must be kept
 * by the C build.
 *
 * The data is built here: ByteRun1 by PackPBM itself and by a small
 * packer (UnPackPBM, BODY chunks), ILBM files with BMHD/CMAP/BODY, and
 * Pack-Ice 2.40 data from an encoder written against the decruncher
 * (literal runs of every length class, strings of every length and
 * offset class, runs, the picture mode).  The ICE results are also
 * checked against the plain data (that checks the encoder).
 *
 * LoadIFF calls RIAmosFuncLib (NextBank, BLoad, Start, Erase): the
 * library is loaded too - the C build with the C set, the AmiBlitz3
 * binary (the reference of the port; its token list is the one of the
 * BSS2 binary the original ripacklib was built against) with the
 * original - and initialised after memlib.  LoadIFF reads an ILBM of
 * this test from RAM: into bank 51; compared are the bitmap, the palette
 * and the bank afterwards (free again, NextBank(51)); with RIAmosFuncLib's
 * default of 50 banks there is no bank 51 (0, BUG(orig)), so the test
 * raises it with BankLimit 60.
 */
#include "sysload.h"
#include "amosfunc.h"

extern UWORD callstk(APTR fn, struct regs *r, const ULONG *args, ULONG nargs, ULONG *popped);

/* ------------------------------------------------------------ framework */

static int cur;
static struct regs R[2], IN[2];
static ULONG POP[2];
static const char *ROUT;
static char VEC[100];
static int prints;

static void fail(const char *what, ULONG o, ULONG c)
{
    char buf[200];
    if (prints++ < 60)
        printf("FAIL %s %s %s: orig $%08lX, C $%08lX\n", ROUT, VEC, what, o, c);
    snprintf(buf, sizeof buf, "%s %s %s", ROUT, VEC, what);
    check(buf, FALSE);
}

#define SAME(what, expr) do {                                               \
        ULONG v_[2];                                                        \
        for (cur = 0; cur < 2; cur++) v_[cur] = (ULONG)(expr);              \
        if (v_[0] != v_[1]) fail(what, v_[0], v_[1]); else checks++;        \
    } while (0)

/* the same bytes in both sets (a SAME of a memcmp would compare the
   result with itself) */
static void samebytes(const char *what, const void *o, const void *c, ULONG n)
{
    const UBYTE *a = o, *b = c;
    ULONG i;
    for (i = 0; i < n && a[i] == b[i]; i++)
        ;
    if (i < n) {
        char buf[60];
        snprintf(buf, sizeof buf, "%s (offset %lu)", what, i);
        fail(buf, a[i], b[i]);
    } else
        checks++;
}

static void preserve(void)
{
    struct call c[2];
    char buf[140];
    int s;
    for (s = 0; s < 2; s++) {
        c[s].in = IN[s];
        c[s].out = R[s];
    }
    sprintf(buf, "%s %s", ROUT, VEC);
    cmp_preserve(buf, &c[0], &c[1]);
    SAME("popped", POP[cur]);
}

#define DO2(fn, setup) do {                                                 \
        for (cur = 0; cur < 2; cur++) {                                     \
            markregs(&R[cur]);                                              \
            setup;                                                          \
            IN[cur] = R[cur];                                               \
            callstk(fn, &R[cur], NULL, 0, &POP[cur]);                       \
        }                                                                   \
        preserve();                                                         \
    } while (0)

static UBYTE *ent(int idx, int form)
{
    return entry_form(getlib(&sets[cur], "ripacklib"), idx, form);
}

enum { PACKPBM, DEICE, UNPACKIFF, ILBMPALETTE, ILBMGRAB, LOADIFF, CHUNKHEADER, UNPACKPBM,
       FINIT, INIT };

static ULONG seed = 0x2468ace1;
static ULONG rnd(void)
{
    seed = seed * 1103515245UL + 12345;
    return (seed >> 16) | ((seed * 69069UL + 1) & 0xffff0000UL);
}

/* test data: plain bytes with runs and repeats */
static void mkdata(UBYTE *p, ULONG n, int kind)
{
    ULONG i = 0;
    while (i < n) {
        ULONG r = rnd(), len = 1 + (r >> 8) % (kind == 2 ? 300 : 40), k;
        int what = kind == 0 ? 0 : (int)(r % 4);
        for (k = 0; k < len && i < n; k++, i++) {
            switch (what) {
            case 0: p[i] = (UBYTE)rnd(); break;                     /* noise */
            case 1: p[i] = (UBYTE)(r >> 24); break;                 /* a run */
            case 2: p[i] = i >= 300 ? p[i - 1 - (r >> 4) % 290] : (UBYTE)k; break;
            default: p[i] = "Blitz Basic 2 "[(i + (r >> 3)) % 14]; break;
            }
        }
    }
}

/* ------------------------------------------------------------- buffers */

#define BUFSZ 8192
static UBYTE src[BUFSZ + 64];           /* input data, the same for both sets */
static UBYTE out[2][BUFSZ + 64];        /* output, per set */

static void clearout(void)
{
    memset(out[0], 0xEE, sizeof out[0]);
    memset(out[1], 0xEE, sizeof out[1]);
}

/* ------------------------------------------------------- ChunkHeader */

static void test_chunkheader(void)
{
    static const char *strs[] = { "FORM", "ILBM....", "ab\0\0", "\xff\x80\x01\x02" };
    unsigned i;

    ROUT = "ChunkHeader";
    for (i = 0; i < sizeof strs / sizeof strs[0]; i++) {
        sprintf(VEC, "%u", i);
        DO2(ent(CHUNKHEADER, 0), R[cur].d[0] = (ULONG)strs[i]);
        SAME("d0", R[cur].d[0]);
    }
}

/* ------------------------------------------------- PackPBM, UnPackPBM */

static ULONG packed[2];

static void packpbm(const char *what, ULONG n, ULONG d3)
{
    ROUT = "PackPBM";
    sprintf(VEC, "%s %lu bytes", what, n);
    clearout();
    DO2(ent(PACKPBM, 0), R[cur].d[0] = (ULONG)src; R[cur].d[1] = (ULONG)out[cur] + 16;
        R[cur].d[2] = n; R[cur].d[3] = d3);
    SAME("d0", R[cur].d[0]);
    samebytes("dest", out[0], out[1], sizeof out[0]);
    for (cur = 0; cur < 2; cur++)
        packed[cur] = R[cur].d[0];
}

static UBYTE back[2][BUFSZ + 64];

/* UnPackPBM of out[] (or of src) into back[] */
static void unpackpbm(const char *what, const UBYTE *from0, const UBYTE *from1, ULONG len,
                      ULONG dlen)
{
    ROUT = "UnPackPBM";
    sprintf(VEC, "%s %lu -> %lu", what, len, dlen);
    memset(back[0], 0x55, sizeof back[0]);
    memset(back[1], 0x55, sizeof back[1]);
    DO2(ent(UNPACKPBM, 0), R[cur].d[0] = (ULONG)(cur ? from1 : from0);
        R[cur].d[1] = (ULONG)back[cur] + 8; R[cur].d[2] = len; R[cur].d[3] = dlen);
    samebytes("dest", back[0], back[1], sizeof back[0]);
}

static void test_pbm(void)
{
    static const ULONG sizes[] = { 0, 1, 2, 3, 5, 127, 128, 129, 200, 1000, 4000 };
    static const UBYTE runs[] = {               /* runs and literals around 127/128 */
        1, 1, 2, 3, 3, 3, 4, 5, 6, 6, 7, 7, 7, 7, 8
    };
    unsigned i;
    ULONG n;

    /* runs of 2 .. 300 equal bytes, literal stretches of 1 .. 300 */
    for (i = 0; i < sizeof sizes / sizeof sizes[0]; i++) {
        seed = 0x1000 + i;
        mkdata(src, sizes[i], 1);
        packpbm("mixed", sizes[i], 0);
        if (packed[0] != (ULONG)-1)
            unpackpbm("mixed", out[0] + 16, out[1] + 16, packed[0], sizes[i] + 4);
        SAME("round trip", packed[cur] == (ULONG)-1 || memcmp(back[cur] + 8, src, sizes[i]) == 0);
    }
    for (n = 120; n <= 140; n += 4) {
        memset(src, 'x', n);
        packpbm("one run", n, 0);
        unpackpbm("one run", out[0] + 16, out[1] + 16, packed[0], n);
    }
    memcpy(src, runs, sizeof runs);
    packpbm("short runs", sizeof runs, 0);
    for (i = 0; i < 300; i++)
        src[i] = (UBYTE)(i < 129 ? i : i < 260 ? 0xAA : i * 7);
    packpbm("129 literals, 131 equal", 300, 0);
    unpackpbm("129 literals, 131 equal", out[0] + 16, out[1] + 16, packed[0], 300);

    /* not smaller: -1, BUG(orig) - then UnPackPBM from address -1 into
       dest, as far as the caller's d3 allows */
    seed = 77;
    mkdata(src, 600, 0);
    packpbm("noise (fails), d3 0", 600, 0);
    SAME("d0 = -1", R[cur].d[0] == (ULONG)-1);
    packpbm("noise (fails), d3 4", 600, 4);
    packpbm("noise (fails), d3 -8", 600, (ULONG)-8);
    src[0] = 1; src[1] = 2;
    packpbm("2 different (fails)", 2, 0);

    /* UnPackPBM: no-ops, runs, a short destination (one byte too many) */
    {
        static const UBYTE pk[] = { 0x80, 2, 'a', 'b', 'c', 0xfe, 'R', 0x80, 0x81, '*', 0, 'z',
                                    0x80, 5, '1', '2', '3', '4', '5', '6' };
        memcpy(src, pk, sizeof pk);
        unpackpbm("crafted", src, src, sizeof pk, 200);
        unpackpbm("crafted, dest 3", src, src, sizeof pk, 3);
        unpackpbm("crafted, dest 0", src, src, sizeof pk, 0);
        unpackpbm("crafted, source 5", src, src, 5, 200);
        unpackpbm("crafted, source 0", src, src, 0, 200);
    }
}

/* ------------------------------------------------------- Pack-Ice data */

/* the encoder writes the stream backwards (the decruncher reads it from
   the end): bit bytes with a marker, literal bytes in between */
struct enc {
    UBYTE *buf;
    LONG p;                             /* next byte at --p */
    LONG bytepos;                       /* the current bit byte */
    int nbits, cap;
};

static void e_init(struct enc *e, UBYTE *buf, LONG size)
{
    e->buf = buf;
    e->p = size;
    e->bytepos = --e->p;
    e->buf[e->bytepos] = 1;             /* marker: 7 data bits */
    e->nbits = 0;
    e->cap = 7;
}

static void e_bit(struct enc *e, int b)
{
    if (e->nbits == e->cap) {
        e->bytepos = --e->p;
        e->buf[e->bytepos] = 0;
        e->nbits = 0;
        e->cap = 8;
    }
    if (b)
        e->buf[e->bytepos] |= (UBYTE)(0x80 >> e->nbits);
    e->nbits++;
}

static void e_bits(struct enc *e, ULONG v, int n)
{
    while (n--)
        e_bit(e, (int)(v >> n) & 1);
}

static void e_byte(struct enc *e, UBYTE v)
{
    e->buf[--e->p] = v;
}

/* a literal run of the bytes plain[pos-1] down to plain[pos-len] */
static void e_literals(struct enc *e, const UBYTE *plain, ULONG pos, ULONG len)
{
    ULONG d1, i;

    if (!len) {
        e_bit(e, 0);
        return;
    }
    e_bit(e, 1);
    d1 = len - 1;
    if (!d1)
        e_bit(e, 0);
    else {
        e_bit(e, 1);
        if (d1 <= 3)
            e_bits(e, d1 - 1, 2);
        else {
            e_bits(e, 3, 2);
            if (d1 <= 6)
                e_bits(e, d1 - 4, 2);
            else {
                e_bits(e, 3, 2);
                if (d1 <= 13)
                    e_bits(e, d1 - 7, 3);
                else {
                    e_bits(e, 7, 3);
                    if (d1 <= 268)
                        e_bits(e, d1 - 14, 8);
                    else {
                        e_bits(e, 255, 8);
                        e_bits(e, d1 - 269, 15);
                    }
                }
            }
        }
    }
    for (i = 1; i <= len; i++)
        e_byte(e, plain[pos - i]);
}

/* a string: len bytes from offset d1 (-1: repeat the byte above) */
static void e_string(struct enc *e, ULONG len, LONG d1)
{
    if (len == 2)
        e_bit(e, 0);
    else if (len == 3)
        e_bits(e, 2, 2);
    else if (len <= 5) {
        e_bits(e, 6, 3);
        e_bit(e, (int)(len - 4));
    } else if (len <= 9) {
        e_bits(e, 14, 4);
        e_bits(e, len - 6, 2);
    } else {
        e_bits(e, 15, 4);
        e_bits(e, len - 10, 10);
    }
    if (len == 2) {
        if (d1 <= 62) {
            e_bit(e, 0);
            e_bits(e, (ULONG)(d1 + 1), 6);
        } else {
            e_bit(e, 1);
            e_bits(e, (ULONG)(d1 - 63), 9);
        }
    } else if (d1 >= 31 && d1 <= 286) {
        e_bit(e, 0);
        e_bits(e, (ULONG)(d1 - 31), 8);
    } else if (d1 <= 30) {
        e_bits(e, 2, 2);
        e_bits(e, (ULONG)(d1 + 1), 5);
    } else {
        e_bits(e, 3, 2);
        e_bits(e, (ULONG)(d1 - 287), 12);
    }
}

/* the longest string for plain[p-len .. p-1] (len <= p): from
   plain[p+d1 ..] (d1 >= 0) or a repeat of plain[p] (d1 = -1).  By the
   distance D = d1 + len between a byte and its source: the run of equal
   pairs from p-1 down, at most D long (d1 >= 0). */
static ULONG e_find(const UBYTE *plain, ULONG n, ULONG p, LONG maxoff, LONG *off)
{
    ULONG best = 0, len, max = p < 1033 ? p : 1033, D;

    if (p < n) {
        for (len = 0; len < max && plain[p - 1 - len] == plain[p]; len++)
            ;
        if (len >= 2) {
            best = len;
            *off = -1;
        }
    }
    for (D = 2; D <= n - p && D <= (ULONG)maxoff + 300; D++) {
        ULONG cap = max < D ? max : D;
        LONG d1;
        for (len = 0; len < cap && plain[p - 1 - len] == plain[p - 1 - len + D]; len++)
            ;
        d1 = (LONG)(D - len);
        if (len >= 2 && len > best && d1 <= maxoff && (len > 2 || d1 <= 574)) {
            best = len;
            *off = d1;
        }
    }
    return best;
}

/* plain[0..n) -> ICE data at pk; returns its length; picture: the
   picture flag (with the 10 bits of the count that come from the data) */
static ULONG ice_pack(const UBYTE *plain, ULONG n, UBYTE *pk, LONG maxoff, int picture, ULONG count10)
{
    static UBYTE st[BUFSZ * 2];
    struct enc e;
    ULONG pos = n, lits = 0, len, size;
    LONG off = 0;

    e_init(&e, st, sizeof st);
    while (pos - lits > 0) {
        ULONG p = pos - lits;
        len = e_find(plain, n, p, maxoff, &off);
        if (len >= 2 && lits < 32000) {
            e_literals(&e, plain, pos, lits);
            pos -= lits;
            lits = 0;
            e_string(&e, len, off);
            pos -= len;
        } else
            lits++;
    }
    e_literals(&e, plain, pos, lits);
    if (picture) {
        e_bit(&e, 1);
        e.nbits = e.cap;                /* the bit buffer is replaced by $9f */
        e_bits(&e, count10, 10);
    } else
        e_bit(&e, 0);
    size = sizeof st - e.p;
    memcpy(pk, "ICE!", 4);
    pk[4] = (UBYTE)((size + 12) >> 24); pk[5] = (UBYTE)((size + 12) >> 16);
    pk[6] = (UBYTE)((size + 12) >> 8); pk[7] = (UBYTE)(size + 12);
    pk[8] = (UBYTE)(n >> 24); pk[9] = (UBYTE)(n >> 16); pk[10] = (UBYTE)(n >> 8); pk[11] = (UBYTE)n;
    memcpy(pk + 12, st + e.p, size);
    return size + 12;
}

static UBYTE plain[BUFSZ];
static UBYTE icebuf[BUFSZ * 2 + 64];

static void deice(const char *what, ULONG n, LONG maxoff, ULONG at)
{
    ULONG size = ice_pack(plain, n, icebuf + at, maxoff, 0, 0);

    ROUT = "DeIce";
    sprintf(VEC, "%s %lu -> %lu bytes", what, n, size);
    clearout();
    DO2(ent(DEICE, 0), R[cur].d[0] = (ULONG)icebuf + at; R[cur].d[1] = (ULONG)out[cur] + 32);
    SAME("d0", R[cur].d[0]);
    samebytes("dest", out[0], out[1], sizeof out[0]);
    check("DeIce: the plain data (encoder)", memcmp(out[0] + 32, plain, n) == 0);
}

static void test_deice(void)
{
    ULONG i;

    for (i = 0; i < 6; i++) {
        seed = 0x4000 + i;
        mkdata(plain, 1000, (int)(i % 3));
        deice(i % 3 == 0 ? "noise" : i % 3 == 1 ? "mixed" : "long repeats", 1000, 300, 0);
    }
    seed = 99;
    mkdata(plain, 2600, 0);
    memcpy(plain + 2000, plain + 200, 500);     /* 12 bit offsets */
    deice("far repeats", 2600, 1500, 1);        /* at an odd address */
    for (i = 0; i < 700; i++)                   /* literal runs of all classes */
        plain[i] = (UBYTE)rnd();
    deice("literals", 700, 0, 0);
    memset(plain, 'A', 1200);
    plain[0] = 'B';
    deice("one long run", 1200, 0, 0);
    deice("one byte", 1, 0, 0);
    plain[0] = plain[1] = 'q';
    deice("two bytes", 2, 0, 0);

    /* not "ICE!" */
    ROUT = "DeIce";
    strcpy(VEC, "no ICE");
    memcpy(icebuf, "ICE?", 4);
    DO2(ent(DEICE, 0), R[cur].d[0] = (ULONG)icebuf; R[cur].d[1] = (ULONG)out[cur]);
    SAME("d0", R[cur].d[0]);

    /* picture mode: 15361+ groups of 8 bytes below the end */
    {
        const ULONG below = 132 * 1024;
        UBYTE *pic[2];
        ULONG size;
        int s;

        seed = 5;
        mkdata(plain, 1000, 1);
        size = ice_pack(plain, 1000, icebuf, 600, 1, 37);
        for (s = 0; s < 2; s++) {
            pic[s] = AllocMem(below + 1100, MEMF_ANY);
            if (pic[s])
                for (i = 0; i < below + 1100; i++)
                    pic[s][i] = (UBYTE)(i * 13 + (i >> 9));
        }
        if (pic[0] && pic[1]) {
            sprintf(VEC, "picture %lu bytes", size);
            DO2(ent(DEICE, 0), R[cur].d[0] = (ULONG)icebuf; R[cur].d[1] = (ULONG)pic[cur] + below);
            SAME("d0", R[cur].d[0]);
            samebytes("dest", pic[0], pic[1], below + 1100);
        } else
            check("picture buffers", FALSE);
        for (s = 0; s < 2; s++)
            if (pic[s])
                FreeMem(pic[s], below + 1100);
    }
}

/* ----------------------------------------------------------- ILBM data */

static ULONG put32(UBYTE *p, ULONG v)
{
    p[0] = (UBYTE)(v >> 24); p[1] = (UBYTE)(v >> 16); p[2] = (UBYTE)(v >> 8); p[3] = (UBYTE)v;
    return 4;
}

static ULONG put16(UBYTE *p, UWORD v)
{
    p[0] = (UBYTE)(v >> 8); p[1] = (UBYTE)v;
    return 2;
}

/* ByteRun1 of one row (the packer of the test; $80 no-ops sometimes) */
static ULONG brun(UBYTE *o, const UBYTE *r, ULONG n)
{
    ULONG i = 0, k = 0;
    while (i < n) {
        ULONG j = i;
        while (j + 1 < n && r[j + 1] == r[i] && j - i < 127)
            j++;
        if (j - i >= 2) {
            o[k++] = (UBYTE)-(LONG)(j - i);
            o[k++] = r[i];
            i = j + 1;
        } else {
            ULONG s = i;
            while (i < n && i - s < 128 && !(i + 2 < n && r[i] == r[i + 1] && r[i] == r[i + 2]))
                i++;
            o[k++] = (UBYTE)(i - s - 1);
            while (s < i)
                o[k++] = r[s++];
        }
        if ((rnd() & 7) == 0)
            o[k++] = 0x80;
    }
    return k;
}

/* an ILBM: width w, height h, nplanes planes, ncols colours; cmap 0: none */
static ULONG mkilbm(UBYTE *f, UWORD w, UWORD h, UBYTE nplanes, ULONG ncols, ULONG lead)
{
    ULONG k = 0, y, pl, i, rowbytes = w / 8, form;
    UBYTE row[256];

    memset(f, 0, lead);
    k = lead;
    form = k;
    k += put32(f + k, 0x464f524d);              /* FORM */
    k += 4;
    k += put32(f + k, 0x494c424d);              /* ILBM */
    k += put32(f + k, 0x424d4844);              /* BMHD */
    k += put32(f + k, 20);
    k += put16(f + k, w);
    k += put16(f + k, h);
    k += put32(f + k, 0);
    f[k++] = nplanes;
    f[k++] = 0;
    f[k++] = 1;
    f[k++] = 0;
    k += put16(f + k, 0);
    f[k++] = 10;
    f[k++] = 11;
    k += put16(f + k, w);
    k += put16(f + k, h);
    if (ncols) {
        k += put32(f + k, 0x434d4150);          /* CMAP */
        k += put32(f + k, ncols * 3);
        for (i = 0; i < ncols * 3; i++)
            f[k++] = (UBYTE)(i * 37 + 5);
        if (k & 1)
            f[k++] = 0;
    }
    k += put32(f + k, 0x424f4459);              /* BODY */
    i = k;
    k += 4;
    for (y = 0; y < h; y++)
        for (pl = 0; pl < nplanes; pl++) {
            ULONG x;
            for (x = 0; x < rowbytes; x++)
                row[x] = (UBYTE)(x < rowbytes / 2 ? y * 16 + pl : rnd() & (pl ? 0x0f : 0xff));
            k += brun(f + k, row, rowbytes);
        }
    put32(f + i, k - i - 4);
    if (k & 1)
        f[k++] = 0;
    put32(f + form + 4, k - form - 8);
    memset(f + k, 0, 64);
    return k;
}

/* bitmap objects: bpr, rows, depth, planes into pb[] */
#define PLANESZ 3000
static UBYTE pb[2][8 * PLANESZ + 64];
static ULONG bmo[2][16];

static void mkbitmap(UWORD bpr, UWORD rows, UBYTE depth, int plane0)
{
    int s, i;
    for (s = 0; s < 2; s++) {
        memset(bmo[s], 0, sizeof bmo[s]);
        bmo[s][0] = (ULONG)bpr << 16 | rows;
        bmo[s][1] = (ULONG)depth << 16;
        for (i = 0; i < 8; i++)
            bmo[s][2 + i] = (ULONG)&pb[s][16 + i * PLANESZ];
        if (!plane0)
            bmo[s][2] = 0;
        memset(pb[s], 0xEE, sizeof pb[s]);
    }
}

static UBYTE ilbm[BUFSZ];

static void unpackiff(const char *what, int form, ULONG lines, ULONG xy)
{
    ROUT = "UnpackIFF";
    sprintf(VEC, "%s form %d lines $%lX xy %lu", what, form, lines, xy);
    DO2(ent(UNPACKIFF, form), R[cur].d[0] = (ULONG)ilbm; R[cur].d[1] = 0x5A001234;
        R[cur].a[1] = (ULONG)bmo[cur]; if (form) { R[cur].d[2] = lines; R[cur].d[3] = xy; });
    SAME("d0", R[cur].d[0]);
    samebytes("planes", pb[0], pb[1], sizeof pb[0]);
}

static void test_unpackiff(void)
{
    seed = 11;
    mkilbm(ilbm, 64, 10, 3, 8, 6);
    mkbitmap(10, 12, 3, 1);
    unpackiff("64x10x3", 0, 0, 0);
    mkbitmap(10, 12, 3, 1);
    unpackiff("64x10x3", 1, 0x5A000004UL, 2);
    mkbitmap(10, 12, 3, 1);
    unpackiff("64x10x3", 1, 0x5A00ffffUL, 0);
    mkbitmap(10, 12, 4, 1);
    unpackiff("64x10x3 more lines", 1, 100, 1);
    mkbitmap(10, 6, 3, 1);
    unpackiff("64x10x3 6 rows", 0, 0, 0);
    mkbitmap(10, 12, 3, 1);
    unpackiff("64x10x3", 1, 0, 0);
    mkbitmap(10, 12, 2, 1);
    unpackiff("64x10x3 depth 2", 0, 0, 0);
    mkbitmap(7, 12, 3, 1);
    unpackiff("64x10x3 bpr 7", 0, 0, 0);
    mkbitmap(10, 12, 3, 0);
    unpackiff("64x10x3 no plane 0", 0, 0, 0);
    mkbitmap(10, 12, 3, 1);
    bmo[0][0] = bmo[1][0] = 8UL << 16 | 12;
    unpackiff("64x10x3 bpr 8", 0, 0, 0);

    seed = 12;
    mkilbm(ilbm, 40, 5, 2, 0, 2);               /* 5 bytes a row, no CMAP */
    mkbitmap(6, 8, 2, 1);
    unpackiff("40x5x2", 0, 0, 0);
    seed = 13;
    mkilbm(ilbm, 1040, 2, 1, 2, 0);             /* 130 bytes: a negative byte count */
    mkbitmap(132, 4, 1, 1);
    unpackiff("1040x2x1", 0, 0, 0);
    seed = 14;
    mkilbm(ilbm, 1016, 2, 1, 2, 0);             /* 127 bytes */
    mkbitmap(128, 4, 1, 1);
    unpackiff("1016x2x1", 0, 0, 0);

    /* a run past the end of the row */
    seed = 15;
    mkilbm(ilbm, 16, 2, 1, 0, 0);
    {
        UBYTE *b = ilbm;
        while (memcmp(b, "BODY", 4))
            b++;
        b[8] = 0xf6;                            /* 11 bytes into a row of 2 */
        b[9] = 0x77;
    }
    mkbitmap(4, 4, 1, 1);
    unpackiff("16x2 run past the row", 0, 0, 0);

    /* no BMHD / no BODY */
    memset(ilbm, 0, 5200);
    mkbitmap(10, 12, 3, 1);
    unpackiff("no BMHD", 0, 0, 0);
    mkilbm(ilbm, 64, 10, 3, 8, 0);
    {
        UBYTE *b = ilbm;
        while (memcmp(b, "BODY", 4))
            b++;
        b[0] = 'X';
    }
    unpackiff("no BODY", 0, 0, 0);
}

/* ------------------------------------------------------------ palettes */

static ULONG palo[2][4];                /* palette objects */

static void samepal(void)
{
    SAME("numcols", *(UWORD *)((UBYTE *)palo[cur] + 4));
    SAME("+6", *(ULONG *)((UBYTE *)palo[cur] + 6));
    SAME("block", palo[cur][0] != 0);
    if (palo[0][0] && palo[1][0]) {
        UWORD n = *(UWORD *)((UBYTE *)palo[0] + 4);
        samebytes("colours", (void *)palo[0][0], (void *)palo[1][0], 4 + 12 * (ULONG)n);
    }
}

static void ilbmpalette(const char *what, int form, ULONG offset)
{
    ROUT = "ILBMPalette";
    sprintf(VEC, "%s form %d offset %ld", what, form, (LONG)offset);
    DO2(ent(ILBMPALETTE, form), R[cur].d[0] = (ULONG)ilbm; R[cur].d[1] = 0x5A001234;
        R[cur].a[1] = (ULONG)palo[cur]; if (form) R[cur].d[2] = offset);
    SAME("d0", R[cur].d[0]);
    samepal();
}

static void test_ilbmpalette(void)
{
    memset(palo, 0, sizeof palo);
    seed = 21;
    mkilbm(ilbm, 32, 2, 3, 8, 0);
    ilbmpalette("8 colours, empty palette", 0, 0);
    seed = 22;
    mkilbm(ilbm, 32, 2, 2, 4, 4);
    ilbmpalette("4 colours", 1, 2);             /* fits: at colour 2 */
    ilbmpalette("4 colours", 1, 0);
    ilbmpalette("4 colours", 1, 4);             /* just fits */
    ilbmpalette("4 colours", 1, 6);             /* too small: a new block, offset lost */
    ilbmpalette("4 colours", 0, 0);             /* always a new block */
    seed = 23;
    mkilbm(ilbm, 32, 2, 1, 2, 2);
    ilbmpalette("2 colours", 1, 1);
    {                                           /* a CMAP of 10 bytes: 3 colours */
        UBYTE *b = ilbm;
        while (memcmp(b, "CMAP", 4))
            b++;
        b[7] = 10;
    }
    ilbmpalette("CMAP 10 bytes", 0, 0);
    memset(ilbm, 0, 5300);
    ilbmpalette("no CMAP", 0, 0);
    ilbmpalette("no CMAP", 1, 3);
}

static void test_ilbmgrab(void)
{
    memset(palo, 0, sizeof palo);
    seed = 31;
    mkilbm(ilbm, 48, 6, 2, 4, 0);
    mkbitmap(6, 8, 2, 1);
    ROUT = "ILBMGrab";
    strcpy(VEC, "48x6x2");
    DO2(ent(ILBMGRAB, 0), R[cur].d[0] = (ULONG)ilbm; R[cur].d[1] = 0x5A000001;
        R[cur].d[2] = 0x5A000002; R[cur].a[0] = (ULONG)bmo[cur]; R[cur].a[1] = (ULONG)palo[cur]);
    samebytes("planes", pb[0], pb[1], sizeof pb[0]);
    samepal();
}

/* ------------------------------------------------- LoadIFF (the banks) */

#define ILBMFILE "RAM:rwe-pack.iff"
static ULONG ilbmlen, liff;              /* LoadIFF's d0 (orig) */

/* a routine of RIAmosFuncLib with d0 = x (other registers markers) */
static ULONG amos(int idx, int form, ULONG x)
{
    markregs(&R[cur]);
    R[cur].d[0] = x;
    callstk(entry_form(getlib(&sets[cur], "riamosfunclib"), idx, form), &R[cur], NULL, 0,
            &POP[cur]);
    return R[cur].d[0];
}

static void writeilbm(ULONG len)
{
    BPTR f = Open((STRPTR)ILBMFILE, MODE_NEWFILE);
    if (f) {
        Write(f, ilbm, (LONG)len);
        Close(f);
    }
}

static void loadiff(const char *name, int form, const char *what)
{
    ULONG len[2], next[2];

    ROUT = "LoadIFF";
    sprintf(VEC, "%s form %d%s", name, form, what);
    mkbitmap(10, 12, 3, 1);
    memset(palo, 0, sizeof palo);
    DO2(ent(LOADIFF, form), R[cur].d[0] = (ULONG)name; R[cur].d[1] = 0x5A000001;
        R[cur].d[2] = 0x5A000002; R[cur].a[0] = (ULONG)bmo[cur]; R[cur].a[1] = (ULONG)palo[cur]);
    SAME("d0", R[cur].d[0]);
    liff = R[0].d[0];
    samebytes("planes", pb[0], pb[1], sizeof pb[0]);
    samepal();
    /* the bank (51) erased again: free, the next free bank from 51 the same */
    for (cur = 0; cur < 2; cur++) {
        len[cur] = amos(AMOS_E_LENGTH, 0, 51);
        next[cur] = amos(AMOS_E_NEXTBANK, 1, 51);
    }
    SAME("bank 51 length", len[cur]);
    check("LoadIFF: bank 51 free afterwards", len[0] == 0);
    SAME("NextBank(51)", next[cur]);
}

static void test_loadiff(void)
{
    seed = 41;
    ilbmlen = mkilbm(ilbm, 64, 10, 3, 8, 0);
    writeilbm(ilbmlen);
    /* BUG(orig): LoadIFF looks for a bank from 51 on, RIAmosFuncLib has 50
       unless BankLimit raised the number: no bank, 0 */
    loadiff(ILBMFILE, 1, ", 50 banks");
    check("LoadIFF: 0 with 50 banks", liff == 0);
    for (cur = 0; cur < 2; cur++)
        amos(AMOS_E_BANKLIMIT, 0, 60);
    loadiff(ILBMFILE, 0, "");
    check("LoadIFF: -1 with 60 banks", liff == (ULONG)-1);
    loadiff(ILBMFILE, 1, "");
    loadiff("RAM:rwe-pack.bad", 1, "");                 /* missing: 0 */
    /* a file that is no ILBM: UnpackIFF fails, LoadIFF says -1 (BUG(orig)) */
    memset(ilbm, 0, 5300);
    writeilbm(5200);
    loadiff(ILBMFILE, 0, "");
    DeleteFile((STRPTR)ILBMFILE);
}

/* ----------------------------------------------------------------- main */

static const char *names[] = { "memlib", "ripacklib", "riamosfunclib", NULL };

int main(int argc, char **argv)
{
    ULONG avail;

    if (argc < 2) {
        printf("usage: rwe_ripacklib DIR\n");
        return 20;
    }
    if (!load_sets(argv[1], names))
        return 20;

    ROUT = "init";
    strcpy(VEC, "");
    DO2(ent(INIT, 0), (void)0);
    test_chunkheader();
    test_pbm();
    test_deice();
    test_unpackiff();

    avail = AvailMem(MEMF_ANY);
    for (cur = 0; cur < 2; cur++) {
        markregs(&R[cur]);
        callstk(entry_form(getlib(&sets[cur], "memlib"), 7, 0), &R[cur], NULL, 0, &POP[cur]);
        amos(AMOS_E_INIT, 0, 0);
    }
    test_ilbmpalette();
    test_ilbmgrab();
    test_loadiff();
    for (cur = 0; cur < 2; cur++) {
        amos(AMOS_E_FINIT, 0, 0);
        markregs(&R[cur]);
        callstk(entry_form(getlib(&sets[cur], "memlib"), 8, 0), &R[cur], NULL, 0, &POP[cur]);
    }
    check("ripacklib: all memory back", AvailMem(MEMF_ANY) == avail);

    ROUT = "finit";
    DO2(ent(FINIT, 0), (void)0);
    printf("%d checks, %d failures\n", checks, failures);
    return failures ? 5 : 0;
}
