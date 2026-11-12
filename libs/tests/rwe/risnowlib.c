/*
 * rwe risnowlib - falling snow (InitSnowDrop, LDoSnow, KillPoint,
 * LDoSnowActivate): the C build or the reference build of
 * rwelibsrc/Snow/SnowLib3.bb (risnowlib.orig), one set as LIBSET says,
 * observed as cases (rwecmp.h; the expected data
 * libs/tests/expected/rwe_risnowlib.txt).
 *
 *   risnowlib DIR     (DIR with memlib, risnowlib .obj/.orig)
 *
 * A bitmap object (struct BitMap) with three planes of 40 x 48 bytes in
 * one memory area with margins around them (flakes at the edges read and
 * write next to the planes), and a random table (word index, byte pairs
 * offset/bit with bits 0-7).  The planes start with random contents:
 * ground and obstacles in plane 0, "pause" pixels in plane 1, "blue"
 * pixels in plane 2.  LDoSnow sets COLOR00 ($DFF180): every absolute
 * $DFF180 in the code of the loaded library is patched to a word in RAM
 * (observed, the display is not touched; at least one address must be
 * patched, their number differs between the builds).  memlib is loaded for globalloc.
 *
 * Observed after every call: the whole memory area (planes and margins),
 * the random table, the list table (InitSnowDrop returns its address;
 * pointers as offsets into the block), the block with the four lists
 * (pointers in it as offsets, the bitmap object as a mark), the COLOR00
 * word, d0 where there is a result, and the kept registers.
 *
 * Covered: init, InitSnowDrop for two list numbers (the second smaller,
 * on the same bitmap), 140 steps of LDoSnow on list 0 and 47 on list 1
 * (flakes falling, moving aside both ways, stopping on plane 1, leaving
 * the plane at the bottom, the index of the random table wrapping at
 * 1024), KillPoint on blue and on clear pixels (also in row 0), and
 * LDoSnowActivate every 7 steps, finit.  Not covered: list numbers
 * outside 0-3, numpoints from 1024 on, more flakes than the lists hold,
 * bits above 7 in the random table, LDoSnowActivate before the first
 * LDoSnow (all BUG(orig), #913 #912 #914 #910 #908: memory outside the library's).
 */
#include "rwecmp.h"
#include <exec/memory.h>
#include <graphics/gfx.h>

#define LIB "risnowlib"
enum { LDOSNOW, INITSNOWDROP, KILLPOINT, ACTIVATE, FINIT, INIT };

#define ROWS 48
#define PL (40 * ROWS)                  /* a plane, = bp_size */
#define MARGIN 256
#define AREA (MARGIN + 3 * (PL + MARGIN))
#define RTSZ (2 + 1024 + 2)
#define NFIELD 2

static UBYTE *area;
static struct BitMap *bm;
static UBYTE *rt;
static UWORD *fake;                     /* COLOR00 */
static ULONG *lp;                       /* the list table */
static ULONG num[NFIELD] = { 300, 40 };
static UBYTE *base[NFIELD];             /* the block of each field */

static UWORD getw(const UBYTE *p) { return (UWORD)(p[0] << 8 | p[1]); }
static ULONG getl(const UBYTE *p) { return (ULONG)getw(p) << 16 | getw(p + 2); }

static int patchcolor(struct lib *l, UWORD *f)
{
    ULONG i;
    int n = 0;
    for (i = 0; i + 4 <= l->size; i += 2)
        if (getl(l->code + i) == 0xdff180) {
            *(ULONG *)(l->code + i) = (ULONG)f;
            n++;
            i += 2;
        }
    return n;
}

/* the block of field f and its size */
static UBYTE *block(int f) { return base[f]; }
static ULONG blocksize(int f) { return 4 * (num[f] << 4) + 40; }

/* a pointer as offset into the block (or the bitmap object as a mark) */
static ULONG rel(ULONG v, int f)
{
    ULONG b = (ULONG)block(f);
    if (v == (ULONG)bm)
        return 0x11111111UL;
    if (v >= b && v < b + blocksize(f) + 16)
        return v - b + 0x40000000UL;
    return v;
}

static void cmpfield(int f)
{
    static UBYTE snap[4 * 300 * 16 + 40];
    ULONG size = blocksize(f), d7 = num[f] << 4, off[6];
    int i;
    char buf[40];

    if (!block(f))
        return;
    off[0] = 4;                                 /* A: bitmap, last */
    off[1] = 12;
    off[2] = 16 + d7 + 4;                       /* B */
    off[3] = 16 + d7 + 12;
    off[4] = 32 + 2 * d7;                       /* kill 1, 2: last */
    off[5] = 36 + 3 * d7;
    memcpy(snap, block(f), size);
    for (i = 0; i < 6; i++)
        *(ULONG *)(snap + off[i]) = rel(*(ULONG *)(snap + off[i]), f);
    for (i = 0; i < 4; i++) {
        sprintf(buf, "list table %d.%d", f, i);
        OBS(buf, rel(lp[4 * f + i], f));
    }
    sprintf(buf, "block %d", f);
    OBSBYTES(buf, snap, size);
}

static void compare(void)
{
    int f;
    OBSBYTES("planes", area, AREA);
    OBSBYTES("random table", rt, RTSZ);
    OBS("COLOR00", *fake);
    for (f = 0; f < NFIELD; f++)
        cmpfield(f);
}

