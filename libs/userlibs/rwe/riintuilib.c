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
 *     set to the Rast only with SetAPen.
 *   - The single character handler reads the character from the address
 *     d0 (MOVE.l d0,a0:MOVE.b (a0)+,...), i.e. low memory, not d0 itself.
 *   - The a0/d0 handler copies d0 (a word) bytes without a limit into the
 *     1024 byte buffer (the pens follow it).
 *   - PrintIText always prints at 0,0 of the Rast (+ the baseline): the
 *     pen position of IPLocate has no effect on IPOutput.
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
void _ipoutput(void), _libinit(void), _libfinit(void);
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

BLITZ_NULLSUB_AT(finit, _libfinit, 0, 0);
BLITZ_NULLSUB_AT(init, _libinit, 0, 0);

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

static void BLITZ_ROUTINE libinit_c(void)
{
    gfxbase = OpenLibrary((CONST_STRPTR)"graphics.library", 0);
    intuibase = OpenLibrary((CONST_STRPTR)"intuition.library", 0);
}

static void BLITZ_ROUTINE libfinit_c(void)
{
    CloseLibrary(intuibase);
    CloseLibrary(gfxbase);
}

/* ------------------------------------------------------------ commands */

/* IPBitMap rast,bitmap: a0 = the Rast object, a1 = the bitmap object */
static void BLITZ_ROUTINE ipbitmap_c(struct RastPort *rp BL_REG(a0),
                                     struct BitMap *bm BL_REG(a1))
{
    InitRastPort(rp);
    rp->BitMap = bm;
}

/* IPLocate x,y (the used Rast) */
static void BLITZ_ROUTINE iplocate_c(ULONG x BL_REG(d0), ULONG y BL_REG(d1),
                                     struct RastPort *rp BL_REG(a0))
{
    Move(rp, x & 0xffff, y & 0xffff);
}

/* IPColour fgpen,bgpen: a negative word leaves a pen.  BUG(orig): the
   RastPort of SetBPen is a2, which is set to the Rast only with SetAPen
   (else the caller's a2) */
static void BLITZ_ROUTINE ipcolour2_c(ULONG fg BL_REG(d0), ULONG bg BL_REG(d1),
                                      struct RastPort *rp BL_REG(a0),
                                      struct RastPort *a2 BL_REG(a2))
{
    if ((WORD)fg >= 0) {
        a2 = rp;
        pr.apen = (UWORD)fg;
        SetAPen(rp, fg);
    }
    bg &= 0xffff;
    if ((WORD)bg >= 0) {
        pr.bpen = (UWORD)bg;
        SetBPen(a2, bg);
    }
}

/* IPColour fgpen: bgpen -1 */
static void BLITZ_ROUTINE ipcolour_c(ULONG fg BL_REG(d0), struct RastPort *rp BL_REG(a0),
                                     struct RastPort *a2 BL_REG(a2))
{
    ipcolour2_c(fg, (ULONG)-1, rp, a2);
}

/* the output handler: the IntuiText with the pens of IPColour at 0,0 */
static void doipprint(void)
{
    pr.it.FrontPen = (UBYTE)pr.apen;
    pr.it.BackPen = (UBYTE)pr.bpen;
    PrintIText(usedrast, &pr.it, 0, 0);
}

/* one character - BUG(orig): read from the address d0 */
static void BLITZ_ROUTINE ipprint_c(ULONG d0 BL_REG(d0))
{
    pr.cbuf[0] = *(const UBYTE *)d0;
    pr.cbuf[1] = 0;
    pr.it.IText = pr.cbuf;
    doipprint();
}

/* a null terminated string */
static void BLITZ_ROUTINE ipprint0_c(UBYTE *s BL_REG(a0))
{
    pr.it.IText = s;
    doipprint();
}

/* d0 (a word) characters at a0, copied to cbuf - BUG(orig): no limit */
static void BLITZ_ROUTINE ipprint1_c(UWORD n BL_REG(d0), const UBYTE *s BL_REG(a0))
{
    UBYTE *d = pr.cbuf;

    pr.it.IText = d;
    while (n) {
        *d++ = *s++;
        n--;
    }
    *d = 0;
    doipprint();
}

/* IPOutput rast: the output channel to ipprint */
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
BLITZ_THUNK(_libinit, libinit_c, "d0-d1/a0-a1");
BLITZ_THUNK(_libfinit, libfinit_c, "d0-d1/a0-a1");
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
