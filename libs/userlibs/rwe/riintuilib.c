/*
 * riintuilib - RWE userlib 15 (Reflective Images, 1995): text and pens on
 * a RastPort of its own, object library "Rast" (128 bytes, default
 * maximum 7).
 *   IPBitMap rast,bitmap   InitRastPort on the Rast object, its BitMap
 *                          the Blitz bitmap object
 *   IPLocate x,y           Move on the used Rast
 *   IPColour fgpen[,bgpen] SetAPen/SetBPen on the used Rast (a negative
 *                          word: unchanged), also the pens of IPOutput
 *   IPOutput rast          the output channel (inputoutputlib) prints
 *                          to that Rast with PrintIText
 *
 * C translation of rwelibsrc/riintuilib2.bb; reference: the reference
 * build of the source (REFERENCE refbuild, ref/riintuilib.obj; no binary
 * exists).  Notes: docs/rwelibs/port/riintuilib.md.
 *
 * init opens graphics.library and intuition.library (any version, not
 * checked), finit closes both.  IPOutput installs the handler ipprint
 * (inputoutputlib dosetout): -8 a null terminated string, -4 a0/d0, 0
 * one character.  The text is printed by PrintIText with an IntuiText of
 * the library: FrontPen/BackPen = the low bytes of the last IPColour
 * pens (1 and 0 at first), DrawMode 0 (JAM1), at 0,0; single characters
 * and a0/d0 strings are copied to a 1024 byte buffer first.
 *
 * The table ends twice like the source: !libfin{...} after the object
 * entries, then the init/finit nullsubs and a plain !libfin.
 *
 * BUG(orig), kept:
 *   - IPColour fgpen,bgpen with a negative fgpen calls SetBPen with the
 *     caller's a2 (the compiler's argument stack) as the RastPort: a2 is
 *     set to the Rast only with SetAPen (#696).
 *   - The single character handler reads the character from the address
 *     d0 (MOVE.l d0,a0:MOVE.b (a0)+,...), i.e. low memory, not d0 itself
 *     (#699).
 *   - The a0/d0 handler copies d0 (a word) bytes without a limit into the
 *     1024 byte buffer (the pens follow it) (#698).
 *   - PrintIText always prints at 0,0 of the Rast (+ the baseline): the
 *     pen position of IPLocate has no effect on IPOutput (#697).
 */
#include "blitzlib.h"
#include "blitzexec.h"
#include <graphics/rastport.h>
#include <intuition/intuition.h>
#define __NOLIBBASE__
#define GRAPHICS_BASE_NAME gfxbase
#define INTUITION_BASE_NAME intuibase
#include <proto/graphics.h>
#include <proto/intuition.h>
#undef __NOLIBBASE__

/* entry points (thunks, defined below) */
void _ipbitmap(void), _iplocate(void), _ipcolour(void), _ipcolour2(void);
void _ipoutput(void), riintuilib_libinit(void), riintuilib_libfinit(void);
void ipprint(void), ipprint0(void), ipprint1(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);
BLITZ_ENTRY_DECL(_load);
BLITZ_ENTRY_DECL(_save);
BLITZ_ENTRY_DECL(_use);
BLITZ_ENTRY_DECL(_free);

#define IA(a, p) (BL_LA(a) | BL_ITEM | BL_PD(p))   /* #ia<a>|#pd<p> */
#define UA(a) (BL_LA(a) | BL_USED)                  /* #ua<a> */

