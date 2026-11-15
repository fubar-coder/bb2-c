/*
 * systest_blit - the blitter commands of blitlib (Blit, Block, BlitColl,
 *   BlitMode), qblitlib (QBlit, UnQueue, QBlitMode), bblitlib (BBlit,
 *   UnBuffer, ShowStencil, BBlitMode), stencillib (SBlit, SBlitMode) and
 *   2dlib (ClipBlit, ClipBlitMode) on the Amiga: one set (the original
 *   from acidlibs or the C build, as LIBSET says), observed as cases
 *   (sysload.h; the expected data libs/tests/expected/systest_blit.txt).
 *
 *   systest_blit DIR
 *
 * The objects (a bitmap of 64 x 24 pixels and 3 planes, a second one as
 * the source of UnQueue, three shapes, a queue, a buffer and a stencil)
 * are made here in chip memory; every case starts from the same bitmap
 * contents and writes the planes after the call (the blitter finished),
 * so a case holds the pixels the command drew, never an address.  The
 * edge cases: every shift (x & 15), x and y with junk in their upper
 * words, the handle, a shape with fewer planes than the bitmap (the
 * excess planes, Excessonoff), the modes, a collision and none, the
 * clipping of ClipBlit at each edge and outside.  Each command runs in
 * Blitz mode (the test owns the blitter around the call) and in Amiga
 * mode (the routine owns it, a6 = graphics.library).  The command
 * entries are found by their names (cmd_named).
 */
#include "sysload.h"
#include <exec/memory.h>
#define __NOLIBBASE__
#define GRAPHICS_BASE_NAME gfx
#include <proto/graphics.h>
#undef __NOLIBBASE__

/* callstk() is defined in systest_mathstr.c (same executable) */
extern UWORD callstk(APTR fn, struct regs *r, const ULONG *args, ULONG nargs, ULONG *popped);

/* ------------------------------------------------------------ framework */

static struct regs R, IN;
static ULONG POP;
static struct GfxBase *gfx;             /* graphics.library (proto: GRAPHICS_BASE_NAME) */

/* the code of form `form` of the command `name` of lib (as in
   systest_cmdlibs.c) */
static UBYTE *cmd_named(struct lib *l, const char *name, int form)
{
    UBYTE *b = l->code, *p = b + 0x30;
    while (!(W(p) & 0x8000)) {
        UWORD kind = W(p);
        UBYTE *code = NULL;
        p += 6;
        if ((kind & 0xf) == 0) {                /* nullsub */
            while (W(p)) p += 4;
            p += 2 + 12;
        } else {
            int f = 0;
            if (kind != 8) {
                while (W(p) != 0xffff) {        /* forms */
                    p += 2 + (W(p) & 0xff);
                    p = b + ((p - b + 1) & ~1);
                    while (W(p)) p += 4;
                    p += 2;
                    if (f++ == form)
                        code = (UBYTE *)L(p + 4);
                    p += 12;
                }
                p += 2;
            }
            p += 6;                             /* next.l, num.w */
            if (!strcmp((const char *)p, name)) {
                check("cmd_named: the form exists", code != NULL);
                return (UBYTE *)((ULONG)code & ~1UL);
            }
            while (*p++) ;                      /* name */
            while (*p++) ;                      /* help */
            p = b + ((p - b + 1) & ~1);
        }
    }
    check("cmd_named: the command exists", FALSE);
    return NULL;
}

/* the Blitz mode code of a Blitz/Amiga routine (`Dc $a001:Dc.l code`
   in front of the Amiga mode code) and its Amiga mode code */
static UBYTE *blitzcode(UBYTE *e)
{
    check("a Blitz/Amiga routine", e && W(e) == 0xa001);
    return e && W(e) == 0xa001 ? (UBYTE *)L(e + 2) : e;
}

static UBYTE *amigacode(UBYTE *e)
{
    return e && W(e) == 0xa001 ? e + 6 : e;
}

/* call fn with the registers `setup` makes in R (the others markers);
   amiga: in Amiga mode (a6 = graphics.library), else in Blitz mode, the
   test owning the blitter around the call; the blitter has finished when
   the kept case is written */
#define CALL(fn, amiga, setup) do {                                         \
        markregs(&R);                                                       \
        setup;                                                              \
        if (amiga)                                                          \
            R.a[6] = (ULONG)gfx;                                            \
        IN = R;                                                             \
        if (!(amiga))                                                       \
            OwnBlitter();                                                   \
        callstk((fn), &R, NULL, 0, &POP);                                   \
        WaitBlit();                                                         \
        if (!(amiga))                                                       \
            DisownBlitter();                                                \
        obs_kept(&IN, &R);                                                  \
        OBS("popped", POP);                                                 \
    } while (0)

