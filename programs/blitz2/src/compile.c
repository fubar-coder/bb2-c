/*
 * compile.c - chunk 02, blitz2.s 1912-3290: main compiler functions.
 *
 * compile / dopass driver, cleanup, nextline + racing stripe, macro
 * expansion, STOP requester and its exec exception handler, end of pass
 * checks, debugger trap records, statement dispatch (dostatement,
 * handle*), library statement calls (handlelib, dolibtoke, sizespec).
 * See notes/survey-02.md and compile.h for the control-flow contracts
 * (passjmp, dolineabort).
 */
#include "blitz2.h"
#include <string.h>
#include <exec/tasks.h>
#include <exec/ports.h>
#include <proto/exec.h>

/* the asm reads `debugga` with move.b/tst.b (= the high byte, set by sne) */
#define DEBUGON ((debugga & 0xff00) != 0)

#define ELSETOK (0x8000 + 34)

/* ================================================================ data */

/* blitz2.s:1913 funcs - marker label, no code */

UWORD smallpass = 0;            /* blitz2.s:1915 */
UWORD bmapmul = 0;              /* blitz2.s:2032 */
UWORD botline = 0;              /* blitz2.s:2033 offset to bottom of stripe */
UWORD riteend = 0;              /* blitz2.s:2034 right end of include strip */
WORD  macmask = 0;              /* blitz2.s:2036 */

jmp_buf *passjmp = NULL;        /* asm passstack/passnxt, see compile.h */
WORD dolineabort = 0;           /* asm addq #4,a7 into doline, see compile.h */

/* ---- blitz2.s:2394-2447 STOP requester (Intuition structures) */

static WORD stopreqla1[6]  = { 199, 4, 8, 4, 8, 23 };      /* 2407 */
static WORD stopreqla3[6]  = { 199, 5, 199, 23, 9, 23 };   /* 2411 */
static WORD stopreqla4[6]  = { 207, 0, 0, 0, 0, 43 };      /* 2415 */
static WORD stopreqla6[6]  = { 207, 1, 207, 43, 1, 43 };   /* 2419 */
static WORD stopreqla8[6]  = { 63, 0, 0, 0, 0, 11 };       /* 2423 */
static WORD stopreqla10[6] = { 63, 1, 63, 11, 1, 11 };     /* 2427 */

static struct Border stopreqla5 = { 0, 0, 2, 1, 1, 3, stopreqla6, NULL };          /* 2416 */
static struct Border stopreqbo2 = { 0, 0, 1, 2, 1, 3, stopreqla4, &stopreqla5 };   /* 2412 */
static struct Border stopreqla2 = { 0, 0, 2, 1, 1, 3, stopreqla3, &stopreqbo2 };   /* 2408 */
static struct Border stopreqbo1 = { 0, 0, 1, 2, 1, 3, stopreqla1, &stopreqla2 };   /* 2404 */
static struct Border stopreqla9 = { 0, 0, 2, 1, 1, 3, stopreqla10, NULL };         /* 2424 */
static struct Border stopreqbo3 = { 0, 0, 1, 2, 1, 3, stopreqla8, &stopreqla9 };   /* 2420 */

/* 2433 stopreqla7 '***** ' / 2434 reqmessage 'COMPILING *****',0 */
char stopreqla7[] = "***** COMPILING *****";
/* 2440 ' STOP! ',0 */
char stopreqla11[] = " STOP! ";

struct IntuiText stopreqin1 = {                 /* blitz2.s:2429 */
    1, 0, 1, 20, 10, NULL, (UBYTE *)stopreqla7, NULL
};
struct IntuiText stopreqin2 = {                 /* blitz2.s:2436 */
    1, 0, 1, 4, 2, NULL, (UBYTE *)stopreqla11, NULL
};

struct Gadget stopreqga1 = {                    /* blitz2.s:2443 */
    NULL, 72, 28, 64, 12,
    0,                  /* Flags */
    1,                  /* Activation: RELVERIFY */
    1,                  /* GadgetType: BOOLGADGET */
    &stopreqbo3, NULL, &stopreqin2, 0, NULL,
    1,                  /* GadgetID */
    NULL
};

struct Requester stopreq = {                    /* blitz2.s:2394 */
    .OlderRequest = NULL,
    .LeftEdge = 172, .TopEdge = 38, .Width = 208, .Height = 44,
    .RelLeft = 0, .RelTop = 0,
    .ReqGadget = &stopreqga1,
    .ReqBorder = &stopreqbo1,
    .ReqText = &stopreqin1,
    .Flags = 0, .BackFill = 0,
};

WORD stopbit = 0;               /* blitz2.s:2449 (asm writes the low byte) */
volatile WORD stopit = 0;       /* blitz2.s:2450 */
WORD clrreq = 0;                /* blitz2.s:2451 */

UWORD intsused = 0;             /* blitz2.s:2480 */

ULONG varsizeat = 0;            /* blitz2.s:2517 */
UWORD debugga = 0;              /* blitz2.s:2518 debugger enabled? */

/* blitz2.s:2520/2523: two strings each, for openstopreq */
char compiling[] = "COMPILING\0 STOP! ";
char executing[] = "EXECUTING\0!PANIC!";

/* blitz2.s:2683 eopcode: moveq #3,d0 / trap #0 (template, read as a long) */
UWORD eopcode[] = {
    0x7003,                     /* +0 moveq #3,d0 */
    0x4E40,                     /* +2 trap #0 */
};

ULONG endjmpat = 0;             /* blitz2.s:2686 */

