/*
 * tokens2.c - chunk 09, blitz2.s 13327-14655: built-in token handlers part 2
 * (Then, On..Goto/Gosub, Goto, Gosub, Return, Statement/Function, End ...,
 * NewType, EndIf/Wend, Stop/Cont, For/Next, Let, Dim).
 * See notes/survey-09.md.
 *
 * Remarks valid for the whole file:
 *  - asm handlers that end in `bra getchar3` / `bra reget` call it and
 *    return; the dispatcher re-reads the char with reget (lastchar).
 *  - DIFF: "garbage" longs: the asm sometimes pokes whatever is in d1 as a long
 *    placeholder that is always patched later (unless nomemleft); the C
 *    code pokes the value d1 would have had where it is obvious, else 0.
 *  - the name length after getvname (asm d2) is getvnamelen (eval.h).
 */
#include <string.h>
#include <proto/exec.h>
#include "blitz2.h"

/* ================================================================ data */

UWORD labcnt;               /* blitz2.s:13368 */
ULONG localslist;           /* blitz2.s:13488 all locals for procs (unused) */
ULONG proclocals;           /* blitz2.s:13489 for debugga! */
WORD procnum;               /* blitz2.s:13490 */
WORD procnum2;              /* blitz2.s:13491 */
UWORD prolineat;            /* blitz2.s:13492 */
UWORD forlineat;            /* blitz2.s:14225 */
UWORD fortemp;              /* blitz2.s:14226 */
UWORD pcodd;                /* blitz2.s:14509 add 1 to this for bytes */
UWORD listadd;              /* blitz2.s:14510 set to 8 if it's a list */
UWORD listsize;             /* blitz2.s:14511 */

/* ============================================================ templates */
/* exact bytes from build/blitz2.lst (tools/asmlisting.py) */

/* blitz2.s:13331 ongotocode..ongotocodef */
UWORD ongotocode[] = {
    0xB07C, 0x0001,                    /* +0   ongotocode cmp #1,d0 */
    0x6D00, 0x0014,                    /* +4   blt gc1skip */
    0xB07C, 0x0000,                    /* +8   cmp #0,d0  (+10 = #labels) */
    0x6E00, 0x000C,                    /* +12  bgt gc1skip */
    0xD040,                            /* +16  add d0,d0 */
    0xD040,                            /* +18  add d0,d0 */
    0x207B, 0x0004,                    /* +20  move.l gc1labs-4(pc,d0),a0 */
    0x4ED0,                            /* +24  jmp (a0) */
    0x6000, 0xFFFE,                    /* +26  gc1skip bra gc1skip (disp patched) */
};
/* blitz2.s:13343 ongosubcode..ongosubcodef */
UWORD ongosubcode[] = {
    0xB07C, 0x0001,                    /* +0   ongosubcode cmp #1,d0 */
    0x6D00, 0x0014,                    /* +4   blt gc2skip */
    0xB07C, 0x0000,                    /* +8   cmp #0,d0 */
    0x6E00, 0x000C,                    /* +12  bgt gc2skip */
    0xD040,                            /* +16  add d0,d0 */
    0xD040,                            /* +18  add d0,d0 */
    0x207B, 0x0004,                    /* +20  move.l gc2labs-4(pc,d0),a0 */
    0x4E90,                            /* +24  jsr (a0) */
    0x6000, 0xFFFE,                    /* +26  gc2skip bra gc2skip */
};
/* blitz2.s:13355 ongcode..ongcodef (debugger version of On..Gosub) */
UWORD ongcode[] = {
    0xB07C, 0x0001,                    /* +0   ongcode cmp #1,d0 */
    0x6D00, 0x0018,                    /* +4   blt gc3skip */
    0xB07C, 0x0000,                    /* +8   cmp #0,d0 */
    0x6E00, 0x0010,                    /* +12  bgt gc3skip */
    0xD040,                            /* +16  add d0,d0 */
    0xD040,                            /* +18  add d0,d0 */
    0x207B, 0x0008,                    /* +20  move.l gc3labs-4(pc,d0),a0 */
    0x3F3C, 0x6753,                    /* +24  goset move #'gS',-(a7) */
    0x4E90,                            /* +28  jsr (a0) */
    0x6000, 0xFFFE,                    /* +30  gc3skip bra gc3skip */
};
UWORD forset[] = { 0x3F3C, 0x664F };   /* blitz2.s:14221 move #'fO',-(a7) */
UWORD thebra[] = { 0x6000, 0xFFFE };   /* blitz2.s:14223 bra thebra */
UWORD thebgt[] = { 0x6E00, 0xFFFE };   /* blitz2.s:14336 bgt thebgt */
UWORD add1a2[] = { 0x524A };           /* blitz2.s:14508 addq #1,a2 */
UWORD setles[] = { 0x383C, 0x0000 };   /* blitz2.s:14654 move #0,d4 */

