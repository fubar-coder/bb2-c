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
        ULONG dividend;
        UWORD q;

        riteend = (UWORD)((w >> 3) - 7);
        dividend = (ULONG)riteend << 16;
        /* divu numlines,d0.  DIFF: numlines==0 would trap in the asm.
           On overflow (quotient > $ffff) divu leaves d0 unchanged, whose
           low word is 0 -> -1 below, as in the asm. */
        if (numlines == 0 || dividend / numlines > 0xffff)
            q = 0;
        else
            q = (UWORD)(dividend / numlines);
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
            WORD gadget = 0;
            if (!ed_getinput(&gadget))  /* getinput: d7 != -1 */
                continue;
            if (gadget == 1) {
                /* .inc */
                if (!alloxinc()) {
                    freeemup();         /* asm: beq freeemup (tail) */
                    errjmp = olderr;
                    return;
                }
                goto l_retry;
            }
            if (gadget == 2)
                goto l_makesmall;
            if (gadget == 3)
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
    struct type *last = lastrestype;
    struct type *rest = last->next;

    last->next = NULL;
    freetypes(rest);
}

/* blitz2.s:2024 frallvars */
void frallvars(void)
{
    struct var *v = firstvar;

    firstvar = NULL;
    freevars(v);
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
        UWORD inc = (UWORD)numincs;
        if (inc < 4) {
            UWORD col = (UWORD)titleadd[inc];   /* offset in line */
            if (col < riteend) {
                UBYTE *bits;
                WORD row;

                titleadd[inc]++;
                row = (WORD)(inc * bmapmul);
                bits = (UBYTE *)CD_L(CD_COMBITMAP) + row;
                bits[col] ^= 0xff;
                bits = (UBYTE *)CD_L(CD_COMBITMAP) + (WORD)botline - row;
                bits[col] ^= 0xff;
            }
        }
        return TRUE;            /* asm: moveq #-1,d0 */
    }
    /* .skip3: do main stripe */
    {
        ULONG newat = titleat + (UWORD)titleadd[0];
        WORD hi = (WORD)(newat >> 16);

        if ((UWORD)hi != (UWORD)(titleat >> 16)) {
            UBYTE *bits = (UBYTE *)CD_L(CD_COMBITMAP);
            bits[hi - 1] ^= 0xff;
            bits += (WORD)botline;
            bits[hi - 1] ^= 0xff;
        }
        titleat = newat;
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
void hexascii(UWORD val, UBYTE *buf)
{
    int i;

    for (i = 3; i >= 0; i--) {
        UWORD digit = (val & 15) + 48;
        if (digit >= 58)
            digit += 7;
        buf[i] = (UBYTE)digit;
        val >>= 4;
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
        UBYTE *bang, *args, *out, *in, *end, *text;
        struct macro *m;
        WORD narg;
        UWORD left;

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
        bang = p;
        {
            UWORD len = makename(NULL);
            if (!len)
                syntaxerr();
            bakup();
            m = findmac(len, NULL);
            if (!m)
                nomacerr();
        }

        args = srcptr;          /* start of parameters! */
        narg = 0;               /* narg=0 */
        if (*args++ == '{') {
            struct macparam *mp = (struct macparam *)macrobuff;
            UWORD t;
            /* no check against the end of macrobuff, as in the original: not
               reachable (#4, closed: >1365 parameters needed) */
            do {
                narg++;         /* another p */
                mp->start = args;
                t = findit(&args);
                mp->len = (UWORD)(args - mp->start - 1);
                mp++;
            } while (t != '}');
            srcptr = args;
        }
        numarg = narg;

        myoline = myline;
        myline = doallocmem(macrolen, MEMF_PUBLIC);
        out = myline;
        end = out + macrolen;   /* end of macro buffer (asm a6) */

        in = start;             /* start of the line */
        do {
            if (out >= end)
                mbovererr();
            *out++ = *in++;
        } while (in < bang);
        out--;                  /* back over '!' */

        left = m->textlen;      /* mac len */
        if (left) {
            macnum = (WORD)m->usecnt;
            m->usecnt++;
            text = m->text;
            left--;
            do {
                /* .loop3 */
                UWORD mch = *text++;
                UWORD param;            /* parameter number (from 1) */
                ULONG hiword = 0;       /* upper word of asm d0 (moveq #0) */
                if (mch != '`') {
                    /* .oky */
                    if (out >= end)
                        mbovererr();
                    *out++ = (UBYTE)mch;
                    if (mch & 0x80) {
                        if (out >= end)
                            mbovererr();
                        *out++ = *text++;
                        left--;
                    }
                    continue;
                }
                left--;
                mch = *text++;
                if (mch == '{') {
                    /* .evalit */
                    UBYTE *savea5 = srcptr;
                    UWORD c;
                    ULONG v;

                    srcptr = text;
                    v = evalconst2(&c);
                    if (c != '}')
                        syntaxerr();
                    left -= (UWORD)(srcptr - text);
                    text = srcptr;
                    quoteflag = 0;
                    srcptr = savea5;
                    if ((LONG)v < 0)
                        ilconsterr();
                    param = (UWORD)v;
                    hiword = v & 0xffff0000UL;  /* move.l d3,d0 */
                    goto l_itsgot;
                }
                if (mch == '@') {
                    /* macro num parameter.  No buffer overflow check for
                       the digits, as in the original: not observable (#5,
                       closed: the rest of the line still ends in "Macro
                       Buffer Overflow") */
                    out = (UBYTE *)makelong((char *)out, (UWORD)macnum);
                    continue;
                }
                /* .notnum */
                param = mch | 32;
                if (param >= 'z')
                    ilconsterr();
                param -= 48;
                if (param >= 10) {
                    param -= 39;
                    if (param < 10)
                        ilconsterr();
                }
            l_itsgot:
                param--;
                if ((WORD)param < 0) {
                    /* here, we do a numarg text replace */
                    /* move numarg,d0 / divu #10,d0: only the low word is
                       replaced, so after `{expr} the upper word of the
                       value is still there (review: kept as in the asm);
                       on divu overflow d0 stays unchanged */
                    ULONG q = hiword | (UWORD)numarg;
                    if (q / 10 <= 0xffff)
                        q = ((q % 10) << 16) | (q / 10);
                    if (out >= end)
                        mbovererr();
                    *out++ = (UBYTE)(q + 48);
                    if (out >= end)
                        mbovererr();
                    *out++ = (UBYTE)((q >> 16) + 48);
                    continue;
                }
                /* .ok: ok to do parameter replacement,
                   d0 = number for parameter replacement! */
                if (param < (UWORD)numarg) {
                    struct macparam *mp = (struct macparam *)macrobuff + param;
                    UWORD n = mp->len;  /* len of replacetext */
                    if (n) {
                        UBYTE *s = mp->start;
                        do {
                            if (out >= end)
                                mbovererr();
                            *out++ = *s++;
                        } while (--n);
                    }
                }
            } while (left-- != 0);      /* dbf d1,.loop3 */
        }

        /* .loop4: rest of the line */
        do {
            if (out >= end)
                mbovererr();
        } while ((*out++ = *srcptr++) != 0);

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
    UBYTE *buf = myline;

    if (buf) {
        myline = NULL;
        FreeMem(buf, macrolen);
    }
}

/* blitz2.s:2351 findit - find the end of a parameter */
UWORD findit(UBYTE **pp)
{
    UBYTE *p = *pp;
    WORD nest = 0;              /* no nesting */
    UWORD c;

    for (;;) {
        c = *p++;
        if (!c)
            syntaxerr();
        if (c & 0x80) {
            p++;
            continue;
        }
        if (c == '"') {
            quoteflag = ~quoteflag;
            continue;
        }
        if (quoteflag)
            continue;
        if (c == ',') {
            if (nest == 0)
                break;
            continue;
        }
        if (isitopen(c)) {
            nest++;
            continue;
        }
        if (c == '}' && nest == 0)
            break;
        if (isitclose(c))
            nest--;
    }
    *pp = p;
    return c;
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
    ULONG sigs = r->d[0];

    if (stopit)
        return;
    if (!(sigs & (1UL << (stopbit & 31))))     /* btst d1,d0 */
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
    const char *src = strs;
    char *dst = reqmessage;
    struct Window *win;
    struct MsgPort *port;
    UBYTE sig;

    clrreq = ~clrreq;
    while ((*dst++ = *src++) != 0)
        ;
    dst[-1] = ' ';              /* the old " *****" tail stays */
    dst = stopreqla11;
    while ((*dst++ = *src++) != 0)
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

    /* the result of the first nextline is not tested (TED always has a line) */
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
        /* BUG(orig) (#6): also in executables (makeexec); without the debugger
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
    struct type *t;

    for (t = firsttype; t; t = t->next) {
        if (t->u.code == TYPE_FORWARD) {
            linenumat = t->size;
            notypeerr();
        }
    }
}

/* blitz2.s:2700 errchx2 - Make sure all the labels have been resolved */
void errchx2(void)
{
    struct label *l;

    for (l = firstlabel; l; l = l->next) {
        if ((ULONG)l->refs & 1)
            continue;
        if (l->pc)
            continue;
        linenumat = l->procnum;         /* line of the first reference */
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
    struct blitzlib *bl;
    WORD count = 0;

    maxsat = data1;
    for (bl = firstlib; bl; bl = bl->next) {
        UWORD unit;

        if (!(bl->flags & LIBF_USED))
            continue;
        if (!LIBMAX_SHIFT(bl))
            continue;
        count++;
        pokedata1((UWORD)LIBMAX_VAROFF(bl));
        unit = (UWORD)(1 << (LIBMAX_SHIFT(bl) & 63));   /* lsl d2,d1 (word) */
        pokedata1l((ULONG)unit * (UWORD)LIBMAX_COUNT(bl)); /* mulu */
    }
    maxsused = count;
    if (count)
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
        struct ifnode *ifn = firstif;
        UWORD kind;

        if (!ifn)
            break;
        kind = ifn->kind;
        if (!(kind & 0x8000))
            break;
        /* an If...Then... found! */
        kind &= 0x7fff;
        doendif2(ifn, kind);
        freetheifz(ifn);
    }
}

/* blitz2.s:2807 createlab - loclabch fetched, make a big label! */
UWORD createlab(void)
{
    const char *src = lastbiglab;
    char *dst = (char *)namebuff;
    UWORD prelen = 0;
    UWORD len;

    if (!*src)
        nolocerr();
    do {
        prelen++;
    } while ((*dst++ = *src++) != 0);
    dst[-1] = loclabch;
    len = makename4(dst, prelen, NULL);
    if (len == prelen)
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
    WORD stop = 0;

    for (;;) {
        WORD gadget = 0;
        LONG input = ed_getinput2(&gadget);
        if (!input)
            break;
        if ((WORD)input == -1 && gadget == 1)
            stop = -1;
    }
    stopit = stop;
    return stop != 0;
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
    ULONG here, prev;

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
    here = pc;
    prev = cont_pc;
    cont_pc = here;
    cont_pc2 = prev;
    if (prev && !nomemleft) {
        W_(prev + 6) = (UWORD)(here - prev);
        lastcontlink = prev;
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
    struct label *l = findadd(len, &last);
    struct labref *ref;

    if (!l) {
        l = (struct label *)addhere(last, NAMEOFF_LABEL, len);
        goto l_done;
    }
    /* .found */
    if ((ULONG)l->refs & 1)
        illlaberr();
    if (l->pc)
        duplaberr();
    l->procnum = procnum;

    /* now, we have to fill in blanks in the Bloody past code.... */
    ref = l->refs;              /* start of refs list */
    while (ref) {
        struct labref *next;

        if (ref->procnum != l->procnum) {
            /* Bad Reference */
            linenumat = ref->line;
            referr();
        }
        if (!nomemleft) {
            if (ref->addr & 1)
                /* OK it's a 'Restore' references */
                L_(ref->addr - 1) = data2;
            else
                L_(ref->addr) = pc;     /* .pcref */
        }
        /* .isaskip */
        next = ref->next;
        l->refs = next;         /* new first. */
        FreeMem(ref, 12);
        ref = next;
    }
l_done:
    l->refs = NULL;             /* no refs */
    l->pc = pc;                 /* pc of label. */
    l->data = data2;            /* pc for data statements */
    l->procnum = procnum;
}

/* blitz2.s:3065 handleconst */
static void handleconst(void)
{
    struct node *last;
    struct label *l;
    UWORD len, c;

    prepstack();                /* forth stax */
    len = makename(NULL);       /* BUG(orig) (#7): empty name not checked */
    l = findadd(len, &last);
    if (!l) {
        l = (struct label *)addhere(last, NAMEOFF_LABEL, len);
        l->refs = (struct labref *)1;
    } else if ((ULONG)l->refs != 1) {
        illconerr();
    }
    c = reget();
    if (c == '=')
        l->pc = evalconst2(NULL);       /* .asscon */
}

/* blitz2.s:3083 handleproc - a procedure! - eg gameover{score.l,player} */
static void handleproc(UWORD len)
{
    struct proc *p = findproc(len, NULL);
    ULONG ppc;
    UWORD c;

    if (!p)
        noprocerr();
    if (p->rettype >= 0)        /* tst.b 5(a2) / bpl */
        illprocerr();
    ppc = p->pc;                /* pc of proc. */
    if (p->nparams)
        c = fetchpees2(p->nparams, p->params);          /* var desc list */
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
    UWORD tok, idx;

    if (c >= 0x8000 + fnum && c < 0x8000 + tnum) {
        if (!(optreq2ga3.Flags & GFLG_SELECTED))
            undodebug();
        asmit(c);               /* assemble the bastard */
        return;
    }
    /* .nono: check if asm type things.... if so, no line remembering etc! */
    tok = c & 0x7fff;
    if (tok & 0x7f00) {
        handlelib(tok);
        return;
    }
    if (tok >= tnum) {
        tok -= tnum - fnum;
        if (tok == fnum + 15 || tok == fnum + 10)
            undodebug();
    } else if (tok == 56 || tok == 57 || tok == 13 || tok == 18 || tok == 19 ||
               tok == 26 || (tok <= 42 && tok >= 35)) {
        /* things which don't want error checking... */
        undodebug();
    }
    /* .doerr */
    idx = (UWORD)((tok - 1) << 2);
    if (idx >= NATOKENS * 4) {
        /* Freaked out Token! */
        flash();
        return;
    }
    /* asm: push handler, bra getchar3 */
    atokens[idx >> 2](getchar3());
}

/* blitz2.s:3169 handlelib - a token from a library! */
static void handlelib(UWORD tok)
{
    struct blitzlib *blib;
    UBYTE *sub = findtoke(tok, &blib);  /* toke stuff in a3 */
    UWORD n;

    if (sub[1] & 4) {
        stamigalib(tok, sub);
        return;
    }
    if (!(sub[1] & 1))
        syntaxerr();            /* check it's a statement */
    sizespec(sub);              /* does it need a size specifier? */
    /* DIFF: asm d0 here is whatever findtoke/sizespec left; countpees only
       stores it (storeloc) and restores it into lastchar, which the
       following getchar overwrites */
    n = countpees(lastchar);
    dolibtoke(n, blib, sub);    /* falls through into dolibtoke */
}

/* blitz2.s:3181 dolibtoke - number of parameters in d1....
   a2=lib base, a3=sub for this toke */
UWORD dolibtoke(UWORD nparams, struct blitzlib *blib, UBYTE *sub)
{
    WORD s_numreps = numreps;   /* recurs. */
    WORD s_userp = userp;
    WORD s_stackuse = stackuse;
    UBYTE *form;
    UWORD fparams, pspec;

    userp = usertype;
    form = sub + 6;
    /* now to find a form with d1 parameters in it..... */
    for (;;) {
        WORD w = (WORD)W_(form);
        UWORD nrep;

        if (w < 0)
            syntaxerr();
        fparams = (UWORD)w & 255;
        if (nparams == fparams)
            break;
        if (nparams > fparams) {
            nrep = form[0] & 7;
            if (nrep) {
                UWORD fixed = fparams - nrep;
                ULONG extra = (UWORD)(nparams - fixed);
                if (extra % nrep == 0)  /* divu / swap / tst */
                    break;
            }
        }
        /* .next */
        form += 2 + fparams;
        form = aligna1(form);
        form = skiplibreg(form);
        form += 12;
    }
    /* Got it! */
    pspec = nparams | (UWORD)form[0] << 8;  /* + repeat info, see fetchlibps */
    stackuse = 0;
    if (pspec)
        fetchlibps(pspec, form + 2);
    else
        getchar3();
    if (stackuse) {
        preplibst[1] = (UWORD)stackuse;         /* move d1,preplibst+2 */
        pokel(L_(preplibst));
        movenumrep[1] = (UWORD)numreps;         /* move numreps,movenumrep+2 */
        if (numreps)
            pokel(L_(movenumrep));
    }
    userjsr(blib, form);        /* make the JSR */
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
    struct type *t;
    UWORD c, code;

    prevloc = srcptr;
    c = getchar_();
    if (c == '.') {
        /* .ok1 */
        UWORD len = makename(NULL);
        if (!len)
            syntaxerr();
        t = findtype((struct node *)&alltypes, len, NULL);
        if (!t)
            notypeerr();
    } else {
        t = defaulttype;
        if (c == '$') {
            code = 7;
            usertype = code;
            return code;
        }
    }
    /* .none */
    bakup();
    if (t->u.code >= 256)
        illtypeerr();
    code = TYPE_CODE(t);
    usertype = code;
    return code;
}

/* blitz2.s:3283 sizespec */
static void sizespec(UBYTE *sub)
{
    UWORD code;

    if (!(sub[1] & 8))
        return;
    code = sizespec2();
    pokewd(code | 0x7000);      /* moveq #type,d0 */
    regat++;
}
