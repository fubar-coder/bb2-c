/*
 * rifxlib - RWE userlib 66 (Stephen McNamara, Reflective Images,
 * 1994-96): effects on planar bitmaps.
 *
 *   ClearBitmap / FadeInBitmap   line by line with an alternating dither
 *       mask ($AAAA/$5555) and a busy-wait delay per line: the masked
 *       line runs 19 lines ahead of the cleared/copied one
 *   ZoomX2 / ZoomX4 (x+y, x only, y only) / ZoomX8 / ZoomyZoom / ReduceX2
 *       through lookup tables that init builds (1856 bytes, memlib
 *       globalloc: reduce 256 bytes, x2 256 words, x4 256 longs, x8 16
 *       longs); GetZoomTables = the address of the four table pointers
 *   Derez, ZoomXY (after InitZoomXY) and their ScreenCopy: lores bitmaps
 *       of 40 bytes per line
 *   ADDValue(bitmap,x,y) = y * bytes per row + x/8
 *
 * C translation of rwelibsrc/FXLib/FX-Library13.bb; reference BSS2
 * RIFXLIB.OBJ (the source builds to it byte for byte with blitz2 2.10,
 * docs/rwelibs/README.md 3.1).  Notes: docs/rwelibs/port/rifxlib.md.
 *
 * Arguments as the compiler passes them (!libs {#bitmaplib,#ia0|#pd0}):
 * a0/a1 = the bitmap (struct BitMap) of the first/second parameter, the
 * parameters in d0-d5; ZoomX4's seventh, xy_select (a byte), is on the
 * argument stack, -1(a2).  The routines are C with the original's loops
 * (word arithmetic and DBRA counts kept: `do ... while (n-- != 0)` runs
 * n+1 times, 65536 for n = 0xFFFF); addresses are ULONGs as in the
 * registers.  The routines that keep every register in the original sit
 * behind thunks (BLITZ_THUNK) that keep d0/d1/a0/a1 too.  No assembler
 * except those thunks.
 *
 * Kept from the original (BUG(orig)):
 *   - ClearBitmap starts at end-4 (its comment says 2): every pass is one
 *     word too low, the last word of the area stays and the last pass
 *     clears the word before the area (before the bitplane for offset 0);
 *   - ClearBitmap, FadeInBitmap and ZoomyZoom read the depth as the word
 *     at 4 (bm_Flags:bm_Depth), right only while bm_Flags is 0;
 *   - delay 0 in ClearBitmap/FadeInBitmap means 2^32 turns of the loop;
 *   - Derez is declared with a result (!acommand{#long}) but sets none:
 *     d0 is what the routine leaves there (the input for derez 0) -
 *     reproduced, it is what a Blitz program gets;
 *   - Derez and ZoomXY keep the upper byte of sbitdepth from the previous
 *     call (MOVE.b into its low byte; after a depth 0 it is $FF);
 *   - Derez with a value >= 3 builds its mask table myline with 20 words
 *     (21 for values > 31) into the 18 words the source reserves: the
 *     19th goes over myflag and the pad byte, the 20th (21st) over the
 *     first instruction(s) of ZoomXY (_AsmZoom: TST.w d0, BEQ ...).  DIFF:
 *     here myline has room for 21 words, ZoomXY stays intact - corrupting
 *     code cannot be translated (the values read back are the same);
 *   - ZoomyZoom ignores the bitmap's size: fixed 32 lines of 80 bytes, the
 *     left 40 bytes doubled in place;
 *   - the pointer arrays bit_add/dest_add hold 16 longs each and are not
 *     checked: FadeInBitmap with more than 8 planes runs from bit_add into
 *     dest_add (as in the original: one array pa[] here, with the
 *     variables that follow them in the same order), more than 18 planes
 *     overwrite the variables behind them (in the original: the code of
 *     InitZoomXY);
 *   - finit frees the tables but keeps the pointers.
 * Not bugs but limits the source names: ZoomXY, Derez (and their
 * ScreenCopy) only for lores bitmaps (40 bytes per line).
 */
#include "blitzlib.h"

/* entry points (C functions and thunks, defined below) */
static void BLITZ_ROUTINE clearbitmap2(ULONG delay BL_REG(d1), ULONG bm BL_REG(a0));
static void BLITZ_ROUTINE clearbitmap(ULONG delay BL_REG(d1), ULONG h BL_REG(d2),
                                      ULONG off BL_REG(d3), ULONG bm BL_REG(a0));
static void BLITZ_ROUTINE fadeinbitmap2(ULONG delay BL_REG(d2), ULONG src BL_REG(a0),
                                        ULONG dst BL_REG(a1));
static void BLITZ_ROUTINE fadeinbitmap(ULONG delay BL_REG(d2), ULONG h BL_REG(d3),
                                       ULONG off1 BL_REG(d4), ULONG off2 BL_REG(d5),
                                       ULONG src BL_REG(a0), ULONG dst BL_REG(a1));
