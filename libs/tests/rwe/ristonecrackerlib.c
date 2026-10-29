/*
 * rwe ristonecrackerlib - STCLength, STCStart, STCSafeLen, STCLoad: the C
 * build against the CD original (BSS2 RISTONECRACKERLIB.OBJ), method of
 * rwecmp.h.
 *
 *   ristonecrackerlib DIR     (DIR with ristonecrackerlib .obj/.orig)
 *
 * StoneCracker 4.04 ("S404") files are made here: a random sequence of
 * the decruncher's codes (literal bytes, literal runs of 14..45 bytes,
 * matches of 2..3, 4..7, 8..22 and 23+ bytes - the last with 255
 * continuation bytes - at near (5 bits), middle (9 bits) and far
 * distances (0..16 bits, the header's word)) is written as the bit
 * stream the decruncher reads (MSB first, words backwards from the
 * trailer), and the data it must give are worked out at the same time.
 * STCLoad loads the file from RAM: and unpacks it in place; compared
 * are the result, the whole buffer (with the expected data and between
 * the sets), the header functions and the registers the original keeps.
 * Covered: every code of the decruncher, bit buffers of 0..16 bits at
 * the start, literal bytes and runs on every bit position (aligned and
 * across words), the three header functions on S404 data and on other
 * data (0, the S404 check of the binary), STCLoad with a missing file and
 * with a directory (OpenFromLock fails), and with a file that is no S404
 * file (BUG(orig): unpacked anyway - a harmless one: offset and lengths
 * chosen so).  Not covered: corrupt data (writes anywhere), more than 16
 * far offset bits (the DIFF of the run table), a far offset that is
 * negative as a word (16 bits >= $8000: reads before the destination).
 */
#include "rwecmp.h"
#include <exec/memory.h>

#define LIB "ristonecrackerlib"
#define MAXOUT 20000
#define BUFSZ (2 * MAXOUT + 16384)
#define FNAME "RAM:rwe_stc.stc"

static UBYTE out[MAXOUT];               /* the data the file must give */
static UBYTE *buf[2];                   /* STCLoad's buffer per set */
static UBYTE file[BUFSZ];               /* the S404 file */
static ULONG filelen;

/* ------------------------------------------------- bit stream writer */

static UWORD words[BUFSZ / 2];          /* in read order */
static ULONG nwords;
static ULONG acc;                       /* bits of the current word */
static int nacc;
static int firstbits;                   /* bits of the first (partial) word */
static ULONG nbits;                     /* bits written */

static void putbit(int b)
{
    acc = acc << 1 | (b & 1);
    nacc++;
    nbits++;
    if ((nwords == 0 && nacc == firstbits) || nacc == 16) {
        words[nwords++] = (UWORD)(acc << (16 - nacc));
        acc = 0;
        nacc = 0;
    }
}

static void putbits(ULONG v, int n)
{
    while (n-- > 0)
        putbit((int)(v >> n) & 1);
}

/* ---------------------------------------------------------- the codes */

static ULONG N, pos;                    /* data length, bytes left to make */
static int d2bits;                      /* offset bits of far matches */

static void lit(void)
{
    UBYTE b = (UBYTE)rnd();
    putbit(0);
    putbits(b, 8);
    out[--pos] = b;
}

static void run(ULONG n)                /* 14..45 literals */
{
    putbits(0x9, 4);
    putbits(15, 4);
    putbits(n - 14, 5);
    while (n--) {
        UBYTE b = (UBYTE)rnd();
        putbits(b, 8);
        out[--pos] = b;
    }
}

/* a match of `len` bytes from `dist` bytes above */
static void match(ULONG len, ULONG dist)
{
    ULONG i;
    if (len <= 3) {
        putbits(3, 2);
        putbit((int)len - 2);
    } else if (len <= 7) {
        putbits(5, 3);
        putbits(len - 4, 2);
    } else if (len <= 22) {
        putbits(9, 4);
        putbits(len - 8, 4);
    } else {
        ULONG r = len - 23;
        putbits(8, 4);
        while (r >= 255) {
            putbits(255, 8);
            r -= 255;
        }
        putbits(r, 8);
    }
    if (dist <= 32) {
        putbits(1, 2);
        putbits(dist - 1, 5);
    } else if (dist <= 544) {
        putbits(0, 2);
        putbits(dist - 33, 9);
    } else {
        putbit(1);
        putbits(dist - 545, d2bits);
    }
    for (i = 0; i < len; i++, pos--)
        out[pos - 1] = out[pos - 1 + dist];
}

