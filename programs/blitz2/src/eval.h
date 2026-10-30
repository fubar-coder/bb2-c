/*
 * eval.h - chunk 06 (blitz2.s 8451-10683): object code emitters, type
 * conversion, the expression evaluator, operator code generators and the
 * expression elements.  See notes/survey-06.md.
 *
 * ---------------------------------------------------------------- conventions
 * Types (asm d2, low byte): T_BYTE..T_STRING from structs.h (0 = unknown /
 *   struct address); calcvar's flag bits TF_ADDRINA2 / TF_POINTER may be set.
 * regat (global, WORD): the data register Dn the generated code leaves the
 *   current value in.  A binary operator has its left operand in D(regat)
 *   and its right operand in D(regat+1); the result goes to D(regat).
 *   Callers set regat before evaluating (normally 0) and get it back
 *   unchanged, except: maineval with regat >= 6 forces push mode and leaves
 *   regat = 6 (setsvars resets it for the next statement).
 * Current char: every evaluator / element entry point RETURNS the current
 *   char/token (asm d0) after the expression, like getchar3; the asm Z flag
 *   is not meaningful there (callers compare the char).
 * Type in/out: asm d2 is both input (wanted type / type hint) and output
 *   (type got) -> passed as `UWORD *type`.
 * String results: string length (long) is on the generated program's stack,
 *   a3 = end of string, D(regat) = start (see maineval).
 */
#ifndef BLITZ2_EVAL_H
#define BLITZ2_EVAL_H

#include <exec/types.h>
#include "structs.h"

/* token numbers (asm: fnum equ 60, tnum = end of asm mnemonic tokens = 179,
   verified in build/blitz2.lst) - identical redefinitions elsewhere are ok */
#ifndef fnum
#define fnum 60
#endif
#ifndef tnum
#define tnum 179
#endif

/* asm-mnemonic tokens used as word operators in `operators` / getelement2.
   asm: (opX-opabcd)/$1c+$8000+fnum (asmmac index); verified against
   build/blitz2.lst: AND $8042, OR $808A, EOR $8073, LSL $807C, ASL $8044,
   LSR $807D, NOT $8089 (9728) */
#define tok_opand (0x8000 + fnum + 6)    /* AND */
#define tok_opasl (0x8000 + fnum + 8)    /* ASL */
#define tok_opasr (0x8000 + fnum + 9)    /* ASR */
#define tok_opeor (0x8000 + fnum + 55)   /* EOR */
#define tok_oplsl (0x8000 + fnum + 64)   /* LSL */
#define tok_oplsr (0x8000 + fnum + 65)   /* LSR */
#define tok_opnot (0x8000 + fnum + 77)   /* NOT */
#define tok_opor  (0x8000 + fnum + 78)   /* OR  */

/* ===================================================== object code emitters
   All write at `pc` (global ULONG, address in the object buffer).  If
   pc >= libat nothing is written, nomemleft = -1, pc still advances.  If
   cfetchmode == 0, bigpc follows the highest pc.  (asm: a4 = new pc after the
   call; use `pc` instead.) */

/* blitz2.s:8476 pokewd - poke d1.w into pc (pc rounded up to even first) */
void pokewd(UWORD w);
/* blitz2.s:8472 pokel - two pokewd, high word first */
void pokel(ULONG l);
/* blitz2.s:8474 pokelc - second half of pokel: pokes the LOW word of l
   (asm entry with d1 already swapped once) */
void pokelc(ULONG l);
/* blitz2.s:8496 pokebyte - poke d1.b, no alignment */
void pokebyte(UBYTE b);
/* blitz2.s:8514 pokecode - a0 = start, a1 = end (exclusive); always pokes
   at least one word (do-while) */
void pokecode(const UWORD *start, const UWORD *end);
/* blitz2.s:8525 pokecode2 - same semantics */
void pokecode2(const UWORD *start, const UWORD *end);
/* a5/a4 variants: when local variables are addressed through a4 the a5
   register field of the opcode is turned into a4 before poking:
     pokewda5s: if varmode, clear bit0  (source reg a5 -> a4)
     pokewda5d: if varmode, clear bit9  (dest reg)
     pokela5s:  if varmode, clear bit16 (source reg in high word)
     pokela5d:  if procmode (sic, not varmode), clear bit25 */
void pokewda5s(UWORD w);
void pokewda5d(UWORD w);
void pokela5s(ULONG l);
void pokela5d(ULONG l);

/* blitz2.s:8512 highest pc emitted */
extern ULONG bigpc;

