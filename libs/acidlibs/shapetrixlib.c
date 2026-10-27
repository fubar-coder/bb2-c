/*
 * shapetrixlib - command library 118: "groovy scaling/rotating
 * functions" for shapes: Scale XFlip YFlip Rotate.
 *
 * C translation of bblibsrc/shapetrixlib; binary of the CD's acidlibs
 * checked: same as the source.  `libincs:qrot` (IncBin) is not on the CD;
 * the table (sin of 0..64/256 turns as 16 bit fractions, -1 = 1.0) is
 * taken from the binary.
 *
 * All four replace the shape's data (and its cookie: if it had one,
 * shapeslib +11 docookie makes a new one).  The new shape of Scale and
 * Rotate is built in `newshape` (MEMF_CHIP|MEMF_CLEAR) and copied over the
 * old block (makeshape).
 *
 * XFlip (Amiga mode: OwnBlitter around the Blitz mode routine): first the
 * planes are shifted right with the blitter by ebw*8-width pixels (so the
 * image is "hard left" after the flip; one blit per plane, the same
 * pointer as source and destination, modulo 0), then every row's bytes
 * are mirrored and bit-reversed by the CPU.  YFlip swaps rows.  Rotate:
 * angle = Quick 0..1 -> 256 steps; the new size comes from the rotated
 * corners; every destination pixel is rotated back (rotit, sin table,
 * rounding +-$8000) and copied when it falls inside the source.  Scale:
 * new size = size * ratio (lmullib $CA00, at least 1), source steps
 * 1/ratio (ldivlib $CB00); every destination pixel takes the colour with
 * the highest sum of "brightness" over the source pixels it covers
 * (without a palette all colours count 1, so the most frequent colour;
 * with a palette r+g+b+3 of the top bytes).
 *
 * BUG(orig), kept:
 *  - XFlip: no wait after the last blit before the CPU flips the bytes
 *    (BTST twice without a loop in the Amiga part); the shift uses the
 *    shape's bltsize-1 words per plane.  The result depends on timing:
 *    the C loop is faster and may reach the last plane while the blit
 *    still runs (seen with 13 and 21 pixel shapes, the latter only with
 *    runtime error checking on); t141 tests XFlip only without the
 *    shift blit (width = byte width * 8).
 *  - The brightness table keeps the values of an earlier Scale with a
 *    palette for colours beyond the current palette (never used: the
 *    colour choice only compares usevals[0..numcols], so no effect).
 *  - Scale reads source pixels to the right/bottom of the shape when a
 *    target size below 1 is raised to 1 (reads only).
 *  - Word arithmetic: row/plane steps ADDA.w, sizes are MULU low words.
 */
#include "blitzlib.h"
#define __NOLIBBASE__
#include <proto/graphics.h>
#undef __NOLIBBASE__

/* entry points (thunks / assembler, defined below) */
void doscale(void), doscale2(void), doxflip(void), doyflip(void), dorotate(void);
void scalechk(void), scalechk2(void), fchk(void);

/* !libheader{#shapetrixlib,0,0,0,errors} */
BLITZ_LIBHEADER(LIB_shapetrixlib, 0, BL_NORET, 0, BLITZ_ERROR_AREA);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_QUICK, BL_QUICK);
BLITZ_LIBS(LIB_shapeslib, 0x1380);
BLITZ_SUBS(doscale2, scalechk2, 0);
BLITZ_ARGS(BL_WORD, BL_QUICK, BL_QUICK, BL_WORD);
BLITZ_LIBS(LIB_shapeslib, 0x1380, LIB_palettelib, 0x1283);
BLITZ_SUBS(doscale, scalechk, 0);
BLITZ_NAME("Scale", "Shape#,X Ratio,Y Ratio[,Palette#]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_shapeslib, 0x1380, LIB_chipbaselib, BL_LA(1), LIB_graphicslib, 0x5600);
BLITZ_SUBS(doxflip, fchk, 0);
BLITZ_NAME("XFlip", "Shape#");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_shapeslib, 0x1380);
BLITZ_SUBS(doyflip, fchk, 0);
BLITZ_NAME("YFlip", "Shape#");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_QUICK);
BLITZ_LIBS(LIB_shapeslib, 0x1380);
BLITZ_SUBS(dorotate, fchk, 0);
BLITZ_NAME("Rotate", "Shape#,Angle Ratio");

