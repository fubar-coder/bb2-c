/*
 * eval.c - chunk 06, blitz2.s 8451-10683: poking object code, type
 * conversion, the expression evaluator (precedence climbing with
 * compile-time constant folding), operator code generators and the
 * expression elements (numbers, variables, functions, strings ...).
 * See notes/survey-06.md and eval.h.
 *
 * Register mapping used throughout:
 *   asm d0 (current char)   -> return value UWORD c
 *   asm d2 (type, in/out)   -> UWORD *type
 *   asm d3 (offset/value)   -> parameters
 *   asm d1 (opcode)         -> local `opcode`
 *   regat                   -> global (D register of the result)
 */
#include "blitz2.h"
#include "exprtype.h"
#include "exprtree.h"
#include "exprfold.h"
#include "exprtyping.h"
#include <proto/exec.h>
#include <string.h>

/* exprtype.c's types are those of structs.h */
_Static_assert((int)ET_BYTE == (int)T_BYTE && (int)ET_WORD == (int)T_WORD && (int)ET_LONG == (int)T_LONG
               && (int)ET_QUICK == (int)T_QUICK && (int)ET_FLOAT == (int)T_FLOAT
               && (int)ET_ADDR == (int)T_ADDR && (int)ET_STRING == (int)T_STRING,
               "exprtype.h: the types of structs.h");

/* ======================================================= local templates */
/* Code copied into the generated program; bytes from build/blitz2.lst
   (tools/asmlisting.py), checked against the shipped binary. */

/* blitz2.s:8600 overchk .code1/.code2/.code3 */
static UWORD overchk_code[] = {
    0x2F00,                            /* .code1 move.l d0,-(a7) */
    0x201F,                            /* .code2 move.l (a7)+,d0 */
    0x2000,                            /* .code3 move.l d0,d0 */
};

/* blitz2.s:8767 ceos */
static UWORD ceos[] = {
    0x421B,                            /* ceos clr.b (a3)+ */
};

/* blitz2.s:8803 somecode..codeisok (16 bytes) */
static UWORD somecode[] = {
    0x0C80, 0x0001, 0x0000,            /* +0  somecode cmpi.l #65536,d0 */
    0x6500, 0x0008,                    /* +6  somecode2 bcs.w codeisok */
    0x4EB9, 0x0000, 0x0000,            /* +10 jsr 0 */
};
#define somecode2 (&somecode[3])

/* blitz2.s:8964 ststfix / 8966 pushut */
static UWORD ststfix[] = {
    0x584F,                            /* ststfix addq.w #4,a7 */
    0x3F00,                            /* pushut move d0,-(a7) */
};
#define pushut (&ststfix[1])

/* blitz2.s:9178 mseq..cmp2 */
static UWORD mseq[] = {
    0x57C0,                            /* +0  mseq seq d0 */
    0x56C0,                            /* +2  msne sne d0 */
    0x5DC0,                            /* +4  mslt slt d0 */
    0x5FC0,                            /* +6  msle sle d0 */
    0x5EC0,                            /* +8  msgt sgt d0 */
    0x5CC0,                            /* +10 msge sge d0 */
    0xB000,                            /* +12 cmp0 cmp.b d0,d0 */
    0xB040,                            /* +14 cmp1 cmp d0,d0 */
    0xB080,                            /* +16 cmp2 cmp.l d0,d0 */
};
#define msne (&mseq[1])
#define mslt (&mseq[2])
#define msle (&mseq[3])
#define msgt (&mseq[4])
#define msge (&mseq[5])
#define cmp0 (&mseq[6])
#define cmp1 (&mseq[7])
#define cmp2_ (&mseq[8])

/* blitz2.s:9947 addrcode..addrcodef (8 bytes, patched in place by doaddr) */
static UWORD addrcode[] = {
    0x206D, 0x0000,                    /* +0 addrcode move.l 0(a5),a0 */
    0xD0C0,                            /* +4 add d0,a0 (add.l: Fixed (#939)) */
    0x2008,                            /* +6 move.l a0,d0 */
};
#define addrcodef (&addrcode[4])

/* blitz2.s:9966 usedcode..usedfix (40 bytes, patched in place by doused) */
static UWORD usedcode[] = {
    0x202D, 0x0000,                    /* +0  usedcode move.l 0(a5),d0 */
    0x6604,                            /* +4  bne.s ucodeskip */
    0x70FF,                            /* +6  moveq #-1,d0 */
    0x6006,                            /* +8  bra.s usedcodef */
    0x90AD, 0x0000,                    /* +10 ucodeskip sub.l 0(a5),d0 */
    0xE048,                            /* +14 ucodediv lsr #8,d0 */
    0x202D, 0x0000,                    /* +16 usedcode2 move.l 0(a5),d0 */
    0x6604,                            /* +20 bne.s ucodeskip2 */
    0x70FF,                            /* +22 moveq #-1,d0 */
    0x6008,                            /* +24 bra.s usedcodef2 */
    0x90AD, 0x0000,                    /* +26 ucodeskip2 sub.l 0(a5),d0 */
    0xE048,                            /* +30 lsr #8,d0 */
    0xE048,                            /* +32 ucodediv2 lsr #8,d0 */
    0x2F00,                            /* +34 usedprep move.l d0,-(a7) */
    0x201F,                            /* +36 useddone move.l (a7)+,d0 */
    0x2000,                            /* +38 usedfix move.l d0,d0 */
};
#define ucodeskip  (&usedcode[5])
#define ucodediv   (&usedcode[7])
#define usedcodef  (&usedcode[8])
#define usedcode2  (&usedcode[8])
#define ucodeskip2 (&usedcode[13])
#define ucodediv2  (&usedcode[16])
#define usedcodef2 (&usedcode[17])
#define usedprep   (&usedcode[17])
#define useddone   (&usedcode[18])
#define usedfix    (&usedcode[19])

/* blitz2.s:10615 deca3 */
static UWORD deca3[] = {
    0x534B,                            /* deca3 subq #1,a3 */
};

/* ============================================================== globals */

ULONG bigpc = 0;            /* 8512 */
UBYTE *usedfrom = 0;        /* 8707 */
WORD unknown = 0;           /* 8801 */
/* 8962: stack use at the start of the current maineval2 (dynamic scope:
   sbtolong reads it, reached through convtable handlers that take no
   parameters; saved/restored by maineval2 like the asm) */
static WORD thisstuse = 0;
WORD fuckpos = 0;           /* 10303 */
UWORD evaldepth = 0;        /* no asm counterpart (eval.h) */
/* Fixed (#1178), no asm counterpart: D(regat) holds a raw Quick (16.16)
   while the type so far is Long.  varcont sets it instead of cutting the
   Quick to a whole number at fetch; every consumer of the value takes
   the Quick with its fraction (operate, mixedop) or rounds it down where
   the original's value stays (rawcut: shifts, logic operators, Mod, Not,
   the end of the expression) */
static BOOL rawq = FALSE;
/* Fixed (#1182), no asm counterpart: the type of D(regat) was lowered by
   a comparison: the value is a comparison's boolean (a Word, as cpoke
   leaves it), or computed from one, in a type below the type so far.
   compareop's result sets it; getelement2 clears it; a bracket, Not and
   a minus keep the one of their operand; any other operator gives it to
   its result if one of its operands has it (operate), a nested evaluation
   (maineval2: a function argument, an array index) starts and ends
   without it */
static BOOL cmpres = FALSE;
/* Fixed (#1179), no asm counterpart: the next maineval2 evaluates a later
   array index (idxnext, set by arrevalchk), the current one does
   (laterindex, saved by maineval2: a nested evaluation, a function
   argument or the index of an index, starts without it), and the
   operator doop runs has two Word values (wordop, set by doop: both
   forthent.wordval): dotimesl and dodivl compute it inline */
static BOOL idxnext = FALSE;
static BOOL laterindex = FALSE;
static BOOL wordop = FALSE;
/* the intrinsics (no asm counterpart, see intrinsics[]): the number of
   elements getelement2 has begun, and where the right side of the last
   assignment began (intrinsic_let: its evaldepth and the number of its
   first element) */
static ULONG elemcount = 0;
static UWORD letdepth = 0;
static ULONG letelem = 0;
/* the state of the armed intrinsic (intrcur): INTR_ARMED until getap
   takes its argument, INTR_ARG while maineval2 compiles it, INTR_QUICK
   when maineval2 accepted it (no type word), INTR_DONE when
   intrinsic_code emitted the code.  Nested calls in the argument save and
   restore it (libfunction). */
enum { INTR_IDLE, INTR_ARMED, INTR_ARG, INTR_QUICK, INTR_DONE };
static const struct intrinsic *intrcur = NULL;
static UWORD intrstate = INTR_IDLE;
/* Fixed (#1070): the element getelement2 fetches next is negated (an odd
   number of '-' in front of it); it hands a literal's sign on as litneg */
static BOOL negnext;

/* ============================================ the build and the emission
   No asm counterpart.  An expression is built before its code is written:
   it is read, typed, its variables and labels made and its libraries used
   in the order of the source, and each part of its code is an action
   (exprtree.h, struct expract), recorded in the order the original wrote
   that code.  The evaluation that began the unit (maineval2 outside one)
   runs the actions when the whole expression is built.  An action writes
   its code from its node and the emitter's state (pc, lasta6, sbgot,
   stackuse, the facts of the code before it...), with regat, varmode and
   evaldepth as they were where its part was read (ctx).  What the build
   decides from the code the original had written by then, it decides from
   what it has read and recorded: the records below. */

/* the forthstack (blitz2.s:9715) as the build has it: per pending element
   whether it is a constant (constgot 0) and a Byte or Word value (#1179);
   the fold runs from the start of its left operand's code (constfold) */
static struct {
    UWORD constgot;
    UBYTE wordval;
} bforth[32];
static UWORD nbforth = 0;

/* the actions recorded (their number), the last one that writes code;
   the action of the last conversion a value's code ends in (#1177: the
   conversion a comparison drops), the last literal made a whole number and
   the minus signs after it (#1186: fraclitfloat), and whether a string
   base is got (sbasegot, set by makesbase's code); whether the code
   built may use the string work area or a3 (bstrwork, a unit's strwork) */
static ULONG actseq = 0, codeseq = 0, prevcodeseq = 0;
static struct {
    ULONG seq;
    UWORD from, to, depth, relocs;
    struct exprnode *leaf;
} bconv;
static struct {
    ULONG seq, before, last;
    struct exprnode *leaf, *neg;     /* the literal, its last run of minus signs */
    UWORD negs;                      /* the minus signs */
} blit;
static BOOL bsbase = FALSE, bstrwork = FALSE;
/* the action of the last value whose code ends in a byte's ext.w, of the
   last integer constant (the code before it, its leaf), for #978 */
static ULONG bbyte = 0;
static struct {
    ULONG seq, before;
    struct exprnode *leaf;
} bcst;

/* no asm counterpart: the state of the build reset (prepstack).
   Fixed (#1215): also the states a compile error inside an expression
   leaves set (the next index, the next element's minus sign, the armed
   intrinsic), so that they do not reach the next statement or compile. */
static void p5reset(void);
/* the unit being built has code of the typing pass of rule P+ (#1216, unitwrite) */
static BOOL p5unit = FALSE;

void exprbuildreset(void)
{
    nbforth = 0;
    bconv.seq = blit.seq = bcst.seq = bbyte = 0;
    p5reset();
    idxnext = FALSE;
    negnext = FALSE;
    intrcur = NULL;
    intrstate = INTR_IDLE;
}

/* no asm counterpart: the state an action is written in, recorded where
   it is read */
static void actctx(struct expract *a)
{
    a->ctx[0] = regat;
    a->ctx[1] = (UWORD)varmode;
    a->ctx[2] = evaldepth;
}

/* no asm counterpart (exprtree.h): the action a run, in the state it was
   recorded in */
void exprtree_runact(struct expract *a)
{
    regat = a->ctx[0];
    varmode = (WORD)a->ctx[1];
    evaldepth = a->ctx[2];
    a->fn(a);
}

/* no asm counterpart (eval.h): the action fn with p, x and y, recorded
   when an expression is built, else run at once; nocode: it writes no
   code (it records a position or a state) */
static void negemit(struct expract *a);
static void convprep(UWORD from, UWORD to);

static struct expract *recordact(exprtree_actfn *fn, void *p, ULONG x, UWORD y, BOOL nocode)
{
    struct expract *a;

    if (!exprtree_building()) {
        /* outside an expression the code is written at once (the action
           kept in the arena until the statement ends, as one recorded) */
        a = exprtree_alloc(sizeof *a);
        memset(a, 0, sizeof *a);
        a->fn = fn;
        a->p = p;
        a->x = x;
        a->y = y;
        actctx(a);
        fn(a);
        return a;
    }
    a = exprtree_act(fn);
    a->p = p;
    a->x = x;
    a->y = y;
    actctx(a);
    actseq++;
    if (!nocode) {
        prevcodeseq = codeseq;
        codeseq = actseq;
        /* #1186: only the minus signs of a Byte or Word may follow the
           literal */
        if (blit.seq) {
            if (fn == negemit && y < T_LONG) {
                blit.last = codeseq;
                ((struct exprnode *)p)->left = blit.neg;
                blit.neg = p;
                blit.negs += ((struct exprnode *)p)->op;
            } else
                blit.seq = 0;
        }
    }
    return a;
}

struct expract *emitact(exprtree_actfn *fn, void *p, ULONG x, UWORD y)
{
    return recordact(fn, p, x, y, FALSE);
}

struct expract *stateact(exprtree_actfn *fn, void *p, ULONG x, UWORD y)
{
    return recordact(fn, p, x, y, TRUE);
}

/* no asm counterpart: a library used while the expression is read (a
   function's lookup, Used, Addr, Maximum, an OS library): the use now,
   and an action where its code is written that takes the library's
   relocations into the program's list there (uselib3, asmlibs.c) */
static void useact(struct expract *a)
{
    uselib3((struct blitzlib *)a->p, (UWORD)a->x);
}

UBYTE *findtokeact(UWORD tok, struct blitzlib **libp)
{
    UBYTE *sub = findtoke(tok, libp);

    stateact(useact, *libp, (*libp)->libnum, 0);
    return sub;
}

struct blitzlib *uselibact(UWORD libnum)
{
    struct blitzlib *bl = uselib(libnum);

    stateact(useact, bl, libnum, 0);
    return bl;
}

/* no asm counterpart: actions of a few words of code (x: the first word,
   y: the second; one word without ACT_TWO) */
void pokeact(struct expract *a)
{
    pokewd((UWORD)a->x);
    if (a->x & 0x10000UL)
        pokewd(a->y);
}

/* forward declarations of the internal routines */
static void fdo2(WORD lvo);
static void bytetof2(void);
static void qtof2(UWORD tok);
static void floattolong2(void);
static void wordtoquick_(void);
static void quicktob2(void);
static UWORD eval3(UWORD *type);
static UWORD eval4(UWORD *type, unsigned kind);
static UWORD element(UWORD c, UWORD *type);
static BOOL intrinsic_quick(UWORD type);
static UWORD eval2(UWORD *type, UWORD *c, unsigned *op);
static UWORD doop(struct opentry *op, UWORD type, BOOL words);
static void mixedop(struct opentry *op, const struct exprtype_bin *d, BOOL zeroonly);
static void doplus2(UWORD opcode);
/* the operator operate emits has two constants (BNF_FOLD) */
static BOOL foldnow = FALSE;
/* divzerochk saw a Quick Mod by a divisor above 0 and below 1 (#1219): its
   DIVS.W would divide by 0, constfold does not run the code */
static BOOL foldmodtrap = FALSE;
/* where its left operand's code begins (divzerochk) */
static ULONG foldfrom = 0;
static void compareop(struct opentry *op, UWORD left, const struct exprtype_bin *d);
static void divzerochk(const struct opentry *op, UWORD type);
static UWORD getelement(UWORD *type);
static UWORD getelement2(UWORD *type);
static void domylib(UWORD tok);
static void doflib(WORD lvo);
static void makesbase(void);
static UWORD variable(UWORD c, UWORD *type);
static UWORD function(UWORD c, UWORD *type);
static UWORD afunction(UWORD oldtype, UWORD *type, UWORD namelen);
static void dopusha3(void);
static void dopulla3(void);
/* the typing pass of rule P+ (the end of this file) */
struct p5node;
enum { P5L_CONT = 1, P5L_CONST, P5L_OTHER };     /* the kinds of leaf */
static void p5reset(void);
static void p5begin(void);
static BOOL p5end(UWORD asked, WORD unk, UWORD *type, struct exprnode *e, WORD push, BOOL cond, BOOL wide);
static void p5leaf(struct exprnode *l, unsigned kind);
static void p5elemdone(struct expract *prev);
static void p5wrap(unsigned kind, struct exprnode *x, struct expract *prev, struct expract *act);
static void p5binop(struct exprnode *n, struct expract *act);
static BOOL p5relaxbin(const struct exprnode *n, struct exprtype_bin *d);
static BOOL p5relaxnot(UWORD t);
static BOOL p5active(void);
static void p5exit(struct exprnode *e, WORD push);
static void p5check(const struct expract *a);


/* ============================================ Poking to Object code stuff */

/* blitz2.s:8453 pokewda5s */
void pokewda5s(UWORD w)
{
    if (varmode)
        w &= ~1;                    /* bclr #0,d1: source reg a5 -> a4 */
    pokewd(w);
}

/* blitz2.s:8458 pokewda5d */
void pokewda5d(UWORD w)
{
    if (varmode)
        w &= ~(1 << 9);             /* bclr #9,d1: dest reg a5 -> a4 */
    pokewd(w);
}

/* blitz2.s:8463 pokela5d */
void pokela5d(ULONG l)
{
    if (procmode)                   /* sic: procmode, not varmode */
        l &= ~(1UL << 25);
    pokel(l);
}

/* blitz2.s:8468 pokela5s - falls through into pokel */
void pokela5s(ULONG l)
{
    if (varmode)
        l &= ~(1UL << 16);
    pokel(l);
}

/* blitz2.s:8472 pokel */
void pokel(ULONG l)
{
    pokewd((UWORD)(l >> 16));
    pokelc(l);                      /* falls through into pokelc */
}

/* blitz2.s:8474 pokelc */
void pokelc(ULONG l)
{
    pokewd((UWORD)l);               /* falls through into pokewd */
}

/* blitz2.s:8476 pokewd - poke d1.w into pc - not an opcode; Fixed (#969):
   the pad byte of the alignment is written as 0 (the original skips it) */
void pokewd(UWORD w)
{
    ULONG at;

    zeropc((pc + 1) & ~1UL);        /* addq.l #1,pc / bclr #0,pc+3 */
    at = pc;
    if (at >= libat) {
        nomemleft = -1;             /* .over */
        at += 2;
    } else {
        W_(at) = w;
        at += 2;
    }
    pc = at;                        /* .ms */
    if (at > bigpc && !cfetchmode)
        bigpc = at;
}

/* blitz2.s:8496 pokebyte */
void pokebyte(UBYTE b)
{
    ULONG at = pc;

    if (at >= libat) {
        nomemleft = -1;
        at += 1;
    } else {
        B_(at) = b;
        at += 1;
    }
    pc = at;
    if (at > bigpc && !cfetchmode)
        bigpc = at;
}

/* blitz2.s:8514 pokecode - a0=start of code to poke, a1=end */
void pokecode(const UWORD *start, const UWORD *end)
{
    do
        pokewd(*start++);
    while (start < end);
}

/* blitz2.s:8525 pokecode2 */
void pokecode2(const UWORD *start, const UWORD *end)
{
    do
        pokewd(*start++);
    while (start < end);
}

/* ======================================================== type conversion */

/* blitz2.s:8533 bytetoword - nothing (bytes are ext.w'd when fetched,
   see varcont) */
void bytetoword(void)
{
    /* Below should ALWAYS be done when something is fetched
       (move #$4880,d1 / or regat,d1 / bra pokewd - commented out) */
}

/* blitz2.s:8541 bytetolong - falls through into wordtolong */
void bytetolong(void)
{
    bytetoword();
    wordtolong();
}

/* blitz2.s:8543 wordtolong - ext.l Dn */
void wordtolong(void)
{
    pokewd(0x48c0 | regat);
}

/* blitz2.s:8546 bytetoquick - falls through into wordtoquick */
void bytetoquick(void)
{
    bytetoword();
    wordtoquick_();
}

/* blitz2.s:8548 wordtoquick - swap Dn / clr.w Dn */
void wordtoquick(void)
{
    wordtoquick_();
}

static void wordtoquick_(void)
{
    pokewd(0x4840 | regat);
    pokewd(0x4240 | regat);
}

/* blitz2.s:8554 bytetofloat - falls through into bytetof2 */
void bytetofloat(void)
{
    bytetolong();
    bytetof2();
}

/* blitz2.s:8556 bytetof2 - SPFlt; falls through into fdo2 */
static void bytetof2(void)
{
    fdo2(-36);
}

/* blitz2.s:8557 fdo2 - call FFP routine lvo on D(regat) */
static void fdo2(WORD lvo)
{
    ULONG push, pull;

    libjsr[1] = (UWORD)lvo;         /* move d1,libjsr+2 */
    savereg(&push, &pull);
    pokemovem(push);
    tokejsr(getffpbase);
    ritetod0();
    pokel(L_(libjsr));
    ritefromd0();
    pokemovem(pull);
}

/* blitz2.s:8570 wordtobyte - falls through into overchk */
void wordtobyte(void)
{
    overchk(wtobover);
}

/* blitz2.s:8573 overchk - d1=overflow check routine. */
void overchk(UWORD token)
{
    if (!(debugga >> 8))                            /* tst.b debugga */
        return;
    if (!(optreq2ga4.Flags & GFLG_SELECTED))       /* overflow checking? */
        return;
    if (cfetchmode)
        return;
    overchks++;                                     /* no fold in C over it */
    if (regat) {
        pokewd(overchk_code[0]);                    /* move.l d0,-(a7) */
        pokewd(regat | overchk_code[2]);            /* move.l Dn,d0 */
        tokejsr(token);                             /* bsr .isok */
        pokewd(overchk_code[1]);                    /* move.l (a7)+,d0 */
        return;
    }
    /* .isok: movem.l d0-d7/a0-a6 around tokejsr (nothing to save in C) */
    tokejsr(token);
}

/* blitz2.s:8604 wordtofloat */
void wordtofloat(void)
{
    wordtolong();
    bytetof2();
}

/* blitz2.s:8607 longtobyte */
void longtobyte(void)
{
    overchk(ltobover);
}

/* blitz2.s:8610 longtoword */
void longtoword(void)
{
    overchk(ltowover);
}

/* blitz2.s:8613 longtoquick */
void longtoquick(void)
{
    overchk(ltowover);
    wordtoquick_();
}

/* blitz2.s:8617 longtofloat */
void longtofloat(void)
{
    bytetof2();
}

/* blitz2.s:8620 quicktobyte - falls through into quicktob2 */
void quicktobyte(void)
{
    overchk(qtobover);
    quicktob2();
}

/* blitz2.s:8624 quicktob2 - clr.w Dn / swap Dn */
static void quicktob2(void)
{
    pokewd(0x4240 | regat);
    pokewd(0x4840 | regat);
}

/* blitz2.s:8630 quicktoword */
void quicktoword(void)
{
    quicktob2();
}

/* blitz2.s:8632 quicktolong */
void quicktolong(void)
{
    quicktob2();
    wordtolong();
}

/* blitz2.s:8635 quicktofloat - put my own routine here; falls into qtof2 */
void quicktofloat(void)
{
    qtof2(qutofl);
}

/* blitz2.s:8639 qtof2 - call Blitz lib routine tok on D(regat) */
static void qtof2(UWORD tok)
{
    ULONG push, pull;

    savereg(&push, &pull);
    pokemovem(push);
    ritetod0();
    tokejsr(tok);
    ritefromd0();
    pokemovem(pull);
}

/* blitz2.s:8652 floattobyte */
void floattobyte(void)
{
    overchk(ftobover);
    floattolong2();
    longtobyte();
}

/* blitz2.s:8659 floattoword */
void floattoword(void)
{
    overchk(ftowover);
    floattolong2();
    longtoword();
}

/* blitz2.s:8666 floattolong - falls through into floattolong2 */
void floattolong(void)
{
    overchk(ftolover);
    floattolong2();
}

/* blitz2.s:8669 floattolong2 - SPFix */
static void floattolong2(void)
{
    fdo2(-30);
}

/* blitz2.s:8673 floattoquick - do my own routine here */
void floattoquick(void)
{
    overchk(ftowover);
    qtof2(fltoqu);
}

/* blitz2.s:8681 stringtolong */
void stringtolong(void)
{
    pokewd(*stol);                  /* addq #4,a7 - lose string len */
    sbtolong();
}

/* blitz2.s:8686 convtypef - as below, but regat+1 should be saved! */
void convtypef(UWORD from, UWORD to)
{
    fuckpos++;
    convtype(from, to);
    fuckpos--;
}

/* blitz2.s:8693 convtype - convert type d3 to d2 */
void convtype(UWORD from, UWORD to)
{
    WORD idx = (WORD)((from - 1) * 7 + to - 1);
    convhandler *h;

    /* DIFF: the asm indexes convtable without a range check (from/to are
       always 1..7 here); out of range does nothing in C */
    if (idx < 0 || idx >= 49)
        return;
    h = convtable[idx];
    if (h) {
        ULONG at = foldat();
        UWORD checks = overchks;

        h();                        /* jmp (a0) */
        if (overchks == checks)     /* the code of the conversion only */
            foldnote(FN_CONV, from, to, 0, at);
    }
}

/* blitz2.s:14693 routines for type conversion */
convhandler *const convtable[49] = {
    0, bytetoword, bytetolong, bytetoquick, bytetofloat, bytetolong, noconerr,
    wordtobyte, 0, wordtolong, wordtoquick, wordtofloat, wordtolong, noconerr,
    longtobyte, longtoword, 0, longtoquick, longtofloat, 0, noconerr,
    quicktobyte, quicktoword, quicktolong, 0, quicktofloat, quicktolong, noconerr,
    floattobyte, floattoword, floattolong, floattoquick, 0, floattolong, noconerr,
    longtobyte, longtoword, 0, longtoquick, longtofloat, 0, noconerr,
    noconerr, noconerr, stringtolong, noconerr, noconerr, stringtolong, 0,
};

/* ============================================================ var names */

/* blitz2.s:8709 getvname - transfer chars to namebuff. set flagmask
   according to array/pointer etc status */
UWORD getvname(UWORD c, UWORD *namelen)
{
    UWORD len, next;

    if (c == '\\') {
        usedfrom = srcptr - 1;      /* subq #1,a5 */
        srcptr = usedpath;
        c = getchar3();
    }
    flagmask = 0;                   /* .skip0 */
    if (c == '*') {
        flagmask = 1;
        len = makename(&c);         /* .more2 */
    } else if (c == '@') {          /* .skip */
        flagmask = 5;
        len = makename(&c);
    } else {
        len = makename2(&c);        /* .skip2 */
    }
    *namelen = len;
    if (!len)                       /* .more */
        syntaxerr();
    if (c == '(') {
        flagmask |= 2;
        return c;
    }
    if (c == '.') {                 /* .morem */
        storeloc(c);
        if (!makename3((char *)namebuff2, &c))
            syntaxerr();
    } else if (c == '$') {
        flagmask |= 8;
        storeloc(c);
        c = getchar3();
    } else
        return c;                   /* .done */
    next = c;                       /* .morez */
    c = resloc();
    if (next == '(')
        flagmask |= 2;              /* .setmask */
    return c;
}

/* blitz2.s:8755 dopusha3 */
static void dopusha3(void)
{
    uselib(65235);
    pokecode(pusha3, pusha3f);
}

/* blitz2.s:8761 dopulla3 */
static void dopulla3(void)
{
    uselib(65235);
    pokecode(pulla3, pulla3f);
}

/* blitz2.s:8769 sbtolong - make sbase into a long */
void sbtolong(void)
{
    UWORD regfield;

    pokewd(ceos[0]);                /* clr.b (a3)+ */
    regfield = regat << 9;
    if ((UWORD)sbgot >= 2) {
        WORD used = stackuse - thisstuse;
        if (used == 6) {
            pokewd(regfield | *movestst2);
            pokecode2(movestst2 + 1, movestst2f);
            ext_strpointer();
            return;
        }
        pokewd(regfield | *movea7dn); /* .lenonly */
        pokel(L_(movea7dn + 1));
        ext_strpointer();
        return;
    }
    ext_strpointer();
    pokewd(regfield | *movestdn);   /* .skip2 */
    pokewd(uselib(65235)->varoff);
}

/* =================================================== array index eval */

/* blitz2.s:8809 arrevalchk - falls through into arrszchk
   Fixed (#1179): a later index (the second and every further one,
   calcvar) is evaluated as a Long with and without runtime error
   checking, as the first index.  The original evaluated it as a Word
   without the checking (sharreval): with a Quick operand the two builds
   computed differently (y=200: g(1,y*200/2) read element 52768 unchecked,
   20000 checked; q=1.5: b(1,q ASL 4) 24 against 16), so the checked build
   did not report the element the unchecked build used, and q AND 1.5 was
   refused only by the checked build ("Illegal Operator for Type").  A
   Word * or / of two Word values is computed inline (laterindex: dotimesl,
   dodivl), in both builds.
   The size check (arrszchk) stays the checked build's.  Without it only
   the low word of a later index is read (makelmul, parse.c: mulu.w), so
   the upper word is not built (idxnarrow, decided while the index is
   built): a constant index is loaded with move.w, and the final ext.l of
   an index that is one Byte, Word or Quick element (its conversion to
   Long at the index's evaldepth) is left out - the code of a(i,j),
   b(i,j,k), a(5,7) stays the original's byte for byte.  A change of
   makelmul that reads the whole Long (indexes above 65535) must drop
   this narrowing. */
static void szchkact(struct expract *a)
{
    (void)a;
    arrszchk();
}

UWORD arrevalchk(void)
{
    UWORD c;

    idxnext = TRUE;
    c = arreval();
    if (debugga >> 8) {
        tokuse(arrerr);
        emitact(szchkact, NULL, 0, 0);
    }
    return c;
}

/* blitz2.s:8812 arrszchk - cmpi.l #65536,Dn / bcs.w +8 / jsr arrerr */
void arrszchk(void)
{
    pokewd(somecode[0] | regat);
    pokel(0x10000);
    pokel(L_(somecode2));
    tokejsr(arrerr);
}

/* blitz2.s:8823 sharreval */
UWORD sharreval(void)
{
    UWORD t = T_WORD;
    return maineval2(0, &t, 0);
}

/* blitz2.s:8828 arreval - getlong, no push and convert */
UWORD arreval(void)
{
    UWORD t = T_LONG;
    return maineval2(0, &t, 0);
}

/* ============================================================ eval entries */

/* blitz2.s:8833 bakpevalu - falls through into pevalu */
UWORD bakpevalu(UWORD *type)
{
    bakup();
    return pevalu(type);
}

/* blitz2.s:8835 pevalu
   Unused (no caller in blitz2.s): moveq #-1,d3 / bra peval - peval reloads d3 with 0, so pevalu
   behaves exactly like peval. */
UWORD pevalu(UWORD *type)
{
    return peval(type);
}

/* blitz2.s:8839 bakpeval - falls through into peval */
UWORD bakpeval(UWORD *type)
{
    bakup();
    return peval(type);
}

/* blitz2.s:8841 peval */
UWORD peval(UWORD *type)
{
    return maineval(-1, type, 0);
}

/* blitz2.s:8845 bakevalu - falls through into evalu */
UWORD bakevalu(UWORD *type)
{
    bakup();
    return evalu(type);
}

/* blitz2.s:8847 evalu */
UWORD evalu(UWORD *type)
{
    return maineval(0, type, -1);
}

/* blitz2.s:8851 evalu2 */
UWORD evalu2(UWORD *type)
{
    return maineval(1, type, -1);
}

/* blitz2.s:8855 bakeval - falls through into eval */
UWORD bakeval(UWORD *type)
{
    bakup();
    return eval(type);
}

/* blitz2.s:8857 eval - no push; falls through into maineval */
UWORD eval(UWORD *type)
{
    return maineval(0, type, 0);
}

/* blitz2.s:8860 maineval - Big Evaluation routine.....
   d2 = type to collect - 0 if unknown
   d3 = 0 if conv to original type
   d1 = push result flag, 0 = no push, < 0 = push result
        > 0 = push type when known
   result will be pushed anyway if using reg d6+
   if getting a string, then strings len is on stack (a long), and a3
   points at arse end of string while D(regat) points to start of string */
UWORD maineval(WORD push, UWORD *type, WORD unk)
{
    if (regat >= 6) {
        push = -1;
        regat = 6;                  /* not restored (setsvars resets it) */
    }
    return maineval2(push, type, unk);
}


/* no asm counterpart; replaces blitz2.s:8879-8886 maineval2's saves - the
   enter action of the evaluation e (EVAL): the stack use at its start,
   the push flag a->y, no string base pushed for it yet; what they were in
   e (value, aux, off) */
static void evalenter(struct expract *a)
{
    struct exprnode *e = a->p;

    e->value = (UWORD)thisstuse;
    e->aux = (UWORD)pushflag;
    e->off = sbgot;
    thisstuse = stackuse;
    pushflag = (WORD)a->y;
    sbgot = 0;                      /* not pushed yet */
}

/* no asm counterpart; replaces blitz2.s:8879 maineval2's rawq cut (Fixed
   (#1178), #1179) - a raw Quick at the end rounded down; a->y: the end of
   a narrowed later index, its ext.l left out (idxnarrow) */
static void rawcutact(struct expract *a)
{
    if (a->y)
        quicktob2();
    else
        quicktolong();
}

/* Fixed (#1179), no asm counterpart; replaces the decisions of
   arrevalchk's narrowing after the index (the original's sharreval
   evaluated a later index as a Word) - is the later index just built
   (its code recorded after the action number from, rawq: it ends in a
   raw Quick) narrowed to its low word?  The index's last code is the
   one narrowed: one integer constant that is its only code (a move.l:
   ELF_NARROW, written as a move.w), the conversion to Long of one Byte,
   Word or Quick element at the index's evaldepth (ELF_NARROW: without
   its ext.l), or the rounding of a raw Quick (TRUE: rawcutact without
   its ext.l).  Not with runtime error checking (arrszchk reads the whole
   Long). */
static BOOL idxnarrow(ULONG from, BOOL raw)
{
    struct exprnode *l;

    if (debugga >> 8)
        return FALSE;
    l = bcst.leaf;
    if (bcst.seq && bcst.seq == codeseq && bcst.before == from
        && (l->tafter == T_LONG || l->tafter == T_QUICK) && !(l->lflags & ELF_FFP)) {
        l->lflags |= ELF_NARROW;
        return FALSE;
    }
    if (raw)
        return TRUE;
    if (bconv.seq && bconv.seq == codeseq && bconv.depth == evaldepth && bconv.to == T_LONG
        && (bconv.from == T_BYTE || bconv.from == T_WORD || bconv.from == T_QUICK))
        bconv.leaf->lflags |= ELF_NARROW;
    return FALSE;
}

/* no asm counterpart; replaces blitz2.s:8893-8962 maineval2 after eval3
   (the conversion, .hi1, .skipme, .notst, .skip) - the exit action of the
   evaluation e: the value converted from e->tbefore to e->tafter
   (EVF_CONV), the push flag a->y (as the argument of an intrinsic
   decided), the type pushed, a string ended, the result pushed; the
   state of the enter action back */
static void evalexit(struct expract *a)
{
    struct exprnode *e = a->p;
    UWORD t = e->tafter;

    if (e->lflags & EVF_P5)
        p5exit(e, (WORD)a->y);      /* the pass's consumer (#1216) */
    if (e->lflags & EVF_CONV)
        convtype(e->tbefore, t);    /* exg d2,d3 / convtype */
    pushflag = (WORD)a->y;
    if (pushflag > 0) {
        /* push the type got. */
        UWORD regfield = regat << 9;
        if (t == T_STRING)
            pokewd(regfield | *getstlen);
        stackuse += 2;                              /* .hi1 */
        if (!(e->lflags & EVF_FIT))     /* set by p5exit at run time */
            pokewd(((regfield | 0x7000) + 0x0200) | t); /* moveq #x,dREGAT+1 */
        pokewd((*pushut | regat) + 1);
        if (t == T_STRING)
            pokewd(*putstlen | regat);
    }
    if (t == T_STRING) {                            /* .skipme */
        stackuse += 4;      /* a length on da stack..... */
        sbtolong();
    } else if ((UWORD)sbgot >= 2)                   /* .notst */
        pokewd(ststfix[0]);
    sbgot = e->off;                                 /* .notst2 */
    if (pushflag < 0) {
        UWORD opcode = *pushd0wd;
        stackuse += 2;
        if (t >= 3) {
            stackuse += 2;
            opcode = *pushd0l;
        }
        pokewd(opcode | regat);
    }
    pushflag = (WORD)e->aux;                        /* .skip */
    thisstuse = (WORD)e->value;
}

/* no asm counterpart; replaces blitz2.s:8879-8962 maineval2's control -
   the evaluation built: its node (EVAL) with its enter action, the
   expression (eval3), its exit action.  The type asked for (unk: none)
   and the push flag are decided here; *type is the type got. */
static UWORD evalbuild(WORD push, UWORD *type, WORD unk)
{
    struct exprnode *e = exprtree_new(EXN_EVAL);
    UWORD asked = *type, t = asked, c;
    BOOL s_rawq = rawq;
    BOOL s_cmpres = cmpres;
    BOOL s_laterindex = laterindex;
    ULONG from;
    BOOL narrow = FALSE;
    BOOL cond = evalcond, wide = evalwide;  /* #1216: for this evaluation only */

    evalcond = evalwide = FALSE;
    if (wide)
        p5wide = FALSE;
    laterindex = idxnext;           /* Fixed (#1179) */
    idxnext = FALSE;
    stateact(evalenter, e, 0, (UWORD)push);
    evaldepth++;
    rawq = FALSE;
    from = codeseq;
    p5begin();
    c = eval3(&t);
    /* the typing pass of rule P+ (#1216, #1183): its decisions written
       into the actions, t the type it gives, or the code as built */
    if (p5end(asked, unk, &t, e, push, cond, wide))
        rawq = FALSE;
    else if (laterindex)            /* Fixed (#1179): narrowed? */
        narrow = idxnarrow(from, rawq);
    if (rawq)                       /* Fixed (#1178): a raw Quick, floor */
        emitact(rawcutact, NULL, 0, (UWORD)narrow);
    rawq = s_rawq;
    cmpres = s_cmpres;              /* Fixed (#1182) */
    evaldepth--;
    e->tbefore = t;
    if (asked && !unk) {            /* type asked for */
        convprep(t, asked);
        e->lflags |= EVF_CONV;
        t = asked;
    }
    if (push == 2)                  /* the argument of an intrinsic */
        push = intrinsic_quick(t) ? 0 : 1;
    e->tafter = t;
    emitact(evalexit, e, 0, (UWORD)push);
    laterindex = s_laterindex;
    *type = t;
    return c;
}

static UWORD evalunit(WORD push, UWORD *type, WORD unk, void (*before)(UWORD type));

/* blitz2.s:8879 maineval2 - d2=type to get, d1=push flag.  Outside an
   expression it begins the unit (exprtree.h): the expression is built,
   then its actions write its code; inside one (a parameter, an index) it
   is built as a part of it. */
UWORD maineval2(WORD push, UWORD *type, WORD unk)
{
    if (exprtree_building())
        return evalbuild(push, type, unk);
    return evalunit(push, type, unk, NULL);
}

