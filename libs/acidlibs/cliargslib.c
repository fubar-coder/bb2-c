/*
 * cliargslib - command library 106: program arguments - NumPars, Par$,
 * FromCLI, ParPath$.
 *
 * C translation of bblibsrc/cliargslib ("aaron was here"; binary checked:
 * same as the source).  No init: every routine gets wbstartuplib's
 * argument data in a0 (!libs{#wbstartuplib,#la0}, see wbstartuplib.c):
 *   +0.w fromCLI (-1 = CLI/Blitz, 0 = Workbench), +2.l number of
 *   arguments, +6.l first argument; an argument (14 bytes):
 *   +0.l next, +4.l text, +8.w length, +10.l lock (Workbench only, 0 for
 *   CLI arguments).
 *
 * Kept from the original (BUG(orig)):
 *   - Not a bug (word function, the compiler uses only d0.w): FromCLI
 *     loads only the low word of d0 (MOVE.w): the upper word stays (thunk
 *     with "move.w d0,2(sp)").
 *   - FindArg walks the list without a check: argument numbers <= 0 or
 *     > NumPars (only caught by the error checks) run over the end of
 *     the list (argument 0: 2^32 steps). (#106)
 *   - ParPath$ calls NameFromLock(lock, pathBuffer, 255) also for CLI
 *     arguments (lock 0) when the error checks are off; the type is
 *     tested as a word (0 = drawer only, otherwise drawer + "/" + name,
 *     no "/" after a ":") (#107); an empty path compares the byte in front
 *     of the string (unreachable: NameFromLock succeeds only with a
 *     non-empty path).
 * DIFF: the original branches on the CCR that dos.library leaves after
 * NameFromLock (BEQ right after the JSR); here d0 = 0 is tested (the
 * documented failure value).  Either way nothing is written and d0 = 0.
 */
#include "blitzlib.h"
#include <dos/dos.h>
#define __NOLIBBASE__
#define DOS_BASE_NAME dosbase
#include <proto/dos.h>
#undef __NOLIBBASE__

/* wbstartuplib's argument data (a0) */
struct arg {
    struct arg *next;                           /* 0.l nextArg */
    UBYTE *text;                                /* 4.l argText */
    UWORD length;                               /* 8.w argLength */
    BPTR lock;                                  /* 10.l argLock */
} BL_PACKED;

struct argdata {
    UWORD fromcli;                              /* 0.w fromCLI */
    LONG numargs;                               /* 2.l numArgs */
    struct arg *first;                          /* 6.l firstArg */
} BL_PACKED;

/* entry points (thunks / C, defined below) */
void GetNumPars(void), GetParString(void), FromCLI(void);
void CheckParNum(void), CheckParPath(void);
ULONG BLITZ_ROUTINE GetParPath(const struct argdata *wb BL_REG(a0), ULONG n BL_REG(d0),
                               UWORD type BL_REG(d1), struct Library *dosbase BL_REG(a6));

/* !libheader{#cliargslib,0,0,0,errors} */
BLITZ_LIBHEADER(LIB_cliargslib, 0, BL_NORET, 0, BLITZ_ERROR_AREA);

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS(LIB_wbstartuplib, BL_LA(0));
BLITZ_SUBS(GetNumPars, 0, 0);
BLITZ_NAME("NumPars", "  pars.w=NumPars  Number of paramters passed to program.");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS(LIB_wbstartuplib, BL_LA(0));
BLITZ_SUBS(GetParString, CheckParNum, 0);
BLITZ_NAME("Par$", "  parameter$=Par$(Paramter#)   Returns the string value of a parameter.");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS(LIB_wbstartuplib, BL_LA(0));
BLITZ_SUBS(FromCLI, 0, 0);
BLITZ_NAME("FromCLI", "  result=FromCLI   0=Run from WorkBench, -1=Run from CLI");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_LONG, BL_WORD);
BLITZ_LIBS(LIB_wbstartuplib, BL_LA(0), LIB_doslib, BL_LA(6));
BLITZ_SUBS(GetParPath, CheckParPath, 0);
BLITZ_NAME("ParPath$", "  path$=ParPath$(Par#,#type)  Full/Dir path of WB arg. (v36+ only!)");

BLITZ_LIBFIN();

static UBYTE pathBuffer[256];                   /* Dcb.b 256,0 */

/* FindArg - argument d0 (from 1); no check */
static struct arg *findarg(const struct argdata *wb, ULONG n)
{
    struct arg *a = wb->first;

    while (--n)                                 /* SUBQ.l #1,d0:BEQ GotIt */
        a = a->next;
    return a;
}

/* MoveString - copy s including the 0 to a3, a3 stays on the 0;
   returns the length */
static ULONG movestring(const UBYTE *s)
{
    ULONG n = 0;

    do
        n++;
    while ((*blitz_string++ = *s++) != 0);
    blitz_string--;                             /* SUBQ.w #1,a3 */
    return n - 1;
}

