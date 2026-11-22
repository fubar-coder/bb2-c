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
#include <proto/exec.h>
#include <string.h>

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
struct precent precstack[64];           /* 9173 ds 6*32 */
struct precent *precsp = precstack;     /* 9171 */
struct forthent forthstack[32];         /* 9715 ds.b 18*32 */
struct forthent *forthsp = forthstack;  /* 9716 */
WORD fuckpos = 0;           /* 10303 */
UWORD evaldepth = 0;        /* no asm counterpart (eval.h) */
/* Fixed (#978), no asm counterpart: pc after the code of the last byte
   value varcont fetched (with its conversion), for eval5 */
static ULONG bytevalend = 0;
/* Fixed (#1177), no asm counterpart: the last conversion varcont emitted
   (code at..end, the element's own type from, the wanted type to, the
   relocation list and lasta6 before it, the evaldepth of the element), for
   the left operand of a comparison in eval5 */
static struct {
    ULONG at, end;
    struct offnode *off;
    WORD a6;
    UWORD from, to, depth;
} conv;
/* the intrinsics (no asm counterpart, see intrinsics[]): the number of
   elements getelement2 has begun, and where the right side of the last
   assignment began (intrinsic_let: its evaldepth and the number of its
   first element) */
static ULONG elemcount = 0;
static UWORD letdepth = 0;
static ULONG letelem = 0;

/* forward declarations of the internal routines */
static void fdo2(WORD lvo);
static void bytetof2(void);
static void qtof2(UWORD tok);
static void floattolong2(void);
static void wordtoquick_(void);
static void quicktob2(void);
static UWORD eval3(UWORD *type);
static UWORD eval4(UWORD *type);
static UWORD eval5(UWORD *type, UWORD *c, struct opentry **op);
static BOOL intrinsic_quick(UWORD type);
static UWORD eval2(UWORD *type, UWORD *c, struct opentry **op);
static UWORD doop(struct opentry *op, UWORD type);
static ULONG cmpownleft(UWORD *type, ULONG lstart, ULONG lend);
static UWORD compareop(struct opentry *op, UWORD left, UWORD right);
static void divzerochk(const struct opentry *op, UWORD type);
static void pushprec(struct opentry *op, UWORD prec);
static struct opentry *popprec(UWORD *prec);
static UWORD getelement(UWORD *type);
static UWORD getelement2(UWORD *type);
static void nocando(void);
static void domylib(UWORD tok);
static void doflib(WORD lvo);
static void makesbase(void);
static UWORD varcont(UWORD oldtype, UWORD *type);
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

