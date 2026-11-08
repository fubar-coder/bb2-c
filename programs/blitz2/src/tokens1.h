/*
 * tokens1.h - chunk 08, blitz2.s 12136-13326: built-in token handlers part 1
 * (DefType, MaxLen, Free/Use, Else, CERR/CNIF/CSIF, Restore/Read/Data,
 * DC/DCB/DS/Even, Shared, Include/XInclude/IncBin, Macro, Select/Case,
 * Repeat/Forever/Until, If/While).  See notes/survey-08.md.
 */
#ifndef BLITZ2_TOKENS1_H
#define BLITZ2_TOKENS1_H

#include <exec/types.h>
#include "structs.h"

#ifndef NORETURN
#define NORETURN __attribute__((noreturn))
#endif

/* ------------------------------------------------ statement handlers
   All tokhandler: entered after getchar3 with c = current char/token
   (asm d0 + Z flag -> eos(c)); they leave the current char in lastchar
   (asm: return via reget/getchar3) as doline expects. */
tokhandler dodeftype;   /* 12139 DefType .t [var,...] */
tokhandler domaxlen;    /* 12168 MaxLen s$=n */
tokhandler dofree;      /* 12253 Free X(i) */
tokhandler douse;       /* 12266 Use X(i) */
tokhandler doelse2;     /* 12279 getchar3, then doelse (called by doline on Else) */
tokhandler doelse;      /* 12280 Else; ends via dothen (chunk 09) */
NORETURN void docerr(UWORD c);  /* 12293 CERR "text" -> err(namebuff) */
tokhandler docnif;      /* 12299 CNIF const (called by getchar cond-compile, 05) */
tokhandler docsif;      /* 12306 CSIF "a" op "b" (ditto) */
tokhandler dorestore;   /* 12351 Restore [label] */
tokhandler doread;      /* 12384 Read v,... */
tokhandler dodcb;       /* 12501 Dcb.s n,v  (asm: dodcb = dodcb2) */
#define dodcb2 dodcb
tokhandler dods;        /* 12532 Ds.s n (the storage written as 0, #969) */
tokhandler dodc;        /* 12548 Dc.s v,... */
tokhandler doeven4;     /* 12600 Even4: pc=(pc+3)&~3 */
tokhandler doeven8;     /* 12605 Even8: pc=(pc+7)&~7 */
tokhandler doeven;      /* 12610 Even (same address as alignpc) */
tokhandler dodata;      /* 12629 Data[.t] v,... */
tokhandler doshared;    /* 12716 Shared v,... */
tokhandler doxinclude;  /* 12760 XInclude "f" */
tokhandler doincbin;    /* 12772 IncBin "f" */
tokhandler doinclude;   /* 12811 Include "f" - does not return normally, see doinc2 */
tokhandler domacro;     /* 12849 Macro name ... End Macro.
                           asm: `addq #4,a7` drops the return into doline and
                           returns straight to the pass loop; implemented with
                           the "abort doline" mechanism of compile.h (chunk 02). */
tokhandler dowhile;     /* 12974 While cond */
tokhandler doendsel;    /* 13032 End Select (also bra'd to from chunk 09) */
tokhandler dodefault;   /* 13077 Default */
tokhandler docase;      /* 13130 Case expr */
tokhandler doselect;    /* 13175 Select expr */
tokhandler doforever;   /* 13228 Forever */
tokhandler dountil;     /* 13241 Until cond */
tokhandler doif;        /* 13272 If cond (doif -> doifm -> doif2) */

/* 12818 doinc2: include file namebuff (d2 = name length, from makeiname).
   asm: ends with `move.l passstack,a7 / jmp passnxt` - never returns; uses
   the pass-loop restart mechanism of compile.h (chunk 02). */
NORETURN void doinc2(UWORD namelen);

/* 13275 doif2: d1 = kind (0 If, IFK_WHILE While), d2 = 0 normal / -1 negate
   (Unless). Falls through into dothen (chunk 09) for one-line Ifs. */
void doif2(UWORD kind, WORD negate);

/* ------------------------------------------------ helpers */

/* 12196 getmaxel: c = token (must be a token, else syntaxerr); evaluates the
   index (word, into d(regat)) and emits the bounds check (debug) + shifts.
   Returns asm a2 = max-info end pointer E = (UBYTE*)lib + lib->maxinfo
   (fields at E-2..E-14, see LIBMAX_* in structs.h).  The asm also set the
   global mymaxlib (12251) = lib for Free/Use; they get it from a local
   variant now. */
UBYTE *getmaxel(UWORD c);

/* Fixed (#939): 1 if the object offset of the max info E is computed as a
   long ((max - 1) << shift > $7FFF), then getmaxel emitted objlongcode
   instead of the lsl.w and the caller adds with add.l */
int objlong(const UBYTE *e);

/* emits dn = (ULONG)(UWORD)dn << shifts (mulu #1 << shifts,dn) */
void objlongcode(UWORD dn, UWORD shifts);