/* no asm counterpart; replaces blitz2.s:8879 maineval2 outside an
   expression - the unit of an expression: built, the code of before (the
   type got; NULL: none), then the expression's code */
static UWORD evalunit(WORD push, UWORD *type, WORD unk, void (*before)(UWORD type))
{
    struct stmtunit u;
    UWORD c;

    unitbegin(&u);
    c = evalbuild(push, type, unk);
    unitbuilt(&u);
    if (before)
        before(*type);
    unitwrite(&u);
    return c;
}

/* no asm counterpart (eval.h): a unit of a statement begun - the arena's
   place, the build's records and the libraries' relocations of the unit
   from here on */
void unitbegin(struct stmtunit *u)
{
    u->m = exprtree_mark();
    u->at = pc;
    u->actseq = actseq;
    u->codeseq = codeseq;
    u->bsbase = bsbase;
    bsbase = sbasegot != 0;
    u->strwork = bstrwork;
    bstrwork = FALSE;
    bconv.seq = blit.seq = bcst.seq = bbyte = 0;
    u->rb = relocunit();
    u->p5 = p5unit;
    p5unit = FALSE;
    exprtree_begin(&u->outer);
}

/* no asm counterpart (eval.h): the unit built, its actions kept, whether
   its code may use the string work area or a3 noted (strwork) */
void unitbuilt(struct stmtunit *u)
{
    BOOL sw = bstrwork;

    if (pc != u->at)
        interr2();                  /* not reached: no code while it is built */
    u->acts = exprtree_end(&u->outer);
    if (p5unit)
        p5check(u->acts);           /* #1185: the typing pass's register pairs */
    bstrwork = sw || u->strwork;    /* for a build around the unit */
    u->strwork = sw;
}

/* no asm counterpart (eval.h): the unit's code written, the unit ended */
void unitwrite(struct stmtunit *u)
{
    exprtree_run(u->acts);
    /* the typing pass's code (#1216) may call no routine of a library the
       build used: its relocations into the list all the same (the
       library is in the program, its calls of other libraries too) */
    if (p5unit)
        relockeep();
    p5unit = u->p5;
    relocdrop(u->rb);
    exprtree_unwind(u->m);
    actseq = u->actseq;
    codeseq = u->codeseq;
    bsbase = u->bsbase;
}

/* no asm counterpart (eval.h): bakevalu with code before the
   expression's (doselect) */
UWORD bakevalubefore(UWORD *type, void (*before)(UWORD type))
{
    WORD push = 0;

    bakup();
    if (exprtree_building())
        interr2();                  /* not reached: a statement's expression */
    if (regat >= 6) {               /* maineval */
        push = -1;
        regat = 6;
    }
    return evalunit(push, type, -1, before);
}

/* no asm counterpart (eval.h): a constant computed in the middle of an
   expression (evalconst2: a ~ constant, CNIF) is a unit of its own, the
   expression being built is set aside meanwhile; the build's records of
   the last elements start again behind it */
void exprsuspend(struct exprunit *s)
{
    exprtree_suspend(s);
}

void exprresume(struct exprunit *s)
{
    exprtree_end(s);
    if (!exprtree_building())
        return;
    bconv.seq = blit.seq = bcst.seq = bbyte = 0;
}


/* ===================================================== the fold in C
   No asm counterpart (task 3.6 of expr-tree-5-operand-typing; exprfold.h):
   the code a constant's value goes through on its way to an operator (the
   constant's move, a fold's move, the minus signs, Not, the conversions,
   getelement's push and pull of d6) is noted when it is written, with what it does to its register.  The
   fold of two constants computes the operation from them in C
   (exprfold_binop with exprtype_binop's decision): when the notes cover
   the code from the left operand's start to the operation without a gap
   and that code is still the one noted.  Otherwise (a Float, code that is
   not noted, an overflow check of the debugger in the operation's code,
   a Long or Quick *
   of lmullib, whose overflow depends on the library loaded:
   exprfold_libdep) the code is run, as the original did. */
#define FOLDNOTES 32                /* the notes kept: the operands of one fold need a few */
#define FOLDCODE 8                  /* the longest code noted, in words */
struct foldnote {
    ULONG from, to;                 /* the code */
    ULONG v;                        /* FN_SETL, FN_SETW: the value */
    UBYTE kind, reg, a, b;          /* FN_*, its register, its types or count */
    UWORD code[FOLDCODE];
};
static struct foldnote foldnotes[FOLDNOTES];
static UWORD nfoldnotes;
/* the overflow checks written (overchk): code the fold in C does not run */
UWORD overchks;
/* the operation operate writes: its decision, where its code begins */
static struct exprtype_bin foldd;
static UWORD foldop, foldleft, foldchecks;
static ULONG foldopat;

/* no asm counterpart: where the code written next begins (pokewd's
   alignment) */
ULONG foldat(void)
{
    return (pc + 1) & ~1UL;
}

/* no asm counterpart: the code from at to pc is of kind kind on D(regat)
   (a and b its types or its count, v its value).  The notes of code at or
   behind at are gone (written over). */
void foldnote(UWORD kind, UWORD a, UWORD b, ULONG v, ULONG at)
{
    struct foldnote *n;
    UWORD i, k = 0;

    for (i = 0; i < nfoldnotes; i++)
        if (foldnotes[i].to <= at)
            foldnotes[k++] = foldnotes[i];
    nfoldnotes = k;
    if (nomemleft || pc <= at || pc - at > 2 * FOLDCODE)
        return;
    if (nfoldnotes == FOLDNOTES) {  /* the oldest goes */
        memmove(foldnotes, foldnotes + 1, sizeof foldnotes - sizeof foldnotes[0]);
        nfoldnotes--;
    }
    n = &foldnotes[nfoldnotes++];
    n->from = at;
    n->to = pc;
    n->v = v;
    n->kind = (UBYTE)kind;
    n->reg = (UBYTE)regat;
    n->a = (UBYTE)a;
    n->b = (UBYTE)b;
    for (i = 0; at + 2 * i < pc; i++)
        n->code[i] = W_(at + 2 * i);
}

/* no asm counterpart: the registers after the code from at to end, by the
   notes (FALSE: not all of it noted, or not as noted) */
static BOOL foldregs(ULONG at, ULONG end, struct exprfold_reg *reg)
{
    const struct foldnote *n;
    struct exprfold_reg saved[4];   /* getelement's pushes of d6 */
    UWORD i, nsaved = 0;
    int ok;

    memset(reg, 0, 8 * sizeof *reg);   /* nothing known */
    while (at < end) {
        for (i = nfoldnotes; i && foldnotes[i - 1].from != at; i--)
            ;
        if (!i)
            return FALSE;
        n = &foldnotes[i - 1];
        if (n->to > end)
            return FALSE;
        for (i = 0; n->from + 2 * i < n->to; i++)
            if (W_(n->from + 2 * i) != n->code[i])
                return FALSE;
        switch (n->kind) {
        case FN_SETL: exprfold_setl(&reg[n->reg], n->v); ok = 1; break;
        case FN_SETW: exprfold_setw(&reg[n->reg], (UWORD)n->v); ok = 1; break;
        case FN_NEG: ok = exprfold_neg(&reg[n->reg], n->a, n->b); break;
        case FN_NOT: ok = exprfold_not(&reg[n->reg], n->a); break;
        case FN_PUSH:               /* move.x d6,-(a7) */
            ok = nsaved < 4;
            if (ok)
                saved[nsaved++] = reg[6];
            break;
        case FN_PULL:               /* move.x d6,d7, move.x (a7)+,d6 */
            ok = nsaved > 0;
            if (!ok)
                break;
            exprfold_move(&reg[7], &reg[6], n->a);
            exprfold_move(&reg[6], &saved[--nsaved], n->b);
            break;
        default: ok = exprfold_conv(&reg[n->reg], n->a, n->b); break;
        }
        if (!ok)
            return FALSE;
        at = n->to;
    }
    return at == end;
}

/* no asm counterpart: the operands of the operation operate writes, from
   at (its left operand's code) in D(regat) and D(regat+1), by the notes */
static BOOL foldoperands(ULONG at, struct exprfold_reg *l, struct exprfold_reg *r)
{
    struct exprfold_reg reg[8];

    if (regat >= 7 || overchks != foldchecks || !foldregs(at, foldopat, reg))
        return FALSE;
    *l = reg[regat];
    *r = reg[regat + 1];
    return TRUE;
}

/* blitz2.s:9017 the constant fold of eval5 (exprtree_emit): the code of
   the operator n's two operands (both constants, BNF_FOLD) and the
   operation is replaced by a move of the result (of type `type`) into
   regat.  Where the left operand's code begins, the relocations and
   lasta6 then: its first action s (exprtree_operand's startmark).
   DIFF: the result is computed in C from the notes of the operands' code
   (the fold in C, above); only where they do not cover it (a Float, code
   not noted, lmullib's * and the other cases named there) the code is
   run, as the original did. */
static void constfold(const struct expract *s, UWORD type)
{
    ULONG at = ((const struct exprnode *)s->p)->start;
    struct m68kregs r;
    struct offnode *node, *keep, **link;
    struct exprfold_reg left, right;
    ULONG value = 0, foldend;
    UWORD opcode;
    BOOL inc;

    /* Set up MOVE.l regat,d0 */
    pokewd(0x2000 | regat);
    pokewd(0x4e75);                         /* rts */
    /* Fixed (#1190): the fold is skipped when the two words did not fit
       (the operands ended at the end of the object buffer or 2 bytes
       before it).  The original (blitz2.s:9017) looked at nomemleft
       only before the pokes and ran the code without its rts, into
       whatever lies behind the buffer (the first library copied into
       the library area): a hang or a crash of the compiler.  The pass is
       thrown away anyway ("Not Enough Object Memory", or the compile
       again with the buffer sized to bigpc); pc stays behind the two
       words, the operands' code and offsets are left as they are. */
    if (nomemleft)
        return;
    foldend = pc;                           /* end of the folded code */
    /* Fixed (#343): only the word lasta6 is restored.  The original's long
       move (blitz2.s:9025) also restored quoteflag as getelement2 saved it
       - after getchar3 had read the left operand's first char: for a
       character constant ("x"*8, only in asmfixer: constmode) the opening
       quote, so quoteflag stayed set after the expression and the operands
       evaluated next (earlier lines, insasm prepends) lost their spaces
       inside quotes: #" " gave 0. */
    lasta6 = (WORD)s->y;
    inc = !exprfold_libdep(foldop, &foldd) && foldoperands(at, &left, &right)
          && exprfold_binop(foldop, &foldd, foldleft, &left, &right) && exprfold_value(&left, type, &value);
    if (!inc && foldmodtrap && foldop == ETO_MOD && foldd.handler == ET_QUICK) {
        /* Fixed (#1219): the exact Mod is below its divisor, a Word 0 */
        value = 0;
        inc = TRUE;
    }
    foldmodtrap = FALSE;
    pc = at;
    if (!inc) {
        /* DIFF: asm saved d2/a5 and did jsr (a0) with the compiler's
           registers; C runs the code through callregs with a6 = maths and a5
           such that ffplib's var slot is `maths` */
        memset(&r, 0, sizeof(r));
        r.a[6] = (ULONG)MathBase;
        r.a[5] = (ULONG)&MathBase - (LONG)ffplib->varoff;
        flushc();
        callregs((APTR)pc, &r);
        value = r.d[0];                     /* d0 has got constant! */
    }
    opcode = (regat << 9) | 0x203c;         /* move.l #x,dn */
    if (type < 3) {
        pokewd(opcode | 0x1000);
        pokewd((UWORD)value);
    } else {
        pokewd(opcode);
        pokel(value);
    }
    foldnote(type < 3 ? FN_SETW : FN_SETL, 0, 0, value, at);
    /* .mode - clean up offset stack.  Fixed (#1049): only the offsets
       inside the folded code (at up to foldend) are freed.  The
       original freed every offset added since the fold began, also the
       relocations of a library that uselib3 linked in meanwhile (its code
       lies in the library area, not in the program code): a constant Mod
       as the first use of modlib left modlib's calls of ldivlib and
       lmullib at their compile-time addresses in the executable. */
    keep = (struct offnode *)s->x;
    link = &firstoff;
    while ((node = *link) != keep) {
        if (node->offset >= at - pcat
            && node->offset < foldend - pcat) {
            *link = node->next;
            numoffs--;
            FreeMem(node, 8);
        } else
            link = &node->next;
    }
}

/* Fixed (#1178), no asm counterpart: the raw Quick in D(reg) rounded
   down to a Long (quicktolong, the code varcont emitted for the cut) */
static void rawcut(UWORD reg)
{
    UWORD s = regat;

    regat = reg;
    quicktolong();
    regat = s;
}

/* no asm counterpart: the operator op as exprtype.h numbers it (its index
   in operators[]; NULL, the end or the sentinel: ETO_END) */
static unsigned opindex(const struct opentry *op)
{
    return op ? (unsigned)(op - operators) : ETO_END;
}

/* no asm counterpart; replaces blitz2.s:8995-9000 (eval5 after .higher's
   right operand: convtypef, .skip's popprec, doop): the code of the
   operator n on its left operand (n->left, of type n->tbefore) in
   D(regat) and the right one, whose evaluation ended with the type so far
   n->vtype, in D(regat+1) (n->left->start, n->start: where their code
   begins; the facts of the build in n->lflags, those of the code before
   it in n->flags and the emitter's state).  exprtype_binop decides, this
   emits what it returns; zeroonly: only the code up to the check of a
   division by zero (divzerobuild).  Returns the type so far.
   Fixed (#1215): zeroonly is a parameter, so that a "Division by Zero"
   the check raises leaves nothing set for the next compile. */
static UWORD operate(struct exprnode *n, BOOL zeroonly)
{
    struct exprnode *l = n->left;
    struct opentry *op = &operators[n->op];
    UWORD left = n->tbefore, right = n->vtype;
    struct exprtype_binfacts f;
    struct exprtype_bin d;

    f.lraw = (n->lflags & BNF_LRAW) != 0;
    f.rraw = (n->lflags & BNF_RRAW) != 0;
    f.rcmp = (n->lflags & BNF_RCMP) != 0;
    f.lbyte = f.lconst = f.rbyte = f.rconst = 0;   /* #978: binopbuild */
    /* Fixed (#1179): a later index, both operands Word values */
    f.words = (n->lflags & BNF_WORDS) != 0;
    d = exprtype_binop(n->op, left, right, &f);
    foldd = d;                      /* for the fold in C */
    foldop = n->op;
    foldleft = left;
    foldopat = pc;
    foldchecks = overchks;
    /* Fixed (#1182): a right operand that a comparison lowered below the
       left operand's type (the comparison's boolean, a Word, or a value
       computed from it) is converted up to the left one's type and the
       operation done in that type.  The original converted the left
       operand down (convtypef below, blitz2.s:8998): with lw=65537
       rl.l=lw AND (l1=l1) gave 1, with fh=0.5 rf.f=fh+(l1=l1) -1.  A
       Float left of AND, OR, EOR or a shift is refused now, as any Float
       there.  Not a Long under a Quick left (the late cut of #1178
       stays): a Long may lie beyond the Quick range.  (A comparison
       converts its right operand up in compareop.) */
    if (d.path != ETP_COMPARE && d.rup_from) {
        regat++;
        convtype(d.rup_from, d.rup_to);
        regat--;
    }
    foldnow = (n->lflags & BNF_FOLD) != 0;
    foldfrom = ((const struct exprnode *)((const struct expract *)l->value)->p)->start;
    if (d.path == ETP_COMPARE || d.path == ETP_CMPLQ) {
        if (d.rcut)                 /* not reached: the right starts at 0 */
            rawcut(regat + 1);
        compareop(op, left, &d);
    } else if (d.path == ETP_MIXED)
        mixedop(op, &d, zeroonly);
    else {
        if (d.lconv_from)
            convtypef(d.lconv_from, d.lconv_to); /* could possibly fuck regat+1 ! */
        divzerochk(op, d.handler);
        if (zeroonly)
            return d.result;
        doop(op, d.handler, d.wordop);
    }
    return d.result;
}

/* no asm counterpart; replaces blitz2.s:9001-9083 eval5 after doop - the
   code of the operator n: the operation, the constant fold of two
   constants (BNF_FOLD, not of a result that is no constant: BNF_NOFOLD) */
static void binopact(struct expract *a)
{
    struct exprnode *n = a->p;
    UWORD t = operate(n, FALSE);

    if (t != n->tafter)
        interr2();                  /* not reached: typed when it was built */
    /* O.K., lets check the forth stack to see if last two operators are
       constants!  if so, we can JSR the routine to optimize into a
       constant! */
    if ((n->lflags & (BNF_FOLD | BNF_NOFOLD)) == BNF_FOLD && !nomemleft)
        constfold((const struct expract *)n->left->value, t);  /* YES! We can do it! */
}

/* no asm counterpart: the emitter's state, kept around the code written
   for divzerobuild's check */
struct emitsave {
    ULONG pc, bigpc;
    WORD nomemleft, a6, stackuse, sbgot, thisstuse, pushflag, fuckpos;
    struct offnode *firstoff;
    ULONG numoffs;
    BOOL foldnow;
};

static void emitkeep(struct emitsave *s)
{
    s->pc = pc;
    s->bigpc = bigpc;
    s->nomemleft = nomemleft;
    s->a6 = lasta6;
    s->stackuse = stackuse;
    s->sbgot = sbgot;
    s->thisstuse = thisstuse;
    s->pushflag = pushflag;
    s->fuckpos = fuckpos;
    s->firstoff = firstoff;
    s->numoffs = numoffs;
    s->foldnow = foldnow;
}

static void emitback(const struct emitsave *s)
{
    struct offnode *node;

    while (firstoff != s->firstoff) {       /* the code's relocations */
        node = firstoff;
        firstoff = node->next;
        FreeMem(node, 8);
    }
    numoffs = s->numoffs;
    pc = s->pc;
    bigpc = s->bigpc;
    nomemleft = s->nomemleft;
    lasta6 = s->a6;
    stackuse = s->stackuse;
    sbgot = s->sbgot;
    thisstuse = s->thisstuse;
    pushflag = s->pushflag;
    fuckpos = s->fuckpos;
    foldnow = s->foldnow;
}

/* no asm counterpart: the check of a division of two constants by zero
   (divzerochk: "Division by Zero"), made while the expression is built,
   where the original made it, before what is read after the operator n:
   the code of its operands (the actions from the left operand's first,
   l->value, to the last one recorded) and of the operation up to the
   check is written behind pc and run, then everything it changed is set
   back.  The code is written again later, the check then finds the same
   divisor. */
static void divzerobuild(struct exprnode *n, const struct exprtype_bin *d)
{
    struct expract *a = (struct expract *)n->left->value, *last = exprtree_lastact();
    struct exprunit outer;
    struct emitsave s;
    UWORD s_regat = regat, s_depth = evaldepth;
    WORD s_varmode = varmode;

    if ((n->op != ETO_DIV && n->op != ETO_MOD) || d->handler < T_BYTE || d->handler > T_FLOAT
        || (d->path != ETP_PLAIN && d->path != ETP_MIXED) || !a || nomemleft)
        return;
    emitkeep(&s);
    exprtree_begin(&outer);         /* nothing is recorded while it runs */
    for (;; a = a->next) {
        exprtree_runact(a);
        if (a == last)
            break;
    }
    regat = n->reg;
    operate(n, TRUE);
    exprtree_end(&outer);
    emitback(&s);
    regat = s_regat;
    varmode = s_varmode;
    evaldepth = s_depth;
}

/* no asm counterpart: what the code of a conversion from -> to needs
   while the expression is built: "Can't Convert Types" where convtable
   has none, the libraries its routines use (convtable's handlers:
   overchk's routines with runtime error and overflow checking, the FFP
   base, floatquicklib) */
static void overuse(UWORD token)
{
    if ((debugga >> 8) && (optreq2ga4.Flags & GFLG_SELECTED) && !cfetchmode)
        tokuse(token);
}

static void convprep(UWORD from, UWORD to)
{
    WORD idx = (WORD)((from - 1) * 7 + to - 1);

    if (idx < 0 || idx >= 49)
        return;
    if (convtable[idx] == noconerr)
        noconerr();
    if (from == T_ADDR)
        from = T_LONG;
    if (to == T_ADDR)
        to = T_LONG;
    switch (from) {
    case T_BYTE:
        if (to == T_FLOAT)
            tokuse(getffpbase);
        break;
    case T_WORD:
        if (to == T_BYTE)
            overuse(wtobover);
        else if (to == T_FLOAT)
            tokuse(getffpbase);
        break;
    case T_LONG:
        if (to == T_BYTE)
            overuse(ltobover);
        else if (to == T_WORD || to == T_QUICK)
            overuse(ltowover);
        else if (to == T_FLOAT)
            tokuse(getffpbase);
        break;
    case T_QUICK:
        if (to == T_BYTE)
            overuse(qtobover);
        else if (to == T_FLOAT)
            tokuse(qutofl);
        break;
    case T_FLOAT:
        if (to == T_BYTE) {
            overuse(ftobover);
            tokuse(getffpbase);
            overuse(ltobover);
        } else if (to == T_WORD) {
            overuse(ftowover);
            tokuse(getffpbase);
            overuse(ltowover);
        } else if (to == T_LONG) {
            overuse(ftolover);
            tokuse(getffpbase);
        } else if (to == T_QUICK) {
            overuse(ftowover);
            tokuse(fltoqu);
        }
        break;
    }
}

/* no asm counterpart: what doop's handler for op and type t needs while
   the expression is built: "Illegal Operator for Type" where it has none,
   the libraries of its routines (wordop: #1179's inline Word * and /) */
static void opprep(unsigned op, UWORD t, BOOL words)
{
    if (!exprtype_optype(op, t))
        illoperr();
    switch (op) {
    case ETO_ADD:
    case ETO_SUB:
        if (t == T_FLOAT)
            tokuse(getffpbase);
        break;
    case ETO_MUL:
        if (t == T_QUICK)
            tokuse(quickmult);
        else if (t == T_LONG && !words)
            tokuse(longmult);
        else if (t == T_FLOAT)
            tokuse(getffpbase);
        break;
    case ETO_DIV:
        if (t == T_LONG && !words)
            tokuse(longdiv);
        else if (t == T_QUICK)
            tokuse(quickdiv);
        else if (t == T_FLOAT)
            tokuse(getffpbase);
        break;
    case ETO_MOD:
        tokuse(t == T_BYTE ? modbyte : t == T_WORD ? modword : t == T_LONG ? modlong
               : t == T_QUICK ? modquick : modfloat);
        break;
    case ETO_POW:                   /* dopow */
        convprep(t, T_FLOAT);
        convprep(t, T_FLOAT);
        tokuse(mathq * 128 + 1);
        break;
    case ETO_EQ: case ETO_NE: case ETO_LT: case ETO_LE: case ETO_GT: case ETO_GE:
        if (t == T_FLOAT)
            tokuse(getffpbase);
        else if (t == T_STRING)
            tokuse(strcomp);
        break;
    case ETO_LSL: case ETO_ASL: case ETO_LSR: case ETO_ASR:     /* shpoke */
        convprep(t, T_WORD);
        if (t != T_QUICK)
            convprep(t, T_LONG);
        break;
    case ETO_BITTST: case ETO_BITSET: case ETO_BITCLR: case ETO_BITCHG: /* dothebit */
        convprep(t, T_LONG);
        convprep(t, T_BYTE);
        break;
    }
}

/* no asm counterpart: what operate's code for the decision d of op needs
   while the expression is built, in operate's order: the errors and the
   libraries of its conversions and its handler */
static void binopprep(unsigned op, const struct exprtype_bin *d)
{
    if (d->path != ETP_COMPARE && d->rup_from)
        convprep(d->rup_from, d->rup_to);
    if (d->path == ETP_CMPLQ)
        return;
    if (d->path == ETP_COMPARE) {
        if (d->rup_from)
            convprep(d->rup_from, d->rup_to);
        if (d->lconv_from)
            convprep(d->lconv_from, d->lconv_to);
        opprep(op, d->handler, FALSE);
        return;
    }
    if (d->path == ETP_MIXED && !d->handler) {
        if (op == ETO_MUL)
            tokuse(d->lq && d->rq ? qqmult : mixmult);
        else if (op == ETO_DIV)
            tokuse(d->rq && !d->lq ? lqdiv : longdiv);
        return;
    }
    if (d->lconv_from)
        convprep(d->lconv_from, d->lconv_to);
    if (d->path == ETP_MIXED && d->rconv_from)
        convprep(d->rconv_from, d->rconv_to);
    opprep(op, d->handler, d->path == ETP_PLAIN && d->wordop);
}

/* no asm counterpart; replaces blitz2.s:8995-9083 eval5 after the right
   operand, the decisions - the operator n built: typed (exprtype_binop
   with the facts of its operands; the left operand is marked EXF_CMP when
   the result is lowered by a comparison, #1182), its errors raised and
   its libraries used, the fold decided by the forthstack (bforth: the
   pairwise rule, the nocando of ^, a Long or Float Mod and a raw Quick's
   * and /), its code recorded.  right: the type so far after the right
   operand. */
static UWORD binopbuild(struct exprnode *n, UWORD right)
{
    struct exprnode *l = n->left, *r = n->right;
    struct exprtype_binfacts f;
    struct exprtype_bin d;

    memset(&f, 0, sizeof f);
    f.lraw = (l->flags & EXF_RAW) != 0;
    f.rraw = rawq;
    rawq = FALSE;
    f.rcmp = cmpres;
    /* Fixed (#1179): a later index, both operands Word values */
    f.words = laterindex && bforth[nbforth - 2].wordval && bforth[nbforth - 1].wordval;
    /* Fixed (#978): an equality comparison of one byte value with one
       constant from 128 to 255 (exprtree_left's facts) */
    if (l->kind == EXN_LEAF) {
        f.lbyte = (l->flags & EXF_BYTEVAL) != 0;
        f.lconst = (l->flags & EXF_BYTECONST) != 0;
    }
    if (r && r->kind == EXN_LEAF) {
        f.rbyte = (r->flags & EXF_BYTEVAL) != 0;
        f.rconst = (r->flags & EXF_BYTECONST) != 0;
    }
    d = exprtype_binop(n->op, n->tbefore, right, &f);
    n->vtype = right;
    /* the constant takes the byte it stands for, sign-extended as varcont
       extends the byte: "a.b=$90 : If a=$90" matches */
    if (d.byteext == ETB_RIGHT)
        r->right->lflags |= ELF_SIGNEXT;
    else if (d.byteext == ETB_LEFT)
        l->right->lflags |= ELF_SIGNEXT;
    if (f.lraw)
        n->lflags |= BNF_LRAW;
    if (f.rraw)
        n->lflags |= BNF_RRAW;
    if (f.rcmp)
        n->lflags |= BNF_RCMP;
    if (f.words)
        n->lflags |= BNF_WORDS;
    /* the typing pass of rule P+ takes a fraction under AND, OR and EOR
       and a shifted constant with a point (#1216): no error here, the
       pass decides (p5end) */
    if (p5relaxbin(n, &d))
        n->lflags |= BNF_P5 | BNF_NOFOLD;
    else
        binopprep(n->op, &d);
    if (!(bforth[nbforth - 2].constgot | bforth[nbforth - 1].constgot))
        n->lflags |= BNF_FOLD;
    if ((n->lflags & (BNF_FOLD | BNF_P5)) == BNF_FOLD)
        divzerobuild(n, &d);       /* "Division by Zero" before the next element */
    if (n->op == ETO_POW || (n->op == ETO_MOD && (d.handler == T_LONG || d.handler == T_FLOAT))
        || (d.path == ETP_MIXED && !d.handler && (n->op == ETO_MUL || n->op == ETO_DIV))
        || (n->lflags & BNF_P5)) {
        n->lflags |= BNF_NOFOLD;    /* nocando */
        bforth[nbforth - 1].constgot = 0xffff;
    }
    bforth[nbforth - 2].constgot |= bforth[nbforth - 1].constgot;
    bforth[nbforth - 2].wordval = 0;    /* Fixed (#1179): a result */
    nbforth--;
    n->tafter = d.result;
    emitact(binopact, n, 0, 0);
    if (d.cmpres)
        l->flags |= EXF_CMP;        /* Fixed (#1182) */
    return d.result;
}

/* The hooks of the walk (exprtree.h, exprtree_climb: eval5's control),
   blitz2.s:8983 eval5.  The context of one expression (eval4): the type
   so far (asm d2), the current char (d0). */
struct evalctx {
    UWORD *type;
    UWORD c;
};

/* no asm counterpart: the marks of an operand's code (its start and end
   in the leaf of the walk; the start keeps the relocations and lasta6 of
   that place in its action, for the fold: constfold, divzerochk) */
static void startmark(struct expract *a)
{
    ((struct exprnode *)a->p)->start = pc;
    a->x = (ULONG)firstoff;         /* for the fold of a constant from here */
    a->y = (UWORD)lasta6;
}

static void endmark(struct expract *a)
{
    ((struct exprnode *)a->p)->end = pc;
}

static void cutact(struct expract *a)
{
    ULONG at = foldat();

    (void)a;
    quicktolong();
    foldnote(FN_CONV, T_QUICK, T_LONG, 0, at);
}

/* no asm counterpart; replaces blitz2.s:8983 eval5's bsr eval2 - the
   operand l and the operator after it (l->aux: the code recorded before
   it, for exprtree_pre) */
unsigned exprtree_operand(void *x, struct exprnode *l, unsigned *op)
{
    struct evalctx *e = x;

    l->aux = codeseq;
    l->value = (ULONG)stateact(startmark, l, 0, 0);
    return eval2(e->type, &e->c, op);
}

/* no asm counterpart: the constant leaf l is from 128 to 255 as its code
   holds it (isbyteconst: a move.w's word, a move.l's long, a Quick's
   whole part) */
static BOOL byteconst(const struct exprnode *l)
{
    ULONG v = l->value;

    if (l->tafter < T_LONG)
        v = (UWORD)v;
    else if (!(v & 0xffff))
        v >>= 16;                   /* q: x.0 */
    return v >= 128 && v <= 255;
}

/* no asm counterpart; replaces blitz2.s:8983 eval5 after its bsr eval2 -
   the operand read is the leaf l (its code from its start mark to its
   end mark) */
void exprtree_left(void *x, struct exprnode *l, unsigned consumer)
{
    struct evalctx *e = x;

    stateact(endmark, l, 0, 0);
    l->tbefore = *e->type;
    if (rawq)                       /* Fixed (#1178): the left operand raw */
        l->flags |= EXF_RAW;
    if (cmpres)                     /* Fixed (#1182): the left one lowered */
        l->flags |= EXF_CMP;
    /* Fixed (#978): the operand is one byte value (its code ends in the
       byte's ext.w), or one integer constant from 128 to 255 (its code
       is the constant's, l->right its leaf) */
    if (bbyte && bbyte == codeseq && bbyte > l->aux)
        l->flags |= EXF_BYTEVAL;
    if (bcst.seq && bcst.seq == codeseq && bcst.before == l->aux && byteconst(bcst.leaf)) {
        l->flags |= EXF_BYTECONST;
        l->right = bcst.leaf;
    }
    /* Fixed (#1178): a raw Quick whose consumer (the next operator if it
       binds tighter than the pending one, else the pending one) is a
       shift, a logic operator, Mod or a bit operator is rounded down here,
       where varcont cut it: today's value and code (exprtype_cutsraw) */
    if ((l->flags & EXF_RAW) && exprtype_cutsraw(consumer)) {
        emitact(cutact, NULL, 0, 0);
        l->flags &= ~EXF_RAW;
    }
}

/* no asm counterpart; replaces blitz2.s:8987 eval5 .more's rts - the
   level ends with its left operand l */
void exprtree_done(void *x, struct exprnode *l)
{
    (void)x;
    rawq = (l->flags & EXF_RAW) != 0;
    cmpres = (l->flags & EXF_CMP) != 0;
}

/* no asm counterpart; replaces blitz2.s:8989 eval5 .higher's left operand
   of a comparison - does the comparison's left operand l (an element's
   leaf) get its own type back (#1177; decided from the actions recorded
   since l began, l->aux: the code before it)?  A literal made a whole
   number (#1186) that is the operand, with only its minus signs after
   it, is the Float it is: its code the FFP value with the signs in it
   (ELF_FFP), the minus signs' code none (EXF_ABSORBED).  A value whose
   code ends in the conversion contemit makes gets the type it had
   before it (exprtype_ownleft): the conversion is not written
   (ELF_DROP), the relocations of the libraries its routine used first
   are freed, the libraries stay.  Byte/Word to Long is exact and stays.
   Fixed (#1183): a compound left operand (q*2) and a Long constant under
   a Float context were evaluated in the wanted type before the
   comparison operator is read, so a Quick inside was cut (`r.l=(q*2>2.5)`
   with q=1.5 gave 0); the typing pass of rule P+ (exprtyping.c, p5end)
   types the left subtree by its own operands and rewrites its code.
   *type is the type the operand has then. */
static BOOL ownleft(const struct exprnode *l, UWORD *type)
{
    struct exprnode *n;

    if (blit.seq && blit.before == l->aux && blit.last == codeseq) {
        if ((blit.negs & 1) && blit.leaf->aux)
            blit.leaf->aux ^= 0x80;             /* the FFP sign bit */
        blit.leaf->lflags |= ELF_FFP;
        for (n = blit.neg; n; n = n->left)
            n->flags |= EXF_ABSORBED;
        *type = T_FLOAT;
        blit.seq = bcst.seq = 0;
        return TRUE;
    }
    if (bconv.seq && bconv.seq == codeseq && bconv.seq > l->aux && bconv.depth == evaldepth
        && exprtype_ownleft(bconv.from, bconv.to)) {
        bconv.leaf->lflags |= ELF_DROP;
        relocforget(bconv.relocs);
        *type = bconv.from;
        bconv.seq = 0;
        return TRUE;
    }
    return FALSE;
}

/* no asm counterpart; replaces blitz2.s:8989 eval5 .higher before its bsr
   pushprec - the operator n on its left operand */
void exprtree_pre(void *x, struct exprnode *n)
{
    struct evalctx *e = x;
    struct exprnode *l = n->left;
    struct exprtype_left pre;

    /* Fixed (#1185): the right operand of an operator whose left operand
       is in d7 would need a register beyond d7 (the 7th and later
       arguments of a statement are evaluated in d6, so Cmd a1,..,a6,x+y*z
       is enough, or eight nested levels).  The original fetched it into
       d7 over the left operand and took the operand from "d8", a register
       field that reads a0 or changes the opcode: a wrong value, a crash
       of the program, or of the compiler when constant folding ran that
       code.  The expression stack's error instead, before any code of the
       operator or its right operand. */
    if (regat >= 7)
        conmemerr();
    /* Fixed (#1177): the operands of a comparison are compared with their
       own values, not in the type the expression is wanted in (the
       original handed that type to both, and varcont cut a Quick under a
       Long: r.l=(q>1) was 0 for q=1.5).  A left operand of one element
       gets its own type back (ownleft), the right operand
       starts at type 0, compareop compares the two (exprtype_leftop). */
    pre = exprtype_leftop(n->op, *e->type, (l->flags & EXF_RAW) != 0);
    *e->type = pre.left;            /* Fixed (#1178): a raw left is a Quick */
    if (pre.lraw)
        l->flags |= EXF_RAW;
    else
        l->flags &= ~EXF_RAW;
    if (pre.ownleft && l->kind == EXN_LEAF && ownleft(l, e->type))
        n->lflags |= BNF_OWNLEFT;
    /* Fixed (#978): an equality comparison (= or <>) of a byte value with
       a constant from 128 to 255 (each operand one element) takes the
       constant as the byte it stands for, sign-extended as varcont
       extends the byte: "a.b=$90 : If a=$90" matches.  The ordering
       comparisons stay mathematical (a<200 is true for every byte).  The
       facts are the operands' (exprtree_left), the rule exprtype_binop's
       (binopbuild). */
    if (pre.compare)
        n->flags |= EXF_COMPARE;
}

/* no asm counterpart; replaces blitz2.s:8990-8992 eval5 .higher after
   pushprec (move d2,-(a7), addq #1,regat) - the right operand of n is read
   next */
void exprtree_enter(void *x, struct exprnode *n)
{
    struct evalctx *e = x;

    n->tbefore = *e->type;
    n->reg = (UBYTE)regat;
    regat++;
    if (n->flags & EXF_COMPARE)
        *e->type = 0;               /* Fixed (#1177) */
}

/* no asm counterpart; replaces blitz2.s:8993-9083 eval5 after the right
   operand (subq #1,regat, the operation, the constant fold, reget) - n
   built (binopbuild) */
void exprtree_emit(void *x, struct exprnode *n)
{
    struct evalctx *e = x;

    regat--;
    *e->type = binopbuild(n, *e->type);
    p5binop(n, exprtree_lastact()); /* the typing pass's node (#1216) */
    e->c = reget();
}

/* no asm counterpart: the walk's errors (exprtree.h) */
void exprtree_overflow(void)
{
    pserr();
}

/* no asm counterpart: no memory for a further block of the arena; the
   compile error of every allocation the compiler cannot get (#369) */
void exprtree_nomem(void)
{
    nomemerr();
}

/* no asm counterpart: the further blocks of the arena (exprtree.h) */
void *exprtree_getblock(void)
{
    return AllocMem(EXPRTREE_BLOCK, MEMF_PUBLIC);
}

void exprtree_freeblock(void *block)
{
    FreeMem(block, EXPRTREE_BLOCK);
}

/* blitz2.s:8972 eval4 - an expression up to the first char that is no
   operator: its node (kind: EXN_EXPR, EXN_BRACKET, EXN_NOT) counted as
   the asm's pushprec of precedence 1, then the walk.  The node stays
   pending: the caller releases it (eval4end) once it has built what
   belongs to it (asm: subq.l #6,precsp here; nothing is counted in
   between). */
static UWORD eval4(UWORD *type, unsigned kind)
{
    struct evalctx e;
    struct exprnode *last;
    unsigned op;

    e.type = type;
    /* asm: a0 is whatever it was (garbage) for the sentinel */
    exprtree_push(exprtree_new(kind), ETO_END, 1);
    if (exprtree_climb(&e, &op, &last))
        syntaxerr();
    return e.c;
}

/* no asm counterpart: eval4's node released */
static void eval4end(void)
{
    exprtree_release(exprtree_top());
}

/* blitz2.s:8968 eval3 (its sub.l #18,forthsp: the build's forthstack) */
static UWORD eval3(UWORD *type)
{
    UWORD c = eval4(type, EXN_EXPR);

    eval4end();
    nbforth--;
    return c;
}

/* blitz2.s:9085 eval2 - get 1 element and operator.
   Returns op prec (asm d1, 0 if .done), *op = the operator (its index in
   operators[]; asm a0: its address; ETO_END if .done). */
static UWORD eval2(UWORD *type, UWORD *c, unsigned *op)
{
    struct opentry *ent;
    UWORD ch;

    ch = *c = getelement(type);
    for (ent = operators; ent->op; ent++)
        if (ent->op == ch)
            goto found;
    *op = ETO_END;
    return 0;                       /* .done */
found:
    if (ch == '<') {
        ch = *c = getchar3();
        if (ch == '=') {
            ent = ople;
            goto found2;
        }
        if (ch != '>')              /* .notlteq */
            goto found3;
        goto isne;
    }
    if (ch == '>') {                /* .notlt */
        ch = *c = getchar3();
        if (ch == '=') {
            ent = opge;
            goto found2;
        }
        if (ch == '<')              /* .notgteq */
            goto isne;
        goto found3;
    }
    if (ch != '=')                  /* .notgt */
        goto found2;
    ch = *c = getchar3();
    if (ch == '>') {
        ent = opge;
        goto found2;
    }
    if (ch != '<')                  /* .noteqgt */
        goto found3;
    /* Fixed (#9): "=<" is less-or-equal, as "<="; the original fell
       through into .isne and compiled it as "<>" */
    ent = ople;
    goto found2;
isne:
    ent = opne;
    goto found2;
found3:
    bakup();
found2:
    /* We've found the operator. */
    *op = opindex(ent);
    return ent->prec;
}

