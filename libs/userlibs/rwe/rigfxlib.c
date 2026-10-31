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
 * +56, +62.w "the planes are allocated here".
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

#define W(p) (*(UWORD *)(p))
#define LG(p) (*(ULONG *)(p))

static UBYTE *paladdr;                  /* colour 0 of PaletteInfo's palette */

/* 12 * colour as the original indexes: MULU.w #12, then the low word as
   a signed index (0(a0,d0.w), ADDA.w) */
static LONG colindex(ULONG c)
{
    return (WORD)(UWORD)bl_mulu_w((UWORD)c, 12);
}

/* ------------------------------------------------------- colour values */

static void BLITZ_ROUTINE paletteinfo_c(UBYTE **obj BL_REG(a0))
{
    paladdr = *obj + 4;
}
BLITZ_THUNK(paletteinfo, paletteinfo_c, "d0-d1/a0-a1");

/* MOVE.b 0/4/8(a0,d0.w),d0: ASR.l #4:ANDI.l #15 (4 bit) or ANDI.l #$ff */
static ULONG BLITZ_ROUTINE palred_c(ULONG c BL_REG(d0))
{
    return paladdr[colindex(c)] >> 4;
}
BLITZ_THUNK(palred, palred_c, "d1/a0-a1");

static ULONG BLITZ_ROUTINE palgreen_c(ULONG c BL_REG(d0))
{
    return paladdr[colindex(c) + 4] >> 4;
}
BLITZ_THUNK(palgreen, palgreen_c, "d1/a0-a1");

static ULONG BLITZ_ROUTINE palblue_c(ULONG c BL_REG(d0))
{
    return paladdr[colindex(c) + 8] >> 4;
}
BLITZ_THUNK(palblue, palblue_c, "d1/a0-a1");

static ULONG BLITZ_ROUTINE agapalred_c(ULONG c BL_REG(d0))
{
    return paladdr[colindex(c)];
}
BLITZ_THUNK(agapalred, agapalred_c, "d1/a0-a1");

static ULONG BLITZ_ROUTINE agapalgreen_c(ULONG c BL_REG(d0))
{
    return paladdr[colindex(c) + 4];
}
BLITZ_THUNK(agapalgreen, agapalgreen_c, "d1/a0-a1");

static ULONG BLITZ_ROUTINE agapalblue_c(ULONG c BL_REG(d0))
{
    return paladdr[colindex(c) + 8];
}
BLITZ_THUNK(agapalblue, agapalblue_c, "d1/a0-a1");

/* ------------------------------------------------------------ PalAdjust */

/* PalAdjust palette#,ratio.q,start,end: target colours start..end =
   PaletteInfo's colours * ratio (8.8 fixed point from the Quick 16.16),
   saturated at 255; only the top byte of each channel is written */