/* 12611 alignpc: pc = (pc+1) & ~1, the pad byte written as 0 (#969; also
   doeven4/doeven8) */
void alignpc(void);

/* zeropc: pc = to, the bytes it skips written as 0 the way pokebyte writes
   (#969; alignpc, doeven4, doeven8, dods and pokewd's alignment) */
void zeropc(ULONG to);

/* 12315 compare: asm a0 = compare routine that sets the CCR, d1 = condition
   index (from collif, 0..11).  C: the caller runs the compare routine
   (numcom / strcom, chunk 07) and passes its sign as `res` (<0 / 0 / >0,
   signed).  Then dispatches through cifbras[cond]:
   pushes comflag on the conditional-compile stack and sets comflag to -1
   (condition true) or 0 (false); expects end of statement. */
void compare(int res, UWORD cond);

/* 1294 cifbras (dispatch table, owned here): index = collif code,
   NULL entries (3, 7, 11) are never selected.
   {ciflt,cifeq,cifgt,0, cifle,cifne,cifle,0, cifge,cifne,cifge,0} */
extern void (*const cifbras[12])(int res);
void cifeq(int res);    /* 12327 asm: beq cifdo / bra cifdont */
void cifne(int res);    /* 12329 */
void ciflt(int res);    /* 12331 */
void cifle(int res);    /* 12333 */
void cifgt(int res);    /* 12335 */
void cifge(int res);    /* 12337 */

/* 12978 / 12982 / 12985 / 12989 list freeing.
   head = address of the list head variable (asm a2), size = item size (d2),
   suboff = offset of a sub-list head in each item (d3, 0 = none),
   subsize = sub-item size (d4).  The head is cleared. */
void freereps(void);                                    /* firstrep, 8 */
void freesels(void);                                    /* firstsel, 14 */
void freeslist(APTR head, ULONG size);                  /* freelist, suboff 0 */
void freelist(APTR head, ULONG size, UWORD suboff, ULONG subsize);

/* ------------------------------------------------ globals */

extern WORD macline;                /* 12847 line of Macro start (macenderr) */
extern struct selnode *firstsel;    /* 13014 Select stack */
extern WORD sellineat;              /* 13173 line of last Select */
extern WORD iflineat;               /* 13271 line of last If */

/* ------------------------------------------------ code templates
   Writable copies of the original bytes (tools/asmlisting.py). */

/* 12165 maxprep1: lea 0(a5),a2 ($45ED, only 1st word used) / maxprep2:
   move.l a2,d0 */
extern UWORD maxprep1[];
#define maxprep2    (&maxprep1[2])

/* 12247 free..freef: move.l 0(a5),a3 / add d0,a3.  free+2 patched in place
   by dofree (max var offset); also read by chunk 07 (11360). */
extern UWORD free_[];
#define freef       (&free_[3])

/* 12348 rescode: move.l #0,0(a5) ($2B7C imm.l disp) / rescode2:
   move.l d0,0(a5).  Patched by uselib (04): rescode+6 = rescode[3],
   rescode2+2 = rescode[5] (a5 offset of the data pointer). */
extern UWORD rescode[];
#define rescode2    (&rescode[4])

/* 12962 thetst1..5, posbr, negbr (If test + branch) */
extern UWORD thetst1[];
#define thetst2     (&thetst1[1])   /* tst.w d0 */
#define thetst3     (&thetst1[2])   /* tst.l d0 */
#define thetst4     (&thetst1[3])   /* tst.l (a7)+ */
#define thetst5     (&thetst1[4])   /* jsr -48(a6) (SPTst), long */
#define posbr       (&thetst1[6])   /* beq.w * ($6700 $FFFE) */
#define negbr       (&thetst1[8])   /* bne.w * ($6600 $FFFE) */

/* 13015 Select push / compare templates, selbne, endsel */
extern UWORD selpushb[];
#define selpushw    (&selpushb[2])
#define selpushbf   (&selpushb[3])
#define selpushl    (&selpushb[3])
#define selpushlf   (&selpushb[4])
#define selpushs    (&selpushb[4])
#define selpushsf   (&selpushb[6])
#define selcomb     (&selpushb[6])
#define selcomw     (&selpushb[8])
#define selcombf    (&selpushb[9])
#define selcoml     (&selpushb[9])
#define selcomlf    (&selpushb[10])
#define selbne      (&selpushb[10]) /* bne.w * ($6600, 1st word used) */
#define endsel      (&selpushb[12]) /* addq #8,a7 ($504F; popselect ORs n<<9) */

/* 13070 endselstr..endselstrf..endselstrf2 (used by popselect, chunk 03) */
extern UWORD endselstr[];
#define endselstrf  (&endselstr[4])
#define endselstrf2 (&endselstr[5])

/* 13169 pcchk: move #"sE",-(a7) (long) / clrst: clr.l -(a7) */
extern UWORD pcchk[];
#define clrst       (&pcchk[2])

#endif
