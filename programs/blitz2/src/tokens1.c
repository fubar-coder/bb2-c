/*
 * tokens1.c - chunk 08, blitz2.s 12136-13326: built-in token handlers
 * part 1 (DefType, MaxLen, Free/Use, Else, CERR/CNIF/CSIF, Restore/Read/
 * Data, DC/DCB/DS/Even, Shared, Include/XInclude/IncBin, Macro,
 * Select/Case/Default/End Select, Repeat/Forever/Until, If/While).
 * See notes/survey-08.md.
 *
 * Statement handlers are entered after getchar3 (c = current char/token,
 * asm d0 + Z flag = eos(c)).  doline re-reads the current char with reget()
 * after a handler, so where the asm ended with `bra getchar3` / `bra reget`
 * / `bra bakup` the C code calls that routine too (it updates lastchar /
 * srcptr) and returns.
 */
#include <string.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "blitz2.h"

/* asm d2 after getvname = length of the name in namebuff (eval.h's
   getvname does not return it; the name is NUL terminated there) */
/* a template long (two UWORDs, high first) - avoids type-punned L_() reads
   of UWORD arrays */
static inline ULONG tl(const UWORD *p)
{
    return ((ULONG)p[0] << 16) | p[1];
}

#define VNAMELEN() getvnamelen    /* asm d2 after getvname */

/* ======================================================= templates / data */

/* blitz2.s:12165 maxprep1 / maxprep2 (6 bytes) */
UWORD maxprep1[] = {
    0x45ED, 0x0000,                    /* +0   maxprep1 lea 0(a5),a2 */
    0x200A,                            /* +4   maxprep2 move.l a2,d0 */
};

/* blitz2.s:12247 free..freef (6 bytes); free+2 patched by dofree */
UWORD free_[] = {
    0x266D, 0x0000,                    /* +0   free move.l 0(a5),a3 */
    0xD6C0,                            /* +4   add d0,a3 */
};

/* blitz2.s:12348 rescode / rescode2 (12 bytes); rescode+6 and rescode2+2
   are patched by uselib (chunk 04) with the data pointer's a5 offset */
UWORD rescode[] = {
    0x2B7C, 0x0000, 0x0000, 0x0000,    /* +0   rescode move.l #0,0(a5) */
    0x2B40, 0x0000,                    /* +8   rescode2 move.l d0,0(a5) */
};

/* blitz2.s:12847 macline dc 0 */
WORD macline;

/* blitz2.s:12962 thetst1..negbr (20 bytes) */
UWORD thetst1[] = {
    0x4A00,                            /* +0   thetst1 tst.b d0 */
    0x4A40,                            /* +2   thetst2 tst.w d0 */
    0x4A80,                            /* +4   thetst3 tst.l d0 */
    0x4A9F,                            /* +6   thetst4 tst.l (a7)+ ;Any Length? */
    0x4EAE, 0xFFD0,                    /* +8   thetst5 jsr -48(a6) */
    0x6700, 0xFFFE,                    /* +12  posbr beq posbr */
    0x6600, 0xFFFE,                    /* +16  negbr bne negbr */
};

/* blitz2.s:13014 firstsel dc.l 0 */
struct selnode *firstsel;

/* blitz2.s:13015 selpushb..endsel (26 bytes).  DIFF: and #255,d0 as Devpac
   encodes it (andi, $0240) like the compiler of the CD (2.10, docs/v2.10):
   the programs are byte-identical to those of 2.10; the reference build
   (vasm) has $C07C */
UWORD selpushb[] = {
    0x0240, 0x00FF,                    /* +0   selpushb and #255,d0 */
    0x3F00,                            /* +4   selpushw move d0,-(a7) */
    0x2F00,                            /* +6   selpushl move.l d0,-(a7) */
    0x45EF, 0x0004,                    /* +8   selpushs lea 4(a7),a2 ;(a7)=len of string! */
    0x0240, 0x00FF,                    /* +12  selcomb and #255,d0 */
    0xB057,                            /* +16  selcomw cmp (a7),d0 */
    0xB097,                            /* +18  selcoml cmp.l (a7),d0 */
    0x6600, 0xFFFE,                    /* +20  selbne bne selbne */
    0x504F,                            /* +24  endsel addq #8,a7 */
};

/* blitz2.s:13070 endselstr..endselstrf2 (10 bytes, used by popselect) */
UWORD endselstr[] = {
    0x225F,                            /* +0   endselstr move.l (a7)+,a1 */
    0x5149,                            /* +2   subq #8,a1 */
    0x7009,                            /* +4   moveq #9,d0 */
    0xD091,                            /* +6   add.l (a1),d0 */
    0x544F,                            /* +8   endselstrf addq #2,a7 ;for runtime errs! */
};