/* blitz2.s:9142 doop - a0=operator,d2=type.  Whether op has a handler
   for type, and the type it gives, is exprtype_optype's (exprtype.c);
   operators[] has a handler wherever that table has a type; words: the
   operands are two Word values in a later index (wordop, set for the
   handler only).  Returns the type. */
static UWORD doop(struct opentry *op, UWORD type, BOOL words)
{
    ophandler *h;
    UWORD t = exprtype_optype(opindex(op), type);

    if (!t)
        illoperr();
    h = op->h[type - 1];
    if (!h)
        illoperr();                 /* not reached: the table agrees */
    wordop = words;
    h(type);                        /* jmp (a0) */
    wordop = FALSE;
    return t;
}

/* Fixed (#1178), no asm counterpart: the operator op on D(regat) (left)
   and D(regat+1) (right) when one of them is a raw Quick (lq, rq) under a
   Long type so far, or the left one a Quick that a Long right operand
   (a constant beyond the Quick range, &var, ?label) made Long, under * or
   / (lq with left T_QUICK).  The original cut the Quick to a whole number
   when it fetched it (varcont, blitz2.s:10491-10498) or after the right
   operand (convtypef, the late cut): r.l=q*2 with q=1.5 gave 2.  Here the
   operation takes the fraction and gives a Long, rounded at the operator
   (quick-long-arithmetic, design decision 3): + - * down, / towards zero
   as BB2's divisions.  A Float type so far (the right side made it one)
   takes the Quick with its fraction (quicktofloat), ^ both operands with
   theirs; the other operators take the Quick rounded down, as the original does.
   d: exprtype_binop's decision (ETP_MIXED); the operation computes in
   d->result; zeroonly: as operate's. */
static __attribute__((noinline))
void mixedop(struct opentry *op, const struct exprtype_bin *d, BOOL zeroonly)
{
    UWORD r = regat;
    BOOL lq = d->lq, rq = d->rq;

    if (d->handler) {
        /* a Float (or another) type so far set by the right side (the
           right operand is of that type already: the Quick converted
           with its fraction), ^ (both to Float with their fractions),
           or an operator that takes the Quick rounded down: the
           conversions, then the handler */
        if (d->lcut)
            rawcut(r);
        if (d->rcut)
            rawcut(r + 1);
        if (d->lconv_from)
            convtypef(d->lconv_from, d->lconv_to);
        if (d->rconv_from) {
            regat++;
            convtype(d->rconv_from, d->rconv_to);
            regat--;
        }
        divzerochk(op, d->handler);
        if (zeroonly)
            return;
        doop(op, d->handler, FALSE);
        return;
    }
    switch (op->op) {
    case '+':
        if (lq && rq) {
            /* floor(q+p) = floor(q) + floor(p) + the carry of the
               fractions, without a 32 bit raw sum (20000.0+20000.0) */
            doplus2(0xd040);                    /* add.w Dp,Dq: X = carry */
            pokewd(0x4840 | r);                 /* swap Dq */
            pokewd(0x4840 | (r + 1));           /* swap Dp */
            pokewd(0x48c0 | r);                 /* ext.l Dq */
            pokewd(0x48c0 | (r + 1));           /* ext.l Dp */
            doplus2(0xd180);                    /* addx.l Dp,Dq */
            return;
        }
        /* l+q, q+l: floor(l+q) = l + floor(q) */
        rawcut(lq ? r : r + 1);
        doplus2(0xd080);                        /* add.l */
        return;
    case '-':
        if (lq && rq) {
            /* floor(q-p) = floor(q) - floor(p) - the borrow of the
               fractions */
            doplus2(0x9040);                    /* sub.w Dp,Dq: X = borrow */
            pokewd(0x4840 | r);
            pokewd(0x4840 | (r + 1));
            pokewd(0x48c0 | r);
            pokewd(0x48c0 | (r + 1));
            doplus2(0x9180);                    /* subx.l Dp,Dq */
            return;
        }
        if (rq) {
            /* l-q: floor(l-q) = l - ceil(q) = l - floor(q) - (frac <> 0),
               as cmplq takes it (exact also for -32768.0) */
            pokewd(0x4440 | (r + 1));           /* neg.w Dq: X = frac<>0 */
            pokewd(0x4840 | (r + 1));           /* swap Dq */
            pokewd(0x48c0 | (r + 1));           /* ext.l Dq: floor(q) */
            doplus2(0x9180);                    /* subx.l Dq,Dl */
            return;
        }
        rawcut(r);                              /* q-l: floor(q) - l */
        doplus2(0x9080);                        /* sub.l */
        return;
    case '*':
        /* floor(l*q): bits 16..47 of the 64 bit product; floor(q*p): bits
           32..63 (lmullib, the port's entries) */
        domylib(lq && rq ? qqmult : mixmult);
        return;
    case '/':
        if (rq && !lq) {
            domylib(lqdiv);                     /* trunc((l<<16)/q) */
            return;
        }
        domylib(longdiv);       /* q/h: trunc(qraw/hraw); q/l: qraw/l */
        if (!rq) {
            /* q/l: the raw quotient (towards zero) to a whole number
               towards zero */
            pokewd(0x4a80 | r);                 /* tst.l Dn */
            pokewd(0x6a06);                     /* bpl.s *+8 */
            pokewd(0x0680 | r);                 /* addi.l #$FFFF,Dn */
            pokel(0x0000ffff);
            pokewd(0x4840 | r);                 /* swap Dn */
            pokewd(0x48c0 | r);                 /* ext.l Dn */
        }
        return;
    }
    /* not reached: exprtype_binop gives every other operator a handler
       (Mod with a whole-number divisor, a shift or a logic operator: the
       Quick rounded down, as the original does; exprtype_cutsraw cut a left operand
       at fetch already) */
}

/* Fixed (#1044): not in blitz2.s - a division or Mod by a constant 0.
   When both operands are constants, eval5 folds the operation by running
   the code just generated in the compiler's own process (blitz2.s:9007),
   without looking at the divisor: a Byte or Word division and Mod
   (DIVS.W, modlib) and a Float division (mathffp SPDiv) by 0 raised the
   CPU's division trap in the compiler (exec's alert 8000 0005, no
   result), a Long or Quick one gave ldivlib's $7FFFFFFF, a Long Mod the
   dividend.  Before doop, with both operands constant (also in cfetchmode:
   #c=..., Dim, CNIF), the operands' code is run up to the divisor (the
   right operand, D(regat+1), already of the operation's type) and a
   divisor 0 is "Division by Zero": the low word for Byte and Word (what
   DIVS.W divides by), the long for Long and Quick, a mantissa 0 for Float
   (FFP zero).  The two instructions poked for the run are overwritten by
   doop: pc, bigpc and nomemleft are restored, no code changes. */
static void divzerochk(const struct opentry *op, UWORD type)
{
    struct m68kregs r;
    ULONG at = pc, big = bigpc, value;
    WORD s_nomemleft = nomemleft;
    BOOL divzero;

    foldmodtrap = FALSE;
    if ((op != opdiv && op != opmod) || type < T_BYTE || type > T_FLOAT)
        return;                     /* doop refuses the other types */
    if (!foldnow || nomemleft)
        return;                     /* not folded: the program divides */
    {
        /* DIFF: the divisor computed in C from the notes of the operands'
           code (the fold in C), where they cover it */
        struct exprfold_reg left, right;
        int isnull;

        if (foldoperands(foldfrom, &left, &right) && exprfold_prepare(foldop, &foldd, &left, &right)
            && (isnull = exprfold_divzero(foldop, &foldd, &right)) >= 0) {
            if (isnull)
                divzeroerr();
            return;
        }
    }
    pokewd(0x2000 | (regat + 1));   /* move.l D(regat+1),d0 */
    pokewd(0x4e75);                 /* rts */
    if (nomemleft) {                /* doop's code reaches as far */
        pc = at;
        bigpc = big;
        nomemleft = s_nomemleft;
        return;
    }
    memset(&r, 0, sizeof(r));       /* the registers of the fold */
    r.a[6] = (ULONG)MathBase;
    r.a[5] = (ULONG)&MathBase - (LONG)ffplib->varoff;
    flushc();
    callregs((APTR)foldfrom, &r);
    value = r.d[0];
    pc = at;
    bigpc = big;
    if (type < T_LONG)
        divzero = (UWORD)value == 0;
    else if (type == T_FLOAT)
        divzero = (value >> 8) == 0;
    else
        divzero = value == 0;
    if (divzero)
        divzeroerr();
    foldmodtrap = type == T_QUICK && op == opmod && (value >> 16) == 0;
}

/* ============================================ PLUS / compare operators */

/* blitz2.s:9292 cmpit - cmp.x D(regat+1),D(regat) */
static void cmpit(UWORD opcode)
{
    UWORD reg = regat;
    opcode |= reg;
    opcode |= reg << 9;
    opcode += 1;
    pokewd(opcode);
}

/* blitz2.s:9322 cpoke - Scc Dn / ext.w Dn; type now WORD! */
static UWORD cpoke(UWORD opcode)
{
    pokewd(opcode | regat);
    pokewd(0x4880 | regat);         /* ext.w D regat */
    return T_WORD;
}

/* blitz2.s:9301..9321 doXXcmp / doXXcmp2 */
static UWORD doeqcmp(void) { return cpoke(*mseq); }
static UWORD donecmp(void) { return cpoke(*msne); }
static UWORD doltcmp(void) { return cpoke(*mslt); }
static UWORD dolecmp(void) { return cpoke(*msle); }
static UWORD dogtcmp(void) { return cpoke(*msgt); }
static UWORD dogecmp(void) { return cpoke(*msge); }
static UWORD doeqcmp2(UWORD opcode) { cmpit(opcode); return doeqcmp(); }
static UWORD donecmp2(UWORD opcode) { cmpit(opcode); return donecmp(); }
static UWORD doltcmp2(UWORD opcode) { cmpit(opcode); return doltcmp(); }
static UWORD dolecmp2(UWORD opcode) { cmpit(opcode); return dolecmp(); }
static UWORD dogtcmp2(UWORD opcode) { cmpit(opcode); return dogtcmp(); }
static UWORD dogecmp2(UWORD opcode) { cmpit(opcode); return dogecmp(); }

/* Fixed (#1177), no asm counterpart: D(regat) OP D(regat+1), one a Long,
   the other a Quick (raw 16.16), compared exactly: the Long against
   floor(q) (the high word, sign-extended), the fraction (the low word)
   deciding on equality - (l:0) against (floor(q):frac) as 48 bits.  The
   original converted the Long to Quick (its low word only) or cut the
   Quick.  The Quick's register is used up; the boolean in D(regat) as a
   Word, as cpoke leaves it. */
static UWORD cmplq(struct opentry *op, UWORD left)
{
    UWORD rl = regat + 1, rq = regat, sc;

    if (left == T_LONG) {
        rl = regat;
        rq = regat + 1;
    } else if (op == oplt)          /* q OP l is l OP' q */
        op = opgt;
    else if (op == opgt)
        op = oplt;
    else if (op == ople)
        op = opge;
    else if (op == opge)
        op = ople;
    if (op == oplt || op == opge) {
        pokewd(0x4440 | rq);                    /* neg.w Dq: X = frac<>0 */
        pokewd(0x4840 | rq);                    /* swap Dq */
        pokewd(0x48c0 | rq);                    /* ext.l Dq: floor(q) */
        pokewd(0x9180 | (rl << 9) | rq);        /* subx.l Dq,Dl: N^V = l<q */
        sc = op == oplt ? *mslt : *msge;
    } else if (op == opgt || op == ople) {
        pokewd(0x4840 | rq);                    /* swap Dq */
        pokewd(0x48c0 | rq);                    /* ext.l Dq */
        pokewd(0xb080 | (rl << 9) | rq);        /* cmp.l Dq,Dl */
        sc = op == opgt ? *msgt : *msle;
    } else {
        pokewd(0x4a40 | rq);                    /* tst.w Dq: a fraction */
        pokewd(0x6606);                         /* bne.s Scc: not equal */
        pokewd(0x4840 | rq);                    /* swap Dq */
        pokewd(0x48c0 | rq);                    /* ext.l Dq */
        pokewd(0xb080 | (rl << 9) | rq);        /* cmp.l Dq,Dl */
        sc = op == opeq ? *mseq : *msne;
    }
    return cpoke(sc);
}

/* Fixed (#1177), no asm counterpart: the comparison of a left operand of
   type left in D(regat) with a right one of its own type right in
   D(regat+1).  A Long and a Quick are compared exactly (cmplq); otherwise
   the lower of the two is converted up (the right one here, the left one by
   convtypef as operate does) and the handler of the common type compares.
   d: exprtype_binop's decision (ETP_CMPLQ or ETP_COMPARE). */
static __attribute__((noinline))
void compareop(struct opentry *op, UWORD left, const struct exprtype_bin *d)
{
    if (d->path == ETP_CMPLQ) {
        cmplq(op, left);
        return;
    }
    if (d->rup_from) {
        regat++;
        convtype(d->rup_from, d->rup_to);
        regat--;
    }
    if (d->lconv_from)
        convtypef(d->lconv_from, d->lconv_to);  /* could possibly fuck regat+1 ! */
    doop(op, d->handler, FALSE);
}

/* blitz2.s:9189-9226 byte/word/long compares */
UWORD doeqb(UWORD t) { (void)t; return doeqcmp2(*cmp0); }
UWORD doneb(UWORD t) { (void)t; return donecmp2(*cmp0); }
UWORD doltb(UWORD t) { (void)t; return doltcmp2(*cmp0); }
UWORD doleb(UWORD t) { (void)t; return dolecmp2(*cmp0); }
UWORD dogtb(UWORD t) { (void)t; return dogtcmp2(*cmp0); }
UWORD dogeb(UWORD t) { (void)t; return dogecmp2(*cmp0); }
UWORD doeqw(UWORD t) { (void)t; return doeqcmp2(*cmp1); }
UWORD donew(UWORD t) { (void)t; return donecmp2(*cmp1); }
UWORD doltw(UWORD t) { (void)t; return doltcmp2(*cmp1); }
UWORD dolew(UWORD t) { (void)t; return dolecmp2(*cmp1); }
UWORD dogtw(UWORD t) { (void)t; return dogtcmp2(*cmp1); }
UWORD dogew(UWORD t) { (void)t; return dogecmp2(*cmp1); }
UWORD doeql(UWORD t) { (void)t; return doeqcmp2(*cmp2_); }
UWORD donel(UWORD t) { (void)t; return donecmp2(*cmp2_); }
UWORD doltl(UWORD t) { (void)t; return doltcmp2(*cmp2_); }
UWORD dolel(UWORD t) { (void)t; return dolecmp2(*cmp2_); }
UWORD dogtl(UWORD t) { (void)t; return dogtcmp2(*cmp2_); }
UWORD dogel(UWORD t) { (void)t; return dogecmp2(*cmp2_); }

/* blitz2.s:9229 docflib - FFP lib call whose CCR result is used: the Scc
   goes to d0 (regat=0), docflib2 moves it back.  asm keeps d5 (pull
   mask) in the register across the Scc code and regat in cfregat
   (blitz2.s:9228); C hands both to docflib2 in a struct cfstate. */
struct cfstate {
    ULONG pull;                     /* asm d5 */
    UWORD regat;                    /* asm cfregat */
};
static void docflib(WORD lvo, struct cfstate *s)
{
    ULONG push;

    libjsr[1] = (UWORD)lvo;
    savereg(&push, &s->pull);
    pokemovem(push);
    tokejsr(getffpbase);
    ritetod0();
    pokel(L_(libjsr));
    s->regat = regat;
    regat = 0;
}

/* blitz2.s:9242 docflib2 */
static void docflib2(const struct cfstate *s)
{
    regat = s->regat;
    ritefromd0();
    pokemovem(s->pull);
}

/* blitz2.s:9248-9271 float compares (SPCmp): docflib, Scc, docflib2 */
static UWORD dofcmp(UWORD (*cmp)(void))
{
    struct cfstate s;
    UWORD t;

    docflib(-42, &s);
    t = cmp();
    docflib2(&s);
    return t;
}
UWORD doeqf(UWORD t) { (void)t; return dofcmp(doeqcmp); }
UWORD donef(UWORD t) { (void)t; return dofcmp(donecmp); }
UWORD doltf(UWORD t) { (void)t; return dofcmp(doltcmp); }
UWORD dolef(UWORD t) { (void)t; return dofcmp(dolecmp); }
UWORD dogtf(UWORD t) { (void)t; return dofcmp(dogtcmp); }
UWORD dogef(UWORD t) { (void)t; return dofcmp(dogecmp); }

/* blitz2.s:9273-9290 string compares */
UWORD doeqs(UWORD t) { (void)t; tokejsr(strcomp); return doeqcmp(); }
UWORD dones(UWORD t) { (void)t; tokejsr(strcomp); return donecmp(); }
UWORD dolts(UWORD t) { (void)t; tokejsr(strcomp); return doltcmp(); }
UWORD doles(UWORD t) { (void)t; tokejsr(strcomp); return dolecmp(); }
UWORD dogts(UWORD t) { (void)t; tokejsr(strcomp); return dogtcmp(); }
UWORD doges(UWORD t) { (void)t; tokejsr(strcomp); return dogecmp(); }

/* blitz2.s:9343 doplus2 - op.x D(regat+1),D(regat) */
static void doplus2(UWORD opcode)
{
    UWORD reg = regat;
    opcode |= reg;
    opcode += 1;
    opcode |= reg << 9;
    pokewd(opcode);
}

/* blitz2.s:9334 doplusb - now a word */
UWORD doplusb(UWORD t)
{
    (void)t;
    doplus2(0xd040);
    return T_WORD;
}

/* blitz2.s:9337 doplusw - now a long */
UWORD doplusw(UWORD t)
{
    (void)t;
    doplus2(0xd040);
    wordtolong();
    return T_LONG;
}

/* blitz2.s:9341 doplusl - add regat+1 to regat */
UWORD doplusl(UWORD t)
{
    doplus2(0xd080);
    return t;
}

/* blitz2.s:9351 doplusf - ffp add; falls through into doflib */
UWORD doplusf(UWORD t)
{
    doflib(-66);
    return t;
}

/* blitz2.s:9355 doflib - same code as fdo2 */
static void doflib(WORD lvo)
{
    ULONG push, pull;

    libjsr[1] = (UWORD)lvo;
    savereg(&push, &pull);
    pokemovem(push);
    tokejsr(getffpbase);
    ritetod0();
    pokel(L_(libjsr));
    ritefromd0();
    pokemovem(pull);
}

/* blitz2.s:9368 addstrings - add the lengths on the stack.
   Fixed (#971): at regat 6 (an argument from the 7th on, or an addition
   nested that deep) the right string was read by getelement's spill:
   move.l d6,-(a7) before it, move.l (a7)+,d6 after it, which pops the
   right length instead of the saved d6, so blitz2's move.l (a7)+,d6 here
   popped the saved d6 and added it to the left length.  The saved d6 is
   removed instead, as AmiBlitz3 does (AmiBlitz3.ab3:14334 add_strings:
   addq.l #4,a7); the right length is in d6 then. */
UWORD addstrings(UWORD t)
{
    UWORD regfield = regat << 9;
    if (regat == 6)
        pokewd(0x588f);                     /* addq.l #4,a7 */
    else
        pokewd(regfield | addcode[0]);      /* move.l (a7)+,Dn */
    pokewd(regfield | addcode[1]);          /* add.l Dn,(a7) */
    return t;
}

/* ---------------------------------------------------------------- MINUS */

/* blitz2.s:9380 dominusb ("moveq #2,d2 ;now a word!" commented out) */
UWORD dominusb(UWORD t) { doplus2(0x9040); return t; }
/* blitz2.s:9383 dominusw */
UWORD dominusw(UWORD t) { doplus2(0x9040); return t; }
/* blitz2.s:9386 dominusl */
UWORD dominusl(UWORD t) { doplus2(0x9080); return t; }
/* blitz2.s:9388 dominusf */
UWORD dominusf(UWORD t) { doflib(-72); return t; }

/* ---------------------------------------------------------------- TIMES */

/* blitz2.s:9402 dotimesb - muls.w, now a word */
UWORD dotimesb(UWORD t)
{
    (void)t;
    doplus2(0xc1c0);
    return T_WORD;
}

/* blitz2.s:9406 dotimesw - now a long. */
UWORD dotimesw(UWORD t)
{
    (void)t;
    doplus2(0xc1c0);
    return T_LONG;
}

/* blitz2.s:9410 dotimesq */
UWORD dotimesq(UWORD t)
{
    domylib(quickmult);
    return t;
}

/* blitz2.s:9413 dotimesl - falls through into domylib */
UWORD dotimesl(UWORD t)
{
    /* Fixed (#1179): two Word values in a later index (wordop, doop):
       muls.w D(regat+1),D(regat), the exact 32-bit product, instead of
       lmullib */
    if (wordop) {
        doplus2(0xc1c0);
        return t;
    }
    domylib(longmult);
    return t;
}

/* blitz2.s:9414 domylib - call Blitz lib routine on D(regat),D(regat+1) */
static void domylib(UWORD tok)
{
    ULONG push, pull;

    savereg(&push, &pull);
    pokemovem(push);
    ritetod0();
    tokejsr(tok);
    ritefromd0();
    pokemovem(pull);
}

/* blitz2.s:9425 dotimesf */
UWORD dotimesf(UWORD t)
{
    doflib(-78);
    return t;
}

/* blitz2.s:9428 dopow - both to float, then the math lib.
   Fixed (#1189) in the library: the original's call is the inline
   mathtrans SPPow (mathtranslib dopow, jsr -90(a6)), which drops the sign
   of a negative base: (-2.5)^3 gave 15.625, also for a whole-number odd
   exponent.  mathtranslib's dopow is now a routine that keeps the sign of
   a whole odd power; the compiler's code is the same (a call of the
   library entry with a6 loaded).  AB3 mode calls AmiBlitz3's library 44
   (mathq), which keeps the original's value. */
UWORD dopow(UWORD t)
{
    convtypef(t, T_FLOAT);          /* now a float */
    regat++;
    convtype(t, T_FLOAT);
    regat--;
    domylib(mathq * 128 + 1);
    return T_FLOAT;
}

/* ------------------------------------------------------------------ MOD */

/* blitz2.s:9449 domodb */
UWORD domodb(UWORD t) { domylib(modbyte); return t; }
/* blitz2.s:9452 domodw */
UWORD domodw(UWORD t) { domylib(modword); return t; }

/* blitz2.s:9455 domodl - modlib uses an alibjsr here! */
UWORD domodl(UWORD t)
{
    domylib(modlong);
    return t;
}

/* blitz2.s:9459 domodq - now a word. */
UWORD domodq(UWORD t)
{
    (void)t;
    domylib(modquick);
    return T_WORD;
}

/* blitz2.s:9463 domodf - modlib lib uses an alibjsr! */
UWORD domodf(UWORD t)
{
    (void)t;
    /* result type long ("now a long"): modlib returns a long - no bug */
    domylib(modfloat);
    return T_LONG;
}

/* --------------------------------------------------------------- DIVIDE */

/* blitz2.s:9472 dodivb - ext.l both; falls through into dodivw */
UWORD dodivb(UWORD t)
{
    UWORD opcode = 0x48c0 | regat;
    pokewd(opcode);
    pokewd(opcode + 1);
    return dodivw(t);
}

/* blitz2.s:9478 dodivw - ext.l Dn / divs.w D(regat+1),Dn */
UWORD dodivw(UWORD t)
{
    pokewd(0x48c0 | regat);
    doplus2(0x81c0);
    return t;
}

/* blitz2.s:9483 dodivl */
UWORD dodivl(UWORD t)
{
    /* Fixed (#1179): two Word values in a later index (wordop, doop):
       divs.w instead of ldivlib.  The quotient of two sign-extended Words
       fits in a Word except -32768 / -1 (V set, the dividend unchanged),
       whose Long quotient 32768 is the dividend negated:
       divs.w D(regat+1),D(regat) / bvc.s +4 / neg.l D(regat) / bra.s +2 /
       ext.l D(regat).  A divisor 0 traps in the CPU as the Word division
       does (with the debugger "Division by Zero", as ldivlib's divchk). */
    if (wordop) {
        doplus2(0x81c0);
        pokewd(0x6804);
        pokewd(0x4480 | regat);
        pokewd(0x6002);
        pokewd(0x48c0 | regat);
        return t;
    }
    domylib(longdiv);
    return t;
}
/* blitz2.s:9485 dodivq */
UWORD dodivq(UWORD t) { domylib(quickdiv); return t; }
/* blitz2.s:9487 dodivf */
UWORD dodivf(UWORD t) { doflib(-84); return t; }

/* ----------------------------------------------------------- AND / OR */

/* Fixed (#1209), no asm counterpart: AND, OR and EOR (also & and |) and
   Not under a Quick type so far work on the whole parts of their Quick
   operands, the 16 fraction bits dropped (rounded down, -1.5 is -2, as
   quicktoword); the result stays a Quick.  The operation on the 16.16
   values followed by clr.w Dn is bit for bit "the operands to Word, the
   operation, back to Quick": its integer part is the operation of the
   integer parts, its fraction 0.  The original operated on the 16.16
   values (the Long handlers in the Quick column of operators[], not.l in
   notit): Not of a whole number had the fraction $FFFF (h=True : h=NOT h
   gave 0.99998, true; NOT 3 -3.0000153), AND, OR and EOR combined the
   fractions (1.5 OR 2.5 gave 3.5).  The shifts keep the 16.16 value, as
   the original does on purpose (blitz2.s:9555).  Under a Long type so far
   a raw Quick is rounded down before these operators already (#1178). */
static void quickwhole(UWORD t)
{
    if (t == T_QUICK)
        pokewd(0x4240 | regat);     /* clr.w Dn */
}

/* blitz2.s:9492-9506; doandl and doorl are the Long and the Quick
   handler (Fixed (#1209): quickwhole for a Quick).
   Fixed (#1218, #1217): doandb, doorb and doeorb write the Word opcodes
   (and.w, or.w, eor.w; blitz2.s:9492, :9501, :9510 write and.b, or.b,
   eor.b, the same size).  A Byte lies in a register as a sign-extended
   Word, and the byte operation keeps bits 8-15 of the left operand
   (-2 AND 0 is $FF00, 3 OR -2 is $00FF); a widening writes no ext.w
   (bytetoword), so it took that stale upper byte as part of the value.
   Two sign-extended words combined bitwise give a sign-extended word
   with the same low byte. */
UWORD doandb(UWORD t) { doplus2(0xc040); return t; }
UWORD doandw(UWORD t) { doplus2(0xc040); return t; }
UWORD doandl(UWORD t) { doplus2(0xc080); quickwhole(t); return t; }
UWORD doorb(UWORD t) { doplus2(0x8040); return t; }
UWORD doorw(UWORD t) { doplus2(0x8040); return t; }
UWORD doorl(UWORD t) { doplus2(0x8080); quickwhole(t); return t; }

/* ------------------------------------------------------------------ EOR */

/* blitz2.s:9512 doeor2 - eor.x D(regat+1),D(regat) (EOR: source Dn in
   bits 9-11, destination in the ea field) */
static void doeor2(UWORD opcode)
{
    opcode |= regat;
    opcode |= (regat + 1) << 9;
    pokewd(opcode);
}

/* blitz2.s:9510 doeorb - falls through into doeor2; the Word opcode: Fixed (#1218, #1217), see doandb */
UWORD doeorb(UWORD t) { doeor2(0xb140); return t; }
/* blitz2.s:9519 doeorw */
UWORD doeorw(UWORD t) { doeor2(0xb140); return t; }
/* blitz2.s:9521 doeorl - the Long and the Quick handler (Fixed (#1209):
   quickwhole for a Quick) */
UWORD doeorl(UWORD t) { doeor2(0xb180); quickwhole(t); return t; }

/* -------------------------------------------------------------- shifts */

/* blitz2.s:9547 shpoke - lsx.l D(regat+1),D(regat) */
static UWORD shpoke(UWORD opcode, UWORD t)
{
    regat++;                        /* make second a word! */
    convtype(t, T_WORD);
    regat--;
    if (t != T_QUICK) {             /* don't fuck with quix */
        convtypef(t, T_LONG);       /* make first (and current) a long */
        t = T_LONG;                 /* review: asm moveq #3,d2 before
                                       convtypef - the result is a long */
    }
    opcode |= regat;                /* .skip */
    opcode |= (regat + 1) << 9;
    pokewd(opcode);
    return t;
}

/* blitz2.s:9526-9528 dolslb/dolslw/dolsll (one routine) */
UWORD dolslb(UWORD t) { return shpoke(0xe1a8, t); }
UWORD dolslw(UWORD t) { return shpoke(0xe1a8, t); }
UWORD dolsll(UWORD t) { return shpoke(0xe1a8, t); }
/* blitz2.s:9533-9535 dolsrb/dolsrw/dolsrl (andi.l commented out) */
UWORD dolsrb(UWORD t) { return shpoke(0xe0a8, t); }
UWORD dolsrw(UWORD t) { return shpoke(0xe0a8, t); }
UWORD dolsrl(UWORD t) { return shpoke(0xe0a8, t); }
/* blitz2.s:9540-9542 doasrb/doasrw/doasrl */
UWORD doasrb(UWORD t) { return shpoke(0xe0a0, t); }
UWORD doasrw(UWORD t) { return shpoke(0xe0a0, t); }
UWORD doasrl(UWORD t) { return shpoke(0xe0a0, t); }

/* --------------------------------------------------------------- bittst */

/* blitz2.s:9599 dothebit - bxxx D(regat+1),D(regat) */
static void dothebit(UWORD op, UWORD t)
{
    convtypef(t, T_LONG);           /* first to long! */
    regat++;
    convtype(t, T_BYTE);            /* second to byte! */
    regat--;
    pokewd(op | regat | (regat + 1) << 9);  /* btst regat+1,regat */
}

/* blitz2.s:9572 bittst */
UWORD bittst(UWORD t)
{
    dothebit(0x100, t);
    pokewd(regat | 0x56c0);         /* sne regat */
    pokewd(regat | 0x4880);         /* ext.w D regat */
    return T_WORD;                  /* type now WORD! */
}

/* blitz2.s:9586 bitset - falls through into dobitmip */
UWORD bitset(UWORD t)
{
    dothebit(0x1c0, t);             /* dobitmip */
    return T_LONG;                  /* type now LONG! */
}

/* blitz2.s:9593 bitclr */
UWORD bitclr(UWORD t)
{
    dothebit(0x180, t);
    return T_LONG;
}

/* blitz2.s:9596 bitchg */
UWORD bitchg(UWORD t)
{
    dothebit(0x140, t);
    return T_LONG;
}

/* blitz2.s:14704 operators
   dc operator ascii / dc.l ..b,.w,.l,.q,.f,.a,.s / dc precedence */
struct opentry operators[] = {
    { '+', { doplusb, doplusw, doplusl, doplusl, doplusf, 0, addstrings }, 3000 },
    { '-', { dominusb, dominusw, dominusl, dominusl, dominusf, 0, 0 }, 3000 },
    { '*', { dotimesb, dotimesw, dotimesl, dotimesq, dotimesf, 0, 0 }, 4000 },
    { '/', { dodivb, dodivw, dodivl, dodivq, dodivf, 0, 0 }, 4000 },
    { '&', { doandb, doandw, doandl, doandl, 0, 0, 0 }, 5000 },       /* AND */
    { '|', { doorb, doorw, doorl, doorl, 0, 0, 0 }, 5000 },           /* OR */
    { TOK_ASM_AND, { doandb, doandw, doandl, doandl, 0, 0, 0 }, 1000 }, /* AND */
    { TOK_ASM_OR, { doorb, doorw, doorl, doorl, 0, 0, 0 }, 1000 },      /* OR */
    { TOK_ASM_EOR, { doeorb, doeorw, doeorl, doeorl, 0, 0, 0 }, 1000 }, /* EOR */
    { TOK_ASM_LSL, { dolslb, dolslw, dolsll, dolsll, 0, 0, 0 }, 6000 }, /* LSL */
    { TOK_ASM_ASL, { dolslb, dolslw, dolsll, dolsll, 0, 0, 0 }, 6000 }, /* ASL */
    { TOK_ASM_LSR, { dolsrb, dolsrw, dolsrl, dolsrl, 0, 0, 0 }, 6000 }, /* LSR */
    { TOK_ASM_ASR, { doasrb, doasrw, doasrl, doasrl, 0, 0, 0 }, 6000 }, /* ASR */
    { TOK_MOD, { domodb, domodw, domodl, domodq, domodf, 0, 0 }, 5000 },
    { '^', { dopow, dopow, dopow, dopow, dopow, 0, 0 }, 7000 },
    /* [15] opeq */ { '=', { doeqb, doeqw, doeql, doeql, doeqf, doeql, doeqs }, 2000 },
    /* [16] opne */ { 0x3c3e, { doneb, donew, donel, donel, donef, donel, dones }, 2000 },
    /* [17] oplt */ { '<', { doltb, doltw, doltl, doltl, doltf, doltl, dolts }, 2000 },
    /* [18] ople */ { 0x3c3d, { doleb, dolew, dolel, dolel, dolef, dolel, doles }, 2000 },
    /* [19] opgt */ { '>', { dogtb, dogtw, dogtl, dogtl, dogtf, dogtl, dogts }, 2000 },
    /* [20] opge */ { 0x3e3d, { dogeb, dogew, dogel, dogel, dogef, dogel, doges }, 2000 },
    { TOK_BITTST, { bittst, bittst, bittst, bittst, 0, 0, 0 }, 8000 },
    { TOK_BITSET, { bitset, bitset, bitset, bitset, 0, 0, 0 }, 8000 },
    { TOK_BITCLR, { bitclr, bitclr, bitclr, bitclr, 0, 0, 0 }, 8000 },
    { TOK_BITCHG, { bitchg, bitchg, bitchg, bitchg, 0, 0, 0 }, 8000 },
    { 0, { 0 }, 0 },
};
/* exprtype.h numbers the entries (enum exprtype_op) */
_Static_assert(sizeof operators / sizeof operators[0] == ETO_COUNT + 1, "exprtype.h: the entries of operators[]");
_Static_assert(ETO_DIV == 3 && ETO_MOD == 13 && ETO_EQ == 15 && ETO_NE == 16 && ETO_LT == 17 && ETO_LE == 18
               && ETO_GT == 19 && ETO_GE == 20, "exprtype.h: the entries eval.h names");

/* ========================================================= register moves */

/* blitz2.s:9623 ritetod0 - put regat to d0 (and regat+1 to d1) */
void ritetod0(void)
{
    UWORD opcode;

    if (!regat)
        return;
    opcode = regat | 0x2000;
    pokewd(opcode);                 /* move.l Dn,d0 */
    opcode += 0x201;
    pokewd(opcode);                 /* move.l D(n+1),d1 */
}

/* blitz2.s:9633 ritefromd0 - get regat from d0 */
void ritefromd0(void)
{
    if (!regat)
        return;
    pokewd((regat << 9) | 0x2000);
}

/* ============================================================== elements */

/* no asm counterpart: pokeact for getelement's code around an element in
   d6, noted for the fold in C: the push of d6 (x: its opcode), the move
   to d7 and the pull (x with ACT_TWO, y) */
static void spillact(struct expract *a)
{
    ULONG at = foldat();

    pokeact(a);
    if (a->x & ACT_TWO)
        foldnote(FN_PULL, a->x & 0x1000 ? 2 : 4, a->y & 0x1000 ? 2 : 4, 0, at);
    else
        foldnote(FN_PUSH, a->x & 0x1000 ? 2 : 4, 0, 0, at);
}

/* blitz2.s:9643 getelement - out of registers (regat >= 7): use d6 and
   keep the old d6 on the stack.  Only regat 7 comes here: exprtree_pre refuses an
   operand beyond d7 (Fixed (#1185)). */
static UWORD getelement(UWORD *type)
{
    UWORD opcode, c;

    if (regat < 7)
        return getelement2(type);
    /* Fixed (#1177): no wanted type yet (the right operand of a
       comparison) saves the long; the element may be a Long or Quick and
       a move.w would bring back only the low word of the old d6 */
    opcode = *type && *type < 3 ? 0x3f06 : 0x2f06;  /* move.x d6,-(a7) */
    emitact(spillact, NULL, opcode, 0);
    regat--;
    c = getelement2(type);
    regat++;
    emitact(spillact, NULL, ACT_TWO | (*type < 3 ? 0x3e06 : 0x2e06), /* move.x d6,d7 */
            (opcode & 0xf000) | 0x0c1f);                         /* move.x (a7)+,d6 */
    return c;
}

/* no asm counterpart; replaces blitz2.s:9667 negate's .doneg - the code of
   the run of minus signs n (op: how many) on a value of type a->y: one
   neg (or SPNeg) per minus sign */
static void negemit(struct expract *a)
{
    const struct exprnode *n = a->p;
    UWORD k;
    ULONG at;

    if (n->flags & EXF_ABSORBED)
        return;                     /* in the literal's FFP value (#1186) */

    at = foldat();
    for (k = n->op; k; k--) {
        if (a->y >= 5)
            fdo2(-60);              /* SPNeg */
        else
            pokewd((a->y >= 3 ? 0x4480 : 0x4440) | regat);  /* neg.l / neg.w */
    }
    foldnote(FN_NEG, a->y, n->op, 0, at);
}

/* no asm counterpart; replaces blitz2.s:9667 negate and its recursion
   through getelement2 for each further minus sign - a run of minus signs
   (the first one read by getelement2) and the element after it, then one
   neg (or SPNeg) per minus sign, innermost first, as the original's
   recursion emitted them.  The run is a node (EXN_NEG, op: how many) and
   a loop: each further minus sign does what getelement2 did before its
   '-' (Fixed (#1070): negnext flips with each one; the flags of #1177,
   #1178 and #1182 are cleared; the intrinsics count it as an element),
   so a run of any length needs no stack of its own. */
static UWORD negate(UWORD *type)
{
    struct exprnode *n = exprtree_new(EXN_NEG);
    struct expract *prev = exprtree_building() ? exprtree_lastact() : NULL;
    UWORD c;

    n->op = 1;
    n->reg = (UBYTE)regat;
    while ((c = getchar3()) == '-') {
        negnext = !negnext;
        rawq = FALSE;
        cmpres = FALSE;
        bconv.seq = 0;
        elemcount++;
        n->op++;
    }
    c = element(c, type);
    n->tbefore = n->tafter = *type;
    if (*type >= 6)
        illoperr();
    if (*type >= 5)
        tokuse(getffpbase);
    emitact(negemit, n, 0, *type);
    p5wrap(ETN_NEG, n, prev, exprtree_lastact());   /* the typing pass (#1216) */
    exprtree_release(n);
    return c;
}

/* blitz2.s:9681 bracket */
static UWORD bracket(UWORD *type)
{
    UWORD c = eval4(type, EXN_BRACKET);

    eval4end();
    if (c != ')' && c != '}')
        syntaxerr();
    return getchar3();
}