BLITZ_LIBFIN();

/* ------------------------------------------------------------- helpers */

#define SW(s, o) (*(UWORD *)((UBYTE *)(s) + (o)))
#define SL(s, o) (*(ULONG *)((UBYTE *)(s) + (o)))
#define SP(s, o) (*(UBYTE **)((UBYTE *)(s) + (o)))

#define TOK_docookie  ((LIB_shapeslib << 7) + 11)
#define TOK_qmultiply 0xca00                    /* lmullib: d0.q * d1.q */
#define TOK_qdivide   0xcb00                    /* ldivlib: d0.q / d1.q */

static void *globalloc(ULONG size, ULONG flags)
{
    struct bl_regs r = { .d0 = size, .d1 = flags };

    return (void *)BLITZ_ALIBJSR(TOK_globalloc, r);
}

static void globfree(void *mem, ULONG size)
{
    struct bl_regs r = { .d0 = size, .a1 = (ULONG)mem };

    BLITZ_ALIBJSR(TOK_globfree, r);
}

/* shapeslib docookie with a3 = shape (a3/a4 kept around the call) */
static void docookie(UBYTE *shape)
{
#ifndef __clang__
    register UBYTE *_s __asm("a0") = shape;

    __asm volatile("movem.l a3-a4,-(sp)\n\t"
                   "move.l %0,a3\n\t"
                   "jsr _blitz_tok+%c1\n\t"
                   "movem.l (sp)+,a3-a4"
        : "+a"(_s) : "i"(BL_TOKVAL(TOK_docookie, 0))
        : "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "a1", "a2", "a6",
          "cc", "memory");
#else
    (void)shape;
#endif
}

static ULONG qcall(ULONG val, ULONG a, ULONG b)
{
    struct bl_regs r = { .d0 = a, .d1 = b };

    if (val == TOK_qmultiply)
        return BLITZ_ALIBJSR(TOK_qmultiply, r);
    return BLITZ_ALIBJSR(TOK_qdivide, r);
}

/* re-cookie if the shape has a cookie */
static void cookieit(UBYTE *s)
{
    if (SL(s, 18))
        docookie(s);
}

/* --------------------------------------------------------- newshape */

static UBYTE newshape[32] __attribute__((aligned(4)));
static UWORD colvals[256];                      /* table of brightness's */
static UWORD usevals[256];                      /* currently collected pixels */
static ULONG xadd, yadd;
static UWORD numcols;                           /* dbf */

/* allocshape: width, height, depth of newshape set; the rest and the
   memory */
static void allocshape(UBYTE *a5)
{
    UWORD d0 = (UWORD)(SW(a5, 0) + 15) >> 4;   /* word width */
    ULONG p;

    SW(a5, 8) = (UWORD)((UWORD)(SW(a5, 2) << 6) | d0) + 1;    /* bltsize */
    SW(a5, 6) = d0 + d0;                        /* even byte width */
    p = (ULONG)(UWORD)(SW(a5, 6) + 2) * SW(a5, 2);
    SW(a5, 24) = (UWORD)p;
    p = (ULONG)(UWORD)p * SW(a5, 4);
    SW(a5, 28) = (UWORD)p;
    p = (ULONG)SW(a5, 6) * SW(a5, 2);
    SW(a5, 22) = (UWORD)p;
    p = (ULONG)(UWORD)p * SW(a5, 4);
    SW(a5, 26) = (UWORD)p;
    SP(a5, 14) = globalloc(p, MEMF_CHIP | MEMF_CLEAR);
    SL(a5, 10) = 0;                             /* handle */
    SL(a5, 18) = 0;                             /* cookie */
}

/* makeshape: newshape into shape a3 */
static void makeshape(UBYTE *a3)
{
    ULONG cookie;
    int i;

    globfree(SP(a3, 14), SW(a3, 26));
    cookie = SL(a3, 18);
    if (cookie)
        globfree((void *)cookie, SW(a3, 22));
    for (i = 0; i < 8; i++)
        ((ULONG *)a3)[i] = ((ULONG *)newshape)[i];
    if (cookie)
        docookie(a3);
}