/* blitz2.s:13169 pcchk / clrst (6 bytes) */
UWORD pcchk[] = {
    0x3F3C, 0x7345,                    /* +0   pcchk move #"sE",-(a7) */
    0x42A7,                            /* +4   clrst clr.l -(a7) */
};

/* blitz2.s:13173 sellineat dc 0 / 13271 iflineat dc 0 */
WORD sellineat;
WORD iflineat;

/* ===================================================== DefType / MaxLen */

/* blitz2.s:12139 dodeftype */
void dodeftype(UWORD c)
{
    struct type *t, *old, *vt;
    UWORD len;

    if (c != '.')
        syntaxerr();
    len = makename(&c);
    t = findtype((struct node *)&alltypes, len, NULL);
    if (!t)
        notypeerr();
    c = reget();
    if (eos(c)) {
        defaulttype = t;
        return;
    }
    /* deftype.l varlist... */
    old = defaulttype;
    defaulttype = t;
    for (;;) {                                  /* .loopz */
        c = getvname(c);
        fetchvar(VNAMELEN(), &vt, &c);
        if (c != ',')
            break;
        c = getchar3();
    }
    defaulttype = old;                          /* .done */
}

/* blitz2.s:12168 domaxlen - set a strings maximum length. */
void domaxlen(UWORD c)
{
    struct var *v;
    struct type *t;
    WORD off;
    UWORD vtype, type;
    WORD vm;

    c = getvname(c);
    v = fetchvar(VNAMELEN(), &t, &c);
    vtype = calcvar(&v, &t, &c, &off);
    if ((vtype & 0xff) != T_STRING)
        mismatcherr();
    vm = varmode;
    if (c != '=')
        syntaxerr();
    if (!(vtype & TF_ADDRINA2)) {
        pokewda5s(maxprep1[0]);                 /* lea d3(a5|a4),a2 */
        pokewd((UWORD)off);
    }
    pokewd(maxprep2[0]);                        /* .already: move.l a2,d0 */
    regat++;                                    /* setsvars resets it per statement */
    type = T_LONG;
    eval(&type);
    tokejsr(vm ? maxlen + 1 : maxlen);
}

/* ================================================================ Free/Use */

/* blitz2.s:12196 getmaxel, *libp = the lib.  The asm kept the lib in the
   global mymaxlib (12251) for maxjsr; Free/Use now hand it over. */
static UBYTE *getmaxlib(UWORD c, struct blitzlib **libp)
{
    struct blitzlib *lib;
    UBYTE *e;
    UWORD type, opcode;
    WORD shifts;

    if (!(c & 0x8000))
        syntaxerr();
    findtoke(c & 0x7fff, &lib);
    if (LIBMAX_SHIFT(lib) == 0)
        freeerr();
    type = T_WORD;
    eval(&type);
    *libp = lib;
    e = LIBMAX_E(lib);
    if (debugga) {
        /* check maximum overflow */
        pokewd(0xb07c | (regat << 9));         /* cmp #x,dn */
        pokewd(*(UWORD *)(e - 4));
        pokel(tl(maxchk));                      /* bcs */
        tokejsr(maxerr);
    }
    shifts = *(WORD *)(e - 2);                  /* .pen: # shifts */
    for (;;) {                                  /* .loop */
        opcode = regat | lslimm[0];
        if ((UWORD)shifts < 8) {                /* .under */
            pokewd(opcode | (shifts << 9));
            break;
        }
        pokewd(opcode);
        shifts -= 8;
        if (!shifts)
            break;
    }
    return e;
}

/* blitz2.s:12196 getmaxel (for Addr, which does not need the lib) */
UBYTE *getmaxel(UWORD c)
{
    struct blitzlib *lib;

    return getmaxlib(c, &lib);
}

/* blitz2.s:12262 maxjsr - asm: Z flag of the caller's move.l d1 */
static void maxjsr(struct blitzlib *lib, ULONG sub)
{
    if (!sub)
        nosuperr();
    makelibsub(lib, sub);
}

/* blitz2.s:12253 dofree */
void dofree(UWORD c)
{
    struct blitzlib *lib;
    UBYTE *e = getmaxlib(c, &lib);

    free_[1] = *(UWORD *)(e - 6);               /* patched in place */
    pokecode(free_, freef);
    maxjsr(lib, *(ULONG *)(e - 10));
}

/* blitz2.s:12266 douse */
void douse(UWORD c)
{
    struct blitzlib *lib;
    UBYTE *e = getmaxlib(c, &lib);

    use[1] = *(UWORD *)(e - 6);                 /* patched in place */
    usef[-1] = *(UWORD *)(e - 6);
    usef[-1] += 4;
    pokecode(use, usef);
    maxjsr(lib, *(ULONG *)(e - 14));
}

