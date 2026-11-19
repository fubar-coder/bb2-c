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
 *   - Derez with a value >= 3 builds its mask table myline with 20 words
 *     (21 for values > 31) into the 18 words the source reserves: the
 *     19th goes over myflag and the pad byte, the 20th (21st) over the
 *     first instruction(s) of ZoomXY (_AsmZoom: TST.w d0, BEQ ...).  DIFF:
 *     here myline has room for 21 words, ZoomXY stays intact - corrupting
 *     code cannot be translated (the values read back are the same)
 *     (#671);
 *   - finit frees the tables but keeps the pointers (#673).
 * Fixed (intended deviations):
 *   - ClearBitmap, FadeInBitmap and ZoomyZoom read the depth as the byte
 *     bm_Depth; the original read the word at 4 (bm_Flags:bm_Depth),
 *     right only while bm_Flags is 0 (#670);
 *   - ClearBitmap starts at the last word of the area (end-2, as its
 *     comment says); the original started at end-4, so every pass was one
 *     word too low, the last word of the area stayed and the last pass
 *     cleared the word before the area (before the bitplane for offset 0)
 *     (#668);
 *   - With runtime error checking every command that counts the planes of
 *     a bitmap (ClearBitmap, FadeInBitmap, ZoomX2, ZoomX4, ZoomX8,
 *     ReduceX2, ZoomyZoom, Derez, ZoomXY) refuses a bitmap of depth 0 or
 *     more than 8: "Illegal BitMap Depth" (the original has no error check
 *     area).  The release code is unchanged (invalid arguments, D-005;
 *     the manual: "the maximum bitmap depth for these functions is 8"):
 *     depth 0 runs the DBRA loops 65536 times (a word: ClearBitmap,
 *     FadeInBitmap, ZoomyZoom, Derez and ZoomXY, whose store_bitplanes
 *     writes 65536 plane pointers over bit_add/dest_add and what follows
 *     them) or 256 times (a byte: the zooms and ReduceX2, through the plane
 *     pointers behind the bitmap object) (#947); the pointer arrays
 *     bit_add/dest_add hold 16 longs each: FadeInBitmap (two pointers per
 *     plane in each) runs from bit_add into dest_add and from dest_add
 *     over the four variables behind it from 9 planes on, and from 11
 *     planes on over what follows them; ClearBitmap, Derez and ZoomXY (one
 *     pointer per plane) reach the variables from 17 planes on and what
 *     follows from 21 on.  What follows is the code of InitZoomXY in the
 *     original, other variables here (one array pa[] with the four
 *     variables in the same order) (#674);
 *   - With runtime error checking ClearBitmap and FadeInBitmap refuse a
 *     delay of 0: "Delay must not be 0".  The release code is unchanged
 *     (the manual: "don't pass 0"; D-005): the busy loop decrements before
 *     its test (SUBQ.l #1 / BNE), so 0 means 2^32 turns per line (#669);
 *   - With runtime error checking ZoomyZoom refuses a bitmap smaller than
 *     640 x 32 (fewer than 80 bytes per row or 32 rows): "ZoomyZoom needs
 *     a bitmap of at least 640 x 32".  The release code is unchanged
 *     (D-005): ZoomyZoom ignores the bitmap's size and always doubles 32
 *     lines of 80 bytes in place (the left 40 bytes of each), so it fits
 *     640 x 32 only: on a narrower bitmap the lines are not its rows, on a
 *     smaller one it writes behind the plane (#676);
 *   - Derez, declared with a result (!acommand{#long}), returns -1 when it
 *     worked and 0 when it did nothing (derez value 0); the original set
 *     none, d0 was what the routine left there (its d0 on entry for 0, the
 *     upper word of it and $FFFF for 1, the last long of UnrezX2 for 2, a
 *     mask or source word for the others) (#672; W6 decision Q3);
 *   - Derez and ZoomXY load the depth into sbitdepth as a whole word; the
 *     original set only its low byte (MOVE.b into sbitdepth+1), so after a
 *     depth 0 the upper byte stayed $FF for every later call (#675; seen
 *     only after a depth-0 call, which itself overruns the arrays).
 * Not bugs but limits the source names: ZoomXY, Derez (and their
 * ScreenCopy) only for lores bitmaps (40 bytes per line).
 */
#include "blitzlib.h"
#include <stddef.h>
#include <graphics/gfx.h>

/* entry points (C functions and thunks, defined below) */
static void BLITZ_ROUTINE clearbitmap2(ULONG delay BL_REG(d1), ULONG bm BL_REG(a0));
static void BLITZ_ROUTINE clearbitmap(ULONG delay BL_REG(d1), ULONG h BL_REG(d2),
                                      ULONG off BL_REG(d3), ULONG bm BL_REG(a0));
static void BLITZ_ROUTINE fadeinbitmap2(ULONG delay BL_REG(d2), ULONG src BL_REG(a0),
                                        ULONG dst BL_REG(a1));
static void BLITZ_ROUTINE fadeinbitmap(ULONG delay BL_REG(d2), ULONG h BL_REG(d3),
                                       ULONG off1 BL_REG(d4), ULONG off2 BL_REG(d5),
                                       ULONG src BL_REG(a0), ULONG dst BL_REG(a1));
void zoomxy_no_xy(void), zoomx4_xy(void), asmderez(void), AsmZoom(void);
void InitZoomxy(void), zoomx2(void), zoomtables(void), zoomx8(void);
void ADDValue(void), ReduceX2(void), dozoomy(void), rifxlib_libinit(void);
void rifxlib_libfinit(void);
void clearchk(void), fadechk(void), bm2chk(void), zoomxychk(void), zoomychk(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* #ia0|#pd0: the bitmap that parameter `p` names, into address register
   a`r` (the source's macro !bmap{r,p}) */
#define RI_BMAP(r, p) LIB_bitmaplib, BL_LA(r) | BL_ITEM | BL_PD(p)
/* source# and dest#: the bitmaps of parameters 0 and 1 into a0 and a1 */
#define RI_BMAPS() BLITZ_LIBS(RI_BMAP(0, 0), RI_BMAP(1, 1))

/* !libheader {#rifxlib,init,0,finit,0}; error field: the error check
   area of the depth checks (fixed, #947, #674) */
BLITZ_LIBHEADER(LIB_rifxlib, init, BL_NORET, finit, BLITZ_ERROR_AREA);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS(RI_BMAP(0, 0));
BLITZ_SUBS(clearbitmap2, clearchk, 0);           /* check: fixed #947 #674 #669 */
BLITZ_ARGS(BL_LONG, BL_LONG, BL_WORD, BL_WORD);
BLITZ_LIBS(RI_BMAP(0, 0));
BLITZ_SUBS(clearbitmap, clearchk, 0);            /* check: fixed #947 #674 #669 */
BLITZ_NAME("ClearBitmap", "bitmap#,delay[,height,offset]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_LONG, BL_LONG);
RI_BMAPS();
BLITZ_SUBS(fadeinbitmap2, fadechk, 0);           /* check: fixed #947 #674 #669 */
BLITZ_ARGS(BL_LONG, BL_LONG, BL_LONG, BL_WORD, BL_WORD, BL_WORD);
RI_BMAPS();
BLITZ_SUBS(fadeinbitmap, fadechk, 0);            /* check: fixed #947 #674 #669 */
BLITZ_NAME("FadeInBitmap", "bitmapsource#,bitmapdest#,delay[,height,offset1,offset2]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_BYTE, BL_BYTE, BL_LONG, BL_LONG, BL_WORD, BL_WORD);
RI_BMAPS();
BLITZ_SUBS(zoomxy_no_xy, bm2chk, 0);             /* check: fixed #947 #674 */
BLITZ_ARGS(BL_BYTE, BL_BYTE, BL_LONG, BL_LONG, BL_WORD, BL_WORD, BL_BYTE);
RI_BMAPS();
BLITZ_SUBS(zoomx4_xy, bm2chk, 0);                /* check: fixed #947 #674 */
BLITZ_NAME("ZoomX4", "source#,dest#,sourceadd,destadd,width,height[,xy_select]");

/* the result: -1 when it worked, 0 for derez value 0 (fixed #672: the
   original declared a result and set none, see asmderez) */
BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG, BL_LONG, BL_LONG, BL_LONG, BL_WORD, BL_WORD);
RI_BMAPS();
BLITZ_SUBS(asmderez, bm2chk, 0);                 /* check: fixed #947 #674 */
BLITZ_NAME("Derez", "source#,dest#,sourceadd,destadd,derez_value,height");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(AsmZoom, zoomxychk, 0);                /* check: fixed #947 #674 */
BLITZ_NAME("ZoomXY", "xzoom,yzoom,height");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_LONG, BL_LONG, BL_LONG);
RI_BMAPS();
BLITZ_SUBS(InitZoomxy, 0, 0);
BLITZ_NAME("InitZoomXY", "source#,dest#,sourceadd,destadd");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_LONG, BL_LONG, BL_LONG, BL_WORD, BL_WORD);
RI_BMAPS();
BLITZ_SUBS(zoomx2, bm2chk, 0);                    /* check: fixed #947 #674 */
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
BLITZ_SUBS(zoomx8, bm2chk, 0);                    /* check: fixed #947 #674 */
BLITZ_NAME("ZoomX8", "source#,dest#,sourceadd,destadd,width,height");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_BYTE, BL_WORD, BL_WORD);
BLITZ_LIBS(RI_BMAP(0, 0));
BLITZ_SUBS(ADDValue, 0, 0);
BLITZ_NAME("ADDValue", "(bitmap#,x,y)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_BYTE, BL_BYTE, BL_LONG, BL_LONG, BL_WORD, BL_WORD);
RI_BMAPS();
BLITZ_SUBS(ReduceX2, bm2chk, 0);                  /* check: fixed #947 #674 */
BLITZ_NAME("ReduceX2", "source#,dest#,sourceadd,destadd,width,height");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(RI_BMAP(0, 0));
BLITZ_SUBS(dozoomy, zoomychk, 0);                 /* check: fixed #947 #674 #676 */
BLITZ_NAME("ZoomyZoom", "bitmap#");

BLITZ_NULLSUB_AT(finit, rifxlib_libfinit, 0, 0);
BLITZ_NULLSUB_AT(init, rifxlib_libinit, 0, 0);
BLITZ_LIBFIN();

/* ------------------------------------------------------------ memory */

/* memory as the registers see it: addresses are ULONGs */
#define B_(a) (*(UBYTE *)(a))
#define W_(a) (*(UWORD *)(a))
#define L_(a) (*(ULONG *)(a))
/* the bitmap (a0/a1, an address): struct BitMap - BytesPerRow +0, Rows
   +2, Flags +4, Depth +5, Planes +8 */
#define BM(bm)        ((struct BitMap *)(bm))
#define BM_BPR(bm)    (BM(bm)->BytesPerRow)
#define BM_ROWS(bm)   (BM(bm)->Rows)
/* Depth as a byte at its offset: as a member access GCC orders the loads
   of Derez/ZoomXY differently (the code must stay the same) */
#define BM_DEPTH(bm)  B_((bm) + offsetof(struct BitMap, Depth))
#define BM_PLANES(bm) ((ULONG)BM(bm)->Planes)
static_assert(sizeof(struct BitMap) == 40);

/* ------------------------------------------------------------ variables */

static UWORD EWWidth, BHeight, BDepth;
static ULONG delay_timer;

/* bytesperline/bytesperline2: longs of which only the low word is ever
   written (MOVE.w d,bytesperline+2), so their value is that word */
static UWORD bytesperline, bytesperline2;
static UWORD xzoom = 1, yzoom = 1;

static ULONG tableaddress;
/* the original's longs reducex2table, zoomx2table, zoomx4table,
   zoomx8table: GetZoomTables returns the address of the four */
static struct {
    ULONG reducex2, zoomx2, zoomx4, zoomx8;
} tables;
static_assert(sizeof tables == 4 * 4);

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
   add_source, add_dest - one block: the stores of the plane pointers do
   not check the depth in the release code and run from one into the next
   (the error checks refuse such a depth: fixed #674, #947), so they go
   through `all`, the block as one array */
static union {
    ULONG all[36];
    struct {
        ULONG bit_add[16], dest_add[16];
        ULONG source_address, dest_address, add_source, add_dest;
    } v;
} pa;
static_assert(sizeof pa == 36 * 4);
#define bit_add        (&pa.all[0])
#define dest_add       (&pa.all[16])
#define source_address pa.v.source_address
#define dest_address   pa.v.dest_address
#define add_source     pa.v.add_source
#define add_dest       pa.v.add_dest

/* ------------------------------------------------------------ init/finit */

#define TABLESIZE 1856

/**
 * @brief finit: frees the zoom tables (1856 bytes, memlib globfree).
 *
 * Nothing if init could not allocate them.  BUG(orig) (#673): the table
 * pointers stay set after the free.
 */
static void BLITZ_ROUTINE libfinit_c(void)
{
    struct bl_regs r;

    if (!tableaddress)
        return;
    r = (struct bl_regs){ .d0 = TABLESIZE, .a1 = tableaddress };
    BLITZ_ALIBJSR(TOK_globfree, r);     /* BUG(orig) (#673): the pointers stay */
}
BLITZ_THUNK(rifxlib_libfinit, libfinit_c, "d1/a0-a1");

/**
 * @brief init: allocates the zoom tables (memlib globalloc, 1856 bytes
 * MEMF_PUBLIC) and fills them.
 *
 * reducex2: 256 bytes, bits 7,5,3,1 to bits 3-0; zoomx2: 256 words, every
 * bit twice; zoomx4: 256 longs, every bit four times; zoomx8: 16 longs,
 * four bits eight times each.  Without memory the pointers stay 0 and the
 * zooms that need a table do nothing.
 */
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
    tables.reducex2 = table;
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
    tables.zoomx2 = table;
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
    tables.zoomx4 = table;
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
    tables.zoomx8 = table;
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
BLITZ_THUNK(rifxlib_libinit, libinit_c, "d1/a0-a1");

/**
 * @brief GetZoomTables: the address of the four table pointers.
 *
 * The longs reducex2, zoomx2, zoomx4, zoomx8 (0 before init or if init
 * found no memory).
 * @return d0: the address of the pointer block
 */
static ULONG BLITZ_ROUTINE zoomtables_c(void)
{
    return (ULONG)&tables;
}
BLITZ_THUNK(zoomtables, zoomtables_c, "d1/a0-a1");

/* ------------------------------------------------- ClearBitmap/FadeIn */

/* the delay: SUBQ.l #1,d7:BNE - 0 = 2^32 turns (clearchk and fadechk
   refuse it: fixed #669) */
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
    BDepth = (UWORD)(BM_DEPTH(bm) - 1);         /* the byte; fixed (#670) */
    BHeight = (UWORD)(h - 1);
    areaend = bl_mulu_w((UWORD)(h + off), BM_BPR(bm));
    areaend -= 2;                               /* fixed (#668): the original 4 */

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

/**
 * @brief ClearBitmap bitmap#,delay: clears the whole bitmap line by line
 * with a dither fade (see clearbitmap_c).
 *
 * Height bm_Rows, offset 0.  Delay 0 = 2^32 turns, more than 16 planes
 * overrun the pointer arrays, depth 0 runs 65536 planes: refused by
 * clearchk (fixed #669, #674, #947).  Fixed: the
 * depth is the byte bm_Depth (#670); the passes start at the last word
 * of the area, not one word lower (#668).
 * @param delay  d1: turns of the busy loop per line
 * @param bm     a0: the bitmap (struct BitMap) of bitmap# (d0)
 * Returns nothing (keeps a4-a6 only).
 */
static void BLITZ_ROUTINE clearbitmap2(ULONG delay BL_REG(d1), ULONG bm BL_REG(a0))
{
    clearbitmap_c(delay, BM_ROWS(bm), 0, bm);
}

/**
 * @brief ClearBitmap bitmap#,delay,height,offset: clears `height` lines
 * from line `offset` on with a dither fade.
 *
 * Backwards from the end of the area: each line is first ANDed with the
 * alternating mask $AAAA/$5555, 19 lines later cleared.  Delay 0 (2^32
 * turns), depth 0 or more than 8: refused by clearchk (fixed #669, #947,
 * #674).  Fixed: the passes start at the last word of the area (end-2);
 * the original started at end-4, one word too low: the last word of the
 * area stayed and the word before the area was cleared (#668).  Fixed
 * (#670): the depth is the byte bm_Depth (the original read the word
 * bm_Flags:bm_Depth).
 * @param delay  d1: turns of the busy loop per line
 * @param h      d2.w: the number of lines
 * @param off    d3.w: the first line
 * @param bm     a0: the bitmap of bitmap# (d0)
 * Returns nothing (keeps a4-a6 only).
 */
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
    BDepth = (UWORD)(BM_DEPTH(src) - 1);        /* the byte; fixed (#670) */
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

/**
 * @brief FadeInBitmap bitmapsource#,bitmapdest#,delay: fades the whole
 * source into the destination line by line (see fadein_c).
 *
 * Height bm_Rows of the source, offsets 0.  Delay 0 = 2^32 turns, more
 * than 8 planes overrun the pointer arrays, depth 0 runs 65536 planes:
 * refused by fadechk (fixed #669, #674, #947).
 * Fixed (#670): the depth is the byte bm_Depth.
 * @param delay  d2: turns of the busy loop per line
 * @param src    a0: the source bitmap (bitmapsource#, d0)
 * @param dst    a1: the destination bitmap (bitmapdest#, d1)
 * Returns nothing (keeps a4-a6 only).
 */
static void BLITZ_ROUTINE fadeinbitmap2(ULONG delay BL_REG(d2), ULONG src BL_REG(a0),
                                        ULONG dst BL_REG(a1))
{
    fadein_c(delay, BM_ROWS(src), 0, 0, src, dst);
}

/**
 * @brief FadeInBitmap bitmapsource#,bitmapdest#,delay,height,offset1,offset2:
 * fades `height` source lines into the destination with a dither.
 *
 * Forwards: each line is first copied through the alternating mask
 * $AAAA/$5555, 19 lines later copied whole.  Width, depth and both line
 * offsets come from the source bitmap (offsets times the SOURCE's bytes
 * per row).  Fixed (#670): the depth is the byte bm_Depth (the original
 * read the word bm_Flags:bm_Depth).  Delay 0 = 2^32 turns, from 9 planes
 * on the two pointers per plane overrun bit_add/dest_add, depth 0 runs
 * 65536 planes: refused by fadechk (fixed #669, #674, #947).
 * @param delay  d2: turns of the busy loop per line
 * @param h      d3.w: the number of lines
 * @param off1   d4.w: the first source line
 * @param off2   d5.w: the first destination line
 * @param src    a0: the source bitmap (d0)
 * @param dst    a1: the destination bitmap (d1)
 * Returns nothing (keeps a4-a6 only).
 */
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
    ULONG smod, dmod, from, line0, line1, line2, line3, tab = tables.zoomx4;
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

/**
 * @brief ZoomX4 source#,dest#,sourceadd,destadd,width,height: every source
 * pixel becomes 4x4 pixels (zoomx4 table, see zoomx4_c).
 *
 * Nothing without the table (init found no memory), with a width below 8
 * or a height below 1.  The number of planes is the source's bm_Depth (a
 * byte: 0 = 256; bm2chk refuses 0 and more than 8: fixed #947, #674).
 * @param srcadd  d2: byte offset added to every source plane
 * @param dstadd  d3: byte offset added to every destination plane
 * @param width   d4.w: the width in pixels (ASR.w #3: whole bytes)
 * @param h       d5.w: the number of source lines
 * @param src     a0: the source bitmap (source#, d0)
 * @param dst     a1: the destination bitmap (dest#, d1)
 * Returns nothing (all registers kept).
 */
static void BLITZ_ROUTINE zoomx4_6_c(ULONG srcadd BL_REG(d2), ULONG dstadd BL_REG(d3),
                                     ULONG width BL_REG(d4), ULONG h BL_REG(d5),
                                     ULONG src BL_REG(a0), ULONG dst BL_REG(a1))
{
    if (!tables.zoomx4)
        return;
    zoomx4_c(0, srcadd, dstadd, (UWORD)width, (UWORD)h, src, dst);   /* CLR.b d7 */
}

/**
 * @brief ZoomX4 source#,dest#,sourceadd,destadd,width,height,xy_select:
 * as ZoomX4 above, in one direction only if asked.
 *
 * xy_select 1 = only x (4 times as wide), 2 = only y (4 times as high),
 * anything else both.
 * @param srcadd  d2: byte offset added to every source plane
 * @param dstadd  d3: byte offset added to every destination plane
 * @param width   d4.w: the width in pixels
 * @param h       d5.w: the number of source lines
 * @param src     a0: the source bitmap (d0)
 * @param dst     a1: the destination bitmap (d1)
 * @param args    a2: the argument stack, xy_select the byte at -1(a2)
 * Returns nothing (all registers kept).
 */
static void BLITZ_ROUTINE zoomx4_7_c(ULONG srcadd BL_REG(d2), ULONG dstadd BL_REG(d3),
                                     ULONG width BL_REG(d4), ULONG h BL_REG(d5),
                                     ULONG src BL_REG(a0), ULONG dst BL_REG(a1),
                                     const UBYTE *args BL_REG(a2))
{
    if (!tables.zoomx4)
        return;
    zoomx4_c(args[-1], srcadd, dstadd, (UWORD)width, (UWORD)h, src, dst);  /* -1(a2) */
}
/* the original keeps all registers (MOVEM.l d0-d7/a0-a6) */
BLITZ_THUNK(zoomxy_no_xy, zoomx4_6_c, "d0-d1/a0-a1");
BLITZ_THUNK(zoomx4_xy, zoomx4_7_c, "d0-d1/a0-a1");

/**
 * @brief ZoomX2 source#,dest#,sourceadd,destadd,width,height: every source
 * pixel becomes 2x2 pixels (zoomx2 table).
 *
 * Nothing without the table, with a width below 8 or a height below 1.
 * The number of planes is the source's bm_Depth (0 = 256;
 * bm2chk refuses 0 and more than 8: fixed #947, #674).
 * @param srcadd  d2: byte offset added to every source plane
 * @param dstadd  d3: byte offset added to every destination plane
 * @param width   d4.w: the width in pixels (ASR.w #3: whole bytes)
 * @param h       d5.w: the number of source lines
 * @param src     a0: the source bitmap (source#, d0)
 * @param dst     a1: the destination bitmap (dest#, d1)
 * Returns nothing (all registers kept).
 */
static void BLITZ_ROUTINE zoomx2_c(ULONG srcadd BL_REG(d2), ULONG dstadd BL_REG(d3),
                                   ULONG width BL_REG(d4), ULONG h BL_REG(d5),
                                   ULONG src BL_REG(a0), ULONG dst BL_REG(a1))
{
    ULONG srcplane = BM_PLANES(src), dstplane = BM_PLANES(dst);
    ULONG smod, dmod, from, line0, line1, tab = tables.zoomx2;
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

/**
 * @brief ZoomX8 source#,dest#,sourceadd,destadd,width,height: every source
 * pixel becomes 8x8 pixels (zoomx8 table).
 *
 * Every source line becomes 8 lines of 8 times the width (two passes of 4
 * lines).  Nothing without the table, with a width below 8 or a height
 * below 1.  The number of planes is the source's bm_Depth (0 = 256;
 * bm2chk refuses 0 and more than 8: fixed #947, #674).
 * @param srcadd  d2: byte offset added to every source plane
 * @param dstadd  d3: byte offset added to every destination plane
 * @param width   d4.w: the width in pixels (ASR.w #3: whole bytes)
 * @param h       d5.w: the number of source lines
 * @param src     a0: the source bitmap (source#, d0)
 * @param dst     a1: the destination bitmap (dest#, d1)
 * Returns nothing (all registers kept).
 */
static void BLITZ_ROUTINE zoomx8_c(ULONG srcadd BL_REG(d2), ULONG dstadd BL_REG(d3),
                                   ULONG width BL_REG(d4), ULONG h BL_REG(d5),
                                   ULONG src BL_REG(a0), ULONG dst BL_REG(a1))
{
    ULONG srcplane = BM_PLANES(src), dstplane = BM_PLANES(dst);
    ULONG smod, dmod, from, line0, line1, line2, line3, save, tab = tables.zoomx8;
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

/**
 * @brief ZoomyZoom bitmap#: doubles the left half of the bitmap in x, in
 * place (zoomx2 table).
 *
 * Fixed 32 lines of 80 bytes whatever the bitmap's size; the left 40
 * bytes of a line are doubled in place (from the right, so nothing is
 * read after it is written).  A bitmap smaller than 640 x 32 is refused
 * by zoomychk (fixed #676; the original's size is kept in the release
 * code).  Nothing without the table.  Fixed (#670): the depth is the
 * byte bm_Depth (the original read the word bm_Flags:bm_Depth); depth 0
 * (65536 planes) and more than 8 are refused by zoomychk (fixed #947,
 * #674).
 * @param bm  a0: the bitmap of bitmap# (d0)
 * Returns nothing (all registers kept).
 */
static void BLITZ_ROUTINE dozoomy_c(ULONG bm BL_REG(a0))
{
    ULONG planes = BM_PLANES(bm), line, tab = tables.zoomx2;
    UWORD plane, y;
    WORD srcx, dstx;

    if (!tab)
        return;
    plane = (UWORD)(BM_DEPTH(bm) - 1);          /* the byte; fixed (#670) */
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

/**
 * @brief ReduceX2 source#,dest#,sourceadd,destadd,width,height: halves the
 * size (reducex2 table).
 *
 * Every second line, two source bytes -> one (bits 7,5,3,1).  Nothing
 * without the table, with a width below 16 or a height below 2.  The
 * number of planes is the source's bm_Depth (0 = 256;
 * bm2chk refuses 0 and more than 8: fixed #947, #674).
 * @param srcadd  d2: byte offset added to every source plane
 * @param dstadd  d3: byte offset added to every destination plane
 * @param width   d4.w: the source width in pixels
 * @param h       d5.w: the number of source lines
 * @param src     a0: the source bitmap (source#, d0)
 * @param dst     a1: the destination bitmap (dest#, d1)
 * Returns nothing (all registers kept).
 */
static void BLITZ_ROUTINE reducex2_c(ULONG srcadd BL_REG(d2), ULONG dstadd BL_REG(d3),
                                     ULONG width BL_REG(d4), ULONG h BL_REG(d5),
                                     ULONG src BL_REG(a0), ULONG dst BL_REG(a1))
{
    ULONG srcplane = BM_PLANES(src), dstplane = BM_PLANES(dst);
    ULONG from, to, tab = tables.reducex2;
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

/* UnrezX2: every second pixel doubled, every second line doubled */
static void unrezx2(void)
{
    ULONG *bitptr = bit_add, *destptr = dest_add, from, to, pixels;
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
}

/**
 * @brief Derez source#,dest#,sourceadd,destadd,derez_value,height: lowers
 * the resolution of a lores bitmap (40 bytes per line).
 *
 * Blocks of derez_value pixels and lines get the colour of their first
 * pixel (1 = ScreenCopy, 2 = UnrezX2, up to 17 through the mask table
 * myline, above that bit by bit); derez_value 0 does nothing.  The
 * result: -1, 0 for derez_value 0 (fixed #672: the original set none, d0
 * was what the routine left there - the input for 0, its upper word and
 * $FFFF for 1, the last long for 2, else the last mask/source word).
 * Values >= 3 write 20 (21) words of myline into 18 (BUG(orig) #671,
 * DIFF: room for 21 here).  A
 * depth-0 source gives 65536 planes, more than 16 planes overrun the
 * pointer arrays: refused by bm2chk (fixed #947, #674).  sbitdepth is
 * set as a whole word (fixed #675: the original set its low byte only).
 * @param sadd      d2: byte offset added to every source plane
 * @param dadd      d3: byte offset added to every destination plane
 * @param rez       d4.w: derez_value, the block size
 * @param h         d5.w: the number of lines
 * @param src       a0: the source bitmap (d0)
 * @param dst       a1: the destination bitmap (d1)
 * @return d0: -1 when it worked, 0 for derez_value 0
 */
static ULONG BLITZ_ROUTINE asmderez_c(ULONG sadd BL_REG(d2),
                                      ULONG dadd BL_REG(d3), ULONG rez BL_REG(d4),
                                      ULONG h BL_REG(d5), ULONG src BL_REG(a0),
                                      ULONG dst BL_REG(a1))
{
    /* bitptr a0, destptr a6, copysrc a1, pos a2 (the source line),
       nextline a2, dstline a3, carry d1, mask/rot/dstword d2, maskptr a5,
       count d4, plane d6, lines d7, bitno d3.b, cols d5.b, value d0 */
    ULONG *bitptr, *destptr, copysrc, pos, nextline, dstline, carry;
    ULONG mask;                                 /* d2: a myline word being built */
    ULONG rot;                                  /* d2: the rotated copy of value */
    ULONG dstword;                              /* d2: the destination word */
    ULONG value;                                /* d0: the source word */
    UWORD *maskptr, count, plane, rv;
    WORD c, b, lines;
    BYTE bitno, cols;

    if (!(UWORD)rez)
        return 0;                       /* fixed (#672): the caller's d0 */
    res_value = (UWORD)rez;
    res_value2 = (UWORD)(rez - 1);
    res_value3 = (UWORD)(rez - 2);
    height = (UWORD)(h - 1);
    sbitdepth = BM_DEPTH(src);          /* the whole word: fixed (#675) */
    sbitdepth--;                        /* depth 0 = $FFFF: bm2chk (#947) */
    store_bitplanes(src, dst, sadd, dadd);

    if (!res_value2) {
        screencopy();
        return (ULONG)-1;               /* fixed (#672): no result set */
    }
    if (res_value == 2) {
        unrezx2();
        return (ULONG)-1;
    }

    add_value = bl_mulu_w(40, res_value);
    yvalue = height;

    /* myline: one word per 16 pixels, a bit at the first pixel of every
       block.  BUG(orig) (#671): 20 (21) words, see the head of the file. */
    maskptr = myline;
    c = 19;
    b = 15;
    rv = res_value;
    mask = 0;
widthloop:
    mask |= 1UL << (b & 31);                    /* BSET d1,d2 */
    b = (WORD)(b - rv);
    if (b >= 0)
        goto widthloop;
    *maskptr++ = (UWORD)mask;
    mask = 0;
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
                    rot = value;
                    count = res_value3;
                    do {
                        rot = (rot >> 1) | (rot << 31); /* ROR.l #1 */
                        value |= rot;
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
            dstword = W_(dstline);
        setbits_loop:
            dstword |= 1UL << (bitno & 31);
            bitno = (BYTE)(bitno - 1);
            if (bitno < 0)
                goto setbits_word;
            if (count-- != 0)
                goto setbits_loop;
            goto next_bit;
        setbits_word:
            pos += 2;
            W_(dstline) = (UWORD)dstword;
            dstline += 2;
            dstword = W_(dstline);
            bitno = 15;
            cols = (BYTE)(cols - 1);
            if (cols < 0)
                goto done_line;
            if (count-- != 0)
                goto setbits_loop;
            goto next_bit;
        clearbits:
            dstword = W_(dstline);
        clearbits_loop:
            dstword &= ~(1UL << (bitno & 31));
            bitno = (BYTE)(bitno - 1);
            if (bitno < 0)
                goto clearbits_word;
            if (count-- != 0)
                goto clearbits_loop;
            goto next_bit;
        clearbits_word:
            pos += 2;
            W_(dstline) = (UWORD)dstword;
            dstline += 2;
            dstword = W_(dstline);
            bitno = 15;
            cols = (BYTE)(cols - 1);
            if (cols < 0)
                goto done_line;
            if (count-- != 0)
                goto clearbits_loop;
        next_bit:
            W_(dstline) = (UWORD)dstword;
            goto bitxloop;

        done_line:                              /* the line downwards */
            count = res_value3;
            copysrc = *destptr;
            nextline = copysrc + 40;
            for (;;) {
                lines = (WORD)(lines - 1);
                if (lines < 0)
                    goto out_of_screen;
                COPY40(copysrc, nextline);
                if (count-- == 0)
                    break;
            }
            *bitptr += add_value;
        out_of_screen:
            bitptr++;
            *destptr++ = nextline;
        } while (plane-- != 0);
        yvalue = (UWORD)(yvalue - res_value);
        if ((WORD)yvalue < 0)
            break;
    }
    return (ULONG)-1;
}
/* the original keeps d1-d7/a0-a6 */
BLITZ_THUNK(asmderez, asmderez_c, "d1/a0-a1");

/**
 * @brief InitZoomXY source#,dest#,sourceadd,destadd: stores the bitmaps
 * and offsets for ZoomXY.
 * @param sadd  d2: byte offset added to every source plane
 * @param dadd  d3: byte offset added to every destination plane
 * @param src   a0: the source bitmap (source#, d0)
 * @param dst   a1: the destination bitmap (dest#, d1)
 * Returns nothing (all registers kept).
 */
static void BLITZ_ROUTINE initzoomxy_c(ULONG sadd BL_REG(d2), ULONG dadd BL_REG(d3),
                                       ULONG src BL_REG(a0), ULONG dst BL_REG(a1))
{
    source_address = src;
    dest_address = dst;
    add_source = sadd;
    add_dest = dadd;
}
BLITZ_THUNK(InitZoomxy, initzoomxy_c, "d0-d1/a0-a1");

/**
 * @brief ZoomXY xzoom,yzoom,height: zooms the bitmaps of InitZoomXY
 * (lores, 40 bytes per line) by whole factors.
 *
 * Every source pixel becomes xzoom pixels wide, every line yzoom lines
 * high; 1,1 = ScreenCopy; a factor 0 does nothing.  A depth-0 source
 * gives 65536 planes, more than 16 planes overrun the pointer arrays:
 * refused by zoomxychk (fixed #947, #674).  sbitdepth is set as a whole
 * word (fixed #675: the original set its low byte only).
 * @param xz  d0.w: xzoom
 * @param yz  d1.w: yzoom
 * @param h   d2.w: the number of lines
 * Returns nothing (keeps d1-d7/a0-a6, and d0 if it does nothing).
 */
static void BLITZ_ROUTINE asmzoom_c(ULONG xz BL_REG(d0), ULONG yz BL_REG(d1), ULONG h BL_REG(d2))
{
    /* bitptr a0, destptr a6, copysrc a1, pos a2 (the source), nextline
       a2, dstline a3, srcbits d0, dstbits d2, count d4, plane d6, lines
       d7, srcbit d1.b, dstbit d3.b, cols d5.b */
    ULONG *bitptr, *destptr, copysrc, pos, nextline, dstline, srcbits, dstbits;
    UWORD count, plane;
    WORD lines;
    BYTE srcbit, dstbit, cols;

    if (!(UWORD)xz || !(UWORD)yz)
        return;
    xzoom_value = (UWORD)(xz - 1);
    yzoom_value2 = (UWORD)yz;
    yzoom_value = (UWORD)(yz - 1);
    height = (UWORD)(h - 1);
    sbitdepth = BM_DEPTH(source_address); /* the whole word: fixed (#675) */
    sbitdepth--;                        /* depth 0 = $FFFF: zoomxychk (#947) */
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
                nextline = *destptr + 40;       /* yzoom1 */
            } else {
                count--;
                copysrc = *destptr;
                nextline = copysrc + 40;
                for (;;) {
                    lines = (WORD)(lines - 1);
                    if (lines < 0)
                        break;
                    COPY40(copysrc, nextline);
                    if (count-- == 0)
                        break;
                }
            }
            *bitptr++ += 40;                    /* out_of_screen */
            *destptr++ = nextline;
        } while (plane-- != 0);
        yvalue = (UWORD)(yvalue - yzoom_value2);
        if ((WORD)yvalue < 0)
            break;
    }
}
/* the original keeps d1-d7/a0-a6, and d0 if it does nothing */
BLITZ_THUNK(AsmZoom, asmzoom_c, "d0-d1/a0-a1");

/**
 * @brief ADDValue(bitmap#,x,y): the byte offset of pixel x,y in a plane.
 * @param x   d1.w: x (ASR.w #3: signed)
 * @param y   d2.w: the line
 * @param bm  a0: the bitmap of bitmap# (d0)
 * @return d0: y * bm_BytesPerRow + x/8 (changes d1)
 */
static ULONG BLITZ_ROUTINE addvalue_c(ULONG x BL_REG(d1), ULONG y BL_REG(d2),
                                      ULONG bm BL_REG(a0))
{
    return bl_mulu_w(BM_BPR(bm), (UWORD)y) + (UWORD)((WORD)x >> 3);
}
BLITZ_THUNK(ADDValue, addvalue_c, "a0-a1");

/* ------------------------------------------- error checks (debugger only) */

BLITZ_ERRCHK_MESSAGE(baddepth, "Illegal BitMap Depth");     /* fixed, #947 #674 */
BLITZ_ERRCHK_MESSAGE(baddelay, "Delay must not be 0");      /* fixed, #669 */
BLITZ_ERRCHK_MESSAGE(badzoomy, "ZoomyZoom needs a bitmap of at least 640 x 32"); /* fixed, #676 */

/* the depth of the bitmap `bm`: 1 to 8 (bitmaplib's checks and text) */
static void BLITZ_ERRCHK depthchk(ULONG bm)
{
    UBYTE depth = BM_DEPTH(bm);

    if (depth == 0 || depth > 8)
        BLITZ_RUNERR(baddepth);
}

/**
 * @brief The error check of ClearBitmap (both forms; debugger only).
 *
 * Fixed (#947, #674), not in the original (rifxlib had no error check
 * area): the depth of the bitmap was not checked - depth 0 ran the plane
 * loops 65536 times (DBRA on depth - 1 as a word), over the 16-entry
 * plane pointer arrays and what follows them, and so did more than 16
 * planes.  The manual: "the maximum bitmap depth for these functions is
 * 8".  Fixed (#669): the delay was not checked - 0 ran the busy loop
 * 2^32 times per line (SUBQ.l #1 before its test); the manual: "don't
 * pass 0".  The release code is unchanged (invalid arguments, D-005).
 *
 * Errors: "Illegal BitMap Depth" (depth 0 or more than 8), "Delay must
 * not be 0".
 * @param delay  d1: the delay (a long)
 * @param bm     a0: the bitmap of bitmap#
 */
static void BLITZ_ERRCHK clearchk_c(ULONG delay BL_REG(d1), ULONG bm BL_REG(a0))
{
    depthchk(bm);
    if (!delay)
        BLITZ_RUNERR(baddelay);
}

/**
 * @brief The error check of FadeInBitmap (both forms; debugger only).
 *
 * Fixed (#947, #674), not in the original: as clearchk_c for both
 * bitmaps; FadeInBitmap stores two pointers per plane in each array, so
 * the original overran them from 9 planes on; the delay as there (#669).
 *
 * Errors: "Illegal BitMap Depth" (depth 0 or more than 8, either bitmap),
 * "Delay must not be 0".
 * @param delay  d2: the delay (a long)
 * @param src    a0: the source bitmap
 * @param dst    a1: the destination bitmap
 */
static void BLITZ_ERRCHK fadechk_c(ULONG delay BL_REG(d2), ULONG src BL_REG(a0),
                                   ULONG dst BL_REG(a1))
{
    depthchk(src);
    depthchk(dst);
    if (!delay)
        BLITZ_RUNERR(baddelay);
}

/**
 * @brief The error check of ZoomX2, ZoomX4 (both forms), ZoomX8,
 * ReduceX2 and Derez (debugger only).
 *
 * Fixed (#947, #674), not in the original: the source's depth was not
 * checked - the zooms count it as a byte (depth 0 = 256 planes, through
 * the plane pointers behind the bitmap object), Derez as a word (65536
 * planes over the 16-entry plane pointer arrays and the 256 KB behind
 * them); more than 16 planes overran Derez's arrays.  The release code is
 * unchanged (D-005).
 *
 * Errors: "Illegal BitMap Depth" (depth 0 or more than 8, either bitmap).
 * @param src  a0: the source bitmap (source#)
 * @param dst  a1: the destination bitmap (dest#)
 */
static void BLITZ_ERRCHK bm2chk_c(ULONG src BL_REG(a0), ULONG dst BL_REG(a1))
{
    depthchk(src);
    depthchk(dst);
}

/**
 * @brief The error check of ZoomXY (debugger only): the bitmaps of
 * InitZoomXY.
 *
 * Fixed (#947, #674), not in the original: as bm2chk_c for the bitmaps
 * InitZoomXY stored (ZoomXY takes no bitmap itself).  Not checked: a
 * ZoomXY before any InitZoomXY (no bitmap stored).
 *
 * Errors: "Illegal BitMap Depth" (depth 0 or more than 8, either bitmap).
 */
static void BLITZ_ERRCHK zoomxychk_c(void)
{
    if (source_address)
        depthchk(source_address);
    if (dest_address)
        depthchk(dest_address);
}

/**
 * @brief The error check of ZoomyZoom (debugger only).
 *
 * Fixed (#947, #674), not in the original: the depth was not checked
 * (depth 0: 65536 planes through the plane pointers behind the bitmap
 * object).  Fixed (#676): nor was the size - ZoomyZoom works on 32 lines
 * of 80 bytes per plane whatever the bitmap is; a narrower bitmap is
 * garbled (the lines are not its rows), a smaller one overwritten behind
 * the plane.  The release code is unchanged (D-005; W6 decision Q5).
 *
 * Errors: "Illegal BitMap Depth" (depth 0 or more than 8), "ZoomyZoom
 * needs a bitmap of at least 640 x 32" (fewer than 80 bytes per row or
 * fewer than 32 rows).
 * @param bm  a0: the bitmap of bitmap#
 */
static void BLITZ_ERRCHK zoomychk_c(ULONG bm BL_REG(a0))
{
    depthchk(bm);
    if (BM_BPR(bm) < 80 || BM_ROWS(bm) < 32)
        BLITZ_RUNERR(badzoomy);
}

/* the thunks of the check functions: they keep every register */
BLITZ_ERRCHK_THUNK(clearchk, clearchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(fadechk, fadechk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(bm2chk, bm2chk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(zoomxychk, zoomxychk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(zoomychk, zoomychk_c, "d0-d1/a0-a1");