/**
 * @brief ParPath$(Par#,#type): the path of a Workbench argument (V36+).
 *
 * NameFromLock(lock, pathBuffer, 255) of the argument's lock, copied to
 * a3; type 0 = the drawer only, otherwise drawer + "/" + name (no "/"
 * after a ":"; the type is tested as a word).  An empty path would
 * compare the byte in front of the string (unreachable: NameFromLock
 * succeeds only with a non-empty path).
 * DIFF: the original branches on the CCR that dos.library leaves after
 * NameFromLock (BEQ right after the JSR); here d0 = 0 is tested (the
 * documented failure value).  Either way nothing is written and d0 = 0.
 * Errors (debugger only, CheckParPath): "Can only call ParPath$() when
 * run from WB.", then those of Par$.
 * BUG(orig) (#106): no range check of the argument number (FindArg).
 * BUG(orig) (#107): without the error checks also called for CLI
 * arguments (lock 0).
 *
 * @param wb  a0: wbstartuplib's argument data (!libs{#wbstartuplib,#la0})
 * @param n  d0: the argument number (from 1)
 * @param type  d1.w: 0 = drawer only, otherwise drawer and name
 * @param dosbase  a6: dos.library
 * @return d0: the length of the string at a3, 0 if NameFromLock fails
 */
ULONG BLITZ_ROUTINE GetParPath(const struct argdata *wb BL_REG(a0), ULONG n BL_REG(d0),
                               UWORD type BL_REG(d1), struct Library *dosbase BL_REG(a6))
{
    const struct arg *a = findarg(wb, n);
    ULONG len;

    len = NameFromLock(a->lock, (STRPTR)pathBuffer, 255);
    if (!len)                                   /* DIFF: BEQ on dos's CCR */
        return len;
    len = movestring(pathBuffer);
    if (!type)                                  /* TST d4 (word) */
        return len;
    if (blitz_string[-1] != ':') {              /* a drawer: append "/" */
        *blitz_string++ = '/';
        len++;
    }
    return len + movestring(a->text);
}

/**
 * @brief FromCLI: whether the program was started from the CLI.
 *
 * MOVE.w (a0),d0: a word function; the upper word of d0 stays (thunk
 * with "move.w d0,2(sp)"; not a bug, the compiler uses only d0.w).
 *
 * @param wb  a0: wbstartuplib's argument data (!libs{#wbstartuplib,#la0})
 * @return d0.w: -1 = CLI (or Blitz), 0 = Workbench
 */
static UWORD BLITZ_ROUTINE fromcli_c(const struct argdata *wb BL_REG(a0))
{
    return wb->fromcli;
}
BLITZ_THUNK_POST(FromCLI, fromcli_c, "d0-d1/a0-a1", "move.w d0,2(sp)");

/**
 * @brief NumPars: the number of arguments passed to the program.
 *
 * MOVE.l numArgs(a0),d0.
 *
 * @param wb  a0: wbstartuplib's argument data (!libs{#wbstartuplib,#la0})
 * @return d0: the number of arguments
 */
static ULONG BLITZ_ROUTINE getnumpars_c(const struct argdata *wb BL_REG(a0))
{
    return wb->numargs;
}
BLITZ_THUNK(GetNumPars, getnumpars_c, "d1/a0-a1");

/**
 * @brief Par$(Parameter#): the text of an argument.
 *
 * Copies the argument's text to a3.  The original changes a0/a1, keeps
 * d1.  Errors (debugger only, CheckParNum): "No parameters passed.",
 * "Non-Existent Parameter." (not 1 <= Parameter# <= NumPars, signed).
 * BUG(orig) (#106): FindArg walks the list without a check: argument
 * numbers <= 0 or > NumPars run over the end of the list (argument 0:
 * 2^32 steps).
 *
 * @param wb  a0: wbstartuplib's argument data (!libs{#wbstartuplib,#la0})
 * @param n  d0: the argument number (from 1)
 * @return d0: the length of the string at a3
 */
static ULONG BLITZ_ROUTINE getparstring_c(const struct argdata *wb BL_REG(a0),
                                          ULONG n BL_REG(d0))
{
    return movestring(findarg(wb, n)->text);
}
BLITZ_THUNK(GetParString, getparstring_c, "d1");

/* ------------------------------------------- error checks (debugger only) */

BLITZ_ERRCHK_MESSAGE(mess1, "Non-Existent Parameter.");
BLITZ_ERRCHK_MESSAGE(mess3, "No parameters passed.");
BLITZ_ERRCHK_MESSAGE(mess2, "Can only call ParPath$() when run from WB.");

/* CheckParNum - arguments passed at all, 1 <= d0 <= numArgs (signed) */
static void BLITZ_ERRCHK checkparnum_c(const struct argdata *wb BL_REG(a0),
                                       LONG n BL_REG(d0))
{
    if (!wb->numargs) {
        BLITZ_RUNERR(mess3);
        return;
    }
    if (n > wb->numargs || n <= 0)
        BLITZ_RUNERR(mess1);
}

/* CheckParPath - only from Workbench, then CheckParNum */
static void BLITZ_ERRCHK checkparpath_c(const struct argdata *wb BL_REG(a0),
                                        LONG n BL_REG(d0))
{
    if (wb->fromcli) {                          /* TST.w (a0) */
        BLITZ_RUNERR(mess2);
        return;
    }
    checkparnum_c(wb, n);
}

/* the checks only test: they keep every register */
BLITZ_ERRCHK_THUNK(CheckParNum, checkparnum_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(CheckParPath, checkparpath_c, "d0-d1/a0-a1");
