/*
 * rwe riwbspritelib - sprites on the Workbench screen (GetWBSprite,
 * FreeWBSprite, MoveWBSprite, ChangeWBSprite, WBSpriteX/Y): the C build
 * against the reference build of rwelibsrc/wbspritelib/WBSprite-library2.bb
 * (riwbspritelib.orig), method of rwecmp.h.
 *
 *   riwbspritelib DIR     (DIR with riwbspritelib .obj/.orig)
 *
 * The library takes real hardware sprites (GetExtSpriteA) and moves them
 * on the Workbench screen's ViewPort, so the two sets cannot run side by
 * side (the second would find the channels taken): the whole scenario
 * runs on the original, which finit ends by freeing every sprite, then
 * on the C build; each step's registers and a snapshot are recorded and
 * compared afterwards step by step.  The snapshot: d0 where there is a
 * result (MoveWBSprite returns what MoveSprite leaves in d0: not
 * compared), the library's data block (found by its "graphics.library"
 * string: the variables, the tag lists, the sprite table as 0 / not 0;
 * pointers into the block as offsets), for every sprite in the table its
 * ExtSprite (height, x, y, num, wordwidth, flags) and image data, and
 * the change of graphics.library's open count.  At the end the free
 * hardware sprites must be the same as at the start (nothing left
 * allocated or shown).
 *
 * The shapes (Blitz shape objects: width, height, depth, data) are built
 * in memory and the same for both sets.  Covered: WBSpriteX without a
 * sprite (reads the word at 6, BUG(orig)), init, GetWBSprite on a free
 * channel, on any channel (-1), on its own channel again (GetExtSpriteA
 * fails, the channel is returned, BUG(orig)), on channel 0 (the mouse
 * pointer's: 0), 32 pixels wide (refused on this Workbench screen: 0),
 * with a narrow shape (0), a shape without data (-1), 20 wide
 * (copied as 16); WBSpriteX/Y; MoveWBSprite on two sprites and without
 * one; ChangeWBSprite (new images with 2 and 1 planes, no sprite, narrow
 * shape, no data); FreeWBSprite with and without a sprite; finit with
 * sprites left.  The results of the original are printed per step.  Not
 * covered: channels outside 0-7 (the table is indexed without checks), a
 * second finit (closes graphics.library again), failing AllocBitMap,
 * AllocSpriteDataA or ChangeExtSpriteA (the last leaks the new data),
 * height or depth 0 (65536 rows/planes).
 */
#include "rwecmp.h"
#include <exec/memory.h>
#include <graphics/sprite.h>
#include <intuition/screens.h>
static struct Library *wbsgfx, *wbsint;
#define __NOLIBBASE__
#define GRAPHICS_BASE_NAME wbsgfx
#define INTUITION_BASE_NAME wbsint
#include <proto/graphics.h>
#include <proto/intuition.h>
#undef __NOLIBBASE__

#define LIB "riwbspritelib"
enum { GETWB, FREEWB, MOVEWB, CHANGEWB, SPRX, SPRY, FINIT, INIT };

#define NSTEP 32
#define IMG 256                         /* image bytes compared per sprite */
#define BLOCK 166                       /* myviewport ... graphicslibbase */
#define NAMEOFF 144                     /* "graphics.library" in the block */

struct snap {
    ULONG d0;
    LONG opened;
    UBYTE block[BLOCK];
    UWORD es[8][6];                     /* height, x, y, num, wordwidth, flags */
    UBYTE img[8][IMG];
};

struct step {
    const char *name;
    char vec[48];
    struct regs in, out;
    ULONG pop;
    ULONG dontcare;
    struct snap sn;
};

static struct step steps[2][NSTEP];
static int nstep;
static UBYTE *blk[2];
static ULONG vp;
static UBYTE *shp[6];                   /* the shape objects */
enum { SHA, SHB, SHC, NARROW, NODATA, SHD };

