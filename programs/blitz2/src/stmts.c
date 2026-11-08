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

/* blitz2.s:3501 / 3502 */
struct repnode *firstrep;
UWORD replineat;

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
    UBYTE *p = usedpath;

    if (eos(c))
        syntaxerr();
    do {
        if (c & 0x8000)
            syntaxerr();
        /* Fixed (#31): a path that does not fit into usedpath (127
           characters and the end byte 2) is refused; blitz2.s writes on
           into the code behind it */
        if (p == usedpath + sizeof usedpath - 1)
            pathlongerr();
        *p++ = (UBYTE)c;
        c = getchar3();
    } while (!eos(c));
    *p = 2;
}

/* blitz2.s:3363 excget - get an Exchange operand */
UWORD excget(UWORD c, UBYTE *type)
{
    UWORD namelen;          /* asm d2 after getvname */
    struct var *v;
    struct type *t;
    WORD offset = 0;
    UWORD vtype;

    c = getvname(c, &namelen);
    v = fetchvar(namelen, &t, &c);     /* asm d2 from getvname */
    vtype = calcvar(&v, &t, &c, &offset);
    if (vtype & TF_POINTER)
        vtype = (vtype & 0xff00) | 4;   /* move.b #4,d2 */
    if (!(vtype & 0xff))
        excerr();
    if (!(vtype & TF_ADDRINA2)) {
        pokewda5s(*leaamp);
        pokewd(offset);
    }
    *type = (UBYTE)vtype;
    return c;
}