ULONG d_pc;                     /* blitz2.s:2861 */
ULONG d_bigpc;                  /* blitz2.s:2862 */
UWORD d_nomemleft;              /* blitz2.s:2863 */
ULONG cont_pc;                  /* blitz2.s:2864 */
ULONG cont_pc2;                 /* blitz2.s:2865 */
ULONG lastcontlink;             /* blitz2.s:2866 */

/* ------------------------------------------------- dispatch tables */

/* `dc.l syntaxerr` entries of atokens: the handler gets a char argument */
static void syntaxerr_t(UWORD c)
{
    (void)c;
    syntaxerr();
}

/* blitz2.s:1298 atokens (index token-1), 92 entries up to atokensend */
tokhandler *const atokens[NATOKENS] = {
    donewtype, doend, dolet, dodim, dogoto,                 /* 1 */
    dogosub, doreturn,                                      /* 6 */
    dostate, dofunc, domwait, doif, dowhile,                /* 8 */
    domacro, doselect, docase,                              /* 13 */
    dodefault, eos_,                                        /* 16 */
    doxinclude, doinclude, syntaxerr_t,                     /* 18 */
    doshared, dofor, donext, syntaxerr_t, syntaxerr_t,      /* 21 */
    dodata, doread,                                         /* 26 */
    /* the folowing should all be intercepted by getchar */
    syntaxerr_t, syntaxerr_t, syntaxerr_t, syntaxerr_t,     /* 28 */
    docerr, dothen, doelse, doeven4, doeven8,               /* 32 */
    dodc, dods, doeven, dodcb, dosetreg, doincbin,          /* 37 */
    dofree, douse, dostop, docont, syntaxerr_t,             /* 43 */
    dosetint, doclrint, domaxlen, dodeftype,                /* 48 */
    doblitz, doamiga, doqamiga, dovwait, dolibjsr,          /* 52 */
    dotokejsr, doblibjsr, syntaxerr_t,                      /* 57 */
    /* A break goes in here for the assembler stuff! */
    syntaxerr_t,        /* flash2 ; for list */              /* 60 */
    doseterr, doclrerr, syntaxerr_t, doerrfail,             /* 61 */
    syntaxerr_t,                                            /* 65 */
    syntaxerr_t,        /* Mod */                           /* 66 */
    syntaxerr_t,        /* Pi */                            /* 67 */
    dorepeat,                                               /* 68 */
    dountil,                                                /* 69 */
    doputreg,                                               /* 70 */
    dopop,                                                  /* 71 */
    doincdir,                                               /* 72 */
    mydoendif2,                                             /* 73 */
    dowend,                                                 /* 74 */
    dosysjsr,                                               /* 75 */
    dowbstartup,                                            /* 76 */
    syntaxerr_t,                                            /* 77 */
    doongo,                                                 /* 78 */
    syntaxerr_t,                                            /* 79 */
    doforever,                                              /* 80 */
    dorestore,                                              /* 81 */
    doexchange,                                             /* 82 */
    dousepath,                                              /* 83 */
    docloseed,                                              /* 84 */
    donocli,                                                /* 85 */
    syntaxerr_t, syntaxerr_t, syntaxerr_t, syntaxerr_t, syntaxerr_t, /* 86 */
    dorunerrson,                                            /* 91 */
    dorunerrsoff,                                           /* 92 */
};

/* blitz2.s:14831 modetable */
tokhandler *const modetable[] = {
    typemode,
};

/* ===================================================== compile driver */

/* blitz2.s:1917 compile - O.K. Here we go..... */
void compile(void)
{
    jmp_buf jb, *olderr = errjmp;

    /* asm: errstack = a7, errcont = freeemup */
    errjmp = &jb;
    if (setjmp(jb)) {
        /* asm: freeemup entered with a7 = errstack, its rts returns from
           compile */
        errjmp = olderr;        /* DIFF: asm never restored errstack */
        passjmp = NULL;
        freeemup();
        return;
    }
    concomsp = concomstack;
    /* racing stripe stuff... */
    {
        UWORD w = W_(CD_L(CD_MYWINDOW2) + 8);
        ULONG d0;
        UWORD q;

        riteend = (UWORD)((w >> 3) - 7);
        d0 = (ULONG)riteend << 16;
        /* divu numlines,d0.  DIFF: numlines==0 would trap in the asm.
           On overflow (quotient > $ffff) divu leaves d0 unchanged, whose
           low word is 0 -> -1 below, as in the asm. */
        if (numlines == 0 || d0 / numlines > 0xffff)
            q = 0;
        else
            q = (UWORD)(d0 / numlines);
        if (q == 0)
            q = 0xffff;
        titleadd[0] = (WORD)q;
    }
    bmapmul = CD_W(CD_BMAPMUL);
    botline = (UWORD)(bmapmul * 9);     /* offset to bottom of racing stripe */

    /* sne debugga: only the high byte is written */
    debugga = (debugga & 0x00ff) |
              ((optreqga10.Flags & GFLG_SELECTED) ? 0xff00 : 0);

    dirmode = 0;                /* direct mode flag */
    ezerr = 0;                  /* quiet error */
    thisproc = NULL;
    connest = 0;
    varmode = 0;
    procmode = 0;
    numincs = 0;
    intstring = 0;
    smallpass = 0;

l_retry:
    dopass();
    if (!(optreqga15.Flags & GFLG_SELECTED))
        goto l_nosmall;
l_makesmall:
    if (!alloxadj())
        goto l_anerr;
    smallpass++;
    dopass();
l_nosmall:
    if (nomemleft) {
        ed_openreq(&nomemreq);  /* open the requester */
        for (;;) {
            WORD d6 = 0;
            if (!ed_getinput(&d6))      /* getinput: d7 != -1 */
                continue;
            if (d6 == 1) {
                /* .inc */
                if (!alloxinc()) {
                    freeemup();         /* asm: beq freeemup (tail) */
                    errjmp = olderr;
                    return;
                }
                goto l_retry;
            }
            if (d6 == 2)
                goto l_makesmall;
            if (d6 == 3)
                goto l_anerr;
        }
    }
    /* .dosave: used to go after beq .nosave */
    qfree();
    errjmp = olderr;
    return;

l_anerr:
    anyerrs = -1;
    qfree();
    errjmp = olderr;
}