void zoomxy_no_xy(void), zoomx4_xy(void), asmderez(void);
void AsmZoom(void), InitZoomxy(void), zoomx2(void), zoomtables(void);
void zoomx8(void), ADDValue(void), ReduceX2(void), dozoomy(void);
void libinit(void), libfinit(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* #ia0|#pd0: the bitmap that parameter `p` names, into address register
   a`r` (the source's macro !bmap{r,p}) */
#define RI_BMAP(r, p) LIB_bitmaplib, BL_LA(r) | BL_ITEM | BL_PD(p)
/* source# and dest#: the bitmaps of parameters 0 and 1 into a0 and a1 */
#define RI_BMAPS() BLITZ_LIBS(RI_BMAP(0, 0), RI_BMAP(1, 1))

/* !libheader {#rifxlib,init,0,finit,0} */
BLITZ_LIBHEADER(LIB_rifxlib, init, BL_NORET, finit, 0);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS(RI_BMAP(0, 0));
BLITZ_SUBS(clearbitmap2, 0, 0);
BLITZ_ARGS(BL_LONG, BL_LONG, BL_WORD, BL_WORD);
BLITZ_LIBS(RI_BMAP(0, 0));
BLITZ_SUBS(clearbitmap, 0, 0);
BLITZ_NAME("ClearBitmap", "bitmap#,delay[,height,offset]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_LONG, BL_LONG);
RI_BMAPS();
BLITZ_SUBS(fadeinbitmap2, 0, 0);
BLITZ_ARGS(BL_LONG, BL_LONG, BL_LONG, BL_WORD, BL_WORD, BL_WORD);
RI_BMAPS();
BLITZ_SUBS(fadeinbitmap, 0, 0);
BLITZ_NAME("FadeInBitmap", "bitmapsource#,bitmapdest#,delay[,height,offset1,offset2]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_BYTE, BL_BYTE, BL_LONG, BL_LONG, BL_WORD, BL_WORD);
RI_BMAPS();
BLITZ_SUBS(zoomxy_no_xy, 0, 0);
BLITZ_ARGS(BL_BYTE, BL_BYTE, BL_LONG, BL_LONG, BL_WORD, BL_WORD, BL_BYTE);
RI_BMAPS();
BLITZ_SUBS(zoomx4_xy, 0, 0);
BLITZ_NAME("ZoomX4", "source#,dest#,sourceadd,destadd,width,height[,xy_select]");

/* BUG(orig): declared with a result, sets none (see asmderez) */
BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG, BL_LONG, BL_LONG, BL_LONG, BL_WORD, BL_WORD);
RI_BMAPS();
BLITZ_SUBS(asmderez, 0, 0);
BLITZ_NAME("Derez", "source#,dest#,sourceadd,destadd,derez_value,height");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(AsmZoom, 0, 0);
BLITZ_NAME("ZoomXY", "xzoom,yzoom,height");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_LONG, BL_LONG, BL_LONG);
RI_BMAPS();
BLITZ_SUBS(InitZoomxy, 0, 0);
BLITZ_NAME("InitZoomXY", "source#,dest#,sourceadd,destadd");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_LONG, BL_LONG, BL_LONG, BL_WORD, BL_WORD);
RI_BMAPS();
BLITZ_SUBS(zoomx2, 0, 0);
BLITZ_NAME("ZoomX2", "source#,dest#,sourceadd,destadd,width,height");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(zoomtables, 0, 0);
BLITZ_NAME("GetZoomTables", "- return pointer to array of tables");

BLITZ_NULLSUB(0, 0, 0);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_BYTE, BL_BYTE, BL_LONG, BL_LONG, BL_WORD, BL_WORD);
RI_BMAPS();
BLITZ_SUBS(zoomx8, 0, 0);
BLITZ_NAME("ZoomX8", "source#,dest#,sourceadd,destadd,width,height");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_BYTE, BL_WORD, BL_WORD);
BLITZ_LIBS(RI_BMAP(0, 0));
BLITZ_SUBS(ADDValue, 0, 0);
BLITZ_NAME("ADDValue", "(bitmap#,x,y)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_BYTE, BL_BYTE, BL_LONG, BL_LONG, BL_WORD, BL_WORD);
RI_BMAPS();
BLITZ_SUBS(ReduceX2, 0, 0);
BLITZ_NAME("ReduceX2", "source#,dest#,sourceadd,destadd,width,height");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(RI_BMAP(0, 0));
BLITZ_SUBS(dozoomy, 0, 0);
BLITZ_NAME("ZoomyZoom", "bitmap#");

BLITZ_NULLSUB_AT(finit, libfinit, 0, 0);
BLITZ_NULLSUB_AT(init, libinit, 0, 0);
BLITZ_LIBFIN();

/* ------------------------------------------------------------ memory */

/* memory as the registers see it: addresses are ULONGs */
#define B_(a) (*(UBYTE *)(a))
#define W_(a) (*(UWORD *)(a))
#define L_(a) (*(ULONG *)(a))
/* struct BitMap: bm_BytesPerRow +0, bm_Rows +2, bm_Depth +5, planes +8 */
#define BM_BPR(bm)    W_((bm) + 0)
#define BM_ROWS(bm)   W_((bm) + 2)
#define BM_FLDEPTH(bm) W_((bm) + 4)    /* bm_Flags:bm_Depth as a word */
#define BM_DEPTH(bm)  B_((bm) + 5)
#define BM_PLANES(bm) ((bm) + 8)

/* ------------------------------------------------------------ variables */

static UWORD EWWidth, BHeight, BDepth;
static ULONG delay_timer;

/* bytesperline/bytesperline2: longs of which only the low word is ever
   written (MOVE.w d,bytesperline+2), so their value is that word */
static UWORD bytesperline, bytesperline2;
static UWORD xzoom = 1, yzoom = 1;

static ULONG tableaddress;
/* reducex2table, zoomx2table, zoomx4table, zoomx8table: GetZoomTables
   returns the address of this array */
static ULONG tables[4];
#define reducex2table tables[0]
#define zoomx2table   tables[1]
#define zoomx4table   tables[2]
#define zoomx8table   tables[3]

static ULONG add_value = 1;
static UWORD res_value, res_value2, res_value3;
/* myline: 18 words in the source, written with up to 21 (BUG(orig) and
   DIFF, see the head of the file) */
static UWORD myline[21] = {
    0x6db6, 0xdb6d, 0xb6db, 0x6db6, 0xdb6d, 0xb6db,
    0x6db6, 0xdb6d, 0xb6db, 0x6db6, 0xdb6d, 0xb6db,
    0x6db6, 0xdb6d, 0xb6db, 0x6db6, 0xdb6d, 0xb6db,
};

static UWORD yzoom_value = 1, yzoom_value2 = 1, xzoom_value = 1;
static UWORD sbitdepth, yvalue, height;

/* bit_add (16 longs), dest_add (16 longs), source_address, dest_address,
   add_source, add_dest - one array: the stores of the plane pointers do
   not check the depth and run from one into the next */
static ULONG pa[36];
#define bit_add        (&pa[0])
#define dest_add       (&pa[16])
#define source_address pa[32]
#define dest_address   pa[33]
#define add_source     pa[34]
#define add_dest       pa[35]

/* ------------------------------------------------------------ init/finit */

#define TABLESIZE 1856

static void BLITZ_ROUTINE libfinit_c(void)
{
    struct bl_regs r;

    if (!tableaddress)
        return;
    r = (struct bl_regs){ .d0 = TABLESIZE, .a1 = tableaddress };
    BLITZ_ALIBJSR(TOK_globfree, r);     /* BUG(orig): the pointers stay */
}
BLITZ_THUNK(libfinit, libfinit_c, "d1/a0-a1");

static void BLITZ_ROUTINE libinit_c(void)
{
    struct bl_regs r = { .d0 = TABLESIZE, .d1 = MEMF_PUBLIC };
    ULONG a0, d1;
    UWORD d0;

    a0 = BLITZ_ALIBJSR(TOK_globalloc, r);
    if (!a0)
        return;
    tableaddress = a0;

    /* CreateReduceX2: bits 7,5,3,1 -> bits 3,2,1,0 */
    reducex2table = a0;
    for (d0 = 0; d0 != 256; d0++) {
        d1 = 0;
        if (d0 & 0x80) d1 |= 8;
        if (d0 & 0x20) d1 |= 4;
        if (d0 & 0x08) d1 |= 2;
        if (d0 & 0x02) d1 |= 1;
        B_(a0) = (UBYTE)d1;
        a0 += 1;
    }
    /* CreateZoomX2: every bit twice */
    zoomx2table = a0;
    for (d0 = 0; d0 != 256; d0++) {
        UWORD v = 0;
        int b;
        for (b = 0; b < 8; b++)
            if (d0 & (1 << b))
                v |= (UWORD)(3 << (2 * b));
        W_(a0) = v;
        a0 += 2;
    }
    /* CreateZoomX4: every bit four times */
    zoomx4table = a0;
    for (d0 = 0; d0 != 256; d0++) {
        ULONG v = 0;
        int b;
        for (b = 0; b < 8; b++)
            if (d0 & (1 << b))
                v |= 0xfUL << (4 * b);
        L_(a0) = v;
        a0 += 4;
    }
    /* CreateZoomX8: four bits, every bit eight times */
    zoomx8table = a0;
    for (d0 = 0; d0 != 16; d0++) {
        ULONG v = 0;
        int b;
        for (b = 0; b < 4; b++)
            if (d0 & (1 << b))
                v |= 0xffUL << (8 * b);
        L_(a0) = v;
        a0 += 4;
    }
}
BLITZ_THUNK(libinit, libinit_c, "d1/a0-a1");

/* GetZoomTables */
static ULONG BLITZ_ROUTINE zoomtables_c(void)
{
    return (ULONG)tables;
}
BLITZ_THUNK(zoomtables, zoomtables_c, "d1/a0-a1");

/* ------------------------------------------------- ClearBitmap/FadeIn */

/* the delay: SUBQ.l #1,d7:BNE - BUG(orig): 0 = 2^32 turns */
static void slow_down(void)
{
    volatile ULONG t = delay_timer;

    do
        t--;
    while (t);
}

/* ClearBitmap bitmap#,delay,height,offset - d1 = delay, d2 = height,
   d3 = offset, a0 = bitmap.  Clears backwards from the end of the area:
   lines are first masked with the alternating mask, 19 lines later
   cleared. */
static void clearbitmap_c(ULONG delay, UWORD h, UWORD off, ULONG bm)
{
    ULONG d2, *a1, *a0, a2;
    UWORD n, d6, d7, mask;
    WORD d5, d4;

    delay_timer = delay;
    EWWidth = (UWORD)(((WORD)BM_BPR(bm) >> 1) - 1);
    BDepth = (UWORD)(BM_FLDEPTH(bm) - 1);       /* BUG(orig): word at 4 */
    BHeight = (UWORD)(h - 1);
    d2 = bl_mulu_w((UWORD)(h + off), BM_BPR(bm));
    d2 -= 4;                                    /* BUG(orig): 4, not 2 */

    a0 = (ULONG *)BM_PLANES(bm);
    a1 = bit_add;
    {
        ULONG *a2p = dest_add;
        n = BDepth;
        do {
            ULONG v = *a0++ + d2;
            *a1++ = v;
            *a2p++ = v;
        } while (n-- != 0);
    }

    d5 = (WORD)BHeight;
    d4 = -19;
    mask = 0xaaaa;
    do {
        if (d5 >= 0) {                          /* Mask_a_line */
            a1 = bit_add;
            d6 = EWWidth;
            do {
                d7 = BDepth;
                a0 = a1;
                do {
                    a2 = *a0;
                    W_(a2) &= mask;
                    *a0++ -= 2;
                } while (d7-- != 0);
            } while (d6-- != 0);
            mask = (UWORD)~mask;
        }
        if (d4 >= 0) {                          /* Clear_a_line */
            a1 = dest_add;
            d6 = EWWidth;
            do {
                d7 = BDepth;
                a0 = a1;
                do {
                    a2 = *a0;
                    W_(a2) = 0;
                    *a0++ -= 2;
                } while (d7-- != 0);
            } while (d6-- != 0);
        }
        slow_down();
        d4 = (WORD)(d4 + 1);
        d5 = (WORD)(d5 - 1);
    } while (d5 != -20);
}

/* the two forms: without height/offset the whole bitmap (bm_Rows, 0).
   The original keeps a4-a6 only: the C functions are the entries. */
static void BLITZ_ROUTINE clearbitmap2(ULONG delay BL_REG(d1), ULONG bm BL_REG(a0))
{
    clearbitmap_c(delay, BM_ROWS(bm), 0, bm);
}
static void BLITZ_ROUTINE clearbitmap(ULONG delay BL_REG(d1), ULONG h BL_REG(d2),
                                      ULONG off BL_REG(d3), ULONG bm BL_REG(a0))
{
    clearbitmap_c(delay, (UWORD)h, (UWORD)off, bm);
}

/* FadeInBitmap source#,dest#,delay,height,offset1,offset2 - d2 = delay,
   d3 = height, d4/d5 = line offsets (both times the SOURCE's bytes per
   row), a0/a1 = bitmaps.  Copies forwards: lines are first copied
   through the mask, 19 lines later copied whole. */
static void fadein_c(ULONG delay, UWORD h, UWORD off1, UWORD off2, ULONG src, ULONG dst)
{
    ULONG o1, o2, *a0, *a1, *a2, *a3, s, d;
    UWORD n, d6, d7, mask;
    WORD d5, d4;

    delay_timer = delay;
    EWWidth = (UWORD)(((WORD)BM_BPR(src) >> 1) - 1);
    BDepth = (UWORD)(BM_FLDEPTH(src) - 1);      /* BUG(orig): word at 4 */
    BHeight = (UWORD)(h - 1);
    o1 = bl_mulu_w(off1, BM_BPR(src));
    o2 = bl_mulu_w(off2, BM_BPR(src));

    a0 = (ULONG *)BM_PLANES(src);
    a1 = (ULONG *)BM_PLANES(dst);
    a2 = bit_add;
    a3 = dest_add;
    n = BDepth;
    do {                                        /* pairs source, dest */
        ULONG v = *a0++ + o1;
        *a2++ = v;
        *a3++ = v;
        v = *a1++ + o2;
        *a2++ = v;
        *a3++ = v;
    } while (n-- != 0);

    d5 = (WORD)BHeight;
    d4 = -19;
    mask = 0xaaaa;
    do {
        if (d5 >= 0) {                          /* Mask_a_line */
            a1 = bit_add;
            d6 = EWWidth;
            do {
                d7 = BDepth;
                a0 = a1;
                do {
                    s = a0[0];
                    d = a0[1];
                    W_(d) = W_(s) & mask;
                    *a0++ += 2;
                    *a0++ += 2;
                } while (d7-- != 0);
            } while (d6-- != 0);
            mask = (UWORD)~mask;
        }
        if (d4 >= 0) {                          /* Clear_a_line: copy */
            a1 = dest_add;
            d6 = EWWidth;
            do {
                d7 = BDepth;
                a0 = a1;
                do {
                    s = a0[0];
                    d = a0[1];
                    W_(d) = W_(s);
                    *a0++ += 2;
                    *a0++ += 2;
                } while (d7-- != 0);
            } while (d6-- != 0);
        }
        slow_down();
        d4 = (WORD)(d4 + 1);
        d5 = (WORD)(d5 - 1);
    } while (d5 != -20);
}

/* the two forms (bm_Rows of the source, offsets 0); entries as above */
static void BLITZ_ROUTINE fadeinbitmap2(ULONG delay BL_REG(d2), ULONG src BL_REG(a0),
                                        ULONG dst BL_REG(a1))
{
    fadein_c(delay, BM_ROWS(src), 0, 0, src, dst);
}
static void BLITZ_ROUTINE fadeinbitmap(ULONG delay BL_REG(d2), ULONG h BL_REG(d3),
                                       ULONG off1 BL_REG(d4), ULONG off2 BL_REG(d5),
                                       ULONG src BL_REG(a0), ULONG dst BL_REG(a1))
{
    fadein_c(delay, (UWORD)h, (UWORD)off1, (UWORD)off2, src, dst);
}

/* ------------------------------------------------------------ the zooms */

/* the common start of ZoomX2/X4/X8: width in bytes (ASR.w #3), the DBRA
   counts in xzoom/yzoom; FALSE if one of them is negative */
static BOOL zoomstart(UWORD width, UWORD h, WORD *w8)
{
    WORD xz, yz;

    *w8 = (WORD)width >> 3;
    xz = (WORD)(*w8 - 1);
    if (xz < 0)
        return FALSE;
    yz = (WORD)(h - 1);
    if (yz < 0)
        return FALSE;
    xzoom = (UWORD)xz;
    yzoom = (UWORD)yz;
    return TRUE;
}

/* ZoomX4 source#,dest#,sourceadd,destadd,width,height[,xy_select] -
   xy_select 1 = only x (4 times as wide), 2 = only y (4 times as high),
   anything else both */
static void zoomx4_c(UBYTE sel, ULONG srcadd, ULONG dstadd, UWORD width, UWORD h,
                     ULONG src, ULONG dst)
{
    ULONG sp = BM_PLANES(src), dp = BM_PLANES(dst);
    ULONG smod, dmod, a0, a1, a2, a3, a5, tab = zoomx4table;
    UWORD planes, d0, d1, dbpr = BM_BPR(dst);
    WORD w8;

    planes = (UBYTE)(BM_DEPTH(src) - 1);        /* SUBQ.b: depth 0 = 256 */
    if (!zoomstart(width, h, &w8))
        return;
    smod = (UWORD)(BM_BPR(src) - w8);

    if (sel == 1) {                             /* X4_x */
        dmod = (UWORD)(dbpr - (UWORD)(w8 << 2));
        do {
            a0 = L_(sp) + srcadd;
            sp += 4;
            a1 = L_(dp) + dstadd;
            dp += 4;
            d1 = yzoom;
            do {
                d0 = xzoom;
                do {
                    L_(a1) = L_(tab + ((ULONG)B_(a0) << 2));
                    a0 += 1;
                    a1 += 4;
                } while (d0-- != 0);
                a0 += smod;
                a1 += dmod;
            } while (d1-- != 0);
        } while (planes-- != 0);
        return;
    }

    bytesperline = dbpr;
    if (sel == 2)                               /* X4_y */
        dmod = (UWORD)((UWORD)(dbpr << 2) - w8);
    else                                        /* x and y */
        dmod = (UWORD)((UWORD)(dbpr << 2) - (UWORD)(w8 << 2));
    do {
        a0 = L_(sp) + srcadd;
        sp += 4;
        a1 = L_(dp) + dstadd;
        dp += 4;
        a2 = a1 + bytesperline;
        a3 = a2 + bytesperline;
        a5 = a3 + bytesperline;
        d1 = yzoom;
        do {
            d0 = xzoom;
            do {
                if (sel == 2) {
                    UBYTE v = B_(a0);
                    B_(a1) = v; a1 += 1;
                    B_(a2) = v; a2 += 1;
                    B_(a3) = v; a3 += 1;
                    B_(a5) = v; a5 += 1;
                } else {
                    ULONG v = L_(tab + ((ULONG)B_(a0) << 2));
                    L_(a1) = v; a1 += 4;
                    L_(a2) = v; a2 += 4;
                    L_(a3) = v; a3 += 4;
                    L_(a5) = v; a5 += 4;
                }
                a0 += 1;
            } while (d0-- != 0);
            a0 += smod;
            a1 += dmod;
            a2 += dmod;
            a3 += dmod;
            a5 += dmod;
        } while (d1-- != 0);
    } while (planes-- != 0);
}

static void BLITZ_ROUTINE zoomx4_6_c(ULONG srcadd BL_REG(d2), ULONG dstadd BL_REG(d3),
                                     ULONG width BL_REG(d4), ULONG h BL_REG(d5),
                                     ULONG src BL_REG(a0), ULONG dst BL_REG(a1))
{
    if (!zoomx4table)
        return;
    zoomx4_c(0, srcadd, dstadd, (UWORD)width, (UWORD)h, src, dst);   /* CLR.b d7 */
}
static void BLITZ_ROUTINE zoomx4_7_c(ULONG srcadd BL_REG(d2), ULONG dstadd BL_REG(d3),
                                     ULONG width BL_REG(d4), ULONG h BL_REG(d5),
                                     ULONG src BL_REG(a0), ULONG dst BL_REG(a1),
                                     const UBYTE *args BL_REG(a2))
{
    if (!zoomx4table)
        return;
    zoomx4_c(args[-1], srcadd, dstadd, (UWORD)width, (UWORD)h, src, dst);  /* -1(a2) */
}
/* the original keeps all registers (MOVEM.l d0-d7/a0-a6) */
BLITZ_THUNK(zoomxy_no_xy, zoomx4_6_c, "d0-d1/a0-a1");
BLITZ_THUNK(zoomx4_xy, zoomx4_7_c, "d0-d1/a0-a1");

/* ZoomX2 source#,dest#,sourceadd,destadd,width,height */
static void BLITZ_ROUTINE zoomx2_c(ULONG srcadd BL_REG(d2), ULONG dstadd BL_REG(d3),
                                   ULONG width BL_REG(d4), ULONG h BL_REG(d5),
                                   ULONG src BL_REG(a0), ULONG dst BL_REG(a1))
{
    ULONG sp = BM_PLANES(src), dp = BM_PLANES(dst);
    ULONG smod, dmod, a0, a1, a2, tab = zoomx2table;
    UWORD planes, d0, d1, dbpr = BM_BPR(dst);
    WORD w8;

    if (!tab)
        return;
    planes = (UBYTE)(BM_DEPTH(src) - 1);
    if (!zoomstart((UWORD)width, (UWORD)h, &w8))
        return;
    smod = (UWORD)(BM_BPR(src) - w8);
    bytesperline = dbpr;
    dmod = (UWORD)(dbpr - w8 - w8 + dbpr);
    do {
        a0 = L_(sp) + srcadd;
        sp += 4;
        a1 = L_(dp) + dstadd;
        dp += 4;
        a2 = a1 + bytesperline;
        d1 = yzoom;
        do {
            d0 = xzoom;
            do {
                UWORD v = W_(tab + ((ULONG)B_(a0) << 1));
                a0 += 1;
                W_(a1) = v; a1 += 2;
                W_(a2) = v; a2 += 2;
            } while (d0-- != 0);
            a0 += smod;
            a1 += dmod;
            a2 += dmod;
        } while (d1-- != 0);
    } while (planes-- != 0);
}
BLITZ_THUNK(zoomx2, zoomx2_c, "d0-d1/a0-a1");

/* ZoomX8 source#,dest#,sourceadd,destadd,width,height - every source
   line becomes 8 lines of 8 times the width (two passes of 4 lines) */
static void BLITZ_ROUTINE zoomx8_c(ULONG srcadd BL_REG(d2), ULONG dstadd BL_REG(d3),
                                   ULONG width BL_REG(d4), ULONG h BL_REG(d5),
                                   ULONG src BL_REG(a0), ULONG dst BL_REG(a1))
{
    ULONG sp = BM_PLANES(src), dp = BM_PLANES(dst);
    ULONG smod, dmod, a0, a1, a2, a3, a5, save, tab = zoomx8table;
    UWORD planes, d0, d1, dbpr = BM_BPR(dst);
    WORD w8;
    int pass;

    if (!tab)
        return;
    planes = (UBYTE)(BM_DEPTH(src) - 1);
    if (!zoomstart((UWORD)width, (UWORD)h, &w8))
        return;
    smod = (UWORD)(BM_BPR(src) - w8);
    bytesperline = dbpr;
    dmod = (UWORD)((UWORD)(dbpr << 2) - (UWORD)(w8 << 3));
    do {
        a0 = L_(sp) + srcadd;
        sp += 4;
        a1 = L_(dp) + dstadd;
        dp += 4;
        a2 = a1 + bytesperline;
        a3 = a2 + bytesperline;
        a5 = a3 + bytesperline;
        d1 = yzoom;
        do {
            save = a0;
            for (pass = 0; pass < 2; pass++) {
                a0 = save;
                d0 = xzoom;
                do {
                    ULONG v = L_(tab + ((B_(a0) & 0xf0) >> 2));
                    L_(a1) = v; a1 += 4;
                    L_(a2) = v; a2 += 4;
                    L_(a3) = v; a3 += 4;
                    L_(a5) = v; a5 += 4;
                    v = L_(tab + ((B_(a0) & 0x0f) << 2));
                    a0 += 1;
                    L_(a1) = v; a1 += 4;
                    L_(a2) = v; a2 += 4;
                    L_(a3) = v; a3 += 4;
                    L_(a5) = v; a5 += 4;
                } while (d0-- != 0);
                a1 += dmod;
                a2 += dmod;
                a3 += dmod;
                a5 += dmod;
            }
            a0 += smod;
        } while (d1-- != 0);
    } while (planes-- != 0);
}
BLITZ_THUNK(zoomx8, zoomx8_c, "d0-d1/a0-a1");

/* ZoomyZoom bitmap# - BUG(orig): depth = the word at 4, and fixed 32
   lines of 80 bytes whatever the bitmap's size; the left 40 bytes of a
   line are doubled in place (from the right, so nothing is read after
   it is written) */
static void BLITZ_ROUTINE dozoomy_c(ULONG bm BL_REG(a0))
{
    ULONG planes = BM_PLANES(bm), a1, tab = zoomx2table;
    UWORD d7, d6;
    WORD d4, d5;

    if (!tab)
        return;
    d7 = (UWORD)(BM_FLDEPTH(bm) - 1);
    do {
        a1 = L_(planes);
        planes += 4;
        d6 = 31;
        do {
            d5 = 78;
            for (d4 = 39; d4 >= 0; d4--, d5 -= 2)
                W_(a1 + d5) = W_(tab + ((ULONG)B_(a1 + d4) << 1));
            a1 += 80;
        } while (d6-- != 0);
    } while (d7-- != 0);
}
/* the original keeps everything if there is no table: keep d0/d1/a0/a1 */
BLITZ_THUNK(dozoomy, dozoomy_c, "d0-d1/a0-a1");

/* ReduceX2 source#,dest#,sourceadd,destadd,width,height - half the size:
   every second line, two source bytes -> one (bits 7,5,3,1) */
static void BLITZ_ROUTINE reducex2_c(ULONG srcadd BL_REG(d2), ULONG dstadd BL_REG(d3),
                                     ULONG width BL_REG(d4), ULONG h BL_REG(d5),
                                     ULONG src BL_REG(a0), ULONG dst BL_REG(a1))
{
    ULONG sp = BM_PLANES(src), dp = BM_PLANES(dst);
    ULONG a0, a1, tab = reducex2table;
    UWORD planes, d0, d1;
    WORD w, xz, yz;

    if (!tab)
        return;
    planes = (UBYTE)(BM_DEPTH(src) - 1);
    bytesperline = (UWORD)(BM_BPR(src) << 1);
    bytesperline2 = BM_BPR(dst);
    w = (WORD)width >> 3;
    bytesperline = (UWORD)(bytesperline - w);
    w >>= 1;
    bytesperline2 = (UWORD)(bytesperline2 - w);
    xz = (WORD)(w - 1);
    if (xz < 0)
        return;
    yz = (WORD)(((WORD)h >> 1) - 1);
    if (yz < 0)
        return;
    xzoom = (UWORD)xz;
    yzoom = (UWORD)yz;
    do {
        a0 = L_(sp) + srcadd;
        sp += 4;
        a1 = L_(dp) + dstadd;
        dp += 4;
        d1 = yzoom;
        do {
            d0 = xzoom;
            do {
                UBYTE v = (UBYTE)(B_(tab + B_(a0)) << 4);
                v |= B_(tab + B_(a0 + 1));
                a0 += 2;
                B_(a1) = v;
                a1 += 1;
            } while (d0-- != 0);
            a0 += bytesperline;
            a1 += bytesperline2;
        } while (d1-- != 0);
    } while (planes-- != 0);
}
BLITZ_THUNK(ReduceX2, reducex2_c, "d0-d1/a0-a1");

/* ------------------------------------------------------- Derez, ZoomXY */

/* 40 bytes (10 longs) from s to d, ascending: MOVE.l (a1)+,(a2)+ x 10 */
#define COPY40(s, d) do {                                               \
        int k_;                                                         \
        for (k_ = 0; k_ < 10; k_++) {                                   \
            L_(d) = L_(s);                                              \
            (s) += 4;                                                   \
            (d) += 4;                                                   \
        }                                                               \
    } while (0)