/* one call, d0-d3 */
static void call1(int idx, const char *name, ULONG d0, ULONG d1, ULONG d2, ULONG d3)
{
    APTR fn = ENT(LIB, idx, 0);
    ROUT = name;
    markregs(&R);
    R.d[0] = d0;
    R.d[1] = idx == LDOSNOW ? (ULONG)rt : d1;
    R.d[2] = d2;
    R.d[3] = d3;
    R.a[3] = (ULONG)bm;
    IN = R;
    *fake = 0x1234;
    NARGS = 0;
    CCR = callstk(fn, &R, ARGS, NARGS, &POP);
    preserve();
    compare();
}

static void memlib(int idx)
{
    markregs(&R);
    callstk(entry_form(getlib(&loaded, "memlib"), idx, 0), &R, NULL, 0, &POP);
}

static void killpoint(UWORD x, UWORD y)
{
    sprintf(VEC, "%u,%u", x, y);
    call1(KILLPOINT, "KillPoint", 0, 0xabcd0000UL | x, 0x12340000UL | y, 0);
    OBS("d0", R.d[0]);
}

int main(int argc, char **argv)
{
    static const char *names[] = { "memlib", LIB, NULL };
    int i, n;
    ULONG k;

    if (argc < 2) {
        printf("usage: risnowlib DIR\n");
        return 20;
    }
    if (!load_set(argv[1], names))
        return 20;
    KEEPS("init", d0-d7|a0-a3|a6);
    KEEPS("InitSnowDrop", d2-d6|a1|a3|a6);
    KEEPS("LDoSnow", a6);
    KEEPS("LDoSnow (list 1)", a6);
    KEEPS("KillPoint", d3-d6|a3|a6);
    KEEPS("LDoSnowActivate", d1|a6);
    KEEPS("finit", d0-d7|a0-a3|a6);
    area = AllocMem(AREA, MEMF_PUBLIC | MEMF_CLEAR);
    bm = AllocMem(sizeof(struct BitMap), MEMF_PUBLIC | MEMF_CLEAR);
    rt = AllocMem(RTSZ, MEMF_PUBLIC | MEMF_CLEAR);
    fake = AllocMem(4, MEMF_PUBLIC | MEMF_CLEAR);
    if (!area || !bm || !rt || !fake) {
        check("risnowlib: memory", FALSE);
        return rwe_done(LIB);
    }
    bm->BytesPerRow = 40;
    bm->Rows = ROWS;
    bm->Depth = 3;
    for (i = 0; i < 3; i++)
        bm->Planes[i] = area + MARGIN + i * (PL + MARGIN);
    n = patchcolor(getlib(&loaded, LIB), fake);
    CacheClearU();
    printf("COLOR00 patched: %d\n", n);
    check("risnowlib: COLOR00 patched", n > 0);

    /* the planes: ground in the last rows and obstacles (plane 0), pause
       pixels (plane 1), blue pixels (plane 2); the random table */
    for (k = 0; k < PL; k++) {
        ULONG row = k / 40;
        UBYTE g = 0;
        if (row >= ROWS - 3)
            g = (UBYTE)(rnd() | rnd());
        else if (row > 8 && !(rnd() % 6))
            g = (UBYTE)(1 << (rnd() % 8));
        area[MARGIN + k] = g;
        area[MARGIN + PL + MARGIN + k] = (UBYTE)(row > 10 && !(rnd() % 3) ? rnd() : 0);
        area[MARGIN + 2 * (PL + MARGIN) + k] = (UBYTE)(!(rnd() % 4) ? rnd() & rnd() : 0);
    }
    for (k = 2; k < RTSZ; k += 2) {
        rt[k] = (UBYTE)(rnd() % 120);           /* offset: rows 0-2 */
        rt[k + 1] = (UBYTE)(rnd() % 8);
    }
    rt[0] = 0x03;                               /* index 1000: wraps soon */
    rt[1] = 0xe8;

    memlib(7);
    strcpy(VEC, "");
    call1(INIT, "init", 0, 0, 0, 0);
    for (i = 0; i < NFIELD; i++) {
        sprintf(VEC, "%d,%lu", i, num[i]);
        call1(INITSNOWDROP, "InitSnowDrop", i, num[i], 0xffff0003UL, PL);
        lp = (ULONG *)R.d[0];
        base[i] = (UBYTE *)lp[4 * i];
        BOTH("d0 = the list table", lp && base[i]);
        compare();
    }
    for (i = 0; i < 140; i++) {
        sprintf(VEC, "step %d", i);
        call1(LDOSNOW, "LDoSnow", 0, 0, 0, 0);
        if (i % 3 == 1) {
            call1(LDOSNOW, "LDoSnow (list 1)", 1, 0, 0, 0);
        }
        if (i % 7 == 6) {
            int j;
            for (j = 0; j < 6; j++) {
                UWORD x = (UWORD)(rnd() % 320), y = (UWORD)(rnd() % ROWS);
                if (j == 0)
                    y = 0;
                /* a blue pixel near (x,y) if there is one */
                for (k = 0; k < 40 && j < 4; k++) {
                    UBYTE *p2 = area + MARGIN + 2 * (PL + MARGIN);
                    if (p2[y * 40 + x / 8] >> (7 - x % 8) & 1)
                        break;
                    x = (UWORD)((x + 1) % 320);
                }
                killpoint(x, y);
            }
            sprintf(VEC, "step %d", i);
            call1(ACTIVATE, "LDoSnowActivate", 0, 0, 0, 0);
            OBS("d0", R.d[0]);
        }
    }
    strcpy(VEC, "");
    call1(FINIT, "finit", 0, 0, 0, 0);
    memlib(8);
    FreeMem(area, AREA);
    FreeMem(bm, sizeof(struct BitMap));
    FreeMem(rt, RTSZ);
    FreeMem(fake, 4);
    return rwe_done(LIB);
}
