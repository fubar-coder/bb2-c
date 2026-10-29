/*
 * s404.h - StoneCracker 4.04 ("S404") files for the tests of
 * ristonecrackerlib and ristcdecrunchlib (libs/tests/rwe/, after
 * rwecmp.h; the includer defines MAXOUT, the longest data, and BUFSZ, the
 * size of the file buffer).
 *
 * make(n, firstb, d2, offset, kinds) writes file[] (filelen bytes): a
 * random sequence of the decruncher's codes (literal bytes, literal runs
 * of 14..45 bytes, matches of 2..3, 4..7, 8..22 and 23+ bytes - the last
 * with 255 continuation bytes - at near (5 bits), middle (9 bits) and far
 * distances (d2 = 0..16 bits, the header's word)) as the bit stream the
 * decruncher reads (MSB first, words backwards from the trailer, the
 * first word with firstb bits), and out[] (N bytes), the data it must
 * give.  kinds: the codes allowed (bit k for case k of make's switch,
 * bit 0 literal, 1 run).
 */
#ifndef S404_H
#define S404_H

static UBYTE out[MAXOUT];               /* the data the file must give */
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


#endif
