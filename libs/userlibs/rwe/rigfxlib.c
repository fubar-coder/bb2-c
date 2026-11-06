/*
 * rigfxlib - userlib 65 (RWE): palette and bitmap helpers.
 *   PaletteInfo palette#   PalRed/PalGreen/PalBlue(c)   AGAPalRed/...(c)
 *   PalAdjust palette#,ratio.q[,start,end]
 *   FillPalette / AGAFillPalette palette#,r,g,b[,start,end]
 *   CopyColour(srcpal#,destpal#,srccol#,destcol#)
 *   CPUCls bitmap#   SaveCMAP(palette#,filename)
 *   MergeBitplanes src1,src2,dest,planes
 *
 * C translation of rwelibsrc/GFXLib/GFX-Library2.bb (21/01/96); the
 * declarations equal BSS2/REDBLITZLIBS/LIBRARIES/USERLIBS/RIGFXLIB.OBJ
 * (ctest rwe_declarations_rigfxlib), the behaviour is compared with it in
 * the emulator (libs/tests/rwe/rigfxlib.c).  Notes:
 * docs/rwelibs/port/rigfxlib.md.
 *
 * Palette objects (palettelib, !libs #palettelib,#ia0|#pd0: a0 = the
 * object): +0.l the data, +4.w the number of colours; the data: +0.w the
 * number of colours, +2.w, then 12 bytes per colour (r.l g.l b.l, the
 * value in the top byte).  Bitmap objects (bitmaplib): a struct BitMap
 * (+0.w BytesPerRow, +2.w Rows, +4.w Flags<<8|Depth, +8 the planes), +52,
 * +56, +62.w "the planes are allocated here" (struct bb_palette and struct
 * bb_bitmap of blitzobj.h, paldata below).
 *
 * The routines are C behind thunks that keep d1/a0/a1 (the original
 * changes several of them).  The word arithmetic of the original
 * (MULU.w, ADDA.w with a sign-extended word, DBRA on a word, the tests
 * on words or the whole register) is reproduced, so that out-of-range
 * colour numbers and counts come out the same.  SaveCMAP calls memlib
 * and RIAmosFuncLib's BSave (form 1) through ALibJsr.
 *
 * BUG(orig), kept:
 *   - CPUCls on a bitmap without plane 0 pops a4-a6 it never pushed and
 *     returns to a foreign address (the BEQ 'oops before the MOVEM) - in
 *     assembler, see cpucls below (#680).
 *   - CPUCls clears with blocks of 40 bytes, DBRA on blocks - 1: a plane
 *     of less than 40 bytes clears 65536 blocks (2.6 MB below the plane)
 *     (#682); an odd rest byte stays (#681); the depth is the word
 *     Flags<<8|Depth, 0 planes = 65536 (#679).
 *   - SaveCMAP writes no pad byte behind a CMAP of odd length (#687); a
 *     palette of 0 colours copies 65536 (#688).
 *   - CopyColour compares the colour numbers signed: negative ones pass
 *     and index far behind the palette (MULU.w) (#678).
 *   - PalAdjust/PalRed... use the palette of the last PaletteInfo
 *     unchecked (before one: address 0) (#686); PalAdjust without
 *     start/end takes the number of colours of THAT palette, not of the
 *     target (#685); colour numbers from 2731 on index before the palette
 *     (12*c as a signed word) (#677); end < start in PalAdjust runs
 *     65536*3 channels (count 0 after MULU.w/SUBQ.w) (#684).
 *   - FillPalette tests the whole register of `end` for 0 (= the last
 *     colour), the word for the comparison with start (#683).
 * SaveCMAP's BMHD chunk is 20 zero bytes, like the original: no defect
 * (a palette file: what is read of it is the CMAP).
 */
#include "blitzlib.h"
#include "blitzobj.h"
#include <datatypes/pictureclass.h>
#include <stddef.h>

/* RIAmosFuncLib (75) BSave: index 15 of the token list of the CD's
   BSS2 RIAMOSFUNCLIB.OBJ; form 1 = (filename$,address/bank,length):
   d0 = name, d1 = address, d2 = length -> d0 */
#define TOK_bsave ((LIB_riamosfunclib << 7) + 16)

/* entry points (thunks, defined below) */
void palred(void), palgreen(void), palblue(void), paletteinfo(void);
void agapalred(void), agapalgreen(void), agapalblue(void);
void paladjust(void), paladjust2(void), fillpalette(void), fillpalette2(void);
void agafillpalette(void), agafillpalette2(void), copycolour(void);
void cpucls(void), savecmap(void), mergebitplanes(void);