/* the plane pointers + offsets into bit_add/dest_add (sbitdepth+1) */
static void store_bitplanes(ULONG src, ULONG dst, ULONG sadd, ULONG dadd)
{
    ULONG sp = BM_PLANES(src), dp = BM_PLANES(dst), *a2 = bit_add, *a3 = dest_add;
    UWORD n = sbitdepth;

    do {
        *a2++ = L_(sp) + sadd;
        sp += 4;
        *a3++ = L_(dp) + dadd;
        dp += 4;
    } while (n-- != 0);
}

/* ScreenCopy: height+1 lines of 40 bytes per plane */
static void screencopy(void)
{
    ULONG *a0 = bit_add, *a1 = dest_add, a2, a3;
    UWORD d6 = sbitdepth, d5;

    do {
        a2 = *a0++;
        a3 = *a1++;
        d5 = height;
        do
            COPY40(a2, a3);
        while (d5-- != 0);
    } while (d6-- != 0);
}

/* UnrezX2: every second pixel doubled, every second line doubled;
   returns the last long (d0) */
static ULONG unrezx2(void)
{
    ULONG *a0 = bit_add, *a1 = dest_add, a2, a3, d0 = 0;
    UWORD d7, d6 = sbitdepth, d5, d4;

    d7 = (UWORD)(((WORD)(height + 1) >> 1) - 1);
    do {
        a2 = *a0++;
        a3 = *a1++;
        d5 = d7;
        do {
            d4 = 9;
            do {
                d0 = L_(a2) & 0xaaaaaaaaUL;
                a2 += 4;
                d0 |= (ULONG)((LONG)d0 >> 1);   /* ASR.l */
                L_(a3) = d0;
                a3 += 4;
                L_(a3 + 36) = d0;               /* the line below */
            } while (d4-- != 0);
            a2 += 40;
            a3 += 40;
        } while (d5-- != 0);
    } while (d6-- != 0);
    return d0;
}