/* no asm counterpart; replaces blitz2.s:9688-9697 notit's code - not.w
   or not.l of a value of type a->y (Fixed (#1209): of a Quick's whole
   part) */
static void notemit(struct expract *a)
{
    UWORD opcode = regat | 0x4640;  /* not.w */
    ULONG at = foldat();

    if (a->y >= 3)
        opcode ^= 0xc0;             /* not.l */
    pokewd(opcode);
    quickwhole(a->y);               /* Fixed (#1209): Not of floor(q) */
    foldnote(FN_NOT, a->y, 0, 0, at);
}

/* blitz2.s:9688 notit - do NOT. (applies to the whole rest of the
   expression: eval4, whose node is the NOT's) */
static UWORD notit(UWORD *type)
{
    struct expract *prev = exprtree_building() ? exprtree_lastact() : NULL;
    UWORD c = eval4(type, EXN_NOT);

    if (rawq) {                     /* Fixed (#1178): Not of floor(q) */
        emitact(cutact, NULL, 0, 0);
        rawq = FALSE;
    }
    /* Not of a constant with a point: the typing pass of rule P+ decides
       (#1216: its whole part) */
    if (p5relaxnot(*type))
        *type = T_LONG;
    if (*type >= 5)
        illoperr();
    emitact(notemit, NULL, 0, *type);
    p5wrap(ETN_NOT, NULL, prev, exprtree_lastact());    /* the typing pass */
    eval4end();
    return c;
}

/* The elements as leaves.  The parse part of an element reads it and makes
   its leaf (exprtree.h: kind LEAF, op its ELK_*), with the type so far it
   was read under, its own type, the type so far after it and its facts;
   its code is an action (the emit part), which writes it from the leaf
   and the emitter's state (regat, lasta6, ...) only, when the expression
   is built.  What each one changes besides the code, and when:
   docs/blitz2/expression-tree.md. */

/* no asm counterpart: a new leaf of the element kind kind, read under the
   type so far tsf */
static struct exprnode *newleaf(unsigned kind, UWORD tsf)
{
    struct exprnode *l = exprtree_new(EXN_LEAF);

    l->op = (UWORD)kind;
    l->tbefore = tsf;
    p5leaf(l, P5L_OTHER);           /* the typing pass (#1216) */
    return l;
}

/* no asm counterpart: the leaf l built (its code recorded); *type is the
   type so far after it */
static void leafdone(struct exprnode *l, UWORD *type)
{
    *type = l->tafter;
    exprtree_release(l);
}

/* no asm counterpart; replaces blitz2.s:9700 fetchpi's code - move.l
   #Pi,D(regat) */
static void piemit(struct expract *a)
{
    struct exprnode *l = a->p;

    l->start = pc;
    pokewd((regat << 9) | 0x203c);  /* move.l #x,dn */
    pokel(0xc90fda42);
    l->end = pc;
}

/* blitz2.s:9700 fetchpi - return Pi.f (a Float whatever the type so
   far: no conversion here) */
static UWORD fetchpi(UWORD *type)
{
    struct exprnode *l = newleaf(ELK_PI, *type);

    l->own = l->tafter = T_FLOAT;
    emitact(piemit, l, 0, 0);
    leafdone(l, type);
    return getchar3();
}

/* --------------------------------------------- forward decls of elements */
static UWORD fetchsize(UWORD *type);
static UWORD fetchon(UWORD *type);
static UWORD fetchoff(UWORD *type);
static UWORD fetchconst(UWORD *type);
static UWORD fetchasm(UWORD *type);
static UWORD fetchqasm(UWORD *type);
static UWORD litstring(UWORD *type);
static UWORD ampersand(UWORD *type);
static UWORD qmark(UWORD *type);

/* blitz2.s:9718 getelement2 - recursive stuff first..... */
static UWORD getelement2(UWORD *type)
{
    return element(getchar3(), type);
}

/* no asm counterpart: AmiBlitz3's report of a token's value (ext_evalvalue,
   a value without a type) */
static void tokvalact(struct expract *a)
{
    (void)a;
    ext_evalvalue(0, 0);
}

/* no asm counterpart: the element that begins with c (element0) and, for
   the typing pass of rule P+ (#1216), its node: a leaf (a value, a
   constant); a bracket, a minus and Not make theirs themselves */
static UWORD element0(UWORD c, UWORD *type);

static UWORD element(UWORD c, UWORD *type)
{
    BOOL sub = c == '(' || c == '{' || c == '-' || c == TOK_ASM_NOT;
    struct expract *prev = NULL;
    UWORD r;

    if (!sub && p5active())
        prev = exprtree_lastact();
    if (!sub)
        p5leaf(NULL, 0);
    r = element0(c, type);
    if (!sub)
        p5elemdone(prev);
    return r;
}

/* blitz2.s:9719 getelement2 after its bsr getchar3 - the element that
   begins with c (negate reads a run of minus signs itself) */
static UWORD element0(UWORD c, UWORD *type)
{
    BOOL neg = negnext;             /* Fixed (#1070) */
    UWORD k;

    negnext = FALSE;
    rawq = FALSE;                   /* Fixed (#1178): not a raw Quick yet */
    cmpres = FALSE;                 /* Fixed (#1182): not lowered yet */
    bconv.seq = 0;                  /* Fixed (#1177): no conversion yet */
    elemcount++;                    /* for the intrinsics */
    if (c & 0x8000)
        stateact(tokvalact, NULL, 0, 0);    /* a token: a value without a type */
    if (c == '(' || c == '{')
        return bracket(type);
    if (c == '-') {
        negnext = !neg;
        c = negate(type);
        bforth[nbforth - 1].wordval = 0;    /* Fixed (#1179): -(-32768) */
        return c;
    }
    if (c == TOK_ASM_NOT) {
        c = notit(type);
        bforth[nbforth - 1].wordval = 0;    /* Fixed (#1179): a result */
        return c;
    }

    if (nbforth >= 32)              /* cmp.l #forthsp,a1 */
        conmemerr();
    k = nbforth++;
    bforth[k].constgot = 0;
    bforth[k].wordval = 0;

    /* First, we have stuff which won't fuck constgot */
    litneg = neg;
    /* Fixed (#1179): an integer literal or #constant 0..32767 is a Word
       value (iswordconst: constword) */
    if (tstnum(c)) {
        c = (UWORD)fetchnum(c, type, FALSE);
        bforth[k].wordval = constword;
        return c;
    }
    if (c == '.')
        return (UWORD)fetchfrac(c, type, FALSE);
    litneg = FALSE;
    if (c == '#' || c == '$' || c == '%') {
        if (c == '#')
            c = fetchconst(type);
        else if (c == '$')
            c = (UWORD)fetchhex(type, FALSE);
        else
            c = (UWORD)fetchbin(type, FALSE);
        bforth[k].wordval = constword;
        return c;
    }
    if (c == TOK_SIZEOF)
        return fetchsize(type);
    if (c == TOK_PI)
        return fetchpi(type);
    if (c == TOK_ON)
        return fetchon(type);
    if (c == TOK_OFF)
        return fetchoff(type);

    if (constmode) {
        if (tstalpha(c) || c == '_')
            return fetchasm(type);
        if (c == '"')
            return fetchqasm(type);
        syntaxerr();
    }
    if (cfetchmode)                 /* .noasm */
        badconerr();                /* we're trying to get a const here! */
    bforth[k].constgot = 0xffff;    /* .asmok: not -(a1) - non-const */

    /* Here, we have stuff which will fuck it! */
    if (tstalpha(c) || c == '*' || c == '\\' || ext_letter(c))
        return variable(c, type);
    if (c == '"')
        return litstring(type);
    if (c & 0x8000)
        return function(c, type);
    if (c == '&')
        return ampersand(type);
    if (c == '?')
        return qmark(type);
    if (ext_element(c, type, &c))
        return c;
    syntaxerr();
}

/* no asm counterpart: the element being built is a constant (fetchnum3:
   ELF_FOLD) */
BOOL elemconst(void)
{
    return nbforth && !bforth[nbforth - 1].constgot;
}

/* no asm counterpart: the constant l, whose code is the last action
   recorded, built: an integer one is the last constant (#978: isbyteconst
   of its code), a literal made a whole number the last such literal
   (#1186: fraclitfloat), each with the code before it, for
   exprtree_left and ownleft */
void constrec(struct exprnode *l)
{
    if (!exprtree_building())
        return;
    bcst.seq = 0;
    if (!(l->lflags & ELF_POINT) && l->tafter != T_FLOAT) {
        bcst.seq = codeseq;
        bcst.before = prevcodeseq;
        bcst.leaf = l;
    }
    if (l->lflags & ELF_CUT) {
        blit.seq = blit.last = codeseq;
        blit.before = prevcodeseq;
        blit.leaf = l;
        blit.neg = NULL;
        blit.negs = 0;
    }
}

/* blitz2.s:9801 fetchsize - SizeOf */
static UWORD fetchsize(UWORD *type)
{
    UWORD c = getchar3();
    UWORD saved, len;
    ULONG value;

    if (c == '.') {
        struct type *t;                             /* .sizeobj */
        saved = *type;
        len = makename(&c);
        if (!len)
            syntaxerr();
        t = findtype((struct node *)&alltypes, len, NULL);
        if (!t)
            notypeerr();
        value = 0;
        c = reget();                                /* d0 after findtype */
        /* Fixed (#974): the size of a type, or the offset of a field, that
           is not final yet is "Type not yet defined"; blitz2.s gave the
           line number held by a forward type (and searched its fields
           at address $ff) or the layout before the forward field */
        if (c == '\\') {
            struct var *v;
            len = makename(&c);
            if (!len)
                syntaxerr();
            if (t->u.code == TYPE_FORWARD)
                notdeferr();
            v = findvar((struct node *)&t->u.fields, len, NULL);
            if (!v)
                noofferr();
            chkfieldoffset(t, v);
            value = (UWORD)v->offset;
        } else {
            chktypesize(t);
            value = (UWORD)t->size;                 /* .jty */
        }
        *type = saved;                              /* .jty2 */
        return (UWORD)fetchnum2(value, type, FALSE);
    }
    /* find size of a var! */
    {
        struct var *v;
        bakup();
        saved = *type;
        len = makename(&c);
        if (!len)
            syntaxerr();
        v = findvar((struct node *)&firstvar, len, NULL);
        if (!v)
            notypeerr();                            /* ! */
        value = (ULONG)(LONG)v->offset;             /* .found: 4(a2), ext.l */
        *type = saved;
        return (UWORD)fetchnum2(value, type, FALSE);
    }
}

/* blitz2.s:9839 fetchon */
static UWORD fetchon(UWORD *type)
{
    getchar3();
    return (UWORD)fetchnum2(0xffffffffUL, type, FALSE);
}

/* blitz2.s:9843 fetchoff */
static UWORD fetchoff(UWORD *type)
{
    getchar3();
    return (UWORD)fetchnum2(0, type, FALSE);
}

/* blitz2.s:9847 chkhex - is d0 a hex number?  asm: Z=1 yes.
   Side effect kept: anything above '9' is upper cased in *c. */
static BOOL chkhex(UWORD *c)
{
    if (*c < '0')
        return FALSE;
    if (*c <= '9')
        return TRUE;
    *c &= 0xffdf;
    if (*c > 'F')
        return FALSE;
    if (*c < 'A')
        return FALSE;
    return TRUE;
}

/* blitz2.s:9861 chkbin - asm: Z=1 if '0' or '1' */
static BOOL chkbin(UWORD c)
{
    return c == '1' || c == '0';
}

/* blitz2.s:9866 fetchhex */
ULONG fetchhex(UWORD *type, BOOL data)
{
    UWORD c = getchar3();
    ULONG value;

    if (!chkhex(&c))
        syntaxerr();
    value = 0;
    do {
        UWORD digit;
        if (value >= 0x10000000UL)
            overerr();
        value <<= 4;
        digit = c - 48;
        if (digit > 9)
            digit -= 7;
        value |= digit;             /* or d0,d3 (word) */
        c = getchar3();
    } while (chkhex(&c));
    return fetchnum2(value, type, data);
}

/* blitz2.s:9883 fetchbin */
ULONG fetchbin(UWORD *type, BOOL data)
{
    UWORD c = getchar3();
    ULONG value;

    if (!chkbin(c))
        syntaxerr();
    value = 0;
    do {
        if (value & 0x80000000UL)   /* lsl.l #1,d3 / bcs overerr */
            overerr();
        value <<= 1;
        value |= (UWORD)(c - 48);
        c = getchar3();
    } while (chkbin(c));
    return fetchnum2(value, type, data);
}

/* #__debug, predefined in both modes (change debug-constant, AmiBlitz3's
   name and values): -1 when the program is compiled with runtime error
   checking (the option, as AmiBlitz3's debugmode at the start of the
   compile; RunErrsOff does not change it), else 0.  Only for a name the
   source has not defined: AmiBlitz3 adds it to its constant list before
   the source, so a source's own #__debug=... replaces it there, as here;
   a resident's #__debug gets the value at the start of the compile
   (fixdebugconst).  An extension of blitz2: the original says "Constant
   Not Found". */
static ULONG debugvalue(void)
{
    return (optreqga10.Flags & GFLG_SELECTED) ? 0xffffffffUL : 0;
}

static BOOL debugconst(UWORD len, ULONG *value)
{
    if (len != 7 || memcmp(namebuff, "__debug", 7))
        return FALSE;
    *value = debugvalue();
    return TRUE;
}

void fixdebugconst(void)
{
    struct label *l;

    if (lastrescon == (struct label *)&firstlabel)
        return;                     /* no resident constants */
    for (l = firstlabel; l; l = l->next) {
        if (l->len == 19 + 7 + 1 && (ULONG)l->refs == 1 && !memcmp(l->name, "__debug", 7))
            l->pc = debugvalue();
        if (l == lastrescon)
            break;
    }
}

/* blitz2.s:9896 fetchconst - #name */
static UWORD fetchconst(UWORD *type)
{
    UWORD saved = *type, c, len;
    struct label *l;
    ULONG value;

    len = makename(&c);
    if (!len)                       /* Fixed (#30): Z not tested in the asm */
        syntaxerr();
    if (!ext_constant(len, &value)) {
        l = findadd(len, NULL);
        if (!l && debugconst(len, &value))
            goto have;
        if (!l)
            cnferr();
        if ((ULONG)l->refs != 1)
            cnferr();
        value = l->pc;
    }
have:
    *type = saved;
    return (UWORD)fetchnum2(value, type, FALSE);  /* to right type. */
}

/* blitz2.s:9906 ampersand */
static UWORD ampersand(UWORD *type)
{
    UWORD c = ampersand2(type);
    *type = T_LONG;
    return c;
}

/* no asm counterpart; replaces blitz2.s:9915-9939 ampersand2 after its
   bsr calcvar - the variable's type reported (ext_evalvalue), then the
   code of its address into D(regat): a string's from its pointer, a
   simple variable's lea x(a5); an address calcvar left in a2 as it is */
static void addrofemit(struct expract *a)
{
    struct exprnode *l = a->p;
    UWORD vartype = l->vtype;

    l->start = pc;
    varmode = (l->lflags & ELF_LOCAL) ? -1 : 0;   /* pokela5s: a4 or a5 */
    ext_evalvalue(l->value, (UWORD)l->aux);         /* the variable's type */
    if ((vartype & 0xff) == T_STRING) {
        if (vartype & 0x8000) {                     /* .algot2 */
            pokewd((regat << 9) | *stamp);          /* move.l (a2),Dn */
            l->end = pc;
            return;
        }
        stamp2[1] = l->off;                         /* move d3,stamp2+2 */
        pokela5s(L_(stamp2));                       /* move.l x(a5),a2 */
    } else if (!(vartype & 0x8000)) {               /* .notst */
        leaamp[1] = l->off;
        pokela5s(L_(leaamp));                       /* lea x(a5),a2 */
    }
    pokewd((regat << 9) | *leaamp2);                /* .algot: move.l a2,Dn */
    l->end = pc;
}

/* no asm counterpart - the variable node l (varparse: calcvar's parse
   part, which emitted the code of its path) made the leaf of kind kind
   read under the type so far tsf: value what ext_evalvalue reports (the
   type's code), aux the flagmask; vtype, off and ELF_LOCAL stay */
static void varleaf(struct exprnode *l, unsigned kind, UWORD tsf)
{
    const struct type *t = (const struct type *)l->aux;

    l->kind = EXN_LEAF;
    l->op = (UWORD)kind;
    l->tbefore = tsf;
    l->value = t ? t->u.code : 0;
    l->aux = flagmask;
    p5leaf(l, P5L_OTHER);           /* the typing pass (#1216) */
}

/* blitz2.s:9910 ampersand2.  The parse part finds the variable (fetchvar
   makes it at first sight) and reads its path (varparse: an array's
   indexes, a pointer's or a path's address code recorded as they are
   read); its variable node becomes the leaf, with calcvar's type, the
   offset and what ext_evalvalue reports.  *type is calcvar's type (with
   its flags). */
UWORD ampersand2(UWORD *type)
{
    UWORD namelen;          /* asm d2 after getvname */
    UWORD c;
    struct var *v;
    struct type *t;
    struct exprnode *l;

    c = getchar3();
    c = getvname(c, &namelen);
    v = fetchvar(namelen, &t, &c);
    l = varparse(v, t, &c);
    varleaf(l, ELK_ADDROF, *type);
    l->own = (UBYTE)l->vtype;
    l->tafter = l->vtype;
    emitact(addrofemit, l, 0, 0);
    leafdone(l, type);
    return c;
}

/* blitz2.s:9941 doern - read err number.
   NB(orig): no getchar; the current char stays the token. */
/* no asm counterpart; replaces blitz2.s:9941 doern's tokejsr ern - the
   code of ErrNum's leaf (a->p): the call */
static void errnumemit(struct expract *a)
{
    struct exprnode *l = a->p;

    l->start = pc;
    tokejsr(ern);
    l->end = pc;
}

static UWORD doern(UWORD c, UWORD *type)
{
    struct exprnode *l = newleaf(ELK_ERRNUM, *type);

    l->own = l->tafter = T_LONG;
    tokuse(ern);                    /* the library used here */
    emitact(errnumemit, l, 0, 0);
    leafdone(l, type);
    return c;
}

/* blitz2.s:9952 fetchit - token of an object lib; returns the lib
   (asm a2, d0 = offset of its max info) */
static struct blitzlib *fetchit(void)
{
    UWORD c = getchar3();
    struct blitzlib *bl;

    if (!(c & 0x8000))
        syntaxerr();
    findtokeact(c & 0x7fff, &bl);
    if (!LIBMAX_SHIFT(bl))          /* tst -2(a2,d0.l) */
        freeerr();
    return bl;
}

/* blitz2.s:10169/10585 .loopsw: bit-reverse a movem mask (lsl #1,d3 / roxr #1,d1 x16) */
static UWORD revmask(UWORD mask)
{
    UWORD rev = 0;
    int i;

    for (i = 0; i < 16; i++) {
        rev = (rev >> 1) | (mask & 0x8000);
        mask <<= 1;
    }
    return rev;
}

static void usedemit(struct expract *a);

/* blitz2.s:9989 doused - return 'used' object.  The parse part is fetchit
   (the object's library: its leaf's value), the code usedemit's. */
static UWORD doused(UWORD *type)
{
    struct exprnode *l;
    struct blitzlib *bl = fetchit();

    l = newleaf(ELK_USED, *type);
    l->value = (ULONG)bl;
    l->own = l->tafter = T_WORD;    /* .skip4: moveq #2,d2 */
    emitact(usedemit, l, 0, 0);
    leafdone(l, type);
    return getchar3();
}

/* no asm counterpart; replaces blitz2.s:9993-10037 doused after its bsr
   fetchit - the code of Used: usedcode (or usedcode2) patched for the
   library's shift and its used pointer, with d0 saved around it when
   regat > 0.
   Fixed (#998): the number is (used pointer - table base) >> shift; the
   asm shifts with lsr.w (also both shifts of usedcode2), so from
   n << shift = $10000 on the high word of the difference was lost (Used
   BitMap after Use BitMap 1025 gave 1).  Where objlong (tokens1.c) says
   the object offset is a long, the shifts are lsr.l; with smaller maxima
   (all defaults) the code is unchanged. */
static void usedemit(struct expract *a)
{
    struct exprnode *l = a->p;
    struct blitzlib *bl = (struct blitzlib *)l->value;
    UWORD shift = LIBMAX_SHIFT(bl);
    UWORD *start, *end, maxoff;
    UWORD lsr = objlong(LIBMAX_E(bl)) ? 0xE088 : 0xE048; /* lsr.l / lsr.w #8,d0 */

    if (shift < 9) {
        /* shift 1-8 */
        start = usedcode;
        end = usedcodef;
        *ucodediv = lsr | (UWORD)((shift & 7) << 9);
    } else {
        /* .skip: shift 9+: lsr #8 and lsr #shift-8 (a count of 8 is
           encoded as 0; objlong is true from shift 16 on, so the word
           form has counts up to 7).  A shift above 16 would need a third
           shift; the libraries of this repository have shifts up to 7. */
        start = usedcode2;
        end = usedcodef2;
        ucodediv2[-1] = lsr;
        *ucodediv2 = lsr | (UWORD)(((shift - 8) & 7) << 9);
    }
    maxoff = LIBMAX_VAROFF(bl);     /* .skip2 */
    start[(ucodeskip - usedcode) + 1] = maxoff;
    start[1] = maxoff + 4;
    l->start = pc;
    if (regat)
        pokewd(*usedprep);
    pokecode(start, end);           /* .skip3 */
    if (regat) {
        pokewd((regat << 9) | *usedfix);
        pokewd(*useddone);
    }
    l->end = pc;
}

/* blitz2.s:10042 domaximum - number of maximums, eg a=Maximum Shape */
static UWORD domaximum(UWORD *type)
{
    struct blitzlib *bl = fetchit();
    ULONG count = (UWORD)LIBMAX_COUNT(bl);

    getchar3();
    return (UWORD)fetchnum2(count, type, FALSE);
}

/* no asm counterpart; replaces blitz2.s:10069-10083 doaddr's code -
   addrcode patched for the object table's offset and regat: the table
   plus the object's offset (in D(regat), from the index) into D(regat).
   Fixed (#939): the offset is added as a long where objlong (tokens1.c)
   says so; Addr Shape(1025) was 32 KB before the shape table. */
static void addremit(struct expract *a)
{
    struct exprnode *l = a->p;
    UBYTE *e = (UBYTE *)l->value;

    l->start = pc;
    addrcode[1] = *(UWORD *)(e - 6);                    /* x(a5) */
    addrcode[2] = (objlong(e) ? 0xD1C0 : 0xD0C0) | regat; /* add(.l) Dregat,a0 */
    addrcode[3] = (addrcode[3] & 0xf1ff) | (regat << 9);/* move.l a0,Dregat */
    pokecode(addrcode, addrcodef);
    l->end = pc;
}

/* blitz2.s:10053 doaddr - address of something, eg a.l=Addr Window(0).
   The parse part reads the object's token and its index, whose code
   getmaxel emits (the index times the object size, before the element's
   own code: its child); the leaf holds the object's maximum entry. */
static UWORD doaddr(UWORD *type)
{
    struct exprnode *l;
    UWORD tok, c;
    UBYTE *e;

    tok = getchar3();               /* get the token */
    c = getchar3();                 /* and the bracket? */
    if (c != '(')
        syntaxerr();
    e = getmaxel(tok);
    c = reget();
    if (c != ')')
        syntaxerr();
    /* OK, now a2=lib it's from */
    l = newleaf(ELK_ADDR, *type);
    l->value = (ULONG)e;
    l->own = l->tafter = T_LONG;    /* now a long */
    emitact(addremit, l, 0, 0);
    leafdone(l, type);
    return getchar3();
}

/* blitz2.s:10260 tstbras - chk that d1 (first bra char) and d0 (last) are
   consistent with d3 (number of parameters); also, skip last if nec. */
static UWORD tstbras(UWORD first, UWORD c, UWORD n)
{
    if (n) {
        if (first != '(')
            syntaxerr();
        if (c != ')')
            syntaxerr();
        return getchar3();
    }
    if (first == '(') {             /* .nopars */
        c = getchar3();
        if (c != ')')
            syntaxerr();
        return getchar3();
    }
    return c;                       /* .skip */
}

/* Fixed (intended deviation, #344): an amigalib in an expression saves
   the parameter registers that hold a value of the expression.  The
   original saves only d0/d1 (savereg) before the parameters and adds a
   data register below regat to the masks only while it pops the
   parameters, after the movem was poked: a parameter in d2-d5 overwrote
   an intermediate value (a+(b+(c+Write_(...))): c in d2 is buf), and a
   parameter in a3 the end of the string being built (sbasegot, as
   afunction pushes a3 for it).  amigasave adds those registers to the
   movem masks before they are poked; a call whose parameters are all
   above regat (and not a3 in a string expression) is compiled as
   before.  The OS itself keeps d2-d7/a2-a6. */
static void amigasave(const UBYTE *regs, ULONG *push, ULONG *pull)
{
    for (; (BYTE)*regs >= 0; regs++) {
        UWORD reg = *regs;
        if (reg & 0x10) {
            if (reg == 0x13 && sbasegot) {  /* a3 */
                *push |= 1UL << 4;
                *pull |= 1UL << 11;
            }
        } else if (reg < regat) {
            *push |= 1UL << (15 - reg);
            *pull |= 1UL << reg;
        }
    }
}

/* The calls as nodes.
   A procedure call (afunction) or an OS call (amigalib) in an expression
   is a node (exprtree.h: kind CALL, op CLK_PROC or CLK_OS), made when the
   call has been looked up.  It lives while its parameters are read: each
   one is an expression of its own (its node EXPR), its code into its
   register or its push recorded as it is read, with the push protocol of
   the original (fetchprocps, peval): the node's children.  Its code has
   two parts: the enter action before the parameters (the registers saved,
   the string base) and the exit action after them (the call, the result
   into D(regat), the registers restored), each where the original wrote
   it; a leaf of the call's value follows the node
   (callcont: varcont's facts and code).  What each one changes besides
   the code, and when: docs/blitz2/expression-tree.md. */

static void contleaf(struct exprnode *l);
static void contrecord(struct exprnode *l, exprtree_actfn *fn);
static void contemit(struct expract *a);

/* no asm counterpart: a new call node of kind kind, read under the type
   so far tsf */
static struct exprnode *callnode(unsigned kind, UWORD tsf)
{
    struct exprnode *n = exprtree_new(EXN_CALL);

    n->op = (UWORD)kind;
    n->tbefore = tsf;
    return n;
}

/* no asm counterpart; replaces blitz2.s:10479 varcont after a call - the
   leaf of the value (of type n->own) of the call n (its exit action
   recorded) under the type so far n->tbefore: its facts and its code (as
   varcont) */
static UWORD callcont(struct exprnode *n, UWORD *type)
{
    struct exprnode *l = newleaf(0, n->tbefore);

    l->own = n->own;
    contleaf(l);
    contrecord(l, contemit);
    leafdone(l, type);
    return reget();                 /* .done */
}

/* no asm counterpart; replaces blitz2.s:10185-10188 amigalib's bsr
   savereg and bsr pokemovem - the enter action of the OS call n: d0/d1
   and the parameter registers that hold a value of the expression saved
   (#344), a4/a5 (#958) */
static void osenter(struct expract *a)
{
    struct exprnode *n = a->p;
    const UBYTE *regs = (const UBYTE *)n->value + 10;
    ULONG push, pull;

    savereg(&push, &pull);
    amigasave(regs, &push, &pull);  /* fixed (#344) */
    pokemovem(push);
    if (amigaa4a5(regs))            /* fixed (#958): movem.l a4/a5,-(a7) */
        n->lflags |= CLF_A45;
    n->aux = pull;
}

/* no asm counterpart; replaces blitz2.s:10214-10255 amigalib's .toend up
   to .nofin's bsr pokemovem - the exit action of the OS call n: the
   parameters popped into their registers, the library base into a6, the
   jsr, the result into D(regat), the registers restored */
static void osexit(struct expract *a)
{
    struct exprnode *n = a->p;
    const UBYTE *sub = (const UBYTE *)n->value, *p;
    BOOL a45 = (n->lflags & CLF_A45) != 0;
    WORD i;

    libjsr[1] = W_(sub + 8);        /* the offset for the lib */
    libbase[1] = (UWORD)n->off;     /* the libbase reg */
    if (a45)
        pokel(L_(libbase));         /* fixed (#958): a6 before a5 is popped */
    p = sub + 10;
    i = (WORD)n->reg - 1;
    if (i >= 0) {
        while ((BYTE)*p++ >= 0)     /* .toend: go to end of params */
            ;
        p--;
        do {                        /* .loop2 */
            UWORD reg = *--p, opcode;
            if (reg & 0x10)
                opcode = ((reg & 7) << 9) | 0x205f; /* .addreg: move.l (a7)+,An */
            else
                opcode = (reg << 9) | 0x201f;       /* .nomovem: move.l (a7)+,Dn;
                                                       fixed (#344): saved by
                                                       amigasave */
            pokewd(opcode);                         /* .gotit */
        } while (--i >= 0);
    }
    if (!a45)                       /* .nopars */
        pokel(L_(libbase));
    /* Fixed (#1211): a6 now holds the OS library's base, so lasta6 is
       unknown: an FFP call or a library function with a base in a6 after
       this one in the same expression loads its base again (the original
       left lasta6 as it was: Abs(-2)+FindTask_(0) jumped through exec's
       base; test compiler-amigalib-lasta6). */
    lasta6 = -1;
    pokel(L_(libjsr));
    if (a45)
        pokel(0x4cdf3000UL);        /* fixed (#958): movem.l (a7)+,a4/a5 */
    if (regat)
        pokewd((regat << 9) | 0x2000);
    pokemovem(n->aux);              /* .nofin */
}

/* no asm counterpart: the stack use adjusted by a->x (signed) */
static void stackact(struct expract *a)
{
    stackuse += (WORD)a->x;
}

/* blitz2.s:10183 amigalib - Amiga OS library function call.  The parse
   part: the OS library looked up (uselib: used from here on), the
   parameters (each pushed by peval as it is read), the brackets; the
   code: the enter action of the call node n (function: its token's
   library used; osenter) before the parameters, its exit action
   (osexit) after them.
   Fixed (intended deviation, #958): parameters in a4/a5 no longer
   clobber the variable bases, as in stamigalib (see amigaa4a5).
   Fixed (#344): parameter registers holding intermediates are saved,
   see amigasave. */
static UWORD amigalib(struct exprnode *n, UWORD *type)
{
    const UBYTE *sub = (const UBYTE *)n->value;
    struct blitzlib *ol;
    UWORD first, c, k;
    const UBYTE *p;

    ol = uselibact(W_(sub + 6));    /* get lib for base address */
    n->off = (WORD)ol->varoff;      /* libbase reg */
    n->own = T_LONG;
    emitact(osenter, n, 0, 0);
    c = first = getchar3();         /* remember first bracket.... */
    /* collect longs for lib */
    k = 0;
    p = sub + 10;
    for (;;) {
        UWORD t;
        if ((BYTE)*p < 0)
            break;                  /* .done */
        if (*p++ == 0x13)
            bstrwork = TRUE;        /* a parameter in a3 */
        t = T_LONG;
        k++;
        c = peval(&t);
        stateact(stackact, NULL, (ULONG)-4, 0); /* readjust stack! it's coming off later! */
        if (c == ',')
            continue;
        if ((BYTE)*p >= 0)
            syntaxerr();
        break;
    }
    c = tstbras(first, c, k);       /* .done */
    n->reg = (UBYTE)k;
    emitact(osexit, n, 0, 0);
    reget();
    (void)c;
    return callcont(n, type);
}


/* Fixed (#46): does a form of the library function sub load a3 (a
   register spec $13xx in its !libs list, toreg in libcall.c: an object
   like the Shape of BlitColl or the Sound of PeekSound)?  The forms as
   dolibtoke walks them: the parameter word and bytes, the !libs pairs
   (0 terminated), the three routine longs. */
static BOOL liba3(const UBYTE *sub)
{
    const UBYTE *form = sub + 6;

    for (;;) {
        WORD w = (WORD)W_(form);
        const UBYTE *p;

        if (w < 0)
            return FALSE;
        p = aligna1(form + 2 + ((UWORD)w & 255));
        for (;;) {
            UWORD libnum = W_(p), regspec;

            p += 2;
            if (!libnum)
                break;
            regspec = W_(p);
            p += 2;
            if (regspec < 0xff00)
                regspec &= 0x3fff;  /* the Blitz/Amiga mode bits */
            if ((regspec >> 8) == 0x13)
                return TRUE;
        }
        form = p + 12;
    }
}

/* ================================================================ intrinsics
   No asm counterpart (change compiler-intrinsics; the requirements:
   openspec/specs/compiler/intrinsics, the list: docs/blitz2/extensions.md).
   A library function whose argument is a Quick, or a plain string
   variable, is compiled as inline code instead of the call when its
   entry in intrinsics[] says so; the value, the registers the compiler
   relies on and the error behaviour equal the call, only the executable
   differs.  An entry is keyed by its library token (library number << 7
   + the function's number + 1, the identity findtoke resolves), and used
   only if the library's form is the one the entry was written for (one
   parameter of unknown type: the compiler pushes the type word, the
   routine reads it at -2(a2); or one string parameter), so a library of
   another form (the original vallib's Frac takes a Float) is called as
   before.  A string entry does not take the way below: libfunction asks
   intrstrvar before libcallnode, which compiles the argument and the call
   as the original does unless it is a plain string variable.

   The way through the shared code: libfunction arms the entry (its
   call node holds it) before libcallnode; libparams compiles the
   argument as for the call, getap asks intrinsic_argpush for the push flag of the
   argument (2 instead of evalu2's 1), and maineval2, when the argument's
   type is known, asks intrinsic_quick: for a Quick the entry accepts it
   pushes no type word, so stackuse stays 0 and libjsrcode sets up no a2;
   libjsrcode then asks intrinsic_code instead of making the JSR (userjsr,
   with the error check routine of the runtime error checking mode), and
   the entry's code works on d0, the argument (libfunction's regat = 0).
   Everything before (libenter's movem of the registers below regat)
   and after (libresult's move.l d0,D(regat), the movem back, callcont) is the
   call's.  For every other type the argument and the call are compiled
   exactly as the original does.  Not in constant evaluation (cfetchmode: the code
   runs in the compiler, with the library's constant rules) and not in
   direct mode (dirmode). */
struct intrinsic {
    UWORD token;                    /* the library token, 0 = end */
    UWORD argtype;                  /* the parameter of the library's first
                                       form: 8 (BL_UNKNOWN, the type word
                                       pushed: the argument a Quick) or
                                       T_STRING (the argument a plain
                                       string variable, intrstrvar) */
    UWORD restype;                  /* the type of the inline result */
    BOOL (*here)(UWORD oldtype);    /* used here? (before the argument;
                                       NULL = always) */
    BOOL (*accept)(void);           /* the argument is a Quick: inline?
                                       (after the argument, srcptr behind
                                       its closing parenthesis; NULL =
                                       always) */
    void (*emit)(void);             /* the code: d0 the Quick argument (a
                                       string: the long of the variable,
                                       its address of the characters or 0,
                                       the flags set by its move), the
                                       result in d0; changes nothing else
                                       but the flags (a string: and a0) */
};

/* Int(Quick) (vallib doint): the integer part truncated towards zero, as
   the library's Quick case (Int(-1.75) = -1), as a Long.  A negative value
   gets $FFFF added before the shift, which rounds the integer part up
   (towards zero) unless the fraction is 0: q + 65535 has no overflow for
   q < 0, and its upper word is then the integer part of q truncated
   (-32768.0 = $80000000 gives $8000FFFF, -32768).  The library's intchk
   (runtime error checking) only refuses a string: no check for a Quick. */
static void intquick(void)
{
    pokewd(0x4a80);                 /* tst.l d0 */
    pokewd(0x6a06);                 /* bpl.s 1f */
    pokewd(0x0680);                 /* addi.l #$FFFF,d0 */
    pokel(0x0000ffffUL);
    pokewd(0x4840);                 /* 1: swap d0 */
    pokewd(0x48c0);                 /* ext.l d0 */
}

/* Frac(Quick) as a Quick (vallib doffrac): only where the Float result
   would be converted to a Quick at once and unchanged - the call is the
   whole right side of an assignment to a Quick (dolet2: the first element
   of the right side's evaluation, asked for a Quick; see intrinsic_let)
   and the statement ends behind its closing parenthesis (fracend).  The
   library's Float is the exact fraction (#1064), which the assignment's
   fltoqu converts back exactly, so the Quick here is the same: the low
   word of the magnitude with the sign of the argument (0 when it is 0:
   Frac(-3.0) = 0, Frac(-1.25) = -0.25 = $FFFFC000).  In every other
   context Frac stays the library's Float. */
static BOOL frachere(UWORD oldtype)
{
    return oldtype == T_QUICK && evaldepth == letdepth && elemcount == letelem;
}

/* the statement ends behind the closing parenthesis: a 0 (end of line),
   ':' or a comment after spaces.  Read from srcptr without getchar_, so
   that nothing of the source (a CNIF, a ~constant~, a UsePath end) is
   acted on; another byte (also 1 and 2, those markers) does not end it */
static BOOL fracend(void)
{
    const UBYTE *p = srcptr;

    while (*p == ' ')
        p++;
    return *p == 0 || *p == ':' || *p == ';';
}

static void fracquick(void)
{
    pokewd(0x4a80);                 /* tst.l d0 */
    pokewd(0x6a0c);                 /* bpl.s 1f */
    pokewd(0x4480);                 /* neg.l d0 (the magnitude) */
    pokewd(0x4840);                 /* swap d0 */
    pokewd(0x4240);                 /* clr.w d0 */
    pokewd(0x4840);                 /* swap d0 (its fraction word) */
    pokewd(0x4480);                 /* neg.l d0 */
    pokewd(0x6006);                 /* bra.s 2f */
    pokewd(0x4840);                 /* 1: swap d0 */
    pokewd(0x4240);                 /* clr.w d0 */
    pokewd(0x4840);                 /* swap d0 */
}                                   /* 2: */

/* Len(a$) of a plain string variable (stringfunclib lenstring): the
   length from the header in front of the characters (-4, struct
   bb_strmem, libs/include/blitzobj.h), 0 for a variable without memory
   (its long is 0) - the length the call gets, which the compiler pushes
   from -4 too (stvarget2) before it copies the characters.  No string
   is copied: the work area, a3 and sbasegot stay as they were.  The
   original's library call sets a3 to the start of its copy: inside a
   string expression (sbasegot set) that is where a3 was; as the
   statement's first string use the original first loads the string base
   (makesbase, blitz2.s:10305), so a3 is the string base afterwards,
   where the inline code leaves a3 as the previous statement left it.
   No BASIC program sees that, since every statement starts its string
   use at the string base (setsvars, blitz2.s:2927); only inline
   assembler reading a3 between statements does.  a0 changes, as with
   the copy (copstring). */
static void lenvar(void)
{
    pokewd(0x6706);                 /* beq.s 1f (no memory: 0) */
    pokewd(0x2040);                 /* move.l d0,a0 */
    pokewd(0x2028);                 /* move.l -4(a0),d0 */
    pokewd(0xfffc);
}                                   /* 1: */

/* Asc(a$) of a plain string variable (stringfunclib ascii): the first
   character zero-extended, -1 for the length 0 and for a variable without
   memory - the library's long; a3 (untouched, unlike the original's
   call, see lenvar) and a0 as Len's */