#define IA(n) (BL_LA(n) | BL_ITEM | BL_PD(n))   /* #ia<n>|#pd<n> */

/* !libheader {#rigfxlib,0,0,0,0} */
BLITZ_LIBHEADER(LIB_rigfxlib, 0, BL_NORET, 0, 0);

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(palred, 0, 0);
BLITZ_NAME("PalRed", "(colour#)");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(palgreen, 0, 0);
BLITZ_NAME("PalGreen", "(colour#)");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(palblue, 0, 0);
BLITZ_NAME("PalBlue", "(colour#)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS(LIB_palettelib, IA(0));
BLITZ_SUBS(paletteinfo, 0, 0);
BLITZ_NAME("PaletteInfo", "palette#");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(agapalred, 0, 0);
BLITZ_NAME("AGAPalRed", "(colour#)");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(agapalgreen, 0, 0);
BLITZ_NAME("AGAPalGreen", "(colour#)");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(agapalblue, 0, 0);
BLITZ_NAME("AGAPalBlue", "(colour#)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_QUICK);
BLITZ_LIBS(LIB_palettelib, IA(0));
BLITZ_SUBS(paladjust, 0, 0);
BLITZ_ARGS(BL_WORD, BL_QUICK, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_palettelib, IA(0));
BLITZ_SUBS(paladjust2, 0, 0);
BLITZ_NAME("PalAdjust", "palette#,ratio.q[,start_col,end_col]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_palettelib, IA(0));
BLITZ_SUBS(fillpalette, 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_palettelib, IA(0));
BLITZ_SUBS(fillpalette2, 0, 0);
BLITZ_NAME("FillPalette", "palette#,r,g,b[,start_col,end_col] - fill palette with r,g,b values");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_palettelib, IA(0));
BLITZ_SUBS(agafillpalette, 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_palettelib, IA(0));
BLITZ_SUBS(agafillpalette2, 0, 0);
BLITZ_NAME("AGAFillPalette",
           "palette#,r,g,b[,start_col,end_col] - fill palette with AGA r,g,b values");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_palettelib, IA(0), LIB_palettelib, IA(1));
BLITZ_SUBS(copycolour, 0, 0);
BLITZ_NAME("CopyColour", "sourcepal#,destpal#,colsource#,coldest#");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS(LIB_bitmaplib, IA(0));
BLITZ_SUBS(cpucls, 0, 0);
BLITZ_NAME("CPUCls", "bitmap#");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS(LIB_palettelib, IA(0));
BLITZ_SUBS(BLITZ_AMIGAONLY(savecmap), 0, 0);
BLITZ_NAME("SaveCMAP", "palette#,filename$");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, IA(0), LIB_bitmaplib, IA(1), LIB_bitmaplib, IA(2));
BLITZ_SUBS(mergebitplanes, 0, 0);
BLITZ_NAME("MergeBitplanes", "srcbitmap,srcbitmap,destbitmap,numplanes");

BLITZ_LIBFIN();

/* ---------------------------------------------------------------- data */

/* the data of a palette object: 12 bytes per colour, each channel's value
   in the top byte of its long */
struct palcol {
    ULONG r, g, b;
};
struct paldata {
    UWORD numcols;                      /* +0 */
    UWORD w2;                           /* +2 */
    struct palcol col[];                /* +4 */
};
static_assert(sizeof(struct palcol) == 12 && offsetof(struct paldata, col) == 4);
/* the colours of a paldata as bytes (the original's +4, then its byte
   offset) and the paldata of a colour 0 (PaletteInfo's paladdr) */
#define COLBYTES(d) ((UBYTE *)(d) + offsetof(struct paldata, col))
#define PALOF(c) ((struct paldata *)((UBYTE *)(c) - offsetof(struct paldata, col)))

/* a palette object (palettelib, 2^7 bytes): struct bb_palette (blitzobj.h),
   its colour block a struct paldata */
#define PALDATA(o) ((struct paldata *)(o)->pdata)

/* a bitmap object (bitmaplib, 2^6 bytes): struct bb_bitmap (blitzobj.h); this
   library takes Flags/Depth as one word (BUG(orig) (#679)), the planes as
   longs, and MergeBitplanes copies two words at a time as a long (PAIR) */
