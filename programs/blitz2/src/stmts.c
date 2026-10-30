/*
 * stmts.c - chunk 03, blitz2.s 3291-4999 ("subs"): statement handlers
 * (RunErrsOn/Off, UsePath, Exchange, Pop, Repeat, ALibJsr/BLibJsr/TokeJsr/
 * SysJsr, VWait, Blitz/Amiga/QAmiga, SetInt/ClrInt, SetErr/ClrErr/ErrFail,
 * PutReg/SetReg), direct mode (C part of the trap #15 handler, see
 * glue_direct.c), object buffer allocation (allox), make / load residents,
 * the asm token table and the assembler effective address parser.
 * See notes/survey-03.md.
 */
#include <string.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "blitz2.h"

/* ================================================================ data */

/* blitz2.s:3304 usedpath - UsePath text, terminated by byte 2 (getchar
   returns to usedfrom when it sees the 2) */
UBYTE usedpath[128];

/* blitz2.s:3316 swapb..swaplf, pulla1 - Exchange code (20 bytes) */
UWORD swapb[] = {
    0x1012,                            /* +0   swapb move.b (a2),d0 */
    0x1491,                            /* +2   move.b (a1),(a2) */
    0x1280,                            /* +4   move.b d0,(a1) */
    0x3012,                            /* +6   swapw move (a2),d0 */
    0x3491,                            /* +8   move (a1),(a2) */
    0x3280,                            /* +10  move d0,(a1) */
    0x2012,                            /* +12  swapl move.l (a2),d0 */
    0x2491,                            /* +14  move.l (a1),(a2) */
    0x2280,                            /* +16  move.l d0,(a1) */
    0x225F,                            /* +18  pulla1 move.l (a7)+,a1 */
};

/* blitz2.s:3333 srctype - type of Exchange's first var.  asm: dc, used
   with .b ops (high byte) */
UWORD srctype;

/* blitz2.s:3501 / 3502 */
struct repnode *firstrep;
UWORD replineat;

/* blitz2.s:3520 infochk - endininfo's 6 byte window */
static UBYTE infochk[6];

/* blitz2.s:3620 fvwait, cgoblitz, cgoamiga (12 bytes) */
UWORD fvwait[] = {
    0x5340,                            /* +0   fvwait subq #1,d0 */
    0x3B7C, 0xFFFF, 0x0000,            /* +2   cgoblitz move #-1,0(a5) */
    0x426D, 0x0000,                    /* +8   cgoamiga clr 0(a5) */
};

/* blitz2.s:3670 intstart0 / intfin (8 bytes) */
UWORD intstart0[] = {
    0x48E7, 0x3F38,                    /* +0   intstart0 movem.l d2-d7/a2-a4,-(a7) */
    0x4CDF, 0x1CFC,                    /* +4   intfin movem.l (a7)+,d2-d7/a2-a4 */
};

/* blitz2.s:3684-3686 */
UWORD inerr;            /* flag - in error trap mode */
ULONG errjmp_;          /* asm errjmp: where err jump is */
ULONG errcode;

/* blitz2.s:3726 */
UWORD intlineat;

/* blitz2.s:3790-3800 */
UWORD intallox;         /* mask for allocates for interrupts (unused) */
ULONG intdata1;         /* where in data1 a5 is being kept */
ULONG intjmpat;
UWORD intline;
WORD  intstring;        /* flag 0 = no setint, >0 = setint, no st space,
                           <0 = set, st space */
UWORD intlevel;
ULONG intcleanat;

/* blitz2.s:3802 / 3803 (used by glue_direct.c) */
ULONG ret15add;
UWORD ret15cc;

/* blitz2.s:4071 divisors */
static const ULONG divisors[10] = {
    1, 10, 100, 1000, 10000, 100000,
    1000000, 10000000, 100000000, 1000000000
};

/* blitz2.s:4098-4103 */
char restext[] = "Name of Resident File to Create";
char respath[192];
char resname[192];

/* blitz2.s:4212 hello - padding bytes for the resident file */
char hello[4] = "ZAP";

/* blitz2.s:4636 unmove */
UWORD unmove;

/* blitz2.s:837 asmpnts - ds.l tnum-fnum (defined in chunk 01's range,
   owned here) */
struct asment *asmpnts[tnum - fnum];

/* popreturn's local templates (blitz2.s 3414-3422).  .rfix..rfixf is copied
   (10 bytes: Devpac assembled the beq without optimisation as beq.w), the
   jsr comes from tokejsr (6 bytes), then .rfix2.  Verified against the
   shipped binary. */
static UWORD rfix[] = {
    0x0C6F, 0x6753, 0x0004,            /* +0   .rfix cmp #"gS",4(a7) */
    0x6700, 0x0008,                    /* +6   beq .rfix2 */
    0x4EB9, 0x0000, 0x0000,            /* +10  .rfixf jsr 0 */
    0x5C4F,                            /* +16  .rfix2 addq #6,a7 */
    0x584F,                            /* +18  .sfix addq #4,a7 */
};
#define rfixf (&rfix[5])
#define rfix2 (&rfix[8])
#define sfix  (&rfix[9])

/* asm `tst.b debugga` (see compile.h: written as 0xff00 / 0) */
#define DEBUGGA (debugga != 0)

/* ====================================================== statements */

/* blitz2.s:3294 dorunerrson - turn on runtime error checking! */
void dorunerrson(UWORD c)
{
    (void)c;
    /* sne debugga: high byte only */
    debugga = (debugga & 0x00ff) |
              ((optreqga10.Flags & GFLG_SELECTED) ? 0xff00 : 0);
}

/* blitz2.s:3300 dorunerrsoff */
void dorunerrsoff(UWORD c)
{
    (void)c;
    debugga &= 0x00ff;          /* sf debugga */
}

/* blitz2.s:3306 dousepath */
void dousepath(UWORD c)
{
    UBYTE *a0 = usedpath;

    if (eos(c))
        syntaxerr();
    do {
        if (c & 0x8000)
            syntaxerr();
        *a0++ = (UBYTE)c;   /* BUG(orig) (#31): no length check (128 bytes) */
        c = getchar3();
    } while (!eos(c));
    *a0 = 2;
}