static UBYTE *findblock(struct lib *l)
{
    ULONG i;
    for (i = NAMEOFF; i + 18 <= l->size; i += 2)
        if (!memcmp(l->code + i, "graphics.library", 17))
            return l->code + i - NAMEOFF;
    return NULL;
}

/* a pointer of the block as offset into it */
static ULONG relb(ULONG v)
{
    ULONG b = (ULONG)blk[cur];
    if (v >= b && v < b + BLOCK)
        return v - b + 0x40000000UL;
    return v;
}

static void snapshot(struct snap *sn, ULONG d0, LONG opened)
{
    UBYTE *b = blk[cur];
    ULONG *tab = (ULONG *)(b + 112);
    int i;

    memset(sn, 0, sizeof *sn);
    sn->d0 = d0;
    sn->opened = opened;
    memcpy(sn->block, b, BLOCK);
    /* myviewport, shapeaddr: the same in both sets; oldpointer,
       temppointer: into the block or a sprite (0 / not 0); mybitmap: 0 /
       not 0; the table: 0 / not 0; the library base: 0 / not 0 */
    *(ULONG *)(sn->block + 4) = relb(*(ULONG *)(b + 4));
    *(ULONG *)(sn->block + 8) = *(ULONG *)(b + 8) != 0;
    if (relb(*(ULONG *)(b + 24)) == *(ULONG *)(b + 24))
        *(ULONG *)(sn->block + 24) = *(ULONG *)(b + 24) != 0;
    else
        *(ULONG *)(sn->block + 24) = relb(*(ULONG *)(b + 24));
    for (i = 0; i < 8; i++) {
        struct ExtSprite *es = (struct ExtSprite *)tab[i];
        *(ULONG *)(sn->block + 112 + 4 * i) = es != NULL;
        if (es) {
            ULONG n = (ULONG)(es->es_SimpleSprite.height + 2) * 4 * es->es_wordwidth;
            sn->es[i][0] = es->es_SimpleSprite.height;
            sn->es[i][1] = (UWORD)es->es_SimpleSprite.x;
            sn->es[i][2] = (UWORD)es->es_SimpleSprite.y;
            sn->es[i][3] = es->es_SimpleSprite.num;
            sn->es[i][4] = es->es_wordwidth;
            sn->es[i][5] = es->es_flags;
            memcpy(sn->img[i], es->es_SimpleSprite.posctldata, n < IMG ? n : IMG);
        }
    }
    *(ULONG *)(sn->block + 162) = *(ULONG *)(b + 162) != 0;
}

/* one step on the current set: d0-d3, a0; recorded */
static void step(int idx, const char *name, ULONG d0, ULONG d1, ULONG d2, ULONG d3, ULONG a0,
                 ULONG dontcare)
{
    struct step *st = &steps[cur][nstep++];
    LONG cnt = wbsgfx->lib_OpenCnt;
    APTR fn = ENT(LIB, idx, 0);

    st->name = name;
    sprintf(st->vec, "%lX,%lX,%lX,%lX", d0, d1, d2, d3);
    markregs(&st->in);
    st->in.d[0] = d0;
    st->in.d[1] = d1;
    st->in.d[2] = d2;
    st->in.d[3] = d3;
    st->in.a[0] = a0;
    st->out = st->in;
    st->dontcare = dontcare;
    if (!fn) {
        check("entry missing", FALSE);
        return;
    }
    NARGS = 0;
    CCR[cur] = callstk(fn, &st->out, ARGS[cur], NARGS, &st->pop);
    snapshot(&st->sn, st->out.d[0], wbsgfx->lib_OpenCnt - cnt);
}

#define SH(n) ((ULONG)shp[n])

