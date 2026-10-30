/*
 * stencillib - command library 122: stencils (one plane masks) - Stencil,
 * SBlit, SBlitMode.
 *
 * C translation of bblibsrc/stencillib (+ bliterrs; binary checked: same
 * as the source).  An object library (!libfin{_toke,...,5,3}): the Blitz
 * object "Stencil" (8 bytes): +0.w bytes per row (0 = none), +2.w rows,
 * +4 memory (MEMF_CHIP|MEMF_CLEAR).
 *   Stencil s,bm: the OR of all planes of the bitmap (A | C -> D).
 *   SBlit s,shape,x,y: blits like blitlib and ORs the cookie into the
 *         stencil at the same position (bltcon0 $BFA).
 * Blitz/Amiga variants as in blitlib; Stencil uses $DFF000 directly.
 *
 * Kept from the original (BUG(orig), #284): the stencil's bltsize is rows << 6
 * | bpr / 2 (correct only up to 1024 rows / 64 words).  Not a bug:
 * FreeStencil leaves the memory pointer (the width is cleared, which is what
 * the free routine and the SBlit check test).
 */
#include "blitzlib.h"
#define __NOLIBBASE__
#define GRAPHICS_BASE_NAME gfx
#include <proto/graphics.h>
#undef __NOLIBBASE__

/* entry points (thunks / C / asm, defined below) */
void dostencil(void), doblit(void), doblitex(void), doblitmode(void);
void dofreestencil(void), sblitchk(void);

/* !libheader{#stencillib,0,0,0,errors} */
BLITZ_LIBHEADER(LIB_stencillib, 0, BL_NORET, 0, BLITZ_ERROR_AREA);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_stencillib, 0x1380, LIB_bitmaplib, 0x1281, LIB_graphicslib, 0x5600);
BLITZ_SUBS(dostencil, 0, 0);
BLITZ_NAME_AT(_toke, "Stencil", "Stencil#,BitMap#");