/* blitz2.s:3363 excget - get an Exchange operand */
UWORD excget(UWORD c, UBYTE *type)
{
    struct var *v;
    struct type *t;
    WORD d3 = 0;
    UWORD d2;

    c = getvname(c);
    v = fetchvar(getvnamelen, &t, &c);     /* asm d2 from getvname */
    d2 = calcvar(&v, &t, &c, &d3);
    if (d2 & TF_POINTER)
        d2 = (d2 & 0xff00) | 4;         /* move.b #4,d2 */
    if (!(d2 & 0xff))
        excerr();
    if (!(d2 & TF_ADDRINA2)) {
        pokewda5s(*leaamp);
        pokewd(d3);
    }
    *type = (UBYTE)d2;
    return c;
}

/* blitz2.s:3335 doexchange - 'swap' 2 variables. */
void doexchange(UWORD c)
{
    UBYTE t;

    c = excget(c, &t);
    if (c != ',')
        syntaxerr();
    srctype = (srctype & 0x00ff) | ((UWORD)t << 8);     /* move.b d2,srctype */
    pokewd(*pusha2);
    c = getchar3();
    c = excget(c, &t);
    if (t != (UBYTE)(srctype >> 8))
        excerr2();
    pokewd(*pulla1);
    if (t < 2)
        pokecode(swapb, swapbf);
    else if (t == 2)
        pokecode(swapw, swapwf);
    else
        pokecode(swapl, swaplf);
}

/* blitz2.s:3379 doallocmem - editor's extalloc (comdata+88):
   jsr AllocMem(a6); on failure the editor closes its requester and
   longjmps to its own out-of-memory handler (never returns 0). */
APTR doallocmem(ULONG size, ULONG flags)
{
    struct m68kregs r;

    memset(&r, 0, sizeof r);
    r.d[0] = size;
    r.d[1] = flags;
    r.a[6] = (ULONG)SysBase;
    callregs(CD_FN(88), &r);
    return (APTR)r.d[0];
}

/* blitz2.s:3383 dowbstartup */
void dowbstartup(UWORD c)
{
    (void)c;
    tokejsr(wbstart);
}

/* blitz2.s:3403 popreturn - Pop Gosub */
void popreturn(void)
{
    if (DEBUGGA) {
        pokecode(rfix, rfixf);
        tokejsr(chkret);
        pokewd(*rfix2);
    } else
        pokewd(*sfix);
    getchar3();
}

/* blitz2.s:3446 popnext */
void popnext(struct fornode *f)
{
    UWORD d1;

    if (DEBUGGA) {
        d1 = snxtchk;
        if (f->vartype >= 3)
            d1++;
        tokejsr(d1);
    }
}

/* blitz2.s:3430 popnext3 - fix stack of a For */
void popnext3(struct fornode *f)
{
    if (DEBUGGA) {
        if (f->vartype < 3)
            pokel(L_(nextfixw2));
        else
            pokel(L_(nextfixl2));
        return;
    }
    if (f->vartype < 3)
        pokewd(*nextfixw);
    else
        pokel(L_(nextfixl));
}

/* blitz2.s:3424 popnext2 - Pop For */
void popnext2(void)
{
    struct fornode *f = firstfor;

    if (!f)
        noforerr();
    getchar3();
    popnext(f);
    popnext3(f);        /* falls through into popnext3 */
}

/* blitz2.s:3465 popselect - fix stack on select */
void popselect(struct selnode *s)
{
    UWORD d2 = s->type, d1;

    if (DEBUGGA)
        tokejsr(d2 < 3 ? casechkw : casechkw + 1); /* make sure next thing on
                                                       stack is a select */
    if (d2 < 3)
        d1 = 2;
    else if (d2 < 7)
        d1 = 4;
    else {
        pokecode(endselstr, DEBUGGA ? endselstrf2 : endselstrf);
        tokejsr(globfree);      /* $c003 global freemem */
        return;
    }
    if (DEBUGGA)
        d1 += 2;                /* for the "sE" */
    pokewd((UWORD)(d1 << 9) | *endsel);
}

/* blitz2.s:3460 popselect2 - Pop Select */
void popselect2(void)
{
    struct selnode *s = firstsel;

    if (!s)
        eselerr();
    getchar3();
    popselect(s);       /* falls through into popselect */
}

/* blitz2.s:3386 dopop */
void dopop(UWORD c)
{
    if (c == 0x8006) {                  /* Gosub */
        popreturn();
        return;
    }
    if (c == 0x8000 + 22) {             /* For */
        popnext2();
        return;
    }
    if (c == 0x8000 + 14) {             /* Select */
        popselect2();
        return;
    }
    if (c == 0x8000 + 11 || c == 0x8000 + 12 || c == 0x8000 + 20 ||
        c == 0x8000 + tnum + 8) {       /* If, While, tok 20, Repeat */
        getchar3();
        return;
    }
    syntaxerr();
}

/* blitz2.s:3504 dorepeat - Repeat ... Until a=10 */
void dorepeat(UWORD c)
{
    struct repnode *r;

    (void)c;
    replineat = linenumat;
    r = doallocmem(8, MEMF_PUBLIC);
    r->next = firstrep;
    firstrep = r;
    r->pc = pc;
}

/* blitz2.s:3522 endininfo - does a0 end in .info? */
BOOL endininfo(const char *a0)
{
    int i;

    memset(infochk, 0, sizeof infochk);
    do {
        for (i = 0; i < 5; i++)
            infochk[i] = infochk[i + 1];
        infochk[5] = (UBYTE)*a0 | 32;   /* NUL becomes ' ' */
    } while (*a0++);
    return infochk[0] == '.' && infochk[1] == 'i' && infochk[2] == 'n' &&
           infochk[3] == 'f' && infochk[4] == 'o' && infochk[5] == ' ';
}

/* blitz2.s:3541 gettokeps - toke jsr: d4=toke, d5=form */
UWORD gettokeps(UWORD c, UWORD *toke, UWORD *form)
{
    ULONG d3;
    UWORD d4, d5 = 0;

    if (c & 0x8000) {                   /* asm: bpl .evaltoke (N of d0) */
        if (!(c & userand))
            tokeerr();
        d4 = c & 0x7fff;                /* got toke number */
        c = getchar3();
    } else {
        d3 = evalconst(&c);
        if (d3 > 65535 || numd6)
            tokeerr();
        d4 = (UWORD)d3;
    }
    if (c == ',') {
        d3 = evalconst2(&c);
        if (d3 > 65535 || numd6)
            tokeerr();
        d5 = (UWORD)d3;
    }
    *toke = d4;
    *form = d5;
    return c;
}