#define PAIR(b, m) (*(ULONG *)&(b)->m)

/* the file SaveCMAP writes: FORM ILBM with an empty BMHD, then the CMAP
   (3 bytes per colour) */
struct cmapfile {
    ULONG form, formlen, ilbm;
    ULONG bmhd, bmhdlen;
    struct BitMapHeader bmhddata;       /* +20: left 0 */
    ULONG cmap, cmaplen;                /* +40 */
    UBYTE colours[];                    /* +48 */
};
static_assert(sizeof(struct cmapfile) == 48);

static UBYTE *paladdr;                  /* colour 0 of PaletteInfo's palette */

/* 12 * colour as the original indexes: MULU.w #12, then the low word as
   a signed index (0(a0,d0.w), ADDA.w) */
static LONG colindex(ULONG c)
{
    return (WORD)(UWORD)bl_mulu_w((UWORD)c, 12);
}

/* ------------------------------------------------------- colour values */

/**
 * @brief PaletteInfo palette#: select the palette PalRed ... and PalAdjust read.
 *
 * Remembers the address of its colour 0 (paladdr); the object's data
 * pointer is taken as it is, not checked.
 * @param obj  a0: the palette object (!libs #palettelib,#ia0|#pd0)
 */
static void BLITZ_ROUTINE paletteinfo_c(struct bb_palette *obj BL_REG(a0))
{
    paladdr = COLBYTES(PALDATA(obj));
}
BLITZ_THUNK(paletteinfo, paletteinfo_c, "d0-d1/a0-a1");

/**
 * @brief PalRed(colour#): the red value (4 bit) of a colour of PaletteInfo's palette.
 *
 * MOVE.b 0/4/8(a0,d0.w),d0: ASR.l #4:ANDI.l #15 (4 bit) or ANDI.l #$ff
 * (the AGA functions).  BUG(orig): the palette of the last PaletteInfo
 * is used unchecked (before one: address 0) (#686); colour numbers from
 * 2731 on index before the palette (12 * c as a signed word) (#677).
 * @param c  d0: colour# (its low word)
 * @return d0: the top 4 bits of the colour's red byte
 */
static ULONG BLITZ_ROUTINE palred_c(ULONG c BL_REG(d0))
{
    return paladdr[colindex(c)] >> 4;
}
BLITZ_THUNK(palred, palred_c, "d1/a0-a1");

/**
 * @brief PalGreen(colour#): the green value (4 bit); as PalRed (#686, #677).
 * @param c  d0: colour#
 * @return d0: the top 4 bits of the colour's green byte
 */
static ULONG BLITZ_ROUTINE palgreen_c(ULONG c BL_REG(d0))
{
    return paladdr[colindex(c) + 4] >> 4;
}
BLITZ_THUNK(palgreen, palgreen_c, "d1/a0-a1");

/**
 * @brief PalBlue(colour#): the blue value (4 bit); as PalRed (#686, #677).
 * @param c  d0: colour#
 * @return d0: the top 4 bits of the colour's blue byte
 */
static ULONG BLITZ_ROUTINE palblue_c(ULONG c BL_REG(d0))
{
    return paladdr[colindex(c) + 8] >> 4;
}
BLITZ_THUNK(palblue, palblue_c, "d1/a0-a1");

/**
 * @brief AGAPalRed(colour#): the red value (8 bit); as PalRed (#686, #677).
 * @param c  d0: colour#
 * @return d0: the colour's red byte
 */
static ULONG BLITZ_ROUTINE agapalred_c(ULONG c BL_REG(d0))
{
    return paladdr[colindex(c)];
}
BLITZ_THUNK(agapalred, agapalred_c, "d1/a0-a1");

/**
 * @brief AGAPalGreen(colour#): the green value (8 bit); as PalRed (#686, #677).
 * @param c  d0: colour#
 * @return d0: the colour's green byte
 */
static ULONG BLITZ_ROUTINE agapalgreen_c(ULONG c BL_REG(d0))
{
    return paladdr[colindex(c) + 4];
}
BLITZ_THUNK(agapalgreen, agapalgreen_c, "d1/a0-a1");

/**
 * @brief AGAPalBlue(colour#): the blue value (8 bit); as PalRed (#686, #677).
 * @param c  d0: colour#
 * @return d0: the colour's blue byte
 */