static void ascvar(void)
{
    pokewd(0x670e);                 /* beq.s 2f (no memory) */
    pokewd(0x2040);                 /* move.l d0,a0 */
    pokewd(0x4aa8);                 /* tst.l -4(a0) */
    pokewd(0xfffc);
    pokewd(0x6706);                 /* beq.s 2f (the length 0) */
    pokewd(0x7000);                 /* moveq #0,d0 */
    pokewd(0x1010);                 /* move.b (a0),d0 */
    pokewd(0x6002);                 /* bra.s 3f */
    pokewd(0x70ff);                 /* 2: moveq #-1,d0 */
}                                   /* 3: */

/* the library tokens: vallib (libs/variants/bb2-2.10/libnums.h LIB_vallib
   120; the same in AB3 mode's list), stringfunclib (LIB_stringfunclib
   185; in AB3 mode Len and Asc are in AmiBlitz3's library 6, which has no
   entry: they stay calls there) */
static const struct intrinsic intrinsics[] = {
    { (120 << 7) + 2, 8, T_LONG, NULL, NULL, intquick },       /* Int */
    { (120 << 7) + 4, 8, T_QUICK, frachere, fracend, fracquick }, /* Frac */
    { (185 << 7) + 15, T_STRING, T_LONG, NULL, NULL, lenvar },  /* Len */
    { (185 << 7) + 7, T_STRING, T_WORD, NULL, NULL, ascvar },   /* Asc */
    { 0, 0, 0, NULL, NULL, NULL }
};

/* the entry of intrinsics[] for the library function token tok (its sub
   from looktoke, called with np parameters), if it may be used here */
static const struct intrinsic *intrfind(UWORD tok, const UBYTE *sub, UWORD np,
                                        UWORD oldtype)
{
    const struct intrinsic *in;
    const UBYTE *form = sub + 6;

    if (cfetchmode || dirmode || np != 1)
        return NULL;
    /* the first form: no repeats, one parameter, of the entry's type (8,
       blitzlib.h BL_UNKNOWN: handlep takes it to getap's evalu2; or a
       string) */
    if (W_(form) != 1)
        return NULL;
    for (in = intrinsics; in->token; in++)
        if (in->token == tok)
            return form[2] == in->argtype && (!in->here || in->here(oldtype))
                   ? in : NULL;
    return NULL;
}

/* the argument of a string intrinsic (Len, Asc): a plain string variable,
   a name with '$' and nothing behind it but the closing parenthesis
   (spaces aside).  Read from srcptr, behind the '(' (countpees), without
   getchar_ as fracend.  If it is one, the variable is fetched as variable
   does (getvname, fetchvar, calcvar: a name with '$' and no '(' is never
   an array or a pointer, so calcvar emits nothing) and the entry's code
   is emitted on its long; TRUE, *c the ')' (as dolibtoke's reget).
   Else FALSE: nothing is read, the call is compiled as the original does. */
/* no asm counterpart: the code of a string intrinsic on a plain string
   variable (the intrinsic a->p, the variable's offset a->x): its long
   into d0, the entry's code */
static void strvaremit(struct expract *a)
{
    pokewda5s(0x202d);              /* move.l off(a5),d0 (a local: a4) */
    pokewd((UWORD)a->x);
    ((const struct intrinsic *)a->p)->emit();
}

static BOOL intrstrvar(const struct intrinsic *in, UWORD *c)
{
    const UBYTE *p = srcptr;
    UWORD ch, namelen, vartype;
    struct var *v;
    struct type *t;
    WORD off = 0;

    while (*p == ' ')
        p++;
    if (*p != '_' && !tstalpha(*p))
        return FALSE;
    while (*p == '_' || tstalpha(*p) || tstnum2(*p))
        p++;
    if (*p++ != '$')
        return FALSE;
    while (*p == ' ')
        p++;
    if (*p != ')')
        return FALSE;
    ch = getvname(getchar3(), &namelen);
    v = fetchvar(namelen, &t, &ch);
    if ((v->flags & 3) || t->u.code != T_STRING)
        syntaxerr();                /* never: fvarback matches a name with
                                       '$' (flagmask 8) only with a plain
                                       string variable */
    vartype = calcvar(&v, &t, &ch, &off);
    if (vartype != T_STRING || ch != ')')
        syntaxerr();
    emitact(strvaremit, (void *)in, (UWORD)off, 0);
    *c = reget();
    return TRUE;
}

/* see eval.h */
void intrinsic_let(void)
{
    letdepth = evaldepth + 1;
    letelem = elemcount + 1;
}

/* see eval.h */
WORD intrinsic_argpush(void)
{
    if (intrstate != INTR_ARMED)
        return 1;
    intrstate = INTR_ARG;
    return 2;
}

/* maineval2, the argument of the armed intrinsic compiled (type got):
   compile it inline (TRUE: push no type word)? */
static BOOL intrinsic_quick(UWORD type)
{
    if (intrstate != INTR_ARG || type != T_QUICK
            || (intrcur->accept && !intrcur->accept()))
        return FALSE;
    intrstate = INTR_QUICK;
    return TRUE;
}

/* see eval.h */
BOOL intrinsic_code(void)
{
    if (intrstate != INTR_QUICK)
        return FALSE;
    intrcur->emit();
    intrstate = INTR_DONE;
    return TRUE;
}

/* The library calls as nodes.  A library function in an expression is a
   call node as a procedure call (exprtree.h: kind CALL, op CLK_LIB), made
   when its token has been looked up (looktoke, no side effect).  Its
   library is used right after the lookup (calluse, before anything of the
   call is read: the moment findtoke used it, so a library first used in
   another one's parameters comes after it in the program); then its enter
   action: the string base and the registers below regat saved
   (libenter), the brackets counted, regat 0 for the parameters (libregs).
   Its form (libform) and its parameters (libparams: each one an
   expression of its own, compiled into its register or pushed, the
   node's children) follow, then its exit action: the call (libexit), the
   ')', the result into D(regat) and the registers restored (libresult).
   An OS call's node is made by the same lookup (CLK_OS, amigalib).  What
   each one changes besides the code, and when:
   docs/blitz2/expression-tree.md. */

/* no asm counterpart; replaces blitz2.s:6300-6305 findtoke's .found for
   a library function or an OS call in an expression - right after the
   lookup of the call n its token's library (n->aux) used (uselib3:
   copied into the program at its first use, its variable offset), its
   relocations taken where the call's code begins */
static void calluse(struct exprnode *n)
{
    struct blitzlib *bl = (struct blitzlib *)n->aux;

    uselib3(bl, bl->libnum);        /* set lib# */
    stateact(useact, bl, bl->libnum, 0);
}

/* no asm counterpart: makesbase's library (workstart, at the first string
   of a statement) used while the expression is built; its code follows */
static void sbaseuse(void)
{
    bstrwork = TRUE;
    if (!bsbase)
        tokuse(workstart);
    bsbase = TRUE;
}

/* no asm counterpart; replaces blitz2.s:10121-10128 libfunction's .no0
   and .notst0 - the enter action of the library call n after its use (its
   result type a->y): the string base for a string result, the registers
   below regat saved (fixed #46: and a3, see libfunction) */
static void libenter(struct expract *a)
{
    struct exprnode *n = a->p;
    UWORD mask, c;

    if (a->y == T_STRING)           /* .no0 */
        makesbase();
    if (sbasegot && a->y != T_STRING && liba3((const UBYTE *)n->value)) {
        /* fixed (#46): the movem regs and a3 */
        mask = 0;
        for (c = regat; c; c--)
            mask = (mask >> 1) | 0x8000;
        mask |= 1 << 4;             /* a3, predecrement order */
        pokewd(0x48e7);
        pokewd(mask);
        n->lflags |= CLF_A3;
    } else
        mask = savem();             /* .notst0: the movem regs */
    n->vtype = mask;
}

/* no asm counterpart; replaces blitz2.s:3229-3254 dolibtoke's .skip up
   to .skip4 for a library function in an expression - the exit action of
   the library call n: the call of its form (libjsrcode: a2 above the
   parameters pushed, the repeats a->x, the jsr or, a->y, an intrinsic's
   code, the stack back) */
static void libexit(struct expract *a)
{
    struct exprnode *n = a->p;
    WORD s_numreps = numreps;
    UWORD s_state = intrstate;
    const struct intrinsic *s_cur = intrcur;

    numreps = (WORD)a->x;
    intrcur = n->prec ? &intrinsics[n->prec - 1] : NULL;
    intrstate = a->y ? INTR_QUICK : INTR_IDLE;
    libjsrcode((struct blitzlib *)n->aux, (UBYTE *)n->value + n->off);
    intrcur = s_cur;
    intrstate = s_state;
    numreps = s_numreps;
}

/* no asm counterpart; replaces blitz2.s:10155-10180 libfunction's .skip
   up to .notst - the rest of the exit action of the library call n
   (its result type a->y), after its ')': the result into D(regat), the
   registers restored, a string's length */
static void libresult(struct expract *a)
{
    struct exprnode *n = a->p;

    if (regat)
        pokewd((regat << 9) | 0x2000);  /* move.l d0,regat */
    if (n->vtype) {                 /* .skip2 */
        pokewd(0x4cdf);
        pokewd(revmask(n->vtype));  /* poke movem (a7)+... */
    }
    if (a->y == T_STRING)           /* .nomovem */
        pokewd(*putstlen | regat);
}

/* no asm counterpart: the stack use kept in the action (dolibtoke's save
   around a call) and set back from it (a->p: the saving action) */
static void stacksave(struct expract *a)
{
    a->x = (UWORD)stackuse;
}

static void stackrestore(struct expract *a)
{
    stackuse = (WORD)((const struct expract *)a->p)->x;
}

/* no asm counterpart; replaces blitz2.s:3181-3258 dolibtoke for a library
   function in an expression - the form of the library call n with np
   parameters (libform, its offset in the node), the parameters
   (libparams), what its call uses (userjsruse; an intrinsic the argument
   made inline: none) and the node's exit action (libexit), between the
   save of the globals a nested call changes and their restore, as
   dolibtoke */
static UWORD libcallnode(struct exprnode *n, UWORD np)
{
    WORD s_numreps = numreps;       /* recurs. */
    WORD s_userp = userp;
    struct expract *save;
    UBYTE *sub = (UBYTE *)n->value, *form;
    BOOL inl;

    userp = usertype;
    form = libform(np, sub);
    n->off = (WORD)(form - sub);
    save = stateact(stacksave, NULL, 0, 0);
    libparams(np, form);
    inl = intrstate == INTR_QUICK;  /* intrinsic_code's state */
    if (inl)
        intrstate = INTR_DONE;
    else
        userjsruse((struct blitzlib *)n->aux, form);
    emitact(libexit, n, (UWORD)numreps, (UWORD)inl);
    stateact(stackrestore, save, 0, 0);     /* .skip3 */
    userp = s_userp;
    numreps = s_numreps;
    return reget();
}

/* blitz2.s:10112 libfunction - do a user library function, eg
   a=pixel{x,y}.  The parse part of the library call n (made and its
   library used by function): the result type (a size specifier), the
   brackets and the parameters counted, the form and the parameters
   (libcallnode), the ')'; the code: the node's enter action (libenter
   before the brackets; libregs's moveq after them: regat kept and 0 for
   the parameters, the type of a size specifier into d0) before the
   parameters, its exit action after them (libexit, the call; libresult
   after the ')'); then the leaf of the call's value (callcont).
   Fixed (intended deviation, #46): a function whose !libs list loads a3
   (an object, $13xx) inside a string expression saves a3, the end of
   the string being built (sbasegot, the criterion afunction uses for
   pushing a3 and amigasave for #344), with the registers below regat in
   the movem before the parameters, and gets it back with them after the
   call.  The original saves only the registers below regat: the rest of
   the expression wrote its characters from the object on (BlitColl's
   Shape, PeekSound's Sound).  A function returning a string leaves its
   result at a3 and is compiled as the original does, as is every call outside a
   string expression.
   tok: the library token (function), for the intrinsics: an entry of
   intrinsics[] is armed for the argument (no asm counterpart); the node
   holds it (prec) and whether the call was compiled inline
   (CLF_INLINE, its type then the entry's). */
static UWORD libfunction(struct exprnode *n, UWORD tok, UWORD *type)
{
    UBYTE *sub = (UBYTE *)n->value;
    UWORD c, np, sstate;
    const struct intrinsic *in, *scur;

    n->own = (UBYTE)(W_(sub) >> 8);
    if (B_(sub + 1) & 8)
        n->own = (UBYTE)sizespec2();
    if (n->own == T_STRING)
        sbaseuse();
    if (liba3(sub))
        bstrwork = TRUE;            /* a3 loaded */
    emitact(libenter, n, 0, n->own);
    c = getchar3();
    np = 0;
    if (c == '(') {
        np = countpees(c);
        if (!np)
            syntaxerr();
    } else
        bakup();                    /* .nopars */
    n->reg = (UBYTE)regat;          /* .skip0 */
    regat = 0;
    if (B_(sub + 1) & 8) {
        emitact(pokeact, NULL, n->own | 0x7000, 0); /* moveq #type,d0 */
        regat++;
    }
    in = intrfind(tok, sub, np, n->tbefore);
    n->prec = in ? (UWORD)(in - intrinsics + 1) : 0;
    scur = intrcur;                 /* an intrinsic around this call */
    sstate = intrstate;
    intrcur = in;
    intrstate = in ? INTR_ARMED : INTR_IDLE;
    if (in && in->argtype == T_STRING) {
        intrstate = INTR_IDLE;      /* nothing for getap to arm */
        if (intrstrvar(in, &c))
            intrstate = INTR_DONE;
        else
            c = libcallnode(n, np);
    } else
        c = libcallnode(n, np);     /* .no2 */
    if (intrstate == INTR_DONE) {
        n->lflags |= CLF_INLINE;
        n->own = (UBYTE)in->restype;    /* compiled inline */
    }
    intrcur = scur;
    intrstate = sstate;
    if (np) {
        if (c != ')')
            syntaxerr();
        getchar3();
    }
    regat = n->reg;                 /* .skip */
    emitact(libresult, n, 0, n->own);
    return callcont(n, type);       /* .notst */
}

/* blitz2.s:10087 function - do a function.  A library function or an OS
   call: its token looked up (looktoke), its call node made and its
   library used (calluse: findtoke's use, at its moment) */
static UWORD function(UWORD c, UWORD *type)
{
    struct blitzlib *bl;
    struct exprnode *n;
    UBYTE *sub;
    UWORD fl;

    if (c <= 0x8000 + tnum)
        syntaxerr();
    /* .overasm */
    if (c == TOK_ERN)
        return doern(c, type);
    if (c == TOK_ADDR)
        return doaddr(type);
    if (c == TOK_MAXIMUM)
        return domaximum(type);
    if (c == TOK_USED)
        return doused(type);
    ext_newtoken(c & 0x7fff);
    sub = looktoke(c & 0x7fff, &bl);    /* findtoke: the lookup */
    n = callnode(CLK_LIB, *type);   /* remember old type! */
    n->value = (ULONG)sub;
    n->aux = (ULONG)bl;
    calluse(n);                     /* and the use of the lib */
    fl = W_(sub);
    if (!(fl & 2))
        syntaxerr();
    if (fl & 4) {
        n->op = CLK_OS;
        return amigalib(n, type);
    }
    return libfunction(n, c & 0x7fff, type);
}


/* blitz2.s:10279 pokemovem */
void pokemovem(ULONG l)
{
    if ((UWORD)l)
        pokel(l);
}

/* blitz2.s:10283 savereg - set up d4 for movem -(a7) and d5 for
   movem (a7)+ */
void savereg(ULONG *push, ULONG *pull)
{
    ULONG pushop = 0x48e70000UL, pullop = 0x4cdf0000UL;

    if (regat) {
        pushop |= 1 << 15;
        pullop |= 1 << 0;
        if (regat >= 2) {
            pushop |= 1 << 14;      /* .skip3 */
            pullop |= 1 << 1;
        }
    } else if (fuckpos) {           /* .skip */
        pushop |= 1 << 14;
        pullop |= 1 << 1;
    }
    *push = pushop;
    *pull = pullop;
}

/* blitz2.s:10305 makesbase */
static void makesbase(void)
{
    if (sbasegot) {
        /* S base has been got, was it for this Eval? */
        if (!sbgot) {
            /* No, push it on da stack! */
            sbgot = 2;
            pokewd(*movea3a7);
        }
        return;
    }
    /* .skip: no sbase got at all! */
    sbasegot = -1;
    sbgot = 1;                      /* voodoo magic! - Will it work? */
    tokejsr(workstart);
}

/* no asm counterpart; replaces blitz2.s:10327 litstring's bsr makesbase -
   the first part of the code of the string literal's leaf (a->p): the
   work area */
static void litbase(struct expract *a)
{
    ((struct exprnode *)a->p)->start = pc;
    makesbase();
}

/* no asm counterpart; replaces blitz2.s:10391 makelit - the rest of the
   code of the string literal's leaf l: the address of its text in the
   literal area (l->value) into a0 with its relocation, its length
   pushed, the copy into the work area (copstring) */
static void litemit(struct expract *a)
{
    struct exprnode *l = a->p;

    pokewd(*numtoa0);
    addoff();
    pokel(l->value);
    pokewd(*pushlen);
    tokejsr(copstring);
    l->end = pc;
}

/* blitz2.s:10327 litstring.  The work area first (makesbase: the first
   part of the leaf's code, at the original's moment, before the text is
   read); then the parse part: the text read and written to the literal
   area as it is read, AmiBlitz3's escapes with it (ext_strescape), its
   place and length in the leaf (ELK_STRING: value, aux); then the rest
   of its code (litemit).  A String whatever the type so far. */
static UWORD litstring(UWORD *type)
{
    struct exprnode *l = newleaf(ELK_STRING, *type);
    UWORD len = 0, c;

    sbaseuse();
    emitact(litbase, l, 0, 0);
    l->value = litdata1 = data1;
    /* placeholder for the length long (asm pokes whatever d1 holds) */
    pokedata1(0);
    pokedata1(0);
    for (;;) {
        c = getchar3();
        if (c == 0)
            notqerr();
        if (c == '"')
            break;
        if (c == '\\' && ext_strescape(&len))
            continue;
        len++;
        pokedata1b((UBYTE)c);
    }
    if (!nomemleft) {               /* .done */
        W_(litdata1) = 0;
        W_(litdata1 + 2) = len;
    }
    /* .skip - Fixed (#969): the original rounds data1 up to even
       (blitz2.s:10346-10347) and never writes the pad byte: with
       Compile/Run it holds the previous compile's byte. Written as 0,
       pokedata1b bounds it like the string's bytes. */
    if (data1 & 1)
        pokedata1b(0);
    l->aux = len;
    l->own = l->tafter = T_STRING;
    tokuse(copstring);
    emitact(litemit, l, 0, 0);
    leafdone(l, type);
    return getchar3();
}

/* blitz2.s:10352 makealab - falls through into makealab2 */
struct label *makealab(UWORD *c)
{
    UWORD ch, len = makename2(&ch);

    if (!len)                       /* Fixed (#30): Z not tested in the asm */
        syntaxerr();
    if (c)
        *c = ch;
    return makealab2(len, c);
}

/* no asm counterpart; replaces blitz2.s:10368-10380 makealab2's .makeit - an
   outstanding reference to the label l at pc + 2 (the long of the
   instruction about to be written) */
static void labref(struct label *l)
{
    struct labref *r = doallocerr(12, MEMF_PUBLIC);

    r->next = l->refs;
    l->refs = r;
    r->addr = pc + 2;
    r->procnum = procnum;
    r->line = linenumat;
}

/* no asm counterpart; replaces blitz2.s:10353-10364 and 10382-10389
   makealab2 without .makeit - the label named namebuff (len), made if
   there is none (a label not defined yet, in the label list from now
   on); *ref: a reference to it is needed (it is not defined yet) */
static struct label *labfind(UWORD len, BOOL *ref)
{
    struct node *last;
    struct label *l = findadd(len, &last);

    *ref = FALSE;
    if (l) {
        /* Fixed (#977): a constant (refs & 1) goes to gotit and illlaberr
           whatever its value.  blitz2.s decides "made" by the value alone:
           a constant of value 0 counted as a label not defined yet, a
           reference was chained in front of the constant's refs (next
           pointer 1), and the later definition of the label walked it. */
        if (l->pc || ((ULONG)l->refs & 1))
            goto gotit;             /* already made, or a constant */
    } else {
        l = (struct label *)addhere(last, NAMEOFF_LABEL, len);
        l->refs = 0;                /* no refs */
        l->pc = 0;                  /* not created yet (no PC) */
        l->data = 0;                /* no data label */
        l->procnum = linenumat;     /* context - NB(orig): line, not procnum */
    }
    /* .makeit: create an oustanding reference to this label! (labref) */
    *ref = TRUE;
    return l;
gotit:
    if ((ULONG)l->refs & 1)
        illlaberr();
    if (procnum != l->procnum)
        referr();
    return l;
}

/* blitz2.s:10353 makealab2 - the label, and the reference to it if it is
   not defined yet (labfind, labref) */
struct label *makealab2(UWORD len, UWORD *c)
{
    BOOL ref;
    struct label *l = labfind(len, &ref);

    (void)c;
    if (ref)
        labref(l);
    return l;
}

/* no asm counterpart; replaces blitz2.s:10409-10416 qmark's code - the
   reference to the label if it is not defined yet, then move.l
   #label,D(regat) with its relocation */
static void labelemit(struct expract *a)
{
    struct exprnode *l = a->p;
    struct label *lab = (struct label *)l->value;

    l->start = pc;
    if (l->lflags & ELF_REF)
        labref(lab);
    pokewd((regat << 9) | 0x203c);  /* create move.l #loc,dn */
    addoff();
    pokel(lab->pc);
    l->end = pc;
}

/* blitz2.s:10406 qmark - ?label.  The parse part finds the label (or
   makes it: in the label list at first sight); the reference and the
   relocation are made by its code (labelemit).  Not inlined: its locals
   (two of them in memory) would be part of element's frame, which every
   level of brackets holds. */
static __attribute__((noinline)) UWORD qmark(UWORD *type)
{
    struct exprnode *l;
    struct label *lab;
    UWORD ch, len;
    BOOL ref;

    getchar3();
    len = makename2(&ch);           /* makealab */
    if (!len)                       /* Fixed (#30): Z not tested in the asm */
        syntaxerr();
    lab = labfind(len, &ref);
    l = newleaf(ELK_LABEL, *type);
    l->value = (ULONG)lab;
    if (ref)
        l->lflags |= ELF_REF;
    l->own = l->tafter = T_LONG;
    emitact(labelemit, l, 0, 0);
    leafdone(l, type);
    return reget();
}

/* blitz2.s:10449 fetchqasm - "abcd" character constant */
static UWORD fetchqasm(UWORD *type)
{
    ULONG value = 0;
    UWORD c;

    for (;;) {
        c = getchar3();
        if (eos(c))
            syntaxerr();
        if (c == '"')
            break;
        value = (value << 8) | (UBYTE)c;
    }
    getchar3();                     /* .done */
    return (UWORD)fetchnum2(value, type, FALSE);
}

/* blitz2.s:10460 fetchasm - label/constant value in an asm operand */
static UWORD fetchasm(UWORD *type)
{
    UWORD saved = *type, c, len;
    struct label *l;

    len = makename2(&c);            /* get the things name */
    if (!len)                       /* Fixed (#30): Z not tested in the asm */
        syntaxerr();
    l = findadd(len, NULL);
    if (!l) {
        ULONG value;

        if (!debugconst(len, &value))
            cnferr();
        *type = saved;              /* #__debug: a constant */
        return (UWORD)fetchnum2(value, type, FALSE);
    }
    if ((ULONG)l->refs != 1)
        asmtype = ~asmtype;         /* NB(orig): toggles (not asmtype) */
    *type = saved;                  /* .npc */
    return (UWORD)fetchnum2(l->pc, type, FALSE);
}

/* ------------------------------------------------------------- variables */

/* blitz2.s:10641 variable2 - get the value of a var into D(regat), from
   the variable's leaf l: l->vtype = calcvar result (asm d2), l->off =
   a5/a4 offset (asm d3) */
static void stvar(const struct exprnode *l);
static void variable2(const struct exprnode *l)
{
    UWORD vartype = l->vtype, opcode;

    if ((vartype & 0xff) == T_STRING) {
        stvar(l);
        return;
    }
    opcode = regat << 9;
    if (vartype & TF_POINTER)
        vartype = (vartype & 0xff00) | 3; /* .pointer: move.b #3,d2 */
    else if (!(vartype & 0xff)) {
        pokewd(opcode | 0x200a);    /* move.l a2,Dn */
        return;
    }
    if (vartype & TF_ADDRINA2) {    /* .skip2 */
        opcode |= *movea2dn;
    } else
        opcode |= *moved3a5dn;      /* .simpvar */
    /* .size - correct for size */
    if (!(vartype & TF_POINTER)) {
        if ((vartype & 0xff) == 1)
            opcode ^= 0x3000;       /* .byte */
        else if ((vartype & 0xff) == 2)
            opcode |= 0x1000;       /* .word */
    }
    if (vartype & TF_ADDRINA2)
        pokewd(opcode);
    else {
        pokewda5s(opcode);
        pokewd(l->off);
    }
}

/* no asm counterpart; replaces the type variable2 leaves in d2
   (blitz2.s:10641-10660: moveq #3 for the address of a NewType variable,
   move.b #3 for a pointer, .more: and #255) - the own type of the value
   of a variable whose calcvar type is vartype */
static UWORD vartypeof(UWORD vartype)
{
    if ((vartype & 0xff) == T_STRING)
        return T_STRING;
    if ((vartype & TF_POINTER) || !(vartype & 0xff))
        return T_LONG;
    return vartype & 0xff;
}

/* no asm counterpart; replaces the decisions of blitz2.s:10479-10498
   varcont - the value of own type l->own under the type so far
   l->tbefore: its facts and the type so far after it, in the leaf
   (exprtype_element) */
static void contleaf(struct exprnode *l)
{
    struct exprtype_elem e = exprtype_element(l->tbefore, l->own, NULL);

    if (e.wordval)
        l->lflags |= ELF_WORD;
    if (e.ext)
        l->lflags |= ELF_BYTE;
    if (e.raw)
        l->lflags |= ELF_RAW;
    else if (e.conv_from)           /* from own to the result */
        l->lflags |= ELF_CONV;
    l->tafter = e.result;
}

/* no asm counterpart; replaces the code of blitz2.s:10479-10499 varcont -
   after a value in D(regat): a Byte extended, the value converted to the
   type so far, from the leaf l (contleaf) */
static void contcode(struct exprnode *l)
{
    /* Fixed (#978): a byte value is fetched sign-extended ($90 ->
       $FF90), while consttype2 takes a constant from 128 to 255 as a
       positive word ($0090): the original's compare of "a.b=$90" with
       "a=$90" never matched (only a=-112 did).  The constant it is
       compared with is sign-extended (binopbuild: ELF_SIGNEXT). */
    if (l->lflags & ELF_BYTE)
        pokewd(regat | 0x4880);     /* ALWAYS CONVERT BYTE TO WORD. ext.w Dn */
    /* .notabyte */
    /* Fixed (#1178): a Quick under a Long type so far is not cut to a
       whole number here (the original's goto conv2, blitz2.s:10491-10498:
       r.l=q*2 with q=1.5 gave 2).  It stays a raw 16.16 value, the type
       so far stays Long and rawq says so (contrecord): the operator
       computes with the fraction and rounds (operate, mixedop: + - *
       down, / towards zero), a comparison takes it as a Quick (#1177), a
       shift, logic operator, Mod, Not and the end of the expression round
       it down as the original does. */
    if ((l->lflags & (ELF_RAW | ELF_CONV | ELF_DROP)) != ELF_CONV)
        return;
    /* Fixed (#1179): the conversion that ends a narrowed later index
       (idxnarrow) leaves out the ext.l of a Byte's, Word's or Quick's
       conversion to Long (bytetolong, wordtolong, quicktolong) */
    if (l->lflags & ELF_NARROW) {
        if (l->own == T_QUICK)
            quicktob2();
        return;
    }
    convtype(l->own, l->tafter);    /* bump up new type to old.... */
}

static void contemit(struct expract *a)
{
    contcode(a->p);
}

/* no asm counterpart: the code fn of the leaf l of a value with varcont's
   code (contemit, varemit) recorded, and what the code after it reads
   while the expression is built: the conversion's errors and libraries,
   a Byte or Word value (#1179, the forthstack's wordval), a raw Quick
   (#1178: rawq), the conversion a comparison may drop (#1177: bconv) */
static void contrecord(struct exprnode *l, exprtree_actfn *fn)
{
    UWORD relocs = relocpending();

    if (l->lflags & ELF_CONV)
        convprep(l->own, l->tafter);
    emitact(fn, l, 0, 0);
    p5leaf(l, P5L_CONT);            /* the typing pass (#1216) */
    bforth[nbforth - 1].wordval = (l->lflags & ELF_WORD) != 0;
    if (l->lflags & ELF_BYTE)
        bbyte = codeseq;            /* #978: its code ends in the ext.w */
    if (l->lflags & ELF_RAW)
        rawq = TRUE;
    else if (l->lflags & ELF_CONV) {
        bconv.seq = codeseq;
        bconv.from = l->own;
        bconv.to = l->tafter;
        bconv.depth = evaldepth;
        bconv.leaf = l;
        bconv.relocs = relocs;
    }
}

/* no asm counterpart; replaces blitz2.s:10478 variable's bsr variable2
   and the code of varcont - the code of the variable's leaf l: its value
   into D(regat), its type reported (ext_evalvalue), then varcont's code */
static void varemit(struct expract *a)
{
    struct exprnode *l = a->p;

    l->start = pc;
    varmode = (l->lflags & ELF_LOCAL) ? -1 : 0;   /* pokewda5s: a4 or a5 */
    variable2(l);
    ext_evalvalue(l->value, (UWORD)l->aux);
    contcode(l);
    l->end = pc;
}

/* blitz2.s:10471 variable.  The parse part finds the variable (fetchvar
   makes it at first sight, with its offset) and reads its path
   (varparse: the code of an array's indexes, a pointer's or a path's
   address recorded as they are read); its variable node becomes the
   leaf, with calcvar's type, the offset, what ext_evalvalue reports, and
   varcont's decisions (contleaf).  A string variable uses the string
   base (stvar).  A name with { is a procedure call (afunction, its call
   node). */
static UWORD variable(UWORD c, UWORD *type)
{
    UWORD namelen;          /* asm d2 after getvname */
    UWORD old = *type;              /* remember old type */
    struct var *v;
    struct type *t;
    struct exprnode *l;

    c = getvname(c, &namelen);
    if (c == '{')
        return afunction(old, type, namelen);
    v = fetchvar(namelen, &t, &c);
    l = varparse(v, t, &c);
    varleaf(l, ELK_VAR, old);
    l->own = (UBYTE)vartypeof(l->vtype);
    contleaf(l);
    if ((l->vtype & 0xff) == T_STRING) {
        sbaseuse();
        tokuse(copstring);
    }
    contrecord(l, varemit);
    leafdone(l, type);
    return reget();                 /* varcont .done */
}

/* blitz2.s:10502 savem */
UWORD savem(void)
{
    ULONG mask = 0;
    UWORD n = regat;

    if (!n)
        return 0;
    while (n--) {
        mask >>= 1;
        mask |= 0x8000;
    }
    pokewd(0x48e7);
    pokewd((UWORD)mask);
    return (UWORD)mask;
}

/* blitz2.s:10515 makefjsr */
void makefjsr(ULONG addr)
{
    pokewd(0x4eb9);
    addoff();
    pokel(addr);
}

/* no asm counterpart; replaces blitz2.s:10531-10548 and 10556 afunction
   from the stack adjustment up to .sssz's bsr savem - the enter action
   of the procedure call n: the stack the procedure removes, the string
   base, a3 pushed in a string expression, the registers below regat
   saved (regat 0 for the parameters: afunction) */
static void procenter(struct expract *a)
{
    struct exprnode *n = a->p;
    const struct proc *p = (const struct proc *)n->value;

    stackuse -= ext_procstackbytes(p);  /* removed by the procedure */
    if (n->own == T_STRING)         /* .skip: function type! */
        makesbase();
    if (sbasegot) {                 /* .sss */
        n->lflags |= CLF_SB;
        dopusha3();
    }
    n->aux = savem();               /* .sssz: the movem regs */
}

/* no asm counterpart; replaces blitz2.s:10550-10553 and 10564-10567
   afunction's makefjsr and bsr chkstak - the first part of the exit
   action of the procedure call n: the jsr (the old regat back:
   afunction) */
static void procexit(struct expract *a)
{
    const struct proc *p = (const struct proc *)((struct exprnode *)a->p)->value;

    if (p->nparams)
        chkstak();
    makefjsr(p->pc);                /* pc of function call */
}

/* no asm counterpart; replaces blitz2.s:10572-10609 afunction from
   .nopars' result move up to .notst - the rest of the exit action of
   the procedure call n, after its }: the result into D(regat), the
   registers restored, a3 back, the type reported (ext_evalvalue) */
static void procresult(struct expract *a)
{
    struct exprnode *n = a->p;

    if (regat)
        pokewd((regat << 9) | 0x2000);  /* move.l d0,regat */
    if (n->aux) {                   /* .nopars3 */
        pokewd(0x4cdf);
        pokewd(revmask((UWORD)n->aux)); /* poke movem (a7)+... */
    }
    if (n->lflags & CLF_SB) {       /* .nomovem */
        if (n->own == T_STRING)
            pokewd(*deca3);         /* .issb: is a string, wack off last '0' */
        else
            /* not a string...set a3 back to start of any strings... */
            pokecode(geta3, geta3f);
        dopulla3();                 /* .nosb: fix up string base */
    }
    ext_evalvalue(ext_proctype((const struct proc *)n->value), 0); /* .nores */
    if (n->own == T_STRING)
        pokewd(*putstlen | regat);
}

/* blitz2.s:10522 afunction - do a local function, eg a=pixel{x,y}.  The
   parse part: the procedure looked up, the call node, the parameters
   (fetchprocps: each compiled into its register or pushed as it is
   read), the }; the code: the node's enter action (procenter) before the
   parameters, its exit action after them (procexit, the jsr; procresult
   after the }, which a call without parameters reads after the jsr).
   regat is kept in the node and 0 for the parameters (.somepars), back
   after the jsr. */
static UWORD afunction(UWORD oldtype, UWORD *type, UWORD namelen)
{
    struct proc *p = findproc(namelen, NULL);
    struct exprnode *n;
    UWORD c;

    if (!p)
        noprocerr();
    if (p->rettype < 0)
        illprocerr();
    n = callnode(CLK_PROC, oldtype);
    n->value = (ULONG)p;
    bstrwork = TRUE;                /* its strings, its a3: not known here */
    n->own = (UBYTE)p->rettype;
    if (n->own == T_STRING)
        sbaseuse();
    emitact(procenter, n, 0, 0);
    n->reg = (UBYTE)regat;          /* .somepars: old regat */
    if (p->nparams)
        regat = 0;
    if (!p->nparams) {
        emitact(procexit, n, 0, 0);
        regat = n->reg;
        c = getchar3();
    } else {
        c = fetchprocps(p);         /* pointer to var desc */
        if ((debugga & 0xff00) && !makeexec)   /* chkstak */
            tokuse(stchk);
        emitact(procexit, n, 0, 0);
        regat = n->reg;
    }
    if (c != '}')                   /* .nopars */
        syntaxerr();
    emitact(procresult, n, 0, 0);
    getchar3();                     /* .notst */
    return callcont(n, type);
}

/* blitz2.s:10617 stvar - handle string variable get!  The code of the
   string variable's leaf l (l->vtype: calcvar's type, l->off: its a5/a4
   offset): the work area (makesbase), the variable's address into a2
   unless its path left it there, its length pushed and its text copied
   into the work area (copstring).  The type (asm moveq #7,d2) is the
   leaf's (vartypeof), the char after it is read by variable (asm: bra
   reget). */
static void stvar(const struct exprnode *l)
{
    makesbase();
    if (!(l->vtype & TF_ADDRINA2)) {
        leaa5d3a2[1] = l->off;
        pokela5s(L_(leaa5d3a2));
    }
    /* .already */
    stvarget2[0] = (regat << 9) | 0x7000;              /* moveq #0,Dn */
    stvarget2[1] = (stvarget2[1] & 0xfff8) | regat;    /* move.l Dn,-(a7) */
    pokecode(stvarget2, stvarget2f);
    /* NB: the template's beq.w skips exactly the 6 byte jsr from tokejsr */
    tokejsr(copstring);
}


/* ======================================================= the typing pass
   No asm counterpart (OpenSpec change expr-tree-5-operand-typing, design
   decisions 1, 6-10, task 4.1; issues #1216, #1183): rule P+.  While an
   expression is built, the tree of the typing pass is made beside it
   (struct p5node: exprtyping.h's node and the actions of its code: an
   element's leaf, a minus sign, Not, an operator).  When the expression is
   built (evalbuild, p5end), exprtyping_type decides every operation.
   Where its decisions give the values the code built gives (p5same),
   that code is written as it is; otherwise its actions are rewritten
   (p5apply): a value is fetched in its own type, every operator, minus
   sign, Not and folded constant writes the code of the pass's decision
   (p5binopact, p5negact, p5notact, p5constact), and the end of the
   evaluation takes the value as the pass says (p5exit).  The libraries
   the new code calls are used when it is decided, while the expression
   is built (the same functions with p5prep set).  A 48.16 intermediate
   result is a register pair, D(r) its low and D(r+1) its high 32 bits
   (design decision 9); the right operand of an operation whose left
   operand is a pair is computed one register higher (p5shift); an
   operand that needs a register beyond D7 is "Expression too Complex",
   as #1185 (design decision 10).  The 48.16 operations are compilerlib's
   routines (a in d0/d1, b in d2/d3, the result in d0/d1; d2 and d3
   changed, every other register kept); an addition, a subtraction and a
   comparison write the routine's add/addx or sub/subx inline in d0-d3
   (task 4.3). */

#define P5STACK 192                 /* the nodes pending in the build */
#define P5C_PAIR 100                /* EVAL's vtype: the value as 48.16 (a Select's, T_SELWIDE) */

BOOL evalcond = FALSE;              /* eval.h */
BOOL evalwide = FALSE, p5wide = FALSE;
#define P5MARKS 48                  /* the evaluations nested */
#define P5DEPTH 64                  /* the deepest tree the pass types (its recursion) */

enum {
    P5F_RAW = 1,                    /* the build's code holds a raw Quick (#1178) */
    P5F_RELAX = 2,                  /* the build's typing refused it: the pass decides */
    P5F_KEEP = 4,                   /* a folded constant whose code stays the build's */
    P5F_SIGNEXT = 8,                /* a constant compared with a byte value (#978) */
    P5F_NDIV = 16,                  /* a 48.16 / of a 16.16 value by a whole constant (p5fastmark) */
    P5F_NMULF = 32,                 /* a 48.16 * of a 16.16 value and a constant below 1 */
    P5F_WMUL = 64,                  /* a 48.16 * of a 16.16 value and a Word, Byte or Word constant */
    P5F_LDIV = 128                  /* a / at the top under a wanted Byte, Word or Long (ETC_TRUNC): its Long */
};