static void BLITZ_ROUTINE paladjust2_c(UBYTE **obj BL_REG(a0), ULONG ratio BL_REG(d1),
                                        ULONG start BL_REG(d2), ULONG end BL_REG(d3))
{
    UWORD r = (UWORD)(((UWORD)(ratio >> 16) << 8) + ((UWORD)ratio >> 8));
    LONG off = colindex(start);         /* ADD.w d5,a0 / a1 */
    const UBYTE *s = paladdr + off;
    UBYTE *d = *obj + 4 + off;
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

/* without start/end: 0 .. (the colours of PaletteInfo's palette) - 1;
   the upper word of `end` is the caller's d3 (SUB.w), unused */
static void BLITZ_ROUTINE paladjust_c(UBYTE **obj BL_REG(a0), ULONG ratio BL_REG(d1),
                                       ULONG end BL_REG(d3))
{
    BL_SETW(end, W(paladdr - 4) - 1);
    paladjust2_c(obj, ratio, 0, end);
}
BLITZ_THUNK(paladjust, paladjust_c, "d0-d1/a0-a1");

/* ---------------------------------------------------------- FillPalette */

/* r, g, b = the top bytes (AGA: the low bytes of the arguments);
   end = 0 (the whole register): the last colour */
static void fill(UBYTE **obj, ULONG r, ULONG g, ULONG b, ULONG start, ULONG end)
{
    UBYTE *data = *obj;
    ULONG *p;
    UWORD n;

    r = (r & 0xff) << 24;               /* ANDI.l #$ff:ROR.l #8 */
    g = (g & 0xff) << 24;
    b = (b & 0xff) << 24;
    if (!end)                           /* TST.l d5 */
        BL_SETW(end, W(data) - 1);
    n = (UWORD)((UWORD)end - (UWORD)start);
    if ((WORD)n < 0)                    /* CMP.w d4,d5:BPL */
        return;
    p = (ULONG *)(data + 4 + bl_mulu_w((UWORD)start, 12));
    do {
        p[0] = r;
        p[1] = g;
        p[2] = b;
        p += 3;
    } while (n-- != 0);
}

/* 4 bit values: ANDI.b #$f:ASL.b #4 */
static ULONG nib(ULONG v)
{
    return (v & 0xf) << 4;
}

static void BLITZ_ROUTINE agafillpalette2_c(UBYTE **obj BL_REG(a0), ULONG r BL_REG(d1),
    ULONG g BL_REG(d2), ULONG b BL_REG(d3), ULONG start BL_REG(d4), ULONG end BL_REG(d5))
{
    fill(obj, r, g, b, start, end);
}
BLITZ_THUNK(agafillpalette2, agafillpalette2_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE agafillpalette_c(UBYTE **obj BL_REG(a0), ULONG r BL_REG(d1),
    ULONG g BL_REG(d2), ULONG b BL_REG(d3))
{
    fill(obj, r, g, b, 0, 0);
}
BLITZ_THUNK(agafillpalette, agafillpalette_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE fillpalette2_c(UBYTE **obj BL_REG(a0), ULONG r BL_REG(d1),
    ULONG g BL_REG(d2), ULONG b BL_REG(d3), ULONG start BL_REG(d4), ULONG end BL_REG(d5))
{
    fill(obj, nib(r), nib(g), nib(b), start, end);
}
BLITZ_THUNK(fillpalette2, fillpalette2_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE fillpalette_c(UBYTE **obj BL_REG(a0), ULONG r BL_REG(d1),
    ULONG g BL_REG(d2), ULONG b BL_REG(d3))
{
    fill(obj, nib(r), nib(g), nib(b), 0, 0);
}
BLITZ_THUNK(fillpalette, fillpalette_c, "d0-d1/a0-a1");

/* ----------------------------------------------------------- CopyColour */

/* -1, or 0 if a colour number is not below the palette's number of
   colours (signed words: BUG(orig) (#678), negative numbers pass) */
static ULONG BLITZ_ROUTINE copycolour_c(UBYTE *src BL_REG(a0), UBYTE *dst BL_REG(a1),
                                        ULONG cs BL_REG(d2), ULONG cd BL_REG(d3))
{
    ULONG *s, *d;

    if ((WORD)cs >= (WORD)W(src + 4))
        return 0;
    if ((WORD)cd >= (WORD)W(dst + 4))
        return 0;
    s = (ULONG *)(*(UBYTE **)src + 4 + bl_mulu_w((UWORD)cs, 12));
    d = (ULONG *)(*(UBYTE **)dst + 4 + bl_mulu_w((UWORD)cd, 12));
    d[0] = s[0];
    d[1] = s[1];
    d[2] = s[2];
    return -1;
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

/* CPUCls bitmap#: every plane, BytesPerRow*Rows bytes; the original's
   register arithmetic for the block count (DIVU #40, the rest as words) */
static void BLITZ_ROUTINE cpucls_c(UBYTE *bm BL_REG(a0))
{
    ULONG size = bl_mulu_w(W(bm), W(bm + 2));
    ULONG moves = bl_divu_w(size, 40);
    UWORD planes = (UWORD)(W(bm + 4) - 1);
    const ULONG *pl = (const ULONG *)(bm + 8);

    BL_SETW(moves, moves - 1);          /* blocks - 1 */
    moves = BL_SWAP(moves);
    BL_SETW(moves, (WORD)moves >> 1);   /* rest / 2 (ASR.w) */
    BL_SETW(moves, moves - 1);
    moves = BL_SWAP(moves);
    do
        clearplane((UBYTE *)(*pl++ + size), moves);
    while (planes-- != 0);
}

/* TST.l 8(a0):BEQ 'oops - BUG(orig) (#680): 'oops is the MOVEM.l (a7)+,a4-a6:RTS
   meant for after MOVEM.l a4-a6,-(a7), which has not run yet: it pops the
   return address and two more longs into a4-a6 and returns to whatever
   follows on the stack.  A stack imbalance C cannot express, so this
   entry is assembler; the real work is cpucls_c. */
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

/* SaveCMAP(palette#,filename): an ILBM with an empty BMHD and the CMAP,
   written by RIAmosFuncLib's BSave; its result, or 0 */
static ULONG BLITZ_ROUTINE savecmap_c(UBYTE **obj BL_REG(a0), ULONG name BL_REG(d1))
{
    struct bl_regs r;
    const UBYTE *data = *obj, *s;
    ULONG colsize, size, res;
    ULONG *h;
    UBYTE *p;
    UWORD n;

    if (!data)
        return 0;
    colsize = bl_mulu_w(W(data), 3);
    size = colsize + 8 + 12 + 28;
    r = (struct bl_regs){ .d0 = size, .d1 = MEMF_CLEAR };
    h = (ULONG *)BLITZ_ALIBJSR(TOK_globalloc, r);
    if (!h)
        return 0;
    h[0] = 0x464f524d;                  /* FORM */
    h[1] = size - 8;
    h[2] = 0x494c424d;                  /* ILBM */
    h[3] = 0x424d4844;                  /* BMHD */
    h[4] = 20;                          /* 20 bytes, left 0 */
    h[10] = 0x434d4150;                 /* CMAP */
    h[11] = colsize;
    p = (UBYTE *)&h[12];
    s = data + 4;
    n = (UWORD)(W(data) - 1);           /* DBRA */
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

/* the planes of src1, then of src2, into dest (numplanes of them; 0 = all
   of both, DBRA from -1); nothing if a source has word 62 = 0, dest not,
   or numplanes > the sum of the depths (signed words) */
static void BLITZ_ROUTINE mergebitplanes_c(UBYTE *src1 BL_REG(a0), UBYTE *src2 BL_REG(a1),
                                            UBYTE *dst BL_REG(a2), ULONG planes BL_REG(d3))
{
    UWORD left1, left2, n;
    const ULONG *planes1, *planes2;
    ULONG *pd;

    if (!W(src1 + 62) || !W(src2 + 62) || W(dst + 62))
        return;
    if ((WORD)planes > (WORD)(UWORD)(W(src1 + 4) + W(src2 + 4)))
        return;
    W(dst + 4) = (UWORD)planes;
    LG(dst) = LG(src1);
    LG(dst + 52) = 0;
    LG(dst + 56) = LG(src1 + 56);
    W(dst + 62) = 1;
    n = (UWORD)(planes - 1);
    left1 = W(src1 + 4);
    left2 = W(src2 + 4);
    planes1 = (const ULONG *)(src1 + 8);
    planes2 = (const ULONG *)(src2 + 8);
    pd = (ULONG *)(dst + 8);
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