/* ================================================================== Else */

/* blitz2.s:12279 doelse2 */
void doelse2(UWORD c)
{
    (void)c;
    doelse(getchar3());
}

/* blitz2.s:12280 doelse */
void doelse(UWORD c)
{
    struct ifnode *ifn = firstif;

    if (!ifn)
        noiferr();
    if ((ifn->kind & 0xff) == IFK_WHILE)
        illelseerr();
    /* DIFF: the asm poked $6000 in the high word and whatever d1's high
       word was in the low word (a placeholder, patched by EndIf) */
    pokel(0x60000000);
    doendif2(ifn, 0);
    ifn->brapc = pc;
    dothen(c);
}

/* =================================================== conditional compile */

/* blitz2.s:12293 docerr */
void docerr(UWORD c)
{
    getsp(c, namebuff);
    /* asm: pushes namebuff as the faked inline string of `bsr err` */
    err((const char *)namebuff);
}

/* blitz2.s:12299 docnif */
void docnif(UWORD c)
{
    (void)c;
    evalconst(NULL);
    bakup();
    compare(numcom(), 5);                       /* asm comment says beq: index 5 = cifne */
}

/* blitz2.s:12942 collif - collect a comparison thingy.
   *c in: first char, out: next char.  Returns the cifbras index. */
static UWORD collif(UWORD *c)
{
    UWORD c1 = *c, c2, cond;

    if (!ifchar(&c1))
        syntaxerr();
    cond = c1;
    c2 = getchar3();
    *c = c2;
    if (ifchar(&c2)) {
        cond += 3 * (c2 + 1);
        if ((cond & 3) == 3)
            syntaxerr();
        *c = getchar3();
    }
    return cond;
}

/* blitz2.s:12306 docsif */
void docsif(UWORD c)
{
    UWORD cond;

    getsp(c, namebuff);
    c = getchar3();
    cond = collif(&c);
    getsp(c, namebuff2);
    compare(strcom(), cond);
}

/* blitz2.s:12339/12340 cifdont / cifdo */
static void cifdo(WORD flag)
{
    *concomsp++ = comflag;
    comflag = flag;
    if (!eos(getchar3()))
        syntaxerr();
    bakup();
}
static void cifdont(void) { cifdo(0); }

/* blitz2.s:12327-12337 */
void cifeq(int res) { if (res == 0) cifdo(-1); else cifdont(); }
void cifne(int res) { if (res != 0) cifdo(-1); else cifdont(); }
void ciflt(int res) { if (res < 0)  cifdo(-1); else cifdont(); }
void cifle(int res) { if (res <= 0) cifdo(-1); else cifdont(); }
void cifgt(int res) { if (res > 0)  cifdo(-1); else cifdont(); }
void cifge(int res) { if (res >= 0) cifdo(-1); else cifdont(); }

/* blitz2.s:1294 cifbras */
void (*const cifbras[12])(int res) = {
    ciflt, cifeq, cifgt, 0,
    cifle, cifne, cifle, 0,
    cifge, cifne, cifge, 0
};

/* blitz2.s:12315 compare - use subroutine in a0, cc in d1 to compare then
   enable or disable compilation accordingly.
   DIFF: the asm called the compare routine here and jumped to the cifbras
   entry with its CCR; the caller passes the routine's result instead. */
void compare(int res, UWORD cond)
{
    cifbras[cond](res);
}

/* ================================================== Restore / Read / Data */

/* blitz2.s:12351 dorestore - Restore 'label' */
void dorestore(UWORD c)
{
    struct label *lab;

    uselib(64535);
    c = reget();
    if (eos(c)) {
        /* .resbeg: restore to beginning... */
        tokejsr(getdstart);
        pokel(tl(rescode2));
        return;
    }
    lab = makealab(NULL);                       /* get label name! */
    pokewd(rescode[0]);
    addoff();                                   /* add offset */
    pokel(lab->data);
    pokewd(rescode[3]);
    if (lab->refs) {                            /* is it outstanding? */
        /* yes - tell it that reference is a DATA statement one */
        lab->refs->addr += 1;                   /* make odd - flag for data reference. */
    }
    /* .no: data label not outstanding...this not working!?!? */
}

