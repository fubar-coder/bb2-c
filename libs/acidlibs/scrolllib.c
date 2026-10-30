/*
 * scrolllib - command library 100: Scroll Plot Point Cls BlockScroll on
 * Blitz bitmaps ("Mark and Simons 2d Lib").
 *
 * C translation of bblibsrc/scrolllib.  The CD binary is a little newer
 * than the source in one place: clsnotint (Cls of a normal bitmap) splits
 * the plane size with long operations (MOVE.l d4,d3:BEQ:LSR.l #1,d3:AND
 * #$FFC0,d3 ... ADD.l d3,d3:SUB.l d3,d4:BEQ ... LSR.l #1,d4); this file
 * follows the binary.  Everything else is the source.
 *
 * Bitmap object: 0 bytes per row, 2 rows, 4 depth, 8 planes[8],
 * 48/50 origin x/y, 52 flags (< 0 interleaved), 54 bytes per line of
 * one plane (interleaved), 56/58 clip width/height, 62 "is real".
 *
 * Plot/Point: origin added, clipped against 56/58 (unsigned), colour -1
 * (any negative) = complement.  Cls / Scroll / BlockScroll use the blitter
 * directly (a1 = chip base from chipbaselib); they are Blitz/Amiga
 * variants ($A001): the Amiga version owns the blitter around the Blitz
 * version (graphics OwnBlitter/DisownBlitter, base from !libs) and waits
 * for it at the end.
 * Scroll copies an area with the blitter (ascending or descending
 * depending on the overlap, masks through a 144 byte chip "cookie"
 * buffer allocated by init), areas wider than 960 pixels in two blits
 * (the width word on the caller's argument stack is changed for that,
 * as in the original); BlockScroll copies whole bytes (x / 8).
 *
 * BUG(orig) (#393), kept: a bitmap of depth 0 runs the plane loops 65536 times;
 * Not a bug: Scroll writes the clipped width back to the argument stack (it
 * belongs to this call and is discarded by the compiler afterwards).
 * Error checks (debugger only): Plot/Point/Cls "Uninitialized BitMap";
 * Scroll/BlockScroll area inside both bitmaps ("Illegal Scroll Area"),
 * height <= 1024 ("Scroll area to large").
 */
#include "blitzlib.h"
#define __NOLIBBASE__
#define GRAPHICS_BASE_NAME gfx
#include <proto/graphics.h>
#undef __NOLIBBASE__

/* entry points (C routines / thunks / prefixes, defined below) */
void doscroll(void), b_doscroll(void), docls(void), docls2(void);
void doblockscroll(void), codeinit(void), doplot(void), dopoint(void);
void scchk1(void), scchk2(void), bmchk(void);

BLITZ_ENTRY_DECL(init);

/* !libheader{#scrolllib,init,0,0,errors} */
BLITZ_LIBHEADER(LIB_scrolllib, init, BL_NORET, 0, BLITZ_ERROR_AREA);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, 0x42, 0x42, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, 0x1302, LIB_bitmaplib, 0x1002, LIB_chipbaselib, BL_LA(1),
           LIB_graphicslib, 0x4600);
BLITZ_SUBS(doscroll, scchk1, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, 0x42, 0x42, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, 0x1302, LIB_bitmaplib, 0x1084, LIB_chipbaselib, BL_LA(1),
           LIB_graphicslib, 0x4600);
BLITZ_SUBS(doscroll, scchk2, 0);
BLITZ_NAME("Scroll", "X1,Y1,Width,Height,X2,Y2[,BitMap#]");

BLITZ_NULLSUB(b_doscroll, 0, 0);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, 0x1202);
BLITZ_SUBS(doplot, bmchk, 0);
BLITZ_NAME("Plot", "X,Y,Colour");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, 0x1202);
BLITZ_SUBS(dopoint, bmchk, 0);
BLITZ_NAME("Point", "X,Y,Colour");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS(LIB_bitmaplib, 0x1202, LIB_chipbaselib, BL_LA(1), LIB_graphicslib, 0x5600);
BLITZ_SUBS(docls, bmchk, 0);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, 0x1202, LIB_chipbaselib, BL_LA(1), LIB_graphicslib, 0x5600);
BLITZ_SUBS(docls2, bmchk, 0);
BLITZ_NAME("Cls", "[Colour]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, 0x42, 0x42, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, 0x1302, LIB_bitmaplib, 0x1002, LIB_chipbaselib, BL_LA(1),
           LIB_graphicslib, 0x4600);