/* -------------------------------------------------------------- XFlip */

/* reverse bits in a byte */
static UBYTE flipbyte(UBYTE b)
{
    UBYTE r = 0;
    int i;

    for (i = 0; i < 8; i++) {
        r = (UBYTE)(r << 1) | (b & 1);
        b >>= 1;
    }
    return r;
}

static void blitwait(volatile UBYTE *chip)
{
    (void)chip[2];
    while (chip[2] & 0x40)
        ;
}

/* a3 = shape, a1 = chip base */
static void xflip(UBYTE *s, volatile UBYTE *chip)
{
    UWORD d0 = (UWORD)(SW(s, 6) << 3) - SW(s, 0);
    UWORD d7;
    UBYTE *a0;

    if (d0) {
        /* scroll across to the right, 'hard left' justified after the flip */
        UWORD d1;

        blitwait(chip);
        d0 &= 15;
        d0 = (UWORD)((d0 >> 4) | (d0 << 12));   /* ROR #4 */
        *(volatile UWORD *)(chip + 0x040) = d0 | 0x9f0;
        *(volatile UWORD *)(chip + 0x042) = 0;
        *(volatile ULONG *)(chip + 0x044) = 0xffffffffUL;
        *(volatile ULONG *)(chip + 0x064) = 0;
        *(volatile ULONG *)(chip + 0x050) = SL(s, 14);
        *(volatile ULONG *)(chip + 0x054) = SL(s, 14);
        d1 = SW(s, 4) - 1;
        d0 = SW(s, 8) - 1;
        do {
            blitwait(chip);
            *(volatile UWORD *)(chip + 0x058) = d0;
        } while (d1-- != 0);
    }
    /* now, to do the flipping! */
    d7 = (UWORD)((ULONG)SW(s, 4) * SW(s, 2)) - 1;
    a0 = SP(s, 14);
    do {
        UWORD l = 0, r = SW(s, 6) - 1;          /* left, right bytes */

        do {
            UBYTE b5 = flipbyte(a0[(WORD)l]);
            UBYTE b2 = flipbyte(a0[(WORD)r]);

            a0[(WORD)r] = b5;
            a0[(WORD)l] = b2;
            l++;
            r--;
        } while (r > l);
        a0 += (WORD)SW(s, 6);
        __asm__("" : "+a"(a0));
    } while (d7-- != 0);
    cookieit(s);
}

void BLITZ_ROUTINE b_doxflip_c(volatile UBYTE *chip BL_REG(a1));
void BLITZ_ROUTINE b_doxflip_c(volatile UBYTE *chip BL_REG(a1))
{
    xflip(blitz_string, chip);
}
BLITZ_THUNK(b_doxflip, b_doxflip_c, "d0-d1/a0-a1");

/* Amiga mode: OwnBlitter, flip, (no real wait), DisownBlitter */
void BLITZ_ROUTINE a_doxflip_c(volatile UBYTE *chip BL_REG(a1), struct GfxBase *gfx BL_REG(a6));
void BLITZ_ROUTINE a_doxflip_c(volatile UBYTE *chip BL_REG(a1), struct GfxBase *gfx BL_REG(a6))
{
    struct GfxBase *GfxBase = gfx;
    volatile UBYTE *dmaconr = (volatile UBYTE *)0xdff002;

    OwnBlitter();
    xflip(blitz_string, chip);
    (void)*dmaconr;                             /* BTST twice, no loop */
    (void)*dmaconr;
    DisownBlitter();
}
BLITZ_AB_ROUTINE(doxflip, b_doxflip,
                 "   movem.l d0-d1/a0-a1,-(sp)\n"
                 "   jsr     _a_doxflip_c\n"
                 "   movem.l (sp)+,d0-d1/a0-a1\n"
                 "   rts");

/* -------------------------------------------------------------- YFlip */