/* blitz2.s:12384 doread */
void doread(UWORD c)
{
    struct var *v;
    struct type *t;
    WORD off;
    UWORD vtype, basetype;
    UWORD *code, *codef;

    uselib(64535);
    for (;;) {                                  /* .loop */
        pokel(tl(dataget));
        c = getvname(c);
        v = fetchvar(VNAMELEN(), &t, &c);
        vtype = calcvar(&v, &t, &c, &off);
        if (vtype & TF_POINTER)
            vtype = (vtype & 0xff00) | T_ADDR;  /* pointer becomes an address! */
        if (!(vtype & 0xff))
            noleterr();                         /* read in a whole struct....later! */
        if (debugga) {
            pokewd(0x7000 | (vtype & 15));      /* moveq #type,d0 */
            tokejsr(datachk);
        }
        basetype = vtype & 0xff;                /* .norerr */
        if (basetype == T_STRING) {             /* .string */
            pokewd(stlenget[0]);                /* move.l (a3)+,-(a7) */
            if (!(vtype & TF_ADDRINA2)) {
                if (varmode) {
                    leaampl[1] = (UWORD)off;
                    pokel(tl(leaampl));
                } else {
                    leaamp[1] = (UWORD)off;     /* .sglobal */
                    pokel(tl(leaamp));
                }
            }
            tokejsr(varmode ? astring - 1 : astring);   /* .ok */
            lasta6 = -1;
            pokecode(stalign, stalignf);
        } else if (basetype == T_BYTE) {
            if (vtype & TF_ADDRINA2) {
                pokecode(dataletb2, dataletb2f);
            } else {                            /* .bimm */
                code = dataletb;
                codef = dataletbf;
                if (varmode) {
                    code = dataletbl;
                    codef = dataletblf;
                }
                code[2] = (UWORD)off;           /* .byteglobal (patched in place) */
                pokecode(code, codef);
            }
        } else {                                /* .notbyte */
            code = dataletw;
            if (basetype >= T_LONG)
                code = dataletl;
            if (vtype & TF_ADDRINA2) {          /* .word */
                pokewd(code[2]);
            } else {                            /* .imm */
                if (varmode)
                    code += 3;
                code[1] = (UWORD)off;           /* .wlglobal (patched in place) */
                pokel(tl(code));
            }
        }
        pokel(tl(dataput));                     /* .next */
        if (c != ',')
            return;
        c = getchar3();                         /* .more */
    }
}

/* blitz2.s:12483 getsize2 - .b/.w/.l -> 0/1/2 (none: 1, bakup) */
static UWORD getsize2(UWORD c)
{
    if (c != '.') {
        bakup();
        return 1;
    }
    c = getchar3();
    if (c == 'w')
        return 1;
    if (c == 'b')
        return 0;
    if (c != 'l')
        syntaxerr();
    return 2;
}

/* blitz2.s:12501 dodcb (= dodcb2) */
void dodcb(UWORD c)
{
    UWORD size;
    ULONG cnt, val;

    size = getsize2(c);
    cnt = evalconst2(&c);
    if (c != ',')
        syntaxerr();
    val = evalconst2(&c);                       /* to put in memory */
    /* BUG(orig) (#37): do-while loops: a count of 0 repeats 2^32 times */
    if (size == 0) {
        do pokebyte((UBYTE)val); while (--cnt);         /* .byte */
        return;
    }
    alignpc();
    if (size == 1) {
        do pokewd((UWORD)val); while (--cnt);           /* .word */
        return;
    }
    do {                                                /* .long */
        pokewd((UWORD)(val >> 16));
        pokewd((UWORD)val);
    } while (--cnt);
}

/* blitz2.s:12532 dods */
void dods(UWORD c)
{
    UWORD size = getsize2(c);
    ULONG n = evalconst2(NULL);

    pc += n << size;
}

/* blitz2.s:12540 insasm2 */
static void insasm2(UWORD mode)
{
    asmlen = asmlen2;
    insasm(&dummyasm, pc, mode);
}

/* blitz2.s:12548 dodc
   DIFF: the asm poked d1 as left by asmconst (copyconst's text count) as a
   placeholder; C pokes asmlen2 (equal to it except for the indirect
   "pointer into the source" form).  The asm resolver overwrites it. */
void dodc(UWORD c)
{
    UWORD size = getsize2(c);

    asmbuff = namebuff;
    if (size == 0) {
        /* Byte! */
        do {                                    /* .bloop */
            c = getchar3();
            if (c == '"') {
                for (;;) {                      /* .qloop */
                    c = getchar3();
                    if (eos(c))
                        syntaxerr();
                    if (c == '"')
                        break;
                    pokebyte((UBYTE)c);
                }
                c = getchar3();                 /* .qdone */
            } else {                            /* .notq */
                bakup();
                c = asmconst();
                pc--;
                insasm2(17);
                pc++;
                pokebyte((UBYTE)asmlen2);
            }
        } while (c == ',');                     /* .bdone */
        return;
    }
    if (size == 1) {
        /* Word */
        alignpc();
        do {                                    /* .wloop */
            c = asmconst();
            insasm2(16);
            pokewd((UWORD)asmlen2);
        } while (c == ',');
        return;
    }
    /* Long! */
    alignpc();
    do {                                        /* .lloop */
        c = asmconst();
        insasm2(11);
        pokel((ULONG)asmlen2);
    } while (c == ',');
}