/* blitz2.s:3571 dolibjsr */
void dolibjsr(UWORD c)
{
    UWORD d4, d5;

    gettokeps(c, &d4, &d5);
    pokewd(0x4eb9);
    addoff();
    pokel(((ULONG)(d5 | 0x8000) << 16) | d4);   /* Toke of Amiga type. */
}

/* blitz2.s:3582 doblibjsr */
void doblibjsr(UWORD c)
{
    UWORD d4, d5;

    gettokeps(c, &d4, &d5);
    pokewd(0x4eb9);
    addoff();
    pokel(((ULONG)(d5 | 0xc000) << 16) | d4);   /* Toke of Blitz type. */
}

/* blitz2.s:3593 dosysjsr */
void dosysjsr(UWORD c)
{
    ULONG d3 = evalconst(&c);

    if (d3 >= 0x10000)
        tokeerr();
    tokejsr((UWORD)d3);
}

/* blitz2.s:3599 dotokejsr */
void dotokejsr(UWORD c)
{
    UWORD d4, d5;
    ULONG d3;

    gettokeps(c, &d4, &d5);
    pokewd(0x4eb9);
    d3 = pc;
    if (blitzmode < 0)                  /* btst #7,blitzmode */
        d5 |= 0x4000;
    maketjsr(d3, d4, d5, NULL);
    addoff();
    /* DIFF: asm poked d1 as left by findtoke (low word = the lib's first
       token, high word undefined); fixjsrs overwrites the long anyway */
    pokel(0);
}

/* blitz2.s:3610 dovwait */
void dovwait(UWORD c)
{
    UWORD type;

    if (eos(c))
        pokewd(0x7000);                 /* moveq #0,d0 */
    else {
        type = T_WORD;                  /* get a word */
        bakeval(&type);
        pokewd(*fvwait);
    }
    tokejsr(intvwait);
}

/* blitz2.s:3626 debugmode */
void debugmode(void)
{
    if (debugga & 0xff00)               /* move.b debugga(pc),d1 */
        tokejsr(0xd500);
}

/* blitz2.s:3632 doblitz - go into Blitz mode! */
void doblitz(UWORD c)
{
    (void)c;
    blitzmode |= 0x8100;        /* bset #0 / bset #7 of the high byte */
    tokejsr(goblitz);
    if (DEBUGGA) {
        pokewd(*cgoblitz);
        pokewd(0xffff);
        pokewd((UWORD)getbbase());
    }
    debugmode();
}

/* blitz2.s:3658 toamode */
void toamode(UWORD tok)
{
    blitzmode = (blitzmode | 0x0100) & ~0x8000;
    tokejsr(tok);
    if (DEBUGGA) {
        pokewd(*cgoamiga);
        pokewd((UWORD)getbbase());
    }
    debugmode();
}

/* blitz2.s:3649 doamiga - go into Amiga mode */
void doamiga(UWORD c)
{
    (void)c;
    toamode(goamiga);
}

/* blitz2.s:3654 doqamiga - go into quick amiga mode */
void doqamiga(UWORD c)
{
    (void)c;
    toamode(goqamiga);
}

/* blitz2.s:3674 doclrint */
void doclrint(UWORD c)
{
    ULONG d3 = evalconst(&c);

    if (d3 >= 14)
        interr4();
    pokewd(0x7000 | (UWORD)d3);
    tokejsr(clrint);            /* clear the interupt */
}

/* blitz2.s:3688 doclrerr */
void doclrerr(UWORD c)
{
    (void)c;
    tokejsr(clrerr);
}

/* blitz2.s:3691 doendseterr */
void doendseterr(UWORD c)
{
    (void)c;
    if (!inerr)
        errerr3();
    inerr = 0;
    if (!nomemleft)
        L_(errjmp_) = pc;
    getchar3();
}

/* blitz2.s:3700 doerrfail */
void doerrfail(UWORD c)
{
    (void)c;
    tokejsr(errfail);
}

/* blitz2.s:3703 doseterr */
void doseterr(UWORD c)
{
    (void)c;
    if (procmode)
        errerr1();
    if (inerr)
        errerr2();
    inerr = ~inerr;
    pokewd(0x203c);                     /* move.l #x,d0 */
    errcode = pc;
    addoff();
    pokel(0x203c);      /* asm: d1 left from pokewd (high word undefined);
                           patched below */
    tokejsr(seterr);
    pokewd(0x4ef9);                     /* JMP */
    errjmp_ = pc;
    addoff();
    pokel(0x4ef9);      /* patched by End SetErr */
    if (!nomemleft)
        L_(errcode) = pc;
}

/* blitz2.s:3730 setint2 - d1 = intstring value */
void setint2(WORD d1)
{
    ULONG d3, at;

    errchx();
    intstring = d1;
    intlineat = linenumat;
    d3 = evalconst(NULL);               /* get constant - int level */
    if (d3 >= 14)
        interr4();
    intsused = (intsused & 0x00ff) | 0xff00;    /* st intsused */
    intlevel = (UWORD)d3;
    pokewd(0x7000 | (UWORD)d3);         /* moveq #x,d0 */
    pokewd(0x223c);                     /* move.l #x,d1 */
    addoff();
    at = pc;
    pokel(0x223c);                      /* patched below */
    tokejsr(setint);
    pokewd(0x4ef9);                     /* jmp */
    addoff();
    intjmpat = pc;
    pokel(0x4ef9);                      /* patched by End SetInt */
    if (!nomemleft)
        L_(at) = pc;
    pokel(L_(intstart0));
    if (DEBUGGA) {
        /* debugga stuff.. */
        maketrap(2);
        addoff();
        intcleanat = pc;
        pokel(0x4e42);                  /* asm: d1 left from maketrap */
        pokewd(intlevel);
        tokejsr(gosup);                 /* disable stack checks */
    }
    uselib(65235);
    tokejsr(newint);
    lasta6 = -1;
    intline = linenumat;
}

/* blitz2.s:3728 dosetint */
void dosetint(UWORD c)
{
    (void)c;
    setint2(-1);        /* type - need new string space */
}