static void BLITZ_ROUTINE doyflip_c(void)
{
    UBYTE *s = blitz_string;
    UWORD d7 = SW(s, 4) - 1;                    /* bitplanes */
    UBYTE *a2 = SP(s, 14);

    do {
        UBYTE *a0 = a2, *a1 = a2 + (WORD)SW(s, 22);   /* top, bottom */

        for (;;) {
            WORD d0;

            a1 -= (WORD)SW(s, 6);
            if ((ULONG)a1 <= (ULONG)a0)
                break;
            d0 = (WORD)(SW(s, 6) - 2);
            do {
                UWORD t = *(UWORD *)(a0 + d0);

                *(UWORD *)(a0 + d0) = *(UWORD *)(a1 + d0);
                *(UWORD *)(a1 + d0) = t;
                d0 -= 2;
            } while (d0 >= 0);
            a0 += (WORD)SW(s, 6);
        }
        a2 += (WORD)SW(s, 22);
    } while (d7-- != 0);
    cookieit(s);
}
BLITZ_THUNK(doyflip, doyflip_c, "d0-d1/a0-a1");

/* ------------------------------------------------------------- Rotate */

/* sin of 0..64 (1/256 turns) as 16 bit fraction; -1 = 1.0
   (libincs:qrot, from the binary) */
static const UWORD qrots[65] = {
    0x0000, 0x0648, 0x0c8f, 0x12d5, 0x1917, 0x1f56, 0x2590, 0x2bc4,
    0x31f1, 0x3817, 0x3e33, 0x4447, 0x4a50, 0x504d, 0x563e, 0x5c22,
    0x61f7, 0x67bd, 0x6d74, 0x7319, 0x78ad, 0x7e2e, 0x839c, 0x88f5,
    0x8e39, 0x9368, 0x987f, 0x9d7f, 0xa267, 0xa736, 0xabeb, 0xb085,
    0xb504, 0xb968, 0xbdae, 0xc1d8, 0xc5e4, 0xc9d1, 0xcd9f, 0xd14d,
    0xd4db, 0xd848, 0xdb94, 0xdebe, 0xe1c5, 0xe4aa, 0xe76b, 0xea09,
    0xec83, 0xeed8, 0xf109, 0xf314, 0xf4fa, 0xf6ba, 0xf853, 0xf9c7,
    0xfb14, 0xfc3b, 0xfd3a, 0xfe13, 0xfec4, 0xff4e, 0xffb1, 0xffec,
    0xffff
};

/* d2.q = sin(d7) * d2.w */
static ULONG mulsin(WORD val, UWORD angle)
{
    UWORD idx, t;
    int neg = 0;
    ULONG r;

    if (angle <= 64)
        idx = angle;                            /* first quadrant */
    else if (angle <= 128)
        idx = 128 - angle;
    else if (angle <= 192) {
        idx = angle - 128;
        neg = 1;
    } else {
        idx = 256 - angle;
        neg = 1;
    }
    t = qrots[idx];
    if (t == 0xffff)
        r = (ULONG)(UWORD)val << 16;            /* SWAP:CLR */
    else if (val < 0)
        r = -((ULONG)(UWORD)-val * t);          /* NEG:MULU:NEG.l */
    else
        r = (ULONG)(UWORD)val * t;
    return neg ? -r : r;
}

static ULONG mulcos(WORD val, UWORD angle)
{
    return mulsin(val, (UWORD)(angle + 64) & 255);
}

/* rotit: (x, y) by angle -> rounded new x, y */
static void rotit(WORD x, WORD y, UWORD angle, WORD *nx, WORD *ny)
{
    ULONG rx = mulcos(x, angle) + mulsin(y, angle);
    ULONG ry = mulcos(y, angle) - mulsin(x, angle);

    ry += (LONG)ry < 0 ? -0x8000UL : 0x8000UL;
    rx += (LONG)rx < 0 ? -0x8000UL : 0x8000UL;
    *nx = (WORD)(rx >> 16);
    *ny = (WORD)(ry >> 16);
}

static WORD xl, yl, xh, yh;

static void rotit2(WORD x, WORD y, UWORD angle)
{
    WORD nx, ny;

    rotit(x, y, angle, &nx, &ny);
    if (nx < xl)
        xl = nx;
    if (nx > xh)
        xh = nx;
    if (ny < yl)
        yl = ny;
    if (ny > yh)
        yh = ny;
}