static void scenario(void)
{
    ULONG any;

    nstep = 0;
    step(SPRX, "WBSpriteX (no sprite)", 0x00010005, 0, 0, 0, 0, 0);
    step(INIT, "init", 0, 0, 0, 0, 0, 0);
    step(GETWB, "GetWBSprite", 10, 20, 1, 2, SH(SHA), 0);
    step(GETWB, "GetWBSprite (any)", 30, 40, 2, (ULONG)-1, SH(SHA), 0);
    any = steps[cur][nstep - 1].out.d[0];
    step(GETWB, "GetWBSprite (32 wide)", 30, 40, 2, 6, SH(SHB), 0);
    step(GETWB, "GetWBSprite (taken)", 50, 60, 1, 2, SH(SHA), 0);
    step(GETWB, "GetWBSprite (pointer)", 0, 0, 3, 0, SH(SHC), 0);
    step(GETWB, "GetWBSprite (narrow)", 0, 0, 4, 5, SH(NARROW), 0);
    step(GETWB, "GetWBSprite (no data)", 0, 0, 5, 5, SH(NODATA), 0);
    step(GETWB, "GetWBSprite (20 wide)", 70, 80, 3, 5, SH(SHC), 0);
    step(SPRX, "WBSpriteX", 0x12340002, 0, 0, 0, 0, 0);
    step(SPRY, "WBSpriteY", 0xfedc0002, 0, 0, 0, 0, 0);
    step(MOVEWB, "MoveWBSprite", vp, 100, 50, 2, 0, 1);
    step(SPRX, "WBSpriteX", 2, 0, 0, 0, 0, 0);
    step(SPRY, "WBSpriteY", 2, 0, 0, 0, 0, 0);
    step(MOVEWB, "MoveWBSprite", vp, 120, 90, any, 0, 1);
    step(MOVEWB, "MoveWBSprite (no sprite)", vp, 1, 1, 6, 0, 0);
    step(CHANGEWB, "ChangeWBSprite", vp, 2, 3, 0, SH(SHD), 0);
    step(SPRX, "WBSpriteX", 2, 0, 0, 0, 0, 0);
    step(CHANGEWB, "ChangeWBSprite (no sprite)", vp, 6, 1, 0, SH(SHA), 0);
    step(CHANGEWB, "ChangeWBSprite (narrow)", vp, 2, 4, 0, SH(NARROW), 0);
    step(CHANGEWB, "ChangeWBSprite (no data)", vp, 2, 5, 0, SH(NODATA), 0);
    step(CHANGEWB, "ChangeWBSprite (1 plane)", vp, 5, 2, 0, SH(SHD), 0);
    step(FREEWB, "FreeWBSprite", any, 0, 0, 0, 0, 0);
    step(FREEWB, "FreeWBSprite (none)", 6, 0, 0, 0, 0, 0);
    step(FINIT, "finit", 0, 0, 0, 0, 0, 0);
}

/* a Blitz shape object: width, height, depth, ebwidth ..., data at 14 */
static UBYTE *mkshape(UWORD w, UWORD h, UWORD d, int data)
{
    UBYTE *s = AllocMem(32, MEMF_PUBLIC | MEMF_CLEAR);
    ULONG words = (ULONG)((w + 15) >> 4) * h * d, i;
    UWORD *p = NULL;

    if (!s)
        return NULL;
    *(UWORD *)(s + 0) = w;
    *(UWORD *)(s + 2) = h;
    *(UWORD *)(s + 4) = d;
    *(UWORD *)(s + 6) = (UWORD)(((w + 15) >> 4) * 2);
    if (data) {
        p = AllocMem(words * 2 + 2, MEMF_CHIP | MEMF_CLEAR);
        for (i = 0; p && i < words; i++)
            p[i] = (UWORD)rnd();
    }
    *(ULONG *)(s + 14) = (ULONG)p;
    *(ULONG *)(s + 28) = words * 2 + 2;         /* for the test's FreeMem */
    return s;
}

static void freeshape(UBYTE *s)
{
    if (!s)
        return;
    if (*(ULONG *)(s + 14))
        FreeMem((APTR) * (ULONG *)(s + 14), *(ULONG *)(s + 28));
    FreeMem(s, 32);
}