/* blitz2.s:3879 stamigalib - an amigalib called through a statement! */
void stamigalib(UWORD c, UBYTE *a3)
{
    struct blitzlib *lib;
    WORD libvar;
    UWORD lvo, type, w;
    UBYTE *regs, d1;
    WORD d3;

    (void)c;
    lib = uselib(W_(a3 + 6));           /* get lib for base address */
    libvar = lib->varoff;               /* libbase reg */
    lvo = W_(a3 + 8);                   /* get offset for lib */
    a3 += 10;
    regs = a3;
    /* collect longs for lib */
    d3 = 0;
    for (;;) {
        if ((BYTE)*a3++ < 0)
            break;
        d3++;
        type = T_LONG;
        c = peval(&type);
        if (c == ',')
            continue;
        if ((BYTE)*a3 >= 0)
            syntaxerr();
        break;
    }
    a3 = regs;
    if (--d3 < 0)
        getchar3();     /* no pars - get : */
    else {
        while ((BYTE)*a3++ >= 0)        /* go to end of params */
            ;
        a3--;
        do {
            d1 = *--a3;
            if (d1 & 0x10)
                w = (UWORD)((d1 & 7) << 9) | 0x205f;    /* move.l (a7)+,An */
            else
                w = (UWORD)(d1 << 9) | 0x201f;          /* move.l (a7)+,Dn */
            pokewd(w);
        } while (--d3 >= 0);
    }
    libjsr[1] = lvo;            /* patched in place, as the asm */
    libbase[1] = libvar;
    pokel(L_(libbase));
    pokel(L_(libjsr));
}

/* ================================================================ allox */

/* blitz2.s:3988 makeallox - d2=pc size,d3=lib size,d4=data1,d5=data2 */
BOOL makeallox(ULONG d2, ULONG d3, ULONG d4, ULONG d5)
{
    ULONG d0, d6;

    anyerrs = -1;
    freeallox();
    /* word align blocks */
    d2 = (d2 + 1) & ~1UL;
    d3 = (d3 + 1) & ~1UL;
    d4 = (d4 + 1) & ~1UL;
    d5 = (d5 + 1) & ~1UL;
    d6 = d2 + d3 + d4 + d5;
    d0 = (ULONG)doallocmem(d6, MEMF_PUBLIC | MEMF_CLEAR);
    if (!d0)
        return FALSE;
    objlen = d6;
    maxpc = d2;
    maxlib = d3;
    maxdata1 = d4;
    maxdata2 = d5;
    pcat = d0;
    d0 += d2;
    libat = d0;
    d0 += d3;
    data1at = d0;
    d0 += d4;
    data2at = d0;
    d0 += d5;
    allat = d0;
    return TRUE;
}

/* blitz2.s:3982 makeiallox - initial allox */
BOOL makeiallox(void)
{
    return makeallox(maxpc, maxlib, maxdata1, maxdata2);  /* falls through */
}

/* blitz2.s:3969 newallox */
BOOL newallox(ULONG d2, ULONG d3, ULONG d4, ULONG d5)
{
    if (makeallox(d2, d3, d4, d5))
        return TRUE;
    /* BUG(orig) (#32): falls back to the initial sizes and reports their result,
       so TRUE does not mean the new sizes were allocated */
    return makeiallox();
}

/* blitz2.s:3936 alloxinc - increment allocs by 150% */
BOOL alloxinc(void)
{
    ULONG d2 = (ULONG)maxpc + ((ULONG)maxpc >> 1);
    ULONG d3 = (ULONG)maxlib + ((ULONG)maxlib >> 1);
    ULONG d4 = (ULONG)maxdata1 + ((ULONG)maxdata1 >> 1);
    ULONG d5 = (ULONG)maxdata2 + ((ULONG)maxdata2 >> 1);

    return newallox(d2, d3, d4, d5);
}

/* blitz2.s:3956 alloxadj - ne if enough mem for allox */
BOOL alloxadj(void)
{
    ULONG d2 = bigpc - pcat;            /* size of object buffer */

    if (!makeexec)
        d2 += 1024;
    return newallox(d2, lib - libat, data1 - data1at, data2 - data2at);
}

/* blitz2.s:3974 freeallox */
void freeallox(void)
{
    ULONG d0 = objlen;

    if (d0) {
        objlen = 0;
        FreeMem((APTR)pcat, d0);
    }
}

/* ========================================================= menu3 (debug) */

#if final == 0
/* blitz2.s:4031 prtname */
static char prtname[] = "PAR:";

/* blitz2.s:4034 menu3 - print tokens to PRT: (only if final==0) */
void menu3(void)
{
    BPTR d7;
    struct token *a2;
    UBYTE *a0, *a1;

    d7 = Open((STRPTR)prtname, 1006);
    if (!d7)
        return;
    for (a2 = &tokens; a2; a2 = a2->next) {
        a0 = namebuff;
        a1 = (UBYTE *)a2->name;
        while ((*a0++ = *a1++))
            ;
        a0[-1] = 10;
        while ((*a0++ = *a1++))
            ;
        a0[-1] = 10;
        *a0++ = 10;
        Write(d7, namebuff, a0 - namebuff);
    }
    Close(d7);
}
#endif

/* blitz2.s:4074 makelong - put long in d2 into (a0)+ */
char *makelong(char *a0, ULONG d2)
{
    int d1 = 0;                 /* no zero printed */
    int d3;
    char d4;

    for (d3 = 9; d3 >= 0; d3--) {
        d4 = '0';
        while (d2 >= divisors[d3]) {
            d2 -= divisors[d3];
            d4++;
        }
        if (d4 != '0' || d1) {
            *a0++ = d4;
            d1 = 1;
        }
    }
    if (!d1)
        *a0++ = '0';
    *a0 = 0;
    return a0;
}

/* ============================================================ residents */

/* writer state: asm d7 = file handle, d6 = offset from start of file */
static BPTR resfh;
static ULONG resd6;

/* blitz2.s:4241 resoff */
static void resoff(void)
{
    addoff2(resd6);
}

/* blitz2.s:4223 writeitem - a2 = item, d5 = offset of its length byte */
static void writeitem(struct node *a2, UWORD d5)
{
    ULONG d3 = ((ULONG)B_((UBYTE *)a2 + d5) + 1) & ~1UL;   /* word align */
    struct node *save = a2->next;

    if (save) {
        resoff();
        a2->next = (struct node *)(resd6 + d3);
    }
    resd6 += d3;
    Write(resfh, a2, d3);
    a2->next = save;
}