BLITZ_SUBS(doblockscroll, scchk1, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, 0x42, 0x42, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, 0x1302, LIB_bitmaplib, 0x1084, LIB_chipbaselib, BL_LA(1),
           LIB_graphicslib, 0x4600);
BLITZ_SUBS(doblockscroll, scchk2, 0);
BLITZ_NAME("BlockScroll", "X1,Y1,Width,Height,X2,Y2[,BitMap#]");

BLITZ_NULLSUB_AT(init, codeinit, 0, 0);

BLITZ_LIBFIN();

/* ------------------------------------------------------------ helpers */

#define CWIDTH 144

static UWORD *cookie;           /* 144 bytes in chip memory, masks */
static UWORD offset;
static UWORD scminiterm = 0xfe2;

#define CW(cb, o) (*(volatile UWORD *)((UBYTE *)(cb) + (o)))
#define CL(cb, o) (*(volatile ULONG *)((UBYTE *)(cb) + (o)))
#define BUSY(cb)  (*(volatile UBYTE *)((UBYTE *)(cb) + 2) & 0x40)
/* !blitwait: BTST #6,2(a1) twice (the first test only syncs) */
#define BLITWAIT(cb) do { (void)BUSY(cb); while (BUSY(cb)) ; } while (0)
#define MW(p, o)  (*(UWORD *)((UBYTE *)(p) + (o)))
#define ML(p, o)  (*(ULONG *)((UBYTE *)(p) + (o)))

static const UWORD masks[17] = {
    0xffff, 0x7fff, 0x3fff, 0x1fff, 0x0fff, 0x07ff, 0x03ff, 0x01ff,
    0x00ff, 0x007f, 0x003f, 0x001f, 0x000f, 0x0007, 0x0003, 0x0001, 0x0000,
};

extern const char mess1[], mess2[], mess3[];

/* --------------------------------------------------------- Plot, Point */

static void BLITZ_ROUTINE doplot_c(UWORD x BL_REG(d0), UWORD y BL_REG(d1),
                                   UWORD col BL_REG(d2), const UBYTE *bm BL_REG(a2))
{
    ULONG off;
    UBYTE bit;
    UWORD n;
    UBYTE **pl = (UBYTE **)(bm + 8);

    x += MW(bm, 48);
    y += MW(bm, 50);
    if (x >= MW(bm, 56) || y >= MW(bm, 58))
        return;
    off = (ULONG)y * MW(bm, 0) + (x >> 3);
    bit = ~x & 7;                               /* NOT d0, bit mod 8 */
    n = MW(bm, 4) - 1;
    if ((WORD)col < 0) {
        do
            (*pl++)[off] ^= 1 << bit;           /* BCHG */
        while (n-- != 0);
        return;
    }
    do {
        UBYTE *p = *pl++ + off;

        *p &= ~(1 << bit);
        if (col & 1)
            *p |= 1 << bit;
        col >>= 1;
    } while (n-- != 0);
}
BLITZ_THUNK(doplot, doplot_c, "d1/a0-a1");

static ULONG BLITZ_ROUTINE dopoint_c(ULONG d0 BL_REG(d0), UWORD y BL_REG(d1),
                                     const UBYTE *bm BL_REG(a2))
{
    UWORD x = d0 + MW(bm, 48), d2 = 0, i = 0;
    UBYTE **pl = (UBYTE **)(bm + 8);
    ULONG off;
    UBYTE bit;

    y += MW(bm, 50);
    if (x >= MW(bm, 56) || y >= MW(bm, 58))
        return 0;                               /* MOVEQ #0,d0 */
    off = (ULONG)y * MW(bm, 0) + (x >> 3);
    bit = ~x & 7;
    do {
        if ((*pl++)[off] & (1 << bit))
            d2 |= 1 << (i & 31);                /* BSET d3,d2 */
        i++;
    } while (i < MW(bm, 4));
    return (d0 & 0xffff0000) | d2;              /* MOVE d2,d0 (upper word of ~x) */
}
BLITZ_THUNK(dopoint, dopoint_c, "d1/a0-a1");

