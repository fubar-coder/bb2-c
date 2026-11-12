/*
 * rwe blitzbomberslibtwo - the C build of blitzbomberslibtwo (userlib
 * 31) or the reference build of
 * rwelibsrc/blitzbomberslib/blitzbombersTwo00.bb (blitz2 2.10), one set
 * as LIBSET says, observed as cases (rwecmp.h, LIBS_ONE_SET; the expected
 * data libs/tests/expected/rwe_blitzbomberslibtwo.txt).
 *
 *   blitzbomberslibtwo DIR     (DIR with blitzbomberslibtwo .obj/.orig)
 *
 * The test's memory: sprite images behind a table of image pointers
 * (KillInvader), a star table and a bitplane (DrawStarField), a bitmap
 * object of four 320x256 planes (the bullets).  After every call the
 * whole memory is observed (and the kept registers).  Covered:
 * KillInvader with x 0..63, y 0..20 and offsets -256..511; DrawStarField
 * over 60 frames with stars that wrap at line 236, bit numbers 0..15
 * (modulo 8) and a speed of 0; DrawBullet/EraseBullet anywhere on the
 * bitmap.  Not covered: offsets that are negative as words (y*224 >=
 * 32768).
 */
#define LIBS_ONE_SET
#include "rwecmp.h"
#include <exec/memory.h>

#define LIB "blitzbomberslibtwo"
#define IMGSZ 8192                      /* KillInvader: table + images */
#define NSTARS 50
#define PLANESZ 10240                   /* 256 lines of 40 bytes */

enum { KILL, STARS, DRAW, ERASE };

static UBYTE *img;                      /* the table at +0, images from +512 */
static UWORD stars[3 * NSTARS + 1];
static UBYTE *plane;
static UBYTE *bplanes[4];
static UBYTE bitm[8 + 4 * 4];

static ULONG pick(ULONG a, ULONG b)
{
    return a + rnd() % (b - a + 1);
}

static void sameimg(void)
{
    /* the table (the pointers relative to the memory, the rest data
       words: numbers), the images */
    int o;
    for (o = 0; o < 512; o += 4) {
        ULONG v = *(ULONG *)(img + o);
        if (o % 16 == 0 && o < 16 * 19)
            v -= (ULONG)img;
        OBS("table", v);
    }
    OBSBYTES("images", img + 512, IMGSZ - 512);
}

static void sameplanes(void)
{
    int p;
    for (p = 0; p < 4; p++)
        OBSBYTES("plane", bplanes[p], PLANESZ);
}

static void kill(UWORD x, UWORD y, WORD off)
{
    ROUT = "KillInvader";
    sprintf(VEC, "x %u y %u offset %d", x, y, off);
    DO1(ENT(LIB, KILL, 0), R.d[0] = (ULONG)img; R.d[1] = 0x5a5a0000 | x;
        R.d[2] = 0x5a5a0000 | y; R.d[3] = 0x5a5a0000 | (UWORD)off);
    sameimg();
}

static void bullet(int idx, UWORD x, UWORD y)
{
    ROUT = idx == DRAW ? "DrawBullet" : "EraseBullet";
    sprintf(VEC, "x %u y %u", x, y);
    DO1(ENT(LIB, idx, 0), R.d[0] = (ULONG)bitm; R.d[1] = 0x5a5a0000 | x;
        R.d[2] = 0x5a5a0000 | y);
    sameplanes();
}

int main(int argc, char **argv)
{
    static const char *names[] = { LIB, NULL };
    int i, k;

    if (argc < 2) {
        printf("usage: blitzbomberslibtwo DIR\n");
        return 20;
    }
    if (!load_set(argv[1], names))
        return 20;
    KEEPS("KillInvader", d3-d7|a6);
    KEEPS("DrawStarField", d4-d7|a2|a3|a6);
    KEEPS("DrawBullet", d3-d7|a6);
    KEEPS("EraseBullet", d3-d7|a6);
    img = AllocMem(IMGSZ, MEMF_PUBLIC);
    plane = AllocMem(PLANESZ, MEMF_PUBLIC);
    for (k = 0; k < 4; k++)
        bplanes[k] = AllocMem(PLANESZ, MEMF_PUBLIC);

    /* KillInvader: 16 table entries of 16 bytes (+0 and +48 used), the
       images 1024 bytes apart from +1024, offsets around them */
    for (i = 0; i < 40; i++) {
        ULONG seed = rnd(), at[19], v = seed;
        for (k = 0; k < 19; k++)
            at[k] = 1024 + 256 * pick(0, 4) + 2 * (ULONG)(k & 3);
        for (k = 0; k < IMGSZ; k++) {
            v = v * 69069 + 1;
            img[k] = (UBYTE)(v >> 24);
        }
        for (k = 0; k < 19; k++)
            *(ULONG *)(img + 16 * k) = (ULONG)img + at[k];
        kill((UWORD)pick(0, 63), (UWORD)pick(0, i < 20 ? 3 : 20), (WORD)(pick(0, 767) - 256) & ~1);
    }

    /* DrawStarField: random stars in the playfield (lines 32..235) */
    for (k = 0; k < NSTARS; k++) {
        stars[3 * k] = (UWORD)(k == 7 ? 0 : pick(1, 6) * 40);
        stars[3 * k + 1] = (UWORD)pick(0, 15);
        stars[3 * k + 2] = (UWORD)pick(32 * 40, 236 * 40 - 1);
    }
    stars[3 * NSTARS] = 0xffff;
    for (k = 0; k < PLANESZ; k++)
        plane[k] = (UBYTE)(k * 13 + (k >> 5));
    ROUT = "DrawStarField";
    for (i = 0; i < 60; i++) {
        sprintf(VEC, "frame %d", i);
        DO1(ENT(LIB, STARS, 0), R.d[0] = (ULONG)stars; R.d[1] = (ULONG)plane);
        OBSBYTES("table", stars, sizeof stars);
        OBSBYTES("plane", plane, PLANESZ);
    }

    /* the bullets */
    for (k = 0; k < 4; k++) {
        ULONG j;
        for (j = 0; j < PLANESZ; j++)
            bplanes[k][j] = (UBYTE)(j * (k + 3) + (j >> 7));
        *(UBYTE **)(bitm + 8 + 4 * k) = bplanes[k];
    }
    *(UWORD *)bitm = 40;
    for (i = 0; i < 40; i++) {
        UWORD x = (UWORD)pick(0, 319), y = (UWORD)pick(0, 253);
        bullet(DRAW, x, y);
        if (i & 1)
            bullet(ERASE, x, y);
        bullet(ERASE, (UWORD)pick(0, 319), (UWORD)pick(0, 253));
    }

    FreeMem(img, IMGSZ);
    FreeMem(plane, PLANESZ);
    for (k = 0; k < 4; k++)
        FreeMem(bplanes[k], PLANESZ);
    return rwe_done(LIB);
}