/* blitz2.s:12600 doeven4 - long align */
void doeven4(UWORD c)
{
    (void)c;
    pc = (pc + 3) & ~3UL;
}

/* blitz2.s:12605 doeven8 - phrase align */
void doeven8(UWORD c)
{
    (void)c;
    pc = (pc + 7) & ~7UL;
}

/* blitz2.s:12611 alignpc */
void alignpc(void)
{
    pc = (pc + 1) & ~1UL;
}

/* blitz2.s:12610 doeven - word align (same address as alignpc) */
void doeven(UWORD c)
{
    (void)c;
    alignpc();
}

/* blitz2.s:12617 pdt - insert type of data item into code if runtime error
   checking is on.....  codes for data..... Dn, when 'D' means data, and
   n = 1 (byte) to 7 (string).  datasize: the asm's global datasize
   (12615), set by dodata, the only caller */
static void pdt(UWORD datasize)
{
    if (debugga)
        pokedata2((UWORD)(('D' << 8) | (datasize & 0xff)));
}

/* blitz2.s:12629 dodata */
void dodata(UWORD c)
{
    UWORD want, type;
    ULONG val, lenat, len;

    uselib(64535);
    bakup();
    want = sizespec2();                         /* get size of it into d1 */
    /* (eg. Data.w) */
    if (want != T_STRING) {
        do {                                    /* .more */
            pdt(want);
            type = want;
            val = fetchdata(&type);
            if (type != want) {
                if (want == T_WORD) {
                    if (type != T_LONG || numval >= 65536)
                        baddaterr();
                } else if (want == T_BYTE) {    /* .notw */
                    if (type != T_WORD || numval >= 256)
                        baddaterr();
                } else {
                    baddaterr();                /* .notb */
                }
            }
            if (want < T_LONG)                  /* .dataok */
                pokedata2((UWORD)val);
            else
                pokedata2l(val);                /* .notword */
            c = reget();                        /* .next */
        } while (c == ',');
        return;
    }
    for (;;) {                                  /* .string */
        pdt(want);
        lenat = data2;
        pokedata2l(0);  /* DIFF: asm poked d0 (garbage) as length placeholder */
        len = 0;
        c = getchar3();                         /* .loop */
        if (!eos(c)) {
            if (c == '"') {
                for (;;) {                      /* .quoted */
                    c = getchar3();
                    if (eos(c))
                        notqerr();
                    if (c == '"')
                        break;
                    pokedata2b((UBYTE)c);
                    len++;
                }
                c = getchar3();                 /* .done2 */
            } else if (c != ',') {
                do {                            /* .loop2 */
                    pokedata2b((UBYTE)c);
                    len++;
                    c = getchar_();
                } while (c && c != ',');
            }
        }
        if (!nomemleft)                         /* .done */
            L_(lenat) = len;
        data2 = (data2 + 1) & ~1UL;             /* .nopoke */
        if (c != ',')
            return;
    }
}

/* ================================================================= Shared */

/* blitz2.s:12716 doshared - set up SHARED variables..... */
void doshared(UWORD c)
{
    WORD pm = procmode;
    struct node *last;
    struct var *g, *v;
    struct type *t;
    UWORD len;

    if (!pm)
        sharederr();
    procmode = 0;
    for (;;) {                                  /* .loop2 */
        c = getvname(c);
        len = VNAMELEN();
        g = findvar((struct node *)&firstglob, len, &last);
        while (g) {                             /* .try2 */
            if (((g->flags ^ flagmask) & 0xff) == 0)
                dupsherr();
            g = (struct var *)findlab((struct node *)g, NAMEOFF_VAR, len, &last);
        }
        g = (struct var *)addhere(last, NAMEOFF_VAR, len);   /* .notfound */
        c = reget();                            /* asm: d0 from addhere's reget */
        v = fetchvar(len, &t, &c);
        memcpy(&g->offset, &v->offset, 10);     /* 5 words from +4 */
        if (flagmask & 2) {
            c = getchar3();
            if (c != ')')
                syntaxerr();
            c = getchar3();
        }
        if (c != ',')                           /* .done */
            break;
        c = getchar3();
    }
    procmode = pm;                              /* .done2 */
}

/* ============================================================== Includes */