/* =========================================================== internals */

/* a template long, read word by word.  DIFF: the asm did move.l label,d1;
   a ULONG* access would break strict aliasing after the word patches. */
static ULONG tl(const UWORD *p)
{
    return ((ULONG)p[0] << 16) | p[1];
}

/* temp1/temp2 are dc.l, but dodim/dimwit use them with word moves, i.e.
   their HIGH word (big endian).  Done with shifts (no type punning). */
#define HIW(l)        ((UWORD)((l) >> 16))
#define SETHIW(l, w)  ((l) = ((l) & 0xffff) | ((ULONG)(UWORD)(w) << 16))

static void dofunc2(WORD d1, UWORD c);
static void linkput(ULONG a0);
static void procfixer(void);
static struct ifnode *doendif3(UWORD kind);
static void dowend2(void);
static void mydoendif(void);
static void doendif(UWORD kind);
static void doend2(void);
static void doend3(UWORD c);
static void endstate(void);
static void normret(void);
static void stateret(UWORD c);
static void funcret(UWORD c);
static void dogoto2(UWORD op);
static void donext2(struct fornode *f);
static void dodim2(UWORD c);
static void dimwit(struct var *v, struct type *t);
static void alignletpc(void);

/* =============================================================== Then */

/* blitz2.s:13327 dothen - doif2 (chunk 08) falls through into here */
void dothen(UWORD c)
{
    (void)c;
    bakup();                /* to reget last */
    lastchar = ':';
}

/* blitz2.s:13329 ifdone */
void ifdone(void)
{
}

/* ==================================================== On Goto / Gosub */

/* blitz2.s:13370 doongo - On expression Goto/Gosub Label[,Label...] */
void doongo(UWORD c)
{
    UWORD *a0 = ongotocode, *a1 = ongotocodef;
    UWORD d1 = 26;          /* gc1skip-ongotocode */
    UWORD t = T_WORD;       /* Get expression as a word. */
    ULONG start;
    UWORD d2;
    struct label *lab;

    (void)c;
    c = bakeval(&t);
    if (c != 0x8005) {
        if (c != 0x8006)
            syntaxerr();
        a0 = ongosubcode; a1 = ongosubcodef;
        d1 = 26;            /* gc2skip-ongosubcode */
        if (debugga) {
            chkstak();
            a0 = ongcode; a1 = ongcodef;
            d1 = 30;        /* gc3skip-ongcode */
        }
    }
    d2 = d1;
    start = pc;             /* for when we know number of labels! */
    pokecode(a0, a1);
    labcnt = 0;
    /* now, make a labels list! */
    do {
        getchar3();
        pc -= 2;            /* for makealab! (it records the ref at pc+2) */
        lab = makealab(NULL);
        pc += 2;
        addoff();
        pokel(lab->pc);
        labcnt++;
        c = reget();
    } while (c == ',');
    if (nomemleft)
        return;
    W_(start + 10) = labcnt;                    /* for cmp# */
    W_(start + d2 + 2) = labcnt * 4 + 2;        /* for bra! */
}

/* ================================================= Goto / Gosub / Return */

/* blitz2.s:13429 dogosub */
void dogosub(UWORD c)
{
    (void)c;
    if (debugga) {
        chkstak();
        pokel(tl(goset));
    }
    dogoto2(gosubcode[0]);
}

/* blitz2.s:13439 dogoto */
void dogoto(UWORD c)
{
    (void)c;
    dogoto2(gotocode[0]);
}

/* blitz2.s:13440 dogoto2 - d1 = opcode (jmp.l / jsr.l) */
static void dogoto2(UWORD op)
{
    struct label *lab = makealab(NULL);
    pokewd(op);
    addoff();
    pokel(lab->pc);
}

/* blitz2.s:13448 eos - jmp to the end of the procedure (patched in
   endstate through the firstpend list) */
void eos_(UWORD c)
{
    struct addrnode *n;

    (void)c;
    if (procmode == 0)
        badpenderr();
    pokewd(0x4ef9);
    addoff();
    n = doallocmem(8, MEMF_PUBLIC);
    n->next = firstpend;
    firstpend = n;
    n->addr = pc;
    pokel(0);               /* asm: d1 left by doallocmem - patched later */
}

/* blitz2.s:13466 stateret - Statement Return */
static void stateret(UWORD c)
{
    eos_(c);
    getchar3();
}

/* blitz2.s:13469 funcret - Function Return expr */
static void funcret(UWORD c)
{
    UWORD t = (UBYTE)thisproc->rettype;     /* type returning! */
    c = eval(&t);                           /* put return val in d0. */
    eos_(c);
}

