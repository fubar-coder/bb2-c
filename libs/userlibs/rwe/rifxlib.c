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
 *     clears the word before the area (before the bitplane for offset 0)
 *     (#668);
 *   - ClearBitmap, FadeInBitmap and ZoomyZoom read the depth as the word
 *     at 4 (bm_Flags:bm_Depth), right only while bm_Flags is 0 (#670);
 *   - delay 0 in ClearBitmap/FadeInBitmap means 2^32 turns of the loop
 *     (#669);
 *   - Derez is declared with a result (!acommand{#long}) but sets none:
 *     d0 is what the routine leaves there (the input for derez 0) -
 *     reproduced, it is what a Blitz program gets (#672);
 *   - Derez and ZoomXY keep the upper byte of sbitdepth from the previous
 *     call (MOVE.b into its low byte; after a depth 0 it is $FF) (#675);
 *   - Derez with a value >= 3 builds its mask table myline with 20 words
 *     (21 for values > 31) into the 18 words the source reserves: the
 *     19th goes over myflag and the pad byte, the 20th (21st) over the
 *     first instruction(s) of ZoomXY (_AsmZoom: TST.w d0, BEQ ...).  DIFF:
 *     here myline has room for 21 words, ZoomXY stays intact - corrupting
 *     code cannot be translated (the values read back are the same)
 *     (#671);
 *   - ZoomyZoom ignores the bitmap's size: fixed 32 lines of 80 bytes, the
 *     left 40 bytes doubled in place (#676);
 *   - the pointer arrays bit_add/dest_add hold 16 longs each and are not
 *     checked (#674): FadeInBitmap (two pointers per plane in each) runs
 *     from bit_add into dest_add and from dest_add over the four
 *     variables behind it from 9 planes on, and from 11 planes on over
 *     what follows them; ClearBitmap, Derez and ZoomXY (one pointer per
 *     plane) reach the variables from 17 planes on and what follows from
 *     21 on.  What follows is the code of InitZoomXY in the original,
 *     other variables here (one array pa[] with the four variables in the
 *     same order);
 *   - finit frees the tables but keeps the pointers (#673).
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

/* BUG(orig) (#672): declared with a result, sets none (see asmderez) */
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
/* myline: 18 words in the source, written with up to 21 (BUG(orig)
   (#671) and DIFF, see the head of the file) */
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
    BLITZ_ALIBJSR(TOK_globfree, r);     /* BUG(orig) (#673): the pointers stay */
}
BLITZ_THUNK(libfinit, libfinit_c, "d1/a0-a1");

static void BLITZ_ROUTINE libinit_c(void)
{
    struct bl_regs r = { .d0 = TABLESIZE, .d1 = MEMF_PUBLIC };
    ULONG table, bits;
    UWORD index;

    table = BLITZ_ALIBJSR(TOK_globalloc, r);
    if (!table)
        return;
    tableaddress = table;

    /* CreateReduceX2: bits 7,5,3,1 -> bits 3,2,1,0 */
    reducex2table = table;
    for (index = 0; index != 256; index++) {
        bits = 0;
        if (index & 0x80) bits |= 8;
        if (index & 0x20) bits |= 4;
        if (index & 0x08) bits |= 2;
        if (index & 0x02) bits |= 1;
        B_(table) = (UBYTE)bits;
        table += 1;
    }
    /* CreateZoomX2: every bit twice */
    zoomx2table = table;
    for (index = 0; index != 256; index++) {
        UWORD v = 0;
        int b;
        for (b = 0; b < 8; b++)
            if (index & (1 << b))
                v |= (UWORD)(3 << (2 * b));
        W_(table) = v;
        table += 2;
    }
    /* CreateZoomX4: every bit four times */
    zoomx4table = table;
    for (index = 0; index != 256; index++) {
        ULONG v = 0;
        int b;
        for (b = 0; b < 8; b++)
            if (index & (1 << b))
                v |= 0xfUL << (4 * b);
        L_(table) = v;
        table += 4;
    }
    /* CreateZoomX8: four bits, every bit eight times */
    zoomx8table = table;
    for (index = 0; index != 16; index++) {
        ULONG v = 0;
        int b;
        for (b = 0; b < 4; b++)
            if (index & (1 << b))
                v |= 0xffUL << (8 * b);
        L_(table) = v;
        table += 4;
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

/* the delay: SUBQ.l #1,d7:BNE - BUG(orig) (#669): 0 = 2^32 turns */
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
    ULONG areaend, *ptrs, *ptr, addr;
    UWORD n, col, plane, mask;
    WORD maskline, clearline;

    delay_timer = delay;
    EWWidth = (UWORD)(((WORD)BM_BPR(bm) >> 1) - 1);
    BDepth = (UWORD)(BM_FLDEPTH(bm) - 1);       /* BUG(orig) (#670): word at 4 */
    BHeight = (UWORD)(h - 1);
    areaend = bl_mulu_w((UWORD)(h + off), BM_BPR(bm));
    areaend -= 4;                               /* BUG(orig) (#668): 4, not 2 */

    ptr = (ULONG *)BM_PLANES(bm);
    ptrs = bit_add;
    {
        ULONG *destptr = dest_add;
        n = BDepth;
        do {
            ULONG v = *ptr++ + areaend;
            *ptrs++ = v;
            *destptr++ = v;
        } while (n-- != 0);
    }

    maskline = (WORD)BHeight;
    clearline = -19;
    mask = 0xaaaa;
    do {
        if (maskline >= 0) {                    /* Mask_a_line */
            ptrs = bit_add;
            col = EWWidth;
            do {
                plane = BDepth;
                ptr = ptrs;
                do {
                    addr = *ptr;
                    W_(addr) &= mask;
                    *ptr++ -= 2;
                } while (plane-- != 0);
            } while (col-- != 0);
            mask = (UWORD)~mask;
        }
        if (clearline >= 0) {                   /* Clear_a_line */
            ptrs = dest_add;
            col = EWWidth;
            do {
                plane = BDepth;
                ptr = ptrs;
                do {
                    addr = *ptr;
                    W_(addr) = 0;
                    *ptr++ -= 2;
                } while (plane-- != 0);
            } while (col-- != 0);
        }
        slow_down();
        clearline = (WORD)(clearline + 1);
        maskline = (WORD)(maskline - 1);
    } while (maskline != -20);
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
    ULONG srcoff, dstoff, *ptr, *ptrs, *bitptr, *destptr, from, to;
    UWORD n, col, plane, mask;
    WORD maskline, copyline;

    delay_timer = delay;
    EWWidth = (UWORD)(((WORD)BM_BPR(src) >> 1) - 1);
    BDepth = (UWORD)(BM_FLDEPTH(src) - 1);      /* BUG(orig) (#670): word at 4 */
    BHeight = (UWORD)(h - 1);
    srcoff = bl_mulu_w(off1, BM_BPR(src));
    dstoff = bl_mulu_w(off2, BM_BPR(src));

    ptr = (ULONG *)BM_PLANES(src);
    ptrs = (ULONG *)BM_PLANES(dst);
    bitptr = bit_add;
    destptr = dest_add;
    n = BDepth;
    do {                                        /* pairs source, dest */
        ULONG v = *ptr++ + srcoff;
        *bitptr++ = v;
        *destptr++ = v;
        v = *ptrs++ + dstoff;
        *bitptr++ = v;
        *destptr++ = v;
    } while (n-- != 0);

    maskline = (WORD)BHeight;
    copyline = -19;
    mask = 0xaaaa;
    do {
        if (maskline >= 0) {                    /* Mask_a_line */
            ptrs = bit_add;
            col = EWWidth;
            do {
                plane = BDepth;
                ptr = ptrs;
                do {
                    from = ptr[0];
                    to = ptr[1];
                    W_(to) = W_(from) & mask;
                    *ptr++ += 2;
                    *ptr++ += 2;
                } while (plane-- != 0);
            } while (col-- != 0);
            mask = (UWORD)~mask;
        }
        if (copyline >= 0) {                    /* Clear_a_line: copy */
            ptrs = dest_add;
            col = EWWidth;
            do {
                plane = BDepth;
                ptr = ptrs;
                do {
                    from = ptr[0];
                    to = ptr[1];
                    W_(to) = W_(from);
                    *ptr++ += 2;
                    *ptr++ += 2;
                } while (plane-- != 0);
            } while (col-- != 0);
        }
        slow_down();
        copyline = (WORD)(copyline + 1);
        maskline = (WORD)(maskline - 1);
    } while (maskline != -20);
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
static BOOL zoomstart(UWORD width, UWORD h, WORD *wbytes)
{
    WORD xz, yz;

    *wbytes = (WORD)width >> 3;
    xz = (WORD)(*wbytes - 1);
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
    ULONG srcplane = BM_PLANES(src), dstplane = BM_PLANES(dst);
    ULONG smod, dmod, from, line0, line1, line2, line3, tab = zoomx4table;
    UWORD planes, x, y, dbpr = BM_BPR(dst);
    WORD wbytes;

    planes = (UBYTE)(BM_DEPTH(src) - 1);        /* SUBQ.b: depth 0 = 256 */
    if (!zoomstart(width, h, &wbytes))
        return;
    smod = (UWORD)(BM_BPR(src) - wbytes);

    if (sel == 1) {                             /* X4_x */
        dmod = (UWORD)(dbpr - (UWORD)(wbytes << 2));
        do {
            from = L_(srcplane) + srcadd;
            srcplane += 4;
            line0 = L_(dstplane) + dstadd;
            dstplane += 4;
            y = yzoom;
            do {
                x = xzoom;
                do {
                    L_(line0) = L_(tab + ((ULONG)B_(from) << 2));
                    from += 1;
                    line0 += 4;
                } while (x-- != 0);
                from += smod;
                line0 += dmod;
            } while (y-- != 0);
        } while (planes-- != 0);
        return;
    }

    bytesperline = dbpr;
    if (sel == 2)                               /* X4_y */
        dmod = (UWORD)((UWORD)(dbpr << 2) - wbytes);
    else                                        /* x and y */
        dmod = (UWORD)((UWORD)(dbpr << 2) - (UWORD)(wbytes << 2));
    do {
        from = L_(srcplane) + srcadd;
        srcplane += 4;
        line0 = L_(dstplane) + dstadd;
        dstplane += 4;
        line1 = line0 + bytesperline;
        line2 = line1 + bytesperline;
        line3 = line2 + bytesperline;
        y = yzoom;
        do {
            x = xzoom;
            do {
                if (sel == 2) {
                    UBYTE v = B_(from);
                    B_(line0) = v; line0 += 1;
                    B_(line1) = v; line1 += 1;
                    B_(line2) = v; line2 += 1;
                    B_(line3) = v; line3 += 1;
                } else {
                    ULONG v = L_(tab + ((ULONG)B_(from) << 2));
                    L_(line0) = v; line0 += 4;
                    L_(line1) = v; line1 += 4;
                    L_(line2) = v; line2 += 4;
                    L_(line3) = v; line3 += 4;
                }
                from += 1;
            } while (x-- != 0);
            from += smod;
            line0 += dmod;
            line1 += dmod;
            line2 += dmod;
            line3 += dmod;
        } while (y-- != 0);
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
    ULONG srcplane = BM_PLANES(src), dstplane = BM_PLANES(dst);
    ULONG smod, dmod, from, line0, line1, tab = zoomx2table;
    UWORD planes, x, y, dbpr = BM_BPR(dst);
    WORD wbytes;

    if (!tab)
        return;
    planes = (UBYTE)(BM_DEPTH(src) - 1);
    if (!zoomstart((UWORD)width, (UWORD)h, &wbytes))
        return;
    smod = (UWORD)(BM_BPR(src) - wbytes);
    bytesperline = dbpr;
    dmod = (UWORD)(dbpr - wbytes - wbytes + dbpr);
    do {
        from = L_(srcplane) + srcadd;
        srcplane += 4;
        line0 = L_(dstplane) + dstadd;
        dstplane += 4;
        line1 = line0 + bytesperline;
        y = yzoom;
        do {
            x = xzoom;
            do {
                UWORD v = W_(tab + ((ULONG)B_(from) << 1));
                from += 1;
                W_(line0) = v; line0 += 2;
                W_(line1) = v; line1 += 2;
            } while (x-- != 0);
            from += smod;
            line0 += dmod;
            line1 += dmod;
        } while (y-- != 0);
    } while (planes-- != 0);
}
BLITZ_THUNK(zoomx2, zoomx2_c, "d0-d1/a0-a1");

/* ZoomX8 source#,dest#,sourceadd,destadd,width,height - every source
   line becomes 8 lines of 8 times the width (two passes of 4 lines) */
static void BLITZ_ROUTINE zoomx8_c(ULONG srcadd BL_REG(d2), ULONG dstadd BL_REG(d3),
                                   ULONG width BL_REG(d4), ULONG h BL_REG(d5),
                                   ULONG src BL_REG(a0), ULONG dst BL_REG(a1))
{
    ULONG srcplane = BM_PLANES(src), dstplane = BM_PLANES(dst);
    ULONG smod, dmod, from, line0, line1, line2, line3, save, tab = zoomx8table;
    UWORD planes, x, y, dbpr = BM_BPR(dst);
    WORD wbytes;
    int pass;

    if (!tab)
        return;
    planes = (UBYTE)(BM_DEPTH(src) - 1);
    if (!zoomstart((UWORD)width, (UWORD)h, &wbytes))
        return;
    smod = (UWORD)(BM_BPR(src) - wbytes);
    bytesperline = dbpr;
    dmod = (UWORD)((UWORD)(dbpr << 2) - (UWORD)(wbytes << 3));
    do {
        from = L_(srcplane) + srcadd;
        srcplane += 4;
        line0 = L_(dstplane) + dstadd;
        dstplane += 4;
        line1 = line0 + bytesperline;
        line2 = line1 + bytesperline;
        line3 = line2 + bytesperline;
        y = yzoom;
        do {
            save = from;
            for (pass = 0; pass < 2; pass++) {
                from = save;
                x = xzoom;
                do {
                    ULONG v = L_(tab + ((B_(from) & 0xf0) >> 2));
                    L_(line0) = v; line0 += 4;
                    L_(line1) = v; line1 += 4;
                    L_(line2) = v; line2 += 4;
                    L_(line3) = v; line3 += 4;
                    v = L_(tab + ((B_(from) & 0x0f) << 2));
                    from += 1;
                    L_(line0) = v; line0 += 4;
                    L_(line1) = v; line1 += 4;
                    L_(line2) = v; line2 += 4;
                    L_(line3) = v; line3 += 4;
                } while (x-- != 0);
                line0 += dmod;
                line1 += dmod;
                line2 += dmod;
                line3 += dmod;
            }
            from += smod;
        } while (y-- != 0);
    } while (planes-- != 0);
}
BLITZ_THUNK(zoomx8, zoomx8_c, "d0-d1/a0-a1");

/* ZoomyZoom bitmap# - BUG(orig) (#670, #676): depth = the word at 4, and
   fixed 32 lines of 80 bytes whatever the bitmap's size; the left 40
   bytes of a line are doubled in place (from the right, so nothing is
   read after it is written) */
static void BLITZ_ROUTINE dozoomy_c(ULONG bm BL_REG(a0))
{
    ULONG planes = BM_PLANES(bm), line, tab = zoomx2table;
    UWORD plane, y;
    WORD srcx, dstx;

    if (!tab)
        return;
    plane = (UWORD)(BM_FLDEPTH(bm) - 1);
    do {
        line = L_(planes);
        planes += 4;
        y = 31;
        do {
            dstx = 78;
            for (srcx = 39; srcx >= 0; srcx--, dstx -= 2)
                W_(line + dstx) = W_(tab + ((ULONG)B_(line + srcx) << 1));
            line += 80;
        } while (y-- != 0);
    } while (plane-- != 0);
}
/* the original keeps everything if there is no table: keep d0/d1/a0/a1 */
BLITZ_THUNK(dozoomy, dozoomy_c, "d0-d1/a0-a1");

/* ReduceX2 source#,dest#,sourceadd,destadd,width,height - half the size:
   every second line, two source bytes -> one (bits 7,5,3,1) */
static void BLITZ_ROUTINE reducex2_c(ULONG srcadd BL_REG(d2), ULONG dstadd BL_REG(d3),
                                     ULONG width BL_REG(d4), ULONG h BL_REG(d5),
                                     ULONG src BL_REG(a0), ULONG dst BL_REG(a1))
{
    ULONG srcplane = BM_PLANES(src), dstplane = BM_PLANES(dst);
    ULONG from, to, tab = reducex2table;
    UWORD planes, x, y;
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
        from = L_(srcplane) + srcadd;
        srcplane += 4;
        to = L_(dstplane) + dstadd;
        dstplane += 4;
        y = yzoom;
        do {
            x = xzoom;
            do {
                UBYTE v = (UBYTE)(B_(tab + B_(from)) << 4);
                v |= B_(tab + B_(from + 1));
                from += 2;
                B_(to) = v;
                to += 1;
            } while (x-- != 0);
            from += bytesperline;
            to += bytesperline2;
        } while (y-- != 0);
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
    ULONG srcplane = BM_PLANES(src), dstplane = BM_PLANES(dst);
    ULONG *bitptr = bit_add, *destptr = dest_add;
    UWORD n = sbitdepth;

    do {
        *bitptr++ = L_(srcplane) + sadd;
        srcplane += 4;
        *destptr++ = L_(dstplane) + dadd;
        dstplane += 4;
    } while (n-- != 0);
}

/* ScreenCopy: height+1 lines of 40 bytes per plane */
static void screencopy(void)
{
    ULONG *bitptr = bit_add, *destptr = dest_add, from, to;
    UWORD plane = sbitdepth, y;

    do {
        from = *bitptr++;
        to = *destptr++;
        y = height;
        do
            COPY40(from, to);
        while (y-- != 0);
    } while (plane-- != 0);
}

/* UnrezX2: every second pixel doubled, every second line doubled;
   returns the last long (d0) */
static ULONG unrezx2(void)
{
    ULONG *bitptr = bit_add, *destptr = dest_add, from, to, pixels = 0;
    UWORD lines, plane = sbitdepth, y, x;

    lines = (UWORD)(((WORD)(height + 1) >> 1) - 1);
    do {
        from = *bitptr++;
        to = *destptr++;
        y = lines;
        do {
            x = 9;
            do {
                pixels = L_(from) & 0xaaaaaaaaUL;
                from += 4;
                pixels |= (ULONG)((LONG)pixels >> 1); /* ASR.l */
                L_(to) = pixels;
                to += 4;
                L_(to + 36) = pixels;           /* the line below */
            } while (x-- != 0);
            from += 40;
            to += 40;
        } while (y-- != 0);
    } while (plane-- != 0);
    return pixels;
}

/* Derez source#,dest#,sourceadd,destadd,derez_value,height - lores
   bitmaps: blocks of derez_value pixels and lines get the colour of
   their first pixel (1 = ScreenCopy, 2 = UnrezX2, up to 17 through the
   mask table myline, above that bit by bit).  BUG(orig) (#672): no result is
   set - the d0 returned here is what the original leaves in d0. */
static ULONG BLITZ_ROUTINE asmderez_c(ULONG value BL_REG(d0), ULONG sadd BL_REG(d2),
                                      ULONG dadd BL_REG(d3), ULONG rez BL_REG(d4),
                                      ULONG h BL_REG(d5), ULONG src BL_REG(a0),
                                      ULONG dst BL_REG(a1))
{
    /* bitptr a0, destptr a6, copysrc a1, pos a2, dstline a3, carry d1,
       bits d2, maskptr a5, count d4, plane d6, lines d7, bitno d3.b,
       cols d5.b, value d0 */
    ULONG *bitptr, *destptr, copysrc, pos, dstline, carry, bits;
    UWORD *maskptr, count, plane, rv;
    WORD c, b, lines;
    BYTE bitno, cols;

    if (!(UWORD)rez)
        return value;
    res_value = (UWORD)rez;
    res_value2 = (UWORD)(rez - 1);
    res_value3 = (UWORD)(rez - 2);
    height = (UWORD)(h - 1);
    sbitdepth = (UWORD)((sbitdepth & 0xff00) | BM_DEPTH(src));  /* BUG(orig) (#675) */
    sbitdepth--;
    store_bitplanes(src, dst, sadd, dadd);
    value = (value & 0xffff0000UL) | 0xffff;    /* DBRA d0 of store_bitplanes */

    if (!res_value2) {
        screencopy();
        return value;
    }
    if (res_value == 2)
        return unrezx2();

    add_value = bl_mulu_w(40, res_value);
    yvalue = height;

    /* myline: one word per 16 pixels, a bit at the first pixel of every
       block.  BUG(orig) (#671): 20 (21) words, see the head of the file. */
    maskptr = myline;
    c = 19;
    b = 15;
    rv = res_value;
    bits = 0;
widthloop:
    bits |= 1UL << (b & 31);                    /* BSET d1,d2 */
    b = (WORD)(b - rv);
    if (b >= 0)
        goto widthloop;
    *maskptr++ = (UWORD)bits;
    bits = 0;
    b = (WORD)(b + 16);
    if (b < 0)
        goto moretodo;
    if (c-- != 0)
        goto widthloop;
    goto yloop;
moretodo:
    *maskptr++ = 0;
    c = (WORD)(c - 1);
    if (c < 0)
        goto yloop;
    b = (WORD)(b + 16);
    if (b < 0)
        goto moretodo;
    if (c-- != 0)
        goto widthloop;

yloop:
    value = (UWORD)c;                           /* d0 = $0000FFFF */
    for (;;) {
        bitptr = bit_add;
        destptr = dest_add;
        plane = sbitdepth;
        do {                                    /* bitplane_loop */
            lines = (WORD)yvalue;
            pos = *bitptr;
            dstline = *destptr;
            carry = 0;
            if (res_value2 <= 16) {
                UWORD col = 19;
                maskptr = myline;
                do {                            /* xloop */
                    value = W_(pos) & *maskptr++;
                    pos += 2;
                    bits = value;
                    count = res_value3;
                    do {
                        bits = (bits >> 1) | (bits << 31); /* ROR.l #1 */
                        value |= bits;
                    } while (count-- != 0);
                    value = (value & 0xffff0000UL) | (UWORD)(value | carry);
                    W_(dstline) = (UWORD)value;
                    dstline += 2;
                    value = BL_SWAP(value);
                    carry = (UWORD)value;
                } while (col-- != 0);
                goto done_line;
            }
            /* bit mode: every block bit by bit */
            bitno = 15;
            cols = 19;
        bitxloop:
            count = res_value2;
            value = (value & 0xffff0000UL) | W_(pos);
            if (!(value & (1UL << (bitno & 31))))
                goto clearbits;
            bits = W_(dstline);
        setbits_loop:
            bits |= 1UL << (bitno & 31);
            bitno = (BYTE)(bitno - 1);
            if (bitno < 0)
                goto setbits_word;
            if (count-- != 0)
                goto setbits_loop;
            goto next_bit;
        setbits_word:
            pos += 2;
            W_(dstline) = (UWORD)bits;
            dstline += 2;
            bits = W_(dstline);
            bitno = 15;
            cols = (BYTE)(cols - 1);
            if (cols < 0)
                goto done_line;
            if (count-- != 0)
                goto setbits_loop;
            goto next_bit;
        clearbits:
            bits = W_(dstline);
        clearbits_loop:
            bits &= ~(1UL << (bitno & 31));
            bitno = (BYTE)(bitno - 1);
            if (bitno < 0)
                goto clearbits_word;
            if (count-- != 0)
                goto clearbits_loop;
            goto next_bit;
        clearbits_word:
            pos += 2;
            W_(dstline) = (UWORD)bits;
            dstline += 2;
            bits = W_(dstline);
            bitno = 15;
            cols = (BYTE)(cols - 1);
            if (cols < 0)
                goto done_line;
            if (count-- != 0)
                goto clearbits_loop;
        next_bit:
            W_(dstline) = (UWORD)bits;
            goto bitxloop;

        done_line:                              /* the line downwards */
            count = res_value3;
            copysrc = *destptr;
            pos = copysrc + 40;
            for (;;) {
                lines = (WORD)(lines - 1);
                if (lines < 0)
                    goto out_of_screen;
                COPY40(copysrc, pos);
                if (count-- == 0)
                    break;
            }
            *bitptr += add_value;
        out_of_screen:
            bitptr++;
            *destptr++ = pos;
        } while (plane-- != 0);
        yvalue = (UWORD)(yvalue - res_value);
        if ((WORD)yvalue < 0)
            break;
    }
    return value;
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
    /* bitptr a0, destptr a6, copysrc a1, pos a2, dstline a3, srcbits d0,
       dstbits d2, count d4, plane d6, lines d7, srcbit d1.b, dstbit d3.b,
       cols d5.b */
    ULONG *bitptr, *destptr, copysrc, pos, dstline, srcbits, dstbits;
    UWORD count, plane;
    WORD lines;
    BYTE srcbit, dstbit, cols;

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
        bitptr = bit_add;
        destptr = dest_add;
        plane = sbitdepth;
        do {                                    /* bitplane_loop */
            lines = (WORD)yvalue;
            pos = *bitptr;
            dstline = *destptr;
            cols = 9;
            dstbit = 31;
            srcbit = 31;
        xloop:
            count = xzoom_value;
            dstbits = L_(dstline);
            srcbits = L_(pos);
            if (!(srcbits & (1UL << (srcbit & 31))))
                goto clearbits_loop;
        setbits_loop:
            dstbits |= 1UL << (dstbit & 31);
            dstbit = (BYTE)(dstbit - 1);
            if (dstbit < 0)
                goto setbits_word;
            if (count-- != 0)
                goto setbits_loop;
            goto next_bit;
        setbits_word:
            L_(dstline) = dstbits;
            dstline += 4;
            dstbits = L_(dstline);
            dstbit = 31;
            cols = (BYTE)(cols - 1);
            if (cols < 0)
                goto done_line;
            if (count-- != 0)
                goto setbits_loop;
            goto next_bit;
        clearbits_loop:
            dstbits &= ~(1UL << (dstbit & 31));
            dstbit = (BYTE)(dstbit - 1);
            if (dstbit < 0)
                goto clearbits_word;
            if (count-- != 0)
                goto clearbits_loop;
            goto next_bit;
        clearbits_word:
            L_(dstline) = dstbits;
            dstline += 4;
            dstbits = L_(dstline);
            dstbit = 31;
            cols = (BYTE)(cols - 1);
            if (cols < 0)
                goto done_line;
            if (count-- != 0)
                goto clearbits_loop;
        next_bit:
            L_(dstline) = dstbits;
            srcbit = (BYTE)(srcbit - 1);
            if (srcbit >= 0)
                goto xloop;
            srcbit = 31;
            pos += 4;
            goto xloop;

        done_line:                              /* the line downwards */
            count = yzoom_value;
            if (!count) {
                pos = *destptr + 40;            /* yzoom1 */
            } else {
                count--;
                copysrc = *destptr;
                pos = copysrc + 40;
                for (;;) {
                    lines = (WORD)(lines - 1);
                    if (lines < 0)
                        break;
                    COPY40(copysrc, pos);
                    if (count-- == 0)
                        break;
                }
            }
            *bitptr++ += 40;                    /* out_of_screen */
            *destptr++ = pos;
        } while (plane-- != 0);
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
