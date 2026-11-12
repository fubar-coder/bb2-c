/*
 * rwe blitzbomberslibtwo - the C build of blitzbomberslibtwo (userlib
 * 31) against the reference build of
 * rwelibsrc/blitzbomberslib/blitzbombersTwo00.bb (blitz2 2.10), method
 * of rwecmp.h.
 *
 *   blitzbomberslibtwo DIR     (DIR with blitzbomberslibtwo .obj/.orig)
 *
 * Each set has its own memory with the same contents: sprite images
 * behind a table of image pointers (KillInvader), a star table and a
 * bitplane (DrawStarField), a bitmap object of four 320x256 planes (the
 * bullets).  After every call the whole memory of both sets is compared
 * (and the kept registers).  Covered: KillInvader with x 0..63, y 0..20
 * and offsets -256..511; DrawStarField over 60 frames with stars that
 * wrap at line 236, bit numbers 0..15 (modulo 8) and a speed of 0;
 * DrawBullet/EraseBullet anywhere on the bitmap.  Not covered: offsets
 * that are negative as words (y*224 >= 32768).
 */
#include "rwecmp.h"
#include <exec/memory.h>

#define LIB "blitzbomberslibtwo"
#define IMGSZ 8192                      /* KillInvader: table + images */
#define NSTARS 50
#define PLANESZ 10240                   /* 256 lines of 40 bytes */

enum { KILL, STARS, DRAW, ERASE };

static UBYTE *img[2];                   /* the table at +0, images from +512 */
static UWORD stars[2][3 * NSTARS + 1];
static UBYTE *plane[2];
static UBYTE *bplanes[2][4];
static UBYTE bitm[2][8 + 4 * 4];

static ULONG pick(ULONG a, ULONG b)
{
    return a + rnd() % (b - a + 1);
}

static void sameimg(void)
{
    /* the table (the pointers relative to each set's memory), the
       images */
    const UBYTE *im[2] = { img[0] + 512, img[1] + 512 };
    int o;
    for (o = 0; o < 512; o += 4) {
        ULONG v[2] = { *(ULONG *)(img[0] + o), *(ULONG *)(img[1] + o) };
        if (o % 16 == 0 && o < 16 * 19)
            v[0] -= (ULONG)img[0], v[1] -= (ULONG)img[1];
        /* the pointers are offsets now and the rest data words: numbers,
           which the range test of tape_obs would write as PROG where
           they fall into the test program's memory */
        v[0] = tape_val("table", v[0], v[1]);
        if (v[0] != v[1])
            fail("table", v[0], v[1]);
        else
            checks++;
    }
    SAMEBYTES("images", im, IMGSZ - 512);
}

static void sameplanes(void)
{
    int p;
    for (p = 0; p < 4; p++) {
        const UBYTE *x[2] = { bplanes[0][p], bplanes[1][p] };
        SAMEBYTES("plane", x, PLANESZ);
    }
}

static void kill(UWORD x, UWORD y, WORD off)
{
    ROUT = "KillInvader";
    sprintf(VEC, "x %u y %u offset %d", x, y, off);
    DO2(ENT(LIB, KILL, 0), R[cur].d[0] = (ULONG)img[cur]; R[cur].d[1] = 0x5a5a0000 | x;
        R[cur].d[2] = 0x5a5a0000 | y; R[cur].d[3] = 0x5a5a0000 | (UWORD)off);
    sameimg();
}

static void bullet(int idx, UWORD x, UWORD y)
{
    ROUT = idx == DRAW ? "DrawBullet" : "EraseBullet";
    sprintf(VEC, "x %u y %u", x, y);
    DO2(ENT(LIB, idx, 0), R[cur].d[0] = (ULONG)bitm[cur]; R[cur].d[1] = 0x5a5a0000 | x;
        R[cur].d[2] = 0x5a5a0000 | y);
    sameplanes();
}

