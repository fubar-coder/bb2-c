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
 *  - the name length after getvname (asm d2) is getvname's out parameter.
 */
#include <string.h>
#include <proto/exec.h>
#include "blitz2.h"

/* ================================================================ data */

/* The scratch globals labcnt (13368), fortemp (14226), pcodd (14509),
   listadd (14510) and listsize (14511) are locals/parameters of their
   routines in C (doongo, dofor, dolet2, dodim2/dimwit). */
ULONG localslist;           /* blitz2.s:13488 all locals for procs (unused) */
ULONG proclocals;           /* blitz2.s:13489 for debugga!  Global: set by
                               Statement/Function (dofunc2), read by
                               droplocals at End Statement/Function, i.e.
                               in a later statement */
WORD procnum;               /* blitz2.s:13490 */
WORD procnum2;              /* blitz2.s:13491 */
UWORD prolineat;            /* blitz2.s:13492 */
UWORD forlineat;            /* blitz2.s:14225 */

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

static void dofunc2(WORD mode, UWORD c);
static void linkput(ULONG at);
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
static void dimwit(struct var *v, struct type *t, UWORD listadd,
                   UWORD wantdims);
static void alignletpc(UWORD *pcodd);

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
    UWORD *code = ongotocode, *codeend = ongotocodef;
    UWORD skipoff = 26;     /* gc1skip-ongotocode */
    UWORD t = T_WORD;       /* Get expression as a word. */
    ULONG start;
    UWORD labcnt;           /* asm: global labcnt (13368) */
    struct label *lab;

    (void)c;
    c = bakeval(&t);
    if (c != 0x8005) {
        if (c != 0x8006)
            syntaxerr();
        code = ongosubcode; codeend = ongosubcodef;
        skipoff = 26;       /* gc2skip-ongosubcode */
        if (debugga) {
            chkstak();
            code = ongcode; codeend = ongcodef;
            skipoff = 30;   /* gc3skip-ongcode */
        }
    }
    start = pc;            /* for when we know number of labels! */
    pokecode(code, codeend);
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
    W_(start + skipoff + 2) = labcnt * 4 + 2;   /* for bra! */
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

/* blitz2.s:13466 stateret - Statement Return.  Fixed (#940): blitz2.s
   checks neither here nor in funcret the kind of the open procedure (in a
   string Function, Statement Return skipped the pushed length endstate
   pops; in a Statement, Function Return evaluated with type $FF).  A
   Return of the other kind is "Illegal Procedure return" (sreterr, which
   blitz2.s defines and never raises); outside a procedure it stays
   "Illegal End Procedure" (eos_). */
static void stateret(UWORD c)
{
    if (procmode > 0)                       /* inside a Function */
        sreterr();
    eos_(c);
    getchar3();
}

/* blitz2.s:13469 funcret - Function Return expr (Fixed (#940), see
   stateret; outside a procedure blitz2.s read the type of thisproc NULL) */
static void funcret(UWORD c)
{
    UWORD t;

    if (procmode == 0)
        badpenderr();
    if (procmode < 0)                       /* inside a Statement */
        sreterr();
    t = (UBYTE)thisproc->rettype;           /* type returning! */
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
    UWORD mode;

    if (c == 0x8007) {
        funcret(c);
        return;
    }
    bakup();
    if (!ext_procrettype(&mode))
        mode = sizespec2();
    c = getchar3();
    dofunc2(mode, c);       /* falls through into dofunc2 */
}

/* blitz2.s:13504 dofunc2 - do some checking..... mode = procmode value (asm d1)
   (-1 statement, else the function's type) */
