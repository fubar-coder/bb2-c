/*
 * blitlib - command library 150: blitting shapes into the used bitmap -
 * Blit, BlitMode, CookieMode, EraseMode, InvMode, SolidMode, Block,
 * BlitColl.
 *
 * C translation of bblibsrc/blitlib (+ the include bliterrs; binary
 * checked: same as the source).  The routines program the blitter
 * directly (a1 = chipbaselib's $DFF000).  Blit/Block/BlitColl have
 * Blitz/Amiga variants (`Dc $a001`): the Amiga mode one takes the blitter
 * with OwnBlitter, waits for the last blit and gives it back
 * (DisownBlitter), the Blitz mode one only blits.
 *   Blit: per shape plane A = cookie, B = shape data (one pointer for all
 *         planes), C = D = bitmap plane; BLTCON0 = shift | bcon0 (BlitMode,
 *         default $FCA cookie cut).  With Excessonoff the bitmap planes
 *         beyond the shape's depth get A = cookie, minterm $0A (cleared
 *         where the cookie is) or $FA (set), one bit per plane.
 *   Block: A -> D copy of the shape data (no shift, x / 8 bytes, bltsize
 *         - 1 = without the extra word).
 *   BlitColl: A = cookie, C = bitmap plane, no D: -1 if any plane has a
 *         set pixel under the cookie (BZERO clear), else 0.
 *
 * Shapes (shapeslib): +0.w width (0 = none), +2.w height, +4.w depth,
 * +6.w even byte width, +8.w bltsize, +10/+12.w handle, +14 data, +18
 * cookie.  Bitmaps (bitmaplib): +0.w bytes per row, +4.w depth, +8
 * planes, +56/+58.w width/height, +62.w set up.
 *
 * Kept from the original, not a bug: Block works on byte positions (x >>
 * 3, the blitter ignores bit 0 of the address; Block is documented as word
 * boundary blitting); BlitColl tests all planes of the bitmap, not of the
 * shape (a collision with any non-zero bitmap pixel needs all planes); the
 * mode functions are inline MOVE.w (upper word of d0 stays; word results,
 * the compiler uses only d0.w).
 */
#include "blitzlib.h"
#define __NOLIBBASE__
#define GRAPHICS_BASE_NAME gfx
#include <proto/graphics.h>
#undef __NOLIBBASE__

/* entry points (thunks / C / asm, defined below) */
void doblit(void), doblitex(void), b_doblitex(void), doblock(void), doblitcoll(void);
void doblitmode(void), docookiemode(void), doerasemode(void), doinvmode(void);
void dosolidmode(void), codeinit(void);
void blitchk(void), blitchknc(void), blitchk3(void);

BLITZ_ENTRY_DECL(init);

/* !libheader{#blitlib,init,0,0,errors} */
BLITZ_LIBHEADER(LIB_blitlib, init, BL_NORET, 0, BLITZ_ERROR_AREA);