static ULONG pick(ULONG lo, ULONG hi)
{
    return lo + rnd() % (hi - lo + 1);
}

/* the data of N bytes, made by a random code sequence: `kinds` the codes
   allowed (bit 0 literal, 1 run, 2 short, 3 near ... see below) */
static void make(ULONG n, int firstb, int d2, ULONG offset, unsigned kinds)
{
    ULONG made;

    N = pos = n;
    d2bits = d2;
    nwords = 0;
    acc = 0;
    nacc = 0;
    nbits = 0;
    firstbits = firstb;
    if (firstb == 0)                    /* d7 = 0: the first word is unused */
        words[nwords++] = 0xa5a5;
    while (pos > 0) {
        unsigned k = rnd() % 8;
        made = N - pos;
        if (!(kinds & (1u << k)))
            continue;
        if (k == 0 || made == 0 || pos < 2) {
            lit();
        } else if (k == 1) {
            if (pos >= 14)
                run(pick(14, pos < 45 ? pos : 45));
        } else {
            ULONG maxlen = pos, len, dist, lo, hi;
            switch (k) {
            case 2: len = pick(2, 3); break;
            case 3: len = pick(4, 7); break;
            case 4: len = pick(8, 22); break;
            case 5: len = pick(23, 300); break;
            case 6: len = 23 + 255 * pick(1, 3) + pick(0, 254); break;
            default: len = pick(2, 30); break;
            }
            if (len > maxlen)
                len = maxlen;
            /* distance class by k: near, middle or far */
            switch (rnd() % 3) {
            case 0: lo = 1; hi = 32; break;
            case 1: lo = 33; hi = 544; break;
            default: lo = 545; hi = 545 + (d2 ? (1UL << d2) - 1 : 0); break;
            }
            if (d2 == 16 && hi > 545 + 0x7fff)
                hi = 545 + 0x7fff;
            if (hi > made)
                hi = made;
            if (lo > hi)
                continue;
            dist = pick(lo, hi);
            match(len, dist);
        }
    }
    if (nacc)                           /* the rest of the last word */
        words[nwords++] = (UWORD)(acc << (16 - nacc));
    /* the file: S404, offset, unpacked length, packed length, the words
       (the last read lowest), the trailer d2, first word, its bits */
    {
        ULONG i, p, plen = 4 + 2 * (nwords - 1);
        memset(file, 0, sizeof file);
        memcpy(file, "S404", 4);
        *(ULONG *)(file + 4) = offset;
        *(ULONG *)(file + 8) = N;
        *(ULONG *)(file + 12) = plen;
        p = 12 + plen;                  /* the trailer */
        for (i = 1; i < nwords; i++)
            *(UWORD *)(file + p - 2 * i) = words[i];
        *(UWORD *)(file + p) = (UWORD)d2;
        *(UWORD *)(file + p + 2) = words[0];
        *(UWORD *)(file + p + 4) = (UWORD)firstb;
        filelen = p + 6;
    }
}

/* ------------------------------------------------------------ the tests */

static void savefile(const UBYTE *d, ULONG len)
{
    BPTR fh = Open((STRPTR)FNAME, MODE_NEWFILE);
    if (fh) {
        Write(fh, (APTR)d, (LONG)len);
        Close(fh);
    }
    check("STC file written", fh != 0);
}

static void headers(const UBYTE *a[2])
{
    static const char *rn[] = { "STCLength", "STCStart", "STCSafeLen" };
    int e;
    for (e = 0; e < 3; e++) {
        ROUT = rn[e];
        DO2(ENT(LIB, e, 0), R[cur].d[0] = (ULONG)a[cur]);
        SAME("d0", e == 1 && R[cur].d[0] ? R[cur].d[0] - (ULONG)a[cur] : R[cur].d[0]);
        if (e == 1)
            BOTH("STCStart = 0 or address + offset", R[cur].d[0] == 0
                 || R[cur].d[0] == (ULONG)a[cur] + *(const ULONG *)(a[cur] + 4));
    }
}