/* ------------------------------------------------------------------ Cls */

/* clsdepth - d0/d1 colour, d2 bltsize, d3 dest modulo, d4 offset */
static void clsdepth(UBYTE *cb, const UBYTE *bm, UWORD col, UWORD size, UWORD mod, ULONG off)
{
    UWORD n = MW(bm, 4);
    const ULONG *pl = (const ULONG *)(bm + 8);

    do {
        UWORD fill = (col & 1) ? 0xffff : 0;   /* LSR #1,d1:SCS:EXT */

        col >>= 1;
        BLITWAIT(cb);
        CW(cb, 0x74) = fill;
        CL(cb, 0x54) = *pl++ + off;
        CW(cb, 0x66) = mod;
        CW(cb, 0x58) = size;
    } while (--n);                              /* SUBQ #1,(a7):BNE */
}

/* clswidth - one band of height d2 (interleaved bitmap) */
static void clswidth(UBYTE *cb, const UBYTE *bm, UWORD col, UWORD h, ULONG off)
{
    UWORD size = h << 6, w = MW(bm, 54), mod;

    for (;;) {
        mod = MW(bm, 0);
        if ((WORD)w <= 128)
            break;
        clsdepth(cb, bm, col, size, mod - 128, off);    /* full width (64 words) */
        off += 128;
        w -= 128;
        if (!w)
            return;
    }
    mod -= w;
    size |= w >> 1;
    clsdepth(cb, bm, col, size, mod, off);
}

static void b_cls(UBYTE *cb, const UBYTE *bm, UWORD col)
{
    BLITWAIT(cb);
    CL(cb, 0x40) = 0x01f00000;
    CL(cb, 0x44) = 0xffffffff;
    if ((WORD)MW(bm, 52) < 0) {                 /* interleaved */
        ULONG off = 0;
        UWORD h = MW(bm, 58);

        for (;;) {
            if ((WORD)h <= 1024) {
                clswidth(cb, bm, col, h, off);
                return;
            }
            clswidth(cb, bm, col, 1024, off);
            h -= 1024;
            if (!h)
                return;
            off += (ULONG)MW(bm, 0) << 10;
        }
    }
    {                                           /* clsnotint (binary) */
        ULONG size = (ULONG)MW(bm, 0) * MW(bm, 58);
        UWORD n = MW(bm, 4) - 1;
        const ULONG *pl = (const ULONG *)(bm + 8);

        CW(cb, 0x66) = 0;
        do {
            UWORD fill = (col & 1) ? 0xffff : 0;
            ULONG d4 = size, d3;

            col >>= 1;
            BLITWAIT(cb);
            CW(cb, 0x74) = fill;
            CL(cb, 0x54) = *pl++;
            while ((LONG)d4 >= 131072) {
                BLITWAIT(cb);
                CW(cb, 0x58) = 0;
                d4 -= 131072;
            }
            d3 = d4;
            if (!d3)
                continue;
            d3 >>= 1;
            BL_SETW(d3, d3 & 0xffc0);
            if ((UWORD)d3) {
                BLITWAIT(cb);
                CW(cb, 0x58) = d3;
                d3 += d3;
                d4 -= d3;
                if (!d4)
                    continue;
            }
            d4 >>= 1;                           /* skip2 */
            BL_SETW(d4, d4 | 64);
            BLITWAIT(cb);
            CW(cb, 0x58) = d4;
        } while (n-- != 0);                     /* DBF d2 */
    }
}

static void BLITZ_ROUTINE b_docls_c(UBYTE *cb BL_REG(a1), const UBYTE *bm BL_REG(a2))
{
    b_cls(cb, bm, 0);
}

static void BLITZ_ROUTINE b_docls2_c(UWORD col BL_REG(d0), UBYTE *cb BL_REG(a1),
                                     const UBYTE *bm BL_REG(a2))
{
    b_cls(cb, bm, col);
}

static void BLITZ_ROUTINE a_docls_c(UBYTE *cb BL_REG(a1), const UBYTE *bm BL_REG(a2),
                                    struct GfxBase *gfx BL_REG(a6))
{
    OwnBlitter();
    b_cls(cb, bm, 0);
    BLITWAIT(cb);
    DisownBlitter();
}