struct p5node {
    struct exprtyping_node t;       /* the pass's node (first member) */
    struct exprnode *x;             /* a leaf: the element's; NEG: its node; BIN: the BINOP */
    struct expract *prev;           /* the action before its code */
    struct expract *act;            /* CONST: constemit's action; NEG, NOT, BIN: its own */
    struct expract *last;           /* its last action */
    UBYTE lkind, flags;             /* P5L_*, P5F_* */
    UBYTE tt;                       /* the type the build's code leaves it in */
    UBYTE dt, df;                   /* the pass's code: the type and format it leaves */
    UBYTE want;                     /* BIN: the wanted type of its expression */
    UBYTE depth;                    /* the depth of its subtree */
    ULONG whole, ffp;               /* CONST: its whole number, its FFP value */
    ULONG enc, enc2;                /* a folded constant the pass writes: its value (a 48.16 one: the low and the high 32 bits) */
};
#define P5(n) ((struct p5node *)(n))

/* an exact value n/d (d > 0, lowest terms); ok FALSE: beyond int64_t */
struct p5q {
    int64_t n, d;
    BOOL ok;
};

static struct p5node *p5stk[P5STACK];
static UWORD p5sp;
static struct {
    UWORD sp;
    BOOL bad, relax;
} p5mk[P5MARKS];
static UWORD p5nm, p5over;
static struct exprnode *p5leafx;    /* the last leaf an element made */
static UBYTE p5leafk;
static UWORD p5leafd;               /* its evaldepth */
static BOOL p5cpoint;               /* the last constant's facts (p5constleaf) */
static ULONG p5cwhole, p5cffp;
static struct expract *p5cact;
static BOOL p5notrelax;
/* the code functions record what they need and write nothing */
static BOOL p5prep;

#define P5_SPFIX (-30)
#define P5_SPFLT (-36)
#define P5_SPNEG (-60)
#define P5_SPADD (-66)
#define P5_SPSUB (-72)
#define P5_SPMUL (-78)
#define P5_SPDIV (-84)

/* the path of an operation whose value is taken as its low 32 bits only
   (p5lowmark: an ETY_Q48 operation under ETC_KEEP, task 4.3) */
#define P5Y_LOW 'l'

/* compilerlib's 48.16 routines (libs/compilerlib; q48add $D603 and q48sub
   $D604 are written inline) */
#define q48mul 0xd605
#define q48div 0xd606
#define q48divq 0xd607
#define q48divw 0xd608
#define q48divb 0xd609
#define q48mod 0xd60a
#define q16mod 0xd60f                /* the exact Mod of two 16.16 values (the speed review) */
#define q48mull 0xd610               /* a Long times a Quick, exact, not widened (the speed review) */
#define q48mulf 0xd611               /* q48mul cut down (#1221) */

static void p5reset(void)
{
    p5sp = p5nm = p5over = 0;
    p5leafx = NULL;
    p5notrelax = FALSE;
    p5unit = FALSE;
    evalcond = evalwide = p5wide = FALSE;
}

/* the build records the tree of the evaluation */
static BOOL p5active(void)
{
    return p5nm && !p5over && !p5mk[p5nm - 1].bad && exprtree_building();
}

static void p5fail(void)
{
    if (p5nm && !p5over)
        p5mk[p5nm - 1].bad = TRUE;
}

static struct p5node *p5new(unsigned kind)
{
    struct p5node *p = exprtree_alloc(sizeof *p);

    memset(p, 0, sizeof *p);
    p->t.kind = (UBYTE)kind;
    p->depth = 1;
    return p;
}

static void p5push(struct p5node *p)
{
    if (p5sp >= P5STACK) {
        p5fail();
        return;
    }
    p5stk[p5sp++] = p;
}

static struct p5node *p5pop(void)
{
    if (p5sp <= p5mk[p5nm - 1].sp) {
        p5fail();
        return NULL;
    }
    return p5stk[--p5sp];
}

/* an evaluation begins (evalbuild) */
static void p5begin(void)
{
    if (p5over || p5nm >= P5MARKS) {
        p5over++;
        return;
    }
    p5mk[p5nm].sp = p5sp;
    p5mk[p5nm].bad = !exprtree_building() || constmode;
    p5mk[p5nm].relax = FALSE;
    p5nm++;
}

/* the leaf l of kind kind (NULL: an element begins) */
static void p5leaf(struct exprnode *l, unsigned kind)
{
    if (!p5active())
        return;
    p5leafx = l;
    p5leafk = (UBYTE)kind;
    p5leafd = evaldepth;
}

void p5constleaf(struct exprnode *l, BOOL point, ULONG whole, ULONG ffp)
{
    if (!p5active())
        return;
    p5leaf(l, P5L_CONST);
    p5cpoint = point;
    p5cwhole = whole;
    p5cffp = ffp;
    p5cact = exprtree_lastact();
}

/* the exact value of the FFP number f as num/den (FALSE: beyond the
   range of the pass) */
static BOOL p5ffpq(ULONG f, int64_t *num, int64_t *den)
{
    int64_t m = (int64_t)(f >> 8);
    int e = (int)(f & 0x7f) - 64 - 24;

    *num = 0;
    *den = 1;
    if (!m)
        return TRUE;
    while (!(m & 1) && e < 0) {
        m >>= 1;
        e++;
    }
    if (e >= 0) {
        if (e > 36)
            return FALSE;
        *num = m << e;
    } else {
        if (e < -60)
            return FALSE;
        *num = m;
        *den = (int64_t)1 << -e;
    }
    if (f & 0x80)
        *num = -*num;
    return TRUE;
}

/* the element is built (prev: the action before its code): its leaf */
static void p5elemdone(struct expract *prev)
{
    struct exprnode *l = p5leafx;
    struct p5node *p;

    if (!p5active())
        return;
    if (!l || !prev || p5leafd != evaldepth) {
        p5fail();
        return;
    }
    if (p5leafk == P5L_CONST) {
        p = p5new(ETN_CONST);
        p->act = p5cact;
        p->whole = p5cwhole;
        p->ffp = p5cffp;
        p->t.point = (UBYTE)p5cpoint;
        if (p5cpoint) {
            if (!p5ffpq(p5cffp, &p->t.num, &p->t.den)) {
                p5fail();
                return;
            }
        } else {
            p->t.num = (LONG)p5cwhole;
            p->t.den = 1;
        }
    } else {
        UWORD t = l->own ? l->own : l->tafter;

        if (t == T_ADDR)
            t = T_LONG;
        if (t < T_BYTE || t > T_STRING || t == T_ADDR) {
            p5fail();
            return;
        }
        p = p5new(ETN_VAR);
        p->t.type = (UBYTE)t;
    }
    p->lkind = p5leafk;
    p->x = l;
    p->prev = prev;
    p->last = exprtree_lastact();
    p5push(p);
}

/* a run of minus signs (NEG, x its node) or Not (NOT) on the node on top */
static void p5wrap(unsigned kind, struct exprnode *x, struct expract *prev, struct expract *act)
{
    struct p5node *c, *p;
    BOOL relax = kind == ETN_NOT && p5notrelax;

    p5notrelax = FALSE;
    if (!p5active())
        return;
    c = p5pop();
    if (!c)
        return;
    if (!prev) {
        p5fail();
        return;
    }
    p = p5new(kind);
    p->t.l = &c->t;
    p->x = x;
    p->prev = prev;
    p->act = p->last = act;
    p->depth = (UBYTE)(c->depth + 1);
    if (p->depth > P5DEPTH)
        p->depth = P5DEPTH + 1;
    if (relax)
        p->flags |= P5F_RELAX;
    p5push(p);
}

/* the operator n is built (act: its action) */
static void p5binop(struct exprnode *n, struct expract *act)
{
    struct p5node *l, *r, *p;

    if (!p5active())
        return;
    r = p5pop();
    l = r ? p5pop() : NULL;
    if (!l)
        return;
    p = p5new(ETN_BIN);
    p->t.op = (UBYTE)n->op;
    p->t.l = &l->t;
    p->t.r = &r->t;
    p->x = n;
    p->prev = l->prev;
    p->act = p->last = act;
    p->depth = (UBYTE)((l->depth > r->depth ? l->depth : r->depth) + 1);
    if (p->depth > P5DEPTH)
        p->depth = P5DEPTH + 1;
    if (n->lflags & BNF_P5)
        p->flags |= P5F_RELAX;
    p5push(p);
}

/* a constant with a point in the subtree */
static BOOL p5point(const struct exprtyping_node *n)
{
    switch (n->kind) {
    case ETN_CONST: return n->point;
    case ETN_BIN: return p5point(n->l) || p5point(n->r);
    case ETN_VAR: return FALSE;
    default: return p5point(n->l);
    }
}

/* a shift of a constant with a point in the subtree (the pass does not
   fold it: the shifted constant is a Quick) */
static BOOL p5shpoint(const struct exprtyping_node *n)
{
    switch (n->kind) {
    case ETN_BIN:
        if (n->op >= ETO_LSL && n->op <= ETO_ASR && p5point(n->l))
            return TRUE;
        return p5shpoint(n->l) || p5shpoint(n->r);
    case ETN_NEG:
    case ETN_NOT: return p5shpoint(n->l);
    default: return FALSE;
    }
}

/* a constant subtree whose value the pass folds */
static BOOL p5folded(const struct exprtyping_node *n)
{
    return exprtyping_isconst(n) && !p5shpoint(n);
}

static BOOL p5isbit(unsigned op)
{
    return op == ETO_AND || op == ETO_OR || op == ETO_EOR || op == ETO_AMP || op == ETO_BAR
           || (op >= ETO_BITTST && op <= ETO_BITCHG);
}

/* the build refuses the operator n (d its decision) for a constant with
   a point under AND, OR, EOR (either operand) or as the shifted operand:
   the pass decides (d gets a type for the build) */
static BOOL p5relaxbin(const struct exprnode *n, struct exprtype_bin *d)
{
    const struct exprtyping_node *l, *r;

    if (!p5active() || p5sp < p5mk[p5nm - 1].sp + 2)
        return FALSE;
    l = &p5stk[p5sp - 2]->t;
    r = &p5stk[p5sp - 1]->t;
    if (!exprtype_bypass(n->op, n->tbefore, n->vtype, exprtyping_isconst(l) && p5point(l),
                         exprtyping_isconst(r) && p5point(r)))
        return FALSE;
    memset(d, 0, sizeof *d);
    d->result = T_LONG;
    p5mk[p5nm - 1].relax = TRUE;
    return TRUE;
}

/* the same for Not of a constant with a point (of the Float type) */
static BOOL p5relaxnot(UWORD t)
{
    const struct exprtyping_node *c;

    if (!p5active() || p5sp <= p5mk[p5nm - 1].sp)
        return FALSE;
    c = &p5stk[p5sp - 1]->t;
    if (!exprtype_bypassnot(t, exprtyping_isconst(c) && p5point(c)))
        return FALSE;
    p5notrelax = TRUE;
    p5mk[p5nm - 1].relax = TRUE;
    return TRUE;
}

/* --------------------------------------------- the build's code compared */

static UWORD p5norm(UWORD t)
{
    return t == T_ADDR ? T_LONG : t;
}

/* the type the build's code leaves each node in (tt); a raw Quick (#1178) */
static void p5today(struct p5node *p)
{
    struct exprnode *x = p->x;

    switch (p->t.kind) {
    case ETN_CONST:
        p->tt = (UBYTE)((x->lflags & ELF_FFP) ? T_FLOAT : x->tafter);
        break;
    case ETN_VAR:
        if (p->lkind == P5L_CONT && (x->lflags & ELF_RAW)) {
            p->flags |= P5F_RAW;
            p->tt = T_QUICK;
        } else if (p->lkind == P5L_CONT && (x->lflags & ELF_DROP))
            p->tt = x->own;
        else
            p->tt = (UBYTE)x->tafter;
        break;
    case ETN_NEG:
        p5today(P5(p->t.l));
        p->tt = (x->flags & EXF_ABSORBED) ? P5(p->t.l)->tt : (UBYTE)p->act->y;
        break;
    case ETN_NOT:
        p5today(P5(p->t.l));
        p->tt = (UBYTE)p->act->y;
        break;
    default:
        p5today(P5(p->t.l));
        p5today(P5(p->t.r));
        p->tt = (UBYTE)x->tafter;
        break;
    }
    p->tt = (UBYTE)p5norm(p->tt);
}

/* the build's decision for the operator n (operate's) */
static struct exprtype_bin p5todaybin(const struct exprnode *n)
{
    struct exprtype_binfacts f;

    memset(&f, 0, sizeof f);
    f.lraw = (n->lflags & BNF_LRAW) != 0;
    f.rraw = (n->lflags & BNF_RRAW) != 0;
    f.rcmp = (n->lflags & BNF_RCMP) != 0;
    f.words = (n->lflags & BNF_WORDS) != 0;
    return exprtype_binop(n->op, n->tbefore, n->vtype, &f);
}

/* a conversion from a to b keeps every value */
static BOOL p5lossless(UWORD a, UWORD b)
{
    a = p5norm(a);
    b = p5norm(b);
    return a == b || ((a == T_BYTE || a == T_WORD) && b > a && b <= T_FLOAT);
}

/* the constant subtree n is a single literal (with its minus signs) */
static const struct exprtyping_node *p5literal(const struct exprtyping_node *n, int *negs)
{
    *negs = 0;
    while (n->kind == ETN_NEG) {
        (*negs)++;
        n = n->l;
    }
    return n->kind == ETN_CONST ? n : NULL;
}

static BOOL p5fits(int64_t v, UWORD t)
{
    switch (t) {
    case T_BYTE: return v >= -128 && v <= 127;
    case T_WORD:
    case T_QUICK: return v >= -32768 && v <= 32767;
    case T_LONG: return v >= INT32_MIN && v <= INT32_MAX;
    case T_FLOAT: return v >= -0xffffff && v <= 0xffffff;
    default: return FALSE;
    }
}

/* the folded constant p taken in las gives the value the build's code
   gives */
static BOOL p5consteq(const struct p5node *p, UWORD las)
{
    const struct exprtyping_node *k;
    int negs;

    if (p->tt == las)
        return TRUE;
    k = p5literal(&p->t, &negs);
    return k && !k->point && k->den == 1 && p5fits(k->num, p->tt) && p5fits(k->num, las) && p->tt >= T_BYTE
           && p->tt <= T_FLOAT && las >= T_BYTE && las <= T_FLOAT;
}

static BOOL p5eq(struct p5node *p, UWORD las);
static struct p5q p5qval(const struct exprtyping_node *n);

/* the type a constant is compared in exactly with a value of type x (a
   comparison of the pass's ETY_EXACT): a whole number in x where it fits
   (a Quick's range), else as a Long; a fraction as a Quick */
static UWORD p5exactconst(const struct p5node *p, UWORD x)
{
    struct p5q v = p5qval(&p->t);

    x = p5norm(x);
    if (!v.ok)
        return T_LONG;
    if (v.d != 1)
        return T_QUICK;
    if (x >= T_BYTE && x <= T_QUICK && p5fits(v.n, x))
        return x;
    return T_LONG;
}

static UWORD p5max(UWORD a, UWORD b)
{
    return a > b ? a : b;
}

/* the pass's result type of the operator p */
static UWORD p5result(const struct p5node *p)
{
    switch (p->t.o.path) {
    case ETY_Q16: return T_QUICK;
    case ETY_EXACT:
    case ETY_COMPARE: return T_WORD;
    case ETY_MIXED:
    case ETY_Q48: return T_LONG;
    default: return p->t.o.result;
    }
}

/* the operator p as the pass decides it gives the value the build's
   code gives */
static BOOL p5eqbin(struct p5node *p)
{
    const struct exprtyping_op *o = &p->t.o;
    struct p5node *l = P5(p->t.l), *r = P5(p->t.r);
    struct exprtype_bin d;
    UWORD lt, rt, T, res = p5result(p);

    if ((p->flags & P5F_RELAX) || o->path == ETY_Q48)
        return FALSE;
    d = p5todaybin(p->x);
    if (d.result != res)
        return FALSE;
    switch (o->path) {
    case ETY_EXACT:
        if (o->lwide || o->rwide)
            return FALSE;
        lt = o->las == ETY_CONST ? p5exactconst(l, o->ras) : o->las;
        rt = o->ras == ETY_CONST ? p5exactconst(r, o->las) : o->ras;
        if ((lt == T_LONG && rt == T_QUICK) || (lt == T_QUICK && rt == T_LONG))
            return d.path == ETP_CMPLQ && p5eq(l, lt) && p5eq(r, rt);
        T = lt == T_QUICK || rt == T_QUICK ? T_QUICK : p5max(lt, rt);
        return d.path == ETP_COMPARE && d.handler == T && p5eq(l, T) && p5eq(r, T);
    case ETY_COMPARE:
        T = o->las;
        return d.path == ETP_COMPARE && d.handler == T && p5eq(l, T) && p5eq(r, T);
    case ETY_PLAIN:
    case ETY_Q16:
        if (p->t.op == ETO_MOD && (res == T_QUICK || res == T_FLOAT))
            return FALSE;           /* the exact Mod */
        T = o->path == ETY_Q16 ? T_QUICK : o->las;
        if (o->path == ETY_PLAIN && o->las != o->ras)
            return FALSE;
        if (d.path != ETP_PLAIN && !(d.path == ETP_MIXED && d.handler))
            return FALSE;
        return d.handler == T && p5eq(l, T) && p5eq(r, T);
    case ETY_MIXED:
        return d.path == ETP_MIXED && !d.handler && d.lq == o->lwide && d.rq == o->rwide && p5eq(l, o->las)
               && p5eq(r, o->ras);
    default: return FALSE;
    }
}

/* the constant subtree n has a Mod with a fraction in an operand: the
   pass's exact Mod (#1219, #1223), not the fold of today's Mod of the whole
   parts (DIVS.W by 0 for a divisor below 1) */
static BOOL p5constmodfrac(const struct exprtyping_node *n)
{
    struct p5q a, b;

    if (n->kind == ETN_NEG)
        return p5constmodfrac(n->l);
    if (n->kind != ETN_BIN)
        return FALSE;
    if (n->op == ETO_MOD) {
        a = p5qval(n->l);
        b = p5qval(n->r);
        if (a.ok && b.ok && b.n && (a.d != 1 || b.d != 1))
            return TRUE;
    }
    return p5constmodfrac(n->l) || p5constmodfrac(n->r);
}

/* the node p taken in las by the pass gives the value the build's code
   gives */
static BOOL p5eq(struct p5node *p, UWORD las)
{
    las = p5norm(las);
    if (p5folded(&p->t) && p5constmodfrac(&p->t))
        return FALSE;
    if (p5folded(&p->t))
        return p5consteq(p, las);
    if (p->t.typed && p->t.fmt == ETF_Q48)
        return FALSE;
    switch (p->t.kind) {
    case ETN_VAR:
        if (p->flags & P5F_RAW)
            return las == T_QUICK || las == T_LONG || las == T_FLOAT;
        return p->tt == las || p5lossless(p->t.type, p->tt);
    case ETN_NEG: return p5eq(P5(p->t.l), las);
    case ETN_NOT:
        if (p->flags & P5F_RELAX)
            return FALSE;
        return (p->t.t == p->tt || (p->t.t <= T_LONG && p->tt <= T_LONG)) && p5eq(P5(p->t.l), p->t.t);
    case ETN_BIN: return p5eqbin(p);
    default: return FALSE;
    }
}

/* the whole expression: the pass's decisions give the build's values */
static BOOL p5same(struct p5node *root, const struct exprtyping_result *res, UWORD want)
{
    switch (res->consumer) {
    case ETC_TYPE:
    case ETC_BOOL:
    case ETC_KEEP:
        if (res->fmt == ETF_Q48)
            return FALSE;
        return p5eq(root, want ? want : res->t);
    case ETC_FLOOR:
        if (res->fmt == ETF_Q48)
            return FALSE;
        return p5eq(root, T_QUICK);
    default: return FALSE;
    }
}

static BOOL p5hasstr(const struct exprtyping_node *n)
{
    switch (n->kind) {
    case ETN_VAR: return n->type == T_STRING;
    case ETN_CONST: return FALSE;
    case ETN_BIN: return p5hasstr(n->l) || p5hasstr(n->r);
    default: return p5hasstr(n->l);
    }
}

/* ------------------------------------------------ the folded constants */

static struct p5q p5qmake(int64_t n, int64_t d, BOOL ok)
{
    struct p5q r;
    int64_t a, b, t;

    r.ok = ok && d != 0 && n != INT64_MIN && d != INT64_MIN;
    r.n = 0;
    r.d = 1;
    if (!r.ok)
        return r;
    if (d < 0) {
        n = -n;
        d = -d;
    }
    a = n < 0 ? -n : n;
    b = d;
    while (b) {
        t = a % b;
        a = b;
        b = t;
    }
    if (!a)
        a = 1;
    r.n = n / a;
    r.d = d / a;
    return r;
}

static struct p5q p5qint(int64_t v)
{
    return p5qmake(v, 1, TRUE);
}

static int64_t p5qfloor(struct p5q a)
{
    int64_t q = a.n / a.d;

    if (a.n % a.d && a.n < 0)
        q--;
    return q;
}

static struct p5q p5qop(struct p5q a, struct p5q b, unsigned op)
{
    int64_t x, y, n, d;

    if (!a.ok || !b.ok)
        return p5qmake(0, 1, FALSE);
    switch (op) {
    case ETO_ADD:
    case ETO_SUB:
        if (__builtin_mul_overflow(a.n, b.d, &x) || __builtin_mul_overflow(b.n, a.d, &y)
            || __builtin_mul_overflow(a.d, b.d, &d)
            || (op == ETO_ADD ? __builtin_add_overflow(x, y, &n) : __builtin_sub_overflow(x, y, &n)))
            return p5qmake(0, 1, FALSE);
        return p5qmake(n, d, TRUE);
    case ETO_MUL:
        if (__builtin_mul_overflow(a.n, b.n, &n) || __builtin_mul_overflow(a.d, b.d, &d))
            return p5qmake(0, 1, FALSE);
        return p5qmake(n, d, TRUE);
    default:                        /* ETO_DIV, b not 0 */
        if (__builtin_mul_overflow(a.n, b.d, &n) || __builtin_mul_overflow(a.d, b.n, &d))
            return p5qmake(0, 1, FALSE);
        return p5qmake(n, d, TRUE);
    }
}

/* the value of the constant subtree n as the pass folds it: + - * / exact,
   Mod by today's sign rule (domodlong: the divisor added to a negative
   remainder), the bit operators and shifts on the whole parts (rounded
   down), a comparison -1 or 0 */
static struct p5q p5qval(const struct exprtyping_node *n)
{
    struct p5q a, b, t;
    int64_t x, y;

    if (n->kind == ETN_CONST)
        return p5qmake(n->num, n->den, TRUE);
    if (n->kind == ETN_NEG)
        return p5qop(p5qint(0), p5qval(n->l), ETO_SUB);
    if (n->kind != ETN_BIN)
        return p5qmake(0, 1, FALSE);
    a = p5qval(n->l);
    b = p5qval(n->r);
    if (!a.ok || !b.ok)
        return p5qmake(0, 1, FALSE);
    if (exprtype_iscompare(n->op)) {
        int s;
        BOOL r;

        t = p5qop(a, b, ETO_SUB);
        if (!t.ok)
            return t;
        s = t.n > 0 ? 1 : t.n < 0 ? -1 : 0;
        switch (n->op) {
        case ETO_EQ: r = s == 0; break;
        case ETO_NE: r = s != 0; break;
        case ETO_LT: r = s < 0; break;
        case ETO_LE: r = s <= 0; break;
        case ETO_GT: r = s > 0; break;
        default: r = s >= 0; break;
        }
        return p5qint(r ? -1 : 0);
    }
    switch (n->op) {
    case ETO_ADD:
    case ETO_SUB:
    case ETO_MUL: return p5qop(a, b, n->op);
    case ETO_DIV: return b.n ? p5qop(a, b, ETO_DIV) : p5qint(0);
    case ETO_MOD:
        if (!b.n)
            return a;
        t = p5qop(a, b, ETO_DIV);
        if (!t.ok)
            return t;
        t = p5qop(a, p5qop(b, p5qint(t.n / t.d), ETO_MUL), ETO_SUB);
        if (t.ok && t.n < 0)
            t = p5qop(t, b, ETO_ADD);
        return t;
    default: break;
    }
    x = p5qfloor(a);
    y = p5qfloor(b);
    switch (n->op) {
    case ETO_AND:
    case ETO_AMP: return p5qint(x & y);
    case ETO_OR:
    case ETO_BAR: return p5qint(x | y);
    case ETO_EOR: return p5qint(x ^ y);
    case ETO_LSL:
    case ETO_ASL: return p5qint(y >= 0 && y < 32 ? (int64_t)((uint64_t)x << y) : 0);
    case ETO_LSR:
    case ETO_ASR: return p5qint(y >= 0 && y < 32 ? x >> y : 0);
    default: return p5qmake(0, 1, FALSE);
    }
}

/* the folded constant p as its code writes it in the type E (bit: its
   whole part, rounded down; signext: a byte from 128 to 255 compared
   with a byte value, sign-extended, #978); FALSE: not here */
static BOOL p5encode(struct p5node *p, UWORD E, BOOL bit, ULONG *enc)
{
    const struct exprtyping_node *k;
    struct p5q v = p5qval(&p->t);
    int64_t x, m;
    int negs;

    if (!v.ok)
        return FALSE;
    if (bit)
        v = p5qint(p5qfloor(v));
    x = p5qfloor(v);
    switch (E) {
    case T_BYTE:
    case T_WORD:
        if ((p->flags & P5F_SIGNEXT) && x >= 128 && x <= 255)
            x -= 256;
        *enc = (UWORD)x;
        return TRUE;
    case T_LONG:
        *enc = (ULONG)x;
        return TRUE;
    case T_QUICK:
        if (v.d == 1) {
            *enc = (ULONG)((uint64_t)x << 16);
            return TRUE;
        }
        /* the magnitude's 16.16 value rounded down, negated (as a literal
           and its minus sign) */
        if (__builtin_mul_overflow(v.n < 0 ? -v.n : v.n, (int64_t)65536, &m))
            return FALSE;
        m /= v.d;
        *enc = (ULONG)(v.n < 0 ? -m : m);
        return TRUE;
    case T_FLOAT:
        k = p5literal(&p->t, &negs);
        if (k && k->point && !bit) {
            /* a literal: its FFP value, with its minus signs */
            *enc = P5(k)->ffp;
            if ((negs & 1) && *enc)
                *enc ^= 0x80;
            return TRUE;
        }
        if (v.d == 1 && x >= INT32_MIN && x <= INT32_MAX) {
            *enc = ffpcall(P5_SPFLT, (ULONG)(LONG)x, 0, NULL);
            return TRUE;
        }
        {
            int64_t n = v.n, d = v.d;

            while (n > INT32_MAX || n < -INT32_MAX || d > INT32_MAX) {
                n /= 2;
                d /= 2;
            }
            if (!d)
                return FALSE;
            *enc = ffpcall(P5_SPDIV, ffpcall(P5_SPFLT, (ULONG)(LONG)n, 0, NULL),
                           ffpcall(P5_SPFLT, (ULONG)(LONG)d, 0, NULL), NULL);
            return TRUE;
        }
    default: return FALSE;
    }
}

/* ------------------------------------------------------- the code */

static void p5w(UWORD w)
{
    if (!p5prep)
        pokewd(w);
}

static void p5l(ULONG l)
{
    if (!p5prep)
        pokel(l);
}

/* a register the pass's code needs */
static void p5reg(UWORD r)
{
    if (r > 7)
        conmemerr();                /* #1185: no register beyond d7 */
}

/* jsr lvo(a6) of mathffp */
static void p5ffp(WORD lvo)
{
    p5w(0x4eae);
    p5w((UWORD)lvo);
}

/* the 48.16 pair D(r)/D(r+1) to a Long in D(r), rounded down: its bits
   16 to 47 */
static void p5floor48(UWORD r)
{
    if (p5prep)
        return;
    p5reg(r + 1);
    p5w(0x4840 | (r + 1));                  /* swap D(r+1) */
    p5w(0x4840 | r);                        /* swap D(r) */
    p5w(0x3000 | ((r + 1) << 9) | r);       /* move.w D(r),D(r+1) */
    p5w(0x2000 | (r << 9) | (r + 1));       /* move.l D(r+1),D(r) */
}

/* the 48.16 pair D(r)/D(r+1) to a Float in D(r): high * 65536 + (low
   >> 1) * 2^-15 (the FFP value of both, rounded) */
static void p5q48tof(UWORD r)
{
    ULONG push, pull;
    UWORD s = regat;

    if (p5prep) {
        tokuse(getffpbase);
        return;
    }
    p5reg(r + 1);
    regat = r;
    savereg(&push, &pull);
    regat = s;
    pokemovem(push);
    tokejsr(getffpbase);
    p5w(0x2f00 | r);                        /* move.l D(r),-(a7): the low part */
    p5w(0x2000 | (r + 1));                  /* move.l D(r+1),d0 */
    p5ffp(P5_SPFLT);
    p5w(0x223c);                            /* move.l #65536.0,d1 */
    p5l(0x80000051);
    p5ffp(P5_SPMUL);
    p5w(0x221f);                            /* move.l (a7)+,d1 */
    p5w(0x2f00);                            /* move.l d0,-(a7) */
    p5w(0x2001);                            /* move.l d1,d0 */
    p5w(0xe288);                            /* lsr.l #1,d0 */
    p5ffp(P5_SPFLT);
    p5w(0x223c);                            /* move.l #2^-15,d1 */
    p5l(0x80000032);
    p5ffp(P5_SPMUL);
    p5w(0x221f);                            /* move.l (a7)+,d1 */
    p5ffp(P5_SPADD);
    if (r)
        p5w(0x2000 | (r << 9));             /* move.l d0,D(r) */
    pokemovem(pull);
}

/* the value in D(regat) of type dt (a 48.16 pair when df is ETF_Q48) to
   the type to; left: D(regat+1) holds the right operand (convtypef) */
static void p5conv(UWORD dt, UWORD df, UWORD to, BOOL left)
{
    if (df == ETF_Q48) {
        if (to == T_QUICK)
            return;                 /* D(regat): its 16.16 value */
        if (to == T_FLOAT) {
            p5q48tof(regat);
            return;
        }
        p5floor48(regat);
        dt = T_LONG;
    }
    if (!to || dt == to || (p5norm(dt) == T_LONG && p5norm(to) == T_LONG))
        return;
    if (p5prep)
        convprep(dt, to);
    else if (left)
        convtypef(dt, to);
    else
        convtype(dt, to);
}

/* the narrow value of type t in dn as the 48.16 pair dn/dn+1 */
static void p5widen(UWORD n, UWORD t)
{
    t = p5norm(t);
    if (t == T_BYTE || t == T_WORD) {
        p5w(0x48c0 | n);                    /* ext.l dn */
        t = T_LONG;
    }
    p5w(0x2000 | ((n + 1) << 9) | n);       /* move.l dn,dn+1 */
    if (t == T_LONG) {
        p5w(0x4840 | (n + 1));              /* swap dn+1 */
        p5w(0x48c0 | (n + 1));              /* ext.l dn+1: the whole part's high word */
        p5w(0x4840 | n);                    /* swap dn */
        p5w(0x4240 | n);                    /* clr.w dn */
    } else {                                /* a Quick: the sign */
        p5w(0xd080 | ((n + 1) << 9) | (n + 1));     /* add.l dn+1,dn+1 */
        p5w(0x9180 | ((n + 1) << 9) | (n + 1));     /* subx.l dn+1,dn+1 */
    }
}

/* the registers p5callin saved (movem.l, d0 bit 0) */
static UWORD p5saved;
/* p5callin's next call leaves the narrow operands as they are (not
   widened to 48.16): the caller's code takes them so */
static BOOL p5nowide;

/* the moves of the registers src[i] to d0-d3 (src[i] > 7: d(i) gets no
   value), in parallel: a move whose destination no other move still
   reads first, a cycle by exg */
static void p5moves(UWORD *src)
{
    UWORD i, j, n;

    for (;;) {
        for (n = i = 0; i < 4; i++)
            if (src[i] <= 7 && src[i] != i)
                n++;
        if (!n)
            return;
        for (i = 0; i < 4; i++) {
            if (src[i] > 7 || src[i] == i)
                continue;
            for (j = 0; j < 4 && !(j != i && src[j] == i); j++)
                ;
            if (j == 4)
                break;
        }
        if (i < 4) {
            p5w(0x2000 | (i << 9) | src[i]);        /* move.l Ds,Di */
            src[i] = i;
            continue;
        }
        /* a cycle: Di and its source exchanged, the move reading Di reads
           the source now */
        for (i = 0; i < 4 && (src[i] > 7 || src[i] == i); i++)
            ;
        p5w(0xc140 | (src[i] << 9) | i);            /* exg Ds,Di */
        for (j = 0; j < 4; j++)
            if (j != i && src[j] == i)
                src[j] = src[i];
        src[i] = i;
    }
}

/* a 48.16 routine tok on a in D(r) (of type at, a pair when aw) and b in
   D(r+1+aw) (of type bt, a pair when bw): the registers below D(r) among
   d0-d3 saved, a in d0/d1 and b in d2/d3 (swap: the other way), each
   widened to 48.16; trap: a divisor 0 raises the CPU's division trap
   (a Quick Mod, as today); tok 0: the caller writes the operation
   (inline, in d0-d3) */
static void p5callin(UWORD tok, UWORD r, UWORD at, BOOL aw, UWORD bt, BOOL bw, BOOL swap, BOOL trap)
{
    UWORD q = r + 1 + aw, k = r < 4 ? r : 4, i, mask = 0, src[4];
    UWORD xr = swap ? q : r, yr = swap ? r : q, xt = swap ? bt : at, yt = swap ? at : bt;
    BOOL xw = swap ? bw : aw, yw = swap ? aw : bw;
    BOOL nowide = p5nowide;

    p5nowide = FALSE;
    if (p5prep) {
        if (tok)
            tokuse(tok);
        return;
    }
    p5reg(q + bw);
    p5reg(r + 1);
    for (i = 0; i < k; i++)
        mask |= 0x8000 >> i;                /* predecrement: d0 is bit 15 */
    if (mask) {
        p5w(0x48e7);                        /* movem.l d0-..,-(a7) */
        p5w(mask);
    }
    p5saved = 0;
    for (i = 0; i < k; i++)
        p5saved |= 1 << i;
    /* the operands to d0-d3 (they may lie there already) */
    src[0] = xr;
    src[1] = xw ? xr + 1 : 8;
    src[2] = yr;
    src[3] = yw ? yr + 1 : 8;
    p5moves(src);
    if (!xw && !nowide)
        p5widen(0, xt);
    if (!yw && !nowide)
        p5widen(2, yt);
    if (trap) {
        p5w(0x4a82);                        /* tst.l d2 */
        p5w(0x6602);                        /* bne.s +2 */
        p5w(0x85c2);                        /* divs.w d2,d2: the trap */
    }
    if (tok)
        tokejsr(tok);
}

/* the routine's result to D(r) (pair: d0/d1 to D(r)/D(r+1), else d0),
   the saved registers back */
static void p5callout(UWORD r, BOOL pair)
{
    if (p5prep)
        return;
    if (pair) {
        if (r == 1) {
            p5w(0x2401);                    /* move.l d1,d2 */
            p5w(0x2200);                    /* move.l d0,d1 */
        } else if (r >= 2) {
            p5w(0x2000 | (r << 9));         /* move.l d0,D(r) */
            p5w(0x2001 | ((r + 1) << 9));   /* move.l d1,D(r+1) */
        }
    } else if (r)
        p5w(0x2000 | (r << 9));
    if (p5saved) {
        p5w(0x4cdf);                        /* movem.l (a7)+,d0-.. */
        p5w(p5saved);
    }
}

/* the operands of an operation: the right one in D(q) to rto, the left
   one in D(r) to lto, the right one to D(r+1) */
static void p5operands(struct p5node *L, struct p5node *R, UWORD r, UWORD q, UWORD lto, UWORD rto)
{
    regat = q;
    p5conv(R->dt, R->df, rto, FALSE);
    regat = r;
    p5conv(L->dt, L->df, lto, TRUE);
    if (q != r + 1)
        p5w(0x2000 | ((r + 1) << 9) | q);   /* move.l D(q),D(r+1) */
    regat = r;
}

static void p5doop(unsigned op, UWORD t)
{
    if (p5prep)
        opprep(op, t, FALSE);
    else
        doop(&operators[op], t, FALSE);
}

/* a Mod in Float, exact: a - b * trunc(a / b), plus b when negative; b
   0: a (D(r) a, D(r+1) b, the result in D(r)) */
static void p5fmod(UWORD r)
{
    ULONG push, pull;
    UWORD s = regat;

    if (p5prep) {
        tokuse(getffpbase);
        return;
    }
    regat = r;
    savereg(&push, &pull);
    regat = s;
    pokemovem(push);
    tokejsr(getffpbase);
    p5w(0x2f00 | (r + 1));                  /* move.l D(r+1),-(a7): b */
    p5w(0x2f00 | r);                        /* move.l D(r),-(a7): a */
    p5w(0x222f);                            /* move.l 4(a7),d1 */
    p5w(4);
    p5w(0x672c);                            /* beq.s .zero */
    p5w(0x2017);                            /* move.l (a7),d0 */
    p5ffp(P5_SPDIV);
    p5ffp(P5_SPFIX);
    p5ffp(P5_SPFLT);
    p5w(0x222f);                            /* move.l 4(a7),d1 */
    p5w(4);
    p5ffp(P5_SPMUL);
    p5w(0x2200);                            /* move.l d0,d1 */
    p5w(0x2017);                            /* move.l (a7),d0 */
    p5ffp(P5_SPSUB);
    p5w(0x4a00);                            /* tst.b d0: the sign */
    p5w(0x6a0c);                            /* bpl.s .done */
    p5w(0x222f);                            /* move.l 4(a7),d1 */
    p5w(4);
    p5ffp(P5_SPADD);
    p5w(0x6002);                            /* bra.s .done */
    p5w(0x2017);                            /* .zero: move.l (a7),d0 */
    p5w(0x508f);                            /* .done: addq.l #8,a7 */
    if (r)
        p5w(0x2000 | (r << 9));             /* move.l d0,D(r) */
    pokemovem(pull);
}

static int p5mixkind(const struct exprtyping_node *n);

/* the scratch registers of the inline 48.16 code at D(r) (p5fast): n
   registers other than D(r), D(r+1), from D(r+2) up, then below D(r);
   one below D(r) or beyond d5 kept on the stack around the code */
static UWORD p5scr[2], p5nscr;

static void p5scratch(UWORD r, UWORD n)
{
    UWORD k, i = 0;

    for (k = r + 2; k <= 7 && i < n; k++)
        p5scr[i++] = k;
    for (k = 0; k < r && i < n; k++)
        p5scr[i++] = k;
    p5nscr = n;
    for (k = 0; k < n; k++)
        if (p5scr[k] < r || p5scr[k] > 5)
            p5w(0x2f00 | p5scr[k]);             /* move.l Dk,-(a7) */
}

static void p5scratchend(UWORD r)
{
    UWORD k;

    for (k = p5nscr; k--;)
        if (p5scr[k] < r || p5scr[k] > 5)
            p5w(0x201f | (p5scr[k] << 9));      /* move.l (a7)+,Dk */
}

/* #1221 (user decision of 2026-11-26, "Quick rounding: down stays; the
   48.16 cut in the final direction (E)"): the cut at 1/65536 of the 48.16
   operation p, a * or /, rounds down: under a wanted Byte, Word or Long
   (a / under a wanted Long, and one at the top under a wanted Byte or Word,
   P5F_LDIV (#1222), is a whole number towards zero instead) */
