/*
 * ab3dir.c - the compiler directives of AB3 mode (blitz2-ab3; extension
 * points of ext.h, spec ab3-compat/directives, design D5 of the change
 * ab3-mode-g2).  In BB2 mode none of them is handled: AmiBlitz3's tokens
 * are not in BB2's token list.
 */
#include "blitz2.h"
#include "ab3.h"

/* AmiBlitz3's optimisation flags (AmiBlitz3.ab3:26176-26180, do_OPTIMIZE
   26829): bit 0 68020 code, 1 FPU code, 2 new syntax, 4 short branches;
   bit 3 is documented but does nothing (the code tests bit 4).  0 at the
   start of every pass (Aclearallvars 2581). */
#define OPT_68020       1
#define OPT_FPU         2
#define OPT_NEWSYNTAX   4
static UWORD optflags;

/* strings2lib's setfpumode ($C703, strings2Lib.ab3:86): the FPU mode of
   the run-time libraries, called by every OPTIMIZE */
#define SETFPUMODE      0xc703

void ext_passstart(void)
{
    optflags = 0;
}

BOOL ext_newsyntax(void)
{
    return (optflags & OPT_NEWSYNTAX) != 0;
}

/* OPTIMIZE n (AmiBlitz3.ab3:26829): a constant expression; its bits
   replace all flags.  The FPU bit is refused until FPU code exists (G6);
   the 68020 bit is accepted with 68000 code (G7), bits 3 and 4 change
   nothing here (docs/ab3/variant.md).  As in AmiBlitz3 the statement
   calls setfpumode with 0 (2 with the FPU bit), move.l #m,d0 first. */
static void dooptimize(void)
{
    ULONG v = evalconst(NULL, NULL);

    optflags = (UWORD)(v & 0x1f);
    if (optflags & OPT_FPU)
        err("Not supported: OPTIMIZE 2 (FPU)");
    pokewd(0x203c);                     /* move.l #0,d0 */
    pokel(0);
    tokejsr(SETFPUMODE);
}

/* the statements of AB3 mode that are not blitz2's (the tokens of
   toklist.h's AmiBlitz3 names) */
BOOL ext_token(UWORD c)
{
    if (progmode != XTRA_MODE_AB3)
        return FALSE;
    switch (c) {
    case 0x8014:                        /* SYNTAX n (BB2's Jimi slot) */
        /* AmiBlitz3.ab3:26884 do_SYNTAX: the one character behind the
           token; its declaration and type checks are G8, they change no
           code.  No code of its own, as AmiBlitz3: no debug trap. */
        undodebug();
        if (!eos(getchar3()))
            getchar3();
        return TRUE;
    case 0x8000 + tnum + 40:            /* OPTIMIZE, $80DB */
        getchar3();
        dooptimize();
        return TRUE;
    }
    return FALSE;
}