/* blitz2.s:12760 doxinclude - do EXCLUSIVE (once only!) include */
void doxinclude(UWORD c)
{
    struct node *last;
    UWORD len;

    (void)c;
    len = makeiname();
    if (findxinc(len, &last))
        return;                                 /* .done */
    addhere(last, NAMEOFF_XINC, len);
    doinc2(len);
}

/* blitz2.s:12772 doincbin */
void doincbin(UWORD c)
{
    BPTR lk, fh;
    struct FileInfoBlock *fib;
    ULONG size, end;
    LONG r;

    (void)c;
    makeiname();
    lk = Lock((CONST_STRPTR)namebuff, ACCESS_READ);
    if (!lk)
        noincerr();
    Examine(lk, (struct FileInfoBlock *)macrobuff);
    fib = (struct FileInfoBlock *)macrobuff;
    if (fib->fib_DirEntryType >= 0)
        noincerr();                             /* BUG(orig) (#38): lock not released */
    size = fib->fib_Size;                       /* length */
    UnLock(lk);
    fh = Open((CONST_STRPTR)namebuff, MODE_OLDFILE);
    if (!fh)
        noincerr();
    end = pc + size;
    if (end > libat) {                          /* .over */
        nomemleft = -1;
        pc = end;
    } else {
        r = Read(fh, (APTR)pc, size);
        if (r == -1)
            readerr();                          /* BUG(orig) (#38): fh not closed */
        pc += r;
    }
    Close(fh);                                  /* .close */
}

/* blitz2.s:12811 doinclude - do a standard include */
void doinclude(UWORD c)
{
    (void)c;
    if (dirmode)
        illdirerr();
    doinc2(makeiname());
}

/* blitz2.s:12818 doinc2 */
void doinc2(UWORD namelen)
{
    struct include *n;

    if (findinc(namelen, NULL))
        alincerr();
    n = (struct include *)addhere2((struct node *)&firstinc, NAMEOFF_INC, namelen);
    n->nbuf = 0;
    n->buf = NULL;
    n->buf = doallocmem(INCLEN, MEMF_PUBLIC);
    n->ptr = n->buf;
    srcptr = n->buf;
    n->fh = Open((CONST_STRPTR)namebuff, MODE_OLDFILE);
    if (!n->fh)
        noincerr();
    numincs++;
    /* DIFF: asm `move.l passstack,a7 / jmp passnxt` */
    longjmp(*passjmp, 1);
}

/* ================================================================= Macro */

/* text being gathered by domacro, handed to macputb/macputw */
struct macbuild {
    UBYTE *at;              /* asm a0: write position in macrobuff */
    WORD got;               /* asm d2: 0 = nothing got yet */
};

/* blitz2.s:12936 macputb */
static void macputb(struct macbuild *mb, UWORD c)
{
    mb->got = -1;
    *mb->at++ = (UBYTE)c;
    if (mb->at >= macrobufff)
        macbigerr();
}

/* blitz2.s:12933 macputw - high byte, then falls into macputb */
static void macputw(struct macbuild *mb, UWORD c)
{
    *mb->at++ = (UBYTE)(c >> 8);
    macputb(mb, c);
}

/* blitz2.s:12849 domacro */
void domacro(UWORD c)
{
    struct node *last;
    struct macro *m;
    struct macbuild mb;
    WORD coolnest;              /* blitz2.s:12845 coolnest dc 0 */
    UWORD len;
    ULONG n;

    (void)c;
    macline = linenumat;
    len = makename2(NULL);
    if (!len)
        syntaxerr();
    bakup();
    if (findmac(len, &last))
        dupmacerr();
    m = (struct macro *)addhere(last, NAMEOFF_MACRO, len);
    m->usecnt = 0;
    mb.at = macrobuff;
    mb.got = 0;                                 /* flag for nothing got yet! */
    coolnest = 0;
    for (;;) {                                  /* .gather */
        c = getcharb();         /* get next character but not conditional stuff */
        if (c == 0) {
            if (!nextline())
                macenderr();
            if (!mb.got)
                continue;
            macputb(&mb, ':');                  /* .notend2 (58) */
            continue;
        }
        if (!(c & 0x8000)) {
            macputb(&mb, c);                    /* .notend2 */
            continue;
        }
        if (c == 0x800d) {                      /* Macro */
            coolnest++;
        } else if (c == 0x8002) {               /* End */
            c = getchar3();
            if (c == 0x800d && --coolnest < 0)
                break;                          /* .macdone */
            /* .notem.  A plain char after End is put as a word
               ($00,char; the 0 ends the statement), spaces/comments after End are lost */
            macputw(&mb, 0x8002);
        }
        macputw(&mb, c);                        /* .notend */
    }
    /* .macdone: DIFF: asm `addq #4,a7` dropped the return into doline */
    dolineabort = 1;
    n = mb.at - macrobuff;
    /* clean up leading and trailing ':'s (only trailing ones in fact) */
    while (n) {                                 /* .mclean */
        if (*--mb.at != ':')
            break;
        if (n != 1 && (BYTE)mb.at[-1] < 0)
            break;                              /* ':' is a token's low byte */
        n--;                                    /* .cdo */
    }
    m->textlen = (UWORD)n;                      /* .cdone */
    if (m->textlen) {   /* text left unset when empty (never read then) */
        m->text = doallocmem(n, MEMF_PUBLIC);
        memcpy(m->text, macrobuff, m->textlen);
    }
    getchar3();                                 /* .skip2 */
}