static BOOL p5down(const struct p5node *p)
{
    if (p->flags & P5F_LDIV)
        return FALSE;
    return p->want == T_BYTE || p->want == T_WORD || (p->want == T_LONG && p->t.op == ETO_MUL);
}

/* task 4.3 (decision "expr-tree-5 4.3: fast path for a Quick with a
   constant divisor/factor", "the Word fast path q*w"): a 48.16 operation
   of a 16.16 value A (in D(r)) whose value p5fastmark knows to be what
   q48div or q48mul gives, written inline:
   - P5F_NDIV, A / d (d whole, 2 <= |d| <= 65535, p->whole = |d|, p->ffp:
     d < 0): q48div's (|A| << 16) / (|d| << 16) = |A| / |d| rounded down by
     two DIVU, the sign applied; fits 16.16, the result in D(r) alone;
     under a wanted Long or at the top (P5F_LDIV) cut towards zero to a
     whole number (part 3) as the 48.16 code does;
   - P5F_NMULF, A * X (X a constant's 16.16 value, |X| < 65536, in the
     other operand's register): q48mul's (|A| * |X|) >> 16 by two MULU,
     the sign applied; fits 16.16, in D(r) alone;
   - P5F_WMUL, A * w (w a Word, Byte or whole constant of the Word range,
     the low word of its register as the 48.16 code widens it): q48mul's
     |A| * |w| exactly (no fraction is cut) by two MULU, as 48.16 in
     D(r)/D(r+1), the sign applied in 64 bits.
   The host test test_exprarith.cpp ("fast paths") checks the same
   steps against exprarith_q48. */
static void p5fast(struct p5node *p, UWORD r)
{
    UWORD a = r, x = r + 1, t, u;

    if (p5prep)
        return;
    if (p->flags & P5F_NDIV) {
        p5scratch(r, 1);
        t = p5scr[0];
        p5w(0x2000 | (x << 9) | a);             /* move.l Da,Dx: |A| in Dx */
        p5w(0x6a02);                            /* bpl.s */
        p5w(0x4480 | x);                        /* neg.l Dx */
        p5w(0x4840 | x);                        /* swap Dx */
        p5w(0x7000 | (t << 9));                 /* moveq #0,Dt */
        p5w(0x3000 | (t << 9) | x);             /* move.w Dx,Dt: the high word */
        p5w(0x80fc | (t << 9));                 /* divu.w #d,Dt */
        p5w((UWORD)p->whole);
        p5w(0x3000 | (x << 9) | t);             /* move.w Dt,Dx */
        p5w(0x4840 | x);                        /* swap Dx: q1, the low word */
        p5w(0x3000 | (t << 9) | x);             /* move.w Dx,Dt: the remainder and the low word */
        p5w(0x80fc | (t << 9));                 /* divu.w #d,Dt */
        p5w((UWORD)p->whole);
        p5w(0x3000 | (x << 9) | t);             /* move.w Dt,Dx: q1:q2 */
        if (p5down(p)) {
            /* #1221: under a wanted Word or Byte cut down, a negative
               quotient with a remainder one 1/65536 lower */
            p5w(0x4a80 | a);                    /* tst.l Da */
            p5w(p->ffp ? 0x6b0a : 0x6a0a);      /* bmi.s / bpl.s: not negative */
            p5w(0x4840 | t);                    /* swap Dt */
            p5w(0x4a40 | t);                    /* tst.w Dt: the remainder */
            p5w(0x6702);                        /* beq.s */
            p5w(0x5280 | x);                    /* addq.l #1,Dx */
            p5w(0x4480 | x);                    /* neg.l Dx */
            p5scratchend(r);
            p5w(0x2000 | (a << 9) | x);         /* move.l Dx,Da */
            return;
        }
        p5scratchend(r);
        p5w(0x4a80 | a);                        /* tst.l Da */
        p5w(p->ffp ? 0x6b02 : 0x6a02);          /* bmi.s / bpl.s: the sign */
        p5w(0x4480 | x);                        /* neg.l Dx */
        p5w(0x2000 | (a << 9) | x);             /* move.l Dx,Da */
        if (p->want == T_LONG || (p->flags & P5F_LDIV)) {
            p5w(0x6a0a);                        /* bpl.s .cut */
            p5w(0x4a40 | a);                    /* tst.w Da */
            p5w(0x6706);                        /* beq.s .cut */
            p5w(0x0680 | a);                    /* addi.l #$10000,Da */
            p5l(0x00010000);
            p5w(0x4240 | a);                    /* .cut: clr.w Da */
        }
        return;
    }
    if (p->ffp & 1)
        p5w(0xc140 | (a << 9) | x);             /* exg Da,Dx: A in Da */
    p5scratch(r, 2);
    t = p5scr[0];
    u = p5scr[1];
    if (p->flags & P5F_NMULF) {
        p5w(0x2000 | (u << 9) | a);             /* move.l Da,Du */
        p5w(0xb180 | (x << 9) | u);             /* eor.l Dx,Du: the sign */
        p5w(0x4a80 | a);                        /* tst.l Da */
        p5w(0x6a02);                            /* bpl.s */
        p5w(0x4480 | a);                        /* neg.l Da */
        p5w(0x4a80 | x);                        /* tst.l Dx */
        p5w(0x6a02);                            /* bpl.s */
        p5w(0x4480 | x);                        /* neg.l Dx: |X| < 65536 */
        p5w(0x3000 | (t << 9) | a);             /* move.w Da,Dt */
        p5w(0xc0c0 | (t << 9) | x);             /* mulu.w Dx,Dt: al * |X| */
        if (p5down(p)) {
            /* #1221: under a wanted whole number cut down, a negative
               product with cut bits one 1/65536 lower */
            p5w(0x4840 | a);                    /* swap Da */
            p5w(0xc0c0 | (a << 9) | x);         /* mulu.w Dx,Da: ah * |X| */
            p5w(0x3000 | (x << 9) | t);         /* move.w Dt,Dx: the cut bits */
            p5w(0x4240 | t);                    /* clr.w Dt */
            p5w(0x4840 | t);                    /* swap Dt: >> 16 */
            p5w(0xd080 | (a << 9) | t);         /* add.l Dt,Da */
            p5w(0x4a80 | u);                    /* tst.l Du */
            p5w(0x6a08);                        /* bpl.s */
            p5w(0x4480 | a);                    /* neg.l Da */
            p5w(0x4a40 | x);                    /* tst.w Dx */
            p5w(0x6702);                        /* beq.s */
            p5w(0x5380 | a);                    /* subq.l #1,Da */
            p5scratchend(r);
            return;
        }
        p5w(0x4240 | t);                        /* clr.w Dt */
        p5w(0x4840 | t);                        /* swap Dt: >> 16 */
        p5w(0x4840 | a);                        /* swap Da */
        p5w(0xc0c0 | (a << 9) | x);             /* mulu.w Dx,Da: ah * |X| */
        p5w(0xd080 | (a << 9) | t);             /* add.l Dt,Da */
        p5w(0x4a80 | u);                        /* tst.l Du */
        p5w(0x6a02);                            /* bpl.s */
        p5w(0x4480 | a);                        /* neg.l Da */
        p5scratchend(r);
        return;
    }
    /* P5F_WMUL */
    p5w(0x2000 | (u << 9) | a);                 /* move.l Da,Du */
    p5w(0x4840 | u);                            /* swap Du */
    p5w(0xb140 | (x << 9) | u);                 /* eor.w Dx,Du: the sign */
    p5w(0x4a80 | a);                            /* tst.l Da */
    p5w(0x6a02);                                /* bpl.s */
    p5w(0x4480 | a);                            /* neg.l Da */
    p5w(0x4a40 | x);                            /* tst.w Dx */
    p5w(0x6a02);                                /* bpl.s */
    p5w(0x4440 | x);                            /* neg.w Dx: |w| */
    p5w(0x2000 | (t << 9) | a);                 /* move.l Da,Dt */
    p5w(0x4840 | t);                            /* swap Dt */
    p5w(0xc0c0 | (t << 9) | x);                 /* mulu.w Dx,Dt: ah * |w| */
    p5w(0xc0c0 | (a << 9) | x);                 /* mulu.w Dx,Da: al * |w| */
    p5w(0x2000 | (x << 9) | t);                 /* move.l Dt,Dx */
    p5w(0x4240 | x);                            /* clr.w Dx */
    p5w(0x4840 | x);                            /* swap Dx: the high part */
    p5w(0x4840 | t);                            /* swap Dt */
    p5w(0x4240 | t);                            /* clr.w Dt: << 16 */
    p5w(0xd080 | (a << 9) | t);                 /* add.l Dt,Da: the low part */
    p5w(0x7000 | (t << 9));                     /* moveq #0,Dt */
    p5w(0xd180 | (x << 9) | t);                 /* addx.l Dt,Dx: the carry */
    p5w(0x4a40 | u);                            /* tst.w Du */
    p5w(0x6a04);                                /* bpl.s */
    p5w(0x4480 | a);                            /* neg.l Da */
    p5w(0x4080 | x);                            /* negx.l Dx */
    p5scratchend(r);
}

/* a 16.16 operand of p5fastmark: not a constant, held as a Quick */
static BOOL p5fastq(const struct p5node *k)
{
    return !p5folded(&k->t) && p5mixkind(&k->t) == 2 && k->dt == T_QUICK && k->df == ETF_TYPE;
}

static void p5nopall(struct p5node *p, struct p5node *keep);

/* the fast paths of p5fast, decided when the operands are applied */
static void p5fastmark(struct p5node *p)
{
    struct p5node *L = P5(p->t.l), *R = P5(p->t.r), *Q, *O;
    struct p5q v;

    if (p->t.o.path == ETY_Q16 && p->t.op == ETO_DIV && (p->flags & P5F_LDIV)) {
        /* p5qdivt: a whole constant divisor of the Word range (#1222) */
        if (!p5fastq(L) || !p5folded(&R->t) || R->df != ETF_TYPE)
            return;
        v = p5qval(&R->t);
        if (!v.ok || v.d != 1 || v.n < -32768 || v.n > 32767 || (v.n > -2 && v.n < 2))
            return;
        p->flags |= P5F_NDIV;
        p->whole = (ULONG)v.n;
        if (!(R->flags & P5F_KEEP) || R->t.kind == ETN_CONST)
            p5nopall(R, NULL);
        return;
    }
    if (p->t.o.path != ETY_Q48)
        return;
    if (p->t.op == ETO_DIV) {
        if (!p5fastq(L) || !p5folded(&R->t) || R->df != ETF_TYPE
            || (R->dt != T_BYTE && R->dt != T_WORD && R->dt != T_LONG))
            return;
        v = p5qval(&R->t);
        if (!v.ok || v.d != 1 || v.n < -65535 || v.n > 65535 || (v.n > -2 && v.n < 2))
            return;
        p->flags |= P5F_NDIV;
        p->whole = (ULONG)(v.n < 0 ? -v.n : v.n);
        p->ffp = v.n < 0;
        /* the divisor is an immediate of the DIVU: its load is dead (a
           subtree of the build's code only when it is one constant) */
        if (!(R->flags & P5F_KEEP) || R->t.kind == ETN_CONST)
            p5nopall(R, NULL);
        return;
    }
    if (p->t.op != ETO_MUL)
        return;
    if (p5fastq(L) && !p5fastq(R)) {
        Q = L;
        O = R;
    } else if (p5fastq(R) && !p5fastq(L)) {
        Q = R;
        O = L;
    } else
        return;
    if (O->df != ETF_TYPE)
        return;
    p->ffp = Q == R;                            /* the 16.16 value on the right */
    if (p5folded(&O->t)) {
        v = p5qval(&O->t);
        if (!v.ok)
            return;
        if (v.d == 1) {
            if (v.n >= -32768 && v.n <= 32767 && (O->dt == T_BYTE || O->dt == T_WORD || O->dt == T_LONG))
                p->flags |= P5F_WMUL;
            return;
        }
        /* below 1, its 16.16 value below 65536 whatever its rounding */
        if (O->dt == T_QUICK && (v.n < 0 ? -v.n : v.n) * 65536 < 65535 * v.d)
            p->flags |= P5F_NMULF;
        return;
    }
    if (O->t.typed && O->t.fmt == ETF_TYPE && (O->t.t == ET_WORD || O->t.t == ET_BYTE)
        && (O->dt == T_WORD || O->dt == T_BYTE))
        p->flags |= P5F_WMUL;
}

/* a branch whose displacement p5patch writes when its target is known */
static ULONG p5bra(UWORD op)
{
    ULONG at = (pc + 1) & ~1UL;

    p5w(op);
    return at;
}

static void p5patch(ULONG at)
{
    if (at + 2 <= libat && !nomemleft)
        W_(at) = (UWORD)((W_(at) & 0xff00) | ((pc - at - 2) & 0xff));
}

/* the 48.16 value d0/d1 to a Long in d0 towards zero (exprarith_q48_trunc,
   q48trunc's code) */
static void p5truncd0(void)
{
    p5w(0x4a81);                                /* tst.l d1 */
    p5w(0x6a0c);                                /* bpl.s .floor */
    p5w(0x4a40);                                /* tst.w d0: a fraction */
    p5w(0x6708);                                /* beq.s .floor */
    p5w(0x3001);                                /* move.w d1,d0 */
    p5w(0x4840);                                /* swap d0 */
    p5w(0x5280);                                /* addq.l #1,d0: the floor + 1 */
    p5w(0x6004);                                /* bra.s .done */
    p5w(0x3001);                                /* .floor: move.w d1,d0 */
    p5w(0x4840);                                /* swap d0: bits 16..47 */
}

/* task 4.3 (decision "expr-tree-5 4.3: a whole Quick divisor at run
   time"; the speed review, "the candidates and the rest", 2026-11-26):
   the 48.16 division of a by a 16.16 value b (in d2, its high word the
   whole part m) in p5callin's frame: when b is whole and not 0 (its low
   word 0), q48div's (|a| << 16) / (|m| << 16) is |a| / |m| rounded down
   (|m| <= 32768), one DIVU per 16-bit digit of |a| (each digit's
   quotient fits a word: the remainder above it is below |m|), a leading
   zero digit skipped (its quotient and remainder are 0), the sign of a
   eor b applied; a fraction or 0 calls the routine tok (its division by
   0, its error check) on the operands widened to 48.16.  longa: a is a
   whole number of type at in d0 and b in d2, neither widened (p5callin's
   p5nowide; a Byte or Word extended to a Long first): |a| << 16 has the
   digits hi(|a|), lo(|a|) and 0; else a is a 48.16 value in d0/d1 (four
   digits) and b is widened in d2/d3.  ldiv: the division at the top
   under a wanted Byte, Word or Long (P5F_LDIV): its value is the quotient
   towards zero (part 3, #1222) modulo 2^32 as a Long in d0, the fraction
   digit not computed (after tok: p5truncd0); a divisor with a fraction
   calls ftok instead of tok (at the top under a wanted Word or Byte the
   cut towards zero of q48div, tok's q48divw or q48divb only for the
   division by 0), and with raw (a dividend a of at most 16.16 bits: a
   Word, Byte or Quick, not a Long) longdiv on the 16.16 bits of a and b,
   whose quotient towards zero is that of a / b (p5qdivt).  down: under a
   wanted Word or Byte the cut rounds down (#1221, p5divsign; tok is then
   q48divw or q48divb, which cut down too).  The host test
   test_exprarith.cpp ("a whole divisor at run time") runs the same steps
   against exprarith_q48 and exprarith_q48_down. */
/* the sign of a 48.16 quotient d1:d0 (its magnitude) applied when the
   long at (a7)+ (stack) or in d2 is negative; down (#1221): a remainder
   in d3's high word not 0 makes it one 1/65536 lower */
static void p5divsign(BOOL stack, BOOL down)
{
    p5w(stack ? 0x4a9f : 0x4a82);               /* tst.l (a7)+ / tst.l d2: the sign */
    if (!down) {
        p5w(0x6a04);                            /* bpl.s */
        p5w(0x4480);                            /* neg.l d0 */
        p5w(0x4081);                            /* negx.l d1 */
        return;
    }
    p5w(0x6a10);                                /* bpl.s .done */
    p5w(0x4243);                                /* clr.w d3 */
    p5w(0x4a83);                                /* tst.l d3: the remainder */
    p5w(0x6706);                                /* beq.s */
    p5w(0x5280);                                /* addq.l #1,d0 */
    p5w(0x6402);                                /* bcc.s */
    p5w(0x5281);                                /* addq.l #1,d1 */
    p5w(0x4480);                                /* neg.l d0 */
    p5w(0x4081);                                /* negx.l d1 */
}

static void p5wholediv(UWORD tok, UWORD ftok, UWORD at, BOOL longa, BOOL ldiv, BOOL raw, BOOL down)
{
    ULONG frac, nul, done, done2 = 0, high;

    if (longa && p5norm(at) != T_LONG)
        p5w(0x48c0);                            /* ext.l d0 */
    p5w(0x4a42);                                /* tst.w d2: a fraction */
    frac = p5bra(0x6600);                       /* bne.s .call */
    p5w(0x4a82);                                /* tst.l d2 */
    nul = p5bra(0x6700);                        /* beq.s .call */
    if (longa) {
        p5w(0x2200);                            /* move.l d0,d1 */
        p5w(0xb581);                            /* eor.l d2,d1: the sign */
        p5w(0x4a80);                            /* tst.l d0 */
        p5w(0x6a02);                            /* bpl.s */
        p5w(0x4480);                            /* neg.l d0: |a| */
        p5w(0x4842);                            /* swap d2 */
        p5w(0x4a42);                            /* tst.w d2 */
        p5w(0x6a02);                            /* bpl.s */
        p5w(0x4442);                            /* neg.w d2: |m| */
        p5w(0x7600);                            /* moveq #0,d3 */
        p5w(0x4840);                            /* swap d0 */
        p5w(0x3600);                            /* move.w d0,d3: 0:hi */
        p5w(0x6702);                            /* beq.s: 0 */
        p5w(0x86c2);                            /* divu.w d2,d3 */
        p5w(0x3003);                            /* move.w d3,d0 */
        p5w(0x4840);                            /* swap d0 */
        p5w(0x3600);                            /* move.w d0,d3: r:lo */
        p5w(0x86c2);                            /* divu.w d2,d3 */
        p5w(0x3003);                            /* move.w d3,d0: the whole quotient */
        if (ldiv) {
            p5w(0x4a81);                        /* tst.l d1 */
            p5w(0x6a02);                        /* bpl.s */
            p5w(0x4480);                        /* neg.l d0 */
        } else {
            p5w(0x4243);                        /* clr.w d3: r:0 */
            p5w(0x86c2);                        /* divu.w d2,d3: the fraction */
            p5w(0x2401);                        /* move.l d1,d2: the sign */
            p5w(0x7200);                        /* moveq #0,d1 */
            p5w(0x4840);                        /* swap d0 */
            p5w(0x3200);                        /* move.w d0,d1: the high word */
            p5w(0x3003);                        /* move.w d3,d0 */
            p5divsign(FALSE, down);
        }
    } else {
        p5w(0x2601);                            /* move.l d1,d3 */
        p5w(0xb583);                            /* eor.l d2,d3: the sign */
        p5w(0x2f03);                            /* move.l d3,-(a7) */
        p5w(0x4a81);                            /* tst.l d1 */
        p5w(0x6a04);                            /* bpl.s */
        p5w(0x4480);                            /* neg.l d0 */
        p5w(0x4081);                            /* negx.l d1: |a| */
        p5w(0x4842);                            /* swap d2 */
        p5w(0x4a42);                            /* tst.w d2 */
        p5w(0x6a02);                            /* bpl.s */
        p5w(0x4442);                            /* neg.w d2: |m| */
        p5w(0x7600);                            /* moveq #0,d3 */
        p5w(0x4a81);                            /* tst.l d1 */
        high = p5bra(0x6700);                   /* beq.s .low: the high digits 0 */
        p5w(0x4841);                            /* swap d1 */
        p5w(0x3601);                            /* move.w d1,d3: 0:w3 */
        p5w(0x6702);                            /* beq.s: 0 */
        p5w(0x86c2);                            /* divu.w d2,d3 */
        p5w(0x3203);                            /* move.w d3,d1 */
        p5w(0x4841);                            /* swap d1 */
        p5w(0x3601);                            /* move.w d1,d3: r:w2 */
        p5w(0x86c2);                            /* divu.w d2,d3 */
        p5w(0x3203);                            /* move.w d3,d1 */
        p5patch(high);
        p5w(0x4840);                            /* .low: swap d0 */
        p5w(0x3600);                            /* move.w d0,d3: r:w1 */
        p5w(0x86c2);                            /* divu.w d2,d3 */
        p5w(0x3003);                            /* move.w d3,d0 */
        p5w(0x4840);                            /* swap d0 */
        if (ldiv) {
            p5w(0x3001);                        /* move.w d1,d0 */
            p5w(0x4840);                        /* swap d0: the whole quotient's low 32 bits */
            p5w(0x4a9f);                        /* tst.l (a7)+: the sign */
            p5w(0x6a02);                        /* bpl.s */
            p5w(0x4480);                        /* neg.l d0 */
        } else {
            p5w(0x3600);                        /* move.w d0,d3: r:w0 */
            p5w(0x86c2);                        /* divu.w d2,d3 */
            p5w(0x3003);                        /* move.w d3,d0 */
            p5divsign(TRUE, down);
        }
    }
    done = p5bra(0x6000);                       /* bra.s .done */
    p5patch(frac);
    if (ldiv && raw) {
        /* #1222: a / b towards zero, the 16.16 bits divided */
        if (longa) {
            p5w(0x4840);                        /* swap d0 */
            p5w(0x4240);                        /* clr.w d0: a's 16.16 bits */
        }
        p5w(0x2202);                            /* move.l d2,d1 */
        tokejsr(longdiv);
        done2 = p5bra(0x6000);                  /* bra.s .done */
    } else if (ftok != tok) {
        if (longa) {
            p5widen(0, T_LONG);                 /* the operands as 48.16 */
            p5widen(2, T_QUICK);
        }
        tokejsr(ftok);
        p5truncd0();
        done2 = p5bra(0x6000);                  /* bra.s .done */
    }
    p5patch(nul);
    if (longa) {
        p5widen(0, T_LONG);                     /* .call: the operands as 48.16 */
        p5widen(2, T_QUICK);
    }
    tokejsr(tok);
    if (ldiv)
        p5truncd0();
    p5patch(done);
    if (done2)
        p5patch(done2);
}

/* the follow-up of task 4.3 (decision "expr-tree-5 4.3 follow-up: a whole
   Quick divisor for Mod"; the speed review, 2026-11-26): q48mod's exact
   Mod of a and b, a - trunc(a / b) * b plus b when negative
   (exprarith_q48's rule), for a whole b = m << 16 (m not 0, |m| <= 32768)
   is the remainder of |a|'s whole part by |m| with |a|'s fraction below
   it, a's sign applied, b added when that is negative.  p5wholemod: a
   dividend a by a 16.16 divisor b (d2) in p5callin's frame, b tested at
   run time: one DIVU per 16-bit digit of the whole part (each digit's
   quotient fits a word), a leading zero digit skipped; a fraction or 0
   calls the routine tok (its Mod by 0: the dividend) on the operands
   widened to 48.16.  longa: a is a whole number of type at in d0 and b in
   d2, neither widened (p5nowide; a Byte or Word extended to a Long
   first): the whole part's digits hi(|a|), lo(|a|), no fraction; a's
   sign kept in the high word of d3 (DIVU reads its low word); else a is a
   48.16 value in d0/d1 (the whole part's three digits) and b is widened
   in d2/d3.  The result in d0/d1.  The host test test_exprarith.cpp ("Mod
   by a whole divisor") runs the same steps against exprarith_q48. */
static void p5wholemod(UWORD tok, UWORD at, BOOL longa)
{
    ULONG frac, nul, pos, rzero, done, high = 0;
    static const UWORD lng[] = {
        0x2602,                 /* move.l d2,d3 */
        0x6a02,                 /* bpl.s */
        0x4483,                 /* neg.l d3: |m|:0 */
        0x2200,                 /* move.l d0,d1 */
        0x6a02,                 /* bpl.s */
        0x4481,                 /* neg.l d1: |a| */
        0x4840,                 /* swap d0 */
        0x3600,                 /* move.w d0,d3 */
        0x4843,                 /* swap d3: a's high word (its sign) : |m| */
        0x7000,                 /* moveq #0,d0 */
        0x4841,                 /* swap d1 */
        0x3001,                 /* move.w d1,d0: 0:hi */
        0x6702,                 /* beq.s: 0 */
        0x80c3,                 /* divu.w d3,d0 */
        0x4841,                 /* swap d1 */
        0x3001,                 /* move.w d1,d0: r:lo */
        0x80c3,                 /* divu.w d3,d0 */
        0x4240,                 /* clr.w d0: |a| Mod |b| */
        0x7200,                 /* moveq #0,d1 */
        0x4a83,                 /* tst.l d3: the sign of a */
    }, digits[] = {
        0x4841,                 /* swap d1 */
        0x3401,                 /* move.w d1,d2: 0:w2 */
        0x6702,                 /* beq.s: 0 */
        0x84c3,                 /* divu.w d3,d2 */
        0x4841,                 /* swap d1 */
        0x3401,                 /* move.w d1,d2: r:w1 */
        0x84c3,                 /* divu.w d3,d2 */
    }, low[] = {
        0x4840,                 /* .low: swap d0 */
        0x3400,                 /* move.w d0,d2: r:w0 */
        0x84c3,                 /* divu.w d3,d2 */
        0x4840,                 /* swap d0 */
        0x3400,                 /* move.w d0,d2: r:fraction, |a| Mod |b| */
        0x7200,                 /* moveq #0,d1 */
        0x2002,                 /* move.l d2,d0 */
        0x241f,                 /* move.l (a7)+,d2: b */
        0x4a9f,                 /* tst.l (a7)+: the sign of a */
    };
    UWORD i;

    if (longa && p5norm(at) != T_LONG)
        p5w(0x48c0);                            /* ext.l d0 */
    p5w(0x4a42);                                /* tst.w d2: a fraction */
    frac = p5bra(0x6600);                       /* bne.s .call */
    p5w(0x4a82);                                /* tst.l d2 */
    nul = p5bra(0x6700);                        /* beq.s .call */
    if (longa) {
        for (i = 0; i < sizeof lng / sizeof lng[0]; i++)
            p5w(lng[i]);
        pos = p5bra(0x6a00);                    /* bpl.s .done */
        p5w(0x4480);                            /* neg.l d0 */
        rzero = p5bra(0x6a00);                  /* bpl.s .done: 0 */
        p5w(0x72ff);                            /* moveq #-1,d1 */
    } else {
        p5w(0x2f01);                            /* move.l d1,-(a7): the sign of a */
        p5w(0x6a04);                            /* bpl.s */
        p5w(0x4480);                            /* neg.l d0 */
        p5w(0x4081);                            /* negx.l d1: |a| */
        p5w(0x2602);                            /* move.l d2,d3 */
        p5w(0x6a02);                            /* bpl.s */
        p5w(0x4483);                            /* neg.l d3 */
        p5w(0x4843);                            /* swap d3: |m| */
        p5w(0x2f02);                            /* move.l d2,-(a7): b */
        p5w(0x7400);                            /* moveq #0,d2 */
        p5w(0x4a81);                            /* tst.l d1 */
        high = p5bra(0x6700);                   /* beq.s .low: the high digits 0 */
        for (i = 0; i < sizeof digits / sizeof digits[0]; i++)
            p5w(digits[i]);
        p5patch(high);
        for (i = 0; i < sizeof low / sizeof low[0]; i++)
            p5w(low[i]);
        pos = p5bra(0x6a00);                    /* bpl.s .done */
        p5w(0x4480);                            /* neg.l d0 */
        p5w(0x4081);                            /* negx.l d1 */
        rzero = p5bra(0x6a00);                  /* bpl.s .done: 0 */
    }
    p5w(0x7600);                                /* moveq #0,d3 */
    p5w(0x4a82);                                /* tst.l d2 */
    p5w(0x6a02);                                /* bpl.s */
    p5w(0x76ff);                                /* moveq #-1,d3 */
    p5w(0xd082);                                /* add.l d2,d0 */
    p5w(0xd383);                                /* addx.l d3,d1: + b */
    done = p5bra(0x6000);                       /* bra.s .done */
    p5patch(frac);
    p5patch(nul);
    if (longa) {
        p5widen(0, T_LONG);                     /* .call: the operands as 48.16 */
        p5widen(2, T_QUICK);
    }
    tokejsr(tok);                               /* .call */
    p5patch(pos);
    p5patch(rzero);
    p5patch(done);
}

/* the Mod of two 16.16 values a in D(r) and b in D(r+1) (the exact Mod,
   p5bin) for a whole b as p5wholemod: one DIVU of |a|'s whole part (at
   most 32768) by |m|, the result in D(r).  cst: b is the constant m << 16
   (m not 0), no test; else b is tested at run time: a fraction calls
   compilerlib's q16mod (the exact 16.16 Mod, the speed review), 0 its trap
   (divs.w by it, before the call) */
static void p5qmod(UWORD r, BOOL cst, LONG m)
{
    UWORD a = r, b = r + 1, t, u;
    ULONG frac = 0, nul = 0, pos, rzero, done;

    if (!cst) {
        p5w(0x4a40 | b);                        /* tst.w Db: a fraction */
        frac = p5bra(0x6600);                   /* bne.s .call */
        p5w(0x4a80 | b);                        /* tst.l Db */
        nul = p5bra(0x6700);                    /* beq.s .call */
    }
    p5scratch(r, cst ? 1 : 2);
    t = p5scr[0];
    u = cst ? b : p5scr[1];
    if (!cst) {
        p5w(0x2000 | (u << 9) | b);             /* move.l Db,Du */
        p5w(0x6a02);                            /* bpl.s */
        p5w(0x4480 | u);                        /* neg.l Du: |m|:0 */
    }
    p5w(0x2000 | (t << 9) | a);                 /* move.l Da,Dt */
    p5w(0x6a02);                                /* bpl.s */
    p5w(0x4480 | t);                            /* neg.l Dt: |a| */
    p5w(0x3000 | (u << 9) | t);                 /* move.w Dt,Du: the fraction */
    if (!cst)
        p5w(0x4840 | u);                        /* swap Du: |m| in the low word */
    p5w(0x4240 | t);                            /* clr.w Dt */
    p5w(0x4840 | t);                            /* swap Dt: the whole part */
    if (cst) {
        p5w(0x80fc | (t << 9));                 /* divu.w #|m|,Dt */
        p5w((UWORD)(m < 0 ? -m : m));
    } else {
        p5w(0x80c0 | (t << 9) | u);             /* divu.w Du,Dt */
        p5w(0x4840 | u);                        /* swap Du: the fraction */
    }
    p5w(0x3000 | (t << 9) | u);                 /* move.w Du,Dt: the remainder, the fraction */
    p5w(0x4a80 | a);                            /* tst.l Da */
    pos = p5bra(0x6a00);                        /* bpl.s .pos */
    p5w(0x4480 | t);                            /* neg.l Dt */
    rzero = p5bra(0x6a00);                       /* bpl.s .pos: 0 */
    if (cst) {
        p5w(0x0680 | t);                        /* addi.l #b,Dt */
        p5l((ULONG)m << 16);
    } else
        p5w(0xd080 | (t << 9) | b);             /* add.l Db,Dt */
    p5patch(pos);
    p5patch(rzero);
    p5w(0x2000 | (a << 9) | t);                 /* .pos: move.l Dt,Da */
    p5scratchend(r);
    if (cst)
        return;
    done = p5bra(0x6000);                       /* bra.s .done */
    p5patch(frac);
    p5patch(nul);
    p5w(0x4a80 | b);                            /* .call: tst.l Db */
    p5w(0x6602);                                /* bne.s +2 */
    p5w(0x81c0 | (b << 9) | b);                 /* divs.w Db,Db: the trap of a Mod by 0 */
    domylib(q16mod);
    p5patch(done);
}

/* the divisor of a 16.16 Mod (p5qmod) that is a whole constant m, not 0,
   that 16.16 takes exactly */
static BOOL p5modconst(const struct p5node *R, LONG *m)
{
    struct p5q v;

    if (!p5folded(&R->t) || R->df != ETF_TYPE
        || (R->dt != T_BYTE && R->dt != T_WORD && R->dt != T_LONG && R->dt != T_QUICK))
        return FALSE;
    v = p5qval(&R->t);
    if (!v.ok || v.d != 1 || v.n < -32768 || v.n > 32767 || !v.n)
        return FALSE;
    *m = (LONG)v.n;
    return TRUE;
}

/* a constant divisor of a Mod that p5modconst does not take: the
   routine without the run-time test */
static BOOL p5modfrac(const struct p5node *R)
{
    LONG m;

    return p5folded(&R->t) && !p5modconst(R, &m);
}

/* a whole number in one register (p5bin's type of an operand) */
static BOOL p5whole1(UWORD t)
{
    return t == T_LONG || t == T_WORD || t == T_BYTE;
}

/* #1222 (the user's decision of 2026-11-26, "P+ part 3 under Byte/Word
   (A1) and F2 under whole destinations (option 1)"): a 16.16 / at the
   top under a wanted Byte, Word or Long (P5F_LDIV, path ETY_Q16: no Long
   in the expression) gives its quotient towards zero as a Long in D(r).
   With a and b the 16.16 bits of the operands, trunc(a / b) is the Long
   division a / b towards zero (both are scaled by 65536): longdiv on
   them, no rounding step.  A divisor without a fraction (also 0) calls
   quickdiv as before (its two DIVU, its value for a division by 0,
   $7FFFFFFF with the dividend's sign, and its error check), then the
   16.16 value towards zero.  Where the 16.16 quotient of a divisor with a
   fraction overflows (|a / b| >= 32768) the value is the exact quotient's
   low bits, as on the 48.16 path, not those of qdivide's wrapped 16.16
   value.  P5F_NDIV here (a whole constant divisor k of the Word range,
   |k| >= 2, p->whole = k; p5fastmark): the dividend towards zero, then
   DIVS.W #k, whose quotient fits a Word.  The host test test_exprarith.cpp
   ("a 16.16 division at the top towards zero") runs the same steps
   against exprarith_q16 and the cut towards zero. */
static void p5qdivt(struct p5node *p, UWORD r, UWORD q)
{
    struct p5node *L = P5(p->t.l), *R = P5(p->t.r);
    ULONG push, pull, whole, done;

    if (p->flags & P5F_NDIV) {
        p5w(0x4a80 | r);                        /* tst.l D(r) */
        p5w(0x6a06);                            /* bpl.s */
        p5w(0x0680 | r);                        /* addi.l #$FFFF,D(r): towards zero */
        p5l(0x0000ffff);
        p5w(0x4840 | r);                        /* swap D(r) */
        p5w(0x48c0 | r);                        /* ext.l D(r): the whole part */
        p5w(0x81fc | (r << 9));                 /* divs.w #k,D(r) */
        p5w((UWORD)p->whole);
        p5w(0x48c0 | r);                        /* ext.l D(r) */
        return;
    }
    p5operands(L, R, r, q, T_QUICK, T_QUICK);
    if (p5prep) {
        tokuse(longdiv);
        tokuse(quickdiv);
        return;
    }
    savereg(&push, &pull);
    pokemovem(push);
    ritetod0();
    p5w(0x4a41);                                /* tst.w d1: a fraction */
    whole = p5bra(0x6700);                      /* beq.s .whole */
    tokejsr(longdiv);
    done = p5bra(0x6000);                       /* bra.s .done */
    p5patch(whole);
    tokejsr(quickdiv);                          /* .whole */
    p5w(0x4a80);                                /* tst.l d0 */
    p5w(0x6a06);                                /* bpl.s */
    p5w(0x0680);                                /* addi.l #$FFFF,d0: towards zero */
    p5l(0x0000ffff);
    p5w(0x4840);                                /* swap d0 */
    p5w(0x48c0);                                /* ext.l d0 */
    p5patch(done);                              /* .done */
    ritefromd0();
    pokemovem(pull);
}

/* the operator p in D(r) (its left operand, a pair when that is 48.16)
   and D(r+1+lwide) (its right operand), its result in D(r) (a pair for
   48.16); in prep mode the libraries its code calls are used */