static void BLITZ_ROUTINE a_docls2_c(UWORD col BL_REG(d0), UBYTE *cb BL_REG(a1),
                                     const UBYTE *bm BL_REG(a2), struct GfxBase *gfx BL_REG(a6))
{
    OwnBlitter();
    b_cls(cb, bm, col);
    BLITWAIT(cb);
    DisownBlitter();
}

BLITZ_THUNK(b_docls, b_docls_c, "d1/a0-a1");
BLITZ_AB_ROUTINE(docls, b_docls, "   jmp     _a_docls_c");
BLITZ_THUNK(b_docls2, b_docls2_c, "d1/a0-a1");
BLITZ_AB_ROUTINE(docls2, b_docls2, "   jmp     _a_docls2_c");

/* ---------------------------------------------------------- init */

static void BLITZ_ROUTINE codeinit_c(void)
{
    struct bl_regs r = { .d0 = CWIDTH, .d1 = MEMF_CHIP };
    ULONG *p;
    int i;

    p = (ULONG *)BLITZ_ALIBJSR(TOK_globalloc, r);
    cookie = (UWORD *)p;
    for (i = 0; i < CWIDTH / 4; i++)
        *p++ = 0xffffffff;
    offset = 0;
}
BLITZ_THUNK(codeinit, codeinit_c, "d1/a0-a1");

/* -------------------------------------------------------------- Scroll */

/* cookwait - wait, restore the cookie words of the last scroll to $FFFF */
static UWORD *cookwait(UBYTE *cb)
{
    UWORD *a5 = cookie;

    BLITWAIT(cb);
    a5[0] = 0xffff;
    MW(a5, CWIDTH - 2) = 0xffff;
    ML(a5, (WORD)offset) = 0xffffffff;
    MW(a5, (WORD)offset + 4) = 0xffff;
    return a5;
}

/* doit - the blit for all planes (d6 src / d7 dest offset, d5 words) */
static UBYTE *doit(UBYTE *cb, const UBYTE *src, const UBYTE *dst, const UWORD *args,
                   ULONG d6, ULONG d7, UWORD d5)
{
    UWORD n, bw, size;
    const ULONG *ps = (const ULONG *)(src + 8), *pd = (const ULONG *)(dst + 8);

    CL(cb, 0x44) = 0xffffffff;
    n = MW(src, 4);
    if (n > MW(dst, 4))
        n = MW(dst, 4);
    n--;
    bw = d5 + d5;
    CW(cb, 0x64) = MW(src, 0) - bw;
    CW(cb, 0x60) = MW(dst, 0) - bw;
    CW(cb, 0x66) = MW(dst, 0) - bw;
    CW(cb, 0x62) = -bw;
    size = (args[-2] << 6) | d5;
    for (;;) {
        ULONG d;

        CL(cb, 0x50) = *ps++ + d6;
        d = *pd++ + d7;
        CL(cb, 0x48) = d;
        CL(cb, 0x54) = d;
        CW(cb, 0x58) = size;
        if (n-- == 0)
            break;
        BLITWAIT(cb);
    }
    return (UBYTE *)pd;                         /* a3 after the loop */
}

static UBYTE *doasc(UWORD x1, UWORD y1, UWORD x2, UWORD y2, const UBYTE *src,
                    const UBYTE *dst, UBYTE *cb, const UWORD *args)
{
    ULONG d6 = (ULONG)y1 * MW(src, 0) + (LONG)((WORD)x1 >> 3);
    ULONG d7 = (ULONG)y2 * MW(dst, 0) + (LONG)((WORD)x2 >> 3);
    UWORD *a5 = cookwait(cb), d4, d5, d3, w = args[-1];

    d5 = (UWORD)(((x1 + w) & 0xfff0) - (x1 & 0xfff0));
    d5 = (d5 >> 4) + 3;
    d4 = (x2 & 15) - (x1 & 15);
    if ((WORD)d4 < 0) {
        d4 &= 15;
        d7 -= 2;
    }
    d4 = (UWORD)(d4 >> 4 | d4 << 12);           /* ROR #4 */
    CW(cb, 0x42) = d4;
    CW(cb, 0x40) = d4 | scminiterm;
    a5[0] = masks[x1 & 15];
    d3 = d5 + d5 - 6;
    offset = d3;
    d4 = ~masks[(x1 + w) & 15];
    MW(a5, (WORD)d3) &= d4;
    ML(a5, (WORD)d3 + 2) = 0;
    CL(cb, 0x4c) = (ULONG)a5;
    return doit(cb, src, dst, args, d6, d7, d5);
}

