/*
 * rishapezlib - RWE userlib 21 (Reflective Images, 1995): three small
 * helpers.
 *
 *   CludgeAcid on|off           1: a4 = the a4 saved before, else save a4
 *   PeekByte(address)           the byte at address (inline)
 *   SetWinDrawShapeZ win,mode   the used window's area pattern: 0 none,
 *                               else the mesh $AAAA/$5555
 *
 * C translation of rwelibsrc/shapezlib.bb; reference: the reference build
 * of that source (no binary exists, docs/rwelibs/README.md 3.3).  Notes:
 * docs/rwelibs/port/rishapezlib.md.
 *
 * CludgeAcid sets a4 (the locals of the Blitz program) and PeekByte is an
 * inline routine (Dc $a000): both assembler.  SetWinDrawShapeZ is C.
 * init and finit do nothing.
 *
 * BUG(orig):
 *   - PeekByte has no RTS: called as a subroutine (not inline), it runs
 *     on into CludgeAcid with d0 = the byte - a byte 1 loads a4 with the
 *     saved value, any other byte saves a4 (#907).  The layout is kept.
 *
 * CludgeAcid 1 before any CludgeAcid 0 sets a4 = 0 (nothing saved yet).
 */
#include "blitzlib.h"
#include <intuition/intuition.h>

/* entry points (below) */
void _cludgeacid(void), _peekb(void), rishapezlib_SetWindowDraw(void);
static void _libinit(void);
static void _libfinit(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !libheader {#rishapezlib,init,0,finit,0} */
BLITZ_LIBHEADER(LIB_rishapezlib, init, BL_NORET, finit, 0);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_cludgeacid, 0, 0);
BLITZ_NAME("CludgeAcid", "on|off");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_peekb, 0, 0);
BLITZ_NAME("PeekByte", "addres");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_windowslib, BL_LA(0) | BL_USED);
BLITZ_SUBS(rishapezlib_SetWindowDraw, 0, 0);
BLITZ_NAME("SetWinDrawShapeZ", "mode    -- 0=normal, 1=meshed");

BLITZ_NULLSUB_AT(finit, _libfinit, 0, 0);
BLITZ_NULLSUB_AT(init, _libinit, 0, 0);

BLITZ_LIBFIN();

/** @brief init: nothing (RTS). */
static void _libinit(void)
{
}

/** @brief finit: nothing (RTS). */
static void _libfinit(void)
{
}

/* CludgeAcid's saved a4 */
ULONG rishapez_myold;

/**
 * @brief PeekByte(address): the byte at address; and CludgeAcid on|off:
 * a4 saved or restored.
 *
 * PeekByte is inline (MOVE.l d0,a0:MOVE.b (a0),d0:ANDI.w #$ff,d0): d0 =
 * the address, result d0.w = the byte.  Directly behind it, as in the
 * original, CludgeAcid on|off: d0.b = 1 sets a4 = the saved a4 (0 if
 * nothing was saved yet), any other value saves a4 (the Blitz program's
 * locals pointer, fixed for GCC: assembler); no result.
 * BUG(orig) (#907): PeekByte has no RTS - as a subroutine it runs into
 * CludgeAcid with d0 = the byte.
 */
__asm__("   .text\n   .even\n   .globl  __peekb\n"
        "__peekb:\n"
        "   .word   0xa000,2f-1f\n"
        "1: move.l  d0,a0\n"
        "   move.b  (a0),d0\n"
        "   andi.w  #0xff,d0\n"
        "2:\n"
        "   .globl  __cludgeacid\n"
        "__cludgeacid:\n"
        "   cmp.b   #1,d0\n"
        "   beq.s   3f\n"
        "   move.l  a4,_rishapez_myold\n"
        "   rts\n"
        "3: move.l  _rishapez_myold,a4\n"
        "   rts\n");

/* the mesh of SetWinDrawShapeZ */
static const UWORD mypattern[2] = { 0xaaaa, 0x5555 };

/**
 * @brief SetWinDrawShapeZ window#,mode: the used window's area pattern.
 *
 * Mode 0: no pattern (AreaPtrn NULL, AreaPtSz 0), else the mesh
 * $AAAA/$5555 (AreaPtSz 1).  The window# argument (d0) is not used: the
 * window is the used one (!libs #windowslib,#ua0); the help text names
 * only the mode.
 * @param mode  d1: 0 normal, else meshed (the word)
 * @param win   a0: the used window object (its first long the Window)
 */
static void BLITZ_ROUTINE setwindowdraw_c(UWORD mode BL_REG(d1),
                                          struct Window **win BL_REG(a0))
{
    struct RastPort *rp = (*win)->RPort;

    if (mode) {
        rp->AreaPtrn = (UWORD *)mypattern;
        rp->AreaPtSz = 1;
    } else {
        rp->AreaPtrn = NULL;
        rp->AreaPtSz = 0;
    }
}
BLITZ_THUNK(rishapezlib_SetWindowDraw, setwindowdraw_c, "d0-d1/a0-a1");
