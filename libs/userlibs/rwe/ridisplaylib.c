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
 *   - InitView: the colour count is lost (MOVE.w d0,d3 overwrites it with
 *     the height before it is read): the size is 16*numsprites +
 *     16*depth + 8*height + 38, computed as a word (#890).
 *   - The copper moves are read from absolute addresses: !cmove{diwstrt,
 *     $2c81} is MOVE.w diwstrt,(a0)+:MOVE.w $2c81,(a0)+, the register
 *     name is its offset ($8E) and the value has no #: the list gets the
 *     words at $8E, $2C81, $90, $2CC1, $92, $38, $94, $D0 - low memory
 *     and the vector table, two odd addresses (an address error on a
 *     68000) (#889).
 *   - No check of AllocMem: without chip memory the moves go to address
 *     0 (over ExecBase at 4) (#888).
 *   - Every InitView allocates a new list; only the last one is freed (by
 *     finit, and only if graphics.library was opened) (#891):
 *     FreeMem(0, size) if the last allocation failed (#888).
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
BLITZ_LIBS(LIB_bitmaplib, BL_LA(0) | BL_PD(0));
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
 * the last InitView is freed, the earlier ones are lost (BUG(orig)
 * (#891)); after a failed AllocMem this is FreeMem(0, size) (#888).
 */
static void BLITZ_ROUTINE codefinit_c(void)
{
    if (!gfxbase)
        return;
    CloseLibrary(gfxbase);
    gfxbase = NULL;
    if (!vd.viewsize)
        return;
    FreeMem(vd.cop, vd.viewsize);
    vd.viewsize = 0;
    vd.cop = NULL;
}

/* the word at an absolute address (a "register name", BUG(orig), #889) */
#define ABSW(a) (*(volatile const UWORD *)(a))

/**
 * @brief InitView height,depth,numsprites,numcolours: allocate a copper
 * list in chip memory and write its first four moves.
 *
 * The size is 16*numsprites + 16*depth + 8*height + 38, computed as a
 * word and sign-extended (EXT.l): the colour count is lost, MOVE.w d0,d3
 * overwrites it with the height before it is read (BUG(orig) (#890)).
 * The four `!cmove`s read their register and value from absolute
 * addresses: the list gets the words at $8E, $2C81, $90, $2CC1, $92,
 * $38, $94, $D0 (two odd addresses: an address error on a 68000)
 * (BUG(orig) (#889)).  AllocMem is not checked: without chip memory the
 * words go to address 0 (BUG(orig) (#888)).  Every call allocates a new
 * list; the old one is forgotten (BUG(orig) (#891)).  The list is never
 * installed.
 * @param height   d0.w: the height (counts 8 bytes per line)
 * @param depth    d1.w: the depth (16 bytes per plane)
 * @param sprites  d2.w: the number of sprites (16 bytes each)
 *                 (d3.w numcolours: ignored, see above)
 * @return d0: the address of the copper list (0 without chip memory)
 */
static ULONG BLITZ_ROUTINE initview_c(UWORD height BL_REG(d0), UWORD depth BL_REG(d1),
                                      UWORD sprites BL_REG(d2))
{
    UWORD size;
    UWORD *p;

    /* d3 = height (MOVE.w d0,d3: the colours are lost, BUG(orig), #890) */
    size = (UWORD)(sprites << 4);                       /* ASL.w #4,d2 */
    size += (UWORD)(depth << 3);                        /* + depth*8 */
    size += (UWORD)(height << 3);                       /* + "colours"*8 */
    size += (UWORD)(depth << 3);                        /* + depth*8 */
    size += 38;
    vd.viewsize = (ULONG)(LONG)(WORD)size;              /* EXT.l d2 */
    p = AllocMem(vd.viewsize, MEMF_CHIP);
    vd.cop = p;
    /* !cmove{diwstrt,$2c81} ...: words from absolute addresses */
    *p++ = ABSW(0x8e);
    *p++ = ABSW(0x2c81);
    *p++ = ABSW(0x90);
    *p++ = ABSW(0x2cc1);
    *p++ = ABSW(0x92);
    *p++ = ABSW(0x38);
    *p++ = ABSW(0x94);
    *p++ = ABSW(0xd0);
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