/* blitz2.s:13477 doreturn */
void doreturn(UWORD c)
{
    (void)c;
    if (debugga) {
        tokejsr(chkret);
        return;
    }
    normret();
}

/* blitz2.s:13483 normret - a normal return */
static void normret(void)
{
    pokewd(0x4e75);
}

/* ================================================= Statement / Function */

/* blitz2.s:13494 dostate */
void dostate(UWORD c)
{
    if (c == 0x8007) {
        stateret(c);
        return;
    }
    dofunc2(-1, c);         /* statement flag */
}

/* blitz2.s:13498 dofunc */
void dofunc(UWORD c)
{
    WORD d1;

    if (c == 0x8007) {
        funcret(c);
        return;
    }
    bakup();
    d1 = sizespec2();
    c = getchar3();
    dofunc2(d1, c);         /* falls through into dofunc2 */
}

/* blitz2.s:13504 dofunc2 - do some checking..... d1 = procmode value
   (-1 statement, else the function's type) */
static void dofunc2(WORD d1, UWORD c)
{
    struct node *last;
    struct proc *p;
    struct var *v;
    struct type *t;
    UBYTE *a4;
    UWORD d6, d5, d2, len, i;
    UWORD *a0;

    (void)c;
    errchx();
    prolineat = linenumat;
    procnum2++;
    procnum = procnum2;
    procmode = d1;
    locvaroff = 0;
    len = makename2(&c);
    if (c != '{')
        syntaxerr();
    memlibstat = memlib->flags;
    memlib->flags &= ~LIBF_USED;
    pokewd(0x4ef9);         /* make a jump around the proc */
    addoff();
    pokel(0x4ef9);          /* asm: d1 = $4ef9 - patched in endstate */
    if (findproc(len, &last))
        dupprocerr();
    p = (struct proc *)addhere(last, NAMEOFF_PROC, len);
    p->pc = pc;             /* new! */
    thisproc = p;
    pokecode2(nops8, nopsf);
    if (debugga) {
        maketrap(4);
        proclocals = pc;
        pokel(0x4e44);      /* asm: d1 from maketrap - filled by droplocals */
        pokel(0x4e44);
    }
    a4 = p->params;
    p->rettype = (BYTE)procmode;            /* move.b procmode+1,5(a2) */
    d6 = 0;
    d5 = 0;
    c = getchar3();
    if (c != '}') {
        for (;;) {
            c = getvname(c);
            v = fetchvar(getvnamelen, &t, &c);
            if (t->u.code == T_STRING) {
                UWORD *sb = (UWORD *)stbuff;
                sb[d5 * 2] = d6;
                sb[d5 * 2 + 1] = v->offset;
                d5++;
                d2 = T_STRING;
            } else {
                UWORD op = 0x2940;          /* move.l dN,x(a4) */
                d2 = T_LONG;
                if (!(flagmask & 1)) {
                    if (t->u.code >= 256)
                        illprocperr();
                    d2 = TYPE_CODE(t);
                    if (d2 == 2)
                        op |= 0x1000;       /* move.w */
                    else if (d2 < 2)
                        op ^= 0x3000;       /* move.b */
                }
                op |= d6;
                pokewd(op);
                pokewd(v->offset);
            }
            d6++;
            if (d6 >= 7)
                toovarerr();
            *a4++ = d2;
            if (c != ',')
                break;
            c = getchar3();
        }
        if (c != '}')
            syntaxerr();
    }
    p = thisproc;
    p->nparams = d6 | (d5 << 4);
    /* do all the string things */
    a0 = (UWORD *)stbuff;
    for (i = d5; i-- > 0;) {
        funcst[1] = i * 4 + 8;
        pokel(tl(funcst));
        pokewd(*a0++ | movedna3[0]);
        least[1] = *a0++;
        pokel(tl(least));
        tokejsr(lastring);
    }
    getchar3();
}

/* blitz2.s:13623 linkput - poke link a4,#n + jsr clrloc at a0 */
static void linkput(ULONG a0)
{
    ULONG save;

    if (nomemleft)
        return;
    save = pc;
    pc = a0;
    pokel(tl(linka4));
    tokejsr2(clrloc);
    pc = save;
}

