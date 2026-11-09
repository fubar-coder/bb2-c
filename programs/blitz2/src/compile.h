/*
 * compile.h - chunk 02 (blitz2.s 1912-3290): compile driver, passes,
 * per-line advance + racing stripe, macro expansion, STOP requester and
 * exception handler, end-of-pass checks, debugger trap records, statement
 * dispatch (dostatement/handle*), library statement calls (dolibtoke).
 * See notes/survey-02.md.
 */
#ifndef BLITZ2_COMPILE_H
#define BLITZ2_COMPILE_H

#include <exec/types.h>
#include <setjmp.h>
#include <intuition/intuition.h>
#include "structs.h"

#ifndef NORETURN
#define NORETURN __attribute__((noreturn))
#endif

/* =================================================================
 * Control-flow contracts (stack tricks of the asm)
 * ================================================================= */

/* --- (1) pass loop re-entry: asm `move.l passstack,a7 / jmp passnxt`
 * (doinc2, chunk 08, after opening an include file).
 * dopass() does setjmp(*passjmp) right before its line loop; a longjmp
 * resumes at `passnxt` (nextline / doline loop) with the C stack unwound
 * to dopass's frame, exactly like resetting a7 to passstack.
 * Usage in doinc2:   longjmp(*passjmp, 1);
 * passjmp is NULL outside dopass (direct mode: doinclude errors out with
 * illdirerr before reaching doinc2).  The asm variable `passstack`
 * (chunk 10) is no longer used.  DIFF: errjmp is not touched; nothing
 * between dopass and doinc2 changes it. */
extern jmp_buf *passjmp;

/* --- (2) handlers that drop their caller's return address.
 *
 * smode (`addq #4,a7` + jmp modetable[linemode]) drops the return into
 * handlechar, so the mode handler (typemode, chunk 09) returns to doline
 * as if dostatement had returned.  In C this needs no help: handlechar
 * calls modetable[linemode](reget()) and returns.  Mode handlers are plain
 * tokhandlers; doline then does its normal reget/illeolerr/un-then.
 *
 * domacro (chunk 08) does `addq #4,a7` as a statement handler, dropping
 * the return into doline: doline returns at once (no reget/illeolerr
 * check, no "un-then", rest of line ignored) to its caller (pass loop or
 * direct mode).  C: the handler sets
 *         dolineabort = 1;
 * and returns normally (the asm's tail `bra getchar3` is still done);
 * dostatement/handlechar return immediately after any handler, and doline
 * tests the flag right after dostatement():
 *         if (dolineabort) { dolineabort = 0; return; }
 * Any other handler that does `addq #4,a7` inside a token handler (i.e.
 * "abort the rest of this line") must use the same flag. */
extern WORD dolineabort;

/* =================================================================
 * Dispatch tables (statement handlers, see tokhandler in structs.h)
 * ================================================================= */

/* blitz2.s:1298 atokens: built-in statement handlers, index token-1
 * (after handletoken's remapping of post-assembler tokens: n -= tnum-fnum).
 * Entries the asm filled with `syntaxerr` / `eos` (13448, chunk 09; the C
 * name must differ from parse.h's eos()) point to tokhandler wrappers.
 * Defined in compile.c. */
#define NATOKENS 92                     /* (atokensend-atokens)/4 */
extern tokhandler *const atokens[NATOKENS];

/* blitz2.s:14831 modetable: special line modes, index linemode (>=0).
 * [0] = typemode (NewType field lines, chunk 09).  Defined in compile.c. */
extern tokhandler *const modetable[];

/* =================================================================
 * Code template
 * ================================================================= */

/* blitz2.s:2683 eopcode: `moveq #3,d0 / trap #0` (0x7003,0x4E40), poked
 * as one long (pokel(L_(eopcode))) at the end of a debug-enabled program. */
extern UWORD eopcode[];

/* =================================================================
 * Globals defined in chunk 02
 * ================================================================= */

extern UWORD smallpass;     /* 1915 second ("small code") pass counter */
extern UWORD riteend;       /* 2034 right end of include stripe */
extern WORD  macmask;       /* 2036 -1: current line was macro-expanded */
extern WORD  stopbit;       /* 2449 sig bit of editor window port */
extern volatile WORD stopit; /* 2450 0 / -1: STOP pressed (set by stopcode,
                               read by the program process too) */
extern WORD  clrreq;        /* 2451 STOP requester open (toggled with ~) */
extern UWORD intsused;      /* 2480 interrupts used -> ints off at end */
extern ULONG varsizeat;     /* 2517 */
extern UWORD debugga;       /* 2518 debugger enabled; asm writes it with
                               `sne debugga` (high byte!) -> 0xff00 / 0;
                               tests are `!= 0` (byte or word) */