static ULONG BLITZ_ROUTINE agapalblue_c(ULONG c BL_REG(d0))
{
    return paladdr[colindex(c) + 8];
}
BLITZ_THUNK(agapalblue, agapalblue_c, "d1/a0-a1");

/* ------------------------------------------------------------ PalAdjust */

/**
 * @brief PalAdjust palette#,ratio.q,start_col,end_col: scale colours of PaletteInfo's palette into a palette.
 *
 * Target colours start..end = PaletteInfo's colours * ratio (8.8 fixed
 * point from the Quick 16.16), saturated at 255; only the top byte of
 * each channel is written.  BUG(orig): PaletteInfo's palette unchecked
 * (#686); colour numbers from 2731 on index before the palettes (#677);
 * end < start runs 65536*3 channels (count 0 after MULU.w/SUBQ.w)
 * (#684).
 * @param obj    a0: the target palette object (!libs #palettelib,#ia0|#pd0)
 * @param ratio  d1: the ratio, a Quick (16.16)
 * @param start  d2: the first colour
 * @param end    d3: the last colour
 */
static void BLITZ_ROUTINE paladjust2_c(struct bb_palette *obj BL_REG(a0), ULONG ratio BL_REG(d1),
                                        ULONG start BL_REG(d2), ULONG end BL_REG(d3))
{
    UWORD r = (UWORD)(((UWORD)(ratio >> 16) << 8) + ((UWORD)ratio >> 8));
    LONG off = colindex(start);         /* ADD.w d5,a0 / a1 */
    const UBYTE *s = paladdr + off;
    UBYTE *d = COLBYTES(PALDATA(obj)) + off;
    UWORD count = (UWORD)((UWORD)end - (UWORD)start + 1);
    UWORD n = (UWORD)(bl_mulu_w(count, 3) - 1);    /* DBRA */

    do {
        ULONG p = bl_mulu_w(*s, r);
        *d = p > 0xffff ? 0xff : (UBYTE)(p >> 8);
        s += 4;
        d += 4;
    } while (n-- != 0);
}
BLITZ_THUNK(paladjust2, paladjust2_c, "d0-d1/a0-a1");

/**
 * @brief PalAdjust palette#,ratio.q: c_paladjust2() for the colours 0 .. n-1.
 *
 * n is the number of colours of PaletteInfo's palette, not of the target
 * (BUG(orig) (#685)); the upper word of `end` is the caller's d3
 * (SUB.w), unused.
 * @param obj    a0: the target palette object
 * @param ratio  d1: the ratio, a Quick (16.16)
 * @param end    d3: the caller's d3 (its low word is replaced)
 */
static void BLITZ_ROUTINE paladjust_c(struct bb_palette *obj BL_REG(a0), ULONG ratio BL_REG(d1),
                                       ULONG end BL_REG(d3))
{
    BL_SETW(end, PALOF(paladdr)->numcols - 1);
    paladjust2_c(obj, ratio, 0, end);
}
BLITZ_THUNK(paladjust, paladjust_c, "d0-d1/a0-a1");

/* ---------------------------------------------------------- FillPalette */

/* r, g, b = the top bytes (AGA: the low bytes of the arguments);
   end = 0 (the whole register): the last colour */
static void fill(struct bb_palette *obj, ULONG r, ULONG g, ULONG b, ULONG start, ULONG end)
{
    struct paldata *data = PALDATA(obj);
    struct palcol *p;
    UWORD n;

    r = (r & 0xff) << 24;               /* ANDI.l #$ff:ROR.l #8 */
    g = (g & 0xff) << 24;
    b = (b & 0xff) << 24;
    if (!end)                           /* TST.l d5 */
        BL_SETW(end, data->numcols - 1);
    n = (UWORD)((UWORD)end - (UWORD)start);
    if ((WORD)n < 0)                    /* CMP.w d4,d5:BPL */
        return;
    p = (struct palcol *)(COLBYTES(data) + bl_mulu_w((UWORD)start, 12));
    do {
        p->r = r;
        p->g = g;
        p->b = b;
        p++;
    } while (n-- != 0);
}

/* 4 bit values: ANDI.b #$f:ASL.b #4 */
static ULONG nib(ULONG v)
{
    return (v & 0xf) << 4;
}