/* blitz2.s:4214 writelist - a2=first, d5=.b len offset (unused) */
static void __attribute__((unused)) writelist(struct node *a2, UWORD d5)
{
    for (; a2; a2 = a2->next)
        writeitem(a2, d5);
}

/* blitz2.s:4244 writeoffs - write the offsets (NewType fields); the type
   pointer is replaced by its index in alltypes for the write */
static void writeoffs(struct var *a2, UWORD d5)
{
    struct type *a1;
    UWORD d1, save;

    for (; a2; a2 = a2->next) {
        d1 = 0;
        for (a1 = alltypes; a1 && a1 != a2->type; a1 = a1->next)
            d1++;
        /* move 10(a2),-(a7) / move d1,10(a2): high word of the pointer */
        save = W_(&a2->type);
        W_(&a2->type) = d1;
        writeitem((struct node *)a2, d5);
        W_(&a2->type) = save;
    }
}

/* blitz2.s:4265 countem - write the number of items of list a2 (word) */
static void countem(struct node *a2)
{
    UWORD d0 = 0;

    for (; a2; a2 = a2->next)
        d0++;
    W_(&temp1) = d0;            /* move d0,temp1: high word of temp1 */
    resd6 += 2;
    Write(resfh, &temp1, 2);
}

/* blitz2.s:4278 resmacs - put out! macros */
static void resmacs(void)
{
    struct macro *a3 = firstmacro, *savenext;
    UBYTE *savetext;
    ULONG d5, d3, d4;

    countem((struct node *)a3);
    for (; a3; a3 = a3->next) {
        d5 = resd6;
        savenext = a3->next;
        savetext = a3->text;
        d3 = ((ULONG)a3->len + 1) & ~1UL;
        resd6 += d3;                    /* add len of struct */
        if (a3->textlen) {
            a3->text = (UBYTE *)resd6;
            addoff2(d5 + 4);
        }
        d4 = (UWORD)(a3->textlen + 1) & ~1;
        resd6 += d4;
        if (a3->next) {
            a3->next = (struct macro *)resd6;
            addoff2(d5);
        }
        Write(resfh, a3, d3);
        if (d4)
            Write(resfh, savetext, d4);
        a3->text = savetext;
        a3->next = savenext;
    }
}

/* blitz2.s:4324 restypes */
static void restypes(void)
{
    struct type *a3 = firsttype;
    ULONG start, d3, save;

    countem((struct node *)a3);
    for (; a3; a3 = a3->next) {
        start = resd6;
        /* no test for TYPE_FORWARD, as in the original: latent, a forward
           type cannot reach here (#33, closed as invalid: every aborted
           compilation frees the non-resident types) */
        writeoffs(a3->u.fields, 14);
        save = a3->u.code;
        a3->u.code = start;
        addoff2(resd6 + 4);
        d3 = ((ULONG)a3->len + 1) & ~1UL;
        resd6 += d3;
        Write(resfh, a3, d3);
        a3->u.code = save;
    }
}

/* blitz2.s:4353 resconsts */
static void resconsts(void)
{
    struct label *a2;
    struct label *save;
    UWORD d0 = 0;
    ULONG d2, d3;

    for (a2 = firstlabel; a2; a2 = a2->next)
        if ((ULONG)a2->refs == 1)
            d0++;
    W_(&temp1) = d0;
    resd6 += 2;
    Write(resfh, &temp1, 2);
    for (a2 = firstlabel; a2; a2 = a2->next) {
        if ((ULONG)a2->refs != 1)
            continue;
        d3 = ((ULONG)a2->len + 1) & ~1UL;      /* length */
        d2 = resd6;
        resd6 += d3;
        save = a2->next;
        /* next := offset of the following written item (= the next
           constant).  BUG(orig) (#34): for the last constant, if non-constant
           labels follow it, next points past the section (fixconsts
           leaves it as loaded) */
        if (a2->next) {
            a2->next = (struct label *)resd6;
            addoff2(d2);
        }
        Write(resfh, a2, d3);
        a2->next = save;
    }
}

/* blitz2.s:4105 menu5 - make resident! */
void menu5(void)
{
    struct label *a2;
    struct offnode *o;
    char *name;
    BPTR d7;
    ULONG d3, aligned;

    if (!firstmacro && !firsttype) {
        for (a2 = firstlabel; a2; a2 = a2->next)
            if ((ULONG)a2->refs == 1)
                break;
        if (!a2)
            return;
    }
    name = ed_ezfreq(respath, resname, restext);
    if (!name)
        return;
    /* O.K.... Now to create it */
    d7 = Open((STRPTR)name, 1006);
    if (!d7)
        return;
    resfh = d7;
    sleepon();
    killoffs();
    Write(d7, tsthead, 32);             /* header */
    resd6 = 0;                          /* offset from start of file! */
    restypes();
    resmacs();
    resconsts();
    aligned = (resd6 + 3) & ~3UL;       /* long word align */
    d3 = aligned - resd6;
    if (d3)
        Write(d7, hello, d3);
    *tstsize3 = numoffs;
    if (numoffs) {
        Write(d7, tstend, 12);
        /* and offsets..... */
        for (o = firstoff; o; o = o->next)
            Write(d7, &o->offset, 4);
        Write(d7, &zero, 4);
    }
    Write(d7, tstdone, 4);
    temp1 = aligned >> 2;
    Seek(d7, 20, OFFSET_BEGINNING);
    Write(d7, &temp1, 4);
    Seek(d7, 28, OFFSET_BEGINNING);
    Write(d7, &temp1, 4);
    Close(d7);
    pointeron();
}

/* blitz2.s:4483 fixconsts */
UBYTE *fixconsts(UBYTE *a3)
{
    UWORD d1 = W_(a3);
    struct node *a2;

    a3 += 2;
    if (!d1)
        return a3;
    ((struct node *)lastrescon)->next = (struct node *)a3;
    d1--;
    do {
        a2 = (struct node *)a3;
        a3 = (UBYTE *)a2->next;
    } while (d1-- != 0);
    lastrescon = (struct label *)a2;
    a3 = (UBYTE *)a2;
    a3 += (WORD)(((UWORD)B_(a3 + 18) + 1) & ~1);
    return a3;
}