static void dofunc2(WORD mode, UWORD c)
{
    UWORD namelen;          /* asm d2 after getvname */
    struct node *last;
    struct proc *p;
    struct var *v;
    struct type *t;
    UBYTE *types;
    UWORD npars, nstrs, partype, len, i;
    UWORD *st;

    (void)c;
    errchx();
    prolineat = linenumat;
    procnum2++;
    procnum = procnum2;
    procmode = mode;
    locvaroff = 0;
    ext_procfast();
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
    ext_procdef(p);
    pokecode2(nops8, nopsf);
    if (debugga) {
        maketrap(4);
        proclocals = pc;
        pokel(0x4e44);      /* asm: d1 from maketrap - filled by droplocals */
        pokel(0x4e44);
    }
    types = p->params;
    p->rettype = (BYTE)procmode;            /* move.b procmode+1,5(a2) */
    npars = 0;
    nstrs = 0;
    c = getchar3();
    if (c != '}') {
        for (;;) {
            c = ext_procoptional(p, npars, c);
            c = getvname(c, &namelen);
            v = fetchvar(namelen, &t, &c);
            if (t->u.code == T_STRING) {
                UWORD *sb = (UWORD *)stbuff;
                sb[nstrs * 2] = npars;
                sb[nstrs * 2 + 1] = v->offset;
                nstrs++;
                partype = T_STRING;
            } else {
                UWORD op = 0x2940;          /* move.l dN,x(a4) */
                partype = T_LONG;
                if (!(flagmask & 1)) {
                    if (t->u.code >= 256)
                        illprocperr();
                    partype = TYPE_CODE(t);
                    if (partype == 2)
                        op |= 0x1000;       /* move.w */
                    else if (partype < 2)
                        op ^= 0x3000;       /* move.b */
                }
                if (!ext_procparam(npars, partype, v->offset)) {
                    op |= npars;
                    pokewd(op);
                    pokewd(v->offset);
                }
            }
            npars++;
            if (npars >= ext_procmax())
                toovarerr();
            *types++ = partype;
            if (c != ',')
                break;
            c = getchar3();
        }
        if (c != '}')
            syntaxerr();
    }
    p = thisproc;
    p->nparams = npars | (nstrs << 4);
    /* do all the string things */
    st = (UWORD *)stbuff;
    if (!ext_procentry(p, st)) {
        for (i = nstrs; i-- > 0;) {
            funcst[1] = i * 4 + 8;
            pokel(tl(funcst));
            pokewd(*st++ | movedna3[0]);
            least[1] = *st++;
            pokel(tl(least));
            tokejsr(lastring);
        }
    }
    getchar3();
}

/* blitz2.s:13623 linkput - poke link a4,#n + jsr clrloc at a0 */
static void linkput(ULONG at)
{
    ULONG save;

    if (nomemleft)
        return;
    save = pc;
    pc = at;
    pokel(tl(linka4));
    if (ext_procclrloc(thisproc))
        tokejsr2(clrloc);
    pc = save;
}

/* blitz2.s:13636 procfixer - fix up procs! (rewrites the 16 nop bytes) */
static void procfixer(void)
{
    struct proc *p = thisproc;
    ULONG at = p->pc;       /* pc of proc */
    UWORD statics;

    if (p->linksize) {
        linka4[1] = p->linksize;
        if (p->nstatics == 0) {
            /* link only */
            at += 6;
            p->pc = at;
            linkput(at);
            return;
        }
        /* link and jsr */
        linkput(at);
        at += 10;
        p = thisproc;
    } else if (p->nstatics == 0) {
        p->pc = at + 16;
        return;
    }
    /* .cont */
    if (!nomemleft) {
        ULONG save = pc;
        ext_procstatics(p);
        pc = at;
        pokewd(0x4eb9);     /* jsr */
        addoff();
        pokel(0x4eb9);      /* asm: d1 - overwritten right below */
        L_(pc - 4) = save;
        pc = save;
    }
    /* .contskip */
    statics = p->nstatics;
    if (statics & 0x8000) {
        tokejsr2(newmem);
        p = thisproc;
        statics = p->nstatics & 32767;
    }
    if (statics) {
        /* allocate statics! */
        UWORD n = p->nparams & 15;
        if (n >= 5)
            pokewd(0x2f04);             /* move.l d4,-(a7) */
        datastart(locdatast, statics);
        tokejsr2(localstat);
        if (n >= 5)
            pokewd(0x281f);             /* move.l (a7)+,d4 */
    }
    pokewd(0x4e75);
}