/* STCLoad FNAME into buf (filled with `fill` first); `expect`: the data
   must unpack to out[] */
static void load(int expect, UBYTE fill)
{
    static const char name[] = FNAME;
    ULONG off = *(ULONG *)(file + 4);
    int s;
    for (s = 0; s < 2; s++)
        memset(buf[s], fill, BUFSZ);
    ROUT = "STCLoad";
    DO2(ENT(LIB, 3, 0), R[cur].d[0] = (ULONG)name; R[cur].d[1] = (ULONG)buf[cur]);
    SAME("d0", R[cur].d[0] ? R[cur].d[0] - (ULONG)buf[cur] : 0);
    SAMEBYTES("buffer", buf, BUFSZ);
    if (expect) {
        BOTH("d0 = start", R[cur].d[0] == (ULONG)buf[cur] + off);
        BOTH("data", memcmp(buf[cur] + off, out, N) == 0);
        BOTH("header kept", memcmp(buf[cur], file, 16) == 0);
    }
}

int main(int argc, char **argv)
{
    static const char *names[] = { LIB, NULL };
    static const UBYTE notstc[16] = "S403\0\0\0\x10\0\0\0\x20\0\0\0\x04";
    const UBYTE *a[2];
    int s, i;

    if (argc < 2) {
        printf("usage: ristonecrackerlib DIR\n");
        return 20;
    }
    if (!load_sets(argv[1], names))
        return 20;
    for (s = 0; s < 2; s++) {
        buf[s] = AllocMem(BUFSZ, MEMF_PUBLIC);
        /* init: dos.library */
        markregs(&R[s]);
        cur = s;
        callstk(entry_form(getlib(&sets[s], LIB), 4, 0), &R[s], NULL, 0, &POP[s]);
    }

    /* the header functions on other data: 0 (the binary's checks) */
    strcpy(VEC, "S403");
    for (s = 0; s < 2; s++)
        a[s] = notstc;
    headers(a);

    /* unpacking: random code sequences */
    for (i = 0; i < 40; i++) {
        static const unsigned kindset[] = { 0xff, 0x01, 0x03, 0x05, 0x09, 0x11, 0x21, 0x41, 0x81 };
        ULONG n = i < 9 ? 200 + 37 * i : pick(1, MAXOUT);
        int firstb = i % 17;
        int d2 = i % 3 == 0 ? 16 : (int)pick(0, 15);
        unsigned kinds = kindset[i % 9];
        ULONG off;
        make(n, firstb, d2, 0, kinds);
        off = (filelen + 3) & ~3UL;     /* above the packed data */
        if (i & 1)
            off += pick(0, 99);
        *(ULONG *)(file + 4) = off;
        if (off + N > BUFSZ) {
            check("test data fits", FALSE);
            continue;
        }
        sprintf(VEC, "#%d: %lu bytes, %lu words, d7 %d, d2 %d, kinds $%X, offset %lu", i, N,
                nwords, firstb, d2, kinds, off);
        for (s = 0; s < 2; s++)
            a[s] = file;
        headers(a);
        savefile(file, filelen);
        load(1, (UBYTE)(i * 13));
    }

    /* no S404 file, unpacked anyway (BUG(orig)): a header whose words make
       a short literal stream inside the buffer */
    strcpy(VEC, "S403 file");
    make(100, 16, 4, 0, 0x01);
    memcpy(file, "S403", 4);
    *(ULONG *)(file + 4) = (filelen + 3) & ~3UL;
    savefile(file, filelen);
    load(1, 0x11);

    strcpy(VEC, "missing file");
    DeleteFile((STRPTR)FNAME);
    load(0, 0x22);

    strcpy(VEC, "a directory");
    {
        static const char dir[] = "RAM:";
        ROUT = "STCLoad";
        DO2(ENT(LIB, 3, 0), R[cur].d[0] = (ULONG)dir; R[cur].d[1] = (ULONG)buf[cur]);
        SAME("d0", R[cur].d[0]);
    }

    for (s = 0; s < 2; s++)
        FreeMem(buf[s], BUFSZ);
    return rwe_done(LIB);
}