/**
 * @brief AGAFillPalette palette#,r,g,b,start_col,end_col: fill colours with an 8 bit r,g,b.
 *
 * The low bytes of r, g, b become the top bytes of the channels.  end =
 * 0 means the last colour; BUG(orig) (#683): that test is on the whole
 * register, the comparison with start on the words.  Nothing if end <
 * start.
 * @param obj    a0: the palette object (!libs #palettelib,#ia0|#pd0)
 * @param r      d1: red
 * @param g      d2: green
 * @param b      d3: blue
 * @param start  d4: the first colour
 * @param end    d5: the last colour (0: the palette's last)
 */
static void BLITZ_ROUTINE agafillpalette2_c(struct bb_palette *obj BL_REG(a0), ULONG r BL_REG(d1),
    ULONG g BL_REG(d2), ULONG b BL_REG(d3), ULONG start BL_REG(d4), ULONG end BL_REG(d5))
{
    fill(obj, r, g, b, start, end);
}
BLITZ_THUNK(agafillpalette2, agafillpalette2_c, "d0-d1/a0-a1");

/**
 * @brief AGAFillPalette palette#,r,g,b: fill the whole palette with an 8 bit r,g,b.
 * @param obj  a0: the palette object
 * @param r    d1: red
 * @param g    d2: green
 * @param b    d3: blue
 */
static void BLITZ_ROUTINE agafillpalette_c(struct bb_palette *obj BL_REG(a0), ULONG r BL_REG(d1),
    ULONG g BL_REG(d2), ULONG b BL_REG(d3))
{
    fill(obj, r, g, b, 0, 0);
}
BLITZ_THUNK(agafillpalette, agafillpalette_c, "d0-d1/a0-a1");

/**
 * @brief FillPalette palette#,r,g,b,start_col,end_col: fill colours with a 4 bit r,g,b.
 *
 * As AGAFillPalette with the low nibbles of r, g, b as the top nibbles
 * of the channel bytes (ANDI.b #$f:ASL.b #4) (#683).
 * @param obj    a0: the palette object (!libs #palettelib,#ia0|#pd0)
 * @param r      d1: red
 * @param g      d2: green
 * @param b      d3: blue
 * @param start  d4: the first colour
 * @param end    d5: the last colour (0: the palette's last)
 */
static void BLITZ_ROUTINE fillpalette2_c(struct bb_palette *obj BL_REG(a0), ULONG r BL_REG(d1),
    ULONG g BL_REG(d2), ULONG b BL_REG(d3), ULONG start BL_REG(d4), ULONG end BL_REG(d5))
{
    fill(obj, nib(r), nib(g), nib(b), start, end);
}
BLITZ_THUNK(fillpalette2, fillpalette2_c, "d0-d1/a0-a1");

/**
 * @brief FillPalette palette#,r,g,b: fill the whole palette with a 4 bit r,g,b.
 * @param obj  a0: the palette object
 * @param r    d1: red
 * @param g    d2: green
 * @param b    d3: blue
 */
static void BLITZ_ROUTINE fillpalette_c(struct bb_palette *obj BL_REG(a0), ULONG r BL_REG(d1),
    ULONG g BL_REG(d2), ULONG b BL_REG(d3))
{
    fill(obj, nib(r), nib(g), nib(b), 0, 0);
}
BLITZ_THUNK(fillpalette, fillpalette_c, "d0-d1/a0-a1");

/* ----------------------------------------------------------- CopyColour */

/**
 * @brief CopyColour(sourcepal#,destpal#,colsource#,coldest#): copy one colour between palettes.
 *
 * The three channel longs of colsource# of the source palette go to
 * coldest# of the target.  BUG(orig) (#678): the colour numbers are
 * compared with the palettes' number of colours as signed words:
 * negative ones pass and index far behind the palette (MULU.w).
 * @param src  a0: the source palette object (#ia0)
 * @param dst  a1: the target palette object (#ia1)
 * @param cs   d2: the source colour
 * @param cd   d3: the target colour
 * @return d0: BB_TRUE, BB_FALSE if a colour number is not below its
 *         palette's number of colours
 */
static BB_BOOL BLITZ_ROUTINE copycolour_c(struct bb_palette *src BL_REG(a0),
                                          struct bb_palette *dst BL_REG(a1),
                                          ULONG cs BL_REG(d2), ULONG cd BL_REG(d3))
{
    struct palcol *s, *d;

    if ((WORD)cs >= (WORD)src->numcols)
        return BB_FALSE;
    if ((WORD)cd >= (WORD)dst->numcols)
        return BB_FALSE;
    s = (struct palcol *)(COLBYTES(PALDATA(src)) + bl_mulu_w((UWORD)cs, 12));
    d = (struct palcol *)(COLBYTES(PALDATA(dst)) + bl_mulu_w((UWORD)cd, 12));
    d->r = s->r;
    d->g = s->g;
    d->b = s->b;
    return BB_TRUE;
}
BLITZ_THUNK(copycolour, copycolour_c, "d1/a0-a1");