/* blitz2.s:4500 fixmacs */
UBYTE *fixmacs(UBYTE *a3)
{
    UWORD d1 = W_(a3);
    struct node *a2;
    UWORD d0;

    a3 += 2;
    if (!d1)
        return a3;
    ((struct node *)lastresmac)->next = (struct node *)a3;
    d1--;
    do {
        a2 = (struct node *)a3;
        a3 = (UBYTE *)a2->next;
    } while (d1-- != 0);
    lastresmac = (struct macro *)a2;
    a3 = (UBYTE *)a2;
    d0 = ((UWORD)B_(a3 + 12) + 1) & ~1;
    d1 = (UWORD)(W_(a3 + 8) + 1) & ~1;
    a3 += (WORD)d0;
    a3 += (WORD)d1;             /* add d1,a3: sign extended */
    return a3;
}

/* blitz2.s:4521 fixtypes - fix up all load types */
UBYTE *fixtypes(UBYTE *a3)
{
    UWORD n = W_(a3);           /* number of types to do */
    WORD d1, d0;
    struct type *a2, *start, *a0, *a4;
    struct var *a1;

    a3 += 2;
    if (!n)
        return a3;
    d1 = n - 1;
    a2 = start = lastrestype;
    do {
        /* skip through offsets.  BUG(orig) (#35): a type without fields is not
           handled (its raw next pointer is followed) */
        while (L_(a3))
            a3 = (UBYTE *)L_(a3);
        a3 += (WORD)(((UWORD)B_(a3 + 14) + 1) & ~1);   /* a3=type */
        a2->next = (struct type *)a3;   /* link */
        a2 = (struct type *)a3;
        a3 += (WORD)(((UWORD)B_(a3 + 10) + 1) & ~1);   /* next offset */
    } while (d1-- != 0);
    /* Now, Fix up pointers to Stuff! */
    for (a0 = start->next; a0; a0 = a0->next) {
        for (a1 = a0->u.fields; a1; a1 = a1->next) {
            d0 = W_(&a1->type);         /* number of type */
            a4 = alltypes;
            if ((UWORD)d0 >= 7)
                d0 -= rescnt;
            d0--;
            if (d0 >= 0) {
                do
                    a4 = a4->next;
                while (d0-- != 0);
            }
            a1->type = a4;
        }
    }
    rescnt += n;
    lastrestype = a2;
    return a3;
}

/* blitz2.s:4441 checkclash - a3 = first item, d7 = name offset */
void checkclash(struct node *a3, UWORD d7)
{
    struct node *a4, *a2;
    UBYTE *a0, *a1;
    UWORD d2;

    for (a4 = a3; a4; a4 = a4->next) {
        a0 = namebuff;
        a1 = (UBYTE *)a4 + d7;
        d2 = (UWORD)-1;
        do
            d2++;
        while ((*a0++ = *a1++));
        a2 = a3;
        for (;;) {
            a2 = findlab(a2, d7, d2, NULL);
            if (!a2)
                break;                  /* not found */
            if (a2 != a4)
                clasherr();
        }
    }
}

/* blitz2.s:4473 freeres2 */
void freeres2(void)
{
    struct resentry *a2 = resbuff;
    BPTR d1;
    int d2;

    for (d2 = 0; d2 < MOSTRES; d2++, a2++) {
        d1 = a2->seg;
        if (d1) {
            a2->seg = 0;
            UnLoadSeg(d1);
        }
    }
}

/* blitz2.s:4460 freeres */
void freeres(void)
{
    lastrestype = (struct type *)&firsttype;
    firsttype = NULL;
    lastresmac = (struct macro *)&firstmacro;
    firstmacro = NULL;
    lastrescon = (struct label *)&firstlabel;
    firstlabel = NULL;
    rescnt = 0;
    freeres2();         /* falls through into freeres2 */
}

/* blitz2.s:4393 loadres - load in resident structs..... */
void loadres(void)
{
    jmp_buf jb, *olderr = errjmp;
    struct resentry * volatile a5;
    volatile int d6;
    UBYTE *a3;
    BPTR seg;

    ezerr = -1;         /* not reset here: compile clears it */
    freeemup();
    freeres();
    for (d6 = 0, a5 = resbuff; d6 < MOSTRES; d6++, a5++) {
        /* asm: errstack = a7 at entry, errcont = .next - an error (reserr)
           continues with the next entry */
        errjmp = &jb;
        if (setjmp(jb))
            continue;
        if (!a5->name[0])
            continue;
        seg = LoadSeg((STRPTR)a5->name);
        a5->seg = seg;
        if (!seg)
            reserr();
        /* res loaded O.K. */
        a3 = (UBYTE *)BADDR(seg) + 4;   /* start of res */
        a3 = fixtypes(a3);
        a3 = fixmacs(a3);
        a3 = fixconsts(a3);
    }
    /* check resident clashes! (errcont = freeres) */
    errjmp = &jb;
    if (setjmp(jb)) {
        errjmp = olderr;
        freeres();
        return;
    }
    checkclash((struct node *)firsttype, NAMEOFF_TYPE);
    checkclash((struct node *)firstmacro, NAMEOFF_MACRO);
    /* BUG(orig) (#36): firstconst is never filled (findconst has no caller,
       constants live in firstlabel, name at 19 - fixconsts appends the
       resident ones via lastrescon), so this walks an empty list and
       clashing constants are never detected; offset 25 does not matter */
    checkclash((struct node *)firstconst, 25);
    errjmp = olderr;    /* DIFF: asm left errstack/errcont pointing here */
}

/* blitz2.s:4577 runerropts */
void runerropts(void)
{
    UWORD save[7];
    WORD d6;

    save[0] = optreq2ga1.Flags;
    save[1] = optreq2ga2.Flags;
    save[2] = optreq2ga3.Flags;
    save[3] = optreq2ga4.Flags;
    save[4] = optreq2ga5.Flags;
    save[5] = optreq2ga6.Flags;
    save[6] = optreq2ga7.Flags;
    ed_openreq(&optreq2);
    for (;;) {
        if (!ed_getinput(&d6))          /* getinput */
            continue;
        if ((UWORD)d6 < 8)
            continue;
        break;
    }
    if (d6 == 8)
        return;
    /* cancel */
    optreq2ga7.Flags = save[6];
    optreq2ga6.Flags = save[5];
    optreq2ga5.Flags = save[4];
    optreq2ga4.Flags = save[3];
    optreq2ga3.Flags = save[2];
    optreq2ga2.Flags = save[1];
    optreq2ga1.Flags = save[0];
}