/* Derez source#,dest#,sourceadd,destadd,derez_value,height - lores
   bitmaps: blocks of derez_value pixels and lines get the colour of
   their first pixel (1 = ScreenCopy, 2 = UnrezX2, up to 17 through the
   mask table myline, above that bit by bit).  BUG(orig): no result is
   set - the d0 returned here is what the original leaves in d0. */
static ULONG BLITZ_ROUTINE asmderez_c(ULONG d0 BL_REG(d0), ULONG sadd BL_REG(d2),
                                      ULONG dadd BL_REG(d3), ULONG rez BL_REG(d4),
                                      ULONG h BL_REG(d5), ULONG src BL_REG(a0),
                                      ULONG dst BL_REG(a1))
{
    ULONG *a0, *a6, a1, a2, a3, d1, d2;
    UWORD *a5, d4, d6, rv;
    WORD c, b, d7;
    BYTE d3b, d5b;

    if (!(UWORD)rez)
        return d0;
    res_value = (UWORD)rez;
    res_value2 = (UWORD)(rez - 1);
    res_value3 = (UWORD)(rez - 2);
    height = (UWORD)(h - 1);
    sbitdepth = (UWORD)((sbitdepth & 0xff00) | BM_DEPTH(src));  /* BUG(orig) */
    sbitdepth--;
    store_bitplanes(src, dst, sadd, dadd);
    d0 = (d0 & 0xffff0000UL) | 0xffff;          /* DBRA d0 of store_bitplanes */

    if (!res_value2) {
        screencopy();
        return d0;
    }
    if (res_value == 2)
        return unrezx2();

    add_value = bl_mulu_w(40, res_value);
    yvalue = height;

    /* myline: one word per 16 pixels, a bit at the first pixel of every
       block.  BUG(orig): 20 (21) words, see the head of the file. */
    a5 = myline;
    c = 19;
    b = 15;
    rv = res_value;
    d2 = 0;
widthloop:
    d2 |= 1UL << (b & 31);                      /* BSET d1,d2 */
    b = (WORD)(b - rv);
    if (b >= 0)
        goto widthloop;
    *a5++ = (UWORD)d2;
    d2 = 0;
    b = (WORD)(b + 16);
    if (b < 0)
        goto moretodo;
    if (c-- != 0)
        goto widthloop;
    goto yloop;
moretodo:
    *a5++ = 0;
    c = (WORD)(c - 1);
    if (c < 0)
        goto yloop;
    b = (WORD)(b + 16);
    if (b < 0)
        goto moretodo;
    if (c-- != 0)
        goto widthloop;

yloop:
    d0 = (UWORD)c;                              /* d0 = $0000FFFF */
    for (;;) {
        a0 = bit_add;
        a6 = dest_add;
        d6 = sbitdepth;
        do {                                    /* bitplane_loop */
            d7 = (WORD)yvalue;
            a2 = *a0;
            a3 = *a6;
            d1 = 0;
            if (res_value2 <= 16) {
                UWORD d5 = 19;
                a5 = myline;
                do {                            /* xloop */
                    d0 = W_(a2) & *a5++;
                    a2 += 2;
                    d2 = d0;
                    d4 = res_value3;
                    do {
                        d2 = (d2 >> 1) | (d2 << 31);    /* ROR.l #1 */
                        d0 |= d2;
                    } while (d4-- != 0);
                    d0 = (d0 & 0xffff0000UL) | (UWORD)(d0 | d1);
                    W_(a3) = (UWORD)d0;
                    a3 += 2;
                    d0 = BL_SWAP(d0);
                    d1 = (UWORD)d0;
                } while (d5-- != 0);
                goto done_line;
            }
            /* bit mode: every block bit by bit */
            d3b = 15;
            d5b = 19;
        bitxloop:
            d4 = res_value2;
            d0 = (d0 & 0xffff0000UL) | W_(a2);
            if (!(d0 & (1UL << (d3b & 31))))
                goto clearbits;
            d2 = W_(a3);
        setbits_loop:
            d2 |= 1UL << (d3b & 31);
            d3b = (BYTE)(d3b - 1);
            if (d3b < 0)
                goto setbits_word;
            if (d4-- != 0)
                goto setbits_loop;
            goto next_bit;
        setbits_word:
            a2 += 2;
            W_(a3) = (UWORD)d2;
            a3 += 2;
            d2 = W_(a3);
            d3b = 15;
            d5b = (BYTE)(d5b - 1);
            if (d5b < 0)
                goto done_line;
            if (d4-- != 0)
                goto setbits_loop;
            goto next_bit;
        clearbits:
            d2 = W_(a3);
        clearbits_loop:
            d2 &= ~(1UL << (d3b & 31));
            d3b = (BYTE)(d3b - 1);
            if (d3b < 0)
                goto clearbits_word;
            if (d4-- != 0)
                goto clearbits_loop;
            goto next_bit;
        clearbits_word:
            a2 += 2;
            W_(a3) = (UWORD)d2;
            a3 += 2;
            d2 = W_(a3);
            d3b = 15;
            d5b = (BYTE)(d5b - 1);
            if (d5b < 0)
                goto done_line;
            if (d4-- != 0)
                goto clearbits_loop;
        next_bit:
            W_(a3) = (UWORD)d2;
            goto bitxloop;

        done_line:                              /* the line downwards */
            d4 = res_value3;
            a1 = *a6;
            a2 = a1 + 40;
            for (;;) {
                d7 = (WORD)(d7 - 1);
                if (d7 < 0)
                    goto out_of_screen;
                COPY40(a1, a2);
                if (d4-- == 0)
                    break;
            }
            *a0 += add_value;
        out_of_screen:
            a0++;
            *a6++ = a2;
        } while (d6-- != 0);
        yvalue = (UWORD)(yvalue - res_value);
        if ((WORD)yvalue < 0)
            break;
    }
    return d0;
}
/* the original keeps d1-d7/a0-a6 */
BLITZ_THUNK(asmderez, asmderez_c, "d1/a0-a1");