int main(int argc, char **argv)
{
    static const char *names[] = { LIB, NULL };
    int s, i, k;

    if (argc < 2) {
        printf("usage: blitzbomberslibtwo DIR\n");
        return 20;
    }
    if (!load_sets(argv[1], names))
        return 20;
    for (s = 0; s < 2; s++) {
        img[s] = AllocMem(IMGSZ, MEMF_PUBLIC);
        plane[s] = AllocMem(PLANESZ, MEMF_PUBLIC);
        for (k = 0; k < 4; k++)
            bplanes[s][k] = AllocMem(PLANESZ, MEMF_PUBLIC);
    }

    /* KillInvader: 16 table entries of 16 bytes (+0 and +48 used), the
       images 1024 bytes apart from +1024, offsets around them */
    for (i = 0; i < 40; i++) {
        ULONG seed = rnd(), at[19];
        for (k = 0; k < 19; k++)
            at[k] = 1024 + 256 * pick(0, 4) + 2 * (ULONG)(k & 3);
        for (s = 0; s < 2; s++) {
            ULONG v = seed;
            for (k = 0; k < IMGSZ; k++) {
                v = v * 69069 + 1;
                img[s][k] = (UBYTE)(v >> 24);
            }
            for (k = 0; k < 19; k++)
                *(ULONG *)(img[s] + 16 * k) = (ULONG)img[s] + at[k];
        }
        kill((UWORD)pick(0, 63), (UWORD)pick(0, i < 20 ? 3 : 20), (WORD)(pick(0, 767) - 256) & ~1);
    }

    /* DrawStarField: random stars in the playfield (lines 32..235) */
    for (k = 0; k < NSTARS; k++) {
        stars[0][3 * k] = (UWORD)(k == 7 ? 0 : pick(1, 6) * 40);
        stars[0][3 * k + 1] = (UWORD)pick(0, 15);
        stars[0][3 * k + 2] = (UWORD)pick(32 * 40, 236 * 40 - 1);
    }
    stars[0][3 * NSTARS] = 0xffff;
    memcpy(stars[1], stars[0], sizeof stars[0]);
    for (k = 0; k < PLANESZ; k++)
        plane[0][k] = plane[1][k] = (UBYTE)(k * 13 + (k >> 5));
    ROUT = "DrawStarField";
    for (i = 0; i < 60; i++) {
        const UBYTE *t[2] = { (UBYTE *)stars[0], (UBYTE *)stars[1] };
        sprintf(VEC, "frame %d", i);
        DO2(ENT(LIB, STARS, 0), R[cur].d[0] = (ULONG)stars[cur]; R[cur].d[1] = (ULONG)plane[cur]);
        SAMEBYTES("table", t, sizeof stars[0]);
        SAMEBYTES("plane", plane, PLANESZ);
    }

    /* the bullets */
    for (s = 0; s < 2; s++) {
        for (k = 0; k < 4; k++) {
            ULONG j;
            for (j = 0; j < PLANESZ; j++)
                bplanes[s][k][j] = (UBYTE)(j * (k + 3) + (j >> 7));
            *(UBYTE **)(bitm[s] + 8 + 4 * k) = bplanes[s][k];
        }
        *(UWORD *)bitm[s] = 40;
    }
    for (i = 0; i < 40; i++) {
        UWORD x = (UWORD)pick(0, 319), y = (UWORD)pick(0, 253);
        bullet(DRAW, x, y);
        if (i & 1)
            bullet(ERASE, x, y);
        bullet(ERASE, (UWORD)pick(0, 319), (UWORD)pick(0, 253));
    }

    for (s = 0; s < 2; s++) {
        FreeMem(img[s], IMGSZ);
        FreeMem(plane[s], PLANESZ);
        for (k = 0; k < 4; k++)
            FreeMem(bplanes[s][k], PLANESZ);
    }
    return rwe_done(LIB);
}