/* =========================================================== assembler */

/* blitz2.s:4609 makeasmtab - make a table of pointers to ASM data */
void makeasmtab(void)
{
    struct token *a0 = &tokens;
    UBYTE *a2;
    int d0;

    for (d0 = 0; d0 < lnum - 1; d0++)
        a0 = a0->next;
    for (d0 = 0; d0 < tnum - fnum; d0++) {
        a2 = (UBYTE *)a0->name;
        while (*a2++)
            ;
        while (*a2++)
            ;
        a2 = (UBYTE *)(((ULONG)a2 + 1) & ~1UL);
        asmpnts[d0] = (struct asment *)a2;
        a0 = a0->next;
    }
}

/* blitz2.s:4631 getimm - get an immediate value */
UWORD getimm(UWORD *mode)
{
    UWORD c = asmconst();

    *mode = EA_IMM;
    return c;
}

/* blitz2.s:4638 regmovems - d4=reg# */
void regmovems(UWORD d4)
{
    pokewd(0x48e7);                     /* movem.l -(a7) */
    pokewd(0xffff & ~(1 << ((15 - d4) & 15)));
    unmove = 0xffff & ~(1 << (d4 & 15));
}

/* blitz2.s:4651 dounmove */
void dounmove(void)
{
    pokewd(0x4cdf);
    pokewd(unmove);
}

/* blitz2.s:4665 doputreg */
void doputreg(UWORD c)
{
    struct var *v;
    struct type *t;
    WORD d3 = 0;
    UWORD d2, d4, d1;

    c = getreg2(c, &d4);
    if (c != ',')
        syntaxerr();
    pokel(0x48e7fffe);                  /* movem to stack */
    pokewd(d4 | 0x2f00);
    c = getchar3();
    c = getvname(c);
    v = fetchvar(getvnamelen, &t, &c);     /* asm d2 from getvname */
    d2 = calcvar(&v, &t, &c, &d3);
    if ((d2 & TF_POINTER) || (d2 & 0xff) == 0 || (d2 & 0xff) == 7)
        d2 = (d2 & 0xff00) | 3;         /* .issa */
    if (!(d2 & TF_ADDRINA2)) {
        pokewd(0x45ed);                 /* lea x(a5),a2 */
        pokewd(d3);
    }
    pokewd(0x201f);                     /* move.l (a7)+,d0 */
    d1 = 0x1480;
    if ((d2 & 0xff) != 1) {
        d1 ^= 0x3000;
        if ((d2 & 0xff) == 2)
            d1 |= 0x1000;
    }
    pokewd(d1);
    pokel(0x4cdf7fff);
}

/* blitz2.s:4707 dosetreg */
void dosetreg(UWORD c)
{
    UWORD d4, type;

    c = getreg2(c, &d4);
    if (c != ',')
        syntaxerr();
    regmovems(d4);
    type = T_LONG;                      /* get a long */
    eval(&type);
    if (d4) {
        if (d4 < 8)
            pokewd((UWORD)(d4 << 9) | 0x2000);
        else
            pokewd((UWORD)((d4 - 8) << 9) | 0x2040);
    }
    dounmove();
}

/* blitz2.s:4744 getan3 */
UWORD getan3(UWORD *reg)
{
    WORD d0 = (WORD)(getchar3() - 48);

    if (d0 < 0 || d0 > 7)
        syntaxerr();
    *reg = d0;
    return getchar3();
}

/* blitz2.s:4742 getan2 */
UWORD getan2(UWORD c, UWORD *reg)
{
    if (c != 'a')
        syntaxerr();
    return getan3(reg);
}

/* blitz2.s:4740 getan */
UWORD getan(UWORD *reg)
{
    UWORD c = getchar3() | 32;

    return getan2(c, reg);
}

/* blitz2.s:4731 getreg2 */
UWORD getreg2(UWORD c, UWORD *reg)
{
    c |= 32;
    if (c == 'd')
        return getan3(reg);
    if (c != 'a')
        syntaxerr();
    c = getan3(reg);
    *reg += 8;
    return c;
}

/* blitz2.s:4730 getreg */
UWORD getreg(UWORD *reg)
{
    return getreg2(getchar3(), reg);
}

/* blitz2.s:4954 tstend2 */
BOOL tstend2(UWORD c)
{
    return c == 0 || c == ':' || c == ',';
}

/* blitz2.s:4753 getmovem - d4 = ASCII digit of the first reg */
UWORD getmovem(UWORD c, UWORD first, UWORD *mask)
{
    ULONG d5 = 0;               /* bits for reg */
    UWORD d4 = first - 48, d1, t;

    for (;;) {
        if (c == '-') {         /* .range */
            d1 = d4;            /* from */
            c = getreg(&d4);
            if (d1 > d4) {
                t = d4;
                d4 = d1;
                d1 = t;
            }
            do
                d5 |= 1UL << (d1 & 31);
            while (++d1 <= d4);
            if (!tstend2(c))
                continue;
            break;
        }
        if (c == '/') {         /* .one */
            d5 |= 1UL << (d4 & 31);
            c = getreg(&d4);
            if (!tstend2(c))
                continue;
            d5 |= 1UL << (d4 & 31);     /* .done2 */
            break;
        }
        syntaxerr();
    }
    *mask = (UWORD)d5;          /* .done (asm also d5 = 15) */
    return c;
}

/* blitz2.s:4752 getmovem2 */
UWORD getmovem2(UWORD c, UWORD first, UWORD *mask)
{
    return getmovem(c, first + 8, mask);
}

/* blitz2.s:4961 getrest */
UWORD getrest(UWORD c, UWORD *mode)
{
    UWORD d1 = 0;
    WORD d0;

    if (c != ',') {             /* .dis */
        if (c != ')')
            syntaxerr();
        *mode = EA_DISP;
        return getchar3();
    }
    c = getchar3() | 32;
    if (c != 'd') {
        if (c != 'a')
            syntaxerr();
        d1 |= 0x8000;
    }
    d0 = (WORD)(getchar3() - 48);       /* .ianxi */
    if (d0 < 0 || d0 > 7)
        syntaxerr();
    d1 |= d0 << 12;
    c = getchar3();
    if (c == '.') {
        c = getchar3() | 32;
        if (c != 'w') {
            if (c != 'l')
                syntaxerr();
            d1 |= 0x0800;
        }
        c = getchar3();         /* .ok */
    }
    if (c != ')')               /* .ok2 */
        syntaxerr();
    extraword = d1;
    *mode = EA_INDEX;
    return getchar3();
}