/* the hardware sprites 1-7 that GetSprite can have: a bit each */
static ULONG freesprites(void)
{
    struct SimpleSprite ss;
    ULONG m = 0;
    WORD n;
    for (n = 1; n < 8; n++) {
        memset(&ss, 0, sizeof ss);
        if (GetSprite(&ss, n) == n) {
            m |= 1UL << n;
            FreeSprite(n);
        }
    }
    return m;
}

int main(int argc, char **argv)
{
    static const char *names[] = { LIB, NULL };
    struct Screen *scr;
    ULONG before, after;
    int s, i, k;

    if (argc < 2) {
        printf("usage: riwbspritelib DIR\n");
        return 20;
    }
    if (!load_sets(argv[1], names))
        return 20;
    wbsgfx = OpenLibrary((CONST_STRPTR)"graphics.library", 39);
    wbsint = OpenLibrary((CONST_STRPTR)"intuition.library", 39);
    scr = wbsint ? LockPubScreen(NULL) : NULL;
    for (s = 0; s < 2; s++)
        blk[s] = findblock(getlib(&sets[s], LIB));
    check("riwbspritelib: data block found (orig)", blk[0] != NULL);
    check("riwbspritelib: data block found (C)", blk[1] != NULL);
    if (!wbsgfx || !scr || !blk[0] || !blk[1]) {
        check("riwbspritelib: graphics, intuition 39, Workbench screen", FALSE);
        if (scr)
            UnlockPubScreen(NULL, scr);
        return rwe_done(LIB);
    }
    vp = (ULONG)&scr->ViewPort;
    shp[SHA] = mkshape(16, 12, 2, 1);
    shp[SHB] = mkshape(32, 9, 2, 1);
    shp[SHC] = mkshape(20, 16, 2, 1);
    shp[NARROW] = mkshape(8, 8, 2, 1);
    shp[NODATA] = mkshape(16, 8, 2, 0);
    shp[SHD] = mkshape(16, 5, 1, 1);

    before = freesprites();
    for (s = 0; s < 2; s++) {
        cur = s;
        scenario();
        after = freesprites();
        check(s ? "riwbspritelib: the sprites free again (C)"
                : "riwbspritelib: the sprites free again (orig)", after == before);
    }

    /* compare step by step */
    for (k = 0; k < nstep; k++) {
        struct step *st0 = &steps[0][k];
        char buf[40];
        ROUT = st0->name;
        strcpy(VEC, st0->vec);
        for (cur = 0; cur < 2; cur++) {
            IN[cur] = steps[cur][k].in;
            R[cur] = steps[cur][k].out;
            POP[cur] = steps[cur][k].pop;
        }
        printf("  %-28s d0 orig $%08lX C $%08lX\n", st0->name, steps[0][k].sn.d0,
               steps[1][k].sn.d0);
        DONTCARE = st0->dontcare;
        preserve();
        DONTCARE = 0;
        if (!st0->dontcare)
            SAME("d0", steps[cur][k].sn.d0);
        SAME("graphics.library open count", steps[cur][k].sn.opened);
        for (i = 0; i < BLOCK; i += 2) {
            sprintf(buf, "block +%d", i);
            SAME(buf, *(UWORD *)(steps[cur][k].sn.block + i));
        }
        for (i = 0; i < 8; i++) {
            int j;
            for (j = 0; j < 6; j++) {
                sprintf(buf, "sprite %d field %d", i, j);
                SAME(buf, steps[cur][k].sn.es[i][j]);
            }
            sprintf(buf, "sprite %d image", i);
            {
                UBYTE *im[2] = { steps[0][k].sn.img[i], steps[1][k].sn.img[i] };
                SAMEBYTES(buf, im, IMG);
            }
        }
    }

    for (i = 0; i < 6; i++)
        freeshape(shp[i]);
    UnlockPubScreen(NULL, scr);
    CloseLibrary(wbsint);
    CloseLibrary(wbsgfx);
    return rwe_done(LIB);
}