/* blitz2.s:1998 qfree - free up un-necessary stuff */
void qfree(void)
{
    freereps();
    freesels();
    freetlist();
    freepends();
    freeifs();
    freeincs();
    freexincs();
    freefors();
    freeasms();
}

/* blitz2.s:2010 freeemup */
void freeemup(void)
{
    qfree();
    killoffs();         /* free up DOS offset table */
    frallvars();        /* free up Global vars */
    fralltypes();       /* free up all types */
    freelabels();       /* free up labels */
    freemacs();         /* free up actual macro defines. */
    freeprocs();        /* free up states/funcs */
    freeprocvs();       /* free up outstanding proc stuff */
}

/* blitz2.s:2019 fralltypes */
void fralltypes(void)
{
    struct type *a0 = lastrestype;
    struct type *a2 = a0->next;

    a0->next = NULL;
    freetypes(a2);
}

/* blitz2.s:2024 frallvars */
void frallvars(void)
{
    struct var *a3 = firstvar;

    firstvar = NULL;
    freevars(a3);
}

/* blitz2.s:2028 freeifs */
void freeifs(void)
{
    freeslist(&firstif, 14);
}

/* ===================================================== per line */

/* blitz2.s:2038 nextline - go on to the next line of the program..... */
BOOL nextline(void)
{
    macmask = 0;
    freembuff();
    quoteflag = 0;
    if (numincs && readinc()) {
        /* do include stripe */
        UWORD d0 = (UWORD)numincs;
        if (d0 < 4) {
            UWORD d2 = (UWORD)titleadd[d0];     /* offset in line */
            if (d2 < riteend) {
                UBYTE *a1;
                WORD row;

                titleadd[d0]++;
                row = (WORD)(d0 * bmapmul);
                a1 = (UBYTE *)CD_L(CD_COMBITMAP) + row;
                a1[d2] ^= 0xff;
                a1 = (UBYTE *)CD_L(CD_COMBITMAP) + (WORD)botline - row;
                a1[d2] ^= 0xff;
            }
        }
        return TRUE;            /* asm: moveq #-1,d0 */
    }
    /* .skip3: do main stripe */
    {
        ULONG d1 = titleat + (UWORD)titleadd[0];
        WORD hi = (WORD)(d1 >> 16);

        if ((UWORD)hi != (UWORD)(titleat >> 16)) {
            UBYTE *a0 = (UBYTE *)CD_L(CD_COMBITMAP);
            a0[hi - 1] ^= 0xff;
            a0 += (WORD)botline;
            a0[hi - 1] ^= 0xff;
        }
        titleat = d1;
    }
    linenumat++;
    linedone = 0;
    if (--linesleft == 0)
        return FALSE;
    thisitem = thisitem->next;
    srcptr = (UBYTE *)thisitem + 9;     /* lea 9(a0),a5 */
    /* review: asm returns the Z flag of `move.l a0,thisitem` */
    return thisitem != NULL;
}

/* blitz2.s:2105 hexascii - convert d1.w to ascii at a0 */
void hexascii(UWORD d1, UBYTE *a0)
{
    int d0;

    for (d0 = 3; d0 >= 0; d0--) {
        UWORD d2 = (d1 & 15) + 48;
        if (d2 >= 58)
            d2 += 7;
        a0[d0] = (UBYTE)d2;
        d1 >>= 4;
    }
}

/* ===================================================== macros */

/* blitz2.s:2121 expandmax - go through statement and expand macros.
   line at a5 (srcptr), check it for !'s */