extern char  compiling[];   /* 2520 "COMPILING\0 STOP! \0" (openstopreq arg) */
extern char  executing[];   /* 2523 "EXECUTING\0!PANIC!\0" */
extern ULONG endjmpat;      /* 2686 object address of the final jmp's long */
extern ULONG d_pc, d_bigpc; /* 2861 saved pc/bigpc for undodebug */
extern UWORD d_nomemleft;   /* 2863 */
extern ULONG cont_pc, cont_pc2, lastcontlink;  /* 2864 debug record chain */

/* Intuition structures of the STOP requester (2394-2447) */
extern struct Requester stopreq;
extern struct Gadget    stopreqga1;     /* GadgetID 1 = STOP */
extern struct IntuiText stopreqin1, stopreqin2;
extern char stopreqla7[];                /* "***** COMPILING *****" */
#define reqmessage (stopreqla7 + 6)     /* label inside stopreqla7 */
extern char stopreqla11[];              /* " STOP! " */

/* =================================================================
 * Routines
 * ================================================================= */

/* --- driver / cleanup */
void compile(void);         /* 1917 compile whole program (menu0/menu1);
                               owns errjmp for the pass (asm errcont =
                               freeemup); anyerrs=-1 on error/cancel */
void qfree(void);           /* 1998 free per-pass structures */
void freeemup(void);        /* 2010 qfree + free labels/vars/types/macros/procs */
void fralltypes(void);      /* 2019 free NewTypes after lastrestype */
void frallvars(void);       /* 2024 free global vars */
void freeifs(void);         /* 2028 free firstif list (14 byte nodes) */
void dopass(void);          /* 2556 one compile pass */

/* --- per line */
/* 2038: advance to next source line (include or editor).
 * asm: Z=1 => end of program.  Returns TRUE if there is a line; srcptr
 * (a5) then points to it.  Also frees the macro buffer and updates the
 * racing stripe. */
BOOL nextline(void);
/* 2105: write val as 4 upper case hex digits at buf (no NUL) */
void hexascii(UWORD val, UBYTE *buf);

/* --- macros */
void expandmax(void);       /* 2121 expand `!name{..}` macros in the statement
                               at srcptr; srcptr may move to myline buffer */
void freembuff(void);       /* 2342 free myline (macrolen bytes) */
/* 2351: scan a macro parameter at *pp (in/out, past the terminator);
 * returns the terminator ',' or '}' found at nesting 0; toggles quoteflag;
 * NUL -> syntaxerr */
UWORD findit(UBYTE **pp);
BOOL isitopen(UWORD c);     /* 2384 asm Z=1: c is '{' or '(' */
BOOL isitclose(UWORD c);    /* 2389 asm Z=1: c is '}' or ')' */

/* --- STOP requester (stopcode_c / stopcode_stub: see glue.h) */
void endstop(void);         /* 2469 close requester, remove exception */
void setcvars(void);        /* 2482 per-pass variable init */
/* 2526: open the STOP requester; strs = two consecutive NUL-terminated
 * strings (compiling / executing); installs stopcode_stub as exception */
void openstopreq(const char *strs);
/* 2828: drain editor input (comdata+32); TRUE (asm NE) if STOP gadget was
 * hit; stopit = -1/0 accordingly */
BOOL chkrealstop(void);
void chkstop(void);         /* 2847 if stopit && chkrealstop(): gostop */
NORETURN void gostop(void); /* 2853 endstop, pointeron, anyerrs=-1,
                               longjmp(*errjmp) like err */

/* --- end of pass checks */
void pointchk(void);        /* 2688 undefined forward types -> notypeerr */
void errchx2(void);         /* 2700 unresolved labels, open CNIF; then errchx */
void errchx(void);          /* 2719 open Select/Repeat/Interrupt/If/
                               Statement/For -> error (preserved asm d1) */
NORETURN void chxerr4(void);/* 2747 nocenderr at ciflineat */
void calcmaxs(void);        /* 2750 emit lib max tables, uselib(memlib) */

/* --- statements */
void doline(void);          /* 2780 compile all statements of one line,
                               un-then; honours dolineabort */
UWORD createlab(void);      /* 2807 namebuff = lastbiglab+"'"+name;
                               returns the length (asm d2) */
void prepstack(void);       /* 2824 reset forth/prec stacks */
void maketrap(UWORD n);     /* 2868 emit TRAP #n ($4E40|n) */
void debugstuff(void);      /* 2871 emit TRAP #1 debug record */
void undodebug(void);       /* 2915 take back last debug record */
void setsvars(void);        /* 2927 per-statement variable init */
void dostatement(void);     /* 2953 compile one statement (falls into
                               handlechar); leaves current char for reget */

/* 3181 dolibtoke: emit a library statement/function call.
 * in: nparams (asm d1), blib (a2), sub (a3, from findtoke), usertype.
 * Returns current char (asm: tail `bra reget`, use eos()). */
UWORD dolibtoke(UWORD nparams, struct blitzlib *blib, UBYTE *sub);

/* 3260 sizespec2: parse optional size specifier (.type / $) after a lib
 * token.  Returns the primitive type code (asm d1), also stored in
 * usertype.  srcptr is backed up unless '$' was consumed. */
UWORD sizespec2(void);

#endif
