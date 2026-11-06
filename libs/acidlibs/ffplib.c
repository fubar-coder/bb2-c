/*
 * ffplib - system library 64935 ($C900): the base of mathffp.library for
 * the floating point code of Blitz programs (token getffpbase, blitz2.s
 * equate 95).  Floats in Blitz 2 are Motorola FFP; the compiler and the
 * libraries call mathffp through a6 = this library's init value
 * (!libs #ffplib,#la6), e.g. floatquicklib, modlib.
 *
 * C translation of bblibsrc/ffplib (binary checked: same as the source).
 *   init:   open mathffp.library (OldOpenLibrary, no version check);
 *           the base is the init value.  No error check: BUG(orig) (#137) a
 *           failing open leaves 0 in the global and the first float
 *           operation jumps to a negative offset from 0.
 *   $C900:  no code - loads the init value into a6.
 *   finit:  close it again (a1 = the init value, !libs 64935,#la1).
 */
#include "blitzlib.h"
#include "blitzexec.h"

/* entry points (defined below) */
static ULONG codeinit(void);
static void codefinit(struct Library *lib BL_REG(a1));

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !syslibheader{64935,$c900,$c901,init,1,finit,0} */
BLITZ_SYSLIBHEADER7(LIB_ffplib, 0xc900, 0xc901, init, BL_LONGRET, finit, 0);
BLITZ_NULLSUB(0, 0, 0, LIB_ffplib, BL_LA(6));           /* $C900 getffpbase */
BLITZ_NULLSUB_AT(init, codeinit, 0, 0);
BLITZ_NULLSUB_AT(finit, codefinit, 0, 0, LIB_ffplib, BL_LA(1));
BLITZ_LIBFIN();

static const char ffpname[] = "mathffp.library";

/**
 * @brief init: open mathffp.library.
 *
 * LEA ffpname(pc),a1:MOVE.l 4,a6:JMP _OldOpenLibrary(a6), no version
 * check.  The original changes d0/d1/a0/a1 (exec) and a6; this keeps a6.
 * BUG(orig) (#137): the result is not checked; with 0 the first float
 * operation jumps to a negative offset from 0.
 *
 * @par Result
 * d0: the library base (the init value, $C900 getffpbase loads
 * it into a6), 0 if it could not be opened
 */
static ULONG BLITZ_ROUTINE codeinit(void)
{
    return (ULONG)OldOpenLibrary((CONST_STRPTR)ffpname);
}

/**
 * @brief finit: close mathffp.library again.
 *
 * MOVE.l 4,a6:JMP _CloseLibrary(a6).
 *
 * @arg lib  a1: the init value, the library base (!libs 64935,#la1)
 */
static void BLITZ_ROUTINE codefinit(struct Library *lib BL_REG(a1))
{
    CloseLibrary(lib);
}