void expandmax(void)
{
    if (!comflag)
        return;
    for (;;) {
        /* .loop0 */
        UBYTE *start = srcptr;  /* asm: move.l a5,-(a7) */
        UBYTE *p = srcptr;
        UBYTE ch;
        UBYTE *a3, *a4, *a1, *a0, *end, *a2;
        struct macro *m;
        WORD d4;
        UWORD d1;

        for (;;) {
            ch = *p++;
            if (!ch)
                goto l_done;
            if (ch & 0x80) {    /* token: skip 2nd byte */
                p++;
                continue;
            }
            if (ch == '"') {
                for (;;) {
                    ch = *p++;
                    if (!ch)
                        notqerr();
                    if (ch & 0x80) {
                        p++;
                        continue;
                    }
                    if (ch == '"')
                        break;
                }
                continue;
            }
            if (ch == ':' || ch == ';')
                goto l_done;
            if (ch == '!')      /* macro character! */
                break;
        }

        /* expand the macro! */
        macmask = -1;
        srcptr = p;
        a3 = p;
        {
            UWORD len = makename(NULL);
            if (!len)
                syntaxerr();
            bakup();
            m = findmac(len, NULL);
            if (!m)
                nomacerr();
        }

        a4 = srcptr;            /* start of parameters! */
        d4 = 0;                 /* narg=0 */
        if (*a4++ == '{') {
            struct macparam *mp = (struct macparam *)macrobuff;
            UWORD t;
            /* BUG(orig): no check against the end of macrobuff */
            do {
                d4++;           /* another p */
                mp->start = a4;
                t = findit(&a4);
                mp->len = (UWORD)(a4 - mp->start - 1);
                mp++;
            } while (t != '}');
            srcptr = a4;
        }
        numarg = d4;

        myoline = myline;
        myline = doallocmem(macrolen, MEMF_PUBLIC);
        a1 = myline;
        end = a1 + macrolen;    /* end of macro buffer (asm a6) */

        a0 = start;             /* start of the line */
        do {
            if (a1 >= end)
                mbovererr();
            *a1++ = *a0++;
        } while (a0 < a3);
        a1--;                   /* back over '!' */

        d1 = m->textlen;        /* mac len */
        if (d1) {
            macnum = (WORD)m->usecnt;
            m->usecnt++;
            a2 = m->text;
            d1--;
            do {
                /* .loop3 */
                UWORD d0 = *a2++;
                ULONG d0hi = 0;         /* upper word of asm d0 (moveq #0) */
                if (d0 != '`') {
                    /* .oky */
                    if (a1 >= end)
                        mbovererr();
                    *a1++ = (UBYTE)d0;
                    if (d0 & 0x80) {
                        if (a1 >= end)
                            mbovererr();
                        *a1++ = *a2++;
                        d1--;
                    }
                    continue;
                }
                d1--;
                d0 = *a2++;
                if (d0 == '{') {
                    /* .evalit */
                    UBYTE *savea5 = srcptr;
                    UWORD c;
                    ULONG v;

                    srcptr = a2;
                    v = evalconst2(&c);
                    if (c != '}')
                        syntaxerr();
                    d1 -= (UWORD)(srcptr - a2);
                    a2 = srcptr;
                    quoteflag = 0;
                    srcptr = savea5;
                    if ((LONG)v < 0)
                        ilconsterr();
                    d0 = (UWORD)v;
                    d0hi = v & 0xffff0000UL;    /* move.l d3,d0 */
                    goto l_itsgot;
                }
                if (d0 == '@') {
                    /* macro num parameter.  BUG(orig): no buffer
                       overflow check for the digits */
                    a1 = (UBYTE *)makelong((char *)a1, (UWORD)macnum);
                    continue;
                }
                /* .notnum */
                d0 |= 32;
                if (d0 >= 'z')
                    ilconsterr();
                d0 -= 48;
                if (d0 >= 10) {
                    d0 -= 39;
                    if (d0 < 10)
                        ilconsterr();
                }
            l_itsgot:
                d0--;
                if ((WORD)d0 < 0) {
                    /* here, we do a numarg text replace */
                    /* move numarg,d0 / divu #10,d0: only the low word is
                       replaced, so after `{expr} the upper word of the
                       value is still there (review: kept as in the asm);
                       on divu overflow d0 stays unchanged */
                    ULONG q = d0hi | (UWORD)numarg;
                    if (q / 10 <= 0xffff)
                        q = ((q % 10) << 16) | (q / 10);
                    if (a1 >= end)
                        mbovererr();
                    *a1++ = (UBYTE)(q + 48);
                    if (a1 >= end)
                        mbovererr();
                    *a1++ = (UBYTE)((q >> 16) + 48);
                    continue;
                }
                /* .ok: ok to do parameter replacement,
                   d0 = number for parameter replacement! */
                if (d0 < (UWORD)numarg) {
                    struct macparam *mp = (struct macparam *)macrobuff + d0;
                    UWORD n = mp->len;  /* len of replacetext */
                    if (n) {
                        UBYTE *s = mp->start;
                        do {
                            if (a1 >= end)
                                mbovererr();
                            *a1++ = *s++;
                        } while (--n);
                    }
                }
            } while (d1-- != 0);        /* dbf d1,.loop3 */
        }

        /* .loop4: rest of the line */
        do {
            if (a1 >= end)
                mbovererr();
        } while ((*a1++ = *srcptr++) != 0);

        srcptr = myline;
        if (myoline)
            FreeMem(myoline, macrolen);
        /* bra .loop0 */
        continue;

    l_done:
        srcptr = start;         /* move.l (a7)+,a5 */
        return;
    }
}

/* blitz2.s:2342 freembuff */
void freembuff(void)
{
    UBYTE *d0 = myline;

    if (d0) {
        myline = NULL;
        FreeMem(d0, macrolen);
    }
}

/* blitz2.s:2351 findit - find the end of a parameter */
UWORD findit(UBYTE **pa4)
{
    UBYTE *a4 = *pa4;
    WORD d2 = 0;                /* no nesting */
    UWORD d0;

    for (;;) {
        d0 = *a4++;
        if (!d0)
            syntaxerr();
        if (d0 & 0x80) {
            a4++;
            continue;
        }
        if (d0 == '"') {
            quoteflag = ~quoteflag;
            continue;
        }
        if (quoteflag)
            continue;
        if (d0 == ',') {
            if (d2 == 0)
                break;
            continue;
        }
        if (isitopen(d0)) {
            d2++;
            continue;
        }
        if (d0 == '}' && d2 == 0)
            break;
        if (isitclose(d0))
            d2--;
    }
    *pa4 = a4;
    return d0;
}

/* blitz2.s:2384 isitopen */
BOOL isitopen(UWORD c)
{
    return (UBYTE)c == '{' || (UBYTE)c == '(';
}

/* blitz2.s:2389 isitclose */
BOOL isitclose(UWORD c)
{
    return (UBYTE)c == '}' || (UBYTE)c == ')';
}

/* ===================================================== STOP requester */

/* blitz2.s:2453 stopcode - exec exception handler (tc_ExceptCode), via
   stopcode_stub: in d0 = signals, out d0 unchanged */