static UBYTE *dodesc(UWORD x1, UWORD y1, UWORD x2, UWORD y2, const UBYTE *src,
                     const UBYTE *dst, UBYTE *cb, const UWORD *args)
{
    UWORD w = args[-1], h = args[-2], d4, d5, d3;
    ULONG d6 = (ULONG)(UWORD)(y1 + h - 1) * MW(src, 0)
             + (LONG)((WORD)(UWORD)(x1 + w - 1) >> 3);
    ULONG d7 = (ULONG)(UWORD)(y2 + h - 1) * MW(dst, 0)
             + (LONG)((WORD)(UWORD)(x2 + w - 1) >> 3);
    UWORD *a5 = cookwait(cb);

    d5 = (UWORD)(((x1 + w - 1) & 0xfff0) - ((x1 - 1) & 0xfff0));
    d5 = (d5 >> 4) + 3;
    d3 = (x2 + w - 1) & 15;
    d4 = ((x1 + w - 1) & 15) - d3;
    if ((WORD)d4 < 0) {
        d4 &= 15;
        d7 += 2;
    }
    d4 = (UWORD)(d4 >> 4 | d4 << 12);
    CW(cb, 0x42) = d4 | 2;
    CW(cb, 0x40) = d4 | scminiterm;
    MW(a5, CWIDTH - 2) = ~masks[((x1 + w - 1) & 15) + 1];
    d3 = CWIDTH - (d5 + d5);
    offset = d3;
    ML(a5, (WORD)d3) = 0;
    MW(a5, (WORD)d3 + 4) &= masks[((x1 - 1) & 15) + 1];
    CL(cb, 0x4c) = (ULONG)((UBYTE *)a5 + CWIDTH - 2);
    return doit(cb, src, dst, args, d6, d7, d5);
}

/* returns a3 as the original leaves it (dest bitmap planes + 4 * planes;
   the 960 pixel split restores a3 in between) */
static UBYTE *scroll(UWORD x1, UWORD y1, UWORD x2, UWORD y2, const UBYTE *src,
                     UBYTE *cb, UWORD *args)
{
    const UBYTE *dst = blitz_string;            /* a3 = dest bitmap */

    if ((WORD)(args[-1] - 961) >= 0) {          /* CMP #961,-2(a2):BMI simon */
        UWORD w = args[-1];

        args[-1] = 960;
        scroll(x1, y1, x2, y2, src, cb, args);  /* both parts with the same d0-d3 */
        args[-1] = w - 960;
        x1 += 960;
        x2 += 960;
    }
    if (y2 > y1 || (y2 == y1 && x2 > x1))       /* simon */
        return dodesc(x1, y1, x2, y2, src, dst, cb, args);
    return doasc(x1, y1, x2, y2, src, dst, cb, args);
}

#define SCROLL_REGS UWORD x1 BL_REG(d0), UWORD y1 BL_REG(d1), UWORD x2 BL_REG(d2), \
    UWORD y2 BL_REG(d3), const UBYTE *src BL_REG(a0), UBYTE *cb BL_REG(a1), \
    UWORD *args BL_REG(a2)

static void BLITZ_ROUTINE b_doscroll_c(SCROLL_REGS)
{
    blitz_string = scroll(x1, y1, x2, y2, src, cb, args);
}

static void BLITZ_ROUTINE a_doscroll_c(SCROLL_REGS, struct GfxBase *gfx BL_REG(d6))
{
    UBYTE *a3;

    OwnBlitter();
    a3 = scroll(x1, y1, x2, y2, src, cb, args);
    BLITWAIT(cb);
    DisownBlitter();
    blitz_string = a3;
}
BLITZ_THUNK(b_doscroll, b_doscroll_c, "d1/a0-a1");
BLITZ_AB_ROUTINE(doscroll, b_doscroll, "   jmp     _a_doscroll_c");

