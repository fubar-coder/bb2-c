/*
 * ridisplaylib - RWE userlib 29 (#ridisplaylib, Reflective Images, "RI
 * DisplayLib"): the start of a display library that builds its own
 * copper list -
 *   InitView(height,depth,numsprites,numcolours)   allocates a copper
 *                       list in chip memory and writes its first four
 *                       moves; returns its address
 *   ViewBitmap(bitmap)  no code
 * init opens graphics.library 39 (the base is not used), finit closes it
 * and frees the last copper list.  Nothing is shown: the library never
 * installs the list, it does not touch the display.
 *
 * C translation of rwelibsrc/mydisplaylib.bb; reference: the reference
 * build of that source with blitz2 2.10 (REFERENCE refbuild,
 * ref/ridisplaylib.obj) - no binary of the library exists.  Notes:
 * docs/rwelibs/port/ridisplaylib.md.
 *
 * The variables viewsize and cop are kept together in the original's
 * order (the test finds them by the address InitView returns).  No
 * assembler besides the thunks.
 *
 * BUG(orig):
 *   - Every InitView allocates a new list; only the last one is freed (by
 *     finit, and only if graphics.library was opened) (#891).
 * Fixed:
 *   - InitView tests AllocMem: without chip memory it returns 0, writes
 *     nothing and keeps the list before; finit frees a list only if
 *     there is one (#888).  The original writes the moves to address 0
 *     (over ExecBase at 4) and finit calls FreeMem(0, size).
 *   - The four copper moves are the register moves DIWSTRT $2C81, DIWSTOP
 *     $2CC1, DDFSTRT $0038, DDFSTOP $00D0 (#889).  The original's
 *     !cmove{diwstrt,$2c81} is MOVE.w diwstrt,(a0)+:MOVE.w $2c81,(a0)+:
 *     the register name is its offset ($8E) and the value has no #, so
 *     the list got the words at $8E, $2C81, $90, $2CC1, $92, $38, $94,
 *     $D0 (low memory and the vector table; the odd addresses are an
 *     address error on a 68000).
 *   - InitView's size is 16*numsprites + 16*depth + 8*numcolours + 38, as
 *     a long (#890).  The original's MOVE.w d0,d3 overwrites the colour
 *     count with the height before it is read: its size was 8*height
 *     instead of 8*numcolours, computed as a word.
 */
#include "blitzlib.h"
#include "blitzexec.h"

