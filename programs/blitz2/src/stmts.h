/*
 * stmts.h - chunk 03, blitz2.s 3291-4999: statement handlers (Exchange, Pop,
 * Repeat, lib/toke jsrs, Blitz/Amiga mode, SetInt/SetErr, PutReg/SetReg ...),
 * direct mode (trap #15), object buffer allocation (allox), residents
 * (make/load), asm token table, asm effective address parser.
 *
 * Statement handlers are `tokhandler`s: entered after getchar3 with the
 * current char/token c (asm d0 + Z: use eos(c)).  Helpers whose asm ended in
 * `bra getchar3` are void: the dispatcher re-reads the char with reget.
 */
#ifndef BLITZ2_STMTS_H
#define BLITZ2_STMTS_H

#include <exec/types.h>
#include "structs.h"
#include "glue.h"

/* ------------------------------------------------------------ globals */

extern UBYTE usedpath[128];     /* 3304 UsePath text, terminated by byte 2 */
extern struct repnode *firstrep;/* 3501 open Repeat list */
extern UWORD replineat;         /* 3502 line of last Repeat */
extern UWORD inerr;             /* 3684 in SetErr block (0 / $ffff) */
extern ULONG errjmp_;           /* 3685 asm `errjmp` (renamed: errjmp is the
                                   error recovery jmp_buf* of parse.h): address
                                   of the JMP operand patched at End SetErr */
extern ULONG errcode;           /* 3686 address of the move.l #x,d0 operand */
extern UWORD intlineat;         /* 3726 SetInt line (errchx) */
extern UWORD intallox;          /* 3790 unused */
extern ULONG intdata1;          /* 3792 where in data1 a5 is kept for ints */
extern ULONG intjmpat;          /* 3794 JMP operand to patch at End SetInt */
extern UWORD intline;           /* 3795 line for interr2 */
extern WORD  intstring;         /* 3796 0 no SetInt, >0 no string space, <0 with */
extern UWORD intlevel;          /* 3799 */
extern ULONG intcleanat;        /* 3800 debug: trap #2 operand to patch */
extern ULONG ret15add;          /* 3802 trap #15 return address (after params) */
extern UWORD ret15cc;           /* 3803 trap #15 saved SR */
extern char restext[];          /* 4098 'Name of Resident File to Create' */
extern char respath[192];       /* 4100 requester path buffer */
extern char resname[192];       /* 4102 requester file buffer */
extern char hello[4];           /* 4212 'ZAP',0 - padding bytes */

/* asm data of every asm mnemonic token (fnum..tnum-1), defined at
   blitz2.s 837 as `ds.l tnum-fnum`, owned here; filled by makeasmtab */
extern struct asment *asmpnts[];

/* ---------------------------------------------- code templates (writable) */

extern UWORD swapb[];           /* 3316 Exchange code, 20 bytes */
#define swapbf (&swapb[3])
#define swapw  (&swapb[3])
#define swapwf (&swapb[6])
#define swapl  (&swapb[6])
#define swaplf (&swapb[9])
#define pulla1 (&swapb[9])      /* move.l (a7)+,a1 ($225f) */
#define pulla1f (&swapb[10])

extern UWORD fvwait[];          /* 3620 subq #1,d0; cgoblitz; cgoamiga (12 bytes) */
#define cgoblitz (&fvwait[1])   /* move #-1,0(a5): only the first word is used */
#define cgoamiga (&fvwait[4])   /* clr 0(a5):      only the first word is used */

extern UWORD intstart0[];       /* 3670 movem.l d2-d7/a2-a4,-(a7) ; intfin */
#define intfin (&intstart0[2])  /* movem.l (a7)+,d2-d7/a2-a4 (chunk 09 reads it as a long) */

/* ------------------------------------------------- statement handlers */

tokhandler dorunerrson, dorunerrsoff, dousepath, doexchange, dowbstartup,
    dopop, dorepeat, dolibjsr, doblibjsr, dosysjsr, dotokejsr, dovwait,
    doblitz, doamiga, doqamiga, doclrint, doclrerr, doerrfail, doseterr,
    dosetint, doputreg, dosetreg;
/* doamiga is also called with c ignored at end of compile (2644);
   dorunerrson also called at end of pass (2616). */

/* End SetErr; jumped to from doend (13966) */
tokhandler doendseterr;

/* setint2: dosetint body with d1 = new intstring value (-1 from dosetint) */
void setint2(WORD strflag);
/* toamode: common tail of doamiga/doqamiga, tok = goamiga/goqamiga */
void toamode(UWORD tok);
/* debugmode: if debugga, tokejsr($d500) */
void debugmode(void);

/* -------------------------------------------------------- Pop helpers */

void popreturn(void);                   /* Pop Gosub, ends with getchar3 */
void popnext2(void);                    /* Pop For: uses firstfor */
void popnext3(struct fornode *f);       /* emit stack drop of a For (donext 14211) */
void popnext(struct fornode *f);        /* debug: snxtchk jsr (donext 14163) */
void popselect2(void);                  /* Pop Select: uses firstsel */
void popselect(struct selnode *s);      /* drop a Select's stack (doendsel 13061) */

/* ------------------------------------------------------------- misc */

/* comdata+88 (editor extalloc = AllocMem, called via callregs with
   a6=SysBase).  Never returns NULL: on failure the editor longjmps away. */
APTR doallocmem(ULONG size, ULONG flags);

/* Exchange operand: asm d2.b type, d0 next char.  Emits lea x(a5),a2 for
   simple vars.  Returns the next char, *type = type byte (4 for pointers). */
UWORD excget(UWORD c, UBYTE *type);

/* asm: Z=1 if name ends with ".info" (case insensitive) */
BOOL endininfo(const char *name);