void stopcode_c(struct m68kregs *r)
{
    ULONG d0 = r->d[0];

    if (stopit)
        return;
    if (!(d0 & (1UL << (stopbit & 31))))       /* btst d1,d0 */
        return;
    stopit = -1;
    Signal(us, panicmask);
}

/* blitz2.s:2469 endstop */
void endstop(void)
{
    if (!clrreq)
        return;
    ed_closereq();              /* turn off the requester */
    clrreq = ~clrreq;
    us->tc_SigExcept = 0;
    us->tc_ExceptCode = NULL;
}

/* blitz2.s:2482 setcvars */
void setcvars(void)
{
    blitzoff = -1;
    intsused = 0;
    anyerrs = 0;
    blitzmode = 0;
    nomemleft = 0;
    constmode = 0;
    cmake = 0;
    cfetchmode = 0;
    intdata1 = 0;
    fuckpos = 0;
    procnum = 0;
    procnum2 = 0;
    dfetch = 0;
    titleat = 0;
    usedpath[0] = 0;
    inerr = 0;
    intlevel = 0;
    cont_pc = 0;
    cont_pc2 = 0;
    lastcontlink = 0;

    pc = pcat;
    bigpc = pcat;
    lib = libat;
    data1 = data1at;
    data2 = data2at;

    comflag = -1;
    linemode = -1;
    oldvcodelen = 0;
    lastbiglab[0] = 0;
}

/* blitz2.s:2526 openstopreq - a0 = two strings (compiling / executing) */
void openstopreq(const char *strs)
{
    const char *a0 = strs;
    char *a1 = reqmessage;
    struct Window *win;
    struct MsgPort *port;
    UBYTE sig;

    clrreq = ~clrreq;
    while ((*a1++ = *a0++) != 0)
        ;
    a1[-1] = ' ';               /* the old " *****" tail stays */
    a1 = stopreqla11;
    while ((*a1++ = *a0++) != 0)
        ;
    ed_openreq(&stopreq);       /* open requester */
    stopit = 0;

    us->tc_ExceptCode = (APTR)stopcode_stub;
    win = (struct Window *)CD_L(CD_REQWINDOW);  /* window */
    port = win->UserPort;                       /* user MsgPort */
    sig = port->mp_SigBit;                      /* Sig Bits. */
    stopbit = (stopbit & 0xff00) | sig;         /* move.b d1,stopbit+1 */
    /* the asm pokes tc_SigExcept directly (no SetExcept) */
    us->tc_SigExcept = 1UL << (sig & 63);       /* Signal Bits */
}

/* ===================================================== a pass */

/* blitz2.s:2556 dopass - compile entire program based on 'pass' */
void dopass(void)
{
    jmp_buf pj;
    int i;

    openstopreq(compiling);
    sleepon();
    setcvars();

    varoff = (WORD)0x8000;
    defaulttype = &quicktype;

    /* asm: passstack = a7 */
    thisitem = (struct editem *)&firstitem;
    linesleft = numlines + 1;
    linenumat = -1;

    resetlibs();
    freeemup();

    for (i = 1; i <= 4; i++)
        titleadd[i] = 0;

    /* put in 4 nops for dummy JSR to init routines! (was 3) */
    pokecode2(nops4, nopsf);

    if (DEBUGON) {
        uselib(runerrlib);
        if (!makeexec) {
            /* not executable! */
            uselib(debuglib);
            uselib(dhandlerlib);
        } else {
            /* is executable! */
            uselib(rundebuglib);
        }
    }

    /* BUG(orig): the result of the first nextline is not tested */
    nextline();

    /* passloop / passnxt.  doinc2 longjmps to passnxt (asm: move.l
       passstack,a7 / jmp passnxt). */
    passjmp = &pj;
    if (setjmp(pj) == 0)
        doline();               /* passloop */
    while (nextline())          /* passnxt */
        doline();
    passjmp = NULL;

    dorunerrson(0);

    pointchk();
    linenumat = 0;
    errchx2();

    if (DEBUGON) {
        debugstuff();
        tokejsr(finalscheck);
        debugstuff();
        /* BUG(orig): also in executables (makeexec); without the debugger
           rundebuglib shows an empty requester when the program runs off
           its end (code 3 is no text pointer; End jumps behind it) */
        pokel(L_(eopcode));
        if (lastcontlink)
            W_(lastcontlink + 6) = 0;
    }

    /* end of whole program! */
    endop = pc;                 /* where end goes... */

    if (blitzmode)
        doamiga(0);             /* back to amiga if in blitz! */

    if (intsused)
        tokejsr2(intcleanup);   /* $c105: interupts off! */

    numstatic = (WORD)calcstatic(&staticdata);
    /* BUG(orig) (#335): before fixjsrs - an object library only reached
       from the code of another library gets no array (base 0), but
       makefinits frees its objects at End */
    calcmaxs();

    if (tokeslist) {
        pokewd(gotocode[0]);    /* jmp! */
        endjmpat = pc;
        addoff();
        /* DIFF: the asm poked whatever d1 held; the long is overwritten
           below unless nomemleft */
        pokel(0);
        fixjsrs();              /* has to be done before finits! */
        /* misc finish-ups */
        if (!nomemleft)
            L_(endjmpat) = pc;
    }
    makefinits();
    pokel(0x70004e75);          /* moveq #0,d0 / rts */
    makeinits();
    asmfixer();

    endstop();
    pointeron();
}

/* blitz2.s:2688 pointchk */
void pointchk(void)
{
    struct type *a2;

    for (a2 = firsttype; a2; a2 = a2->next) {
        if (a2->u.code == TYPE_FORWARD) {
            linenumat = a2->size;
            notypeerr();
        }
    }
}