/* --------------------------------------------------------- BlockScroll */

static UBYTE *blockscroll(UWORD x1, UWORD y1, UWORD x2, UWORD y2, const UBYTE *src,
                        UBYTE *cb, const UWORD *args)
{
    const UBYTE *dst = blitz_string;
    UWORD d4 = MW(src, 0), d5 = MW(dst, 0), n, d2;
    ULONG d1 = (ULONG)y1 * d4 + (LONG)((WORD)x1 >> 3);
    ULONG d3 = (ULONG)y2 * d5 + (LONG)((WORD)x2 >> 3);
    const ULONG *ps = (const ULONG *)(src + 8), *pd = (const ULONG *)(dst + 8);
    UWORD h = args[-2], w = args[-1];           /* MOVEM -4(a2),d0/d2 */

    d2 = w >> 3;
    d4 -= d2;
    d5 -= d2;
    d2 = ((WORD)d2 >> 1) | (UWORD)(h << 6);
    BLITWAIT(cb);
    CL(cb, 0x40) = 0x09f00000;
    CL(cb, 0x44) = 0xffffffff;
    CW(cb, 0x64) = d4;
    CW(cb, 0x66) = d5;
    n = MW(src, 4);
    if (n > MW(dst, 4))
        n = MW(dst, 4);
    n--;
    for (;;) {
        CL(cb, 0x50) = *ps++ + d1;
        CL(cb, 0x54) = *pd++ + d3;
        CW(cb, 0x58) = d2;
        if (n-- == 0)
            break;
        BLITWAIT(cb);
    }
    return (UBYTE *)pd;                         /* a3 after the loop */
}

static void BLITZ_ROUTINE b_doblockscroll_c(SCROLL_REGS)
{
    blitz_string = blockscroll(x1, y1, x2, y2, src, cb, args);
}

static void BLITZ_ROUTINE a_doblockscroll_c(SCROLL_REGS, struct GfxBase *gfx BL_REG(d6))
{
    UBYTE *a3;

    OwnBlitter();
    a3 = blockscroll(x1, y1, x2, y2, src, cb, args);
    BLITWAIT(cb);
    DisownBlitter();
    blitz_string = a3;
}
void b_doblockscroll(void);
BLITZ_THUNK(b_doblockscroll, b_doblockscroll_c, "d1/a0-a1");
BLITZ_AB_ROUTINE(doblockscroll, b_doblockscroll, "   jmp     _a_doblockscroll_c");

/* ------------------------------------------- error checks (debugger only) */

/* scchk - d4 = x, d5 = y, a0 = bitmap */
static void BLITZ_ERRCHK scchk(UWORD x, UWORD y, const UBYTE *bm, const UWORD *args)
{
    if (args[-2] > 1024)
        BLITZ_RUNERR(mess3);
    if (!MW(bm, 62))
        BLITZ_RUNERR(mess2);
    if (y >= MW(bm, 58))
        BLITZ_RUNERR(mess1);
    if ((UWORD)(y + args[-2]) > MW(bm, 58))
        BLITZ_RUNERR(mess1);
    if (x >= MW(bm, 56))
        BLITZ_RUNERR(mess1);
    if ((UWORD)(x + args[-1]) > MW(bm, 56))
        BLITZ_RUNERR(mess1);
}

static void BLITZ_ERRCHK scchk1_c(SCROLL_REGS)
{
    (void)cb;
    scchk(x1, y1, src, args);
    scchk(x2, y2, src, args);
}

static void BLITZ_ERRCHK scchk2_c(SCROLL_REGS)
{
    (void)cb;
    scchk(x1, y1, src, args);
    scchk(x2, y2, blitz_string, args);
}

static void BLITZ_ERRCHK bmchk_c(const UBYTE *bm BL_REG(a2))
{
    if (!MW(bm, 62))
        BLITZ_RUNERR(mess2);
}

BLITZ_ERRCHK_THUNK(scchk1, scchk1_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(scchk2, scchk2_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(bmchk, bmchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_MESSAGE(mess1, "Illegal Scroll Area");
BLITZ_ERRCHK_MESSAGE(mess2, "Uninitialized BitMap");
BLITZ_ERRCHK_MESSAGE(mess3, "Scroll area to large");