/* InitZoomXY source#,dest#,sourceadd,destadd - for ZoomXY */
static void BLITZ_ROUTINE initzoomxy_c(ULONG sadd BL_REG(d2), ULONG dadd BL_REG(d3),
                                       ULONG src BL_REG(a0), ULONG dst BL_REG(a1))
{
    source_address = src;
    dest_address = dst;
    add_source = sadd;
    add_dest = dadd;
}
BLITZ_THUNK(InitZoomxy, initzoomxy_c, "d0-d1/a0-a1");

/* ZoomXY xzoom,yzoom,height - the bitmaps of InitZoomXY (lores): every
   source pixel xzoom pixels wide, every line yzoom lines high; 1,1 =
   ScreenCopy */
static void BLITZ_ROUTINE asmzoom_c(ULONG xz BL_REG(d0), ULONG yz BL_REG(d1), ULONG h BL_REG(d2))
{
    ULONG *a0, *a6, a1, a2, a3, d0, d2;
    UWORD d4, d6;
    WORD d7;
    BYTE d1b, d3b, d5b;

    if (!(UWORD)xz || !(UWORD)yz)
        return;
    xzoom_value = (UWORD)(xz - 1);
    yzoom_value2 = (UWORD)yz;
    yzoom_value = (UWORD)(yz - 1);
    height = (UWORD)(h - 1);
    sbitdepth = (UWORD)((sbitdepth & 0xff00) | BM_DEPTH(source_address));
    sbitdepth--;
    store_bitplanes(source_address, dest_address, add_source, add_dest);
    if (!xzoom_value && !yzoom_value) {
        screencopy();
        return;
    }
    yvalue = height;
    for (;;) {
        a0 = bit_add;
        a6 = dest_add;
        d6 = sbitdepth;
        do {                                    /* bitplane_loop */
            d7 = (WORD)yvalue;
            a2 = *a0;
            a3 = *a6;
            d5b = 9;
            d3b = 31;
            d1b = 31;
        xloop:
            d4 = xzoom_value;
            d2 = L_(a3);
            d0 = L_(a2);
            if (!(d0 & (1UL << (d1b & 31))))
                goto clearbits_loop;
        setbits_loop:
            d2 |= 1UL << (d3b & 31);
            d3b = (BYTE)(d3b - 1);
            if (d3b < 0)
                goto setbits_word;
            if (d4-- != 0)
                goto setbits_loop;
            goto next_bit;
        setbits_word:
            L_(a3) = d2;
            a3 += 4;
            d2 = L_(a3);
            d3b = 31;
            d5b = (BYTE)(d5b - 1);
            if (d5b < 0)
                goto done_line;
            if (d4-- != 0)
                goto setbits_loop;
            goto next_bit;
        clearbits_loop:
            d2 &= ~(1UL << (d3b & 31));
            d3b = (BYTE)(d3b - 1);
            if (d3b < 0)
                goto clearbits_word;
            if (d4-- != 0)
                goto clearbits_loop;
            goto next_bit;
        clearbits_word:
            L_(a3) = d2;
            a3 += 4;
            d2 = L_(a3);
            d3b = 31;
            d5b = (BYTE)(d5b - 1);
            if (d5b < 0)
                goto done_line;
            if (d4-- != 0)
                goto clearbits_loop;
        next_bit:
            L_(a3) = d2;
            d1b = (BYTE)(d1b - 1);
            if (d1b >= 0)
                goto xloop;
            d1b = 31;
            a2 += 4;
            goto xloop;

        done_line:                              /* the line downwards */
            d4 = yzoom_value;
            if (!d4) {
                a2 = *a6 + 40;                  /* yzoom1 */
            } else {
                d4--;
                a1 = *a6;
                a2 = a1 + 40;
                for (;;) {
                    d7 = (WORD)(d7 - 1);
                    if (d7 < 0)
                        break;
                    COPY40(a1, a2);
                    if (d4-- == 0)
                        break;
                }
            }
            *a0++ += 40;                        /* out_of_screen */
            *a6++ = a2;
        } while (d6-- != 0);
        yvalue = (UWORD)(yvalue - yzoom_value2);
        if ((WORD)yvalue < 0)
            break;
    }
}
/* the original keeps d1-d7/a0-a6, and d0 if it does nothing */
BLITZ_THUNK(AsmZoom, asmzoom_c, "d0-d1/a0-a1");

/* ADDValue(bitmap#,x,y) = y * bm_BytesPerRow + x/8 (ASR.w); changes d1 */
static ULONG BLITZ_ROUTINE addvalue_c(ULONG x BL_REG(d1), ULONG y BL_REG(d2),
                                      ULONG bm BL_REG(a0))
{
    return bl_mulu_w(BM_BPR(bm), (UWORD)y) + (UWORD)((WORD)x >> 3);
}
BLITZ_THUNK(ADDValue, addvalue_c, "a0-a1");