/* blitz2.s:2700 errchx2 - Make sure all the labels have been resolved */
void errchx2(void)
{
    struct label *a2;

    for (a2 = firstlabel; a2; a2 = a2->next) {
        if ((ULONG)a2->refs & 1)
            continue;
        if (a2->pc)
            continue;
        linenumat = a2->procnum;        /* line of the first reference */
        nolaberr();
    }
    if (concomsp != concomstack)
        chxerr4();
    if (connest)
        chxerr4();
    errchx();
}

/* blitz2.s:2719 errchx */
void errchx(void)
{
    if (firstsel) {
        linenumat = sellineat;
        selerrz();
    }
    if (firstrep) {
        linenumat = replineat;
        unterr3();
    }
    if (intstring) {
        linenumat = intlineat;
        interr2();
    }
    if (firstif) {
        linenumat = iflineat;
        noenderr();
    }
    if (procmode) {
        linenumat = prolineat;
        nopenderr();
    }
    if (firstfor) {
        linenumat = forlineat;
        nonexterr();
    }
}

/* blitz2.s:2747 chxerr4 */
void chxerr4(void)
{
    linenumat = ciflineat;
    nocenderr();
}

/* blitz2.s:2750 calcmaxs - How many max's used in prog? */
void calcmaxs(void)
{
    struct blitzlib *a2;
    WORD d4 = 0;

    maxsat = data1;
    for (a2 = firstlib; a2; a2 = a2->next) {
        UWORD d1;

        if (!(a2->flags & LIBF_USED))
            continue;
        if (!LIBMAX_SHIFT(a2))
            continue;
        d4++;
        pokedata1((UWORD)LIBMAX_VAROFF(a2));
        d1 = (UWORD)(1 << (LIBMAX_SHIFT(a2) & 63));     /* lsl d2,d1 (word) */
        pokedata1l((ULONG)d1 * (UWORD)LIBMAX_COUNT(a2)); /* mulu */
    }
    maxsused = d4;
    if (d4)
        uselib(65530);          /* use memlib */
}

/* ===================================================== statements */

/* blitz2.s:2780 doline */
void doline(void)
{
    UWORD c;

    for (;;) {
        dostatement();
        if (dolineabort) {      /* asm: handler dropped the return here */
            dolineabort = 0;
            return;
        }
        c = reget();
        if (!eos(c))
            illeolerr();
        if (c == ELSETOK) {
            doelse2(c);
            continue;
        }
        if (c)
            continue;
        break;
    }
    /* here we do any 'un-thens' */
    for (;;) {
        struct ifnode *a0 = firstif;
        UWORD d1;

        if (!a0)
            break;
        d1 = a0->kind;
        if (!(d1 & 0x8000))
            break;
        /* an If...Then... found! */
        d1 &= 0x7fff;
        doendif2(a0, d1);
        freetheifz(a0);
    }
}

/* blitz2.s:2807 createlab - loclabch fetched, make a big label! */
UWORD createlab(void)
{
    const char *a0 = lastbiglab;
    char *a1 = (char *)namebuff;
    UWORD d2 = 0;
    UWORD len;

    if (!*a0)
        nolocerr();
    do {
        d2++;
    } while ((*a1++ = *a0++) != 0);
    a1[-1] = loclabch;
    len = makename4(a1, d2, NULL);
    if (len == d2)
        syntaxerr();
    return len;
}

/* blitz2.s:2824 prepstack */
void prepstack(void)
{
    forthsp = forthstack;
    precsp = precstack;
}

/* blitz2.s:2828 chkrealstop - return ne if really a stop! */
BOOL chkrealstop(void)
{
    WORD d4 = 0;

    for (;;) {
        WORD d6 = 0;
        LONG d7 = ed_getinput2(&d6);
        if (!d7)
            break;
        if ((WORD)d7 == -1 && d6 == 1)
            d4 = -1;
    }
    stopit = d4;
    return d4 != 0;
}

/* blitz2.s:2847 chkstop */
void chkstop(void)
{
    if (!stopit)
        return;
    if (!chkrealstop())
        return;
    gostop();                   /* falls through into gostop */
}

/* blitz2.s:2853 gostop */
void gostop(void)
{
    endstop();
    pointeron();
    /* asm: a7 = errstack, jmp (errcont) */
    anyerrs = -1;
    longjmp(*errjmp, 1);
}

/* blitz2.s:2868 maketrap */
void maketrap(UWORD n)
{
    pokewd(n | 0x4e40);
}

/* blitz2.s:2871 debugstuff */
void debugstuff(void)
{
    ULONG d0, d1;

    if (!DEBUGON)
        return;
    if (macmask)
        return;                 /* no debugging of Macros! */
    if (dirmode)
        return;                 /* or while in direct mode... */

    d_pc = pc;
    d_bigpc = bigpc;
    d_nomemleft = (UWORD)nomemleft;

    maketrap(1);

    /* hook up cont address... */
    d0 = pc;
    d1 = cont_pc;
    cont_pc = d0;
    cont_pc2 = d1;
    if (d1 && !nomemleft) {
        W_(d1 + 6) = (UWORD)(d0 - d1);
        lastcontlink = d1;
    }
    pokel((ULONG)thisitem);
    pokewd((UWORD)(srcptr - (UBYTE *)thisitem));
    pokewd(0);                  /* next cont goes here! */
}

/* blitz2.s:2915 undodebug */
void undodebug(void)
{
    if (!DEBUGON)
        return;
    if (macmask)
        return;
    nomemleft = (WORD)d_nomemleft;
    bigpc = d_bigpc;
    pc = d_pc;
    cont_pc = cont_pc2;
}