/* blitz2.s:3335 doexchange - 'swap' 2 variables. */
void doexchange(UWORD c)
{
    UBYTE t, srctype;   /* srctype: asm global srctype (3333), .b at its
                           high byte */

    c = excget(c, &t);
    if (c != ',')
        syntaxerr();
    srctype = t;                        /* move.b d2,srctype */
    pokewd(*pusha2);
    c = getchar3();
    c = excget(c, &t);
    if (t != srctype)
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
    UWORD chk;

    if (DEBUGGA) {
        chk = snxtchk;
        if (f->vartype >= 3)
            chk++;
        tokejsr(chk);
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
    UWORD seltype = s->type, drop;

    if (DEBUGGA)
        tokejsr(seltype < 3 ? casechkw : casechkw + 1); /* make sure next
                                                    thing on stack is a select */
    if (seltype < 3)
        drop = 2;
    else if (seltype < 7)
        drop = 4;
    else {
        pokecode(endselstr, DEBUGGA ? endselstrf2 : endselstrf);
        tokejsr(globfree);      /* $c003 global freemem */
        return;
    }
    if (DEBUGGA)
        drop += 2;              /* for the "sE" */
    pokewd((UWORD)(drop << 9) | *endsel);
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
BOOL endininfo(const char *name)
{
    UBYTE infochk[6] = { 0 };   /* asm global infochk (3520): 6 byte window */
    int i;

    do {
        for (i = 0; i < 5; i++)
            infochk[i] = infochk[i + 1];
        infochk[5] = (UBYTE)*name | 32; /* NUL becomes ' ' */
    } while (*name++);
    return infochk[0] == '.' && infochk[1] == 'i' && infochk[2] == 'n' &&
           infochk[3] == 'f' && infochk[4] == 'o' && infochk[5] == ' ';
}

/* blitz2.s:3541 gettokeps - toke jsr: d4=toke, d5=form */
UWORD gettokeps(UWORD c, UWORD *toke, UWORD *form)
{
    ULONG value;
    WORD isfloat;
    UWORD tk, fm = 0;

    if (c & 0x8000) {                   /* asm: bpl .evaltoke (N of d0) */
        if (!(c & userand))
            tokeerr();
        tk = c & 0x7fff;                /* got toke number */
        c = getchar3();
    } else {
        value = evalconst(&c, &isfloat);
        if (value > 65535 || isfloat)
            tokeerr();
        tk = (UWORD)value;
    }
    if (c == ',') {
        value = evalconst2(&c, &isfloat);
        if (value > 65535 || isfloat)
            tokeerr();
        fm = (UWORD)value;
    }
    *toke = tk;
    *form = fm;
    return c;
}

/* blitz2.s:3571 dolibjsr */
void dolibjsr(UWORD c)
{
    UWORD toke, form;

    gettokeps(c, &toke, &form);
    pokewd(0x4eb9);
    addoff();
    pokel(((ULONG)(form | 0x8000) << 16) | toke); /* Toke of Amiga type. */
}

/* blitz2.s:3582 doblibjsr */
void doblibjsr(UWORD c)
{
    UWORD toke, form;

    gettokeps(c, &toke, &form);
    pokewd(0x4eb9);
    addoff();
    pokel(((ULONG)(form | 0xc000) << 16) | toke); /* Toke of Blitz type. */
}

/* blitz2.s:3593 dosysjsr */
void dosysjsr(UWORD c)
{
    ULONG toke = evalconst(&c, NULL);

    if (toke >= 0x10000)
        tokeerr();
    tokejsr((UWORD)toke);
}

/* blitz2.s:3599 dotokejsr */
void dotokejsr(UWORD c)
{
    UWORD toke, form;
    ULONG at;

    gettokeps(c, &toke, &form);
    pokewd(0x4eb9);
    at = pc;
    if (blitzmode < 0)                  /* btst #7,blitzmode */
        form |= 0x4000;
    maketjsr(at, toke, form, NULL);
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
    ULONG level = evalconst(&c, NULL);

    if (level >= 14)
        interr4();
    pokewd(0x7000 | (UWORD)level);
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
void setint2(WORD strflag)
{
    ULONG level, at;

    errchx();
    intstring = strflag;
    intlineat = linenumat;
    level = evalconst(NULL, NULL);            /* get constant - int level */
    if (level >= 14)
        interr4();
    intsused = (intsused & 0x00ff) | 0xff00;    /* st intsused */
    intlevel = (UWORD)level;
    pokewd(0x7000 | (UWORD)level);      /* moveq #x,d0 */
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

/* Fixed (intended deviation, #958): an amigalib with a parameter in a4 or
   a5 (exec Supervisor_, graphics LockLayerRom_ ..., workbench
   AddAppIconA_).  The original pops the parameters into their registers
   before libbase (move.l off(a5),a6) and does not save a4/a5: with a5 the
   library base is read relative to the parameter (a6 garbage), and after
   the call a4/a5 hold the parameters instead of the bases of the local /
   global variables.  For such a call: movem.l a4/a5,-(a7) before the
   parameters, libbase before the pops (a5 is still the base) and
   movem.l (a7)+,a4/a5 after the jsr.  Every other call is compiled as
   before.
   amigaa4a5 - TRUE if the register list (asm a3: reg bytes, <0 end) has
   a4 or a5; it pokes the movem.l a4/a5,-(a7) then. */
BOOL amigaa4a5(const UBYTE *regs)
{
    for (; (BYTE)*regs >= 0; regs++)
        if (*regs == 0x14 || *regs == 0x15) {   /* a4, a5 */
            pokel(0x48e7000cUL);                /* movem.l a4/a5,-(a7) */
            return TRUE;
        }
    return FALSE;
}

/* blitz2.s:3879 stamigalib - an amigalib called through a statement!
   Fixed (#958): parameters in a4/a5, see amigaa4a5. */
void stamigalib(UWORD c, UBYTE *sub)
{
    struct blitzlib *lib;
    WORD libvar;
    UWORD lvo, type, w;
    UBYTE *regs, *p, reg;
    WORD npars;
    BOOL a45;

    (void)c;
    lib = uselib(W_(sub + 6));          /* get lib for base address */
    libvar = lib->varoff;               /* libbase reg */
    lvo = W_(sub + 8);                  /* get offset for lib */
    regs = sub + 10;
    a45 = amigaa4a5(regs);
    p = regs;
    /* collect longs for lib */
    npars = 0;
    for (;;) {
        if ((BYTE)*p++ < 0)
            break;
        npars++;
        type = T_LONG;
        c = peval(&type);
        if (c == ',')
            continue;
        if ((BYTE)*p >= 0)
            syntaxerr();
        break;
    }
    libjsr[1] = lvo;            /* patched in place, as the asm */
    libbase[1] = libvar;
    if (a45)
        pokel(L_(libbase));     /* fixed (#958): a6 before a5 is popped */
    p = regs;
    if (--npars < 0)
        getchar3();     /* no pars - get : */
    else {
        while ((BYTE)*p++ >= 0)         /* go to end of params */
            ;
        p--;
        do {
            reg = *--p;
            if (reg & 0x10)
                w = (UWORD)((reg & 7) << 9) | 0x205f;   /* move.l (a7)+,An */
            else
                w = (UWORD)(reg << 9) | 0x201f;         /* move.l (a7)+,Dn */
            pokewd(w);
        } while (--npars >= 0);
    }
    if (!a45)
        pokel(L_(libbase));
    pokel(L_(libjsr));
    if (a45)
        pokel(0x4cdf3000UL);    /* fixed (#958): movem.l (a7)+,a4/a5 */
}

/* ================================================================ allox */

/* blitz2.s:3988 makeallox - d2=pc size,d3=lib size,d4=data1,d5=data2 */
BOOL makeallox(ULONG pcsize, ULONG libsize, ULONG data1size, ULONG data2size)
{
    ULONG at, total;

    anyerrs = -1;
    freeallox();
    /* word align blocks */
    pcsize = (pcsize + 1) & ~1UL;
    libsize = (libsize + 1) & ~1UL;
    data1size = (data1size + 1) & ~1UL;
    data2size = (data2size + 1) & ~1UL;
    total = pcsize + libsize + data1size + data2size;
    at = (ULONG)doallocmem(total, MEMF_PUBLIC | MEMF_CLEAR);
    if (!at)
        return FALSE;
    objlen = total;
    maxpc = pcsize;
    maxlib = libsize;
    maxdata1 = data1size;
    maxdata2 = data2size;
    pcat = at;
    at += pcsize;
    libat = at;
    at += libsize;
    data1at = at;
    at += data1size;
    data2at = at;
    at += data2size;
    allat = at;
    return TRUE;
}

/* blitz2.s:3982 makeiallox - initial allox */
BOOL makeiallox(void)
{
    return makeallox(maxpc, maxlib, maxdata1, maxdata2);  /* falls through */
}

/* blitz2.s:3969 newallox */
BOOL newallox(ULONG pcsize, ULONG libsize, ULONG data1size, ULONG data2size)
{
    if (makeallox(pcsize, libsize, data1size, data2size))
        return TRUE;
    /* falls back to the initial sizes and reports their result, so TRUE
       does not mean the new sizes were allocated; not reachable (#32,
       closed: TED's extalloc never returns 0) */
    return makeiallox();
}

/* blitz2.s:3936 alloxinc - increment allocs by 150% */
BOOL alloxinc(void)
{
    ULONG pcsize = (ULONG)maxpc + ((ULONG)maxpc >> 1);
    ULONG libsize = (ULONG)maxlib + ((ULONG)maxlib >> 1);
    ULONG data1size = (ULONG)maxdata1 + ((ULONG)maxdata1 >> 1);
    ULONG data2size = (ULONG)maxdata2 + ((ULONG)maxdata2 >> 1);

    return newallox(pcsize, libsize, data1size, data2size);
}

/* blitz2.s:3956 alloxadj - ne if enough mem for allox */
BOOL alloxadj(void)
{
    ULONG pcsize = bigpc - pcat;        /* size of object buffer */

    if (!makeexec)
        pcsize += 1024;
    return newallox(pcsize, lib - libat, data1 - data1at, data2 - data2at);
}

/* blitz2.s:3974 freeallox */
void freeallox(void)
{
    ULONG len = objlen;

    if (len) {
        objlen = 0;
        FreeMem((APTR)pcat, len);
    }
}

/* ======================================================== macro buffer */

/* Fixed (#336): the original allocates macrobuff once at its start
   (blitz2.s:1384) with the Macro Buffer of then (8192), so an .xtra or
   the Compiler Options requester changes macrolen but not the buffer.
   Called where the other option-sized buffers follow the options
   (makeiallox in loadxtra and optsdone), outside of a compile: no macro
   text is in the buffer then.  The limit of a macro's text (macputb) is
   macrolen; at least MACROMIN bytes are allocated, because doincbin's
   FileInfoBlock and the parameters of a macro call (unchecked, #4) use
   the buffer too, as with the original's fixed 8192.  If the new buffer
   cannot be allocated the old one and its limit stay. */
#define MACROMIN 8192                   /* blitz2.s optreqsg5's default */

BOOL makemacbuff(void)
{
    ULONG len = macrolen;
    ULONG size = len < MACROMIN ? MACROMIN : len;
    UBYTE *buf;

    if (macrobuff && len == (ULONG)(macrobufff - macrobuff))
        return TRUE;
    buf = AllocMem(size, MEMF_PUBLIC);
    if (!buf)
        return FALSE;
    if (macrobuff)
        FreeMem(macrobuff, macrobuffsize);
    macrobuff = buf;
    macrobuffsize = size;
    macrobufff = buf + len;
    return TRUE;
}

/* ========================================================= menu3 (debug) */

#if final == 0
/* blitz2.s:4031 prtname */
static char prtname[] = "PAR:";

/* blitz2.s:4034 menu3 - print tokens to PRT: (only if final==0) */
void menu3(void)
{
    BPTR fh;
    struct token *tok;
    UBYTE *dst, *src;

    fh = Open((STRPTR)prtname, 1006);
    if (!fh)
        return;
    for (tok = &tokens; tok; tok = tok->next) {
        dst = namebuff;
        src = (UBYTE *)tok->name;
        while ((*dst++ = *src++))
            ;
        dst[-1] = 10;
        while ((*dst++ = *src++))
            ;
        dst[-1] = 10;
        *dst++ = 10;
        Write(fh, namebuff, dst - namebuff);
    }
    Close(fh);
}
#endif

/* blitz2.s:4074 makelong - put long in d2 into (a0)+ */
char *makelong(char *buf, ULONG v)
{
    int started = 0;            /* no zero printed */
    int digit;
    char ch;

    for (digit = 9; digit >= 0; digit--) {
        ch = '0';
        while (v >= divisors[digit]) {
            v -= divisors[digit];
            ch++;
        }
        if (ch != '0' || started) {
            *buf++ = ch;
            started = 1;
        }
    }
    if (!started)
        *buf++ = '0';
    *buf = 0;
    return buf;
}

/* ============================================================ residents */

/* writer state of one Make Resident (asm: d7 = file handle, d6 = offset
   from start of file), on menu5's stack */
struct reswriter {
    BPTR fh;
    ULONG pos;
};

/* blitz2.s:4241 resoff */
static void resoff(struct reswriter *w)
{
    addoff2(w->pos);
}

/* Write SIZE bytes from P and, if SIZE is odd, a 0 behind them: the
   items of a resident are word aligned.  Fixed (intended deviation, #970):
   the original writes the length rounded up to even from the item, for an
   odd one the byte behind its allocation (addhere allocates exactly the
   node length, domacro exactly the length of the text), which nothing
   set: the same source gave a different file depending on the memory. */
static void writepadded(struct reswriter *w, const void *p, ULONG size)
{
    static const UBYTE pad;

    Write(w->fh, (APTR)p, size);
    if (size & 1)
        Write(w->fh, (APTR)&pad, 1);
}

/* blitz2.s:4223 writeitem - a2 = item, d5 = offset of its length byte */
static void writeitem(struct reswriter *w, struct node *item, UWORD off)
{
    ULONG size = B_((UBYTE *)item + off);
    ULONG len = (size + 1) & ~1UL;      /* word align */
    struct node *save = item->next;

    if (save) {
        resoff(w);
        item->next = (struct node *)(w->pos + len);
    }
    w->pos += len;
    writepadded(w, item, size);         /* fixed (#970) */
    item->next = save;
}

/* blitz2.s:4214 writelist - a2=first, d5=.b len offset (unused) */
static void __attribute__((unused)) writelist(struct reswriter *w,
                                             struct node *item, UWORD off)
{
    for (; item; item = item->next)
        writeitem(w, item, off);
}

/* blitz2.s:4244 writeoffs - write the offsets (NewType fields); the type
   pointer is replaced by its index in alltypes for the write */
static void writeoffs(struct reswriter *w, struct var *field, UWORD off)
{
    struct type *t, *save;
    UWORD index;

    for (; field; field = field->next) {
        index = 0;
        for (t = alltypes; t && t != field->type; t = t->next)
            index++;
        /* the index in the high word, which fixtypes reads.  Fixed
           (intended deviation, #970): the low word is 0; the original
           replaces only the high word of the pointer (move d1,10(a2)),
           the low word of the type's address stays */
        save = field->type;
        field->type = (struct type *)((ULONG)index << 16);
        writeitem(w, (struct node *)field, off);
        field->type = save;
    }
}

/* blitz2.s:4265 countem - write the number of items of list a2 (word) */
static void countem(struct reswriter *w, struct node *item)
{
    UWORD count = 0;

    for (; item; item = item->next)
        count++;
    w->pos += 2;                /* asm: written via temp1 */
    Write(w->fh, &count, 2);
}

/* blitz2.s:4278 resmacs - put out! macros */
static void resmacs(struct reswriter *w)
{
    struct macro *m = firstmacro, *savenext;
    UBYTE *savetext;
    ULONG start, len, tlen;

    countem(w, (struct node *)m);
    for (; m; m = m->next) {
        start = w->pos;
        savenext = m->next;
        savetext = m->text;
        len = ((ULONG)m->len + 1) & ~1UL;
        w->pos += len;                  /* add len of struct */
        if (m->textlen) {
            m->text = (UBYTE *)w->pos;
            addoff2(start + 4);
        }
        /* Fixed (#995): the text length rounded up to even as a long;
           blitz2.s rounds in a word (addq #1,d4), a text of 65535 bytes
           got the length 0 and was not written */
        tlen = ((ULONG)m->textlen + 1) & ~1UL;
        w->pos += tlen;
        if (m->next) {
            m->next = (struct macro *)w->pos;
            addoff2(start);
        }
        writepadded(w, m, m->len);              /* fixed (#970) */
        if (tlen)
            writepadded(w, savetext, m->textlen);
        m->text = savetext;
        m->next = savenext;
    }
}

/* blitz2.s:4324 restypes */
static void restypes(struct reswriter *w)
{
    struct type *t = firsttype, *savenext;
    ULONG start, len, save;

    countem(w, (struct node *)t);
    for (; t; t = t->next) {
        start = w->pos;
        /* no test for TYPE_FORWARD, as in the original: latent, a forward
           type cannot reach here (#33, closed as invalid: every aborted
           compilation frees the non-resident types) */
        writeoffs(w, t->u.fields, 14);
        save = t->u.code;
        t->u.code = start;
        addoff2(w->pos + 4);
        len = ((ULONG)t->len + 1) & ~1UL;
        w->pos += len;
        /* Fixed (intended deviation, #970): next is written as 0; the
           original writes the pointer to the next type in its memory
           (fixtypes links the types itself) */
        savenext = t->next;
        t->next = NULL;
        writepadded(w, t, t->len);
        t->next = savenext;
        t->u.code = save;
    }
}

/* blitz2.s:4353 resconsts */
static void resconsts(struct reswriter *w)
{
    struct label *l;
    struct label *save;
    UWORD count = 0;
    ULONG start, len;

    for (l = firstlabel; l; l = l->next)
        if ((ULONG)l->refs == 1)
            count++;
    w->pos += 2;
    Write(w->fh, &count, 2);
    for (l = firstlabel; l; l = l->next) {
        if ((ULONG)l->refs != 1)
            continue;
        len = ((ULONG)l->len + 1) & ~1UL;      /* length */
        start = w->pos;
        w->pos += len;
        save = l->next;
        /* next := offset of the following written item (= the next
           constant).  For the last constant, if non-constant labels
           follow it, next points past the section (as in the original;
           such residents exist, AmiBlitz3's all.res too): fixconsts ends
           the list (fixed, #34) */
        if (l->next) {
            l->next = (struct label *)w->pos;
            addoff2(start);
        }
        writepadded(w, l, l->len);              /* fixed (#970) */
        l->next = save;
    }
}

/* blitz2.s:4105 menu5 - make resident! */
void menu5(void)
{
    struct label *l;
    struct offnode *o;
    char *name;
    struct reswriter wr;
    BPTR fh;
    ULONG pad, aligned, hunksize;

    if (!firstmacro && !firsttype) {
        for (l = firstlabel; l; l = l->next)
            if ((ULONG)l->refs == 1)
                break;
        if (!l)
            return;
    }
    name = ed_ezfreq(respath, resname, restext);
    if (!name)
        return;
    /* O.K.... Now to create it */
    fh = Open((STRPTR)name, 1006);
    if (!fh)
        return;
    wr.fh = fh;
    sleepon();
    killoffs();
    Write(fh, tsthead, 32);             /* header */
    wr.pos = 0;                         /* offset from start of file! */
    restypes(&wr);
    resmacs(&wr);
    resconsts(&wr);
    aligned = (wr.pos + 3) & ~3UL;      /* long word align */
    pad = aligned - wr.pos;
    if (pad)
        Write(fh, hello, pad);
    *tstsize3 = numoffs;
    if (numoffs) {
        Write(fh, tstend, 12);
        /* and offsets..... */
        for (o = firstoff; o; o = o->next)
            Write(fh, &o->offset, 4);
        Write(fh, &zero, 4);
    }
    Write(fh, tstdone, 4);
    hunksize = aligned >> 2;    /* asm: temp1 */
    Seek(fh, 20, OFFSET_BEGINNING);
    Write(fh, &hunksize, 4);
    Seek(fh, 28, OFFSET_BEGINNING);
    Write(fh, &hunksize, 4);
    Close(fh);
    pointeron();
}

/* blitz2.s:4483 fixconsts.
   Fixed (intended deviation, #34): the next of the last constant is set to
   0, as AmiBlitz3's fixconsts does (CLR.l (a2) ;bugfix for object,
   AmiBlitz/AmiBlitz3@ea8c25c7).  The original left it as loaded; in a
   resident whose last constant is followed by other labels (resconsts)
   it points to the end of the hunk, and every compile's freelabels (from
   lastrescon) freed what lay behind the hunk. */
UBYTE *fixconsts(UBYTE *p)
{
    UWORD count = W_(p);
    struct node *item;

    p += 2;
    if (!count)
        return p;
    ((struct node *)lastrescon)->next = (struct node *)p;
    count--;
    do {
        item = (struct node *)p;
        p = (UBYTE *)item->next;
    } while (count-- != 0);
    item->next = NULL;          /* fixed (#34) */
    lastrescon = (struct label *)item;
    p = (UBYTE *)item;
    p += (WORD)(((UWORD)B_(p + 18) + 1) & ~1);
    return p;
}

/* blitz2.s:4500 fixmacs */
UBYTE *fixmacs(UBYTE *p)
{
    UWORD count = W_(p);
    struct node *item;
    UWORD len;
    ULONG textlen;

    p += 2;
    if (!count)
        return p;
    ((struct node *)lastresmac)->next = (struct node *)p;
    count--;
    do {
        item = (struct node *)p;
        p = (UBYTE *)item->next;
    } while (count-- != 0);
    lastresmac = (struct macro *)item;
    p = (UBYTE *)item;
    len = ((UWORD)B_(p + 12) + 1) & ~1;
    /* Fixed (#995): the text length rounded up to even as an unsigned
       long; blitz2.s adds the word rounded in a word sign-extended (add
       d1,a3): a last macro with a text of 32767 bytes or more moved p
       backwards, one of 65535 bytes not at all */
    textlen = ((ULONG)W_(p + 8) + 1) & ~1UL;
    p += (WORD)len;
    p += textlen;
    return p;
}

/* blitz2.s:4521 fixtypes - fix up all load types */
UBYTE *fixtypes(UBYTE *p)
{
    UWORD n = W_(p);            /* number of types to do */
    WORD count, index;
    struct type *last, *start, *t, *ftype;
    struct var *field;

    p += 2;
    if (!n)
        return p;
    count = n - 1;
    last = start = lastrestype;
    do {
        /* Fixed (#35): a type without fields (restypes wrote none) starts
           on the type entry, whose relocated field pointer (+4) points to
           the entry itself; a first field has +4 = offset 0 / flags.  Its
           field list becomes empty (u.code 0, as donewtype leaves it).
           blitz2.s skips the "fields" of every type: for an empty one it
           followed the type's next (the raw pointer of the creating
           session in the original's residents, 0 in this compiler's,
           #970) and took the name byte +14 as a field length. */
        if (L_(p + 4) == (ULONG)p) {
            L_(p + 4) = 0;
        } else {
            /* skip through offsets */
            while (L_(p))
                p = (UBYTE *)L_(p);
            p += (WORD)(((UWORD)B_(p + 14) + 1) & ~1);     /* a3=type */
        }
        last->next = (struct type *)p;  /* link */
        last = (struct type *)p;
        p += (WORD)(((UWORD)B_(p + 10) + 1) & ~1);     /* next offset */
    } while (count-- != 0);
    /* Now, Fix up pointers to Stuff! */
    for (t = start->next; t; t = t->next) {
        for (field = t->u.fields; field; field = field->next) {
            index = W_(&field->type);   /* number of type */
            ftype = alltypes;
            if ((UWORD)index >= 7)
                index -= rescnt;
            index--;
            if (index >= 0) {
                do
                    ftype = ftype->next;
                while (index-- != 0);
            }
            field->type = ftype;
        }
    }
    rescnt += n;
    lastrestype = last;
    return p;
}

/* blitz2.s:4441 checkclash - a3 = first item, d7 = name offset */
void checkclash(struct node *first, UWORD nameoff)
{
    struct node *item, *found;
    UBYTE *dst, *src;
    UWORD len;

    for (item = first; item; item = item->next) {
        dst = namebuff;
        src = (UBYTE *)item + nameoff;
        len = (UWORD)-1;
        do
            len++;
        while ((*dst++ = *src++));
        found = first;
        for (;;) {
            found = findlab(found, nameoff, len, NULL);
            if (!found)
                break;                  /* not found */
            if (found != item)
                clasherr();
        }
    }
}

/* blitz2.s:4473 freeres2 */
void freeres2(void)
{
    struct resentry *res = resbuff;
    BPTR seg;
    int i;

    for (i = 0; i < MOSTRES; i++, res++) {
        seg = res->seg;
        if (seg) {
            res->seg = 0;
            UnLoadSeg(seg);
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

/* the constants each resident appended to firstlabel (loadres): first and
   last, NULL if none (warning resclash: the residents of a clash) */
static struct label *resconst[MOSTRES][2];

#define CLASHSHOWN 8    /* clashing names reported; then "N more" */

/* the resident (index into resbuff) whose constants hold l */
static int resof(const struct label *l)
{
    const struct label *p;
    int i;

    for (i = 0; i < MOSTRES; i++)
        for (p = resconst[i][0]; p; p = p->next) {
            if (p == l)
                return i;
            if (p == resconst[i][1])
                break;
        }
    return 0;
}

/* n in decimal at s, with its 0 */
static void dec(char *s, ULONG n)
{
    char t[12];
    int k = 0;

    do
        t[k++] = (char)('0' + n % 10);
    while ((n /= 10));
    while (k)
        *s++ = t[--k];
    *s = 0;
}

static ULONG namehash(const char *s)
{
    ULONG h = 0;

    while (*s)
        h = h * 31 + (UBYTE)*s++;
    return h;
}

/* Fixed (#36): the constants of the residents (firstlabel, name at 19,
   value in pc) for a name defined again with another value than its first
   definition: the warning resclash "Clash in Residents: #NAME" once per
   name (the first CLASHSHOWN, then one "Clash in Residents: N more"), the
   two residents as the detail for the console.  The same value again is
   the same definition included twice: no message.  The first definition
   stays the one the compiler finds.  A hash table (big residents hold
   thousands of constants), only when two or more residents have
   constants; without the memory for it, no check. */
static void checkconstclash(void)
{
    static char msg[sizeof "Clash in Residents: #" + 64];
    static char detail[2 * sizeof resbuff->name + 2];
    struct label *shown[CLASHSHOWN][2];
    struct label *l, *f, **ent;
    ULONG *bucket, *nxt;
    UBYTE *rep, *block;
    ULONG n = 0, nb = 64, size, k = 0, j, h, clashes = 0;
    int i, with = 0;

    for (i = 0; i < MOSTRES; i++)
        if (resconst[i][0])
            with++;
    if (with < 2)
        return;
    for (l = firstlabel; l; l = l->next)
        n++;
    while (nb < n)
        nb <<= 1;
    size = nb * 4 + n * 4 + n * 4 + n;
    if (!(block = AllocMem(size, MEMF_CLEAR)))
        return;
    bucket = (ULONG *)block;            /* entry index + 1, 0: empty */
    ent = (struct label **)(bucket + nb);
    nxt = (ULONG *)(ent + n);           /* the next entry + 1 of a bucket */
    rep = (UBYTE *)(nxt + n);           /* the name has been reported */
    for (l = firstlabel; l; l = l->next) {
        h = namehash(l->name) & (nb - 1);
        for (j = bucket[h]; j; j = nxt[j - 1])
            if (!strcmp(ent[j - 1]->name, l->name))
                break;
        if (!j) {                       /* the first definition */
            ent[k] = l;
            nxt[k] = bucket[h];
            bucket[h] = ++k;
            continue;
        }
        f = ent[j - 1];
        if (f->pc == l->pc || rep[j - 1])
            continue;
        rep[j - 1] = 1;
        if (clashes < CLASHSHOWN) {
            shown[clashes][0] = f;
            shown[clashes][1] = l;
        }
        clashes++;
    }
    FreeMem(block, size);
    /* the messages after the table is freed: a warning that is an error
       does not return */
    for (j = 0; j < clashes && j < CLASHSHOWN; j++) {
        strcpy(msg, "Clash in Residents: #");
        strncat(msg, shown[j][0]->name, sizeof msg - sizeof "Clash in Residents: #");
        strcpy(detail, resbuff[resof(shown[j][0])].name);
        strcat(detail, ", ");
        strcat(detail, resbuff[resof(shown[j][1])].name);
        warn(W_RESCLASH, msg, detail);
    }
    if (clashes > CLASHSHOWN) {
        strcpy(msg, "Clash in Residents: ");
        dec(msg + strlen(msg), clashes - CLASHSHOWN);
        strcat(msg, " more");
        warn(W_RESCLASH, msg, NULL);
    }
}

/* blitz2.s:4393 loadres - load in resident structs..... */
void loadres(void)
{
    jmp_buf jb, *olderr = errjmp;
    struct resentry * volatile res;
    volatile int i;
    UBYTE *p;
    BPTR seg;
    struct label *last;

    ezerr = -1;         /* not reset here: compile clears it */
    freeemup();
    freeres();
    ext_resbegin();
    memset(resconst, 0, sizeof resconst);
    for (i = 0, res = resbuff; i < MOSTRES; i++, res++) {
        /* asm: errstack = a7 at entry, errcont = .next - an error (reserr)
           continues with the next entry */
        errjmp = &jb;
        if (setjmp(jb))
            continue;
        if (!res->name[0])
            continue;
        if (ext_loadres(res)) {
            seg = res->seg;
            if (!seg)
                continue;
        } else {
            seg = LoadSeg((STRPTR)res->name);
            res->seg = seg;
            if (!seg)
                reserr();
        }
        /* res loaded O.K. */
        p = (UBYTE *)BADDR(seg) + 4;    /* start of res */
        p = fixtypes(p);
        p = fixmacs(p);
        last = lastrescon;
        p = fixconsts(p);
        if (lastrescon != last) {       /* the constants it appended */
            resconst[i][0] = (struct label *)((struct node *)last)->next;
            resconst[i][1] = lastrescon;
        }
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
    /* Fixed (#36): the original walked firstconst here, a list nothing
       fills (the constants are labels in firstlabel, name at 19), so a
       clash of constants was never found; the constants are checked in
       firstlabel, a warning when the values differ */
    checkconstclash();
    errjmp = olderr;    /* DIFF: asm left errstack/errcont pointing here */
}

/* blitz2.s:4577 runerropts */
void runerropts(void)
{
    UWORD save[7];
    WORD id;

    save[0] = optreq2ga1.Flags;
    save[1] = optreq2ga2.Flags;
    save[2] = optreq2ga3.Flags;
    save[3] = optreq2ga4.Flags;
    save[4] = warnopt;                  /* the asm: optreq2ga5.Flags */
    save[5] = optreq2ga6.Flags;
    save[6] = optreq2ga7.Flags;
    warnopt_text();
    ed_openreq(&optreq2);
    for (;;) {
        if (!ed_getinput(&id))          /* getinput */
            continue;
        if (id == 5) {                  /* the warnings button: step it */
            warnopt_step();
            ed_refgads(&optreq2ga5, 1);
            continue;
        }
        if ((UWORD)id < 8)
            continue;
        break;
    }
    if (id == 8)
        return;
    /* cancel */
    optreq2ga7.Flags = save[6];
    optreq2ga6.Flags = save[5];
    warnopt = save[4];
    optreq2ga4.Flags = save[3];
    optreq2ga3.Flags = save[2];
    optreq2ga2.Flags = save[1];
    optreq2ga1.Flags = save[0];
}

/* =========================================================== assembler */

/* blitz2.s:4609 makeasmtab - make a table of pointers to ASM data */
void makeasmtab(void)
{
    struct token *tok = &tokens;
    UBYTE *p;
    int i;

    for (i = 0; i < lnum - 1; i++)
        tok = tok->next;
    for (i = 0; i < tnum - fnum; i++) {
        p = (UBYTE *)tok->name;
        while (*p++)
            ;
        while (*p++)
            ;
        p = (UBYTE *)(((ULONG)p + 1) & ~1UL);
        asmpnts[i] = (struct asment *)p;
        tok = tok->next;
    }
}

/* blitz2.s:4631 getimm - get an immediate value */
UWORD getimm(UWORD *mode)
{
    UWORD c = asmconst();

    *mode = EA_IMM;
    return c;
}

/* blitz2.s:4638 regmovems - d4=reg#.  Returns the mask for dounmove (asm:
   global unmove, 4636) */
UWORD regmovems(UWORD reg)
{
    pokewd(0x48e7);                     /* movem.l -(a7) */
    pokewd(0xffff & ~(1 << ((15 - reg) & 15)));
    return 0xffff & ~(1 << (reg & 15));
}

/* blitz2.s:4651 dounmove */
void dounmove(UWORD unmove)
{
    pokewd(0x4cdf);
    pokewd(unmove);
}

/* blitz2.s:4665 doputreg */
void doputreg(UWORD c)
{
    UWORD namelen;          /* asm d2 after getvname */
    struct var *v;
    struct type *t;
    WORD offset = 0;
    UWORD vtype, reg, op;

    c = getreg2(c, &reg);
    if (c != ',')
        syntaxerr();
    pokel(0x48e7fffe);                  /* movem to stack */
    pokewd(reg | 0x2f00);
    c = getchar3();
    c = getvname(c, &namelen);
    v = fetchvar(namelen, &t, &c);     /* asm d2 from getvname */
    vtype = calcvar(&v, &t, &c, &offset);
    if ((vtype & TF_POINTER) || (vtype & 0xff) == 0 || (vtype & 0xff) == 7)
        vtype = (vtype & 0xff00) | 3;   /* .issa */
    if (!(vtype & TF_ADDRINA2)) {
        pokewd(0x45ed);                 /* lea x(a5),a2 */
        pokewd(offset);
    }
    pokewd(0x201f);                     /* move.l (a7)+,d0 */
    op = 0x1480;
    if ((vtype & 0xff) != 1) {
        op ^= 0x3000;
        if ((vtype & 0xff) == 2)
            op |= 0x1000;
    }
    pokewd(op);
    pokel(0x4cdf7fff);
}

/* blitz2.s:4707 dosetreg */
void dosetreg(UWORD c)
{
    UWORD reg, type, unmove;

    c = getreg2(c, &reg);
    if (c != ',')
        syntaxerr();
    unmove = regmovems(reg);
    type = T_LONG;                      /* get a long */
    eval(&type);
    if (reg) {
        if (reg < 8)
            pokewd((UWORD)(reg << 9) | 0x2000);
        else
            pokewd((UWORD)((reg - 8) << 9) | 0x2040);
    }
    dounmove(unmove);
}

/* blitz2.s:4744 getan3 */
UWORD getan3(UWORD *reg)
{
    WORD num = (WORD)(getchar3() - 48);

    if (num < 0 || num > 7)
        syntaxerr();
    *reg = num;
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
    ULONG bits = 0;             /* bits for reg */
    UWORD reg = first - 48, from, t;

    for (;;) {
        if (c == '-') {         /* .range */
            from = reg;         /* from */
            c = getreg(&reg);
            if (from > reg) {
                t = reg;
                reg = from;
                from = t;
            }
            do
                bits |= 1UL << (from & 31);
            while (++from <= reg);
            if (!tstend2(c))
                continue;
            break;
        }
        if (c == '/') {         /* .one */
            bits |= 1UL << (reg & 31);
            c = getreg(&reg);
            if (!tstend2(c))
                continue;
            bits |= 1UL << (reg & 31);  /* .done2 */
            break;
        }
        syntaxerr();
    }
    *mask = (UWORD)bits;        /* .done (asm also d5 = 15) */
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
    UWORD ext = 0;
    WORD num;

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
        ext |= 0x8000;
    }
    num = (WORD)(getchar3() - 48);      /* .ianxi */
    if (num < 0 || num > 7)
        syntaxerr();
    ext |= num << 12;
    c = getchar3();
    if (c == '.') {
        c = getchar3() | 32;
        if (c != 'w') {
            if (c != 'l')
                syntaxerr();
            ext |= 0x0800;
        }
        c = getchar3();         /* .ok */
    }
    if (c != ')')               /* .ok2 */
        syntaxerr();
    extraword = ext;
    *mode = EA_INDEX;
    return getchar3();
}

/* blitz2.s:4783 getea - evaluate the ea of an asm bit.
   asm d4 is in/out: paths that do not set a register leave the caller's
   value, or a character the parser put there (kept). */
UWORD getea(UWORD *mode, UWORD *reg)
{
    UBYTE *start = srcptr;
    UWORD c, first, regval = *reg;

    c = getchar3();
    if (c == '#')
        return getimm(mode);
    if (c == '(') {                     /* .ind */
        c = getan(&regval);
        if (c != ')')
            syntaxerr();
        *reg = regval;
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
        c = getan(&regval);
        if (c != ')')
            syntaxerr();
        *reg = regval;
        *mode = EA_PREDEC;
        return getchar3();
    }
    /* .more */
    c |= 32;
    first = c;                          /* first */
    c = getchar3();                     /* get second */
    if (first == 'd') {
        if (c < '0' || c > '7')
            goto notan;
        regval = c;
        c = getchar3();
        if (tstend2(c)) {               /* .dn */
            *mode = EA_DN;
            *reg = regval - 48;
            return c;
        }
        if (c == '-' || c == '/') {
            *mode = EA_REGLIST;
            return getmovem(c, regval, reg);
        }
        goto notan2;
    }
    if (first == 'a') {                 /* .notdn */
        if (c < '0' || c > '7')
            goto notan;
        regval = c;
        c = getchar3();
        if (tstend2(c)) {               /* .an */
            *mode = EA_AN;
            *reg = regval - 48;
            return c;
        }
        if (c == '-' || c == '/') {
            *mode = EA_REGLIST;
            return getmovem2(c, regval, reg);
        }
        goto notan2;
    }
notan:
    c |= 32;
    regval = c;
    c = getchar3();                     /* get third */
notan2:
    /* d1,d4,d0=chars */
    if (tstend2(c) && first == 's') {
        if (regval == 'p') {
            if (!tstend2(c))
                goto notccr;
            /* Sp */
            *reg = 7;
            *mode = EA_AN;
            return c;
        }
        if (regval != 'r' || !tstend2(c)) /* .notsp2 */
            goto notccr;
        /* Sr */
        *reg = regval;
        *mode = EA_SR;
        return c;
    }
    /* .notsx */
    c |= 32;
    if (first == 'c') {
        if (regval != 'c' || c != 'r')
            goto notccr;
        c = getchar3();
        if (!eos(c) && c != ',')
            goto notccr;
        /* ccr */
        *reg = regval;
        *mode = EA_CCR;
        return c;
    }
    if (first == 'u') {                 /* .notccr0 */
        if (regval != 's' || c != 'p')
            goto notccr;
        c = getchar3();
        if (!eos(c) && c != ',')
            goto notccr;
        *reg = regval;
        *mode = EA_USP;
        return c;
    }
notccr:
    /* not anything - must be a label or abs address of some kind */
    srcptr = start;
    c = asmconst();
    if (tstend2(c)) {                   /* .absl */
        *reg = regval;
        *mode = EA_ABSL;
        return c;
    }
    if (c == '.') {                     /* .absx */
        c = getchar3() | 32;
        *reg = regval;
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
        *reg = regval;
        c = getrest(c, mode);
        *mode += 4;                     /* d(PC) / d(PC,Xn) */
        return c;
    }
    /* .notpcrel */
    c = getan2(c, &regval);
    *reg = regval;
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
    UBYTE *params, *line;

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
    params = (UBYTE *)ret15add;
    blitzmode = W_(params - 2);
    L_(params - 6) = 0;
    firstglob = (struct var *)L_(params - 10);
    firstlocal = (struct var *)L_(params - 14);
    procmode = 0;
    if (firstglob || firstlocal)
        procmode = ~procmode;
    line = (UBYTE *)L_(params - 18);    /* address of string */
    ed_tokone(line);                    /* tokenise line..... */
    srcptr = line;
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
