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
 * rounding +$8000, fixed #1004) and copied when it falls inside the
 * source.  Scale: new size = size * ratio (lmullib $CA00, at least 1),
 * source steps 1/ratio (ldivlib $CB00); every destination pixel takes the colour with
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
 *    shift blit (width = byte width * 8). (#271)
 *  - Scale reads source pixels to the right/bottom of the shape when a
 *    target size below 1 is raised to 1 (reads only). (#273)
 *  - Word arithmetic: row/plane steps ADDA.w, sizes are MULU low words. (#274)
 * Kept from the original, not a bug: the brightness table keeps the values
 * of an earlier Scale with a palette for colours beyond the current
 * palette (never used: the colour choice only compares
 * usevals[0..numcols]). (#272)
 */
#include "blitzlib.h"
#include "blitzobj.h"
#define __NOLIBBASE__
#include <proto/graphics.h>
#undef __NOLIBBASE__

/* entry points (thunks / assembler, defined below) */
void doscale(void), doscale2(void), doxflip(void), doyflip(void);
void dorotate(void), scalechk(void), scalechk2(void), shapetrixlib_fchk(void);

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
BLITZ_SUBS(doxflip, shapetrixlib_fchk, 0);
BLITZ_NAME("XFlip", "Shape#");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_shapeslib, 0x1380);
BLITZ_SUBS(doyflip, shapetrixlib_fchk, 0);
BLITZ_NAME("YFlip", "Shape#");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_QUICK);
BLITZ_LIBS(LIB_shapeslib, 0x1380);
BLITZ_SUBS(dorotate, shapetrixlib_fchk, 0);
BLITZ_NAME("Rotate", "Shape#,Angle Ratio");

BLITZ_LIBFIN();

/* ------------------------------------------------------------- helpers */

/* the objects: struct bb_shape, struct bb_palette (blitzobj.h) */
/* the two handles as one long (CLR.l 10(a0)) */
#define HANDLES(s) (*(ULONG *)&(s)->handlex)

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
static void docookie(struct bb_shape *shape)
{
#ifndef BL_CODE_MODEL
    register struct bb_shape *_s __asm("a0") = shape;

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
static void cookieit(struct bb_shape *s)
{
    if (s->cookie)
        docookie(s);
}

/* --------------------------------------------------------- newshape */

static struct bb_shape newshape __attribute__((aligned(4)));
static UWORD colvals[256];                      /* table of brightness's */
static UWORD usevals[256];                      /* currently collected pixels */
static ULONG xadd, yadd;
static UWORD numcols;                           /* dbf */

/* allocshape: width, height, depth of newshape set; the rest and the
   memory */
static void allocshape(struct bb_shape *ns)
{
    UWORD words = (UWORD)(ns->width + 15) >> 4; /* word width */
    ULONG p;

    ns->bltsize = (UWORD)((UWORD)(ns->height << 6) | words) + 1; /* bltsize */
    ns->ebwidth = words + words;                  /* even byte width */
    p = (ULONG)(UWORD)(ns->ebwidth + 2) * ns->height;
    ns->xplanesize = (UWORD)p;
    p = (ULONG)(UWORD)p * ns->depth;
    ns->xsize = (UWORD)p;
    p = (ULONG)ns->ebwidth * ns->height;
    ns->planesize = (UWORD)p;
    p = (ULONG)(UWORD)p * ns->depth;
    ns->size = (UWORD)p;
    ns->data = globalloc(p, MEMF_CHIP | MEMF_CLEAR);
    HANDLES(ns) = 0;                            /* handle */
    ns->cookie = NULL;                          /* cookie */
}

/* makeshape: newshape into shape a3 */
static void makeshape(struct bb_shape *shape)
{
    ULONG cookie;
    int i;

    globfree(shape->data, shape->size);
    cookie = (ULONG)shape->cookie;
    if (cookie)
        globfree((void *)cookie, shape->planesize);
    for (i = 0; i < 8; i++)
        ((ULONG *)shape)[i] = ((ULONG *)&newshape)[i];
    if (cookie)
        docookie(shape);
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
static void xflip(struct bb_shape *s, volatile UBYTE *chip)
{
    UWORD pad = (UWORD)(s->ebwidth << 3) - s->width; /* d0: the pad bits */
    UWORD rows;
    UBYTE *row;

    if (pad) {
        /* scroll across to the right, 'hard left' justified after the flip */
        UWORD planes;
        UWORD shift;                            /* d0: the BLTCON0 shift */
        UWORD bltsize;                          /* d0: BLTSIZE - 1 */

        blitwait(chip);
        pad &= 15;
        shift = (UWORD)((pad >> 4) | (pad << 12)); /* ROR #4 */
        *(volatile UWORD *)(chip + 0x040) = shift | 0x9f0;
        *(volatile UWORD *)(chip + 0x042) = 0;
        *(volatile ULONG *)(chip + 0x044) = 0xffffffffUL;
        *(volatile ULONG *)(chip + 0x064) = 0;
        *(volatile ULONG *)(chip + 0x050) = (ULONG)s->data;
        *(volatile ULONG *)(chip + 0x054) = (ULONG)s->data;
        planes = s->depth - 1;
        bltsize = s->bltsize - 1;
        do {
            blitwait(chip);
            *(volatile UWORD *)(chip + 0x058) = bltsize;
        } while (planes-- != 0);
    }
    /* now, to do the flipping! */
    rows = (UWORD)((ULONG)s->depth * s->height) - 1;
    row = s->data;
    do {
        UWORD l = 0, r = s->ebwidth - 1;          /* left, right bytes */

        do {
            UBYTE lbits = flipbyte(row[(WORD)l]);
            UBYTE rbits = flipbyte(row[(WORD)r]);

            row[(WORD)r] = lbits;
            row[(WORD)l] = rbits;
            l++;
            r--;
        } while (r > l);
        row += (WORD)s->ebwidth;
        __asm__("" : "+a"(row));
    } while (rows-- != 0);
    cookieit(s);
}

/**
 * @brief XFlip Shape# - mirror a shape horizontally (Blitz mode).
 *
 * a3 = the shape.  First the planes are shifted right with the blitter by
 * ebw*8-width pixels (so the image is "hard left" after the flip; one
 * blit per plane, the same pointer as source and destination, modulo 0),
 * then every row's bytes are mirrored and bit-reversed by the CPU; a
 * cookie is made again if the shape had one (shapeslib +11).
 *
 * BUG(orig) (#271): no wait after the last blit before the CPU flips the
 * bytes; the shift uses the shape's bltsize-1 words per plane.  The
 * result depends on timing.  BUG(orig) (#274): row steps ADDA.w, the
 * row count a MULU low word.  Errors: "Uninitialized Shape" (fchk,
 * debugger only).
 *
 * @param chip a1: the chip base ($DFF000, chipbaselib)
 */
void BLITZ_ROUTINE b_doxflip_c(volatile UBYTE *chip BL_REG(a1));
void BLITZ_ROUTINE b_doxflip_c(volatile UBYTE *chip BL_REG(a1))
{
    xflip((struct bb_shape *)blitz_string, chip);
}
BLITZ_THUNK(b_doxflip, b_doxflip_c, "d0-d1/a0-a1");

/**
 * @brief XFlip Shape# in Amiga mode: OwnBlitter, flip, (no real wait:
 * BTST twice without a loop, #271), DisownBlitter.
 *
 * @param chip a1: the chip base ($DFF000)
 * @param gfx  a6: graphics.library
 */
void BLITZ_ROUTINE a_doxflip_c(volatile UBYTE *chip BL_REG(a1), struct GfxBase *gfx BL_REG(a6));
void BLITZ_ROUTINE a_doxflip_c(volatile UBYTE *chip BL_REG(a1), struct GfxBase *gfx BL_REG(a6))
{
    struct GfxBase *GfxBase = gfx;
    volatile UBYTE *dmaconr = (volatile UBYTE *)0xdff002;

    OwnBlitter();
    xflip((struct bb_shape *)blitz_string, chip);
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

/**
 * @brief YFlip Shape# - mirror a shape vertically (rows swapped plane by
 * plane); a3 = the shape.
 *
 * A cookie is made again if the shape had one (shapeslib +11).
 * BUG(orig) (#274): plane steps ADDA.w.  Errors: "Uninitialized Shape"
 * (fchk, debugger only).
 */
static void BLITZ_ROUTINE doyflip_c(void)
{
    struct bb_shape *s = (struct bb_shape *)blitz_string;
    UWORD planes = s->depth - 1;                /* bitplanes */
    UBYTE *plane = s->data;

    do {
        UBYTE *top = plane, *bot = plane + (WORD)s->planesize; /* top, bottom */

        for (;;) {
            WORD off;

            bot -= (WORD)s->ebwidth;
            if ((ULONG)bot <= (ULONG)top)
                break;
            off = (WORD)(s->ebwidth - 2);
            do {
                UWORD topword = *(UWORD *)(top + off);

                *(UWORD *)(top + off) = *(UWORD *)(bot + off);
                *(UWORD *)(bot + off) = topword;
                off -= 2;
            } while (off >= 0);
            top += (WORD)s->ebwidth;
        }
        plane += (WORD)s->planesize;
    } while (planes-- != 0);
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
    UWORD idx, sinv;
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
    sinv = qrots[idx];
    if (sinv == 0xffff)
        r = (ULONG)(UWORD)val << 16;            /* SWAP:CLR */
    else if (val < 0)
        r = -((ULONG)(UWORD)-val * sinv);       /* NEG:MULU:NEG.l */
    else
        r = (ULONG)(UWORD)val * sinv;
    return neg ? -r : r;
}

static ULONG mulcos(WORD val, UWORD angle)
{
    return mulsin(val, (UWORD)(angle + 64) & 255);
}

/* rotit: (x, y) by angle -> rounded new x, y.  Fixed (#1004): +$8000
   for both signs (the nearest integer, halves up); the original took
   -$8000 for a negative value, floor(v - 0.5) */
static void rotit(WORD x, WORD y, UWORD angle, WORD *nx, WORD *ny)
{
    ULONG rx = mulcos(x, angle) + mulsin(y, angle) + 0x8000UL;
    ULONG ry = mulcos(y, angle) - mulsin(x, angle) + 0x8000UL;

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

/**
 * @brief Rotate Shape#,Angle Ratio - rotate a shape.
 *
 * a3 = the shape.  angle = Quick 0..1 -> 256 steps; the new size comes
 * from the rotated corners; every destination pixel is rotated back
 * (rotit, sin table, rounding +$8000) and copied when it falls inside
 * the source.  The new shape (newshape, MEMF_CHIP|MEMF_CLEAR, not
 * checked, handle 0) replaces the old one; a cookie is made again if the
 * shape had one.
 *
 * BUG(orig) (#274): word arithmetic (ADDA.w steps, MULU low words).
 * Errors: "Uninitialized Shape" (fchk, debugger only).
 *
 * @param q d1: the angle as a Quick (0..1 = a full turn)
 */
static void BLITZ_ROUTINE dorotate_c(ULONG q BL_REG(d1))
{
    struct bb_shape *s = (struct bb_shape *)blitz_string, *ns = &newshape;
    UBYTE *row;
    UWORD angle = 256 - (UWORD)(((q << 8) >> 16) & 255);
    WORD y;

    xl = yl = xh = yh = 0;                      /* initial low/high vals */
    rotit2(s->width - 1, 0, angle);
    rotit2(s->width - 1, s->height - 1, angle);
    rotit2(0, s->height - 1, angle);
    ns->width = xh - xl + 1;
    ns->height = yh - yl + 1;
    ns->depth = s->depth;
    allocshape(ns);
    angle = 256 - angle;
    y = yl;
    row = ns->data;
    do {
        UBYTE *dst = row;
        WORD x = xl;
        WORD dbit = 7;

        do {
            WORD nx, ny;

            rotit(x, y, angle, &nx, &ny);
            if ((UWORD)nx < s->width && (UWORD)ny < s->height) {
                /* transfer pixel from src to dest */
                UBYTE *src = s->data + (WORD)((UWORD)nx >> 3)
                            + (ULONG)(UWORD)ny * s->ebwidth;
                UWORD bit = ~(UWORD)nx & 7;
                UWORD planes = s->depth - 1;
                UBYTE *dplane = dst;

                do {
                    if (*src & (1 << bit))
                        *dplane |= (UBYTE)(1 << dbit);
                    src += (WORD)s->planesize;
                    dplane += (WORD)ns->planesize;
                } while (planes-- != 0);
            }
            if (--dbit < 0) {                   /* across an X... */
                dbit = 7;
                dst++;
            }
            x++;
        } while (x <= xh);
        row += (WORD)ns->ebwidth;                 /* down a Y... */
        y++;
    } while (y <= yh);
    makeshape(s);
}
BLITZ_THUNK(dorotate, dorotate_c, "d0-d1/a0-a1");

/* -------------------------------------------------------------- Scale */

static void scale(struct bb_shape *shape, ULONG xr, ULONG yr)
{
    struct bb_shape *ns = &newshape;
    UBYTE *drow;
    ULONG sxq, syq, w;
    UWORD dx, dy;

    w = qcall(TOK_qmultiply, (ULONG)shape->width << 16, xr);
    if ((WORD)(w >> 16) < 1)
        w = 1UL << 16;
    ns->width = (UWORD)(w >> 16);
    w = qcall(TOK_qmultiply, (ULONG)shape->height << 16, yr);
    if ((WORD)(w >> 16) < 1)
        w = 1UL << 16;
    ns->height = (UWORD)(w >> 16);
    ns->depth = shape->depth;
    allocshape(ns);
    xadd = qcall(TOK_qdivide, 1UL << 16, xr);   /* 1/ratio */
    yadd = qcall(TOK_qdivide, 1UL << 16, yr);
    syq = 0;
    dy = 0;
    drow = ns->data;
    do {                                        /* yloop */
        ULONG ny = syq + yadd;
        WORD ycount = (WORD)((ny >> 16) - (syq >> 16)) - 1;
        UBYTE *srow;

        if (ycount < 0)
            ycount = 0;
        sxq = 0;
        dx = 0;
        srow = shape->data + (ULONG)(UWORD)(syq >> 16) * shape->ebwidth;
        do {                                    /* xloop */
            ULONG nx = sxq + xadd;
            WORD xcount = (WORD)((nx >> 16) - (sxq >> 16)) - 1;
            UWORD col = 0;

            if (xcount < 0)
                xcount = 0;
            {
                UWORD i, sx = (UWORD)(sxq >> 16);
                UBYTE *p = srow + (WORD)(sx >> 3);
                UWORD bit = ~sx & 7;
                WORD best;
                WORD rows = ycount;

                for (i = 0; ; i++) {            /* clear usevals */
                    usevals[i] = 0;
                    if (i == numcols)
                        break;
                }
                do {                            /* yl */
                    UBYTE *q = p;
                    UWORD b = bit;
                    WORD cols = xcount;

                    do {                        /* xl */
                        UWORD pl = 0;
                        ULONG c = 0;

                        do {
                            if (*q & (1 << (b & 7)))
                                c |= 1UL << (pl & 31);  /* BSET d2,d6 */
                            q += (WORD)shape->planesize;
                            __asm__("" : "+a"(q));      /* no __mulsi3 */
                            pl++;
                        } while (pl < shape->depth);
                        usevals[(UWORD)c] += colvals[(UWORD)c];
                        q -= (WORD)shape->size;
                        if ((WORD)--b < 0) {
                            b = 7;
                            q++;
                        }
                        __asm__("" : "+a"(q));
                    } while (cols-- != 0);
                    p += (WORD)shape->ebwidth;
                } while (rows-- != 0);
                /* brightest */
                best = -1;
                for (i = 0; ; i++) {
                    if ((WORD)usevals[i] >= best) {
                        best = (WORD)usevals[i];
                        col = i;
                    }
                    if (i == numcols)
                        break;
                }
            }
            {
                /* plot the destination pixel in colour d6c */
                UBYTE *q = drow + (WORD)(dx >> 3);
                UWORD bit = ~dx & 7, n = shape->depth - 1;

                do {
                    if (col & 1)
                        *q |= (UBYTE)(1 << bit);
                    col >>= 1;
                    q += (WORD)ns->planesize;
                } while (n-- != 0);
            }
            sxq = nx;
            dx++;
        } while (dx < ns->width);
        syq = ny;
        drow += (WORD)ns->ebwidth;
        dy++;
    } while (dy < ns->height);
    makeshape(shape);
}

/**
 * @brief Scale Shape#,X Ratio,Y Ratio - scale a shape.
 *
 * a3 = the shape.  New size = size * ratio (lmullib $CA00, at least 1),
 * source steps 1/ratio (ldivlib $CB00); every destination pixel takes the
 * colour with the highest sum of "brightness" over the source pixels it
 * covers - without a palette every colour counts 1, so the most frequent
 * colour.  The new shape replaces the old one as in Rotate.
 *
 * BUG(orig) (#273): reads source pixels to the right/bottom of the shape
 * when a target size below 1 is raised to 1 (reads only); (#274) word
 * arithmetic.  Errors: "Uninitialized Shape" (scalechk2, debugger only).
 *
 * @param xr d1: the x ratio (Quick)
 * @param yr d2: the y ratio (Quick)
 */
static void BLITZ_ROUTINE doscale2_c(ULONG xr BL_REG(d1), ULONG yr BL_REG(d2))
{
    int i;

    numcols = 255;
    for (i = 0; i < 256; i++)
        colvals[i] = 1;
    scale((struct bb_shape *)blitz_string, xr, yr);
}
BLITZ_THUNK(doscale2, doscale2_c, "d0-d1/a0-a1");

/**
 * @brief Scale Shape#,X Ratio,Y Ratio,Palette# - scale a shape, the colour
 * choice weighted by the palette.
 *
 * As doscale2_c, the brightness of a colour = r+g+b+3 of the top bytes of
 * the palette's entries.  Not a bug (#272): entries beyond the current
 * palette keep the values of an earlier Scale (never used: the choice
 * only compares usevals[0..numcols]).
 *
 * Errors (scalechk, debugger only): "Uninitialized Palette",
 * "Uninitialized Shape".
 *
 * @param xr  d1: the x ratio (Quick)
 * @param yr  d2: the y ratio (Quick)
 * @param pal a2: the palette object
 */
static void BLITZ_ROUTINE doscale_c(ULONG xr BL_REG(d1), ULONG yr BL_REG(d2),
                                    struct bb_palette *pal BL_REG(a2))
{
    UWORD n = pal->numcols - 1;
    const ULONG *c = (const ULONG *)pal->pdata + 1;
    UWORD *val = colvals;

    numcols = n;
    do {
        UWORD v = (UWORD)((c[0] >> 24) + 1);

        v += (UWORD)((c[1] >> 24) + 1);
        v += (UWORD)((c[2] >> 24) + 1);
        c += 3;
        *val++ = v;
    } while (n-- != 0);
    scale((struct bb_shape *)blitz_string, xr, yr);
}
BLITZ_THUNK(doscale, doscale_c, "d0-d1/a0-a1");

/* ************************** ERROR CHECKING ************************** */

BLITZ_ERRCHK_MESSAGE(mess1, "Uninitialized Shape");
BLITZ_ERRCHK_MESSAGE(mess2, "Uninitialized Palette");

static void BLITZ_ERRCHK scalechk2_c(void)
{
    if (!((const struct bb_shape *)blitz_string)->width)
        BLITZ_RUNERR(mess1);
}

static void BLITZ_ERRCHK scalechk_c(struct bb_palette *pal BL_REG(a2))
{
    if (!pal->pdata)
        BLITZ_RUNERR(mess2);
    scalechk2_c();
}

/* the thunks after all check functions: their assembler switches back to
   .text, which GCC does not notice (a check function after a thunk would
   end up in the normal code) */
BLITZ_ERRCHK_THUNK(scalechk2, scalechk2_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(shapetrixlib_fchk, scalechk2_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(scalechk, scalechk_c, "d0-d1/a0-a1");