/* --------------------------------------------------------------- CPUCls */

/* ClearPlane: clear below `end`: (UWORD)moves + 1 blocks of 40 bytes,
   then (UWORD)(moves >> 16) + 1 words unless that is $FFFF */
static void clearplane(UBYTE *end, ULONG moves)
{
    ULONG *l = (ULONG *)end;
    UWORD *w;
    UWORD n = (UWORD)moves;

    do {
        l -= 10;                        /* MOVEM.l d0-d6/a1-a3,-(a0) */
        l[0] = l[1] = l[2] = l[3] = l[4] = l[5] = l[6] = l[7] = l[8] = l[9] = 0;
    } while (n-- != 0);
    n = (UWORD)(moves >> 16);
    if (n == 0xffff)
        return;
    w = (UWORD *)l;
    do
        *--w = 0;
    while (n-- != 0);
}

/**
 * @brief CPUCls bitmap#: clear every plane of a bitmap with the CPU (the work behind the entry cpucls).
 *
 * BytesPerRow*Rows bytes per plane, cleared from the end down; the
 * original's register arithmetic for the block count (DIVU #40, the rest
 * as words).  BUG(orig): blocks of 40 bytes, DBRA on blocks - 1: a plane
 * of less than 40 bytes clears 65536 blocks (2.6 MB below the plane)
 * (#682); an odd rest byte stays (#681); the depth is the word
 * Flags<<8|Depth, 0 planes = 65536 (#679).
 * @param bm  a0: the bitmap object (!libs #bitmaplib,#ia0|#pd0)
 */
static void BLITZ_ROUTINE cpucls_c(struct bb_bitmap *bm BL_REG(a0))
{
    ULONG size = bl_mulu_w(bm->bpr, bm->rows);
    ULONG moves = bl_divu_w(size, 40);
    UWORD planes = (UWORD)(bm->flagsdepth - 1);    /* BUG(orig) (#679) */
    const ULONG *pl = (const ULONG *)bm->planes;

    BL_SETW(moves, moves - 1);          /* blocks - 1 */
    moves = BL_SWAP(moves);
    BL_SETW(moves, (WORD)moves >> 1);   /* rest / 2 (ASR.w) */
    BL_SETW(moves, moves - 1);
    moves = BL_SWAP(moves);
    do
        clearplane((UBYTE *)(*pl++ + size), moves);
    while (planes-- != 0);
}

/**
 * @brief CPUCls bitmap#: the entry; nothing (but a crash) without plane 0.
 *
 * TST.l 8(a0):BEQ 'oops - BUG(orig) (#680): 'oops is the MOVEM.l
 * (a7)+,a4-a6:RTS meant for after MOVEM.l a4-a6,-(a7), which has not
 * run yet: it pops the return address and two more longs into a4-a6 and
 * returns to whatever follows on the stack.  A stack imbalance C cannot
 * express, so this entry is assembler; the real work is cpucls_c(),
 * called with d0/d1/a0/a1 kept.
 * a0: the bitmap object.
 */
BLITZ_ASM_ROUTINE(cpucls,
    "   tst.l   8(a0)\n"
    "   beq.s   1f\n"
    "   movem.l d0-d1/a0-a1,-(sp)\n"
    "   jsr     _cpucls_c\n"
    "   movem.l (sp)+,d0-d1/a0-a1\n"
    "   rts\n"
    "1: movem.l (sp)+,a4-a6\n"
    "   rts");

/* ------------------------------------------------------------- SaveCMAP */

/**
 * @brief SaveCMAP(palette#,filename$): save a palette as an IFF ILBM with a CMAP.
 *
 * Builds FORM ILBM with an empty BMHD (20 zero bytes: no defect, a
 * palette file) and the CMAP (3 bytes per colour, the channels' top
 * bytes) in memory (memlib globalloc, MEMF_CLEAR), writes it with
 * RIAmosFuncLib's BSave (form 1: d0 name, d1 address, d2 length) and
 * frees it.  BUG(orig): no pad byte behind a CMAP of odd length (#687);
 * a palette of 0 colours copies 65536 (#688).
 * @param obj   a0: the palette object (!libs #palettelib,#ia0|#pd0)
 * @param name  d1: the file name
 * @return d0: BSave's result, 0 if the palette has no data or there is
 *         no memory
 */