/* blitz2.s:14001 endstate - End Statement / End Function */
static void endstate(void)
{
    struct addrnode *pend;
    struct proc *p;
    ULONG tab, save;
    UWORD statics, n;

    /* first, fill in any FUNCTION RETURN type shit..... */
    if (procmode == 0)
        badpenderr();
    if (!nomemleft)
        for (pend = firstpend; pend; pend = pend->next)
            L_(pend->addr) = pc;
    if (debugga)
        maketrap(5);
    statics = calcstatic2(firstlocal, &tab);
    p = thisproc;
    p->nstatics = statics;
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
        pokewd(getstlen[0]);    /* the length of End Function / Function
                                   Return (Fixed (#940): only those end a
                                   Function) */
    if (p->linksize) {
        uselib(64635);
        pokewd(unlinka4[0]);
    }
    /* fix the stacked string lengths! */
    p = thisproc;
    n = ext_procstackbytes(p);
    if (n) {
        if (n == 4)
            pokewd(fix1[0]);
        else {
            UWORD size = n;
            moverts[1] = size;
            pokel(tl(moverts));
            if (size <= 8)
                pokewd(((size & 7) << 9) | fixq[0]);
            else {
                fixstack[1] = size;
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
    struct alllocal *al = doallocmem(12, MEMF_PUBLIC);
    struct var *locals, *globs;

    al->next = alllocals;
    alllocals = al;
    locals = firstlocal;
    firstlocal = NULL;
    al->locals = locals;
    globs = firstglob;
    firstglob = NULL;
    al->globs = globs;
    if (debugga && !nomemleft) {
        L_(proclocals) = (ULONG)locals;
        L_(proclocals + 4) = (ULONG)globs;
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

/* Fixed (#974): not in blitz2.s - the by-value fields of a type whose size
   is not known yet.  A field of a type that so far only a pointer field
   named (TYPE_FORWARD), or of a type that has such a field itself (by
   value, also over several types), is laid out with the size 0 and
   recorded here; its type (the owner) is incomplete from that field on.
   When the field's type is complete (its End NEWTYPE, or its own last
   such field resolved), the owner is laid out again with typemode's
   rules, and so on for the types that contain the owner.  struct type is
   not changed: residents store it.  Until then a use of the owner's size
   or of the offset of a field behind the recorded one is "Type not yet
   defined"; at the end of the pass a field left here is "Type Not Found"
   (pointchk). */
struct fwdfield {
    struct fwdfield *next;
    struct type *owner;     /* the type that has the field */
    struct var *field;
    struct type *dep;       /* the field's type: forward or incomplete */
    WORD line;              /* linenumat of the field */
};
static struct fwdfield *fwdfields;

/* the type has a by-value field whose size is not known yet */
static BOOL haspending(const struct type *t)
{
    const struct fwdfield *e;

    for (e = fwdfields; e; e = e->next)
        if (e->owner == t)
            return TRUE;
    return FALSE;
}

/* the size of the type is not final yet: forward or incomplete */
static BOOL notfinal(const struct type *t)
{
    return t->u.code == TYPE_FORWARD || haspending(t);
}

/* the type contains `to` by value over recorded fields */
static BOOL reaches(const struct type *from, const struct type *to)
{
    const struct fwdfield *e;

    if (from == to)
        return TRUE;
    for (e = fwdfields; e; e = e->next)
        if (e->owner == from && reaches(e->dep, to))
            return TRUE;
    return FALSE;
}

/* lay the fields of t out again, as typemode does */
static void relayout(struct type *t)
{
    struct var *f;
    ULONG len = 0, size;

    for (f = t->u.fields; f; f = f->next) {
        if (f->type != &bytetype)
            len = (len + 1) & ~1UL;
        size = (f->flags & 1) ? 4 : (UWORD)f->type->size;
        if (f->flags & 2)
            size = (ULONG)(UWORD)f->ndims * (UWORD)size;
        f->offset = len;
        len += size;
        if (len >= 32768)
            toolongerr();
    }
    t->size = (len + 1) & ~1UL;
}

/* t is complete: lay out again every type waiting for it */
static void fwdcomplete(struct type *t)
{
    struct fwdfield **pp = &fwdfields, *e;
    struct type *owner;

    while ((e = *pp)) {
        if (e->dep != t) {
            pp = &e->next;
            continue;
        }
        owner = e->owner;
        *pp = e->next;
        FreeMem(e, sizeof *e);
        if (!haspending(owner)) {
            relayout(owner);
            fwdcomplete(owner);
            pp = &fwdfields;            /* the list has changed */
        }
    }
}

void chktypesize(const struct type *t)
{
    if (notfinal(t))
        notdeferr();
}

void chkfieldoffset(const struct type *t, const struct var *f)
{
    const struct var *g;
    const struct fwdfield *e;

    if (!haspending(t))
        return;
    for (g = t->u.fields; g != f; g = g->next)
        for (e = fwdfields; e; e = e->next)
            if (e->field == g)
                notdeferr();
}

void fwdfieldchk(void)
{
    struct fwdfield *e, *at = fwdfields;

    if (!at)
        return;
    for (e = fwdfields; e; e = e->next) {
        if (e->dep->u.code == TYPE_FORWARD) {
            at = e;
            break;
        }
    }
    linenumat = at->line;
    notypeerr();
}

void freefwdfields(void)
{
    struct fwdfield *e = fwdfields, *dead;

    fwdfields = NULL;
    while (e) {
        dead = e;
        e = e->next;
        FreeMem(dead, sizeof *dead);
    }
}

/* blitz2.s:13740 typemode - one field of a NewType */
void typemode(UWORD c)
{
    struct node *last;
    struct var *f;
    struct type *ftype;
    UWORD flags, len;
    ULONG size, offset;
    BOOL pending;

    if (c == 0x8002) {
        doend2();
        return;
    }
    bakup();
    flags = 0;              /* not pointer */
    if (c == '*') {
        flags = 1;
        getchar3();
    } else if (c == '@') {
        flags = 5;
        getchar3();
    }
    flagmask = flags;
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
        ftype = findtype((struct node *)&alltypes, len, &last);
        if (!ftype) {
            if (!(flagmask & 1))
                notypeerr();
            /* pointer type, gotta do something tricky! */
            ftype = (struct type *)addhere(last, NAMEOFF_TYPE, len);
            c = reget();
            ftype->size = linenumat;
            ftype->u.code = TYPE_FORWARD; /* set to pointer crap */
        }
        if (ftype != &bytetype)
            typelen = (typelen + 1) & ~1;
    } else if (c == '$') {
        ftype = &stringtype;
        c = getchar3();
        typelen = (typelen + 1) & ~1;
    } else {
        ftype = prevtype;
        if (!ftype)
            notypeerr();
        if (ftype != &bytetype)
            typelen = (typelen + 1) & ~1;
    }
    /* Fixed (#973): a field of the type being defined, not a pointer
       (x.a, or an untyped field behind *nxt.a), took the type's size
       before End NEWTYPE sets it - whatever the node's memory held.  A
       type cannot contain itself: "Illegal Type". */
    if (ftype == thistype && !(flagmask & 1))
        illtypeerr();
    /* Fixed (#974): a field of a type that so far only a pointer field
       named (TYPE_FORWARD, its size holds linenumat, the line of that
       pointer field), not a pointer itself (x.c, or an untyped field
       behind *p.c), took that line number as its size; the type defined
       later with NEWTYPE .c did not change it (pointchk only refused a
       type never defined).  Now it is recorded (fwdfields) and laid out
       when its type is complete; a type that would contain itself that
       way is "Illegal Type". */
    pending = FALSE;
    if (!(flagmask & 1) && notfinal(ftype)) {
        struct fwdfield *e, **pp;

        if (reaches(ftype, thistype))
            illtypeerr();
        e = doallocmem(sizeof *e, MEMF_PUBLIC);
        e->next = NULL;
        e->owner = thistype;
        e->field = f;
        e->dep = ftype;
        e->line = linenumat;
        for (pp = &fwdfields; *pp; pp = &(*pp)->next)
            ;
        *pp = e;
        pending = TRUE;
    }
    /* .byte */
    size = (UWORD)ftype->size;
    if (pending)
        size = 0;
    prevtype = ftype;
    f->type = ftype;
    offset = typelen;
    f->offset = offset;
    if (flagmask & 1)
        size = 4;
    if (c == '[') {
        /* get constant value! */
        WORD isfloat;
        ULONG count = evalconst2(&c, &isfloat);
        if (c != ']')
            syntaxerr();
        if (isfloat)
            fpconerr();
        getchar3();
        f->ndims = count;
        size = (ULONG)(UWORD)count * (UWORD)size; /* mulu d1,d3 */
        flagmask |= 2;
    }
    offset += size;
    if (offset >= 32768)
        toolongerr();
    f->flags = flagmask;
    typelen = offset;
}

/* ============================================================ End If */

/* blitz2.s:13830 doendif3 */
static struct ifnode *doendif3(UWORD kind)
{
    struct ifnode *node = firstif;

    if (!node)
        noiferr();
    if ((node->kind & 255) != kind)
        badenderr();
    doendif2(node, kind);
    return node;
}

/* blitz2.s:13837 doendif2 */
void doendif2(struct ifnode *node, UWORD kind)
{
    ULONG dist;

    if (kind == 1) {
        /* put in while bra */
        ULONG at;
        pokewd(0x6000);
        at = pc;
        pokewd(node->startpc - at);
    }
    dist = pc - node->brapc + 2;
    if (dist >= 32768)
        bigiferr();
    if (!nomemleft)
        W_(node->brapc - 2) = dist;
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
void freetheifz(struct ifnode *node)
{
    firstif = node->next;
    FreeMem(node, 14);
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
        /* Fixed (#974): the types waiting for this one are laid out */
        if (!haspending(thistype))
            fwdcomplete(thistype);
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
    /* Fixed (#940): blitz2.s does not check End Statement / End Function
       against the open procedure (procmode): a string Function ended with
       End Statement popped its return address as the length.  The End of
       the other kind is "Illegal End Procedure", as outside a procedure. */
    if (c == 0x8008) {                  /* End Statement */
        if (procmode > 0)               /* inside a Function */
            badpenderr();
        endstate();
        return;
    }
    if (c == 0x8009) {                  /* End Function */
        if (procmode <= 0)              /* a Statement or no procedure */
            badpenderr();
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
    struct fornode *f = firstfor, *dead;

    firstfor = NULL;
    while (f) {
        dead = f;
        f = dead->next;
        FreeMem(dead, dead->len);
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
        struct fornode *f;
        UWORD len;

        c = getvname(c, &len);
        f = firstfor;
        if ((UWORD)(f->len - 18) != len)
            noforerr();
        if (memcmp(namebuff, f->name, len))
            noforerr();
        donext2(f);
        c = reget();
        if (eos(c))
            return;
        if (c != ',')
            syntaxerr();
        c = getchar3();
    }
}

/* blitz2.s:14163 donext .unknown2 - close For node a2 (= firstfor) */
static void donext2(struct fornode *f)
{
    UWORD *code = nextl, *codeend = nextlf;
    UWORD vtype;
    ULONG exitdist, backdist;

    popnext(f);             /* error checking for next. */
    vtype = f->vartype;
    if (vtype == 1) {
        code = nextb; codeend = nextbf;
    } else if (vtype == 2) {
        code = nextw; codeend = nextwf;
    } else if (vtype == 5) {
        tokejsr(getffpbase);
        code = nextf; codeend = nextff;
    }
    pokecode(code, codeend);
    pokel(tl(thebra));
    firstfor = f->next;
    /* pass2, o.k. to put in my BRA's! */
    exitdist = pc - f->brapc + 2;       /* d1=positive bra */
    if (exitdist >= 32768)
        bigforerr();
    if (!nomemleft)
        W_(f->brapc - 2) = exitdist;
    backdist = pc - f->toppc - 6;
    if (backdist >= 32768)
        bigforerr();
    if (!nomemleft)
        W_(pc - 2) = -backdist;
    popnext3(f);            /* fix stack */
    FreeMem(f, f->len);
    reget();
}

/* blitz2.s:14228 dofor - For var = START To FINISH [Step INCREMENT] */
void dofor(UWORD c)
{
    struct node *last;
    struct fornode *f;
    UWORD vartype, t, op, namelen;  /* namelen: asm global fortemp (14226) */
    WORD offset;
    UWORD *code, *codeend;

    if (dirmode)
        baddirerr();
    forlineat = linenumat;
    /* new stuff! */
    letstart = (ULONG)srcptr;
    c = getvname(c, &namelen);
    if (findfor(namelen, &last))
        fornexterr();
    f = (struct fornode *)addhere2((struct node *)&firstfor, NAMEOFF_FOR,
                                   namelen);
    c = reget();
    if (debugga) {
        chkstak();
        pokel(tl(forset));
    }
    c = dolet2(c, namelen, &vartype, &offset);
    if (c != 0x8018)
        syntaxerr();
    if (commode)
        syntaxerr();
    if ((UBYTE)vartype >= 6)
        badforerr();
    if (!(vartype & 0x8000)) {
        pokewda5s(leaamp[0]);
        pokewd(offset);
    }
    pokewd(pushindex[0]);   /* index lea on stack */
    vartype &= 255;
    f->vartype = vartype;
    t = vartype;
    c = eval(&t);           /* get fin */
    op = vartype >= 3 ? pushd0l[0] : pushd0wd[0];
    pokewd(op);             /* push fin on stack */
    if (c == 0x8019) {
        t = vartype;
        eval(&t);           /* get step */
    } else if (vartype < 4)
        pokewd(0x7001);     /* moveq #1,d0 */
    else if (vartype == 4) {
        pokewd(0x7001);
        pokewd(swapd0[0]);
    } else {
        pokewd(0x203c);
        pokel(0x80000041);  /* ffp '1' */
    }
    pokewd(op);             /* push step on stack */
    f->toppc = pc;
    if (vartype == 1) {
        code = forcompb; codeend = forcompbf;
    } else if (vartype == 2) {
        code = forcompw; codeend = forcompwf;
    } else if (vartype == 5) {
        pokecode(forcompf, forcompff);
        tokejsr(getffpbase);
        code = forcompf2; codeend = forcompf2f;
    } else {
        code = forcompl; codeend = forcomplf;
    }
    pokecode(code, codeend);
    pokel(tl(thebgt));
    f->brapc = pc;
    reget();
}

/* ================================================================ Let */

/* blitz2.s:14338 dolet */
void dolet(UWORD c)
{
    UWORD namelen;          /* asm d2 after getvname */
    letstart = (ULONG)srcptr;
    c = getvname(c, &namelen);
    dolet2(c, namelen, NULL, NULL);
}

/* blitz2.s:14468 dolet2 .code - poke the store opcode for type vartype (asm d1, d2) */
static void letcode(UWORD opcode, UWORD vartype, WORD offset, UWORD *pcodd)
{
    if ((UBYTE)vartype == 1) {
        (*pcodd)++;
        opcode ^= 0x3000;
    } else {
        alignletpc(pcodd);
        if ((UBYTE)vartype < 3)
            opcode |= 0x1000;
    }
    pokewd(opcode);
    if (!(opcode & 0x80))
        pokewd(offset);
}

/* blitz2.s:14340 dolet2 */
UWORD dolet2(UWORD c, UWORD len, UWORD *type, WORD *offset)
{
    struct var *v;
    struct type *t;
    UWORD vartype, et;
    UWORD pcodd;            /* asm: global pcodd (14509), add 1 for bytes */
    WORD voff;

    pushstart();
    regat = 1;              /* don't disturb this reg! */
    v = fetchvar(len, &t, &c);              /* var in a3, type in a2 */
    vartype = calcvar(&v, &t, &c, &voff);   /* get offset stuff */
    regat = 0;
    pushdo();
    commode = 0;
    pcodd = 0;
    if (c != '=')
        srcptr = (UBYTE *)letstart - 1;     /* move char to start of var name! */

    for (;;) {              /* .nextcom */
        ULONG svoff;
        WORD svmode;

        if ((UBYTE)vartype == T_STRING) {
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
            if (!(vartype & 0x8000)) {
                leaa5d3a2[1] = voff;
                pokela5s(tl(leaa5d3a2));
            }
            uselib(65135);
            tokejsr(varmode ? lastring : lastring + 1);  /* +1: global allocmem */
            /* Fixed (#39): the type returned stays the string type; the
               asm's `move varmode,d2` (14389) returned varmode (0 for a
               global, $ffff for a local), so `For a$=..` passed dofor's
               type check for a global a$ */
            if (c != ',')
                break;
            pokewd(addq4a2[0]);
        } else {
            UWORD opcode;

            if (vartype & 0x4000)           /* pointer, ignore! */
                vartype = (vartype & 0xff00) | 3;
            else if ((UBYTE)vartype == 0)
                noleterr();                 /* tried to assign a struct - later! */
            /* let's get what is to be assigned..... */
            svmode = varmode;
            svoff = lastoffset;
            et = vartype & 0xff;
            ext_assignstart();
            c = eval(&et);
            ext_assigncheck(vartype);
            lastoffset = svoff;
            if (!commode)
                pushput();
            varmode = svmode;
            opcode = 0x2b40;                /* move.l d0,x(a5) */
            if (varmode)
                opcode &= ~0x200;
            if (!(vartype & 0x8000)) {
                letcode(opcode, vartype, voff, &pcodd);
                break;
            }
            if (commode)
                pokewd(pulla2[0]);
            opcode = 0x2480;                /* move.l d0,(a2) */
            if (c != ',') {
                letcode(opcode, vartype, voff, &pcodd);
                break;
            }
            letcode(0x24c0, vartype, voff, &pcodd); /* move.l d0,(a2)+ */
        }
        /* .nextstr */
        {
            struct var *field;
            ULONG nt;

            if (!lastoffset)
                nocomerr();
            commode = -1;
            varcodelen = 0;
            pokewd(pusha2[0]);
            field = ((struct var *)lastoffset)->next;
            if (!field)
                comerr();                   /* comma past end of struct */
            lastoffset = (ULONG)field;
            if (field->flags & 2)
                nocomerr();                 /* can't use comma on multiple entries */
            nt = field->type->u.code;
            if (field->flags & 1) {         /* pointer */
                if (nt >= 256)
                    nt = 0;
                nt |= 0x4000;
            } else if (nt >= 256)
                nocomerr();                 /* or on structs */
            vartype = nt | 0x8000;
        }
    }
    if (type)
        *type = vartype;
    if (offset)
        *offset = voff;
    return c;
}

/* blitz2.s:14498 alignletpc */
static void alignletpc(UWORD *pcodd)
{
    if (*pcodd & 1) {
        *pcodd = 0;
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
    UWORD namelen;          /* asm d2 after getvname */
    struct var *v;
    struct type *t;
    UWORD listadd;          /* asm: global listadd (14510), 8 = Dim List */
    UWORD wantdims;         /* asm: high word of temp1 (15010) */

    for (;;) {
        listadd = 0;
        if (c == 0x8000 + tnum) {
            listadd = 8;    /* it's a list */
            uselib(65530);  /* use memlib */
            c = getchar3();
        }
        nonew = 0;
        c = getvname(c, &namelen);
        if (!(flagmask & 2))
            syntaxerr();
        v = fetchvar(namelen, &t, &c);
        if (!added) {
            /* Here, an Array is being Re-Dimmed */
            wantdims = v->ndims;          /* move 8(a3),temp1: high word */
            if (procmode == 0) {
                WORD save = varoff;
                varoff = v->offset + 4;
                dimwit(v, t, listadd, wantdims);
                varoff = save;
            } else {
                WORD save = locvaroff;
                locvaroff = v->offset - 4;
                dimwit(v, t, listadd, wantdims);
                locvaroff = save;
            }
        } else {
            /* here, we are doing a brand new dim! */
            wantdims = 0xffff;            /* no care on # dims */
            if (listadd) {
                wantdims = 0x4000;        /* yes, we do care - must be 1 coz it's a list */
                v->flags |= 0x0100;     /* set to list type of array */
            }
            dimwit(v, t, listadd, wantdims);
        }
        c = getchar3();
        if (c != ',')
            return;
        c = getchar3();
    }
}

/* blitz2.s:14571 dimwit */
static void dimwit(struct var *v, struct type *vt, UWORD listadd,
                   UWORD wantdims)
{
    UWORD size, ndims, tok, t;
    UWORD c;

    nonew = -1;
    pokewd(beginarr[0]);
    pokewd(0);
    size = 4;
    if (!(flagmask & 1)) {
        chktypesize(vt);    /* Fixed (#974) */
        size = vt->size;
    }
    size += listadd;        /* size of one element. */
    pokewd(size);
    ndims = 0;              /* # dim's got (asm: temp2) */
    for (;;) {
        ndims++;
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
    if ((WORD)wantdims >= 0) {
        if (wantdims & 0x4000) {
            /* is it a list? */
            if (ndims != 1)
                redimerr();
            v->ndims = ndims;
        } else {
            if (wantdims != ndims)
                redimerr();
            /* writes the assembler's extension word into
               varoff/locvaroff (harmless: the caller restores it) */
            if (procmode)
                locvaroff = extraword;
            else
                varoff = extraword;
        }
    } else
        v->ndims = ndims;       /* set # dim's */
    /* .care */
    doarr[1] = v->offset;
    doarr[3] = putarr[1];
    pokel(tl(&doarr[0]));
    pokel(tl(&doarr[2]));
    tok = newarr;
    if (procmode)
        tok++;
    if (listadd) {
        pokewd(setles[0]);      /* move #x,d4 */
        pokewd(size);           /* asm: listsize (14511) */
        tok += 3;               /* for list Tokejsr */
    }
    tokejsr(tok);
}