static ULONG seed = 0x13579bdf;
static ULONG rnd(void)
{
    seed = seed * 1103515245UL + 12345;
    return (seed >> 16) | ((seed * 69069UL + 1) & 0xffff0000UL);
}

#define N(lib, name, form) cmd_named(getlib(&loaded, lib), (name), (form))
#define CHIP 0xdff000UL

/* ------------------------------------------------------------ the objects */

#define BPR 8
#define ROWS 24
#define DEPTH 3
#define PLANE (BPR * ROWS)

/* a struct bb_bitmap (64 bytes) on planes in chip memory */
struct bitmap {
    ULONG obj[16];
    UBYTE *planes;
};
static struct bitmap bm, src;

/* a struct bb_shape (32 bytes) with its data and cookie in chip memory */
static UBYTE shape[3][32];
static UBYTE *chipmem;
static ULONG chipused;

static UBYTE *chip(ULONG n)
{
    UBYTE *p = chipmem + chipused;
    chipused += (n + 3) & ~3UL;
    return p;
}

static void mkbitmap(struct bitmap *b)
{
    UBYTE *o = (UBYTE *)b->obj;
    int i;
    memset(b->obj, 0, sizeof b->obj);
    b->planes = chip(PLANE * DEPTH);
    *(UWORD *)(o + 0) = BPR;
    *(UWORD *)(o + 2) = ROWS;
    *(UWORD *)(o + 4) = DEPTH;                  /* flags 0, depth */
    for (i = 0; i < DEPTH; i++)
        *(UBYTE **)(o + 8 + 4 * i) = b->planes + i * PLANE;
    *(UWORD *)(o + 56) = BPR * 8;               /* width */
    *(UWORD *)(o + 58) = ROWS;                  /* height */
    *(UWORD *)(o + 62) = 1;                     /* set up */
}

/* the bitmap contents every case starts from: some bits set */
static void fill(struct bitmap *b, ULONG s)
{
    int i;
    seed = s;
    for (i = 0; i < PLANE * DEPTH; i++)
        b->planes[i] = (UBYTE)(rnd() & rnd() & 0xff);
}

static void clear(struct bitmap *b)
{
    memset(b->planes, 0, PLANE * DEPTH);
}

/* shape n: width x height x depth, handle hx, hy */
static void mkshape(int n, UWORD width, UWORD height, UWORD depth, WORD hx, WORD hy)
{
    UBYTE *s = shape[n];
    UWORD ebw = (UWORD)(((width + 15) >> 4) * 2), ps = (UWORD)(ebw * height);
    UBYTE *data = chip((ULONG)ps * depth), *cookie = chip(ps);
    int i, p;

    memset(s, 0, 32);
    *(UWORD *)(s + 0) = width;
    *(UWORD *)(s + 2) = height;
    *(UWORD *)(s + 4) = depth;
    *(UWORD *)(s + 6) = ebw;
    *(UWORD *)(s + 8) = (UWORD)(height << 6 | (ebw / 2 + 1));
    *(WORD *)(s + 10) = hx;
    *(WORD *)(s + 12) = hy;
    *(UBYTE **)(s + 14) = data;
    *(UBYTE **)(s + 18) = cookie;
    *(UWORD *)(s + 22) = ps;
    *(UWORD *)(s + 24) = (UWORD)(ps + 2 * height);
    *(UWORD *)(s + 26) = (UWORD)(ps * depth);
    *(UWORD *)(s + 28) = (UWORD)((ps + 2 * height) * depth);
    seed = 0x2468ace1 + n;
    for (i = 0; i < ps * depth; i++)
        data[i] = (UBYTE)rnd();
    for (i = 0; i < ps; i++) {
        UBYTE c = 0;
        for (p = 0; p < depth; p++)
            c |= data[p * ps + i];
        cookie[i] = c;
    }
}

/* a queue (12 bytes: pos, mem, len) for 8 items, a buffer (16 bytes:
   pos, last, mem, len; the memory in chip memory) and a stencil (8
   bytes: bpr, rows, mem) of the bitmap's size */
static ULONG queue[3], buffer[4];
static struct { UWORD bpr, rows; UBYTE *mem; } stencil;
static UBYTE *qmem, *bmem, *smem;