/* blitz2.s:2927 setsvars - set statement vars... */
void setsvars(void)
{
    prepstack();
    regat = 0;
    nonew = 0;
    sbasegot = 0;
    oldvcodelen = varcodelen;
    varcodelen = 0;
    lasta6 = -1;
}

/* blitz2.s:2939 smode - special mode...
   DIFF: the asm drops the return into handlechar (addq #4,a7) and jumps
   to the mode handler; here smode returns TRUE and handlechar returns. */
static BOOL smode(void)
{
    UWORD c;

    if (linemode < 0)
        return FALSE;
    undodebug();
    c = reget();
    modetable[linemode](c);
    return TRUE;
}

static void handlechar(void);
static void handleascii(UWORD c);
static void handleeol(UWORD c);
static void handlelocal(void);
static void handlemouse(void);
static void handlelabel(UWORD len);
static void handlelab2(UWORD len);
static void handleconst(void);
static void handleproc(UWORD len);
static void handletoken(UWORD c);
static void handlelib(UWORD tok);
static void sizespec(UBYTE *sub);

/* blitz2.s:2953 dostatement - process individual statement */
void dostatement(void)
{
    chkstop();
    expandmax();
    debugstuff();
    handlechar();               /* falls through into handlechar */
}

/* blitz2.s:2961 handlechar - Handle the first character in statement */
static void handlechar(void)
{
    UWORD c = getchar3();

    if (eos(c)) {
        handleeol(c);
        return;
    }
    setsvars();
    if (smode())
        return;
    if (c == '.') {
        handlemouse();
        return;
    }
    if (c == '#') {
        handleconst();
        return;
    }
    if (c == loclabch) {
        handlelocal();
        return;
    }
    if (c & 0x8000) {
        handletoken(c);
        return;
    }
    handleascii(c);             /* falls through into handleascii */
}

/* blitz2.s:2977 handleascii */
static void handleascii(UWORD c)
{
    UWORD len;

    letstart = (ULONG)srcptr;
    getvname(c);
    len = getvnamelen;                          /* asm d2 from makename */
    c = reget();
    if (eos(c)) {
        handlelabel(len);
        return;
    }
    if (c == '{') {
        handleproc(len);
        return;
    }
    dolet2(c, len, NULL, NULL);
}

/* blitz2.s:2986 handleeol */
static void handleeol(UWORD c)
{
    if (c == ELSETOK) {
        handletoken(c);
        return;
    }
    undodebug();                /* !*!*! */
}

/* blitz2.s:2991 handlelocal */
static void handlelocal(void)
{
    UWORD len;

    undodebug();                /* !***! */
    len = createlab();
    handlelab2(len);
}

/* blitz2.s:2996 handlemouse */
static void handlemouse(void)
{
    UWORD c = getchar3();
    UWORD len;

    if (eos(c)) {
        handleeol(c);           /* syntaxerr */
        return;
    }
    if (c == loclabch) {
        handlelocal();
        return;
    }
    len = makename2(NULL);
    handlelabel(len);           /* falls through into handlelabel */
}

/* blitz2.s:3002 handlelabel - an ordinary label */
static void handlelabel(UWORD len)
{
    undodebug();                /* !***! */
    strcpy(lastbiglab, (char *)namebuff);
    handlelab2(len);            /* falls through into handlelab2 */
}

/* blitz2.s:3011 handlelab2 */
static void handlelab2(UWORD len)
{
    struct node *last;
    struct label *a2 = findadd(len, &last);
    struct labref *a3;

    if (!a2) {
        a2 = (struct label *)addhere(last, NAMEOFF_LABEL, len);
        goto l_done;
    }
    /* .found */
    if ((ULONG)a2->refs & 1)
        illlaberr();
    if (a2->pc)
        duplaberr();
    a2->procnum = procnum;

    /* now, we have to fill in blanks in the Bloody past code.... */
    a3 = a2->refs;              /* start of refs list */
    while (a3) {
        struct labref *next;

        if (a3->procnum != a2->procnum) {
            /* Bad Reference */
            linenumat = a3->line;
            referr();
        }
        if (!nomemleft) {
            if (a3->addr & 1)
                /* OK it's a 'Restore' references */
                L_(a3->addr - 1) = data2;
            else
                L_(a3->addr) = pc;      /* .pcref */
        }
        /* .isaskip */
        next = a3->next;
        a2->refs = next;        /* new first. */
        FreeMem(a3, 12);
        a3 = next;
    }
l_done:
    a2->refs = NULL;            /* no refs */
    a2->pc = pc;                /* pc of label. */
    a2->data = data2;           /* pc for data statements */
    a2->procnum = procnum;
}

/* blitz2.s:3065 handleconst */
static void handleconst(void)
{
    struct node *last;
    struct label *a2;
    UWORD len, c;

    prepstack();                /* forth stax */
    len = makename(NULL);       /* BUG(orig): empty name not checked */
    a2 = findadd(len, &last);
    if (!a2) {
        a2 = (struct label *)addhere(last, NAMEOFF_LABEL, len);
        a2->refs = (struct labref *)1;
    } else if ((ULONG)a2->refs != 1) {
        illconerr();
    }
    c = reget();
    if (c == '=')
        a2->pc = evalconst2(NULL);      /* .asscon */
}