/* a3 = shape, d1 = angle (Quick, 0..1) */
static void BLITZ_ROUTINE dorotate_c(ULONG q BL_REG(d1))
{
    UBYTE *s = blitz_string, *a5 = newshape, *a0;
    UWORD d7 = 256 - (UWORD)(((q << 8) >> 16) & 255);
    WORD y;

    xl = yl = xh = yh = 0;                      /* initial low/high vals */
    rotit2(SW(s, 0) - 1, 0, d7);
    rotit2(SW(s, 0) - 1, SW(s, 2) - 1, d7);
    rotit2(0, SW(s, 2) - 1, d7);
    SW(a5, 0) = xh - xl + 1;
    SW(a5, 2) = yh - yl + 1;
    SW(a5, 4) = SW(s, 4);
    allocshape(a5);
    d7 = 256 - d7;
    y = yl;
    a0 = SP(a5, 14);
    do {
        UBYTE *a1 = a0;
        WORD x = xl;
        WORD d6 = 7;

        do {
            WORD nx, ny;

            rotit(x, y, d7, &nx, &ny);
            if ((UWORD)nx < SW(s, 0) && (UWORD)ny < SW(s, 2)) {
                /* transfer pixel from src to dest */
                UBYTE *a4 = SP(s, 14) + (WORD)((UWORD)nx >> 3)
                            + (ULONG)(UWORD)ny * SW(s, 6);
                UWORD bit = ~(UWORD)nx & 7;
                UWORD d1 = SW(s, 4) - 1;
                UBYTE *a2 = a1;

                do {
                    if (*a4 & (1 << bit))
                        *a2 |= (UBYTE)(1 << d6);
                    a4 += (WORD)SW(s, 22);
                    a2 += (WORD)SW(a5, 22);
                } while (d1-- != 0);
            }
            if (--d6 < 0) {                     /* across an X... */
                d6 = 7;
                a1++;
            }
            x++;
        } while (x <= xh);
        a0 += (WORD)SW(a5, 6);                  /* down a Y... */
        y++;
    } while (y <= yh);
    makeshape(s);
}
BLITZ_THUNK(dorotate, dorotate_c, "d0-d1/a0-a1");

/* -------------------------------------------------------------- Scale */