/* blitz2.s:13636 procfixer - fix up procs! (rewrites the 16 nop bytes) */
static void procfixer(void)
{
    struct proc *p = thisproc;
    ULONG a0 = p->pc;       /* pc of proc */
    UWORD d4;

    if (p->linksize) {
        linka4[1] = p->linksize;
        if (p->nstatics == 0) {
            /* link only */
            a0 += 6;
            p->pc = a0;
            linkput(a0);
            return;
        }
        /* link and jsr */
        linkput(a0);
        a0 += 10;
        p = thisproc;
    } else if (p->nstatics == 0) {
        p->pc = a0 + 16;
        return;
    }
    /* .cont */
    if (!nomemleft) {
        ULONG save = pc;
        pc = a0;
        pokewd(0x4eb9);     /* jsr */
        addoff();
        pokel(0x4eb9);      /* asm: d1 - overwritten right below */
        L_(pc - 4) = save;
        pc = save;
    }
    /* .contskip */
    d4 = p->nstatics;
    if (d4 & 0x8000) {
        tokejsr2(newmem);
        p = thisproc;
        d4 = p->nstatics & 32767;
    }
    if (d4) {
        /* allocate statics! */
        UWORD n = p->nparams & 15;
        if (n >= 5)
            pokewd(0x2f04);             /* move.l d4,-(a7) */
        datastart(locdatast, d4);
        tokejsr2(localstat);
        if (n >= 5)
            pokewd(0x281f);             /* move.l (a7)+,d4 */
    }
    pokewd(0x4e75);
}

/* blitz2.s:14001 endstate - End Statement / End Function */
static void endstate(void)
{
    struct addrnode *a2;
    struct proc *p;
    ULONG tab, save;
    UWORD d4, n;

    /* first, fill in any FUNCTION RETURN type shit..... */
    if (procmode == 0)
        badpenderr();
    if (!nomemleft)
        for (a2 = firstpend; a2; a2 = a2->next)
            L_(a2->addr) = pc;
    if (debugga)
        maketrap(5);
    d4 = calcstatic2(firstlocal, &tab);
    p = thisproc;
    p->nstatics = d4;
    locdatast = tab;
    if ((WORD)memlib->flags < 0) {
        p->nstatics |= 0x8000;          /* bset #7,8(a0) */
        tokejsr2(freelast);
    } else
        memlib->flags = memlibstat;
    p = thisproc;
    save = p->pc;
    p->linksize = locvaroff & ~3;
    if (p->rettype == T_STRING)
        pokewd(getstlen[0]);
    if (p->linksize) {
        uselib(64635);
        pokewd(unlinka4[0]);
    }
    /* fix the stacked string lengths! */
    p = thisproc;
    n = p->nparams >> 4;
    if (n) {
        if (n == 1)
            pokewd(fix1[0]);
        else {
            UWORD d1 = n << 2;
            moverts[1] = d1;
            pokel(tl(moverts));
            if (d1 <= 8)
                pokewd(((d1 & 7) << 9) | fixq[0]);
            else {
                fixstack[1] = d1;
                pokel(tl(fixstack));
            }
        }
    }
    normret();
    /* put in the setup stuff here! */
    procfixer();
    if (!nomemleft)
        L_(save - 4) = pc;              /* the jmp around the proc */
    procmode = 0;
    thisproc = NULL;
    procnum = 0;
    droplocals();
    freepends();
    getchar3();
}

/* blitz2.s:14104 droplocals */
void droplocals(void)
{
    struct alllocal *a0 = doallocmem(12, MEMF_PUBLIC);
    struct var *d0, *d1;

    a0->next = alllocals;
    alllocals = a0;
    d0 = firstlocal;
    firstlocal = NULL;
    a0->locals = d0;
    d1 = firstglob;
    firstglob = NULL;
    a0->globs = d1;
    if (debugga && !nomemleft) {
        L_(proclocals) = (ULONG)d0;
        L_(proclocals + 4) = (ULONG)d1;
    }
}

/* ============================================================== misc */

/* blitz2.s:13715 domwait */
void domwait(UWORD c)
{
    (void)c;
    pokecode(mwait, mwaitf);
}

/* blitz2.s:13975 dostop */
void dostop(UWORD c)
{
    (void)c;
    if (dirmode)
        baddirerr();
    if (!debugga)
        badstoperr();
    tokejsr(0xd501);
}

/* blitz2.s:13983 docont - cont not currently supported! */
void docont(UWORD c)
{
    (void)c;
    badconterr();
}

/* blitz2.s:13985 nop */
void nop_(void)
{
    pokewd(0x4e71);
}

/* ============================================================= NewType */

/* blitz2.s:13719 donewtype */
void donewtype(UWORD c)
{
    struct node *last;
    struct type *t;
    UWORD len;

    if (c != '.')
        syntaxerr();
    len = makename(&c);
    if (!len)
        laberr();
    t = findtype((struct node *)&alltypes, len, &last);
    if (t) {
        if (t->u.code != TYPE_FORWARD)
            typeerr();
    } else
        t = (struct type *)addhere(last, NAMEOFF_TYPE, len);
    t->u.code = 0;
    thistype = t;
    prevtype = NULL;
    typelen = 0;
    linemode = 0;
    c = reget();
    if (!eos(c))
        typemode(c);
}