#define SBLIT_LIBS BLITZ_LIBS(LIB_stencillib, 0x1080, LIB_shapeslib, 0x1381,       \
                              LIB_graphicslib, 0x5600, LIB_bitmaplib, 0x1202,      \
                              LIB_chipbaselib, BL_LA(1))

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
SBLIT_LIBS;
BLITZ_SUBS(doblit, sblitchk, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
SBLIT_LIBS;
BLITZ_SUBS(doblitex, sblitchk, 0);
BLITZ_NAME("SBlit", "Stencil#,Shape#,X,Y[,Excessonoff]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(doblitmode, 0, 0);
BLITZ_NAME("SBlitMode", "BLTCON0");

BLITZ_ENTRY_DECL(_load);
BLITZ_ENTRY_DECL(_save);
BLITZ_ENTRY_DECL(_use);
BLITZ_ENTRY_DECL(_free);
BLITZ_NULLSUB_AT(_load, 0, 0, 0);
BLITZ_NULLSUB_AT(_save, 0, 0, 0);
BLITZ_NULLSUB_AT(_use, 0, 0, 0);
BLITZ_NULLSUB_AT(_free, dofreestencil, 0, 0);

/* !libfin{_toke,_load,_save,_use,_free,5,3} */
BLITZ_LIBFIN7(_toke.n, _load, _save, _use, _free, 5, 3);

/* ********************** CODE BELOW ************************ */

static UWORD bcon0 = 0xfca;

#define SW(p, o) (*(UWORD *)((UBYTE *)(p) + (o)))
#define SL(p, o) (*(ULONG *)((UBYTE *)(p) + (o)))

/* a stencil object */
struct stencil {
    UWORD bpr, rows;            /* +0, +2 */
    UBYTE *mem;                 /* +4 */
};

/* SBlitMode */
static void BLITZ_ROUTINE doblitmode_c(UWORD m BL_REG(d0))
{
    bcon0 = m;
}
BLITZ_THUNK(doblitmode, doblitmode_c, "d0-d1/a0-a1");

/* dofreestencil (_free) */
static void freest(struct stencil *s)
{
    UWORD b = s->bpr;

    if (b) {
        struct bl_regs r = { .d0 = (ULONG)b * s->rows, .a1 = (ULONG)s->mem };

        s->bpr = 0;
        BLITZ_ALIBJSR(TOK_globfree, r);
    }
}

static void BLITZ_ROUTINE dofreestencil_c(void)
{
    freest((struct stencil *)blitz_string);
}
BLITZ_THUNK(dofreestencil, dofreestencil_c, "a0");

/* b_dostencil - a3 = stencil, a2 = bitmap: OR of the planes */
static void makestencil(struct stencil *s, const UBYTE *bm)
{
    struct bl_regs r = { .d1 = MEMF_CHIP | MEMF_CLEAR };
    UBYTE *cb = BL_CUSTOM;
    UBYTE **plane = (UBYTE **)(bm + 8);
    ULONG mem;
    UWORD size, n;

    freest(s);
    r.d0 = (ULONG)SW(bm, 0) * SW(bm, 2);
    mem = BLITZ_ALIBJSR(TOK_globalloc, r);
    *(ULONG *)s = SL(bm, 0);
    s->mem = (UBYTE *)mem;
    size = (UWORD)(SW(bm, 2) << 6) | (UWORD)(SW(bm, 0) >> 1);
    n = SW(bm, 4) - 1;
    BL_BLITWAIT(cb);
    BL_CUSTOML(cb, 0x40) = 0x0bfa0000;
    BL_CUSTOML(cb, 0x44) = 0xffffffff;
    BL_CUSTOML(cb, 0x64) = 0;
    BL_CUSTOMW(cb, 0x60) = 0;
    goto loop2;
    do {
        BL_BLITWAIT(cb);
loop2:
        BL_CUSTOML(cb, 0x50) = (ULONG)*plane++;
        BL_CUSTOML(cb, 0x48) = mem;
        BL_CUSTOML(cb, 0x54) = mem;
        BL_CUSTOMW(cb, 0x58) = size;
    } while (n-- != 0);
}

void BLITZ_ROUTINE b_dostencil_c(const UBYTE *bm BL_REG(a2));
void BLITZ_ROUTINE b_dostencil_c(const UBYTE *bm BL_REG(a2))
{
    makestencil((struct stencil *)blitz_string, bm);
}

void BLITZ_ROUTINE a_dostencil_c(const UBYTE *bm BL_REG(a2), struct Library *gfx BL_REG(a6));
void BLITZ_ROUTINE a_dostencil_c(const UBYTE *bm BL_REG(a2), struct Library *gfx BL_REG(a6))
{
    OwnBlitter();
    makestencil((struct stencil *)blitz_string, bm);
    BL_BLITWAIT(BL_CUSTOM);
    DisownBlitter();
}
BLITZ_AB_ROUTINE(dostencil, b_dostencil_c, "   jmp     _a_dostencil_c");

struct blitstate {
    ULONG cookie;               /* d0 */
    UWORD shift;                /* d4 */
    UWORD size;                 /* d6 */
    ULONG off;                  /* d2 */
    UBYTE **plane;              /* a0 */
};

/* b_doblit - a0 = stencil, a3 = shape, a2 = bitmap, a1 = chip base, d2/d3
   = x/y: blit, then OR the cookie into the stencil */
static void sblit(const struct stencil *s, const UBYTE *shape, const UBYTE *bm, UBYTE *cb,
                  WORD x, WORD y, struct blitstate *st)
{
    ULONG smem = (ULONG)s->mem, d7, d6;
    UWORD n;

    x -= SW(shape, 10);
    y -= SW(shape, 12);
    d7 = 0xfffe0000UL | (UWORD)(SW(bm, 0) - SW(shape, 6) - 2);
    d6 = d7 << 16 | d7 >> 16;
    BL_BLITWAIT(cb);
    BL_CUSTOML(cb, 0x60) = d6;
    BL_CUSTOML(cb, 0x64) = d7;
    st->shift = (UWORD)((x & 15) << 12);
    BL_CUSTOMW(cb, 0x42) = st->shift;
    BL_CUSTOMW(cb, 0x40) = st->shift | bcon0;
    BL_CUSTOML(cb, 0x44) = 0xffff0000UL;
    BL_CUSTOML(cb, 0x4c) = SL(shape, 14);
    st->cookie = SL(shape, 18);
    st->off = (ULONG)((LONG)y * (WORD)SW(bm, 0)) + (LONG)(WORD)(x >> 3);
    st->plane = (UBYTE **)(bm + 8);
    n = SW(shape, 4) - 1;
    st->size = SW(shape, 8);
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
    /* the cookie into the stencil */
    BL_BLITWAIT(cb);
    BL_CUSTOML(cb, 0x50) = st->cookie;
    BL_CUSTOML(cb, 0x48) = smem + st->off;
    BL_CUSTOML(cb, 0x54) = smem + st->off;
    BL_CUSTOMW(cb, 0x40) = st->shift | 0xbfa;
    BL_CUSTOMW(cb, 0x58) = st->size;
}

static void sblitex(const struct stencil *s, const UBYTE *shape, const UBYTE *bm, UBYTE *cb,
                    WORD x, WORD y, UWORD bits)
{
    struct blitstate st;
    UWORD p;

    sblit(s, shape, bm, cb, x, y, &st);
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

#define SBLIT_REGS WORD x BL_REG(d2), WORD y BL_REG(d3), const struct stencil *s BL_REG(a0), \
                   UBYTE *cb BL_REG(a1), const UBYTE *bm BL_REG(a2)

void BLITZ_ROUTINE b_doblit_c(SBLIT_REGS);
void BLITZ_ROUTINE b_doblit_c(SBLIT_REGS)
{
    struct blitstate st;

    sblit(s, blitz_string, bm, cb, x, y, &st);
}

void BLITZ_ROUTINE a_doblit_c(SBLIT_REGS, struct Library *gfx BL_REG(a6));
void BLITZ_ROUTINE a_doblit_c(SBLIT_REGS, struct Library *gfx BL_REG(a6))
{
    struct blitstate st;

    OwnBlitter();
    sblit(s, blitz_string, bm, cb, x, y, &st);
    BL_BLITWAIT(cb);
    DisownBlitter();
}
BLITZ_AB_ROUTINE(doblit, b_doblit_c, "   jmp     _a_doblit_c");

void BLITZ_ROUTINE b_doblitex_c(SBLIT_REGS, UWORD bits BL_REG(d4));
void BLITZ_ROUTINE b_doblitex_c(SBLIT_REGS, UWORD bits BL_REG(d4))
{
    sblitex(s, blitz_string, bm, cb, x, y, bits);
}

void BLITZ_ROUTINE a_doblitex_c(SBLIT_REGS, UWORD bits BL_REG(d4),
                                struct Library *gfx BL_REG(a6));
void BLITZ_ROUTINE a_doblitex_c(SBLIT_REGS, UWORD bits BL_REG(d4),
                                struct Library *gfx BL_REG(a6))
{
    OwnBlitter();
    sblitex(s, blitz_string, bm, cb, x, y, bits);
    BL_BLITWAIT(cb);
    DisownBlitter();
}
BLITZ_AB_ROUTINE(doblitex, b_doblitex_c, "   jmp     _a_doblitex_c");

/* ------------------------------------------- error checks (debugger only) */

BLITZ_ERRCHK_MESSAGE(messz, "Uninitialized Stencil");
BLITZ_ERRCHK_MESSAGE(mess0, "Shape has too many BitPlanes");
BLITZ_ERRCHK_MESSAGE(mess1, "Unitialized BitMap");
BLITZ_ERRCHK_MESSAGE(mess2, "Uninialized Shape");
BLITZ_ERRCHK_MESSAGE(mess3, "Shape does not have a cookie cut");
BLITZ_ERRCHK_MESSAGE(mess4, "Coords outside of BitMap");

/* sblitchk - stencil there, then blitchk with x/y from d2/d3 */
static void BLITZ_ERRCHK sblitchk_c(WORD x BL_REG(d2), WORD y BL_REG(d3),
                                    const UWORD *s BL_REG(a0), const UBYTE *bm BL_REG(a2))
{
    const UBYTE *shape = blitz_string;

    if (!*s)
        BLITZ_RUNERR(messz);
    else if (!SL(shape, 18))
        BLITZ_RUNERR(mess3);
    else if (!SW(bm, 62))
        BLITZ_RUNERR(mess1);
    else if (!SW(shape, 0))
        BLITZ_RUNERR(mess2);
    else if (SW(shape, 4) > SW(bm, 4))
        BLITZ_RUNERR(mess0);
    else {
        x -= SW(shape, 10);
        y -= SW(shape, 12);
        if (x < 0 || y < 0 || (UWORD)(x + SW(shape, 0)) > SW(bm, 56)
            || (UWORD)(y + SW(shape, 2)) > SW(bm, 58))
            BLITZ_RUNERR(mess4);
    }
}

/* the original changes d1 (= x) and d5-d7 */
BLITZ_ERRCHK_THUNK(sblitchk, sblitchk_c, "d0-d1/a0-a1");