/* blitz2.s:10279 pokemovem - if (mask.w = low word of l) != 0 pokel(l) */
void pokemovem(ULONG l);
/* blitz2.s:10283 savereg - movem longs to save/restore d0/d1 around a lib
   call according to regat/fuckpos: *push = $48e7<<16|mask,
   *pull = $4cdf<<16|mask (asm d4/d5) */
void savereg(ULONG *push, ULONG *pull);
/* blitz2.s:10502 savem - emit movem.l d0-d(regat-1),-(a7) if regat != 0;
   returns the predecrement mask (asm d3, 0 = nothing emitted) */
UWORD savem(void);
/* blitz2.s:10515 makefjsr - emit jsr addr.l with relocation (addoff) */
void makefjsr(ULONG addr);
/* blitz2.s:9623 / 9633 move D(regat),D(regat+1) to d0,d1 / d0 to D(regat);
   nothing emitted when regat == 0 */
void ritetod0(void);
void ritefromd0(void);

/* ========================================================== type conversion */

/* blitz2.s:8693 convtype - "convert type d3 to d2": emit code converting
   D(regat) from type `from` to type `to` via convtable (may be nothing or
   noconerr).  The caller's d2 is `to` afterwards. */
void convtype(UWORD from, UWORD to);
/* blitz2.s:8686 convtypef - as convtype, but regat+1 is live (fuckpos++) */
void convtypef(UWORD from, UWORD to);

/* blitz2.s:14693 convtable - 7x7 conversion routines,
   index (from-1)*7 + (to-1), NULL = no conversion.  Data defined in eval.c
   (moved from chunk 10's range: tables live with their dispatcher). */
extern convhandler *const convtable[49];

/* conversion routines (convtable entries), all operate on D(regat) */
convhandler bytetoword, bytetolong, bytetoquick, bytetofloat;
convhandler wordtobyte, wordtolong, wordtoquick, wordtofloat;
convhandler longtobyte, longtoword, longtoquick, longtofloat;
convhandler quicktobyte, quicktoword, quicktolong, quicktofloat;
convhandler floattobyte, floattoword, floattolong, floattoquick;
convhandler stringtolong;
/* blitz2.s:8573 overchk - emit call of overflow check lib routine `token`
   (only when debugger on, overflow option on, not cfetchmode) */
void overchk(UWORD token);
/* blitz2.s:8769 sbtolong - make string base into a long in D(regat) */
void sbtolong(void);

/* nesting count "right operand live in d1" (savereg saves d1 too) */
extern WORD fuckpos;

/* ===================================================== expression evaluator
   asm maineval inputs: d1 push flag, d2 wanted type, d3 "unknown" flag:
     push   0 = no push, < 0 = push result (move.w/.l Dn,-(a7)),
            > 0 = push the type word when known (moveq #type / move.w)
     *type  wanted type, 0 = whatever comes (in) / type got (out)
     unknown 0 = convert result to *type, != 0 = *type is only a hint
   Returns the current char after the expression (asm d0). */
UWORD maineval(WORD push, UWORD *type, WORD unknown);   /* 8860 */
UWORD maineval2(WORD push, UWORD *type, WORD unknown);  /* 8879, no regat>=6 check */

/* entry variants (asm: which d1/d3 they load) */
UWORD eval(UWORD *type);        /* 8857  push 0, convert            */
UWORD bakeval(UWORD *type);     /* 8855  bakup; eval                */
UWORD evalu(UWORD *type);       /* 8847  push 0, unknown            */
UWORD bakevalu(UWORD *type);    /* 8845  bakup; evalu               */
UWORD evalu2(UWORD *type);      /* 8851  push type, unknown         */
UWORD peval(UWORD *type);       /* 8841  push result, convert       */
UWORD bakpeval(UWORD *type);    /* 8839  bakup; peval               */
UWORD pevalu(UWORD *type);      /* 8835  same as peval (unused)     */
UWORD bakpevalu(UWORD *type);   /* 8833  bakup; pevalu              */
/* array index evaluation (calcvar): result in D(regat), type fixed */
UWORD arreval(void);            /* 8828  long, no push              */
UWORD sharreval(void);          /* 8823  word, no push              */
UWORD arrevalchk(void);         /* 8809  debug: arreval + arrszchk, else sharreval */
void arrszchk(void);            /* 8812  emit cmpi.l #$10000,Dn / bcs / jsr arrerr */

/* "don't convert" flag of the current maineval (asm: unknown dc 0) */
extern WORD unknown;

/* precedence stack (pushprec/popprec); reset by prepstack (chunk 02).
   asm bound: precstackf = end label (64 entries) */