/* blitz2.s:4783 getea - evaluate the ea of an asm bit.
   asm d4 is in/out: paths that do not set a register leave the caller's
   value, or a character the parser put there (kept). */
UWORD getea(UWORD *mode, UWORD *reg)
{
    UBYTE *a4 = srcptr;
    UWORD c, d1, d4 = *reg;

    c = getchar3();
    if (c == '#')
        return getimm(mode);
    if (c == '(') {                     /* .ind */
        c = getan(&d4);
        if (c != ')')
            syntaxerr();
        *reg = d4;
        c = getchar3();
        if (c == '+') {                 /* .postinc */
            *mode = EA_POSTINC;
            return getchar3();
        }
        *mode = EA_IND;
        return c;
    }
    if (c == '-') {
        c = getchar3();
        if (c != '(')
            goto notccr;
        c = getan(&d4);
        if (c != ')')
            syntaxerr();
        *reg = d4;
        *mode = EA_PREDEC;
        return getchar3();
    }
    /* .more */
    c |= 32;
    d1 = c;                             /* first */
    c = getchar3();                     /* get second */
    if (d1 == 'd') {
        if (c < '0' || c > '7')
            goto notan;
        d4 = c;
        c = getchar3();
        if (tstend2(c)) {               /* .dn */
            *mode = EA_DN;
            *reg = d4 - 48;
            return c;
        }
        if (c == '-' || c == '/') {
            *mode = EA_REGLIST;
            return getmovem(c, d4, reg);
        }
        goto notan2;
    }
    if (d1 == 'a') {                    /* .notdn */
        if (c < '0' || c > '7')
            goto notan;
        d4 = c;
        c = getchar3();
        if (tstend2(c)) {               /* .an */
            *mode = EA_AN;
            *reg = d4 - 48;
            return c;
        }
        if (c == '-' || c == '/') {
            *mode = EA_REGLIST;
            return getmovem2(c, d4, reg);
        }
        goto notan2;
    }
notan:
    c |= 32;
    d4 = c;
    c = getchar3();                     /* get third */
notan2:
    /* d1,d4,d0=chars */
    if (tstend2(c) && d1 == 's') {
        if (d4 == 'p') {
            if (!tstend2(c))
                goto notccr;
            /* Sp */
            *reg = 7;
            *mode = EA_AN;
            return c;
        }
        if (d4 != 'r' || !tstend2(c))   /* .notsp2 */
            goto notccr;
        /* Sr */
        *reg = d4;
        *mode = EA_SR;
        return c;
    }
    /* .notsx */
    c |= 32;
    if (d1 == 'c') {
        if (d4 != 'c' || c != 'r')
            goto notccr;
        c = getchar3();
        if (!eos(c) && c != ',')
            goto notccr;
        /* ccr */
        *reg = d4;
        *mode = EA_CCR;
        return c;
    }
    if (d1 == 'u') {                    /* .notccr0 */
        if (d4 != 's' || c != 'p')
            goto notccr;
        c = getchar3();
        if (!eos(c) && c != ',')
            goto notccr;
        *reg = d4;
        *mode = EA_USP;
        return c;
    }
notccr:
    /* not anything - must be a label or abs address of some kind */
    srcptr = a4;
    c = asmconst();
    if (tstend2(c)) {                   /* .absl */
        *reg = d4;
        *mode = EA_ABSL;
        return c;
    }
    if (c == '.') {                     /* .absx */
        c = getchar3() | 32;
        *reg = d4;
        if (c == 'w') {
            *mode = EA_ABSW;
            return c;
        }
        if (c != 'l')
            syntaxerr();
        *mode = EA_ABSL;
        return c;
    }
    if (c != '(')
        syntaxerr();
    c = getchar3() | 32;
    if (c == 'p') {
        c = getchar3() | 32;
        if (c != 'c')
            syntaxerr();
        c = getchar3();
        *reg = d4;
        c = getrest(c, mode);
        *mode += 4;                     /* d(PC) / d(PC,Xn) */
        return c;
    }
    /* .notpcrel */
    c = getan2(c, &d4);
    *reg = d4;
    return getrest(c, mode);
}

/* ========================================================== direct mode */

/* blitz2.s:3819 directdo (C part) / 3871 dircont.
   Entered from the asm stub in glue_direct.c after trap #15 (directtrap):
   the parameter block lies before ret15add:
     -18 code.l, -14 firstlocal.l, -10 firstglobal.l, -6 returned.l,
     -2 blitz mode status.w
   Returns the address to continue at (generated code), or 0 to return to
   ret15add (error: text pointer stored in the "returned" long). */
ULONG directdo_c(struct m68kregs *r)
{
    jmp_buf jb, *olderr = errjmp;
    UBYTE *a1, *a0;

    (void)r;
    errjmp = &jb;               /* asm: errcont = dircont, errstack = a7 */
    if (setjmp(jb)) {
        /* dircont: direct mode error! - d0=error text */
        L_(ret15add - 6) = (ULONG)errmsg;
        errjmp = olderr;        /* DIFF: asm never restored errcont */
        dirmode = 0;            /* direxit */
        flushc();
        return 0;
    }
    dirmode = -1;
    a1 = (UBYTE *)ret15add;
    blitzmode = W_(a1 - 2);
    L_(a1 - 6) = 0;
    firstglob = (struct var *)L_(a1 - 10);
    firstlocal = (struct var *)L_(a1 - 14);
    procmode = 0;
    if (firstglob || firstlocal)
        procmode = ~procmode;
    a0 = (UBYTE *)L_(a1 - 18);          /* address of string */
    ed_tokone(a0);                      /* tokenise line..... */
    srcptr = a0;
    pc = dirpcat;
    nomemleft = 0;
    lasta6 = -1;
    doline();                           /* .....compile it */
    procmode = 0;
    errchx();
    pokewd(0x4ef9);                     /* JMP */
    pokel(ret15add);                    /* jmp to done */
    if (nomemleft)
        nodirmem();
    errjmp = olderr;
    /* direxit */
    dirmode = 0;
    flushc();   /* DIFF: asm did this after restoring the registers */
    return dirpcat;                     /* install code address */
}