static void scale(UBYTE *a3, ULONG d6, ULONG d7)
{
    UBYTE *a5 = newshape, *a1;
    ULONG d0, d1, w;
    UWORD d2, d3;

    w = qcall(TOK_qmultiply, (ULONG)SW(a3, 0) << 16, d6);
    if ((WORD)(w >> 16) < 1)
        w = 1UL << 16;
    SW(a5, 0) = (UWORD)(w >> 16);
    w = qcall(TOK_qmultiply, (ULONG)SW(a3, 2) << 16, d7);
    if ((WORD)(w >> 16) < 1)
        w = 1UL << 16;
    SW(a5, 2) = (UWORD)(w >> 16);
    SW(a5, 4) = SW(a3, 4);
    allocshape(a5);
    xadd = qcall(TOK_qdivide, 1UL << 16, d6);   /* 1/ratio */
    yadd = qcall(TOK_qdivide, 1UL << 16, d7);
    d1 = 0;
    d3 = 0;
    a1 = SP(a5, 14);
    do {                                        /* yloop */
        ULONG ny = d1 + yadd;
        WORD d5 = (WORD)((ny >> 16) - (d1 >> 16)) - 1;
        UBYTE *a0;

        if (d5 < 0)
            d5 = 0;
        d0 = 0;
        d2 = 0;
        a0 = SP(a3, 14) + (ULONG)(UWORD)(d1 >> 16) * SW(a3, 6);
        do {                                    /* xloop */
            ULONG nx = d0 + xadd;
            WORD d4 = (WORD)((nx >> 16) - (d0 >> 16)) - 1;
            UWORD d6c = 0;

            if (d4 < 0)
                d4 = 0;
            {
                UWORD i, sx = (UWORD)(d0 >> 16);
                UBYTE *p = a0 + (WORD)(sx >> 3);
                UWORD bit = ~sx & 7;
                WORD best;
                WORD rows = d5;

                for (i = 0; ; i++) {            /* clear usevals */
                    usevals[i] = 0;
                    if (i == numcols)
                        break;
                }
                do {                            /* yl */
                    UBYTE *q = p;
                    UWORD b = bit;
                    WORD cols = d4;

                    do {                        /* xl */
                        UWORD pl = 0;
                        ULONG c = 0;

                        do {
                            if (*q & (1 << (b & 7)))
                                c |= 1UL << (pl & 31);  /* BSET d2,d6 */
                            q += (WORD)SW(a3, 22);
                            __asm__("" : "+a"(q));      /* no __mulsi3 */
                            pl++;
                        } while (pl < SW(a3, 4));
                        usevals[(UWORD)c] += colvals[(UWORD)c];
                        q -= (WORD)SW(a3, 26);
                        if ((WORD)--b < 0) {
                            b = 7;
                            q++;
                        }
                        __asm__("" : "+a"(q));
                    } while (cols-- != 0);
                    p += (WORD)SW(a3, 6);
                } while (rows-- != 0);
                /* brightest */
                best = -1;
                for (i = 0; ; i++) {
                    if ((WORD)usevals[i] >= best) {
                        best = (WORD)usevals[i];
                        d6c = i;
                    }
                    if (i == numcols)
                        break;
                }
            }
            {
                /* plot the destination pixel in colour d6c */
                UBYTE *q = a1 + (WORD)(d2 >> 3);
                UWORD bit = ~d2 & 7, n = SW(a3, 4) - 1;

                do {
                    if (d6c & 1)
                        *q |= (UBYTE)(1 << bit);
                    d6c >>= 1;
                    q += (WORD)SW(a5, 22);
                } while (n-- != 0);
            }
            d0 = nx;
            d2++;
        } while (d2 < SW(a5, 0));
        d1 = ny;
        a1 += (WORD)SW(a5, 6);
        d3++;
    } while (d3 < SW(a5, 2));
    makeshape(a3);
}

/* Scale without a palette: every colour counts 1 */
static void BLITZ_ROUTINE doscale2_c(ULONG xr BL_REG(d1), ULONG yr BL_REG(d2))
{
    int i;

    numcols = 255;
    for (i = 0; i < 256; i++)
        colvals[i] = 1;
    scale(blitz_string, xr, yr);
}
BLITZ_THUNK(doscale2, doscale2_c, "d0-d1/a0-a1");

/* Scale with a palette (a2): brightness = r+g+b+3 of the top bytes */
static void BLITZ_ROUTINE doscale_c(ULONG xr BL_REG(d1), ULONG yr BL_REG(d2), UBYTE *pal BL_REG(a2))
{
    UWORD n = SW(pal, 4) - 1;
    const ULONG *c = (const ULONG *)SP(pal, 0) + 1;
    UWORD *a0 = colvals;

    numcols = n;
    do {
        UWORD v = (UWORD)((c[0] >> 24) + 1);

        v += (UWORD)((c[1] >> 24) + 1);
        v += (UWORD)((c[2] >> 24) + 1);
        c += 3;
        *a0++ = v;
    } while (n-- != 0);
    scale(blitz_string, xr, yr);
}
BLITZ_THUNK(doscale, doscale_c, "d0-d1/a0-a1");

/* ************************** ERROR CHECKING ************************** */

BLITZ_ERRCHK_MESSAGE(mess1, "Uninitialized Shape");
BLITZ_ERRCHK_MESSAGE(mess2, "Uninitialized Palette");

static void BLITZ_ERRCHK scalechk2_c(void)
{
    if (!*(UWORD *)blitz_string)
        BLITZ_RUNERR(mess1);
}

static void BLITZ_ERRCHK scalechk_c(ULONG *pal BL_REG(a2))
{
    if (!*pal)
        BLITZ_RUNERR(mess2);
    scalechk2_c();
}

/* the thunks after all check functions: their assembler switches back to
   .text, which GCC does not notice (a check function after a thunk would
   end up in the normal code) */
BLITZ_ERRCHK_THUNK(scalechk2, scalechk2_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(fchk, scalechk2_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(scalechk, scalechk_c, "d0-d1/a0-a1");