/* ============================================================ If / While */

/* blitz2.s:12971 dounless - not referenced in the asm (not in atokens) */
__attribute__((unused)) static void dounless(UWORD c)
{
    (void)c;
    doif2(0, -1);
}

/* blitz2.s:12974 dowhile */
void dowhile(UWORD c)
{
    (void)c;
    doif2(IFK_WHILE, 0);                        /* while flag, negate flag */
}

/* blitz2.s:13272 doif / 13273 doifm */
void doif(UWORD c)
{
    (void)c;
    doif2(0, 0);
}

/* blitz2.s:13275 doif2 - the main IF bit */
void doif2(UWORD kind, WORD negate)
{
    struct ifnode *n;
    ULONG start = pc;
    UWORD type, c;

    iflineat = linenumat;
    type = 0;
    bakevalu(&type);                            /* Get True/False Expression! */
    if (type == T_FLOAT) {                      /* .float */
        tokejsr(getffpbase);
        pokel(tl(thetst5));
    } else if (type == T_BYTE) {
        pokewd(thetst1[0]);
    } else if (type == T_WORD) {
        pokewd(thetst2[0]);
    } else if (type == T_STRING) {
        pokewd(thetst4[0]);
    } else {
        pokewd(thetst3[0]);
    }
    pokel(negate ? tl(negbr) : tl(posbr));      /* .gotst2 */
    n = doallocmem(sizeof(struct ifnode), MEMF_PUBLIC);
    n->next = firstif;
    firstif = n;
    n->startpc = start;
    n->brapc = pc;
    n->kind = kind;
    c = reget();
    if (eos(c))
        return;                                 /* ifdone */
    n->kind |= IFK_ONELINE;                     /* one liner */
    dothen(c);                                  /* falls through into dothen */
}

/* ===================================================== Repeat lists etc. */

/* blitz2.s:12989 freelist - a2=lea of list, d2=size of item, d3=offset to
   sub list, d4=size of sublist item.  if d3=0 then single list free only. */
void freelist(APTR head, ULONG size, UWORD suboff, ULONG subsize)
{
    struct node **h = head;
    struct node *n = *h, *nx;

    *h = NULL;
    while (n) {
        if (suboff)
            freeslist((UBYTE *)n + suboff, subsize);
        nx = n->next;
        FreeMem(n, size);
        n = nx;
    }
}

/* blitz2.s:12985 freeslist - free a simple list. a2=lea of list, d2=size */
void freeslist(APTR head, ULONG size)
{
    freelist(head, size, 0, 0);
}

/* blitz2.s:12978 freereps */
void freereps(void)
{
    freeslist(&firstrep, sizeof(struct repnode));
}

/* blitz2.s:12982 freesels */
void freesels(void)
{
    freeslist(&firstsel, sizeof(struct selnode));
}

/* blitz2.s:13266 unlinkrep */
static void unlinkrep(void)
{
    struct repnode *r = firstrep;

    firstrep = r->next;
    FreeMem(r, sizeof(struct repnode));
}

/* blitz2.s:13228 doforever */
void doforever(UWORD c)
{
    struct repnode *r = firstrep;
    LONG d;

    (void)c;
    if (!r)
        unterr1();
    pokewd(0x6000);                             /* BRA */
    d = r->pc - pc;                             /* dest pc */
    if ((WORD)d >= 0)
        unterr2();
    pokewd((UWORD)d);
    unlinkrep();
}

/* blitz2.s:13241 dountil */
void dountil(UWORD c)
{
    struct ifnode *n;
    ULONG at;
    LONG disp;

    if (!firstrep)
        unterr1();                              /* no repeat! */
    doif(c);                                    /* do the until */
    if (!eos(reget()))
        syntaxerr();
    if (!nomemleft) {
        at = pc - 2;
        disp = firstrep->pc - at;               /* dest pc. */
        if ((WORD)disp >= 0)
            unterr2();
        W_(at) = (UWORD)disp;                   /* put in bra dest. */
    }
    n = firstif;                                /* .skip */
    firstif = n->next;
    FreeMem(n, sizeof(struct ifnode));
    unlinkrep();
}