/* entry points (thunks, below) */
void _initview(void), _viewbitmap(void), ridisplaylib_codeinit(void);
void ridisplaylib_codefinit(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !libheader{#ridisplaylib,init,0,finit,0} */
BLITZ_LIBHEADER(LIB_ridisplaylib, init, BL_NORET, finit, 0);

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_initview, 0, 0);
BLITZ_NAME("InitView", "height,depth,numsprites,numcolours");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS(LIB_bitmaplib, BL_LA0|BL_PD0);
BLITZ_SUBS(_viewbitmap, 0, 0);
BLITZ_NAME("ViewBitmap", "bitmap");

BLITZ_NULLSUB_AT(init, ridisplaylib_codeinit, 0, 0);
BLITZ_NULLSUB_AT(finit, ridisplaylib_codefinit, 0, 0);

BLITZ_LIBFIN();

/* ------------------------------------------------------------- the data */

static struct BL_PACKED {
    ULONG viewsize;                     /* the size of the last list */
    UWORD *cop;                         /* the last list */
} vd = { 0, 0 };

static struct Library *gfxbase;

/**
 * @brief init: open graphics.library 39.
 *
 * The base is only used by finit (to know that the library is open).
 * Returns nothing (BL_NORET).
 */
static void BLITZ_ROUTINE codeinit_c(void)
{
    gfxbase = OpenLibrary((CONST_STRPTR)"graphics.library", 39);
}

/**
 * @brief finit: close graphics.library and free the last copper list.
 *
 * Nothing happens if init could not open the library.  Only the list of
 * the last InitView that got one is freed, the earlier ones are lost
 * (BUG(orig) (#891)); no list, nothing freed (fixed (#888): the original
 * calls FreeMem(0, size) after a failed AllocMem).
 */
static void BLITZ_ROUTINE codefinit_c(void)
{
    if (!gfxbase)
        return;
    CloseLibrary(gfxbase);
    gfxbase = NULL;
    if (!vd.cop)
        return;
    FreeMem(vd.cop, vd.viewsize);
    vd.viewsize = 0;
    vd.cop = NULL;
}

/**
 * @brief InitView height,depth,numsprites,numcolours: allocate a copper
 * list in chip memory and write its first four moves.
 *
 * The size is 16*numsprites + 16*depth + 8*numcolours + 38, as a long
 * (fixed (#890): the original's MOVE.w d0,d3 overwrites the colour count
 * with the height before it is read, so its size is 8*height instead of
 * 8*numcolours, computed as a word and sign-extended).  The height is not
 * part of the size.  The four moves are DIWSTRT $2C81, DIWSTOP $2CC1,
 * DDFSTRT $0038, DDFSTOP $00D0 (fixed (#889): the original's `!cmove`s
 * read register and value from those absolute addresses, an address
 * error on a 68000 at the odd ones).  Without chip memory InitView
 * returns 0, writes nothing and keeps the list before (fixed (#888): the
 * original does not test AllocMem, the words go to address 0 and finit
 * calls FreeMem(0, size)).  Every call allocates a new list; the old one
 * is forgotten (BUG(orig) (#891)).  The list is never installed.
 * @param height   d0.w: the height (not used)
 * @param depth    d1.w: the depth (16 bytes per plane)
 * @param sprites  d2.w: the number of sprites (16 bytes each)
 * @param colours  d3.w: the number of colours (8 bytes each)
 * @return d0: the address of the copper list (0 without chip memory)
 */
static ULONG BLITZ_ROUTINE initview_c(UWORD height BL_REG(d0), UWORD depth BL_REG(d1),
                                      UWORD sprites BL_REG(d2), UWORD colours BL_REG(d3))
{
    ULONG size;
    UWORD *p;

    (void)height;
    /* sprites*16 + depth*8 + colours*8 + depth*8 (colour setups) + 38,
       as a long (#890) */
    size = ((ULONG)sprites << 4) + ((ULONG)depth << 4) + ((ULONG)colours << 3) + 38;
    p = AllocMem(size, MEMF_CHIP);
    if (!p)
        return 0;                                       /* (#888) */
    vd.viewsize = size;
    vd.cop = p;
    /* !cmove{diwstrt,$2c81} ...: the register moves (#889) */
    *p++ = 0x008e;                                      /* DIWSTRT */
    *p++ = 0x2c81;
    *p++ = 0x0090;                                      /* DIWSTOP */
    *p++ = 0x2cc1;
    *p++ = 0x0092;                                      /* DDFSTRT */
    *p++ = 0x0038;
    *p++ = 0x0094;                                      /* DDFSTOP */
    *p++ = 0x00d0;
    return (ULONG)vd.cop;
}

/**
 * @brief ViewBitmap bitmap: no code (RTS).
 *
 * a0 = the bitmap object (!libs #bitmaplib) is not used; d0 stays
 * unchanged.
 */
static void BLITZ_ROUTINE nothing_c(void)
{
}

/* Registers: the original changes d0, d1, a0, a1, a6 (init, finit) and
   d0-d6/a0/a1/a6 (InitView); the thunks keep d1/a0/a1 (and d0 for
   ViewBitmap, which changes nothing), the C code d2-d7/a2-a6. */
BLITZ_THUNK(ridisplaylib_codeinit, codeinit_c, "d0-d1/a0-a1");
BLITZ_THUNK(ridisplaylib_codefinit, codefinit_c, "d0-d1/a0-a1");
BLITZ_THUNK(_initview, initview_c, "d1/a0-a1");
BLITZ_THUNK(_viewbitmap, nothing_c, "d0-d1/a0-a1");