/* blitz2.s:13740 typemode - one field of a NewType */
void typemode(UWORD c)
{
    struct node *last;
    struct var *f;
    struct type *d2;
    UWORD d6, len;
    ULONG d3, d4;

    if (c == 0x8002) {
        doend2();
        return;
    }
    bakup();
    d6 = 0;                 /* not pointer */
    if (c == '*') {
        d6 = 1;
        getchar3();
    } else if (c == '@') {
        d6 = 5;
        getchar3();
    }
    flagmask = d6;
    len = makename(&c);
    if (!len)
        syntaxerr();
    if (findvar((struct node *)&thistype->u.fields, len, &last))
        dupofferr();
    f = (struct var *)addhere(last, NAMEOFF_VAR, len);
    c = reget();
    /* .skip2 */
    if (c == '.') {
        len = makename(&c);
        if (!len)
            syntaxerr();
        d2 = findtype((struct node *)&alltypes, len, &last);
        if (!d2) {
            if (!(flagmask & 1))
                notypeerr();
            /* pointer type, gotta do something tricky! */
            d2 = (struct type *)addhere(last, NAMEOFF_TYPE, len);
            c = reget();
            d2->size = linenumat;
            d2->u.code = TYPE_FORWARD;  /* set to pointer crap */
        }
        if (d2 != &bytetype)
            typelen = (typelen + 1) & ~1;
    } else if (c == '$') {
        d2 = &stringtype;
        c = getchar3();
        typelen = (typelen + 1) & ~1;
    } else {
        d2 = prevtype;
        if (!d2)
            notypeerr();
        if (d2 != &bytetype)
            typelen = (typelen + 1) & ~1;
    }
    /* .byte */
    d3 = (UWORD)d2->size;
    prevtype = d2;
    f->type = d2;
    d4 = typelen;
    f->offset = d4;
    if (flagmask & 1)
        d3 = 4;
    if (c == '[') {
        /* get constant value! */
        ULONG d1 = evalconst2(&c);
        if (c != ']')
            syntaxerr();
        if (numd6)
            fpconerr();
        getchar3();
        f->ndims = d1;
        d3 = (ULONG)(UWORD)d1 * (UWORD)d3;      /* mulu d1,d3 */
        flagmask |= 2;
    }
    d4 += d3;
    if (d4 >= 32768)
        toolongerr();
    f->flags = flagmask;
    typelen = d4;
}

/* ============================================================ End If */

/* blitz2.s:13830 doendif3 */
static struct ifnode *doendif3(UWORD kind)
{
    struct ifnode *a0 = firstif;

    if (!a0)
        noiferr();
    if ((a0->kind & 255) != kind)
        badenderr();
    doendif2(a0, kind);
    return a0;
}

/* blitz2.s:13837 doendif2 */
void doendif2(struct ifnode *a0, UWORD kind)
{
    ULONG d1;

    if (kind == 1) {
        /* put in while bra */
        ULONG a1;
        pokewd(0x6000);
        a1 = pc;
        pokewd(a0->startpc - a1);
    }
    d1 = pc - a0->brapc + 2;
    if (d1 >= 32768)
        bigiferr();
    if (!nomemleft)
        W_(a0->brapc - 2) = d1;
}

/* blitz2.s:13860 dowend */
void dowend(UWORD c)
{
    (void)c;
    bakup();
    dowend2();
}

/* blitz2.s:13861 dowend2 */
static void dowend2(void)
{
    doendif(1);
}

/* blitz2.s:13863 mydoendif2 */
void mydoendif2(UWORD c)
{
    (void)c;
    bakup();
    mydoendif();
}

/* blitz2.s:13864 mydoendif */
static void mydoendif(void)
{
    doendif(0);
}

/* blitz2.s:13865 doendif */
static void doendif(UWORD kind)
{
    freetheifz(doendif3(kind));
    getchar3();
}

/* blitz2.s:13869 freetheifz */
void freetheifz(struct ifnode *a0)
{
    firstif = a0->next;
    FreeMem(a0, 14);
}

/* ================================================================ End */

/* blitz2.s:13875 doend2 */
static void doend2(void)
{
    doend(getchar3());
}

/* blitz2.s:13876 doend */
void doend(UWORD c)
{
    if (eos(c)) {
        /* .done */
        if (dirmode)
            baddirerr();
        tokejsr(endjmp);
        return;
    }
    do {
        doend3(c);
        c = reget();
    } while (!eos(c));
}