/* ================================================================ Select */

/* blitz2.s:13086 fillbne - a2=sel */
static void fillbne(struct selnode *s)
{
    LONG d;

    if (!s->bnepc)
        return;                                 /* none to do! */
    d = pc - s->bnepc;
    if ((ULONG)d > 32767)
        cbraerr();
    if (!nomemleft)
        W_(s->bnepc) = (UWORD)d;
}

/* blitz2.s:13099 fillbra - make a bra if necessary */
static void fillbra(struct selnode *s)
{
    struct addrnode *b;

    if (!s->bnepc)
        return;
    /* OK, we're after a case thingy. add a BRA */
    pokewd(0x6000);                             /* BRA */
    b = doallocmem(sizeof(struct addrnode), MEMF_PUBLIC);
    b->next = s->bras;
    s->bras = b;
    b->addr = pc;
    pokewd(0);  /* DIFF: asm poked d1 as left by doallocmem (placeholder) */
}

/* blitz2.s:13121 casechk */
static void casechk(UWORD type)
{
    if (debugga)
        tokejsr(type < T_LONG ? casechkw : casechkl);
}

/* blitz2.s:13032 doendsel - END SELECT */
void doendsel(UWORD c)
{
    struct selnode *s = firstsel;
    struct addrnode *b;
    LONG d;

    (void)c;
    if (!s)
        eselerr();
    fillbne(s);
    /* fill in all bras at end of cases! */
    while ((b = s->bras) != NULL) {             /* .loop */
        d = pc - b->addr;
        if ((ULONG)d > 32767)
            cbraerr();
        if (!nomemleft)
            W_(b->addr) = (UWORD)d;
        s->bras = b->next;                      /* free it up! */
        FreeMem(b, sizeof(struct addrnode));
    }
    popselect(s);                               /* fix stack on select */
    s = firstsel;
    firstsel = s->next;
    FreeMem(s, sizeof(struct selnode));
    getchar3();
}

/* blitz2.s:13077 dodefault */
void dodefault(UWORD c)
{
    struct selnode *s = firstsel;

    (void)c;
    if (!s)
        defaerr();
    fillbra(s);
    fillbne(s);
    s->bnepc = 0;
    casechk(s->type);
}

/* blitz2.s:13130 docase - eg : CASE a+10 */
void docase(UWORD c)
{
    struct selnode *s = firstsel;
    UWORD t, type;

    (void)c;
    /* first, fill in last bra (if there is one!) */
    if (!s)
        caseerr();
    fillbra(s);
    fillbne(s);
    t = s->type;                                /* type to get! */
    casechk(t);
    type = t;
    bakeval(&type);
    /* types to compare: the select's type (asm: popped d2) */
    if (t == T_BYTE)
        pokecode(selcomb, selcombf);
    else if (t == T_WORD)
        pokecode(selcomw, selcombf);
    else if (t < T_STRING)
        pokecode(selcoml, selcomlf);
    else
        tokejsr(casestrcomp);                   /* Strings.... */
    pokewd(selbne[0]);                          /* .doneit */
    firstsel->bnepc = pc;
    pokewd(selbne[0]);                          /* disp placeholder */
}

/* blitz2.s:13175 doselect - OK, as in : SELECT opt */
void doselect(UWORD c)
{
    struct selnode *s;
    UWORD type;

    (void)c;
    if (debugga)
        pokel(tl(pcchk));                       /* move #"sE",-(a7) */
    sellineat = linenumat;
    pushstart();                                /* we may have to CLR -(a7)! */
    type = 0;
    bakevalu(&type);                            /* Evaluate */
    pushdo();
    if (type == T_STRING)
        pokewd(clrst[0]);
    pushput();                                  /* .nos */
    if (type == T_BYTE) {
        pokecode(selpushb, selpushbf);
    } else if (type == T_WORD) {
        pokecode(selpushw, selpushbf);
    } else if (type < T_STRING) {
        pokecode(selpushl, selpushlf);
    } else {
        pokewd(moved0a3[0]);                    /* d0 to a3! */
        pokecode(selpushs, selpushsf);
        tokejsr(astring);
    }
    s = doallocmem(sizeof(struct selnode), MEMF_PUBLIC);    /* .doneit */
    s->next = firstsel;
    firstsel = s;
    s->bras = NULL;
    s->bnepc = 0;                               /* default mode! */
    s->type = type;
}