/* !libheader {#riintuilib,init,0,finit,0} */
BLITZ_LIBHEADER(LIB_riintuilib, init, BL_NORET, finit, 0);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_riintuilib, IA(0, 0), LIB_bitmaplib, IA(1, 1));
BLITZ_SUBS(_ipbitmap, 0, 0);
BLITZ_NAME("IPBitMap", "rast,bitmap");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_riintuilib, UA(0));
BLITZ_SUBS(_iplocate, 0, 0);
BLITZ_NAME("IPLocate", "x,y");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_riintuilib, UA(0));
BLITZ_SUBS(_ipcolour, 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_riintuilib, UA(0));
BLITZ_SUBS(_ipcolour2, 0, 0);
BLITZ_NAME("IPColour", "fgpen[,bgpen]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_riintuilib, IA(0, 0));
BLITZ_SUBS(_ipoutput, 0, 0);
BLITZ_NAME("IPOutput", "rast");

BLITZ_DUMTOKE(_toke, "Rast", "");

BLITZ_NULLSUB_AT(_load, 0, 0, 0);
BLITZ_NULLSUB_AT(_save, 0, 0, 0);
BLITZ_NULLSUB_AT(_use, 0, 0, 0);
BLITZ_NULLSUB_AT(_free, 0, 0, 0);

/* !libfin{_toke,_load,_save,_use,_free,7,7}: 128 byte objects */
BLITZ_LIBFIN7(_toke.n, _load, _save, _use, _free, 7, 7);

BLITZ_NULLSUB_AT(finit, riintuilib_libfinit, 0, 0);
BLITZ_NULLSUB_AT(init, riintuilib_libinit, 0, 0);

BLITZ_LIBFIN();

/* ---------------------------------------------------------------- data */

static struct Library *intuibase, *gfxbase;
static struct RastPort *usedrast;

/* myintuitext, cbuf, apen, bpen in the original's order: an a0/d0 string
   over 1024 bytes runs into the pens */
static struct BL_PACKED {
    struct IntuiText it;                /* ia, ib, ijam.w, ix, iy, ifnt, itxt, 0 */
    UBYTE cbuf[1024];
    UWORD apen, bpen;
} pr = { .apen = 1, .bpen = 0 };

/* ------------------------------------------------------- init / finit */

/**
 * @brief init: opens graphics.library and intuition.library.
 *
 * Any version; the results are not checked.
 */
static void BLITZ_ROUTINE libinit_c(void)
{
    gfxbase = OpenLibrary((CONST_STRPTR)"graphics.library", 0);
    intuibase = OpenLibrary((CONST_STRPTR)"intuition.library", 0);
}

/**
 * @brief finit: closes intuition.library and graphics.library.
 */
static void BLITZ_ROUTINE libfinit_c(void)
{
    CloseLibrary(intuibase);
    CloseLibrary(gfxbase);
}

/* ------------------------------------------------------------ commands */

/**
 * @brief IPBitMap rast,bitmap: initialises a Rast object and gives it a bitmap.
 *
 * InitRastPort on the Rast, then its BitMap = the Blitz bitmap object.
 * @param rp  a0: the Rast object (#ia0|#pd0)
 * @param bm  a1: the bitmap object (bitmaplib, #ia1|#pd1)
 */
static void BLITZ_ROUTINE ipbitmap_c(struct RastPort *rp BL_REG(a0),
                                     struct BitMap *bm BL_REG(a1))
{
    InitRastPort(rp);
    rp->BitMap = bm;
}

/**
 * @brief IPLocate x,y: moves the pen of the used Rast.
 *
 * Move with the low words of x and y.  BUG(orig) (#697): IPOutput always
 * prints at 0,0, so this has no effect on its output.
 * @param x   d0: x (a word)
 * @param y   d1: y (a word)
 * @param rp  a0: the used Rast (#ua0)
 */
static void BLITZ_ROUTINE iplocate_c(ULONG x BL_REG(d0), ULONG y BL_REG(d1),
                                     struct RastPort *rp BL_REG(a0))
{
    Move(rp, x & 0xffff, y & 0xffff);
}

/**
 * @brief IPColour fgpen,bgpen: sets the pens of the used Rast and of IPOutput.
 *
 * SetAPen/SetBPen; a negative word leaves that pen unchanged.  The pens
 * are also kept for the IntuiText of IPOutput (their low bytes).
 * BUG(orig) (#696): the RastPort of SetBPen is a2, which is set to the
 * Rast only with SetAPen (else the caller's a2, the argument stack).
 * @param fg      d0: the foreground pen (a word)
 * @param bg      d1: the background pen (a word)
 * @param rp      a0: the used Rast (#ua0)
 * @param bpenrp  a2: the caller's a2 (SetBPen's RastPort with fg < 0)
 */
static void BLITZ_ROUTINE ipcolour2_c(ULONG fg BL_REG(d0), ULONG bg BL_REG(d1),
                                      struct RastPort *rp BL_REG(a0),
                                      struct RastPort *bpenrp BL_REG(a2))
{
    if ((WORD)fg >= 0) {
        bpenrp = rp;
        pr.apen = (UWORD)fg;
        SetAPen(rp, fg);
    }
    bg &= 0xffff;
    if ((WORD)bg >= 0) {
        pr.bpen = (UWORD)bg;
        SetBPen(bpenrp, bg);
    }
}

/**
 * @brief IPColour fgpen: sets the foreground pen only.
 *
 * As IPColour fgpen,bgpen with bgpen -1 (unchanged).
 * @param fg      d0: the foreground pen (a word)
 * @param rp      a0: the used Rast (#ua0)
 * @param bpenrp  a2: the caller's a2
 */
static void BLITZ_ROUTINE ipcolour_c(ULONG fg BL_REG(d0), struct RastPort *rp BL_REG(a0),
                                     struct RastPort *bpenrp BL_REG(a2))
{
    ipcolour2_c(fg, (ULONG)-1, rp, bpenrp);
}

/* the pens of IPColour into the IntuiText: every handler takes them first,
   before it copies a string (so #698's overflow changes only the pens of
   the next text) */
static void setpens(void)
{
    pr.it.FrontPen = (UBYTE)pr.apen;
    pr.it.BackPen = (UBYTE)pr.bpen;
}

/* the output handler: the IntuiText at 0,0 */
static void doipprint(void)
{
    PrintIText(usedrast, &pr.it, 0, 0);
}

/**
 * @brief The output handler, entry 0: one character.
 *
 * Copied to the buffer with a 0, then printed.  BUG(orig) (#699): the
 * character is read from the address d0 (MOVE.l d0,a0:MOVE.b (a0)+,...),
 * i.e. low memory, not d0 itself.
 * @param chaddr  d0: the character (used as an address)
 */
static void BLITZ_ROUTINE ipprint_c(ULONG chaddr BL_REG(d0))
{
    setpens();
    pr.cbuf[0] = *(const UBYTE *)chaddr;
    pr.cbuf[1] = 0;
    pr.it.IText = pr.cbuf;
    doipprint();
}

/**
 * @brief The output handler, entry -8: a null terminated string.
 *
 * Printed directly (the IntuiText points to it).
 * @param s  a0: the string
 */
static void BLITZ_ROUTINE ipprint0_c(UBYTE *s BL_REG(a0))
{
    setpens();
    pr.it.IText = s;
    doipprint();
}

/**
 * @brief The output handler, entry -4: d0 characters at a0.
 *
 * Copied to cbuf with a 0, then printed.  BUG(orig) (#698): no limit -
 * more than 1024 characters run into the pens behind the buffer (they
 * change the pens of the next text only).
 * @param n  d0: the number of characters (a word)
 * @param s  a0: the characters
 */
static void BLITZ_ROUTINE ipprint1_c(UWORD n BL_REG(d0), const UBYTE *s BL_REG(a0))
{
    UBYTE *d = pr.cbuf;

    setpens();
    pr.it.IText = d;
    while (n) {
        *d++ = *s++;
        n--;
    }
    *d = 0;
    doipprint();
}

/**
 * @brief IPOutput rast: makes the Rast the output channel (Print draws on it).
 *
 * Keeps the Rast and installs the handler ipprint with inputoutputlib's
 * dosetout (TOK_setout).  The text is printed by PrintIText at 0,0 of
 * the Rast (BUG(orig) #697), JAM1, with the pens of the last IPColour.
 * @param rp  a0: the Rast object (#ia0|#pd0)
 */
static void BLITZ_ROUTINE ipoutput_c(struct RastPort *rp BL_REG(a0))
{
    struct bl_regs r = { 0 };

    usedrast = rp;
    r.a0 = (ULONG)ipprint;
    BLITZ_ALIBJSR(TOK_setout, r);
}

/* Registers: the original changes a6 everywhere, d0/d1 (and a0/a1 but in
   IPBitMap) through the graphics/intuition calls, a2 in IPColour with a
   pen >= 0; the handlers keep d3/d6 only.  The thunks keep d0/d1/a0/a1,
   the C code d2-d7/a2-a6. */
BLITZ_THUNK(riintuilib_libinit, libinit_c, "d0-d1/a0-a1");
BLITZ_THUNK(riintuilib_libfinit, libfinit_c, "d0-d1/a0-a1");
BLITZ_THUNK(_ipbitmap, ipbitmap_c, "d0-d1/a0-a1");
BLITZ_THUNK(_iplocate, iplocate_c, "d0-d1/a0-a1");
BLITZ_THUNK(_ipcolour, ipcolour_c, "d0-d1/a0-a1");
BLITZ_THUNK(_ipcolour2, ipcolour2_c, "d0-d1/a0-a1");
BLITZ_THUNK(_ipoutput, ipoutput_c, "d0-d1/a0-a1");

/* the output handler: -8 ipprint0, -4 ipprint1, 0 ipprint */
__asm__("   .text\n   .even\n"
        "   bra.w   _ipprint0\n"
        "   bra.w   _ipprint1\n"
        "   .globl  _ipprint\n"
        "_ipprint:\n"
        "   movem.l d0-d1/a0-a1,-(sp)\n"
        "   jsr     _ipprint_c\n"
        "   movem.l (sp)+,d0-d1/a0-a1\n"
        "   rts\n");
BLITZ_THUNK(ipprint0, ipprint0_c, "d0-d1/a0-a1");
BLITZ_THUNK(ipprint1, ipprint1_c, "d0-d1/a0-a1");