#define VECF(...) snprintf(VEC, sizeof VEC, __VA_ARGS__)
#define PLANES() OBSBYTES("planes", bm.planes, PLANE * DEPTH)

/* x and y with junk in the upper words (only the low words count) */
#define JX(x) ((ULONG)(UWORD)(x) | 0xabcd0000UL)
#define JY(y) ((ULONG)(UWORD)(y) | 0x12340000UL)

/* ------------------------------------------------------------- blitlib */

static const WORD xs[] = { 0, 1, 7, 15, 16, 33 };
static const WORD ys[] = { 0, 5 };

static void test_blitlib(void)
{
    static const UWORD modes[] = { 0xb0a, 0xb5a, 0xbfa, 0x5ca, 0xfca };
    unsigned s, i, j, k;
    int a;

    for (a = 0; a < 2; a++) {
        UBYTE *blit = N("blitlib", "Blit", 0), *blitx = N("blitlib", "Blit", 1);
        UBYTE *block = N("blitlib", "Block", 0), *coll = N("blitlib", "BlitColl", 0);
        UBYTE *bl = a ? amigacode(blit) : blitzcode(blit);
        UBYTE *blx = a ? amigacode(blitx) : blitzcode(blitx);
        UBYTE *bk = a ? amigacode(block) : blitzcode(block);
        UBYTE *bc = a ? amigacode(coll) : blitzcode(coll);

        ROUT = a ? "Blit (Amiga mode)" : "Blit";
        for (s = 0; s < 3; s++)
            for (i = 0; i < sizeof xs / sizeof xs[0]; i++)
                for (j = 0; j < sizeof ys / sizeof ys[0]; j++) {
                    VECF("shape %u at %d,%d", s, xs[i], ys[j]);
                    fill(&bm, 1);
                    CALL(bl, a, R.d[1] = JX(xs[i] + 5 * (s == 1)); R.d[2] = JY(ys[j] + 3 * (s == 1));
                         R.a[1] = CHIP; R.a[2] = (ULONG)bm.obj; R.a[3] = (ULONG)shape[s]);
                    PLANES();
                }
        /* the modes of BlitMode: erase, invert, solid, another minterm,
           cookie cut again */
        for (k = 0; k < sizeof modes / sizeof modes[0]; k++) {
            ROUT = "BlitMode";
            VECF("$%X", modes[k]);
            CALL(N("blitlib", "BlitMode", 0), 0, R.d[0] = 0xffff0000UL | modes[k]);
            ROUT = a ? "Blit (Amiga mode)" : "Blit";
            VECF("mode $%X shape 0 at 3,2", modes[k]);
            fill(&bm, 2);
            CALL(bl, a, R.d[1] = JX(3); R.d[2] = JY(2); R.a[1] = CHIP;
                 R.a[2] = (ULONG)bm.obj; R.a[3] = (ULONG)shape[0]);
            PLANES();
        }
        /* Excessonoff: shape 0 has 2 planes, the bitmap 3 */
        ROUT = a ? "Blit excess (Amiga mode)" : "Blit excess";
        for (k = 0; k < 4; k++)
            for (i = 0; i < 3; i++) {
                VECF("bits %u shape %u at %d,4", k, i, xs[i + 2]);
                fill(&bm, 3);
                CALL(blx, a, R.d[1] = JX(xs[i + 2]); R.d[2] = JY(4); R.d[3] = 0xfff00000UL | k;
                     R.a[1] = CHIP; R.a[2] = (ULONG)bm.obj; R.a[3] = (ULONG)shape[i]);
                PLANES();
            }
        /* Block: byte positions, no cookie */
        ROUT = a ? "Block (Amiga mode)" : "Block";
        for (s = 0; s < 3; s++)
            for (i = 0; i < sizeof xs / sizeof xs[0]; i++) {
                VECF("shape %u at %d,%d", s, xs[i] + 8, 2 + (int)s);
                fill(&bm, 4);
                CALL(bk, a, R.d[1] = JX(xs[i] + 8 + 5 * (s == 1));
                     R.d[2] = JY(2 + s + 3 * (s == 1)); R.a[1] = CHIP;
                     R.a[2] = (ULONG)bm.obj; R.a[3] = (ULONG)shape[s]);
                PLANES();
            }
        /* BlitColl: the cookie on set pixels, on none, on one set pixel */
        ROUT = a ? "BlitColl (Amiga mode)" : "BlitColl";
        for (k = 0; k < 4; k++)
            for (i = 0; i < sizeof xs / sizeof xs[0]; i++) {
                VECF("bitmap %u shape 0 at %d,3", k, xs[i]);
                if (k == 0)
                    fill(&bm, 5);
                else
                    clear(&bm);
                if (k == 2)
                    bm.planes[2 * PLANE + 6 * BPR + 2] = 0x01;  /* x 23, y 6, plane 2 */
                if (k == 3)
                    bm.planes[PLANE + 20 * BPR + 7] = 0x80;     /* x 56, y 20: outside */
                CALL(bc, a, R.d[1] = JX(xs[i]); R.d[2] = JY(3); R.a[1] = CHIP;
                     R.a[2] = (ULONG)bm.obj; R.a[3] = (ULONG)shape[0]);
                /* a word function: the upper word of d0 is undefined
                   (in Amiga mode the original's is DisownBlitter's) */
                OBS("d0.w", (UWORD)R.d[0]);
            }
    }
}

