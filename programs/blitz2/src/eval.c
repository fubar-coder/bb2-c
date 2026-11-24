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
void exprbuildreset(void)
{
    nbforth = 0;
    bconv.seq = blit.seq = bcst.seq = bbyte = 0;
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
    if (h)
        h();                        /* jmp (a0) */
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

    if (e->lflags & EVF_CONV)
        convtype(e->tbefore, t);    /* exg d2,d3 / convtype */
    pushflag = (WORD)a->y;
    if (pushflag > 0) {
        /* push the type got. */
        UWORD regfield = regat << 9;
        if (t == T_STRING)
            pokewd(regfield | *getstlen);
        stackuse += 2;                              /* .hi1 */
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

    laterindex = idxnext;           /* Fixed (#1179) */
    idxnext = FALSE;
    stateact(evalenter, e, 0, (UWORD)push);
    evaldepth++;
    rawq = FALSE;
    from = codeseq;
    c = eval3(&t);
    if (laterindex)                 /* Fixed (#1179): narrowed? */
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
    bstrwork = sw || u->strwork;    /* for a build around the unit */
    u->strwork = sw;
}

/* no asm counterpart (eval.h): the unit's code written, the unit ended */
void unitwrite(struct stmtunit *u)
{
    exprtree_run(u->acts);
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


/* blitz2.s:9017 the constant fold of eval5 (exprtree_emit): the code of
   the operator n's two operands (both constants, BNF_FOLD) and the
   operation is run and replaced by a move of the result (of type `type`)
   into regat.  Where the left operand's code begins, the relocations and
   lasta6 then: its first action s (exprtree_operand's startmark). */
static void constfold(const struct expract *s, UWORD type)
{
    ULONG at = ((const struct exprnode *)s->p)->start;
    struct m68kregs r;
    struct offnode *node, *keep, **link;
    ULONG value, foldend;
    UWORD opcode;

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
    pc = at;
    /* DIFF: asm saved d2/a5 and did jsr (a0) with the compiler's
       registers; C runs the code through callregs with a6 = maths and a5
       such that ffplib's var slot is `maths` */
    memset(&r, 0, sizeof(r));
    r.a[6] = (ULONG)MathBase;
    r.a[5] = (ULONG)&MathBase - (LONG)ffplib->varoff;
    flushc();
    callregs((APTR)pc, &r);
    value = r.d[0];                         /* d0 has got constant! */
    opcode = (regat << 9) | 0x203c;         /* move.l #x,dn */
    if (type < 3) {
        pokewd(opcode | 0x1000);
        pokewd((UWORD)value);
    } else {
        pokewd(opcode);
        pokel(value);
    }
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
    binopprep(n->op, &d);
    if (!(bforth[nbforth - 2].constgot | bforth[nbforth - 1].constgot))
        n->lflags |= BNF_FOLD;
    if (n->lflags & BNF_FOLD)
        divzerobuild(n, &d);       /* "Division by Zero" before the next element */
    if (n->op == ETO_POW || (n->op == ETO_MOD && (d.handler == T_LONG || d.handler == T_FLOAT))
        || (d.path == ETP_MIXED && !d.handler && (n->op == ETO_MUL || n->op == ETO_DIV))) {
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
    (void)a;
    quicktolong();
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
   are freed, the libraries stay.  Byte/Word to Long is exact and stays;
   not covered, BUG(orig) (#1183): a compound left operand
   (q*2) and a Long constant under a Float context are evaluated in the
   wanted type before the comparison operator is read, so a Quick inside
   is cut (`r.l=(q*2>2.5)` with q=1.5 gives 0).  *type is the type the
   operand has then. */
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

    if ((op != opdiv && op != opmod) || type < T_BYTE || type > T_FLOAT)
        return;                     /* doop refuses the other types */
    if (!foldnow || nomemleft)
        return;                     /* not folded: the program divides */
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
   handler (Fixed (#1209): quickwhole for a Quick) */
UWORD doandb(UWORD t) { doplus2(0xc000); return t; }
UWORD doandw(UWORD t) { doplus2(0xc040); return t; }
UWORD doandl(UWORD t) { doplus2(0xc080); quickwhole(t); return t; }
UWORD doorb(UWORD t) { doplus2(0x8000); return t; }
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

/* blitz2.s:9510 doeorb - falls through into doeor2 */
UWORD doeorb(UWORD t) { doeor2(0xb100); return t; }
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
    emitact(pokeact, NULL, opcode, 0);
    regat--;
    c = getelement2(type);
    regat++;
    emitact(pokeact, NULL, ACT_TWO | (*type < 3 ? 0x3e06 : 0x2e06), /* move.x d6,d7 */
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

    if (n->flags & EXF_ABSORBED)
        return;                     /* in the literal's FFP value (#1186) */

    for (k = n->op; k; k--) {
        if (a->y >= 5)
            fdo2(-60);              /* SPNeg */
        else
            pokewd((a->y >= 3 ? 0x4480 : 0x4440) | regat);  /* neg.l / neg.w */
    }
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

    if (a->y >= 3)
        opcode ^= 0xc0;             /* not.l */
    pokewd(opcode);
    quickwhole(a->y);               /* Fixed (#1209): Not of floor(q) */
}

/* blitz2.s:9688 notit - do NOT. (applies to the whole rest of the
   expression: eval4, whose node is the NOT's) */
static UWORD notit(UWORD *type)
{
    UWORD c = eval4(type, EXN_NOT);

    if (rawq) {                     /* Fixed (#1178): Not of floor(q) */
        emitact(cutact, NULL, 0, 0);
        rawq = FALSE;
    }
    if (*type >= 5)
        illoperr();
    emitact(notemit, NULL, 0, *type);
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

/* blitz2.s:9719 getelement2 after its bsr getchar3 - the element that
   begins with c (negate reads a run of minus signs itself) */
static UWORD element(UWORD c, UWORD *type)
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