static ULONG BLITZ_ROUTINE savecmap_c(struct bb_palette *obj BL_REG(a0), ULONG name BL_REG(d1))
{
    struct bl_regs r;
    const struct paldata *data = PALDATA(obj);
    const UBYTE *s;
    ULONG colsize, size, res;
    struct cmapfile *h;
    UBYTE *p;
    UWORD n;

    if (!data)
        return 0;
    colsize = bl_mulu_w(data->numcols, 3);
    size = colsize + 8 + 12 + 28;
    r = (struct bl_regs){ .d0 = size, .d1 = MEMF_CLEAR };
    h = (struct cmapfile *)BLITZ_ALIBJSR(TOK_globalloc, r);
    if (!h)
        return 0;
    h->form = ID_FORM;
    h->formlen = size - 8;
    h->ilbm = ID_ILBM;
    h->bmhd = ID_BMHD;
    h->bmhdlen = sizeof(struct BitMapHeader);   /* 20 bytes, left 0 */
    h->cmap = ID_CMAP;
    h->cmaplen = colsize;
    p = h->colours;
    s = COLBYTES(data);                 /* the top bytes of r, g, b */
    n = (UWORD)(data->numcols - 1);     /* DBRA */
    do {
        p[0] = s[0];
        p[1] = s[4];
        p[2] = s[8];
        p += 3;
        s += 12;
    } while (n-- != 0);
    res = BL_JSR_D4_(BL_TOKVAL(TOK_bsave, 1), name, (ULONG)h, size, 0);
    r = (struct bl_regs){ .d0 = size, .a1 = (ULONG)h };
    BLITZ_ALIBJSR(TOK_globfree, r);
    return res;
}
BLITZ_THUNK(savecmap, savecmap_c, "d1/a0-a1");

/* ------------------------------------------------------- MergeBitplanes */

/**
 * @brief MergeBitplanes srcbitmap,srcbitmap,destbitmap,numplanes: a bitmap of the planes of two others.
 *
 * The plane pointers of src1, then of src2, go into dest (numplanes of
 * them; 0 = all of both, DBRA from -1); dest gets src1's BytesPerRow,
 * Rows, width and height, numplanes as its Flags<<8|Depth word, +52 = 0
 * and +62 = 1.  Nothing if a source has word 62 = 0, dest not, or
 * numplanes > the sum of the depths (signed words).
 * @param src1    a0: the first source bitmap object (#ia0)
 * @param src2    a1: the second source bitmap object (#ia1)
 * @param dst     a2: the target bitmap object (#ia2)
 * @param planes  d3: numplanes
 */
static void BLITZ_ROUTINE mergebitplanes_c(struct bb_bitmap *src1 BL_REG(a0),
                                            struct bb_bitmap *src2 BL_REG(a1),
                                            struct bb_bitmap *dst BL_REG(a2), ULONG planes BL_REG(d3))
{
    UWORD left1, left2, n;
    const ULONG *planes1, *planes2;
    ULONG *pd;

    if (!src1->isreal || !src2->isreal || dst->isreal)
        return;
    if ((WORD)planes > (WORD)(UWORD)(src1->flagsdepth + src2->flagsdepth))
        return;
    dst->flagsdepth = (UWORD)planes;
    PAIR(dst, bpr) = PAIR(src1, bpr);
    PAIR(dst, ilword) = 0;
    PAIR(dst, width) = PAIR(src1, width);
    dst->isreal = 1;
    n = (UWORD)(planes - 1);
    left1 = src1->flagsdepth;
    left2 = src2->flagsdepth;
    planes1 = (ULONG *)src1->planes;
    planes2 = (ULONG *)src2->planes;
    pd = (ULONG *)dst->planes;
    while ((WORD)--left1 >= 0) {        /* SUBQ.w #1:BMI */
        *pd++ = *planes1++;
        if (n-- == 0)                   /* DBRA */
            return;
    }
    while ((WORD)--left2 >= 0) {
        *pd++ = *planes2++;
        if (n-- == 0)
            return;
    }
}
BLITZ_THUNK(mergebitplanes, mergebitplanes_c, "d0-d1/a0-a1");