/* blitz2.s:13882 doend .doend3 */
static void doend3(UWORD c)
{
    if (c == 0x8001) {                  /* End NewType */
        if (linemode)
            modeerr();
        typelen = (typelen + 1) & ~1;
        thistype->size = typelen;
        linemode = -1;
        getchar3();
        return;
    }
    if (c == 0x800b) {                  /* End If */
        mydoendif();
        return;
    }
    if (c == 0x800c) {                  /* End While */
        dowend2();
        return;
    }
    if (c == 0x8008) {                  /* End Statement */
        endstate();
        return;
    }
    if (c == 0x8009) {                  /* End Function */
        pokewd(0x7000);                 /* moveq #0,d0 */
        if (thisproc->rettype == T_STRING)
            pokewd(putstlen[0]);
        endstate();
        return;
    }
    if (c == 0x8000 + 48) {             /* End SetInt */
        if (procmode)
            interr3();
        if (debugga) {
            maketrap(3);
            if (!nomemleft) {
                L_(intcleanat) = pc;
                tokejsr(gouse);         /* for runerrlib stack chex */
            }
        }
        if (intstring == 0)
            interr();
        if (intstring < 0) {
            pokewd(intlevel | 0x7200);  /* moveq #level,d1 */
            tokejsr(oldint);
            intlevel = 0;
        }
        intstring = 0;
        pokel(tl(intfin));
        pokel(0x70004e75);
        if (!nomemleft)
            L_(intjmpat) = pc;
        getchar3();
        return;
    }
    if (c == 0x800e) {                  /* end select? */
        doendsel(c);
        return;
    }
    if (c == 0x8001 + tnum) {           /* End SetErr */
        doendseterr(c);
        return;
    }
    syntaxerr();
}

/* blitz2.s:13988 freefors */
void freefors(void)
{
    struct fornode *a2 = firstfor, *a1;

    firstfor = NULL;
    while (a2) {
        a1 = a2;
        a2 = a1->next;
        FreeMem(a1, a1->len);
    }
}

/* ========================================================== For / Next */

/* blitz2.s:14130 donext */
void donext(UWORD c)
{
    if (dirmode)
        baddirerr();
    if (!firstfor)
        noforerr();
    c = reget();
    if (eos(c)) {
        donext2(firstfor);              /* .unknown */
        return;
    }
    /* here, we've got 'Next Var[,Var...]' */
    for (;;) {
        struct fornode *a2;
        UWORD d2;

        c = getvname(c);
        d2 = getvnamelen;
        a2 = firstfor;
        if ((UWORD)(a2->len - 18) != d2)
            noforerr();
        if (memcmp(namebuff, a2->name, d2))
            noforerr();
        donext2(a2);
        c = reget();
        if (eos(c))
            return;
        if (c != ',')
            syntaxerr();
        c = getchar3();
    }
}

/* blitz2.s:14163 donext .unknown2 - close For node a2 (= firstfor) */
static void donext2(struct fornode *a2)
{
    UWORD *a0 = nextl, *a1 = nextlf;
    UWORD d0;
    ULONG d1;

    popnext(a2);            /* error checking for next. */
    d0 = a2->vartype;
    if (d0 == 1) {
        a0 = nextb; a1 = nextbf;
    } else if (d0 == 2) {
        a0 = nextw; a1 = nextwf;
    } else if (d0 == 5) {
        tokejsr(getffpbase);
        a0 = nextf; a1 = nextff;
    }
    pokecode(a0, a1);
    pokel(tl(thebra));
    firstfor = a2->next;
    /* pass2, o.k. to put in my BRA's! */
    d1 = pc - a2->brapc + 2;            /* d1=positive bra */
    if (d1 >= 32768)
        bigforerr();
    if (!nomemleft)
        W_(a2->brapc - 2) = d1;
    d1 = pc - a2->toppc - 6;
    if (d1 >= 32768)
        bigforerr();
    if (!nomemleft)
        W_(pc - 2) = -d1;
    popnext3(a2);           /* fix stack */
    FreeMem(a2, a2->len);
    reget();
}