extern struct precent precstack[64];
#define precstackf (&precstack[64])
extern struct precent *precsp;

/* constant folding stack (one entry per element, see struct forthent);
   reset by prepstack.  asm bound: `cmp.l #forthsp,a1` = end of forthstack */
extern struct forthent forthstack[32];
extern struct forthent *forthsp;

/* Constant folding (eval5, blitz2.s 9007-9083), DIFF/G: when both operands
   are constants, eval5 appends `move.l Dn,d0 / rts` to the code generated
   for the subexpression and RUNS it at compile time:
       struct m68kregs r;   (d2/a5 of the compiler need no saving in C)
       r.a[6] = (ULONG)MathBase;                         asm: maths
       r.a[5] = (ULONG)&MathBase - ffplib->varoff;       asm: lea maths,a5
                                                            sub 10(ffplib),a5
       flushc();
       callregs((APTR)left->pc, &r);   value = r.d[0]
   then rewinds pc to left->pc, emits move.l/move.w #value,Dn and frees the
   offset nodes added since left->firstoff (exec FreeMem, 8 bytes). */

/* ================================================ operator code generators
   blitz2.s:14704 operators - 0-terminated table (moved here from chunk 10's
   range).  Entries named in the asm: */
extern struct opentry operators[];
#define opeq (&operators[15])
#define opne (&operators[16])
#define oplt (&operators[17])
#define ople (&operators[18])
#define opgt (&operators[19])
#define opge (&operators[20])

ophandler doeqb, doneb, doltb, doleb, dogtb, dogeb;
ophandler doeqw, donew, doltw, dolew, dogtw, dogew;
ophandler doeql, donel, doltl, dolel, dogtl, dogel;
ophandler doeqf, donef, doltf, dolef, dogtf, dogef;
ophandler doeqs, dones, dolts, doles, dogts, doges;
ophandler doplusb, doplusw, doplusl, doplusf, addstrings;
ophandler dominusb, dominusw, dominusl, dominusf;
ophandler dotimesb, dotimesw, dotimesq, dotimesl, dotimesf;
ophandler dopow;
ophandler domodb, domodw, domodl, domodq, domodf;
ophandler dodivb, dodivw, dodivl, dodivq, dodivf;
ophandler doandb, doandw, doandl;
ophandler doorb, doorw, doorl;
ophandler doeorb, doeorw, doeorl;
ophandler dolslb, dolslw, dolsll;   /* asm: 3 labels, one routine */
ophandler dolsrb, dolsrw, dolsrl;
ophandler doasrb, doasrw, doasrl;
ophandler bittst, bitset, bitclr, bitchg;

/* ================================================================ elements
   Element routines: in *type = type hint (asm d2), out *type = type got;
   return the current char after the element (asm d0). */

/* blitz2.s:9866 / 9883 '$' hex / '%' binary number (current char is the
   '$'/'%', the digits are read here); tail into fetchnum2 (chunk 07), so
   the result follows fetchnum2's contract: dfetch != 0 -> the value,
   else the code is emitted and the next char/token is returned. */
ULONG fetchhex(UWORD *type);
ULONG fetchbin(UWORD *type);

/* blitz2.s:9910 ampersand2 - &var: getchar3, getvname, fetchvar, calcvar
   and emit the address (or string pointer) into D(regat).
   *type out = calcvar's d2 (with TF_ flags).  Returns the current char. */
UWORD ampersand2(UWORD *type);

/* blitz2.s:8709 getvname - in c = first char of the name (already read).
   Copies the name to namebuff (type suffix to namebuff2), sets flagmask
   (1 '*', 5 '@', |2 '(', |8 '$'); '\' redirects srcptr to usedpath
   (usedfrom = old srcptr-1).  Returns the char after the name (after a
   '.'/'$' suffix: the char at the suffix - srcptr is rewound by resloc).
   Empty name -> syntaxerr.
   The name length (asm d2, needed by fetchvar / findproc / dolet2) is left
   in getvnamelen (DIFF: asm register d2). */
UWORD getvname(UWORD c);
extern UWORD getvnamelen;
/* srcptr saved while reading from usedpath (restored by getchar) */
extern UBYTE *usedfrom;

/* blitz2.s:10352 makealab - read a label name (makename2) and return its
   node, creating it and/or an outstanding reference (labref at pc+2) if
   not defined yet.  *c (may be NULL) = char after the name.
   Errors: illlaberr (it's a constant), referr (other proc). */
struct label *makealab(UWORD *c);
/* 10353: name (length len) already in namebuff */
struct label *makealab2(UWORD len, UWORD *c);

#endif
