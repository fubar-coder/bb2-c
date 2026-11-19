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
 *   - PalAdjust without start/end takes the number of colours of
 *     PaletteInfo's palette, not of the target (#685); colour numbers
 *     from 2731 on index before the palette (12*c as a signed word)
 *     (#677).
 * SaveCMAP's BMHD chunk is 20 zero bytes, like the original: no defect
 * (a palette file: what is read of it is the CMAP).
 * Fixed (intended deviations, docs/libs/deviations.md):
 *   - CPUCls on a bitmap without plane 0 returns without doing anything;
 *     the original's BEQ 'oops went to the MOVEM.l (a7)+,a4-a6:RTS of the
 *     routine's end before the MOVEM.l a4-a6,-(a7) had run: it popped the
 *     return address and two more longs into a4-a6 and returned to a
 *     foreign address (#680).
 *   - CPUCls clears with blocks of 40 bytes, DBRA on blocks - 1; a plane
 *     of less than 40 bytes has no block (the original's counter was
 *     $FFFF: 65536 blocks, 2.6 MB below the plane) (#682).
 *   - CPUCls clears the last byte of an odd rest, the plane's first byte;
 *     the original cleared the rest as words (ASR.w #1) and left it
 *     (#681).
 *   - CPUCls reads the depth as the byte Depth; the original read the
 *     word Flags<<8|Depth, so a Flags byte other than 0 gave hundreds of
 *     planes.  With runtime error checking a depth of 0 is refused:
 *     "Illegal BitMap Depth" (cpuclschk; the original has no error check
 *     area); the release code clears 65536 planes then, as the original
 *     (an invalid argument, D-005) (#679).
 *   - CopyColour compares the colour numbers unsigned (CMP.w : BCC), so
 *     negative ones fail (0), as the manual says ("The command fails if
 *     either of the colour numbers is out of the range of the relevant
 *     palette"); the original compared them signed (BGE): they passed and
 *     indexed up to 786 KB behind the palette (MULU.w) (#678).
 *   - With runtime error checking PalAdjust's long form refuses end <
 *     start (signed words, as FillPalette's own test): "PalAdjust end
 *     colour before the start colour" (paladj2chk).  The release code is
 *     unchanged: the count end - start + 1 is a word, * 3 (MULU.w) - 1
 *     the DBRA counter, so end = start - 1 writes 65536 channels (an
 *     invalid argument, D-005) (#684).
 *   - FillPalette/AGAFillPalette test the word of `end` for 0 (= the last
 *     colour), as the comparison with start; the original tested the
 *     whole register (TST.l), so an end of 0 with an upper word other than
 *     0 was colour 0 (#683).
 *   - SaveCMAP writes the pad byte behind a CMAP of odd length (an odd
 *     number of colours): the file and the FORM length are rounded up to
 *     even, the CMAP length is not; the original wrote no pad byte, the
 *     file was no correct IFF file (#687).
 *   - SaveCMAP of a palette of 0 colours writes a CMAP of length 0 (a
 *     file of 48 bytes); the original copied with DBRA from numcols - 1,
 *     65536 colours, 196608 bytes behind its 48-byte block (#688).
 *   - init (the original has none) clears the PaletteInfo palette: a
 *     second run from the editor with the library still loaded started
 *     with the run before's palette, which PalRed ... and PalAdjust read
 *     before a PaletteInfo (#1125).
 *   - With runtime error checking PalRed ... AGAPalBlue and both forms
 *     of PalAdjust refuse to run before a PaletteInfo: "No palette
 *     selected with PaletteInfo" (colchk, paladjchk, paladj2chk).  The
 *     release code is unchanged: before the first PaletteInfo they read
 *     from address 0 (a programming error, as #176) (#686).
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
void cpucls(void), savecmap(void), mergebitplanes(void), cpuclschk(void), paladj2chk(void);
void colchk(void), paladjchk(void);
static void codeinit(void);

BLITZ_ENTRY_DECL(init);

/* !libheader {#rigfxlib,0,0,0,0}; init: fixed, #1125 (the original has
   none); error field: the error check area (fixed, #679, #684, #686; the
   original has none) */
BLITZ_LIBHEADER(LIB_rigfxlib, init, BL_NORET, 0, BLITZ_ERROR_AREA);

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(palred, colchk, 0);                       /* check: fixed #686 */
BLITZ_NAME("PalRed", "(colour#)");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(palgreen, colchk, 0);                     /* check: fixed #686 */
BLITZ_NAME("PalGreen", "(colour#)");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(palblue, colchk, 0);                      /* check: fixed #686 */
BLITZ_NAME("PalBlue", "(colour#)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS(LIB_palettelib, BL_IA0|BL_PD0);
BLITZ_SUBS(paletteinfo, 0, 0);
BLITZ_NAME("PaletteInfo", "palette#");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(agapalred, colchk, 0);                    /* check: fixed #686 */
BLITZ_NAME("AGAPalRed", "(colour#)");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(agapalgreen, colchk, 0);                  /* check: fixed #686 */
BLITZ_NAME("AGAPalGreen", "(colour#)");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(agapalblue, colchk, 0);                   /* check: fixed #686 */
BLITZ_NAME("AGAPalBlue", "(colour#)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_QUICK);
BLITZ_LIBS(LIB_palettelib, BL_IA0|BL_PD0);
BLITZ_SUBS(paladjust, paladjchk, 0);                 /* check: fixed #686 */
BLITZ_ARGS(BL_WORD, BL_QUICK, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_palettelib, BL_IA0|BL_PD0);
BLITZ_SUBS(paladjust2, paladj2chk, 0);               /* check: fixed #684, #686 */
BLITZ_NAME("PalAdjust", "palette#,ratio.q[,start_col,end_col]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_palettelib, BL_IA0|BL_PD0);
BLITZ_SUBS(fillpalette, 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_palettelib, BL_IA0|BL_PD0);
BLITZ_SUBS(fillpalette2, 0, 0);
BLITZ_NAME("FillPalette", "palette#,r,g,b[,start_col,end_col] - fill palette with r,g,b values");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_palettelib, BL_IA0|BL_PD0);
BLITZ_SUBS(agafillpalette, 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_palettelib, BL_IA0|BL_PD0);
BLITZ_SUBS(agafillpalette2, 0, 0);
BLITZ_NAME("AGAFillPalette",
           "palette#,r,g,b[,start_col,end_col] - fill palette with AGA r,g,b values");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_palettelib, BL_IA0|BL_PD0, LIB_palettelib, BL_IA1|BL_PD1);
BLITZ_SUBS(copycolour, 0, 0);
BLITZ_NAME("CopyColour", "sourcepal#,destpal#,colsource#,coldest#");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS(LIB_bitmaplib, BL_IA0|BL_PD0);
BLITZ_SUBS(cpucls, cpuclschk, 0);                    /* check: fixed #679 */
BLITZ_NAME("CPUCls", "bitmap#");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS(LIB_palettelib, BL_IA0|BL_PD0);
BLITZ_SUBS(BLITZ_AMIGAONLY(savecmap), 0, 0);
BLITZ_NAME("SaveCMAP", "palette#,filename$");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, BL_IA0|BL_PD0, LIB_bitmaplib, BL_IA1|BL_PD1, LIB_bitmaplib, BL_IA2|BL_PD2);
BLITZ_SUBS(mergebitplanes, 0, 0);
BLITZ_NAME("MergeBitplanes", "srcbitmap,srcbitmap,destbitmap,numplanes");

BLITZ_NULLSUB_AT(init, codeinit, 0, 0);                  /* fixed #1125 */

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
   library takes the planes as longs; CPUCls reads the byte Depth (fixed
   #679), MergeBitplanes the word Flags<<8|Depth (it writes numplanes there,
   Flags 0) and copies two words at a time as a long (PAIR) */
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

/**
 * @brief init: no PaletteInfo palette.
 *
 * Fixed (#1125): the original has no init, so a second run of a program
 * with the library still loaded (from the editor) started with the
 * PaletteInfo palette of the run before: PalRed ... AGAPalBlue and
 * PalAdjust before a PaletteInfo read the first run's freed palette.
 * Clears with CLR.l on an absolute address: no register changes.
 */
static void BLITZ_ROUTINE codeinit(void)
{
    paladdr = NULL;
}

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
 * (the AGA functions).  The palette of the last PaletteInfo is used
 * unchecked (before one: address 0), refused by colchk with runtime
 * error checking (fixed #686).  BUG(orig): colour numbers from 2731 on
 * index before the palette (12 * c as a signed word) (#677).
 * @param c  d0: colour# (its low word)
 * @return d0: the top 4 bits of the colour's red byte
 */
static ULONG BLITZ_ROUTINE palred_c(ULONG c BL_REG(d0))
{
    return paladdr[colindex(c)] >> 4;
}
BLITZ_THUNK(palred, palred_c, "d1/a0-a1");

/**
 * @brief PalGreen(colour#): the green value (4 bit); as PalRed (fixed #686; #677).
 * @param c  d0: colour#
 * @return d0: the top 4 bits of the colour's green byte
 */
static ULONG BLITZ_ROUTINE palgreen_c(ULONG c BL_REG(d0))
{
    return paladdr[colindex(c) + 4] >> 4;
}
BLITZ_THUNK(palgreen, palgreen_c, "d1/a0-a1");

/**
 * @brief PalBlue(colour#): the blue value (4 bit); as PalRed (fixed #686; #677).
 * @param c  d0: colour#
 * @return d0: the top 4 bits of the colour's blue byte
 */
static ULONG BLITZ_ROUTINE palblue_c(ULONG c BL_REG(d0))
{
    return paladdr[colindex(c) + 8] >> 4;
}
BLITZ_THUNK(palblue, palblue_c, "d1/a0-a1");

/**
 * @brief AGAPalRed(colour#): the red value (8 bit); as PalRed (fixed #686; #677).
 * @param c  d0: colour#
 * @return d0: the colour's red byte
 */
static ULONG BLITZ_ROUTINE agapalred_c(ULONG c BL_REG(d0))
{
    return paladdr[colindex(c)];
}
BLITZ_THUNK(agapalred, agapalred_c, "d1/a0-a1");

/**
 * @brief AGAPalGreen(colour#): the green value (8 bit); as PalRed (fixed #686; #677).
 * @param c  d0: colour#
 * @return d0: the colour's green byte
 */
static ULONG BLITZ_ROUTINE agapalgreen_c(ULONG c BL_REG(d0))
{
    return paladdr[colindex(c) + 4];
}
BLITZ_THUNK(agapalgreen, agapalgreen_c, "d1/a0-a1");

/**
 * @brief AGAPalBlue(colour#): the blue value (8 bit); as PalRed (fixed #686; #677).
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
 * each channel is written.  PaletteInfo's palette is used unchecked
 * (before one: address 0) and end < start runs 65536*3 channels (count 0
 * after MULU.w/SUBQ.w): refused by paladj2chk with runtime error checking
 * (fixed #686, #684), here unchecked.  BUG(orig): colour numbers from
 * 2731 on index before the palettes (#677).
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
   end = 0 (its word: fixed #683, the original tested the whole register):
   the last colour */
static void fill(struct bb_palette *obj, ULONG r, ULONG g, ULONG b, ULONG start, ULONG end)
{
    struct paldata *data = PALDATA(obj);
    struct palcol *p;
    UWORD n;

    r = (r & 0xff) << 24;               /* ANDI.l #$ff:ROR.l #8 */
    g = (g & 0xff) << 24;
    b = (b & 0xff) << 24;
    if (!(UWORD)end)                    /* TST.w d5 - fixed #683 */
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
 * 0 means the last colour; that test is on the word, as the comparison
 * with start (fixed #683: the original tested the whole register, so an
 * end whose upper word was not 0 was colour 0).  Nothing if end < start.
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
 * of the channel bytes (ANDI.b #$f:ASL.b #4) (the end test fixed, #683).
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
 * coldest# of the target.  The colour numbers are compared with the
 * palettes' number of colours as unsigned words (fixed #678: the
 * original compared them signed, so negative ones passed and indexed up
 * to 786 KB behind the palette, MULU.w).
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

    if ((UWORD)cs >= (UWORD)src->numcols)    /* CMP.w : BCC - fixed #678 */
        return BB_FALSE;
    if ((UWORD)cd >= (UWORD)dst->numcols)
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

/* ClearPlane: clear below `end`: (UWORD)moves + 1 blocks of 40 bytes if
   `blocks` (fixed #682: none for a plane of less than 40 bytes), then
   (UWORD)(moves >> 16) + 1 words unless that is $FFFF, then one byte if
   `odd` (fixed #681: the rest of an odd number of bytes) */
static void clearplane(UBYTE *end, ULONG moves, BOOL blocks, BOOL odd)
{
    ULONG *l = (ULONG *)end;
    UWORD *w;
    UWORD n = (UWORD)moves;

    if (blocks)
        do {
            l -= 10;                    /* MOVEM.l d0-d6/a1-a3,-(a0) */
            l[0] = l[1] = l[2] = l[3] = l[4] = l[5] = l[6] = l[7] = l[8] = l[9] = 0;
        } while (n-- != 0);
    w = (UWORD *)l;
    n = (UWORD)(moves >> 16);
    if (n != 0xffff)
        do
            *--w = 0;
        while (n-- != 0);
    if (odd)
        *((UBYTE *)w - 1) = 0;          /* the plane's first byte */
}

/**
 * @brief CPUCls bitmap#: clear every plane of a bitmap with the CPU.
 *
 * Nothing without plane 0 (fixed #680: the original's test left through
 * the wrong exit, see the file's head).  BytesPerRow*Rows bytes per
 * plane, cleared from the end down; the
 * original's register arithmetic for the block count (DIVU #40, the rest
 * as words).  Blocks of 40 bytes, DBRA on blocks - 1; a plane of less
 * than 40 bytes has no block (fixed #682: the original's counter was
 * $FFFF, 65536 blocks, 2.6 MB below the plane); the last byte of an odd
 * rest, the plane's first, is cleared as well (fixed #681: the original
 * cleared the rest as words, ASR.w #1, and left it).  Both decided once
 * per call.  The depth is the byte Depth (fixed #679: the original read
 * the word Flags<<8|Depth); depth 0 gives the DBRA counter $FFFF, 65536
 * planes - an invalid argument, refused by cpuclschk with runtime error
 * checking (fixed #679), here unchecked.
 * @param bm  a0: the bitmap object (!libs #bitmaplib,#ia0|#pd0)
 */
static void BLITZ_ROUTINE cpucls_c(struct bb_bitmap *bm BL_REG(a0))
{
    ULONG size = bl_mulu_w(bm->bpr, bm->rows);
    ULONG moves = bl_divu_w(size, 40);
    UWORD planes = (UWORD)(bm->depth - 1);         /* fixed #679 */
    const ULONG *pl = (const ULONG *)bm->planes;
    BOOL blocks = size >= 40;           /* fixed #682 */
    BOOL odd = (moves >> 16) & 1;       /* the rest; fixed #681 */

    if (!*pl)                           /* TST.l 8(a0):BEQ - fixed #680 */
        return;
    BL_SETW(moves, moves - 1);          /* blocks - 1 */
    moves = BL_SWAP(moves);
    BL_SETW(moves, (WORD)moves >> 1);   /* rest / 2 (ASR.w) */
    BL_SETW(moves, moves - 1);
    moves = BL_SWAP(moves);
    do
        clearplane((UBYTE *)(*pl++ + size), moves, blocks, odd);
    while (planes-- != 0);
}

BLITZ_THUNK(cpucls, cpucls_c, "d0-d1/a0-a1");

/* ------------------------------------------------------------- SaveCMAP */

/**
 * @brief SaveCMAP(palette#,filename$): save a palette as an IFF ILBM with a CMAP.
 *
 * Builds FORM ILBM with an empty BMHD (20 zero bytes: no defect, a
 * palette file) and the CMAP (3 bytes per colour, the channels' top
 * bytes) in memory (memlib globalloc, MEMF_CLEAR), writes it with
 * RIAmosFuncLib's BSave (form 1: d0 name, d1 address, d2 length) and
 * frees it.  A CMAP of odd length gets its pad byte (the cleared memory;
 * the file and the FORM length rounded up to even, the CMAP length not:
 * fixed #687, the original wrote none).  A palette of 0 colours gives a
 * CMAP of length 0, a file of 48 bytes (fixed #688: the original's DBRA
 * from numcols - 1 copied 65536 colours, 196608 bytes behind the block).
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
    size = (colsize + 8 + 12 + 28 + 1) & ~1UL;     /* the pad byte: fixed #687 */
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
    n = data->numcols;                  /* 0: no pass, fixed #688 */
    while (n-- != 0) {
        p[0] = s[0];
        p[1] = s[4];
        p[2] = s[8];
        p += 3;
        s += 12;
    }
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

/* ------------------------------------------- error checks (debugger only) */

BLITZ_ERRCHK_MESSAGE(baddepth, "Illegal BitMap Depth");          /* fixed, #679 */
BLITZ_ERRCHK_MESSAGE(endbefore, "PalAdjust end colour before the start colour");  /* fixed, #684 */
BLITZ_ERRCHK_MESSAGE(noinfo, "No palette selected with PaletteInfo");   /* fixed, #686 */

/**
 * @brief The error check of CPUCls (debugger only).
 *
 * Fixed (#679), not in the original (rigfxlib had no error check area):
 * CPUCls counts the planes with SUBQ.w #1 ... DBRA on the depth, so a
 * bitmap of depth 0 clears 65536 "planes" through the longs behind the
 * object's 8 plane pointers.  The release code is unchanged (an invalid
 * argument, D-005).
 *
 * Errors: "Illegal BitMap Depth" (bitmaplib's text) for a depth of 0.
 * @param bm  a0: the bitmap object (!libs #bitmaplib,#ia0|#pd0)
 */
static void BLITZ_ERRCHK cpuclschk_c(const struct bb_bitmap *bm BL_REG(a0))
{
    if (!bm->depth)
        BLITZ_RUNERR(baddepth);
}

/* the PaletteInfo palette: fixed (#686), the commands that read it used
   address 0 before the first PaletteInfo */
static void BLITZ_ERRCHK infochk(void)
{
    if (!paladdr)
        BLITZ_RUNERR(noinfo);
}

/**
 * @brief The error check of PalRed ... AGAPalBlue (debugger only).
 *
 * Fixed (#686), not in the original: the colour functions read the
 * palette of the last PaletteInfo without a test, before the first one
 * from address 0 (the exception vectors).  The release code is unchanged
 * (a command before its setup command, a programming error; as #176).
 *
 * Errors: "No palette selected with PaletteInfo".
 */
static void BLITZ_ERRCHK colchk_c(void)
{
    infochk();
}

/**
 * @brief The error check of PalAdjust's short form (debugger only).
 *
 * Fixed (#686), not in the original: PalAdjust takes the colours and their
 * number from the palette of the last PaletteInfo, before the first one
 * from address 0 and the count from address -4.  The release code is
 * unchanged.
 *
 * Errors: "No palette selected with PaletteInfo".
 */
static void BLITZ_ERRCHK paladjchk_c(void)
{
    infochk();
}

/**
 * @brief The error check of PalAdjust's long form (debugger only).
 *
 * Fixed (#684, #686), not in the original: PalAdjust reads the palette of
 * the last PaletteInfo without a test (before the first one: address 0),
 * and it takes the count end - start + 1 as a word, * 3 (MULU.w) - 1 as
 * the DBRA counter, so end = start - 1 wrote 65536 channels (256 KB) from
 * colour start on, and a smaller end about as many.  The release code is
 * unchanged (invalid arguments, D-005; FillPalette refuses end < start in
 * its release code, PalAdjust has no such test).
 *
 * Errors: "No palette selected with PaletteInfo"; "PalAdjust end colour
 * before the start colour" (end < start as signed words, FillPalette's
 * compare).
 * @param start  d2: the first colour
 * @param end    d3: the last colour
 */
static void BLITZ_ERRCHK paladj2chk_c(ULONG start BL_REG(d2), ULONG end BL_REG(d3))
{
    infochk();
    if ((WORD)end < (WORD)start)
        BLITZ_RUNERR(endbefore);
}

/* the thunks of the check functions */
BLITZ_ERRCHK_THUNK(cpuclschk, cpuclschk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(colchk, colchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(paladjchk, paladjchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(paladj2chk, paladj2chk_c, "d0-d1/a0-a1");
