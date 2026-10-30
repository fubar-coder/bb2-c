/*
 * tokens2.h - chunk 09, blitz2.s 13327-14655: built-in token handlers part 2
 * (Then, On..Goto/Gosub, Goto, Gosub, Return, Statement/Function, End ...,
 * NewType, EndIf/Wend, Stop/Cont, For/Next, Let, Dim).
 * See notes/survey-09.md.
 *
 * Statement handlers are tokhandlers: entered after getchar3 with the current
 * char/token c (asm d0; Z flag = eos(c)).
 */
#ifndef BLITZ2_TOKENS2_H
#define BLITZ2_TOKENS2_H

#include <exec/types.h>
#include "structs.h"

#ifndef NORETURN
#define NORETURN __attribute__((noreturn))
#endif

/* ------------------------------------------------------------- If / Then */

/* blitz2.s:13327 dothen - bakup; lastchar=':' (next reget sees end of
   statement).  doif2 (chunk 08) FALLS THROUGH into this after flagging a
   one-line If; doelse tail-calls it.  c is ignored. */
void dothen(UWORD c);
/* blitz2.s:13329 ifdone - just rts (target of doif2's `beq ifdone`);
   chunk 08 may simply return instead. */
void ifdone(void);

/* blitz2.s:13837 doendif2 - close If/While node `node` (asm a0) of kind `kind` (asm d1)
   (0 = If, 1 = While): While emits `bra.w` back to node->startpc, then
   patches the bcc disp at node->brapc-2 (unless nomemleft).
   bigiferr if out of range.  Does not unlink/free the node. */
void doendif2(struct ifnode *node, UWORD kind);
/* blitz2.s:13869 freetheifz - unlink `node` (must be firstif) and FreeMem it
   (14 bytes). */
void freetheifz(struct ifnode *node);

/* blitz2.s:13860 dowend - "Wend" (bakup, then close a While) */
void dowend(UWORD c);
/* blitz2.s:13863 mydoendif2 - "EndIf" (bakup, then close an If) */
void mydoendif2(UWORD c);

/* ------------------------------------------------------- goto / gosub */

void doongo(UWORD c);       /* 13370 On expr Goto|Gosub l1[,l2...] */
void dogosub(UWORD c);      /* 13429 */
void dogoto(UWORD c);       /* 13439 */
void doreturn(UWORD c);     /* 13477 */

/* -------------------------------------------------- Statement / Function */

/* blitz2.s:13448 eos - Statement/Function Return (also atokens "AsmExit"):
   emits `jmp abs.l` to the proc epilogue (firstpend list).  badpenderr if not
   inside a proc.  Renamed: the asm label clashes with BOOL eos(UWORD) of
   parse.h.  Returns with the current char unchanged (asm: d0 untouched). */
void eos_(UWORD c);
void dostate(UWORD c);      /* 13494 Statement name{...} / Statement Return */
void dofunc(UWORD c);       /* 13498 Function[.t] name{...} / Function Return */

/* blitz2.s:14104 droplocals - move firstlocal/firstglob into a new alllocals
   record and clear both heads (debug: also store them into the inline longs
   after the proc's trap #4). */
void droplocals(void);

extern WORD procnum;        /* 13490 current proc number (0 = main) */
extern WORD procnum2;       /* 13491 proc counter */
extern UWORD prolineat;     /* 13492 line of the open proc (errchx) */
extern ULONG localslist;    /* 13488 unused */
extern ULONG proclocals;    /* 13489 output address of the 2 debug longs */

/* ------------------------------------------------------- End and others */

/* blitz2.s:13876 doend - "End" (eos(c): plain End) or End NewType/If/While/
   Statement/Function/SetInt/Select/SetErr. */
void doend(UWORD c);
void dostop(UWORD c);       /* 13975 */
NORETURN void docont(UWORD c);  /* 13983 always badconterr */
void domwait(UWORD c);      /* 13715 MouseWait */

/* blitz2.s:13985 nop - poke $4e71.  Renamed (clashes with common nop macros). */
void nop_(void);

/* ---------------------------------------------------------------- NewType */

void donewtype(UWORD c);    /* 13719 NewType .name */
/* blitz2.s:13740 typemode - one NewType field line; reached through
   modetable[0] (smode, chunk 02) with c = first char/token of the line.
   Returns normally; the caller (smode/handlechar) must then end the
   statement itself (asm: smode had dropped dostatement's frame). */
void typemode(UWORD c);

/* --------------------------------------------------------- For / Next */

void dofor(UWORD c);        /* 14228 For var=a To b [Step c] */
void donext(UWORD c);       /* 14130 Next [var[,var...]] */
/* blitz2.s:13988 freefors - free the whole firstfor list */
void freefors(void);
extern UWORD forlineat;     /* 14225 line of the last For (errchx) */
extern UWORD fortemp;       /* 14226 */

/* ---------------------------------------------------------- Let / Dim */

void dolet(UWORD c);        /* 14338 Let */
/* blitz2.s:14340 dolet2 - assignment after getvname.
   in:  c = char after the var name (asm d0), len = name length (asm d2 from
        getvname), name in namebuff, srcptr just behind it, letstart set.
   out: returns the terminating char/token (asm d0);
        *type = destination type/flags (asm d2: low byte T_*, TF_ADDRINA2,
        TF_POINTER), *offset = var offset (asm d3).  Either may be NULL.
   No '=' after the name rewinds srcptr to letstart-1 (`a+1` == `a=a+1`). */
UWORD dolet2(UWORD c, UWORD len, UWORD *type, WORD *offset);
void dodim(UWORD c);        /* 14513 Dim [List] a(..)[,...] */

extern UWORD labcnt;        /* 13368 On..Goto label count */
extern UWORD pcodd;         /* 14509 byte counter for (a2)+ alignment */
extern UWORD listadd;       /* 14510 8 = Dim List */
extern UWORD listsize;      /* 14511 */

/* ------------------------------------------------------------ templates */

/* On..Goto/Gosub dispatch code (copied, then patched: +10 = label count,
   word at skip+2 = labcnt*4+2; the label table follows). */
extern UWORD ongotocode[];          /* 30 bytes */
#define gc1skip      (&ongotocode[13])
#define ongotocodef  (&ongotocode[15])
extern UWORD ongosubcode[];         /* 30 bytes */
#define gc2skip      (&ongosubcode[13])
#define ongosubcodef (&ongosubcode[15])
extern UWORD ongcode[];             /* 34 bytes, debug Gosub */
#define goset        (&ongcode[12]) /* move #'gS',-(a7), also pokel'd alone */
#define gc3skip      (&ongcode[15])
#define ongcodef     (&ongcode[17])
extern UWORD forset[];      /* 14221 move #'fO',-(a7) */
extern UWORD thebra[];      /* 14223 bra.w * */
extern UWORD thebgt[];      /* 14336 bgt.w * */
extern UWORD add1a2[];      /* 14508 addq #1,a2 */
extern UWORD setles[];      /* 14654 move #0,d4 (only the opcode word used) */

#endif