/* ------------------------------------------------------------- qblitlib */

static void newqueue(void)
{
    queue[0] = queue[1] = (ULONG)qmem;
    queue[2] = 8 * 14;
}

static void test_qblitlib(void)
{
    static const WORD qx[] = { 3, 20, 41, 1, 30 }, qy[] = { 2, 10, 4, 15, 3 };
    unsigned i;
    int a, u;

    ROUT = "QBlitMode";
    strcpy(VEC, "$FCA");
    CALL(N("qblitlib", "QBlitMode", 0), 0, R.d[0] = 0xfca);
    for (a = 0; a < 2; a++)
        for (u = 0; u < 3; u++) {
            UBYTE *qb = N("qblitlib", "QBlit", u == 2);
            UBYTE *uq = N("qblitlib", "UnQueue", u == 1);
            UBYTE *q = a ? amigacode(qb) : blitzcode(qb);
            UBYTE *un = a ? amigacode(uq) : blitzcode(uq);

            newqueue();
            fill(&bm, 6);
            fill(&src, 7);
            ROUT = a ? (u == 2 ? "QBlit excess (Amiga mode)" : "QBlit (Amiga mode)")
                     : (u == 2 ? "QBlit excess" : "QBlit");
            for (i = 0; i < sizeof qx / sizeof qx[0]; i++) {
                VECF("run %d shape %u at %d,%d", u, i % 3, qx[i], qy[i]);
                CALL(q, a, R.d[2] = JX(qx[i]); R.d[3] = JY(qy[i]); R.d[4] = 0xfff00000UL | i;
                     R.a[0] = (ULONG)queue; R.a[1] = CHIP; R.a[2] = (ULONG)bm.obj;
                     R.a[3] = (ULONG)shape[i % 3]);
                PLANES();
                OBS("queue position", queue[0] - (ULONG)qmem);
            }
            ROUT = a ? (u == 1 ? "UnQueue bitmap (Amiga mode)" : "UnQueue (Amiga mode)")
                     : (u == 1 ? "UnQueue bitmap" : "UnQueue");
            VECF("run %d", u);
            CALL(un, a, R.a[1] = CHIP; R.a[2] = (ULONG)src.obj; R.a[3] = (ULONG)queue);
            PLANES();
            OBS("queue position", queue[0] - (ULONG)qmem);
            /* an empty queue */
            VECF("run %d empty", u);
            CALL(un, a, R.a[1] = CHIP; R.a[2] = (ULONG)src.obj; R.a[3] = (ULONG)queue);
            PLANES();
        }
}

/* ------------------------------------------- bblitlib and stencillib */

static void newbuffer(void)
{
    buffer[0] = buffer[2] = (ULONG)bmem;
    buffer[1] = 0;
    buffer[3] = 1024;
}

