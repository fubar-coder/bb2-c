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
#include "exprtree.h"

/* ===================================================== object code emitters
   All write at `pc` (global ULONG, address in the object buffer).  If
   pc >= libat nothing is written, nomemleft = -1, pc still advances.  If
   cfetchmode == 0, bigpc follows the highest pc.  (asm: a4 = new pc after the
   call; use `pc` instead.) */

/* blitz2.s:8476 pokewd - poke d1.w into pc (made even first, pad 0: #969) */
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

/* the nesting of maineval2 (no asm counterpart): 1 inside a statement's
   expression, more inside the parameters or indices in it; for the
   extension points of ext.h (ext_evalvalue) */
extern UWORD evaldepth;

/* The code of an expression is written after it is built (no asm
   counterpart; exprtree.h, eval.c): a part of it that shared code writes
   while an expression is built (a parameter pushed, a path's step, a
   procedure's default arguments) is an action.  emitact records the
   action fn (with p, x and y) in the expression being built, with regat,
   varmode and evaldepth as they are now, or runs it at once outside one;
   stateact the same for one that writes no code (a position, a state).
   They return the action (outside an expression: kept in the arena until
   the statement ends).  exprbuildreset: prepstack. */
struct expract *emitact(exprtree_actfn *fn, void *p, ULONG x, UWORD y);
struct expract *stateact(exprtree_actfn *fn, void *p, ULONG x, UWORD y);
void exprbuildreset(void);
/* a constant computed in the middle of an expression being built
   (evalconst2): the expression set aside, then back */
void exprsuspend(struct exprunit *s);
void exprresume(struct exprunit *s);
/* the element being built is a constant of the fold (ELF_FOLD); the
   constant l (its code the last action) built: the facts of #978 and
   #1186 for the comparison it is an operand of */
BOOL elemconst(void);
void constrec(struct exprnode *l);
/* findtoke and uselib for an expression being built: the use now, the
   library's relocations where its code is written (eval.c) */
UBYTE *findtokeact(UWORD tok, struct blitzlib **libp);
struct blitzlib *uselibact(UWORD libnum);
/* the action of one word of code (x), or of two (x with ACT_TWO, y) */
void pokeact(struct expract *a);
#define ACT_TWO 0x10000UL

/* Intrinsics (no asm counterpart; openspec/specs/compiler/intrinsics,
   docs/blitz2/extensions.md): a library function compiled as inline code
   instead of a call, in eval.c's table intrinsics[].  libfunction arms
   one for its argument; these are its points in the shared code.
   intrinsic_let (dolet2): the right side of an assignment to a number
   begins (for Frac, whose Quick result is used only there).
   intrinsic_argpush (getap): the push flag for an argument of unknown
   type, 2 for the argument of an armed intrinsic (maineval2 then pushes
   no type word for a Quick the intrinsic accepts), else 1 (evalu2).
   intrinsic_code (libjsrcode, in place of userjsr): emits the inline code
   if the argument was accepted; FALSE = make the call.  (A string
   intrinsic, Len or Asc of a plain string variable, is compiled by
   libfunction itself, without these points.) */
void intrinsic_let(void);
WORD intrinsic_argpush(void);
BOOL intrinsic_code(void);

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


/* Constant folding (eval5, blitz2.s 9007-9083; eval.c constfold, called by
   binopact), DIFF/G: when both operands are constants (BNF_FOLD), the fold
   appends `move.l Dn,d0 / rts` to the code generated
   for the subexpression and RUNS it at compile time:
       struct m68kregs r;   (d2/a5 of the compiler need no saving in C)
       r.a[6] = (ULONG)MathBase;                         asm: maths
       r.a[5] = (ULONG)&MathBase - ffplib->varoff;       asm: lea maths,a5
                                                            sub 10(ffplib),a5
       flushc();
       callregs((APTR)start, &r);   value = r.d[0]   (start: the left operand's code)
   then rewinds pc to start, emits move.l/move.w #value,Dn and frees the
   offset nodes added since the left operand began that lie in the folded code
   (exec FreeMem, 8 bytes; fixed #1049: those of a library linked in
   meanwhile stay). */

/* ================================================ operator code generators
   blitz2.s:14704 operators - 0-terminated table (moved here from chunk 10's
   range).  Entries named in the asm: */
extern struct opentry operators[];
#define opdiv (&operators[3])
#define opmod (&operators[13])
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
   the result follows fetchnum2's contract: data -> the value, else the
   code is emitted and the next char/token is returned. */
ULONG fetchhex(UWORD *type, BOOL data);
ULONG fetchbin(UWORD *type, BOOL data);

/* blitz2.s:9910 ampersand2 - &var: getchar3, getvname, fetchvar, calcvar
   and emit the address (or string pointer) into D(regat).
   *type out = calcvar's d2 (with TF_ flags).  Returns the current char. */
UWORD ampersand2(UWORD *type);
/* change debug-constant: at the start of a compile, a resident's #__debug
   (AmiBlitz3's all.res has one, =0, used by its macros) gets this
   compile's value, as AmiBlitz3 sets its entry at the start of every
   compile (AmiBlitz3.ab3 3422-3443).  Only the residents' constants (up to
   lastrescon); none is added, so a resident made with Create Resident
   stays as it was. */
void fixdebugconst(void);

/* blitz2.s:8709 getvname - in c = first char of the name (already read).
   Copies the name to namebuff (type suffix to namebuff2), sets flagmask
   (1 '*', 5 '@', |2 '(', |8 '$'); '\' redirects srcptr to usedpath
   (usedfrom = old srcptr-1).  Returns the char after the name (after a
   '.'/'$' suffix: the char at the suffix - srcptr is rewound by resloc).
   Empty name -> syntaxerr.
   The name length (asm d2, needed by fetchvar / findproc / dolet2) goes
   to *namelen (DIFF: asm register d2). */
UWORD getvname(UWORD c, UWORD *namelen);
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