/* blitz2.s:8809 arrevalchk - falls through into arrszchk */
UWORD arrevalchk(void)
{
    UWORD c;

    if (!(debugga >> 8))
        return sharreval();
    c = arreval();
    arrszchk();
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

/* blitz2.s:8879 maineval2 - d2=type to get, d1=push flag */
UWORD maineval2(WORD push, UWORD *type, WORD unk)
{
    WORD s_thisstuse = thisstuse;
    WORD s_pushflag, s_sbgot, s_unknown;
    UWORD asked, t, c;

    thisstuse = stackuse;
    s_pushflag = pushflag;
    pushflag = push;
    s_sbgot = sbgot;
    sbgot = 0;                      /* not pushed yet */
    s_unknown = unknown;
    unknown = unk;

    asked = *type;
    t = asked;
    evaldepth++;
    c = eval3(&t);
    evaldepth--;
    if (asked && !unknown) {        /* type asked for */
        convtype(t, asked);         /* exg d2,d3 / convtype */
        t = asked;
    }
    unknown = s_unknown;            /* .noconv */
    if (pushflag == 2)              /* the argument of an intrinsic */
        pushflag = intrinsic_quick(t) ? 0 : 1;
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
    sbgot = s_sbgot;                                /* .notst2 */
    if (pushflag < 0) {
        UWORD opcode = *pushd0wd;
        stackuse += 2;
        if (t >= 3) {
            stackuse += 2;
            opcode = *pushd0l;
        }
        pokewd(opcode | regat);
    }
    pushflag = s_pushflag;                          /* .skip */
    thisstuse = s_thisstuse;
    *type = t;
    return c;
}

/* blitz2.s:8968 eval3 */
static UWORD eval3(UWORD *type)
{
    UWORD c = eval4(type);
    forthsp--;                      /* sub.l #18,forthsp */
    return c;
}

/* blitz2.s:8972 eval4 */
static UWORD eval4(UWORD *type)
{
    struct opentry *op;
    UWORD c;

    /* asm: a0 is whatever it was (garbage) for the sentinel */
    pushprec(NULL, 1);
    if (eval5(type, &c, &op))
        syntaxerr();
    precsp--;                       /* pop the prec set up */
    return c;
}

/* blitz2.s:9017 the constant fold of eval5: the code of the last two
   operands (both constants: forthsp[-2], forthsp[-1]) and their operator
   is run and replaced by a move of the result (of type `type`) into
   regat.  Not inlined: eval5 recurses once per nesting level, and the
   register frame of the call would be part of every level's stack frame
   (#1080). */
static __attribute__((noinline)) void constfold(UWORD type)
{
    struct forthent *fs = forthsp;
    struct m68kregs r;
    struct offnode *node, *keep, **link;
    ULONG value, foldend;
    UWORD opcode;

    /* Set up MOVE.l regat,d0 */
    pokewd(0x2000 | regat);
    pokewd(0x4e75);                         /* rts */
    foldend = pc;                           /* end of the folded code */
    /* Fixed (#343): only the word lasta6 is restored.  The original's long
       move (blitz2.s:9025) also restored quoteflag as getelement2 saved it
       - after getchar3 had read the left operand's first char: for a
       character constant ("x"*8, only in asmfixer: constmode) the opening
       quote, so quoteflag stayed set after the expression and the operands
       evaluated next (earlier lines, insasm prepends) lost their spaces
       inside quotes: #" " gave 0. */
    lasta6 = fs[-2].lasta6w;
    pc = fs[-2].pc;
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
    fraclitdrop();                  /* #1186 */
    /* .mode - clean up offset stack.  Fixed (#1049): only the offsets
       inside the folded code (fs[-2].pc up to foldend) are freed.  The
       original freed every offset added since the fold began, also the
       relocations of a library that uselib3 linked in meanwhile (its code
       lies in the library area, not in the program code): a constant Mod
       as the first use of modlib left modlib's calls of ldivlib and
       lmullib at their compile-time addresses in the executable. */
    keep = fs[-2].firstoff;
    link = &firstoff;
    while ((node = *link) != keep) {
        if (node->offset >= fs[-2].pc - pcat
            && node->offset < foldend - pcat) {
            *link = node->next;
            numoffs--;
            FreeMem(node, 8);
        } else
            link = &node->next;
    }
}

/* blitz2.s:8983 eval5 - precedence climbing.  Returns the precedence of
   the pending operator (asm d1, 0 = none), *op = that operator (a0),
   *c = current char (d0). */
static UWORD eval5(UWORD *type, UWORD *c, struct opentry **op)
{
    ULONG lstart = pc;              /* the left operand: lstart..lend */
    UWORD prec = eval2(type, c, op);
    ULONG lend = pc;

    for (;;) {
        UWORD left, nprec;
        struct opentry *nop, *cur;
        struct forthent *fs;
        BOOL cmp, cmpop, lbyte, lconst;
        ULONG rstart;

        if (!(prec > precsp[-1].prec))  /* cmp -(a1),d1 / bhi .higher */
            return prec;
        /* .higher */
        /* Fixed (#1185): the right operand of an operator whose left
           operand is in d7 would need a register beyond d7 (the 7th and
           later arguments of a statement are evaluated in d6, so
           Cmd a1,..,a6,x+y*z is enough, or eight nested levels).  The
           original fetched it into d7 over the left operand and took the
           operand from "d8", a register field that reads a0 or changes
           the opcode: a wrong value, a crash of the program, or of the
           compiler when constant folding ran that code.  The expression
           stack's error instead, before any code of the operator
           or its right operand. */
        if (regat >= 7)
            conmemerr();
        /* Fixed (#1177): the operands of a comparison are compared with
           their own values, not in the type the expression is wanted in
           (the original handed that type to both, and varcont cut a Quick
           under a Long: r.l=(q>1) was 0 for q=1.5).  A left operand of one
           element gets its own type back (cmpownleft), the right operand
           starts at type 0, compareop compares the two. */
        cmpop = *op >= opeq && *op <= opge;
        if (cmpop && lend)
            lend = cmpownleft(type, lstart, lend);
        /* Fixed (#978): an equality comparison (= or <>) of a byte value
           with a constant from 128 to 255 (each operand one element)
           takes the constant as the byte it stands for, sign-extended as
           varcont extends the byte: "a.b=$90 : If a=$90" matches.  The
           ordering comparisons stay mathematical (a<200 is true for every
           byte). */
        cmp = *op == opeq || *op == opne;
        lbyte = lend && bytevalend == lend && bytevalend > lstart;
        lconst = lend && isbyteconst(lstart, lend);
        rstart = pc;
        pushprec(*op, prec);
        left = *type;
        regat++;
        if (cmpop)
            *type = 0;              /* Fixed (#1177) */
        nprec = eval5(type, c, &nop);
        regat--;
        if (cmp && lbyte && isbyteconst(rstart, pc))
            byteconstext(rstart);
        else if (cmp && lconst && bytevalend == pc && bytevalend > rstart)
            byteconstext(lstart);
        lstart = lend = 0;          /* the left operand is a result now */
        cur = popprec(&prec);       /* .skip */
        if (cmpop)
            *type = compareop(cur, left, *type);
        else {
            if (left)
                convtypef(left, *type); /* could possibly fuck regat+1 ! */
            divzerochk(cur, *type);
            *type = doop(cur, *type);
        }
        /* O.K., lets check the forth stack to see if last two operators
           are constants!  if so, we can JSR the routine to optimize into a
           constant! */
        fs = forthsp;
        fs[-2].constgot |= fs[-1].constgot;
        if (!fs[-2].constgot && !nomemleft) {
            /* YES! We can do it! */
            constfold(*type);
            fs = forthsp;
        }
        forthsp = fs - 1;                           /* .no */
        prec = nprec;
        *op = nop;
        *c = reget();
    }
}

/* blitz2.s:9085 eval2 - get 1 element and operator.
   Returns op prec (asm d1, 0 if .done), *op = operator address (a0). */
static UWORD eval2(UWORD *type, UWORD *c, struct opentry **op)
{
    struct opentry *ent;
    UWORD ch;

    ch = *c = getelement(type);
    for (ent = operators; ent->op; ent++)
        if (ent->op == ch)
            goto found;
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
    *op = ent;
    return ent->prec;
}

/* blitz2.s:9142 doop - a0=operator,d2=type */
static UWORD doop(struct opentry *op, UWORD type)
{
    ophandler *h;

    if ((WORD)(type - 1) < 0)
        illoperr();
    h = op->h[type - 1];
    if (!h)
        illoperr();
    return h(type);                 /* jmp (a0) */
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
   doop: pc, bigpc and nomemleft are restored, no code changes.  Not
   inlined: its register frame would be part of eval5's frame on every
   level of the recursion (#1080). */
static __attribute__((noinline))
void divzerochk(const struct opentry *op, UWORD type)
{
    struct forthent *fs = forthsp;
    struct m68kregs r;
    ULONG at = pc, big = bigpc, value;
    WORD s_nomemleft = nomemleft;
    BOOL zero;

    if ((op != opdiv && op != opmod) || type < T_BYTE || type > T_FLOAT)
        return;                     /* doop refuses the other types */
    if ((fs[-2].constgot | fs[-1].constgot) || nomemleft)
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
    callregs((APTR)fs[-2].pc, &r);
    value = r.d[0];
    pc = at;
    bigpc = big;
    if (type < T_LONG)
        zero = (UWORD)value == 0;
    else if (type == T_FLOAT)
        zero = (value >> 8) == 0;
    else
        zero = value == 0;
    if (zero)
        divzeroerr();
}

/* blitz2.s:9153 pushprec - push operator on stack */
static void pushprec(struct opentry *op, UWORD prec)
{
    /* DIFF: the asm has no overflow check here (popprec checks later,
       after the write past the end); C stops before writing */
    if (precsp >= precstackf)
        pserr();
    precsp->op = op;
    precsp->prec = prec;
    precsp++;
}

/* blitz2.s:9161 popprec - pull precedence into d1 from stack */
static struct opentry *popprec(UWORD *prec)
{
    if (precsp >= precstackf)
        pserr();
    precsp--;
    *prec = precsp->prec;
    return precsp->op;
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

/* Fixed (#1177), no asm counterpart: the left operand lstart..lend of a
   comparison, when it is one element (a variable, field, array element,
   function value) whose code ends in a conversion varcont made to the
   wanted type, gets its own type back: the conversion is dropped (with its
   relocations, as constfold frees them, and lasta6 as before it) when it
   changes how the value is held - Quick to Long (the cut), a whole number
   to Quick, a whole number or a Quick to Float.  Byte/Word to Long is exact
   and stays.  #1186 (port): a decimal literal with a point (also
   negated) that the wanted Byte cut to a whole number is a Float again
   (fraclitfloat): rb.b=(1.5=1.5) compared 1 with 1.5.  Not covered
   (#1183): a compound left operand (q*2) and -q, whose code goes on after
   the conversion.  Returns the new end of the operand. */
static __attribute__((noinline))
ULONG cmpownleft(UWORD *type, ULONG lstart, ULONG lend)
{
    struct offnode **link = &firstoff, *node;
    UWORD from = conv.from, to = conv.to;

    if (fraclitfloat(lstart, &lend, regat, type))
        return lend;
    if (conv.end != lend || conv.at <= lstart || conv.depth != evaldepth)
        return lend;
    if (!((from == T_QUICK && to == T_LONG)
          || (from >= T_BYTE && from <= T_LONG && to == T_QUICK)
          || (from >= T_BYTE && from <= T_QUICK && to == T_FLOAT)))
        return lend;
    while ((node = *link) != conv.off && node) {
        if (node->offset >= conv.at - pcat) {
            *link = node->next;
            numoffs--;
            FreeMem(node, 8);
        } else
            link = &node->next;
    }
    pc = conv.at;
    lasta6 = conv.a6;
    if (bytevalend == lend)
        bytevalend = pc;            /* a byte keeps its ext.w (#978) */
    conv.end = 0;
    *type = from;
    return pc;
}

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
   convtypef as eval5 did) and the handler of the common type compares. */
static __attribute__((noinline))
UWORD compareop(struct opentry *op, UWORD left, UWORD right)
{
    if ((left == T_LONG && right == T_QUICK)
        || (left == T_QUICK && right == T_LONG))
        return cmplq(op, left);
    if (left && right < left) {
        regat++;
        convtype(right, left);
        regat--;
        right = left;
    }
    if (left)
        convtypef(left, right);     /* could possibly fuck regat+1 ! */
    return doop(op, right);
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

/* blitz2.s:9428 dopow - both to float, then the math lib */
UWORD dopow(UWORD t)
{
    convtypef(t, T_FLOAT);          /* now a float */
    regat++;
    convtype(t, T_FLOAT);
    regat--;
    nocando();
    domylib(mathq * 128 + 1);
    return T_FLOAT;
}

/* blitz2.s:9441 nocando - can't eval this as a const! */
static void nocando(void)
{
    forthsp[-1].constgot = 0xffff;
}

/* ------------------------------------------------------------------ MOD */

/* blitz2.s:9449 domodb */
UWORD domodb(UWORD t) { domylib(modbyte); return t; }
/* blitz2.s:9452 domodw */
UWORD domodw(UWORD t) { domylib(modword); return t; }

/* blitz2.s:9455 domodl - modlib uses an alibjsr here! */
UWORD domodl(UWORD t)
{
    nocando();
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
    nocando();
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
UWORD dodivl(UWORD t) { domylib(longdiv); return t; }
/* blitz2.s:9485 dodivq */
UWORD dodivq(UWORD t) { domylib(quickdiv); return t; }
/* blitz2.s:9487 dodivf */
UWORD dodivf(UWORD t) { doflib(-84); return t; }

/* ----------------------------------------------------------- AND / OR */

/* blitz2.s:9492-9506 */
UWORD doandb(UWORD t) { doplus2(0xc000); return t; }
UWORD doandw(UWORD t) { doplus2(0xc040); return t; }
UWORD doandl(UWORD t) { doplus2(0xc080); return t; }
UWORD doorb(UWORD t) { doplus2(0x8000); return t; }
UWORD doorw(UWORD t) { doplus2(0x8040); return t; }
UWORD doorl(UWORD t) { doplus2(0x8080); return t; }

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
/* blitz2.s:9521 doeorl */
UWORD doeorl(UWORD t) { doeor2(0xb180); return t; }

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
    { tok_opand, { doandb, doandw, doandl, doandl, 0, 0, 0 }, 1000 }, /* AND */
    { tok_opor, { doorb, doorw, doorl, doorl, 0, 0, 0 }, 1000 },      /* OR */
    { tok_opeor, { doeorb, doeorw, doeorl, doeorl, 0, 0, 0 }, 1000 }, /* EOR */
    { tok_oplsl, { dolslb, dolslw, dolsll, dolsll, 0, 0, 0 }, 6000 }, /* LSL */
    { tok_opasl, { dolslb, dolslw, dolsll, dolsll, 0, 0, 0 }, 6000 }, /* ASL */
    { tok_oplsr, { dolsrb, dolsrw, dolsrl, dolsrl, 0, 0, 0 }, 6000 }, /* LSR */
    { tok_opasr, { doasrb, doasrw, doasrl, doasrl, 0, 0, 0 }, 6000 }, /* ASR */
    { 0x8006 + tnum, { domodb, domodw, domodl, domodq, domodf, 0, 0 }, 5000 }, /* Mod */
    { '^', { dopow, dopow, dopow, dopow, dopow, 0, 0 }, 7000 },
    /* [15] opeq */ { '=', { doeqb, doeqw, doeql, doeql, doeqf, doeql, doeqs }, 2000 },
    /* [16] opne */ { 0x3c3e, { doneb, donew, donel, donel, donef, donel, dones }, 2000 },
    /* [17] oplt */ { '<', { doltb, doltw, doltl, doltl, doltf, doltl, dolts }, 2000 },
    /* [18] ople */ { 0x3c3d, { doleb, dolew, dolel, dolel, dolef, dolel, doles }, 2000 },
    /* [19] opgt */ { '>', { dogtb, dogtw, dogtl, dogtl, dogtf, dogtl, dogts }, 2000 },
    /* [20] opge */ { 0x3e3d, { dogeb, dogew, dogel, dogel, dogef, dogel, doges }, 2000 },
    { 0x8000 + 26 + tnum, { bittst, bittst, bittst, bittst, 0, 0, 0 }, 8000 },
    { 0x8000 + 27 + tnum, { bitset, bitset, bitset, bitset, 0, 0, 0 }, 8000 },
    { 0x8000 + 28 + tnum, { bitclr, bitclr, bitclr, bitclr, 0, 0, 0 }, 8000 },
    { 0x8000 + 29 + tnum, { bitchg, bitchg, bitchg, bitchg, 0, 0, 0 }, 8000 },
    { 0, { 0 }, 0 },
};

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
   keep the old d6 on the stack.  Only regat 7 comes here: eval5 refuses an
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
    pokewd(opcode);
    regat--;
    c = getelement2(type);
    regat++;
    pokewd(*type < 3 ? 0x3e06 : 0x2e06);    /* move.x d6,d7 */
    pokewd((opcode & 0xf000) | 0x0c1f);     /* move.x (a7)+,d6 */
    return c;
}

/* Fixed (#1070): the element getelement2 fetches next is negated (an odd
   number of '-' in front of it); it hands a literal's sign on as litneg */
static BOOL negnext;

/* blitz2.s:9667 negate */
static UWORD negate(UWORD *type)
{
    UWORD c = getelement2(type);
    UWORD opcode = 0x4440;          /* neg.w */

    if (*type >= 3) {
        opcode = 0x4480;            /* neg.l */
        if (*type >= 5) {
            if (*type >= 6)
                illoperr();
            fdo2(-60);              /* SPNeg */
            return c;
        }
    }
    pokewd(opcode | regat);         /* .doneg */
    return c;
}

/* blitz2.s:9681 bracket */
static UWORD bracket(UWORD *type)
{
    UWORD c = eval4(type);

    if (c != ')' && c != '}')
        syntaxerr();
    return getchar3();
}

/* blitz2.s:9688 notit - do NOT. (applies to the whole rest of the
   expression: eval4) */
static UWORD notit(UWORD *type)
{
    UWORD c = eval4(type);
    UWORD opcode;

    if (*type >= 5)
        illoperr();
    opcode = regat | 0x4640;        /* not.w */
    if (*type >= 3)
        opcode ^= 0xc0;             /* not.l */
    pokewd(opcode);
    return c;
}

/* blitz2.s:9700 fetchpi - return Pi.f */
static UWORD fetchpi(UWORD *type)
{
    *type = T_FLOAT;
    pokewd((regat << 9) | 0x203c);  /* move.l #x,dn */
    pokel(0xc90fda42);
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
    UWORD c = getchar3();
    struct forthent *f;
    BOOL neg = negnext;             /* Fixed (#1070) */

    negnext = FALSE;
    conv.end = 0;                   /* Fixed (#1177): no conversion yet */
    elemcount++;                    /* for the intrinsics */
    if (c & 0x8000)
        ext_evalvalue(0, 0);        /* a token: a value without a type */
    if (c == '(' || c == '{')
        return bracket(type);
    if (c == '-') {
        negnext = !neg;
        return negate(type);
    }
    if (c == tok_opnot)
        return notit(type);

    f = forthsp;
    if (f >= &forthstack[32])       /* cmp.l #forthsp,a1 */
        conmemerr();
    /* stack contains..... pc.l,firstoff.l,optat.l,lasta6.w,0,constgot.w! */
    f->pc = pc;
    f->firstoff = firstoff;
    f->lasta6q = L_(lasta6qf);
    f->lasta6w = lasta6;
    f->zero = 0;
    f->constgot = 0;
    forthsp = f + 1;

    /* First, we have stuff which won't fuck constgot */
    litneg = neg;
    if (tstnum(c))
        return (UWORD)fetchnum(c, type, FALSE);
    if (c == '.')
        return (UWORD)fetchfrac(c, type, FALSE);
    litneg = FALSE;
    if (c == '#')
        return fetchconst(type);
    if (c == '$')
        return (UWORD)fetchhex(type, FALSE);
    if (c == '%')
        return (UWORD)fetchbin(type, FALSE);
    if (c == 0x8000 + 47)
        return fetchsize(type);
    if (c == 0x8007 + tnum)
        return fetchpi(type);
    if (c == 0x8000 + tnum + 18)
        return fetchon(type);
    if (c == 0x8000 + tnum + 19)
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
    f->constgot = ~f->constgot;     /* .asmok: not -(a1) - non-const */

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

/* blitz2.s:9910 ampersand2 */
UWORD ampersand2(UWORD *type)
{
    UWORD namelen;          /* asm d2 after getvname */
    UWORD c, vartype;
    struct var *v;
    struct type *t;
    WORD off = 0;

    c = getchar3();
    c = getvname(c, &namelen);
    v = fetchvar(namelen, &t, &c);
    vartype = calcvar(&v, &t, &c, &off);
    *type = vartype;
    ext_evalvalue(t ? t->u.code : 0, flagmask);    /* the variable's type */

    if ((vartype & 0xff) == T_STRING) {
        if (vartype & 0x8000) {                     /* .algot2 */
            pokewd((regat << 9) | *stamp);          /* move.l (a2),Dn */
            return c;
        }
        stamp2[1] = off;                            /* move d3,stamp2+2 */
        pokela5s(L_(stamp2));                       /* move.l x(a5),a2 */
    } else if (!(vartype & 0x8000)) {               /* .notst */
        leaamp[1] = off;
        pokela5s(L_(leaamp));                       /* lea x(a5),a2 */
    }
    pokewd((regat << 9) | *leaamp2);                /* .algot: move.l a2,Dn */
    return c;
}

/* blitz2.s:9941 doern - read err number.
   NB(orig): no getchar; the current char stays the token. */
static UWORD doern(UWORD c, UWORD *type)
{
    tokejsr(ern);
    *type = T_LONG;
    return c;
}

/* blitz2.s:9952 fetchit - token of an object lib; returns the lib
   (asm a2, d0 = offset of its max info) */
static struct blitzlib *fetchit(void)
{
    UWORD c = getchar3();
    struct blitzlib *lib;

    if (!(c & 0x8000))
        syntaxerr();
    findtoke(c & 0x7fff, &lib);
    if (!LIBMAX_SHIFT(lib))         /* tst -2(a2,d0.l) */
        freeerr();
    return lib;
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

/* blitz2.s:9989 doused - return 'used' object.
   Fixed (#998): the number is (used pointer - table base) >> shift; the
   asm shifts with lsr.w (also both shifts of usedcode2), so from
   n << shift = $10000 on the high word of the difference was lost (Used
   BitMap after Use BitMap 1025 gave 1).  Where objlong (tokens1.c) says
   the object offset is a long, the shifts are lsr.l; with smaller maxima
   (all defaults) the code is unchanged. */
static UWORD doused(UWORD *type)
{
    struct blitzlib *lib = fetchit();
    UWORD shift = LIBMAX_SHIFT(lib);
    UWORD *start, *end, maxoff;
    UWORD lsr = objlong(LIBMAX_E(lib)) ? 0xE088 : 0xE048; /* lsr.l / lsr.w #8,d0 */

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
    maxoff = LIBMAX_VAROFF(lib);    /* .skip2 */
    start[(ucodeskip - usedcode) + 1] = maxoff;
    start[1] = maxoff + 4;
    if (regat)
        pokewd(*usedprep);
    pokecode(start, end);           /* .skip3 */
    if (regat) {
        pokewd((regat << 9) | *usedfix);
        pokewd(*useddone);
    }
    *type = T_WORD;                 /* .skip4 */
    return getchar3();
}

/* blitz2.s:10042 domaximum - number of maximums, eg a=Maximum Shape */
static UWORD domaximum(UWORD *type)
{
    struct blitzlib *lib = fetchit();
    ULONG count = (UWORD)LIBMAX_COUNT(lib);

    getchar3();
    return (UWORD)fetchnum2(count, type, FALSE);
}

/* blitz2.s:10053 doaddr - address of something, eg a.l=Addr Window(0).
   Fixed (#939): the offset is added as a long where objlong (tokens1.c)
   says so; Addr Shape(1025) was 32 KB before the shape table. */
static UWORD doaddr(UWORD *type)
{
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
    addrcode[1] = *(UWORD *)(e - 6);                    /* x(a5) */
    addrcode[2] = (objlong(e) ? 0xD1C0 : 0xD0C0) | regat; /* add(.l) Dregat,a0 */
    addrcode[3] = (addrcode[3] & 0xf1ff) | (regat << 9);/* move.l a0,Dregat */
    pokecode(addrcode, addrcodef);
    *type = T_LONG;                 /* now a long */
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

/* blitz2.s:10183 amigalib - Amiga OS library function call.
   Fixed (intended deviation, #958): parameters in a4/a5 no longer
   clobber the variable bases, as in stamigalib (see amigaa4a5).
   Fixed (#344): parameter registers holding intermediates are saved,
   see amigasave. */
static UWORD amigalib(UWORD oldtype, UBYTE *sub, UWORD *type)
{
    struct blitzlib *ol;
    ULONG push, pull;
    UWORD base, lvo, first, c, n;
    UBYTE *p, *regs;
    WORD i;
    BOOL a45;

    ol = uselib(W_(sub + 6));       /* get lib for base address */
    base = ol->varoff;              /* libbase reg */
    p = sub + 8;
    lvo = W_(p);                    /* get offset for lib */
    p += 2;
    regs = p;
    savereg(&push, &pull);
    amigasave(regs, &push, &pull);  /* fixed (#344) */
    pokemovem(push);
    a45 = amigaa4a5(regs);          /* fixed (#958): movem.l a4/a5,-(a7) */
    c = first = getchar3();         /* remember first bracket.... */
    /* collect longs for lib */
    n = 0;
    for (;;) {
        UWORD t;
        if ((BYTE)*p++ < 0)
            break;                  /* .done */
        t = T_LONG;
        n++;
        c = peval(&t);
        stackuse -= 4;              /* readjust stack! it's coming off later! */
        if (c == ',')
            continue;
        if ((BYTE)*p >= 0)
            syntaxerr();
        break;
    }
    c = tstbras(first, c, n);       /* .done */
    libjsr[1] = lvo;
    libbase[1] = base;
    if (a45)
        pokel(L_(libbase));         /* fixed (#958): a6 before a5 is popped */
    p = regs;
    i = (WORD)n - 1;
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
    pokel(L_(libjsr));
    if (a45)
        pokel(0x4cdf3000UL);        /* fixed (#958): movem.l (a7)+,a4/a5 */
    if (regat)
        pokewd((regat << 9) | 0x2000);
    pokemovem(pull);                /* .nofin */
    *type = T_LONG;
    reget();
    (void)c;
    return varcont(oldtype, type);
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
   intrstrvar before dolibtoke, which compiles the argument and the call
   as before unless it is a plain string variable.

   The way through the shared code: libfunction arms the entry before
   dolibtoke; dolibtoke compiles the argument as for
   the call, getap asks intrinsic_argpush for the push flag of the
   argument (2 instead of evalu2's 1), and maineval2, when the argument's
   type is known, asks intrinsic_quick: for a Quick the entry accepts it
   pushes no type word, so stackuse stays 0 and dolibtoke sets up no a2;
   dolibtoke then asks intrinsic_code instead of making the JSR (userjsr,
   with the error check routine of the runtime error checking mode), and
   the entry's code works on d0, the argument (libfunction's regat = 0).
   Everything before (libfunction's movem of the registers below regat)
   and after (move.l d0,D(regat), the movem back, varcont) is the
   call's.  For every other type the argument and the call are compiled
   exactly as before.  Not in constant evaluation (cfetchmode: the code
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
   library sets a3 to the copy, which begins where a3 was: the same a3
   (design.md Decision 4).  a0 changes, as with the copy (copstring). */
static void lenvar(void)
{
    pokewd(0x6706);                 /* beq.s 1f (no memory: 0) */
    pokewd(0x2040);                 /* move.l d0,a0 */
    pokewd(0x2028);                 /* move.l -4(a0),d0 */
    pokewd(0xfffc);
}                                   /* 1: */

/* Asc(a$) of a plain string variable (stringfunclib ascii): the first
   character zero-extended, -1 for the length 0 and for a variable without
   memory - the library's long; a3 and a0 as Len's */
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

/* the state of the armed intrinsic (intrcur): INTR_ARMED until getap
   takes its argument, INTR_ARG while maineval2 compiles it, INTR_QUICK
   when maineval2 accepted it (no type word), INTR_DONE when
   intrinsic_code emitted the code.  Nested calls in the argument save and
   restore it (libfunction). */
enum { INTR_IDLE, INTR_ARMED, INTR_ARG, INTR_QUICK, INTR_DONE };
static const struct intrinsic *intrcur = NULL;
static UWORD intrstate = INTR_IDLE;

/* the entry of intrinsics[] for the library function token tok (its sub
   from findtoke, called with np parameters), if it may be used here */
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
   Else FALSE: nothing is read, the call is compiled as before. */
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
    pokewda5s(0x202d);              /* move.l off(a5),d0 (a local: a4) */
    pokewd((UWORD)off);
    in->emit();
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

/* blitz2.s:10112 libfunction - do a user library function, eg
   a=pixel{x,y}.
   Fixed (intended deviation, #46): a function whose !libs list loads a3
   (an object, $13xx) inside a string expression saves a3, the end of
   the string being built (sbasegot, the criterion afunction uses for
   pushing a3 and amigasave for #344), with the registers below regat in
   the movem before the parameters, and gets it back with them after the
   call.  The original saves only the registers below regat: the rest of
   the expression wrote its characters from the object on (BlitColl's
   Shape, PeekSound's Sound).  A function returning a string leaves its
   result at a3 and is compiled as before, as is every call outside a
   string expression.
   tok: the library token (function), for the intrinsics: an entry of
   intrinsics[] is armed for the argument (no asm counterpart). */
static UWORD libfunction(UWORD oldtype, UWORD flags, struct blitzlib *lib,
                         UBYTE *sub, UWORD tok, UWORD *type)
{
    UWORD rettype, mask, c, np, sregat, sstate;
    const struct intrinsic *in, *scur;

    rettype = flags >> 8;
    if (B_(sub + 1) & 8)
        rettype = sizespec2();
    if (rettype == T_STRING)        /* .no0 */
        makesbase();
    if (sbasegot && rettype != T_STRING && liba3(sub)) {
        /* fixed (#46): the movem regs and a3 */
        mask = 0;
        for (c = regat; c; c--)
            mask = (mask >> 1) | 0x8000;
        mask |= 1 << 4;             /* a3, predecrement order */
        pokewd(0x48e7);
        pokewd(mask);
    } else
        mask = savem();             /* .notst0: the movem regs */
    c = getchar3();
    np = 0;
    if (c == '(') {
        np = countpees(c);
        if (!np)
            syntaxerr();
    } else
        bakup();                    /* .nopars */
    sregat = regat;                 /* .skip0 */
    regat = 0;
    if (B_(sub + 1) & 8) {
        pokewd(rettype | 0x7000);   /* moveq #type,d0 */
        regat++;
    }
    in = intrfind(tok, sub, np, oldtype);
    scur = intrcur;                 /* an intrinsic around this call */
    sstate = intrstate;
    intrcur = in;
    intrstate = in ? INTR_ARMED : INTR_IDLE;
    if (in && in->argtype == T_STRING) {
        intrstate = INTR_IDLE;      /* nothing for getap to arm */
        if (intrstrvar(in, &c))
            intrstate = INTR_DONE;
        else
            c = dolibtoke(np, lib, sub);
    } else
        c = dolibtoke(np, lib, sub);    /* .no2 */
    if (intrstate == INTR_DONE)
        rettype = in->restype;      /* compiled inline */
    intrcur = scur;
    intrstate = sstate;
    if (np) {
        if (c != ')')
            syntaxerr();
        getchar3();
    }
    regat = sregat;                 /* .skip */
    if (regat)
        pokewd((regat << 9) | 0x2000);  /* move.l d0,regat */
    if (mask) {                     /* .skip2 */
        pokewd(0x4cdf);
        pokewd(revmask(mask));      /* poke movem (a7)+... */
    }
    *type = rettype;                /* .nomovem */
    if (rettype == T_STRING)
        pokewd(*putstlen | regat);
    return varcont(oldtype, type);  /* .notst */
}

/* blitz2.s:10087 function - do a function */
static UWORD function(UWORD c, UWORD *type)
{
    struct blitzlib *lib;
    UBYTE *sub;
    UWORD old, fl;

    if (c <= 0x8000 + tnum)
        syntaxerr();
    /* .overasm */
    if (c == 0x8003 + tnum)
        return doern(c, type);
    if (c == 0x8005 + tnum)
        return doaddr(type);
    if (c == 0x8011 + tnum)
        return domaximum(type);
    if (c == 0x801e + tnum)
        return doused(type);
    ext_newtoken(c & 0x7fff);
    old = *type;                    /* remember old type! */
    sub = findtoke(c & 0x7fff, &lib);
    fl = W_(sub);
    if (!(fl & 2))
        syntaxerr();
    if (fl & 4)
        return amigalib(old, sub, type);
    return libfunction(old, fl, lib, sub, c & 0x7fff, type);
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

/* blitz2.s:10391 makelit - put lit string from d3 (data1) into stringbuff */
static void makelit(void)
{
    pokewd(*numtoa0);
    addoff();
    pokel(litdata1);
    pokewd(*pushlen);
    tokejsr(copstring);
}

/* blitz2.s:10327 litstring */
static UWORD litstring(UWORD *type)
{
    UWORD len = 0, c;

    makesbase();
    litdata1 = data1;
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
    makelit();
    *type = T_STRING;
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

/* blitz2.s:10353 makealab2 */
struct label *makealab2(UWORD len, UWORD *c)
{
    struct node *last;
    struct label *l = findadd(len, &last);
    struct labref *r;

    (void)c;
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
    /* .makeit: create an oustanding reference to this label! */
    r = doallocerr(12, MEMF_PUBLIC);
    r->next = l->refs;
    l->refs = r;
    r->addr = pc + 2;
    r->procnum = procnum;
    r->line = linenumat;
    return l;
gotit:
    if ((ULONG)l->refs & 1)
        illlaberr();
    if (procnum != l->procnum)
        referr();
    return l;
}

/* blitz2.s:10406 qmark - ?label */
static UWORD qmark(UWORD *type)
{
    struct label *l;

    getchar3();
    l = makealab(NULL);
    pokewd((regat << 9) | 0x203c);  /* create move.l #loc,dn */
    addoff();
    pokel(l->pc);
    *type = T_LONG;
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

/* blitz2.s:10641 variable2 - get the value of a var into D(regat).
   vartype = calcvar result (asm d2), off = a5/a4 offset (asm d3) */
static UWORD stvar(UWORD vartype, WORD off, UWORD *type);
static void variable2(UWORD vartype, WORD off, UWORD *type)
{
    UWORD opcode;

    if ((vartype & 0xff) == T_STRING) {
        stvar(vartype, off, type);
        return;
    }
    opcode = regat << 9;
    if (vartype & TF_POINTER)
        vartype = (vartype & 0xff00) | 3; /* .pointer: move.b #3,d2 */
    else if (!(vartype & 0xff)) {
        *type = T_LONG;
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
        pokewd(off);
    }
    *type = vartype & 255;          /* .more */
}

/* blitz2.s:10471 variable */
static UWORD variable(UWORD c, UWORD *type)
{
    UWORD namelen;          /* asm d2 after getvname */
    UWORD old = *type;              /* remember old type */
    struct var *v;
    struct type *t;
    WORD off = 0;
    UWORD vartype;

    c = getvname(c, &namelen);
    if (c == '{')
        return afunction(old, type, namelen);
    v = fetchvar(namelen, &t, &c);
    vartype = calcvar(&v, &t, &c, &off);
    variable2(vartype, off, type);
    ext_evalvalue(t ? t->u.code : 0, flagmask);
    return varcont(old, type);
}

/* blitz2.s:10479 varcont - common tail after a variable / function
   value: ext byte to word, bump the type up to the old one */
static UWORD varcont(UWORD oldtype, UWORD *type)
{
    BOOL byte = *type == T_BYTE;

    /* Fixed (#978): a byte value is fetched sign-extended ($90 ->
       $FF90), while consttype2 takes a constant from 128 to 255 as a
       positive word ($0090): the original's compare of "a.b=$90" with
       "a=$90" never matched (only a=-112 did).  bytevalend tells eval5
       where a byte value ends, which then sign-extends the constant. */
    if (byte)
        pokewd(regat | 0x4880);     /* ALWAYS CONVERT BYTE TO WORD. ext.w Dn */
    /* .notabyte */
    if (!oldtype)                   /* type not set yet... it is now! */
        goto done;
    /* A Quick is cut to Long at once: see issue #1178.  Fixed (#1177)
       for the left operand of a comparison: eval5 drops the conversion
       recorded in conv (cmpownleft); the right operand is fetched without
       a wanted type. */
    if (oldtype == T_LONG && *type == T_QUICK)
        goto conv2;
    if (oldtype <= *type)           /* .nob */
        goto done;
conv2:
    conv.at = pc;
    conv.off = firstoff;
    conv.a6 = lasta6;
    conv.from = *type;
    conv.depth = evaldepth;
    convtype(*type, oldtype);       /* bump up new type to old.... */
    conv.end = pc;
    conv.to = oldtype;
    *type = oldtype;
done:
    if (byte)
        bytevalend = pc;
    return reget();                 /* .done */
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

/* blitz2.s:10522 afunction - do a local function, eg a=pixel{x,y} */
static UWORD afunction(UWORD oldtype, UWORD *type, UWORD namelen)
{
    struct proc *p = findproc(namelen, NULL);
    UWORD rt, mask, c;
    WORD ssg;
    ULONG addr;

    if (!p)
        noprocerr();
    if (p->rettype < 0)
        illprocerr();
    rt = (UBYTE)p->rettype;
    stackuse -= ext_procstackbytes(p);  /* removed by the procedure */
    if (rt == T_STRING)             /* .skip: function type! */
        makesbase();
    ssg = sbasegot;                 /* .sss */
    if (ssg)
        dopusha3();
    mask = savem();                 /* .sssz: the movem regs */
    addr = p->pc;                   /* pc of function call */
    if (!p->nparams) {
        makefjsr(addr);
        c = getchar3();
    } else {
        UWORD sregat = regat;       /* .somepars: old regat */
        regat = 0;
        c = fetchprocps(p);         /* pointer to var desc */
        chkstak();
        makefjsr(addr);
        regat = sregat;
    }
    if (c != '}')                   /* .nopars */
        syntaxerr();
    if (regat)
        pokewd((regat << 9) | 0x2000);  /* move.l d0,regat */
    if (mask) {                     /* .nopars3 */
        pokewd(0x4cdf);
        pokewd(revmask(mask));      /* poke movem (a7)+... */
    }
    if (ssg) {                      /* .nomovem */
        if (rt == T_STRING)
            pokewd(*deca3);         /* .issb: is a string, wack off last '0' */
        else
            /* not a string...set a3 back to start of any strings... */
            pokecode(geta3, geta3f);
        dopulla3();                 /* .nosb: fix up string base */
    }
    *type = rt;                     /* .nores: what it returns! */
    ext_evalvalue(ext_proctype(p), 0);
    if (rt == T_STRING)
        pokewd(*putstlen | regat);
    getchar3();                     /* .notst */
    return varcont(oldtype, type);
}

/* blitz2.s:10617 stvar - handle string variable get! */
static UWORD stvar(UWORD vartype, WORD off, UWORD *type)
{
    makesbase();
    if (!(vartype & TF_ADDRINA2)) {
        leaa5d3a2[1] = off;
        pokela5s(L_(leaa5d3a2));
    }
    /* .already */
    stvarget2[0] = (regat << 9) | 0x7000;              /* moveq #0,Dn */
    stvarget2[1] = (stvarget2[1] & 0xfff8) | regat;    /* move.l Dn,-(a7) */
    pokecode(stvarget2, stvarget2f);
    /* NB: the template's beq.w skips exactly the 6 byte jsr from tokejsr */
    tokejsr(copstring);
    *type = T_STRING;
    return reget();
}