/* blitz2.s:3083 handleproc - a procedure! - eg gameover{score.l,player} */
static void handleproc(UWORD len)
{
    struct proc *a2 = findproc(len, NULL);
    ULONG ppc;
    UWORD c;

    if (!a2)
        noprocerr();
    if (a2->rettype >= 0)       /* tst.b 5(a2) / bpl */
        illprocerr();
    ppc = a2->pc;               /* pc of proc. */
    if (a2->nparams)
        c = fetchpees2(a2->nparams, a2->params);        /* var desc list */
    else
        c = getchar3();
    if (c != '}')
        syntaxerr();
    chkstak();
    makefjsr(ppc);
    getchar3();
}

/* blitz2.s:3105 handletoken - process a token in d0 */
static void handletoken(UWORD c)
{
    UWORD d0, idx;

    if (c >= 0x8000 + fnum && c < 0x8000 + tnum) {
        if (!(optreq2ga3.Flags & GFLG_SELECTED))
            undodebug();
        asmit(c);               /* assemble the bastard */
        return;
    }
    /* .nono: check if asm type things.... if so, no line remembering etc! */
    d0 = c & 0x7fff;
    if (d0 & 0x7f00) {
        handlelib(d0);
        return;
    }
    if (d0 >= tnum) {
        d0 -= tnum - fnum;
        if (d0 == fnum + 15 || d0 == fnum + 10)
            undodebug();
    } else if (d0 == 56 || d0 == 57 || d0 == 13 || d0 == 18 || d0 == 19 ||
               d0 == 26 || (d0 <= 42 && d0 >= 35)) {
        /* things which don't want error checking... */
        undodebug();
    }
    /* .doerr */
    idx = (UWORD)((d0 - 1) << 2);
    if (idx >= NATOKENS * 4) {
        /* Freaked out Token! */
        flash();
        return;
    }
    /* asm: push handler, bra getchar3 */
    c = getchar3();
    atokens[idx >> 2](c);
}

/* blitz2.s:3169 handlelib - a token from a library! */
static void handlelib(UWORD tok)
{
    struct blitzlib *a2;
    UBYTE *a3 = findtoke(tok, &a2);     /* toke stuff in a3 */
    UWORD n;

    if (a3[1] & 4) {
        stamigalib(tok, a3);
        return;
    }
    if (!(a3[1] & 1))
        syntaxerr();            /* check it's a statement */
    sizespec(a3);               /* does it need a size specifier? */
    /* DIFF: asm d0 here is whatever findtoke/sizespec left; countpees only
       stores it (storeloc) and restores it into lastchar, which the
       following getchar overwrites */
    n = countpees(lastchar);
    dolibtoke(n, a2, a3);       /* falls through into dolibtoke */
}

/* blitz2.s:3181 dolibtoke - number of parameters in d1....
   a2=lib base, a3=sub for this toke */
UWORD dolibtoke(UWORD d1, struct blitzlib *a2, UBYTE *a3)
{
    WORD s_numreps = numreps;   /* recurs. */
    WORD s_userp = userp;
    WORD s_stackuse = stackuse;
    UBYTE *a1;
    UWORD d2;

    userp = usertype;
    a1 = a3 + 6;
    /* now to find a form with d1 parameters in it..... */
    for (;;) {
        WORD w = (WORD)W_(a1);
        UWORD d3;

        if (w < 0)
            syntaxerr();
        d2 = (UWORD)w & 255;
        if (d1 == d2)
            break;
        if (d1 > d2) {
            d3 = a1[0] & 7;
            if (d3) {
                UWORD d4 = d2 - d3;
                ULONG d5 = (UWORD)(d1 - d4);
                if (d5 % d3 == 0)       /* divu / swap / tst */
                    break;
            }
        }
        /* .next */
        a1 += 2 + d2;
        a1 = aligna1(a1);
        a1 = skiplibreg(a1);
        a1 += 12;
    }
    /* Got it! */
    d1 |= (UWORD)a1[0] << 8;
    stackuse = 0;
    if (d1)
        fetchlibps(d1, a1 + 2);
    else
        getchar3();
    if (stackuse) {
        preplibst[1] = (UWORD)stackuse;         /* move d1,preplibst+2 */
        pokel(L_(preplibst));
        movenumrep[1] = (UWORD)numreps;         /* move numreps,movenumrep+2 */
        if (numreps)
            pokel(L_(movenumrep));
    }
    userjsr(a2, a1);            /* make the JSR */
    if (stackuse) {
        UWORD s = (UWORD)stackuse;
        if (s <= 8) {
            pokewd((UWORD)(((s & 7) << 9) | 0x504f));   /* addq #x,a7 */
        } else {
            fixlibst[1] = s;
            pokel(L_(fixlibst));
        }
    }
    stackuse = s_stackuse;
    userp = s_userp;
    numreps = s_numreps;
    return reget();
}

/* blitz2.s:3260 sizespec2 */
UWORD sizespec2(void)
{
    struct type *a2;
    UWORD c, d1;

    prevloc = srcptr;
    c = getchar_();
    if (c == '.') {
        /* .ok1 */
        UWORD len = makename(NULL);
        if (!len)
            syntaxerr();
        a2 = findtype((struct node *)&alltypes, len, NULL);
        if (!a2)
            notypeerr();
    } else {
        a2 = defaulttype;
        if (c == '$') {
            d1 = 7;
            usertype = d1;
            return d1;
        }
    }
    /* .none */
    bakup();
    if (a2->u.code >= 256)
        illtypeerr();
    d1 = TYPE_CODE(a2);
    usertype = d1;
    return d1;
}

/* blitz2.s:3283 sizespec */
static void sizespec(UBYTE *sub)
{
    UWORD d1;

    if (!(sub[1] & 8))
        return;
    d1 = sizespec2();
    pokewd(d1 | 0x7000);        /* moveq #type,d0 */
    regat++;
}