#define BLIT_LIBS BLITZ_LIBS(LIB_shapeslib, 0x1380, LIB_graphicslib, 0x5600, \
                             LIB_bitmaplib, 0x1202, LIB_chipbaselib, BL_LA(1))

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLIT_LIBS;
BLITZ_SUBS(doblit, blitchk, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLIT_LIBS;
BLITZ_SUBS(doblitex, blitchk, 0);
BLITZ_NAME("Blit", "Shape#,X,Y[,Excessonoff]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(doblitmode, 0, 0);
BLITZ_NAME("BlitMode", "BLTCON0");

#define MODE_FN(code, name)                     \
    BLITZ_AFUNCTION(BL_WORD);                   \
    BLITZ_ARGS0();                              \
    BLITZ_LIBS0();                              \
    BLITZ_SUBS(code, 0, 0);                     \
    BLITZ_NAME(name, "")

MODE_FN(docookiemode, "CookieMode");
MODE_FN(doerasemode, "EraseMode");
MODE_FN(doinvmode, "InvMode");
MODE_FN(dosolidmode, "SolidMode");

BLITZ_NULLSUB(b_doblitex, 0, 0);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLIT_LIBS;
BLITZ_SUBS(doblock, blitchknc, 0);
BLITZ_NAME("Block", "Shape#,X,Y");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLIT_LIBS;
BLITZ_SUBS(doblitcoll, blitchk3, 0);
BLITZ_NAME("BlitColl", "(Shape#,x,y)");

BLITZ_NULLSUB_AT(init, codeinit, 0, 0);

BLITZ_LIBFIN();

/* ******************* CODE BELOW **************** */

static UWORD bcon0 = 0xfca;

/* custom chip registers relative to the chip base */
/* BTST #6,$002(a1):'w:BTST #6,$002(a1):BNE 'w */

#define SW(p, o) (*(UWORD *)((UBYTE *)(p) + (o)))
#define SL(p, o) (*(ULONG *)((UBYTE *)(p) + (o)))

/* registers b_doblit leaves for b_doblitex */
struct blitstate {
    ULONG cookie;               /* d0 */
    UWORD shift;                /* d4 */
    UWORD size;                 /* d6 */
    ULONG off;                  /* d2 */
    UBYTE **plane;              /* a0: next bitmap plane */
};

/* the destination offset: MULS bpr,y + x ASR 3 */
static ULONG offset(const UBYTE *bm, WORD x, WORD y)
{
    return (ULONG)((LONG)y * (WORD)SW(bm, 0)) + (LONG)(WORD)(x >> 3);
}

/* the modulos: C = D = bpr - ebwidth - 2, A = B = -2 */
static void setmods(const UBYTE *shape, const UBYTE *bm, UBYTE *cb)
{
    ULONG d7 = 0xfffe0000UL | (UWORD)(SW(bm, 0) - SW(shape, 6) - 2);
    ULONG d6 = d7 << 16 | d7 >> 16;

    BL_BLITWAIT(cb);
    BL_CUSTOML(cb, 0x60) = d6;                          /* MOVEM.l d6-d7,$060 */
    BL_CUSTOML(cb, 0x64) = d7;
}

/* b_doblit - a3 = shape, a2 = used bitmap, a1 = chip base, d1/d2 = x/y */
static void blit(const UBYTE *shape, const UBYTE *bm, UBYTE *cb, WORD x, WORD y,
                 struct blitstate *st)
{
    UWORD d0, n;

    x -= SW(shape, 10);
    y -= SW(shape, 12);
    setmods(shape, bm, cb);
    d0 = (UWORD)((x & 15) << 12);               /* AND #15:ROR #4 */
    st->shift = d0;
    BL_CUSTOMW(cb, 0x42) = d0;
    BL_CUSTOMW(cb, 0x40) = d0 | bcon0;
    BL_CUSTOML(cb, 0x44) = 0xffff0000UL;
    BL_CUSTOML(cb, 0x4c) = SL(shape, 14);
    st->cookie = SL(shape, 18);
    st->off = offset(bm, x, y);
    st->plane = (UBYTE **)(bm + 8);
    st->size = SW(shape, 8);
    n = SW(shape, 4);
    if (!n)
        return;
    n--;
    goto doblit;
    do {
        BL_BLITWAIT(cb);
doblit:
        BL_CUSTOML(cb, 0x50) = st->cookie;
        {
            ULONG d = (ULONG)*st->plane++ + st->off;

            BL_CUSTOML(cb, 0x48) = d;
            BL_CUSTOML(cb, 0x54) = d;
        }
        BL_CUSTOMW(cb, 0x58) = st->size;
    } while (n-- != 0);
}

/* b_doblitex - and the excess planes: d3 = one bit per plane, set =
   minterm $FA, clear = $0A */
static void blitex(const UBYTE *shape, const UBYTE *bm, UBYTE *cb, WORD x, WORD y,
                   UWORD bits)
{
    struct blitstate st;
    UWORD p;

    blit(shape, bm, cb, x, y, &st);
    for (p = SW(shape, 4); p < SW(bm, 4); p++) {
        UWORD con = (bits & 1) ? 0xbfa : 0xb0a;
        ULONG d;

        bits >>= 1;
        con |= st.shift;
        d = (ULONG)*st.plane++ + st.off;
        BL_BLITWAIT(cb);
        BL_CUSTOMW(cb, 0x40) = con;
        BL_CUSTOML(cb, 0x50) = st.cookie;
        BL_CUSTOML(cb, 0x48) = d;
        BL_CUSTOML(cb, 0x54) = d;
        BL_CUSTOMW(cb, 0x58) = st.size;
    }
}

/* b_doblock - A -> D copy, without the extra word */
static void block(const UBYTE *shape, const UBYTE *bm, UBYTE *cb, WORD x, WORD y)
{
    UWORD n = SW(shape, 4), size;
    UBYTE **plane = (UBYTE **)(bm + 8);
    ULONG off;

    if (!n)
        return;
    n--;
    x -= SW(shape, 10);
    y -= SW(shape, 12);
    off = offset(bm, x, y);
    size = SW(shape, 8) - 1;
    BL_BLITWAIT(cb);
    BL_CUSTOML(cb, 0x40) = 0x09f00000;
    BL_CUSTOMW(cb, 0x66) = SW(bm, 0) - SW(shape, 6);
    BL_CUSTOML(cb, 0x44) = 0xffffffff;
    BL_CUSTOMW(cb, 0x64) = 0;
    BL_CUSTOML(cb, 0x50) = SL(shape, 14);
    goto doblit;
    do {
        BL_BLITWAIT(cb);
doblit:
        BL_CUSTOML(cb, 0x54) = (ULONG)*plane++ + off;
        BL_CUSTOMW(cb, 0x58) = size;
    } while (n-- != 0);
}

/* b_doblitcoll - A = cookie, C = every bitmap plane, no D: BZERO */
static UWORD blitcoll(const UBYTE *shape, const UBYTE *bm, UBYTE *cb, WORD x, WORD y)
{
    UWORD d0, n, size;
    UBYTE **plane = (UBYTE **)(bm + 8);
    ULONG cookie, off;

    x -= SW(shape, 10);
    y -= SW(shape, 12);
    setmods(shape, bm, cb);
    d0 = (UWORD)((x & 15) << 12);
    BL_CUSTOMW(cb, 0x42) = d0;
    BL_CUSTOMW(cb, 0x40) = d0 | 0xaa0;
    BL_CUSTOML(cb, 0x44) = 0xffff0000UL;
    cookie = SL(shape, 18);
    off = offset(bm, x, y);
    size = SW(shape, 8);
    n = SW(bm, 4) - 1;
    do {
        BL_CUSTOML(cb, 0x50) = cookie;
        BL_CUSTOML(cb, 0x48) = (ULONG)*plane++ + off;
        BL_CUSTOMW(cb, 0x58) = size;
        BL_BLITWAIT(cb);
        if (!(*(volatile UBYTE *)(cb + 2) & 0x20))      /* BTST #5: BZERO */
            return 0xffff;
    } while (n-- != 0);
    return 0;
}

/* ---------------------------------------------------- the entries */

#define REG_ARGS WORD x BL_REG(d1), WORD y BL_REG(d2), UBYTE *cb BL_REG(a1), \
                 const UBYTE *bm BL_REG(a2)

void BLITZ_ROUTINE b_doblit_c(REG_ARGS);
void BLITZ_ROUTINE b_doblit_c(REG_ARGS)
{
    struct blitstate st;

    blit(blitz_string, bm, cb, x, y, &st);
}

void BLITZ_ROUTINE a_doblit_c(REG_ARGS, struct Library *gfx BL_REG(a6));
void BLITZ_ROUTINE a_doblit_c(REG_ARGS, struct Library *gfx BL_REG(a6))
{
    struct blitstate st;

    OwnBlitter();
    blit(blitz_string, bm, cb, x, y, &st);
    BL_BLITWAIT(cb);
    DisownBlitter();
}
BLITZ_AB_ROUTINE(doblit, b_doblit_c, "   jmp     _a_doblit_c");

void BLITZ_ROUTINE b_doblitex_c(REG_ARGS, UWORD bits BL_REG(d3));
void BLITZ_ROUTINE b_doblitex_c(REG_ARGS, UWORD bits BL_REG(d3))
{
    blitex(blitz_string, bm, cb, x, y, bits);
}
BLITZ_THUNK(b_doblitex, b_doblitex_c, "a2");

void BLITZ_ROUTINE a_doblitex_c(REG_ARGS, UWORD bits BL_REG(d3),
                                struct Library *gfx BL_REG(a6));
void BLITZ_ROUTINE a_doblitex_c(REG_ARGS, UWORD bits BL_REG(d3),
                                struct Library *gfx BL_REG(a6))
{
    OwnBlitter();
    blitex(blitz_string, bm, cb, x, y, bits);
    BL_BLITWAIT(cb);
    DisownBlitter();
}
BLITZ_AB_ROUTINE(doblitex, b_doblitex_c, "   jmp     _a_doblitex_c");

void BLITZ_ROUTINE b_doblock_c(REG_ARGS);
void BLITZ_ROUTINE b_doblock_c(REG_ARGS)
{
    block(blitz_string, bm, cb, x, y);
}

void BLITZ_ROUTINE a_doblock_c(REG_ARGS, struct Library *gfx BL_REG(a6));
void BLITZ_ROUTINE a_doblock_c(REG_ARGS, struct Library *gfx BL_REG(a6))
{
    OwnBlitter();
    block(blitz_string, bm, cb, x, y);
    BL_BLITWAIT(cb);
    DisownBlitter();
}
BLITZ_AB_ROUTINE(doblock, b_doblock_c, "   jmp     _a_doblock_c");

/* BlitColl: result word (MOVEQ: full long) */
ULONG BLITZ_ROUTINE b_doblitcoll_c(REG_ARGS);
ULONG BLITZ_ROUTINE b_doblitcoll_c(REG_ARGS)
{
    return (ULONG)(LONG)(WORD)blitcoll(blitz_string, bm, cb, x, y);
}

/* Amiga mode: MOVE d0,-(a7):DisownBlitter:MOVE (a7)+,d0 - only the low
   word of the result is set, the upper word is what DisownBlitter left
   in d0 (DIFF: here 0; BlitColl is a #word function) */
ULONG BLITZ_ROUTINE a_doblitcoll_c(REG_ARGS, struct Library *gfx BL_REG(a6));
ULONG BLITZ_ROUTINE a_doblitcoll_c(REG_ARGS, struct Library *gfx BL_REG(a6))
{
    UWORD r;

    OwnBlitter();
    r = blitcoll(blitz_string, bm, cb, x, y);
    DisownBlitter();
    return r;
}
BLITZ_AB_ROUTINE(doblitcoll, b_doblitcoll_c, "   jmp     _a_doblitcoll_c");

/* BlitMode / init */
static void BLITZ_ROUTINE doblitmode_c(UWORD m BL_REG(d0))
{
    bcon0 = m;
}
BLITZ_THUNK(doblitmode, doblitmode_c, "d0-d1/a0-a1");

/* codeinit: MOVE #$fca,d0 and on into doblitmode */
static ULONG BLITZ_ROUTINE codeinit_c(ULONG d0 BL_REG(d0))
{
    bcon0 = 0xfca;
    return (d0 & 0xffff0000UL) | 0xfca;
}
BLITZ_THUNK(codeinit, codeinit_c, "d1/a0-a1");

/* the modes: inline MOVE #mode,d0 */
BLITZ_INLINE(docookiemode, "move.w #0xfca,d0");
BLITZ_INLINE(doerasemode, "move.w #0xb0a,d0");
BLITZ_INLINE(doinvmode, "move.w #0xb5a,d0");
BLITZ_INLINE(dosolidmode, "move.w #0xbfa,d0");

/* ------------------------------------------- error checks (debugger only) */

BLITZ_ERRCHK_MESSAGE(mess0, "Shape has too many BitPlanes");
BLITZ_ERRCHK_MESSAGE(mess1, "Unitialized BitMap");
BLITZ_ERRCHK_MESSAGE(mess2, "Uninialized Shape");
BLITZ_ERRCHK_MESSAGE(mess3, "Shape does not have a cookie cut");
BLITZ_ERRCHK_MESSAGE(mess4, "Coords outside of BitMap");

/* maxchk2 - bitmap set up, shape there; maxchk - and with cookie */
static int BLITZ_ERRCHK maxchk2(const UBYTE *shape, const UBYTE *bm)
{
    if (!SW(bm, 62)) {
        BLITZ_RUNERR(mess1);
        return 0;
    }
    if (!SW(shape, 0)) {
        BLITZ_RUNERR(mess2);
        return 0;
    }
    return 1;
}

static int BLITZ_ERRCHK maxchk(const UBYTE *shape, const UBYTE *bm)
{
    if (!SL(shape, 18)) {
        BLITZ_RUNERR(mess3);
        return 0;
    }
    return maxchk2(shape, bm);
}

/* poschk2 - the shape (minus handle) inside the bitmap; poschk - and
   not more planes than the bitmap */
static void BLITZ_ERRCHK poschk2(const UBYTE *shape, const UBYTE *bm, WORD x, WORD y)
{
    x -= SW(shape, 10);
    y -= SW(shape, 12);
    if (x < 0 || y < 0)
        BLITZ_RUNERR(mess4);
    else if ((UWORD)(x + SW(shape, 0)) > SW(bm, 56) || (UWORD)(y + SW(shape, 2)) > SW(bm, 58))
        BLITZ_RUNERR(mess4);
}

static void BLITZ_ERRCHK poschk(const UBYTE *shape, const UBYTE *bm, WORD x, WORD y)
{
    if (SW(shape, 4) > SW(bm, 4))
        BLITZ_RUNERR(mess0);
    else
        poschk2(shape, bm, x, y);
}

/* blitchk (Blit) - x/y in d1/d2 */
static void BLITZ_ERRCHK blitchk_c(WORD x BL_REG(d1), WORD y BL_REG(d2), const UBYTE *bm BL_REG(a2))
{
    if (maxchk(blitz_string, bm))
        poschk(blitz_string, bm, x, y);
}

/* blitchknc (Block) - no cookie needed */
static void BLITZ_ERRCHK blitchknc_c(WORD x BL_REG(d1), WORD y BL_REG(d2),
                                     const UBYTE *bm BL_REG(a2))
{
    if (maxchk2(blitz_string, bm))
        poschk(blitz_string, bm, x, y);
}

/* blitchk3 (BlitColl) - no depth check */
static void BLITZ_ERRCHK blitchk3_c(WORD x BL_REG(d1), WORD y BL_REG(d2),
                                    const UBYTE *bm BL_REG(a2))
{
    if (maxchk(blitz_string, bm))
        poschk2(blitz_string, bm, x, y);
}

/* the original checks change d5-d7 */
BLITZ_ERRCHK_THUNK(blitchk, blitchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(blitchknc, blitchknc_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(blitchk3, blitchk3_c, "d0-d1/a0-a1");