static void p5bin(struct p5node *p, UWORD r)
{
    const struct exprtyping_op *o = &p->t.o;
    struct p5node *L = P5(p->t.l), *R = P5(p->t.r);
    UWORD q = r + 1 + (L->df == ETF_Q48), lt = p5norm(L->dt), rt = p5norm(R->dt), T;
    unsigned op = p->t.op;
    BOOL lw = L->df == ETF_Q48, rw = R->df == ETF_Q48;
    UWORD s = regat;

    switch (o->path) {
    case ETY_Q48: {
        UWORD tok, ftok;
        BOOL ldiv = (p->flags & P5F_LDIV) != 0;
        BOOL wdiv = ldiv && (p->want == T_WORD || p->want == T_BYTE);

        if (p->flags & (P5F_NDIV | P5F_NMULF | P5F_WMUL)) {
            p5fast(p, r);
            break;
        }
        switch (op) {
        case ETO_ADD:
        case ETO_SUB: tok = 0; break;   /* inline: q48add's, q48sub's code */
        case ETO_MUL: tok = p5down(p) ? q48mulf : q48mul; break;
        case ETO_MOD: tok = q48mod; break;
        default:
            tok = p->want == T_QUICK ? q48divq : p->want == T_WORD ? q48divw : p->want == T_BYTE ? q48divb : q48div;
            break;
        }
        /* #1222: at the top under a wanted Word or Byte towards zero, as
           q48div cuts (q48divw, q48divb for their division by 0) */
        ftok = wdiv ? q48div : tok;
        if ((op == ETO_DIV || (op == ETO_MOD && !p5modfrac(R))) && !rw && rt == T_QUICK) {
            /* a 16.16 divisor: whole at run time, the division or Mod
               inline (not a Mod by a constant with a fraction); a whole
               dividend not widened */
            BOOL longa = !lw && (lt == T_LONG || lt == T_WORD || lt == T_BYTE);
            BOOL raw = ldiv && !lw && lt != T_LONG;

            if (p5prep) {
                p5callin(tok, r, lt, lw, rt, rw, FALSE, FALSE);
                if (op == ETO_DIV && raw)
                    tokuse(longdiv);
                else if (op == ETO_DIV && ftok != tok)
                    tokuse(ftok);
                break;
            }
            p5nowide = longa;
            p5callin(0, r, lt, lw, rt, rw, FALSE, FALSE);
            if (op == ETO_DIV)
                p5wholediv(tok, ftok, lt, longa, ldiv, raw, p5down(p));
            else
                p5wholemod(tok, lt, longa);
        } else if (op == ETO_MUL && !lw && !rw && ((p5whole1(lt) && rt == T_QUICK) || (lt == T_QUICK && p5whole1(rt)))) {
            /* a whole number times a 16.16 value (the speed review,
               candidate #8): compilerlib's q48mull on the two 32-bit
               values, not widened, the exact product q48mul gives */
            p5nowide = TRUE;
            p5callin(p5prep ? q48mull : 0, r, lt, lw, rt, rw, FALSE, FALSE);
            if (lt != T_QUICK && lt != T_LONG)
                p5w(0x48c0);                /* ext.l d0: a Word or Byte */
            if (rt != T_QUICK && rt != T_LONG)
                p5w(0x48c2);                /* ext.l d2 */
            if (!p5prep)
                tokejsr(q48mull);
        } else if (op == ETO_DIV && wdiv && !p5folded(&R->t)) {
            /* #1222: towards zero (q48div), a divisor 0 by tok for its
               value */
            ULONG z, z2, done;

            p5callin(0, r, lt, lw, rt, rw, FALSE, FALSE);
            if (p5prep) {
                tokuse(tok);
                tokuse(ftok);
                break;
            }
            p5w(0x4a82);                    /* tst.l d2 */
            z = p5bra(0x6600);              /* bne.s .div */
            p5w(0x4a83);                    /* tst.l d3 */
            z2 = p5bra(0x6600);             /* bne.s .div */
            tokejsr(tok);
            done = p5bra(0x6000);           /* bra.s .done */
            p5patch(z);
            p5patch(z2);
            tokejsr(ftok);                  /* .div */
            p5patch(done);
            p5truncd0();
        } else {
            /* a constant divisor at the top under a wanted Word or Byte:
               q48div (#1222), tok when it is 0 */
            p5callin(wdiv && p5qval(&R->t).ok && p5qval(&R->t).n ? ftok : tok, r, lt, lw, rt, rw, FALSE, FALSE);
            if (ldiv && !p5prep)
                p5truncd0();
        }
        if (op == ETO_ADD) {
            p5w(0xd082);                    /* add.l d2,d0 */
            p5w(0xd383);                    /* addx.l d3,d1 */
        } else if (op == ETO_SUB) {
            p5w(0x9082);                    /* sub.l d2,d0 */
            p5w(0x9383);                    /* subx.l d3,d1 */
        }
        p5callout(r, !ldiv);
        if (op == ETO_DIV && p->want == T_LONG && !ldiv && !p5prep) {
            /* part 3 (EM_R_PDIVL): under a wanted Long the quotient is a
               whole number towards zero at the operator */
            p5w(0x4a80 | (r + 1));          /* tst.l D(r+1) */
            p5w(0x6a0e);                    /* bpl.s .cut */
            p5w(0x4a40 | r);                /* tst.w D(r): a fraction */
            p5w(0x670a);                    /* beq.s .cut */
            p5w(0x0680 | r);                /* addi.l #$10000,D(r) */
            p5l(0x00010000);
            p5w(0x6402);                    /* bcc.s .cut */
            p5w(0x5280 | (r + 1));          /* addq.l #1,D(r+1) */
            p5w(0x4240 | r);                /* .cut: clr.w D(r) */
        }
        break;
    }
    case ETY_EXACT:
        if (o->lwide || o->rwide || lw || rw) {
            /* the difference of the two 48.16 values, its sign */
            BOOL swap = op == ETO_GT || op == ETO_LE;
            UWORD scc = op == ETO_EQ ? *mseq : op == ETO_NE ? *msne : op == ETO_LT || op == ETO_GT ? *mslt : *msge;

            p5callin(0, r, lt, lw, rt, rw, swap, FALSE);
            p5w(0x9082);                    /* sub.l d2,d0: q48sub's code */
            p5w(0x9383);                    /* subx.l d3,d1 */
            if (op == ETO_EQ || op == ETO_NE)
                p5w(0x8081);                /* or.l d1,d0 */
            else
                p5w(0x4a81);                /* tst.l d1 */
            p5w(scc);                       /* scc d0 */
            p5w(0x4880);                    /* ext.w d0 */
            p5callout(r, FALSE);
            break;
        }
        if ((lt == T_LONG && rt == T_QUICK) || (lt == T_QUICK && rt == T_LONG)) {
            p5operands(L, R, r, q, lt, rt);
            if (!p5prep)
                cmplq(&operators[op], lt);
            break;
        }
        T = lt == T_QUICK || rt == T_QUICK ? T_QUICK : p5max(lt, rt);
        p5operands(L, R, r, q, T, T);
        p5doop(op, T);
        break;
    case P5Y_LOW:
        /* the low 32 bits of the 48.16 value: a whole operand as l << 16
           modulo 2^32, a 16.16 one as it is */
        p5operands(L, R, r, q, o->las, o->ras);
        if (op == ETO_MUL) {
            /* l * q modulo 2^32 (one operand whole, the 48.16 product of
               the two is exact): three MULU in D(r+2), D(r+3) */
            p5w(0x2000 | ((r + 2) << 9) | r);           /* move.l D(r),D(r+2) */
            p5w(0x4840 | (r + 2));                      /* swap D(r+2) */
            p5w(0xc0c0 | ((r + 2) << 9) | (r + 1));     /* mulu.w D(r+1),D(r+2): ah*bl */
            p5w(0x2000 | ((r + 3) << 9) | (r + 1));     /* move.l D(r+1),D(r+3) */
            p5w(0x4840 | (r + 3));                      /* swap D(r+3) */
            p5w(0xc0c0 | ((r + 3) << 9) | r);           /* mulu.w D(r),D(r+3): al*bh */
            p5w(0xd040 | ((r + 2) << 9) | (r + 3));     /* add.w D(r+3),D(r+2) */
            p5w(0x4840 | (r + 2));                      /* swap D(r+2) */
            p5w(0x4240 | (r + 2));                      /* clr.w D(r+2) */
            p5w(0xc0c0 | (r << 9) | (r + 1));           /* mulu.w D(r+1),D(r): al*bl */
            p5w(0xd080 | (r << 9) | (r + 2));           /* add.l D(r+2),D(r) */
            break;
        }
        if (o->ras == T_LONG) {
            p5w(0x4840 | (r + 1));                      /* swap D(r+1) */
            p5w(0x4240 | (r + 1));                      /* clr.w D(r+1) */
        }
        if (o->las == T_LONG) {
            p5w(0x4840 | r);                            /* swap D(r) */
            p5w(0x4240 | r);                            /* clr.w D(r) */
        }
        p5w((op == ETO_ADD ? 0xd080 : 0x9080) | (r << 9) | (r + 1));   /* add.l / sub.l D(r+1),D(r) */
        break;
    case ETY_MIXED: {
        struct exprtype_bin d;

        memset(&d, 0, sizeof d);
        d.path = ETP_MIXED;
        d.lq = o->lwide;
        d.rq = o->rwide;
        p5operands(L, R, r, q, o->lwide ? T_QUICK : T_LONG, o->rwide ? T_QUICK : T_LONG);
        if (p5prep) {
            if (op == ETO_MUL)
                tokuse(d.lq && d.rq ? qqmult : mixmult);
            else if (op == ETO_DIV)
                tokuse(d.rq && !d.lq ? lqdiv : longdiv);
        } else
            mixedop(&operators[op], &d, FALSE);
        break;
    }
    default:
        T = o->path == ETY_Q16 ? T_QUICK : o->las;
        if (op == ETO_MOD && (p->dt == T_QUICK || p->dt == T_FLOAT)) {
            /* the exact Mod (an operand with a fraction) */
            LONG m;

            p5operands(L, R, r, q, p->dt, p->dt);
            if (p->dt == T_FLOAT)
                p5fmod(r);
            else if (p5modconst(R, &m)) {
                if (!p5prep)
                    p5qmod(r, TRUE, m);         /* a whole constant divisor */
            } else if (!p5prep && !p5folded(&R->t))
                p5qmod(r, FALSE, 0);            /* whole at run time, else q16mod */
            else if (p5folded(&R->t) && !p5qval(&R->t).n) {
                /* a constant 0: the trap */
                p5callin(q48mod, r, T_QUICK, FALSE, T_QUICK, FALSE, FALSE, TRUE);
                p5callout(r, FALSE);
            } else if (p5prep)
                tokuse(q16mod);
            else
                domylib(q16mod);                /* a constant with a fraction: not 0, no trap */
            break;
        }
        if (op == ETO_POW && o->las != o->ras) {
            p5operands(L, R, r, q, o->las, o->ras);
            regat = r + 1;
            p5conv(o->ras, ETF_TYPE, T_FLOAT, FALSE);
            regat = r;
            p5conv(o->las, ETF_TYPE, T_FLOAT, TRUE);
            if (p5prep)
                tokuse(mathq * 128 + 1);
            else
                domylib(mathq * 128 + 1);
            break;
        }
        if (op == ETO_DIV && (p->flags & P5F_LDIV)) {
            p5qdivt(p, r, q);
            break;
        }
        p5operands(L, R, r, q, T, o->path == ETY_Q16 ? T_QUICK : o->ras);
        p5doop(op, T);
        break;
    }
    regat = s;
}

static void p5binopact(struct expract *a)
{
    p5bin(P5(a->x), regat);
}

static void p5nop(struct expract *a)
{
    (void)a;
}

/* a folded constant (a->x: its p5node) */
static void p5constact(struct expract *a)
{
    const struct p5node *p = P5(a->x);

    if (p->df == ETF_Q48) {
        /* a 48.16 value (#1220): the low 32 bits in Dn, the high ones in Dn+1 */
        p5reg(regat + 1);
        pokewd((UWORD)(regat << 9) | 0x203c);
        pokel(p->enc);
        pokewd((UWORD)((regat + 1) << 9) | 0x203c);
        pokel(p->enc2);
    } else if (p->dt < T_LONG) {
        pokewd((UWORD)(regat << 9) | 0x303c);   /* move.w #v,Dn */
        pokewd((UWORD)p->enc);
    } else {
        pokewd((UWORD)(regat << 9) | 0x203c);   /* move.l #v,Dn */
        pokel(p->enc);
    }
}

/* the run of minus signs on its operand (a->x: its p5node) */
static void p5neg(const struct p5node *p)
{
    const struct p5node *c = P5(p->t.l);
    UWORD k;

    for (k = p->x->op; k; k--) {
        if (c->df == ETF_Q48) {
            p5reg(regat + 1);
            p5w(0x4480 | regat);                /* neg.l D(r) */
            p5w(0x4080 | (regat + 1));          /* negx.l D(r+1) */
        } else if (c->dt >= T_FLOAT) {
            if (p5prep)
                tokuse(getffpbase);
            else
                fdo2(P5_SPNEG);
        } else
            p5w((c->dt >= T_LONG ? 0x4480 : 0x4440) | regat);  /* neg.l / neg.w */
    }
}

static void p5negact(struct expract *a)
{
    p5neg(P5(a->x));
}

/* Not of its operand in its type (a->p: its p5node) */
static void p5not(const struct p5node *p)
{
    const struct p5node *c = P5(p->t.l);
    UWORD t = p->dt;

    p5conv(c->dt, c->df, t, FALSE);
    p5w((t >= T_LONG ? 0x4680 : 0x4640) | regat);  /* not.l / not.w */
    if (!p5prep)
        quickwhole(t);              /* Fixed (#1209): Not of floor(q) */
}

static void p5notact(struct expract *a)
{
    p5not(a->p);
}

/* the actions of the node p's code, from its first one to its last */
#define P5ACTS(a, p) for ((a) = (p)->prev->next; (a); (a) = (a) == (p)->last ? NULL : (a)->next)

/* the right operand R of an operation whose left operand is a pair:
   computed one register higher */
static void p5shift(const struct p5node *R)
{
    struct expract *a;

    P5ACTS(a, R) {
        if (a->fn == spillact || ++a->ctx[0] > 7)
            conmemerr();            /* #1185 */
    }
}

static void p5nopall(struct p5node *p, struct p5node *keep)
{
    switch (p->t.kind) {
    case ETN_CONST:
        if (p != keep)
            p->act->fn = p5nop;
        break;
    case ETN_NEG:
        p->act->fn = p5nop;
        p5nopall(P5(p->t.l), keep);
        break;
    case ETN_BIN:
        p->act->fn = p5nop;
        p5nopall(P5(p->t.l), keep);
        p5nopall(P5(p->t.r), keep);
        break;
    default: break;
    }
}

/* a node of the subtree that the build's typing refused (P5F_RELAX) */
static BOOL p5relaxed(const struct exprtyping_node *n)
{
    switch (n->kind) {
    case ETN_BIN: return (P5(n)->flags & P5F_RELAX) || p5relaxed(n->l) || p5relaxed(n->r);
    case ETN_NEG:
    case ETN_NOT: return (P5(n)->flags & P5F_RELAX) || p5relaxed(n->l);
    default: return FALSE;
    }
}

/* the folded constant p taken in the type take (0: as an operand of a
   48.16 operation, a whole number a Long, else a Quick); bit: its whole
   part */
static void p5constapply(struct p5node *p, UWORD take, BOOL bit)
{
    struct p5node *k = p;
    struct expract *a;
    ULONG enc = 0;
    BOOL ok, relax;
    BOOL pair = FALSE;
    int64_t m = 0;

    if (!take) {
        struct p5q v = p5qval(&p->t);

        take = v.ok && v.d == 1 && p5fits(v.n, T_LONG) ? T_LONG : T_QUICK;
        /* a value beyond the Quick range (a fraction, 16.16 value beyond
           32 bits): a 48.16 value in a register pair (#1220) */
        if (take == T_QUICK && v.ok && !__builtin_mul_overflow(v.n, (int64_t)65536, &m)) {
            m /= v.d;
            pair = m < INT32_MIN || m > INT32_MAX;
        }
    }
    if (pair) {
        p->enc = (ULONG)(uint64_t)m;
        p->enc2 = (ULONG)((uint64_t)m >> 32);
        p->dt = T_LONG;
        p->df = ETF_Q48;
        goto place;
    }
    ok = p5encode(p, take, bit, &enc);
    relax = p5relaxed(&p->t);
    if (!ok && relax)
        illoperr();                 /* a value beyond the pass (an overflow) */
    if (!relax && (p->tt == take || !ok) && !(ok && p5constmodfrac(&p->t))) {
        /* the build's code: today's value in its type */
        p->flags |= P5F_KEEP;
        p->dt = p->tt;
        p->df = ETF_TYPE;
        return;
    }
    p->enc = enc;
    p->dt = (UBYTE)take;
    p->df = ETF_TYPE;
place:
    while (k->t.kind != ETN_CONST)
        k = P5(k->t.l);             /* the first constant writes the value */
    p5nopall(p, k);
    k->act->fn = p5constact;
    k->act->x = (ULONG)p;
    /* getelement's spill of a constant after the first one (the spill of
       the first one, at d7, lies around the subtree's code) */
    P5ACTS(a, p) {
        if (a->fn == spillact)
            a->fn = p5nop;
    }
}

static void p5apply(struct p5node *p, UWORD take, BOOL bit, UWORD want);

/* the operator p, its operands */
static void p5binapply(struct p5node *p, UWORD want)
{
    const struct exprtyping_op *o = &p->t.o;
    struct p5node *L = P5(p->t.l), *R = P5(p->t.r);
    unsigned op = p->t.op;
    BOOL bit = p5isbit(op);
    UWORD lt = o->las, rt = o->ras;

    switch (o->path) {
    case ETY_Q48: lt = rt = 0; break;
    case ETY_Q16: lt = rt = T_QUICK; break;
    case P5Y_LOW: break;            /* las, ras: T_LONG or T_QUICK */
    case ETY_MIXED:
        lt = o->lwide ? T_QUICK : T_LONG;
        rt = o->rwide ? T_QUICK : T_LONG;
        break;
    case ETY_EXACT:
        if (o->lwide || o->rwide) {
            lt = o->las == ETY_CONST ? 0 : o->las;
            rt = o->ras == ETY_CONST ? 0 : o->ras;
            break;
        }
        if (o->las == ETY_CONST)
            lt = p5exactconst(L, o->ras);
        if (o->ras == ETY_CONST)
            rt = p5exactconst(R, o->las);
        break;
    default: break;
    }
    /* #978: an equality comparison of a byte value with a constant from
       128 to 255 takes the byte the constant stands for */
    if ((op == ETO_EQ || op == ETO_NE) && o->path == ETY_COMPARE && o->las == T_WORD) {
        if (L->t.kind == ETN_VAR && L->t.type == T_BYTE && R->t.kind == ETN_CONST)
            R->flags |= P5F_SIGNEXT;
        if (R->t.kind == ETN_VAR && R->t.type == T_BYTE && L->t.kind == ETN_CONST)
            L->flags |= P5F_SIGNEXT;
    }
    /* the operands of a comparison have no wanted type (part 8) */
    p5apply(L, lt, bit, exprtype_iscompare(op) ? 0 : want);
    p5apply(R, rt, bit, exprtype_iscompare(op) ? 0 : want);
    if (L->df == ETF_Q48)
        p5shift(R);
    p->want = (UBYTE)want;
    p5fastmark(p);
    switch (o->path) {
    case ETY_Q48:
        p->dt = T_LONG;
        p->df = ETF_Q48;
        if (p->flags & (P5F_NDIV | P5F_NMULF)) {
            p->dt = T_QUICK;        /* its value fits 16.16: one register */
            p->df = ETF_TYPE;
        } else if (p->flags & P5F_LDIV)
            p->df = ETF_TYPE;       /* its Long towards zero (p5bin) */
        break;
    case ETY_Q16: p->dt = p->flags & P5F_LDIV ? T_LONG : T_QUICK; break;    /* p5qdivt's Long */
    case ETY_EXACT:
    case ETY_COMPARE: p->dt = T_WORD; break;
    case ETY_MIXED: p->dt = T_LONG; break;
    case P5Y_LOW: p->dt = T_QUICK; break;
    default: p->dt = o->result; break;
    }
    p->act->fn = p5binopact;
    p->act->x = (ULONG)p;
    p5prep = TRUE;
    p5bin(p, p->x->reg);
    p5prep = FALSE;
}

/* the pass's code for the node p (take: the type its consumer takes a
   folded constant in, bit: as its whole part) */
static void p5apply(struct p5node *p, UWORD take, BOOL bit, UWORD want)
{
    struct exprnode *x = p->x;
    struct p5node *c;

    if (p5folded(&p->t)) {
        p5constapply(p, p5norm(take), bit);
        return;
    }
    p->df = ETF_TYPE;
    switch (p->t.kind) {
    case ETN_VAR:
        if (p->lkind == P5L_CONT) {
            /* fetched in its own type */
            x->tafter = x->own;
            x->lflags &= ~(ELF_RAW | ELF_CONV | ELF_DROP | ELF_NARROW);
        }
        p->dt = (UBYTE)(x->own ? x->own : x->tafter);
        break;
    case ETN_NEG:
        c = P5(p->t.l);
        p5apply(c, take, FALSE, want);
        p->dt = c->dt;
        p->df = c->df;
        p->act->fn = p5negact;
        p->act->x = (ULONG)p;
        p5prep = TRUE;
        p5neg(p);
        p5prep = FALSE;
        break;
    case ETN_NOT:
        c = P5(p->t.l);
        p->dt = p->t.t;
        p5apply(c, p->t.t, TRUE, want);
        p->act->fn = p5notact;
        p->act->p = p;
        p5prep = TRUE;
        p5not(p);
        p5prep = FALSE;
        break;
    default:
        p5binapply(p, want);
        break;
    }
}

/* an operand of an operation under ETC_FLOOR (p5mixfloor): 1 a whole
   number in one register (a Word or Long value, a whole constant of the
   Long range), 2 a 16.16 value (a Quick, a 16.16 result, a constant with
   a fraction in the Quick range), 0 else (a 48.16 value, a Byte, a Float) */
static int p5mixkind(const struct exprtyping_node *n)
{
    if (p5folded(n)) {
        struct p5q v = p5qval(n);

        if (!v.ok)
            return 0;
        if (v.d == 1)
            return p5fits(v.n, T_LONG) ? 1 : 0;
        return p5fits(p5qfloor(v), T_QUICK) ? 2 : 0;
    }
    if (!n->typed)
        return 0;
    if (n->fmt == ETF_Q16)
        return 2;
    if (n->fmt != ETF_TYPE)
        return 0;
    if (n->kind == ETN_VAR && n->type == ET_QUICK)
        return 2;
    return n->t == ET_WORD || n->t == ET_LONG ? 1 : 0;
}

/* a constant below 1 with a fraction: a factor p5fastmark takes inline
   (P5F_NMULF) beside a 16.16 value */
static BOOL p5below1(const struct exprtyping_node *n)
{
    struct p5q v;

    if (!p5folded(n))
        return FALSE;
    v = p5qval(n);
    return v.ok && v.d != 1 && (v.n < 0 ? -v.n : v.n) * 65536 < 65535 * v.d;
}

/* n is a 48.16 * of two 16.16 values that the routine qqmult of #1178 takes
   (not a factor below 1: that is inline) */
static BOOL p5qqmul(const struct exprtyping_node *n)
{
    return n->kind == ETN_BIN && n->typed && n->op == ETO_MUL && n->o.path == ETY_Q48 && p5mixkind(n->l) == 2
           && p5mixkind(n->r) == 2 && !p5below1(n->l) && !p5below1(n->r);
}

/* the operation n of the routines of #1178 (ETY_MIXED; qqmult for two
   16.16 values): a whole number (lk, rk 1) or a 16.16 value (2) on each
   side, the whole part rounded down as a Long */
static void p5mixset(struct exprtyping_node *n, int lk, int rk)
{
    struct exprtyping_op *o = &n->o;

    o->path = ETY_MIXED;
    o->lwide = (unsigned char)(lk == 2);
    o->rwide = (unsigned char)(rk == 2);
    o->las = lk == 2 ? ET_QUICK : ET_LONG;
    o->ras = rk == 2 ? ET_QUICK : ET_LONG;
    o->result = ET_LONG;
    o->fast = 0;
    n->t = ET_LONG;
    n->fmt = ETF_TYPE;
}

/* task 4.3 (a tree optimisation, design decision 9): the root of an
   expression taken as its whole part rounded down (ETC_FLOOR), a 48.16
   +, - or * of a whole number and a 16.16 value (+ and - also of two
   16.16 values), is the routine of #1178 that gives that whole part
   directly (mixedop: l + floor(q), l - ceil(q), mixmult's bits 16..47 of
   the product), the same value: neither operand has more than 32 bits,
   so the 48.16 operation is exact and its floor is the routine's.  The
   speed review (2026-11-26, candidate #4): a * of two 16.16 values is
   compilerlib's qqmult, the exact product rounded down (p5qqmul; with the cut of
   #1221 that is the 48.16 value: the cut rounds down there too); a + of such a
   product and a whole number l, or a - of l from it, is floor(q * p) + l
   or - l as Longs (floor(x + l) = floor(x) + l for a whole l, modulo
   2^32 as the consumer takes it). */
static void p5mixfloor(struct p5node *root, struct exprtyping_result *res)
{
    struct exprtyping_op *o = &root->t.o;
    struct exprtyping_node *l = root->t.l, *r = root->t.r;
    int lk, rk;

    if (res->consumer != ETC_FLOOR || res->fmt != ETF_Q48 || root->t.kind != ETN_BIN || o->path != ETY_Q48
        || (root->t.op != ETO_ADD && root->t.op != ETO_SUB && root->t.op != ETO_MUL))
        return;
    lk = p5mixkind(l);
    rk = p5mixkind(r);
    if (root->t.op != ETO_MUL && ((p5qqmul(l) && rk == 1) || (root->t.op == ETO_ADD && lk == 1 && p5qqmul(r)))) {
        /* floor(q * p) + l, floor(q * p) - l, l + floor(q * p) */
        if (lk == 1)
            p5mixset(r, 2, 2);
        else
            p5mixset(l, 2, 2);
        o->path = ETY_PLAIN;
        o->las = o->ras = o->result = ET_LONG;
        o->lwide = o->rwide = 0;
        o->fast = 0;
        root->t.t = ET_LONG;
        root->t.fmt = ETF_TYPE;
        res->fmt = ETF_TYPE;
        return;
    }
    if (!lk || !rk || (lk == 1 && rk == 1) || (root->t.op == ETO_MUL && lk == 2 && rk == 2 && !p5qqmul(&root->t)))
        return;
    p5mixset(&root->t, lk, rk);
    res->fmt = ETF_TYPE;
}

/* n, a 48.16 + or - whose value is taken as its low 32 bits, can compute
   them alone: each operand a whole number, a 16.16 value or such an
   operation (p5mixkind) */
static BOOL p5lowable(const struct exprtyping_node *n)
{
    const struct exprtyping_node *k[2];
    int i;

    if (n->kind != ETN_BIN || !n->typed || n->o.path != ETY_Q48 || (n->op != ETO_ADD && n->op != ETO_SUB))
        return FALSE;
    k[0] = n->l;
    k[1] = n->r;
    for (i = 0; i < 2; i++)
        if (!p5mixkind(k[i]) && !p5lowable(k[i]))
            return FALSE;
    return TRUE;
}

/* the operation n (and the + and - below it, p5lowable) on the path
   P5Y_LOW */
static void p5lowset(struct exprtyping_node *n)
{
    struct exprtyping_op *o = &n->o;
    struct exprtyping_node *k[2];
    unsigned char *as[2];
    int i;

    k[0] = n->l;
    k[1] = n->r;
    as[0] = &o->las;
    as[1] = &o->ras;
    for (i = 0; i < 2; i++) {
        int m = p5mixkind(k[i]);

        if (!m)
            p5lowset(k[i]);
        *as[i] = m == 1 ? ET_LONG : ET_QUICK;
    }
    o->path = P5Y_LOW;
    o->lwide = o->rwide = 0;
    o->result = ET_QUICK;
    o->fast = 0;
    n->t = ET_QUICK;
    n->fmt = ETF_TYPE;
}

/* task 4.3 (a tree optimisation, design decision 9): an expression taken
   as its low 32 bits (ETC_KEEP: a wanted Quick takes the 16.16 value)
   whose root is a 48.16 operation that needs no more than the low 32 bits
   of its operands: a chain of + and - of whole numbers and 16.16 values
   (the low 32 bits of a sum are the sum of the low 32 bits), or a * of a
   whole number and a 16.16 value (the exact product l * q, its low 32
   bits by three MULU; D(r+2), D(r+3) free below d6) computes in 32 bits
   (P5Y_LOW): the same value without the 48.16 operations */
static void p5lowmark(struct p5node *root, struct exprtyping_result *res)
{
    struct exprtyping_node *n = &root->t;

    if (res->consumer != ETC_KEEP || res->fmt != ETF_Q48 || n->kind != ETN_BIN || !n->typed
        || n->o.path != ETY_Q48)
        return;
    if (n->op == ETO_MUL) {
        int lk = p5mixkind(n->l), rk = p5mixkind(n->r);

        if (lk + rk != 3 || root->x->reg + 3 > 5)
            return;
    } else if (!p5lowable(n))
        return;
    p5lowset(n);
    res->fmt = ETF_TYPE;
}

/* the evaluation is built (asked: the type asked for, unk: none): the
   pass of rule P+; TRUE: its code is the pass's, *type the type it gets
   (e: the evaluation's node) */
static BOOL p5end(UWORD asked, WORD unk, UWORD *type, struct exprnode *e, WORD push, BOOL cond, BOOL wide)
{
    struct p5node *root = NULL;
    struct exprtyping_result res;
    struct expract *a;
    BOOL relax, pair;
    UWORD want, S;

    if (p5over) {
        p5over--;
        return FALSE;
    }
    if (!p5nm)
        return FALSE;
    p5nm--;
    relax = p5mk[p5nm].relax;
    if (!p5mk[p5nm].bad && p5sp == p5mk[p5nm].sp + 1)
        root = p5stk[p5sp - 1];
    p5sp = p5mk[p5nm].sp;
    if (!root || root->depth > P5DEPTH || asked > T_FLOAT || (unk && asked) || p5hasstr(&root->t)) {
        if (relax)
            illoperr();
        return FALSE;
    }
    want = unk ? 0 : asked;
    if (exprtyping_type(&root->t, want, &res))
        illoperr();                 /* refused by rule P+ */
    p5today(root);
    /* a Select's 48.16 value is kept as 48.16 (#1216; a Quick or 16.16
       Select as before) */
    pair = wide && res.consumer != ETC_BOOL && res.fmt == ETF_Q48;
    if (!pair) {
        p5mixfloor(root, &res);
        p5lowmark(root, &res);
    }
    if (!relax && !pair && p5same(root, &res, want))
        return FALSE;
    /* the pass's code: the conversions of the raw Quicks (#1178) and
       getelement's spills of this evaluation whole longs */
    P5ACTS(a, root) {
        if (a->ctx[2] != evaldepth)
            continue;
        if (a->fn == cutact)
            a->fn = p5nop;
        else if (a->fn == spillact) {
            if (a->x & ACT_TWO) {
                a->x = ACT_TWO | 0x2e06;    /* move.l d6,d7 */
                a->y = 0x2c1f;              /* move.l (a7)+,d6 */
            } else
                a->x = 0x2f06;              /* move.l d6,-(a7) */
        }
    }
    /* a 48.16 or 16.16 / at the top under a wanted Byte, Word or Long
       gives its value towards zero as a Long itself (part 3, #1222: the
       consumer's cut once) */
    if (res.consumer == ETC_TRUNC && root->t.kind == ETN_BIN && root->t.op == ETO_DIV
        && (root->t.o.path == ETY_Q48 || root->t.o.path == ETY_Q16))
        root->flags |= P5F_LDIV;
    p5apply(root, res.t, FALSE, want);
    switch (res.consumer) {
    case ETC_BOOL: S = T_WORD; break;
    case ETC_FLOOR: S = root->df == ETF_Q48 ? T_LONG : root->dt; break;
    case ETC_TRUNC:
    case ETC_FIT: S = T_LONG; break;
    default: S = root->df == ETF_Q48 ? T_QUICK : root->dt; break;
    }
    if ((res.consumer == ETC_TRUNC || res.consumer == ETC_FIT) && root->df != ETF_Q48)
        S = root->dt;               /* ETC_TRUNC: p5qdivt's Long; ETC_FIT only for 48.16 */
    e->lflags |= EVF_P5;
    p5unit = TRUE;
    if (res.consumer == ETC_FIT && root->df == ETF_Q48)
        e->lflags |= EVF_FIT;
    e->left = (struct exprnode *)root;
    e->vtype = res.consumer;
    if (pair) {
        e->vtype = P5C_PAIR;
        e->lflags &= ~EVF_FIT;
        S = T_QUICK;
        p5wide = TRUE;
    } else if (res.consumer == ETC_FIT && push != 1 && !cond) {
        /* no type word, no condition: as an assignment to a Quick takes it
           (its low 32 bits, the 16.16 value) */
        e->vtype = ETC_KEEP;
        e->lflags &= ~EVF_FIT;
        S = T_QUICK;
    }
    *type = S;
    return TRUE;
}

/* the end of the evaluation e of the pass's code: its value taken as the
   pass says (push > 0: the type got is pushed) */
static void p5exit(struct exprnode *e, WORD push)
{
    const struct p5node *p = P5(e->left);
    UWORD r = regat;

    if (e->vtype == P5C_PAIR) {
        /* a Select's value with a fraction (#1216): as 48.16 */
        if (p->df != ETF_Q48) {
            p5reg(r + 1);
            p5widen(r, p->dt);
        }
        return;
    }
    if (e->vtype == ETC_TRUNC && p->df != ETF_Q48 && p->dt == T_QUICK) {
        /* a 16.16 / at the top that p5qdivt does not write: the value
           towards zero, the consumer's cut down then exact (#1222) */
        pokewd(0x4a80 | r);                         /* tst.l D(r) */
        pokewd(0x6a06);                             /* bpl.s */
        pokewd(0x0680 | r);                         /* addi.l #$FFFF,D(r) */
        pokel(0x0000ffff);
        return;
    }
    if (p->df != ETF_Q48)
        return;
    switch (e->vtype) {
    case ETC_FLOOR: p5floor48(r); break;
    case ETC_FIT:
        /* a Quick when the value fits one, else its whole part */
        p5reg(r + 2);
        pokewd(0x2000 | ((r + 2) << 9) | r);        /* move.l D(r),D(r+2) */
        pokewd(0xd080 | ((r + 2) << 9) | (r + 2));  /* add.l D(r+2),D(r+2) */
        pokewd(0x9180 | ((r + 2) << 9) | (r + 2));  /* subx.l D(r+2),D(r+2): the sign */
        pokewd(0xb080 | ((r + 2) << 9) | (r + 1));  /* cmp.l D(r+1),D(r+2) */
        pokewd(push > 0 ? 0x670c : 0x6708);         /* beq.s: it fits */
        p5floor48(r);
        if (push > 0) {
            pokewd(0x7003 | ((r + 1) << 9));        /* moveq #3,D(r+1): a Long */
            pokewd(0x6002);                         /* bra.s */
            pokewd(0x7004 | ((r + 1) << 9));        /* moveq #4,D(r+1): a Quick */
        }
        break;
    default: break;                 /* its low 32 bits: the 16.16 value (ETC_TRUNC: not
                                       reached, the division gives its Long, P5F_LDIV) */
    }
}

/* the unit a (its actions) is built: the registers of the typing pass's
   code, its register pairs and the right operands it computes one
   register higher (p5shift: also those of evaluations inside it), fit
   d0-d7 (#1185, design decision 10: refused when the expression is
   built, as exprtree_pre's check; the line is the statement's) */
static void p5check(const struct expract *a)
{
    for (; a; a = a->next) {
        UWORD r = a->ctx[0], need = r;

        if (a->fn == p5binopact) {
            const struct p5node *p = P5(a->x), *L = P5(p->t.l), *R = P5(p->t.r);

            need = r + 1 + (L->df == ETF_Q48) + (R->df == ETF_Q48);
            if (p->df == ETF_Q48 && need < r + 1)
                need = r + 1;
        } else if (a->fn == p5negact || a->fn == p5notact) {
            const struct p5node *p = a->fn == p5negact ? P5(a->x) : P5(a->p);

            if (P5(p->t.l)->df == ETF_Q48)
                need = r + 1;
        } else if (a->fn == evalexit) {
            const struct exprnode *e = a->p;

            if ((e->lflags & EVF_P5) && (P5(e->left)->df == ETF_Q48 || e->vtype == P5C_PAIR))
                need = r + ((e->lflags & EVF_FIT) ? 2 : 1);
        }
        if (need > 7)
            conmemerr();            /* "Expression too Complex" */
    }
}

/* ------------------------------------------- Select with a 48.16 value
   No asm counterpart (#1216; decisions "the Select value with a fraction",
   "only a 48.16 Select is wide"):
   a Select whose value rule P+ computes in 48.16 (T_SELWIDE, eval.h)
   keeps it as 48.16 on the stack, the low 32 bits at P5SELLO(a7), the
   high ones 4 bytes above; with runtime error checking a long $7345 on
   top, so that casechkw finds the frame "sE" at 2(a7) as for a Byte or
   Word Select (the "sE" doselect pushed lies below the value). */
#define P5SELLO (debugga ? 4 : 0)

/* doselect: the 48.16 value in D(regat)/D(regat+1) pushed */
void selwidepush(void)
{
    UWORD r = regat;

    p5reg(r + 1);
    pokewd(0x2f00 | (r + 1));               /* move.l D(r+1),-(a7): the high part */
    pokewd(0x2f00 | r);                     /* move.l D(r),-(a7): the low part */
    if (debugga) {
        pokewd(0x2f3c);                     /* move.l #$7345,-(a7) */
        pokel(0x7345);
    }
}

/* the emitter's state before a Case's evaluation (selwidestart) */
static ULONG p5casepc, p5casebig, p5caseoffs;

/* docase, before the Case's evaluation: where its code begins */
void selwidestart(void)
{
    p5casepc = pc;
    p5casebig = bigpc;
    p5caseoffs = numoffs;
}

/* the Case's code since selwidestart, when it is a constant's load into
   d0 alone (moveq, move.w # of a Byte or Word, move.l # without a
   relocation): its value as 48.16 in *v */
static BOOL p5caseconst(UWORD type, int64_t *v)
{
    ULONG at = (p5casepc + 1) & ~1UL, n = pc - at;
    UWORD w;
    LONG x;

    if (nomemleft || numoffs != p5caseoffs || pc < at)
        return FALSE;
    w = W_(at);
    if (n == 2 && (w & 0xff00) == 0x7000)
        x = (BYTE)w;                        /* moveq #n,d0 */
    else if (n == 4 && w == 0x303c && (type == T_BYTE || type == T_WORD))
        x = (WORD)W_(at + 2);               /* move.w #n,d0 */
    else if (n == 6 && w == 0x203c)
        x = (LONG)(((ULONG)W_(at + 2) << 16) | W_(at + 4));    /* move.l #n,d0 */
    else
        return FALSE;
    switch (type) {
    case T_BYTE: *v = (int64_t)(BYTE)x * 65536; return TRUE;
    case T_WORD: *v = (int64_t)(WORD)x * 65536; return TRUE;
    case T_LONG: *v = (int64_t)x * 65536; return TRUE;
    case T_QUICK: *v = x; return TRUE;
    default: return FALSE;
    }
}

/* cmpi.l #k,d(a7), tst.l d(a7) for k 0 (d 0: (a7)) */
static void p5cmpsp(ULONG k, UWORD d)
{
    pokewd(k ? (d ? 0x0caf : 0x0c97) : (d ? 0x4aaf : 0x4a97));
    if (k)
        pokel(k);
    if (d)
        pokewd(d);
}

/* the size of p5cmpsp's instruction */
static UWORD p5cmpsize(ULONG k, UWORD d)
{
    return (UWORD)(2 + (k ? 4 : 0) + (d ? 2 : 0));
}

/* docase: the Case's value in D(regat) of type type (T_SELWIDE: a 48.16
   value, p5wide) compared with the Select's: a whole number or a Quick as
   48.16, a Float with the Select's value made a Float; the flags for
   docase's bne.  A constant Case (p5caseconst) is compared as immediates
   against the Select's value on the stack, its load taken back (task 4.3,
   the candidate of 2026-11-26; the same flags) */
void selwidecase(UWORD type)
{
    UWORD r = regat, lo = P5SELLO;
    int64_t v;

    if (r)
        interr2();                  /* not reached: a statement's evaluation */
    if (p5wide) {
        p5wide = FALSE;
        type = T_SELWIDE;
    }
    if (type == T_STRING)
        noconerr();                 /* as a Case of a numeric Select */
    if (type == T_FLOAT) {
        pokewd(0x2f00);                     /* move.l d0,-(a7): the Case */
        pokewd(0x202f);                     /* move.l lo+4(a7),d0 */
        pokewd(lo + 4);
        pokewd(0x222f);                     /* move.l lo+8(a7),d1 */
        pokewd(lo + 8);
        p5q48tof(0);
        pokewd(0x221f);                     /* move.l (a7)+,d1 */
        pokewd(0xb081);                     /* cmp.l d1,d0 */
        return;
    }
    if (type != T_SELWIDE && p5caseconst(type, &v)) {
        ULONG kl = (ULONG)v, kh = (ULONG)((uint64_t)v >> 32);

        pc = p5casepc;
        bigpc = p5casebig;
        p5cmpsp(kl, lo);                    /* the low parts */
        pokewd(0x6600 | p5cmpsize(kh, (UWORD)(lo + 4)));       /* bne.s: not equal */
        p5cmpsp(kh, (UWORD)(lo + 4));       /* the high parts */
        return;
    }
    if (type == T_BYTE)
        pokewd(0x4880);                     /* ext.w d0 */
    if (type != T_SELWIDE)
        p5widen(0, type);
    pokewd(0xb0af);                         /* cmp.l lo(a7),d0 */
    pokewd(lo);
    pokewd(0x6604);                         /* bne.s: not equal */
    pokewd(0xb2af);                         /* cmp.l lo+4(a7),d1 */
    pokewd(lo + 4);
}

/* End Select and Pop Select: the value and its frame off the stack */
void selwidedrop(void)
{
    if (debugga)
        tokejsr(casechkw);          /* the frame "sE" */
    pokewd(0x4fef);                         /* lea d(a7),a7 */
    pokewd(debugga ? 4 + 8 + 2 : 8);
}