/* `Toke[,form]` parameter.  asm: in d0 + N, out d4 toke, d5 form, d0 next. */
UWORD gettokeps(UWORD c, UWORD *toke, UWORD *form);

/* Amiga library statement (from handlelib 3174).  c = current char,
   sub = token sub record (asm a3: +6.w lib#, +8.w LVO, +10 reg bytes, <0 end) */
void stamigalib(UWORD c, UBYTE *sub);

/* Fixed (#958): TRUE if an amigalib's register list (reg bytes, <0 end)
   has a4 or a5; then it has poked movem.l a4/a5,-(a7). */
BOOL amigaa4a5(const UBYTE *regs);

/* decimal ULONG -> buf (no leading zeros, NUL terminated).
   asm: d2 value, a0 buf; returns a0 = address of the NUL. */
char *makelong(char *buf, ULONG v);

/* ------------------------------------------------------------- allox */

/* All return asm NE: TRUE = buffers allocated (newallox also TRUE if only the
   fallback to the initial sizes worked - not reachable, #32). */
BOOL alloxinc(void);                    /* max sizes * 1.5 */
BOOL alloxadj(void);                    /* sizes actually used */
BOOL newallox(ULONG pcsize, ULONG libsize, ULONG data1size, ULONG data2size);
BOOL makeiallox(void);                  /* maxpc/maxlib/maxdata1/maxdata2 */
BOOL makeallox(ULONG pcsize, ULONG libsize, ULONG data1size, ULONG data2size);
void freeallox(void);
/* macrobuff for the Macro Buffer option (macrolen), fixed (#336): FALSE
   if it could not be allocated (the old buffer stays) */
BOOL makemacbuff(void);

/* ---------------------------------------------------- menus / residents */

void menu3(void);       /* only assembled if final==0 (debug token print) */
void menu5(void);       /* Make Resident */
void loadres(void);     /* load all residents of resbuff (own error recovery) */
void freeres(void);     /* reset resident lists, then freeres2 */
void freeres2(void);    /* UnLoadSeg all resbuff entries */
void runerropts(void);  /* runtime error options requester */

/* loader helpers: p = cursor in the loaded hunk, returns the advanced cursor */
UBYTE *fixtypes(UBYTE *p);
UBYTE *fixmacs(UBYTE *p);
UBYTE *fixconsts(UBYTE *p);
/* check list `first` for duplicate names (name offset nameoff); clasherr */
void checkclash(struct node *first, UWORD nameoff);

/* ------------------------------------------------------------ assembler */

void makeasmtab(void);  /* fill asmpnts[] from the tokens list (startup) */

/* getea modes (asm d5) */
enum {
    EA_DN = 0, EA_AN = 1, EA_IND = 2, EA_POSTINC = 3, EA_PREDEC = 4,
    EA_DISP = 5, EA_INDEX = 6, EA_ABSW = 7, EA_ABSL = 8, EA_PCDISP = 9,
    EA_PCINDEX = 10, EA_IMM = 11, EA_CCR = 12, EA_SR = 13, EA_USP = 14,
    EA_REGLIST = 15
};

/* asm getea: parse one effective address at srcptr (does getchar3 first).
   out: *mode (asm d5), *reg (asm d4: register 0-7, SP=7, or movem mask);
   index extension word in `extraword`; expression text via asmconst.
   Returns the next char (asm d0). */
UWORD getea(UWORD *mode, UWORD *reg);
UWORD getimm(UWORD *mode);              /* '#': asmconst, mode 11 */
/* asm: Z=1 if c is 0, ':' or ',' */
BOOL tstend2(UWORD c);
/* after d(An / d(PC: ')' -> mode 5, ',Xn[.w|.l])' -> mode 6 + extraword */
UWORD getrest(UWORD c, UWORD *mode);
/* register parsers: *reg = 0..7 (Dn), 8..15 (An) for getreg*; return next char */
UWORD getreg(UWORD *reg);               /* getchar3 first */
UWORD getreg2(UWORD c, UWORD *reg);
UWORD getan(UWORD *reg);                /* getchar3 first, must be An */
UWORD getan2(UWORD c, UWORD *reg);      /* c must be 'a' */
UWORD getan3(UWORD *reg);               /* digit 0-7 */
/* movem list; first = ASCII digit of the first reg (getmovem2: An).
   *mask = register mask (bit n: d0-d7 = 0-7, a0-a7 = 8-15); asm also set d5=15 */
UWORD getmovem(UWORD c, UWORD first, UWORD *mask);
UWORD getmovem2(UWORD c, UWORD first, UWORD *mask);
/* emit movem.l all-but-reg,-(a7); returns the mask for dounmove */
UWORD regmovems(UWORD reg);
void dounmove(UWORD unmove);            /* emit movem.l (a7)+,unmove */

/* ------------------------------------------------------ direct mode */

/* glue_direct.c (asm):
   directtrap - trap #15 handler (VBR+$bc): skips the 18 parameter bytes after
                the trap, saves PC/SR in ret15add/ret15cc, rte to directdo.
   directdo   - user mode: pushes a fake rtr frame [ret15cc, ret15add], saves
                d0-d7/a0-a6, calls directdo_c(frame); if it returns non-zero
                the frame's return PC is replaced by it (generated code);
                then (direxit) restores ccr/regs, flushc, rtr.
   Parameter block before ret15add: -18 code.l, -14 firstlocal.l,
   -10 firstglobal.l, -6 result.l (0 / error text), -2 blitz mode.w */
extern void directtrap(void);
extern void directdo(void);
/* C part of directdo/dircont: compile the line; returns dirpcat (enter the
   generated code, which jumps back to ret15add) or 0 after an error (error
   text stored at ret15add-6, asm dircont). */
ULONG directdo_c(struct m68kregs *r);

#endif