/* blitz2.s:14228 dofor - For var = START To FINISH [Step INCREMENT] */
void dofor(UWORD c)
{
    struct node *last;
    struct fornode *f;
    UWORD d2, t, op;
    WORD d3;
    UWORD *a0, *a1;

    if (dirmode)
        baddirerr();
    forlineat = linenumat;
    /* new stuff! */
    letstart = (ULONG)srcptr;
    c = getvname(c);
    fortemp = getvnamelen;
    if (findfor(fortemp, &last))
        fornexterr();
    f = (struct fornode *)addhere2((struct node *)&firstfor, NAMEOFF_FOR,
                                   fortemp);
    c = reget();
    if (debugga) {
        chkstak();
        pokel(tl(forset));
    }
    c = dolet2(c, fortemp, &d2, &d3);
    if (c != 0x8018)
        syntaxerr();
    if (commode)
        syntaxerr();
    if ((UBYTE)d2 >= 6)
        badforerr();
    if (!(d2 & 0x8000)) {
        pokewda5s(leaamp[0]);
        pokewd(d3);
    }
    pokewd(pushindex[0]);   /* index lea on stack */
    d2 &= 255;
    f->vartype = d2;
    t = d2;
    c = eval(&t);           /* get fin */
    op = d2 >= 3 ? pushd0l[0] : pushd0wd[0];
    pokewd(op);             /* push fin on stack */
    if (c == 0x8019) {
        t = d2;
        eval(&t);           /* get step */
    } else if (d2 < 4)
        pokewd(0x7001);     /* moveq #1,d0 */
    else if (d2 == 4) {
        pokewd(0x7001);
        pokewd(swapd0[0]);
    } else {
        pokewd(0x203c);
        pokel(0x80000041);  /* ffp '1' */
    }
    pokewd(op);             /* push step on stack */
    f->toppc = pc;
    if (d2 == 1) {
        a0 = forcompb; a1 = forcompbf;
    } else if (d2 == 2) {
        a0 = forcompw; a1 = forcompwf;
    } else if (d2 == 5) {
        pokecode(forcompf, forcompff);
        tokejsr(getffpbase);
        a0 = forcompf2; a1 = forcompf2f;
    } else {
        a0 = forcompl; a1 = forcomplf;
    }
    pokecode(a0, a1);
    pokel(tl(thebgt));
    f->brapc = pc;
    reget();
}

/* ================================================================ Let */

/* blitz2.s:14338 dolet */
void dolet(UWORD c)
{
    letstart = (ULONG)srcptr;
    c = getvname(c);
    dolet2(c, getvnamelen, NULL, NULL);
}

/* blitz2.s:14468 dolet2 .code - poke the store opcode d1 for type d2 */
static void letcode(UWORD d1, UWORD d2, WORD d3)
{
    if ((UBYTE)d2 == 1) {
        pcodd++;
        d1 ^= 0x3000;
    } else {
        alignletpc();
        if ((UBYTE)d2 < 3)
            d1 |= 0x1000;
    }
    pokewd(d1);
    if (!(d1 & 0x80))
        pokewd(d3);
}

/* blitz2.s:14340 dolet2 */
UWORD dolet2(UWORD c, UWORD len, UWORD *type, WORD *offset)
{
    struct var *v;
    struct type *t;
    UWORD d2, et;
    WORD d3;

    pushstart();
    regat = 1;              /* don't disturb this reg! */
    v = fetchvar(len, &t, &c);              /* var in a3, type in a2 */
    d2 = calcvar(&v, &t, &c, &d3);          /* get offset stuff */
    regat = 0;
    pushdo();
    commode = 0;
    pcodd = 0;
    if (c != '=')
        srcptr = (UBYTE *)letstart - 1;     /* move char to start of var name! */

    for (;;) {              /* .nextcom */
        ULONG svoff;
        WORD svmode;

        if ((UBYTE)d2 == T_STRING) {
            /* string get! */
            svoff = lastoffset;
            svmode = varmode;
            setsvars();     /* !@! */
            et = T_STRING;
            c = eval(&et);
            varmode = svmode;
            lastoffset = svoff;
            pokewd(moved0a3[0]);
            if (commode)
                pokecode(pulla2st, pulla2stf);
            else
                pushput();
            if (!(d2 & 0x8000)) {
                leaa5d3a2[1] = d3;
                pokela5s(tl(leaa5d3a2));
            }
            uselib(65135);
            tokejsr(varmode ? lastring : lastring + 1);  /* +1: global allocmem */
            if (c != ',') {
                /* review: BUG(orig) kept - asm `move varmode,d2` (14389)
                   leaves d2 = varmode (0/$ffff) as the returned type, so
                   `For a$=..` passes dofor's type check for a global a$ */
                d2 = (UWORD)varmode;
                break;
            }
            pokewd(addq4a2[0]);
        } else {
            UWORD d1;

            if (d2 & 0x4000)                /* pointer, ignore! */
                d2 = (d2 & 0xff00) | 3;
            else if ((UBYTE)d2 == 0)
                noleterr();                 /* tried to assign a struct - later! */
            /* let's get what is to be assigned..... */
            svmode = varmode;
            svoff = lastoffset;
            et = d2 & 0xff;
            c = eval(&et);
            lastoffset = svoff;
            if (!commode)
                pushput();
            varmode = svmode;
            d1 = 0x2b40;                    /* move.l d0,x(a5) */
            if (varmode)
                d1 &= ~0x200;
            if (!(d2 & 0x8000)) {
                letcode(d1, d2, d3);
                break;
            }
            if (commode)
                pokewd(pulla2[0]);
            d1 = 0x2480;                    /* move.l d0,(a2) */
            if (c != ',') {
                letcode(d1, d2, d3);
                break;
            }
            letcode(0x24c0, d2, d3);        /* move.l d0,(a2)+ */
        }
        /* .nextstr */
        {
            struct var *a0;
            ULONG nt;

            if (!lastoffset)
                nocomerr();
            commode = -1;
            varcodelen = 0;
            pokewd(pusha2[0]);
            a0 = ((struct var *)lastoffset)->next;
            if (!a0)
                comerr();                   /* comma past end of struct */
            lastoffset = (ULONG)a0;
            if (a0->flags & 2)
                nocomerr();                 /* can't use comma on multiple entries */
            nt = a0->type->u.code;
            if (a0->flags & 1) {            /* pointer */
                if (nt >= 256)
                    nt = 0;
                nt |= 0x4000;
            } else if (nt >= 256)
                nocomerr();                 /* or on structs */
            d2 = nt | 0x8000;
        }
    }
    if (type)
        *type = d2;
    if (offset)
        *offset = d3;
    return c;
}