static void test_bblitlib(void)
{
    static const WORD bx[] = { 3, 20, 41, 1, 30 }, by[] = { 2, 10, 4, 15, 3 };
    unsigned i;
    int a, u;

    ROUT = "BBlitMode";
    strcpy(VEC, "$FCA");
    CALL(N("bblitlib", "BBlitMode", 0), 0, R.d[0] = 0xfca);
    ROUT = "SBlitMode";
    CALL(N("stencillib", "SBlitMode", 0), 0, R.d[0] = 0xfca);
    for (a = 0; a < 2; a++)
        for (u = 0; u < 2; u++) {
            UBYTE *bb = N("bblitlib", "BBlit", u), *ub = N("bblitlib", "UnBuffer", 0);
            UBYTE *sb = N("stencillib", "SBlit", u), *ss = N("bblitlib", "ShowStencil", 0);
            UBYTE *b = a ? amigacode(bb) : blitzcode(bb);
            UBYTE *un = a ? amigacode(ub) : blitzcode(ub);
            UBYTE *sbl = a ? amigacode(sb) : blitzcode(sb);
            UBYTE *show = a ? amigacode(ss) : blitzcode(ss);

            newbuffer();
            fill(&bm, 8);
            ROUT = a ? (u ? "BBlit excess (Amiga mode)" : "BBlit (Amiga mode)")
                     : (u ? "BBlit excess" : "BBlit");
            for (i = 0; i < sizeof bx / sizeof bx[0]; i++) {
                VECF("run %d shape %u at %d,%d", u, i % 3, bx[i], by[i]);
                CALL(b, a, R.d[2] = JX(bx[i]); R.d[3] = JY(by[i]); R.d[4] = 0xfff00000UL | i;
                     R.a[0] = (ULONG)buffer; R.a[1] = CHIP; R.a[2] = (ULONG)bm.obj;
                     R.a[3] = (ULONG)shape[i % 3]);
                PLANES();
                OBS("buffer position", buffer[0] - (ULONG)bmem);
            }
            ROUT = a ? "UnBuffer (Amiga mode)" : "UnBuffer";
            VECF("run %d", u);
            CALL(un, a, R.a[2] = CHIP; R.a[3] = (ULONG)buffer);
            PLANES();
            OBS("buffer position", buffer[0] - (ULONG)bmem);
            OBS("last record", buffer[1]);
            VECF("run %d empty", u);
            CALL(un, a, R.a[2] = CHIP; R.a[3] = (ULONG)buffer);
            PLANES();

            /* SBlit into the stencil, BBlit on top, ShowStencil: the
               saved background back where the stencil is clear */
            memset(smem, 0, PLANE);
            fill(&bm, 9);
            newbuffer();
            ROUT = a ? (u ? "SBlit excess (Amiga mode)" : "SBlit (Amiga mode)")
                     : (u ? "SBlit excess" : "SBlit");
            for (i = 0; i < 3; i++) {
                VECF("run %d shape %u at %d,%d", u, i, bx[i + 1], by[i + 1]);
                CALL(sbl, a, R.d[2] = JX(bx[i + 1]); R.d[3] = JY(by[i + 1]);
                     R.d[4] = 0xfff00000UL | (i + 1); R.a[0] = (ULONG)&stencil; R.a[1] = CHIP;
                     R.a[2] = (ULONG)bm.obj; R.a[3] = (ULONG)shape[i]);
                PLANES();
                OBSBYTES("stencil", smem, PLANE);
            }
            ROUT = a ? (u ? "BBlit excess (Amiga mode)" : "BBlit (Amiga mode)")
                     : (u ? "BBlit excess" : "BBlit");
            for (i = 0; i < 3; i++) {
                VECF("run %d over the stencil shape %u at %d,%d", u, 2 - i, bx[i] + 2, by[i] + 1);
                CALL(b, a, R.d[2] = JX(bx[i] + 2); R.d[3] = JY(by[i] + 1);
                     R.d[4] = 0xfff00000UL | i; R.a[0] = (ULONG)buffer; R.a[1] = CHIP;
                     R.a[2] = (ULONG)bm.obj; R.a[3] = (ULONG)shape[2 - i]);
                PLANES();
            }
            ROUT = a ? "ShowStencil (Amiga mode)" : "ShowStencil";
            VECF("run %d", u);
            CALL(show, a, R.a[1] = (ULONG)&stencil; R.a[2] = CHIP; R.a[3] = (ULONG)buffer);
            PLANES();
            OBS("buffer position", buffer[0] - (ULONG)bmem);
        }
}

/* ---------------------------------------------------------- ClipBlit */

static void test_clipblit(void)
{
    static const WORD cx[] = { -30, -17, -16, -9, -1, 0, 3, 40, 49, 55, 63, 64, 80 };
    static const WORD cy[] = { -12, -5, 0, 19, 23, 24, 30 };
    unsigned s, i, j, k;
    int a;

    ROUT = "ClipBlitMode";
    strcpy(VEC, "$FCA");
    CALL(N("2dlib", "ClipBlitMode", 0), 0, R.d[0] = 0xfca);
    for (a = 0; a < 2; a++) {
        UBYTE *cb = N("2dlib", "ClipBlit", 0), *cbx = N("2dlib", "ClipBlit", 1);
        UBYTE *c = a ? amigacode(cb) : blitzcode(cb);
        UBYTE *cx1 = a ? amigacode(cbx) : blitzcode(cbx);

        ROUT = a ? "ClipBlit (Amiga mode)" : "ClipBlit";
        for (s = 0; s < 3; s++)
            for (i = 0; i < sizeof cx / sizeof cx[0]; i++)
                for (j = 0; j < sizeof cy / sizeof cy[0]; j++) {
                    VECF("shape %u at %d,%d", s, cx[i], cy[j]);
                    fill(&bm, 10);
                    CALL(c, a, R.d[1] = JX(cx[i]); R.d[2] = JY(cy[j]); R.a[1] = CHIP;
                         R.a[2] = (ULONG)bm.obj; R.a[3] = (ULONG)shape[s]);
                    PLANES();
                }
        ROUT = a ? "ClipBlit excess (Amiga mode)" : "ClipBlit excess";
        for (k = 0; k < 2; k++)
            for (i = 0; i < sizeof cx / sizeof cx[0]; i += 3) {
                VECF("bits %u shape 0 at %d,%d", k, cx[i], cy[k + 1]);
                fill(&bm, 11);
                CALL(cx1, a, R.d[1] = JX(cx[i]); R.d[2] = JY(cy[k + 1]); R.d[3] = 0xfff00000UL | k;
                     R.a[1] = CHIP; R.a[2] = (ULONG)bm.obj; R.a[3] = (ULONG)shape[0]);
                PLANES();
            }
    }
}

/* ------------------------------------------------------------------ main */

static const char *names[] = {
    "memlib", "blitlib", "qblitlib", "bblitlib", "stencillib", "2dlib", NULL
};

int main(int argc, char **argv)
{
    static const char *const routs[] = {
        "Blit", "Blit (Amiga mode)", "Blit excess", "Blit excess (Amiga mode)",
        "Block", "Block (Amiga mode)", "BlitMode",
        "QBlit", "QBlit (Amiga mode)", "QBlit excess", "QBlit excess (Amiga mode)",
        "UnQueue", "UnQueue (Amiga mode)", "UnQueue bitmap", "UnQueue bitmap (Amiga mode)",
        "QBlitMode", "BBlit", "BBlit (Amiga mode)", "BBlit excess", "BBlit excess (Amiga mode)",
        "UnBuffer", "UnBuffer (Amiga mode)", "ShowStencil", "ShowStencil (Amiga mode)",
        "BBlitMode", "SBlit", "SBlit (Amiga mode)", "SBlit excess", "SBlit excess (Amiga mode)",
        "SBlitMode", "ClipBlit", "ClipBlit (Amiga mode)", "ClipBlit excess",
        "ClipBlit excess (Amiga mode)", "ClipBlitMode", NULL
    };
    int i;

    if (argc < 2) {
        printf("usage: systest_blit DIR\n");
        return 20;
    }
    if (!load_set(argv[1], names))
        return 20;
    for (i = 0; routs[i]; i++)
        KEEPS_STATEMENT(routs[i], none);
    KEEPS_FUNCTION("BlitColl", none);
    KEEPS_FUNCTION("BlitColl (Amiga mode)", none);
    gfx = (struct GfxBase *)OpenLibrary((STRPTR)"graphics.library", 0);
    chipmem = AllocMem(8192, MEMF_CHIP | MEMF_CLEAR);
    qmem = AllocMem(8 * 14, MEMF_PUBLIC | MEMF_CLEAR);
    if (!gfx || !chipmem || !qmem) {
        check("graphics.library and the memory", FALSE);
        return 20;
    }
    mkbitmap(&bm);
    mkbitmap(&src);
    mkshape(0, 16, 8, 2, 0, 0);
    mkshape(1, 16, 8, 3, 5, 3);
    mkshape(2, 32, 6, 3, 0, 0);
    bmem = chip(1024);
    smem = chip(PLANE);
    stencil.bpr = BPR;
    stencil.rows = ROWS;
    stencil.mem = smem;
    check("the chip memory suffices", chipused <= 8192);

    test_blitlib();
    test_qblitlib();
    test_bblitlib();
    test_clipblit();

    FreeMem(qmem, 8 * 14);
    FreeMem(chipmem, 8192);
    CloseLibrary((struct Library *)gfx);
    printf("%d checks, %d failures\n", checks, failures);
    kept_summary();
    return failures ? 5 : 0;
}