/* blitz2.s:14498 alignletpc */
static void alignletpc(void)
{
    if (pcodd & 1) {
        pcodd = 0;
        pokewd(add1a2[0]);
    }
}

/* ================================================================ Dim */

/* blitz2.s:14513 dodim - dim an array */
void dodim(UWORD c)
{
    regat++;                /* get things into d1 */
    uselib(65434);          /* arrays lib! */
    dodim2(c);
}

/* blitz2.s:14520 dodim2 */
static void dodim2(UWORD c)
{
    struct var *v;
    struct type *t;

    for (;;) {
        listadd = 0;
        if (c == 0x8000 + tnum) {
            listadd = 8;    /* it's a list */
            uselib(65530);  /* use memlib */
            c = getchar3();
        }
        nonew = 0;
        c = getvname(c);
        if (!(flagmask & 2))
            syntaxerr();
        v = fetchvar(getvnamelen, &t, &c);
        if (!added) {
            /* Here, an Array is being Re-Dimmed */
            SETHIW(temp1, v->ndims);      /* move 8(a3),temp1: high word */
            if (procmode == 0) {
                WORD save = varoff;
                varoff = v->offset + 4;
                dimwit(v, t);
                varoff = save;
            } else {
                WORD save = locvaroff;
                locvaroff = v->offset - 4;
                dimwit(v, t);
                locvaroff = save;
            }
        } else {
            /* here, we are doing a brand new dim! */
            SETHIW(temp1, 0xffff);        /* no care on # dims */
            if (listadd) {
                SETHIW(temp1, 0x4000);    /* yes, we do care - must be 1 coz it's a list */
                v->flags |= 0x0100;     /* set to list type of array */
            }
            dimwit(v, t);
        }
        c = getchar3();
        if (c != ',')
            return;
        c = getchar3();
    }
}

/* blitz2.s:14571 dimwit */
static void dimwit(struct var *a3, struct type *a2)
{
    UWORD d1, t;
    UWORD c;

    nonew = -1;
    pokewd(beginarr[0]);
    pokewd(0);
    d1 = 4;
    if (!(flagmask & 1))
        d1 = a2->size;
    d1 += listadd;          /* size of one element. */
    listsize = d1;
    pokewd(d1);
    SETHIW(temp2, 0);         /* # dim's got */
    for (;;) {
        SETHIW(temp2, HIW(temp2) + 1);
        t = T_LONG;         /* get word */
        c = eval(&t);
        if (debugga)
            tokejsr(arrchk);
        tokejsr(arrmult);
        if (procmode) {
            locvaroff -= 4;
            putarr[1] = locvaroff;
        } else {
            putarr[1] = varoff;
            varoff += 4;
        }
        pokela5d(tl(putarr));
        if (c == ',')
            continue;
        if (c != ')')
            syntaxerr();
        break;
    }
    d1 = HIW(temp1);
    if ((WORD)d1 >= 0) {
        if (d1 & 0x4000) {
            /* is it a list? */
            if (HIW(temp2) != 1)
                redimerr();
            a3->ndims = HIW(temp2);
        } else {
            if (d1 != HIW(temp2))
                redimerr();
            /* BUG(orig): writes the assembler's extension word into
               varoff/locvaroff (harmless: the caller restores it) */
            if (procmode)
                locvaroff = extraword;
            else
                varoff = extraword;
        }
    } else
        a3->ndims = HIW(temp2); /* set # dim's */
    /* .care */
    doarr[1] = a3->offset;
    doarr[3] = putarr[1];
    pokel(tl(&doarr[0]));
    pokel(tl(&doarr[2]));
    d1 = newarr;
    if (procmode)
        d1++;
    if (listadd) {
        pokewd(setles[0]);      /* move #x,d4 */
        pokewd(listsize);
        d1 += 3;                /* for list Tokejsr */
    }
    tokejsr(d1);
}
