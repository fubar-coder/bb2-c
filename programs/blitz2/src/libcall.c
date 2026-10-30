/*
 * libcall.c - chunk 07, blitz2.s 10684-12135:
 * number fetch, code push/pull, library call / init code generation,
 * compile-time constant evaluation, include reader, Data poking and the
 * CNIF/CSIF compare helpers.  See notes/survey-07.md and libcall.h.
 */
#include "blitz2.h"
#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>

/* asm `tst.b debugga`: tests the high byte of the word (big endian) */
#define DEBUGGA_B ((debugga & 0xff00) != 0)

/* mathffp LVOs (asm: jsr -n(a6) with a6 = maths) */
#define SPFix (-30)
#define SPFlt (-36)
#define SPAdd (-66)
#define SPSub (-72)
#define SPMul (-78)
#define SPDiv (-84)

/* long read of a template (two words, no type punning) */
static inline ULONG tl(const UWORD *p)
{
    return ((ULONG)p[0] << 16) | p[1];
}

/* ===================================================================
 * register results
 * =================================================================== */

ULONG numval;       /* asm d3 after a number fetch / evalconst */
WORD numfloat;      /* asm d6: 0 int, != 0 float */


static ULONG fetchnum3(ULONG d0, UWORD *type);
static ULONG consttype2(UWORD *type, ULONG d0);
static ULONG fetchpoint2(UWORD *type);
static void dothee(void);
static UWORD fetchdec(UWORD c, ULONG *d5);

/* ===================================================================
 * number fetch
 * =================================================================== */

/* blitz2.s:10701 fetchdata .datado - dispatch on the first char */
static ULONG datado(UWORD c, UWORD *type)
{
    if (c == '$')
        return fetchhex(type);
    if (c == '%')
        return fetchbin(type);
    /* .doit */
    if (c != '.')
        return fetchnum(c, type);
    return fetchfrac(c, type);
}

/* blitz2.s:10686 fetchdata - fetch number and update type if necessary! */
ULONG fetchdata(UWORD *type)
{
    UWORD c;
    ULONG d0;

    dfetch = -1;
    c = getchar3();
    if (c != '-')
        return datado(c, type);
    c = getchar3();
    d0 = datado(c, type);
    if (*type < T_FLOAT)
        return (ULONG)-(LONG)d0;
    /* .nfp: negate floating point data! */
    return d0 ^ 0x80;           /* bchg #7,d0 (FFP sign bit) */
}

/* blitz2.s:10709 fetchfrac */
ULONG fetchfrac(UWORD c, UWORD *type)
{
    (void)c;
    numval = 0;
    return fetchpoint2(type);
}

/* blitz2.s:10712 fetchnum */
ULONG fetchnum(UWORD c, UWORD *type)
{
    ULONG d5, d0;

    numval = 0;
    c = fetchdec(c, &d5);
    if (c == '.') {
        /* blitz2.s:10734 fetchpoint */
        if (numval >= 0x800000)
            overerr();
        return fetchpoint2(type);
    }
    c |= 32;
    if (c != 'e')
        return fetchnum2(numval, type);
    if (numval >= 0x800000)
        overerr();
    numfloat = -1;
    numval = ffpcall(SPFlt, numval, 0, NULL);     /* int to ffp */
    dothee();
    c = reget();
    d0 = consttype2(type, c);
    return fetchnum3(d0, type);
}

/* blitz2.s:10737 fetchpoint2 - now a float.  in: numval = whole part */
static ULONG fetchpoint2(UWORD *type)
{
    ULONG whole, d5, d0, t;
    UWORD c;

    numfloat = -1;
    whole = ffpcall(SPFlt, numval, 0, NULL);     /* save whole number */
    c = getchar3();
    if (!tstnum(c))
        syntaxerr();                /* BUG(orig) (#10): "1." is a syntax error */
    numval = 0;
    c = fetchdec(c, &d5);           /* get int frac */
    if (numval == 0) {
        /* .pskip.  BUG(orig) (#11): no check for an E exponent here
           ("1.0e5" leaves the 'e' unparsed). */
        numval = whole;
    } else {
        if (numval >= 0x800000)
            overerr();
        d5 = ffpcall(SPFlt, d5, 0, NULL);       /* divisor to ffp */
        t = ffpcall(SPFlt, numval, 0, NULL);     /* to ffp */
        t = ffpcall(SPDiv, t, d5, NULL);        /* num/divisor (eg .25=25/100) */
        numval = ffpcall(SPAdd, t, whole, NULL); /* add 'em */
        c = reget();
        c |= 32;
        if (c == 'e')
            dothee();               /* E Something! */
    }
    /* .pskip2 */
    c = reget();
    d0 = consttype2(type, c);
    return fetchnum3(d0, type);
}

/* blitz2.s:10822 facts - FFP 10^0 .. 10^18 */
static const ULONG facts[19] = {
    0x80000041, 0xa0000044, 0xc8000047, 0xfa00004a,
    0x9c40004e, 0xc3500051, 0xf4240054, 0x98968058,
    0xbebc205b, 0xee6b285e, 0x9502f962, 0xba43b765,
    0xe8d4a568, 0x9184e76c, 0xb5e6216f, 0xe35fa972,
    0x8e1bca76, 0xb1a2bd79, 0xde0b6c7c
};

/* blitz2.s:10804 getsign - 1 or 2 exponent digits -> FFP 10^n (asm d4) */
static ULONG getsign(UWORD c)
{
    UWORD d4;

    if (!tstnum(c))
        syntaxerr();
    d4 = c - 48;
    c = getchar3();
    if (tstnum(c)) {
        d4 = d4 * 10 + (UWORD)(c - 48);
        getchar3();
    }
    /* .skip: d4=number E+10 */
    if (d4 > 18)
        overerr();
    return facts[d4];
}

/* blitz2.s:10775 dothee - Suss stuff after the E (numval = FFP so far) */
static void dothee(void)
{
    UWORD c = getchar3();
    ULONG d4;

    if (c == '-') {
        c = getchar3();
        d4 = getsign(c);
        /* d3=number, d4=divisor */
        numval = ffpcall(SPDiv, numval, d4, NULL);
        return;
    }
    if (c == '+')                   /* .pos */
        c = getchar3();
    /* .pos2: d3=number so far, d4=multiplier */
    d4 = getsign(c);
    numval = ffpcall(SPMul, numval, d4, NULL);
}

/* blitz2.s:10833 fetchdec - numval = numval*10 + digits; *d5 = 10^digits
   (divisor for frac convert!).  Returns the next (non digit) char. */
static UWORD fetchdec(UWORD c, ULONG *d5p)
{
    ULONG d5 = 1, d1;

    do {
        d5 = d5 * 10;               /* unchecked, as the asm */
        /* only the shifts are overflow checked (bcs overerr) */
        if (numval & 0x80000000)
            overerr();
        numval <<= 1;
        d1 = numval;
        if (numval & 0x80000000)
            overerr();
        numval <<= 1;
        if (numval & 0x80000000)
            overerr();
        numval <<= 1;
        numval += d1;
        numval += (UWORD)(c - 48);
        c = getchar3();
    } while (tstnum(c));
    *d5p = d5;
    return c;
}

/* blitz2.s:10854 dfetch - fetching data? */
WORD dfetch;

/* blitz2.s:10856 fetchnum3 - return the value (Data) or emit
   move.w/move.l #d0,D(regat) and return the next char */
static ULONG fetchnum3(ULONG d0, UWORD *type)
{
    if (dfetch) {
        dfetch = 0;
        return d0;
    }
    switch (*type) {
    case T_BYTE:
    case T_WORD:
        /* .word */
        pokewd((UWORD)(regat << 9) | 0x303c);
        pokewd((UWORD)d0);
        break;
    case T_LONG:
    case T_QUICK:
    case T_FLOAT:
        /* .long */
        pokewd((UWORD)(regat << 9) | 0x203c);
        pokel(d0);
        break;
    default:
        mismatcherr();
    }
    return reget();
}

/* blitz2.s:10892 makeint - *d0 = numval as integer.  asm: on SPFix
   overflow it drops its return address (addq #4,a7) and jumps to
   numdofloat, i.e. returns from consttype2 as a float.
   DIFF: returns FALSE instead, consttype2 then does numdofloat. */
static BOOL makeint(ULONG *d0)
{
    UWORD ccr;

    *d0 = numval;
    if (numfloat == 0)
        return TRUE;                /* already integer */
    *d0 = ffpcall(SPFix, numval, 0, &ccr);
    if (ccr & CCR_V)
        return FALSE;
    return TRUE;
}

/* blitz2.s:10902 fetchnum2 - no frac */
ULONG fetchnum2(ULONG d3, UWORD *type)
{
    ULONG d0;

    numval = d3;
    numfloat = 0;
    /* d0 only matters on consttype2's float paths (numfloat != 0) */
    d0 = consttype2(type, lastchar);
    return fetchnum3(d0, type);
}

/* blitz2.s:10906 constotype - fetch a constant of type d2, return new
   type in d2! */
ULONG constotype(UWORD *type)
{
    UWORD c;

    evalconst(&c);
    return consttype2(type, c);
}

/* blitz2.s:10913 consttype2 - size the constant numval (int if numfloat == 0,
   else FFP) to *type (0 = decide).  Returns the value (asm d0); *type =
   final type.  `d0` = the asm d0 on entry (normally the current char),
   see the BUG at .quick2. */
static ULONG consttype2(UWORD *type, ULONG d0)
{
    UWORD d2 = *type;
    UWORD whole, ccr;
    ULONG t;

    if (d2 == 0) {
        /* Let's set type */
        if (numfloat) {
            d2 = T_QUICK;
            goto quick2;
        }
        d2 = T_BYTE;
        goto byte;
    }
    /* .something */
    switch (d2) {
    case T_BYTE:  goto byte;
    case T_WORD:  goto word;
    case T_LONG:  goto long_;
    case T_QUICK: goto quick;
    case T_FLOAT: goto float_;
    default:      mismatcherr();
    }

byte:
    if (!makeint(&d0))
        goto numdofloat;
    if ((LONG)d0 <= 127 && (LONG)d0 >= -128)
        goto done;                  /* .dobword */
over1:
    d2 = T_WORD;
    if ((LONG)d0 <= 32767 && (LONG)d0 >= -32768)
        goto done;
byte2:
    d2 = T_LONG;
    goto done;

word:
    if (numfloat)
        goto quick2;    /* BUG(orig) (#12): d0 not loaded (see .quick2) */
    if (!makeint(&d0))
        goto numdofloat;
    goto over1;

quick:
    if (!makeint(&d0))
        goto numdofloat;
    if (numfloat == 0)
        goto qint;
quick2:
    /* BUG(orig) (#12): expects d0 = int part (from makeint), but the paths
       "type 0 + float" and "type w + float" arrive with d0 = the current
       char.  The fraction is then usually out of range and the constant
       falls back to float.  Kept.  (a6 = maths assumed, as the asm) */
    if ((LONG)d0 > 32767 || (LONG)d0 < -32768)
        goto numdofloat;
    whole = (UWORD)d0;                                  /* save whole part */
    t = ffpcall(SPFlt, d0, 0, NULL);                    /* back to ffp */
    d0 = ffpcall(SPSub, numval, t, NULL);                /* d0=fraction! */
    d0 = ffpcall(SPMul, d0, 0x80000051, NULL);          /* *65536 */
    d0 = ffpcall(SPFix, d0, 0, &ccr);                   /* back to int */
    /* BUG(orig) (#13): a zero fraction (beq) and a negative one (unsigned bcc)
       also go to numdofloat.  Kept. */
    if ((ccr & CCR_Z) || d0 >= 65536)
        goto numdofloat;                                /* .knob */
    d0 = ((ULONG)whole << 16) | (UWORD)d0;
    goto done;

qint:
    if ((LONG)d0 > 32767 || (LONG)d0 < -32768)
        goto byte2;     /* quick int out of range -> long (sic) */
    d0 = (d0 << 16) | (d0 >> 16);                       /* swap d0 */
    goto done;

long_:
    if (numfloat)
        goto numdofloat;
    d0 = numval;
    goto done;

float_:
    if (numfloat)
        goto numdofloat;
    numval = ffpcall(SPFlt, numval, 0, NULL);             /* int to float */
    goto numdofloat;

numdofloat:
    /* blitz2.s:11011 numdofloat */
    d2 = T_FLOAT;
    d0 = numval;
done:
    *type = d2;
    return d0;
}

/* ===================================================================
 * code push/pull
 * =================================================================== */

static ULONG pushfrom __attribute__((unused));  /* 11015, unused */
static ULONG pushpc;                /* 11016 */
static struct offnode *pushoff;     /* 11017 */
static struct offnode *pushdooff;   /* 11018 */
static UWORD pushclen;              /* 11019 */
static ULONG pushat;                /* 11020 */

/* blitz2.s:11022 pushstart - Here, we prepare to collect code to be saved
   and added later */
void pushstart(void)
{
    pushpc = pc;
    pushoff = firstoff;
}

/* blitz2.s:11028 pushdo - OK, push code since push start */
void pushdo(void)
{
    ULONG len = pc - pushpc;
    ULONG a0, a1;
    UWORD n;

    pc = pushpc;                    /* New PC */
    pushclen = (UWORD)len;
    if (pushclen == 0 || nomemleft)
        return;
    pushat = (ULONG)doallocmem(len, MEMF_PUBLIC);
    a0 = pushat;                    /* destination */
    a1 = pushpc;                    /* source */
    pc = a1;
    n = (pushclen >> 1) - 1;        /* to word dbf */
    do {
        W_(a0) = W_(a1);
        a0 += 2;
        a1 += 2;
    } while (n-- != 0);
    pushdooff = firstoff;
}

/* blitz2.s:11057 pushput - replace pulled out code! */
void pushput(void)
{
    ULONG d0 = pushclen, d1, a0, a1;
    struct offnode *o;
    UWORD n;

    if (d0 == 0)
        return;
    d1 = pc;                        /* pc before put */
    pc += d0;
    if (nomemleft)
        return;
    n = (UWORD)((d0 >> 1) - 1);
    a1 = d1;
    a0 = pushat;
    do {
        if (a1 >= libat) {
            nomemleft = -1;
            return;     /* BUG(orig) (#14): save buffer pushat is not freed */
        }
        W_(a1) = W_(a0);
        a1 += 2;
        a0 += 2;
    } while (n-- != 0);
    /* OK, now all offsets between pushdooff and pushoff must be adjusted
       by d1-pushpc */
    d1 -= pushpc;
    if (d1) {
        for (o = pushdooff; o != pushoff; o = o->next) {
            /* address in prog.; leave offs >= lib */
            if (o->offset + pcat < libat)
                o->offset += d1;
        }
        /* Whew! */
    }
    FreeMem((APTR)pushat, pushclen);
}

/* ===================================================================
 * code generation
 * =================================================================== */

/* blitz2.s:11126 muld1 .once - lsl.l #n,D(regat) (n = 0 means 8) */
static void muld1once(UWORD n)
{
    pokewd((UWORD)(n << 9) | 0xe188 | regat);
}

/* blitz2.s:11106 muld1 - generate code for #d1.w*regat */
void muld1(UWORD d1)
{
    UWORD save = d1, d2 = 0, d4 = 0;
    WORD d3;

    for (d3 = 15; d3 >= 0; d3--) {  /* .shloop (dbf) */
        if (d1 & 0x8000) {
            d2++;
            d4 = d3;
        }
        d1 <<= 1;
    }
    if (d2 != 1) {
        /* .domul */
        pokewd((UWORD)(regat << 9) | 0xc0fc);
        pokewd(save);
        return;
    }
    d1 = d4;
    if (d1 == 0)
        return;
    if (d1 < 8) {
        muld1once(d1);
        return;
    }
    /* BUG(orig) (#15): d1 == 8 (x256) emits lsl #8 twice (count field 0 = 8),
       i.e. x65536.  Kept. */
    muld1once(d1 - 8);
    muld1once(0);
}

/* blitz2.s:11280 makesize - create code for #d1 into d0 */
static void makesize(ULONG d1)
{
    pokewd(0x203c);
    pokel(d1);
}

/* blitz2.s:11269 makessize */
static void makessize(void)
{
    makesize((ULONG)stringwork);
}

/* blitz2.s:11272 makevsize - create code for amount of variables in d0 */
static void makevsize(void)
{
    ULONG d1 = (UWORD)(varoff - 0x8000);

    if (d1 == 0)
        d1 = 8;
    makesize(d1);
}

/* blitz2.s:11288 findhilib - find highest lib# under *d7, set *d7 to this
   lib number.  asm: ne if found (a2 = lib), eq if not. */
static BOOL findhilib(UWORD *d7, struct blitzlib **a2)
{
    UWORD d1 = 0;
    struct blitzlib *a1;

    for (a1 = firstlib; a1; a1 = a1->next) {
        UWORD n = (UWORD)a1->libnum;
        if (*d7 <= n)
            continue;
        if (d1 >= n)
            continue;
        d1 = n;
        *a2 = a1;
    }
    *d7 = d1;
    return d1 != 0;
}

/* blitz2.s:11307 findlolib - find lowest lib# above *d7, set *d7 to this
   lib number.  asm: ne if found, eq if not. */
static BOOL findlolib(UWORD *d7, struct blitzlib **a2)
{
    UWORD d1 = 0xffff;
    struct blitzlib *a1;

    for (a1 = firstlib; a1; a1 = a1->next) {
        UWORD n = (UWORD)a1->libnum;
        if (*d7 >= n)
            continue;
        if (d1 <= n)
            continue;
        d1 = n;
        *a2 = a1;
    }
    *d7 = d1;
    return d1 != 0xffff;
}

/* blitz2.s:11140 makeinits - make any initialing jsr's */
void makeinits(void)
{
    ULONG start = pc;
    UWORD d7 = 0xffff, n;
    struct blitzlib *a2 = NULL;

    while (findhilib(&d7, &a2)) {
        if (!(a2->flags & LIBF_USED))
            continue;
        if (a2->initsub == 0)
            continue;
        n = (UWORD)a2->libnum;
        if (n == allocvars) {
            makevsize();
        } else if (n == debuglib) {
            /* pass flag in d0 : non zero=auto run */
            pokewd((optreq2ga1.Flags & GFLG_SELECTED) ? 0x70ff : 0x7000);
        } else if (n == 65235) {
            makessize();
        } else if (n == 65035) {
            /* .isexit */
            pokewd(0x203c);
            addoff();
            pokel(endop);
        } else if (n == 64535) {
            pokewd(0x203c);
            addoff();
            pokel(data2at);
        } else if (n == dhandlerlib) {
            pokewd(0x203c);         /* move.l #x,d0 (?) */
            pokel((ULONG)dseg);
        }
        /* .more */
        lasta6 = -1;
        makelibsub(a2, a2->initsub);
        if ((UWORD)a2->libnum == allocvars && intdata1) {
            pokewd(putidata1[0]);
            addoff();
            pokel(intdata1);
        }
        /* .notvar2 */
        if (a2->varkind == 0)
            continue;
        pokewd(a2->varkind < 0 ? 0x3b40 : 0x2b40);  /* move.w/.l d0,x(a5) */
        pokewd((UWORD)a2->varoff);
        if ((UWORD)a2->libnum != 65530)
            continue;
        if (numstatic) {
            datastart(staticdata, (UWORD)numstatic);
            tokejsr2(alstat);
        }
        /* .nostats */
        if (maxsused) {
            datastart(maxsat, (UWORD)maxsused);
            tokejsr2(setmaxs);
        }
    }
    /* .done: misc inits */
    if (pc == start) {
        noinits = (noinits & 0xffff0000) | 8;       /* move #8,noinits+2 */
        return;
    }
    /* .yi */
    noinits &= 0xffff0000;                          /* clr noinits+2 */
    if (!nomemleft) {
        W_(pcat) = 0x4eb9;                          /* jsr start.l */
        L_(pcat + 2) = start;
        addoff2(2);
    }
    pokewd(0x4e75);                                 /* rts */
}

/* blitz2.s:11263-11267 prepd1 / putidata1 / prepint */
UWORD prepd1[] = {
    0x222D, 0x0000,                    /* +0   prepd1 move.l 0(a5),d1 */
    0x23CD, 0x0000, 0x0000,            /* +4   putidata1 move.l a5,0 */
    0x247C, 0x0000, 0x0000,            /* +10  prepint move.l #0,a2 */
};

/* blitz2.s:11327 makefinits - create jsr's to FINIT routines from libs.
   also calculate the REGAT variables for inits! */
void makefinits(void)
{
    UWORD d7 = 0, n, size, sh;
    struct blitzlib *a2 = NULL;
    UBYTE *e, *a3;
    ULONG loop;

    varoff = (varoff + 1) & ~1;
    while (findlolib(&d7, &a2)) {
        if (!(a2->flags & LIBF_USED))
            continue;
        /* .skipvar */
        e = LIBMAX_E(a2);
        if (W_(e - 2)) {
            /* A max lib, possible multiple frees! */
            a3 = (UBYTE *)a2 + 6 + L_(e - 10);
            /* .plop: skip the reg pairs, 0 terminated */
            for (;;) {
                UWORD w = W_(a3);
                a3 += 2;
                if (w == 0)
                    break;
                a3 += 2;
            }
            if (L_(a3 + 4)) {
                /* Something to Do! */
                pokewd(freemax[0]);                 /* move.w #x,-(a7) */
                pokewd(W_(e - 4));                  /* x (max) */
                pokel(((ULONG)free_[0] << 16) | W_(e - 6)); /* move.l x(a5),a3 */
                loop = pc;
                sh = W_(e - 2) & 63;                /* lsl d2,d1 (mod 64) */
                size = sh < 16 ? (UWORD)(1 << sh) : 0;
                makelibsub(a2, L_(e - 10));
                pokewd(freemax3[0]);                /* lea x(a3),a3 */
                pokewd(size);
                pokel(tl(freemax2));                /* subq #1,(a7) / bne.w */
                pokewd((UWORD)(loop - pc));
                pokewd(freemax4[0]);                /* addq #2,a7 */
            }
        }
        /* .nomax */
        if (a2->finitsub == 0)
            continue;
        /* check special cases! */
        n = (UWORD)a2->libnum;
        if (n == allocvars)
            makevsize();
        else if (n == 65235)
            makessize();
        /* .more */
        lasta6 = -1;
        makelibsub(a2, a2->finitsub);
    }
}

/* blitz2.s:11407-11414 freemax templates */
UWORD freemax[] = {
    0x3F3C, 0x0000,                    /* +0   freemax move #0,-(a7) */
    0x5357,                            /* +4   freemax2 subq #1,(a7) */
    0x6600, 0xFFFC,                    /* +6   bne freemax2 */
    0x544F,                            /* +10  freemax4 addq #2,a7 */
    0x47EB, 0x0000,                    /* +12  freemax3 lea 0(a3),a3 */
};

/* blitz2.s:11416 chkstak */
void chkstak(void)
{
    if (DEBUGGA_B && !makeexec)
        tokejsr(stchk);
}

/* blitz2.s:11427 tokejsr2 (.skip) */
struct blitzlib *tokejsr2(UWORD token)
{
    /* (stchk skipping for executables commented out in the asm) */
    lasta6 = -1;
    return tokejsr(token);
}

/* blitz2.s:11434 tokejsr - make a jsr (or simple reg get) from d1 */
struct blitzlib *tokejsr(UWORD token)
{
    struct blitzlib *lib;
    UBYTE *sub = findtoke(token, &lib);

    return makelibsub(lib, (ULONG)(sub - (UBYTE *)lib));
}

/* blitz2.s:11464 libisat - lib of the call being generated */
static struct blitzlib *libisat;

static void makelsub3(UBYTE *a2);

/* blitz2.s:11444 userjsr - a2 = lib base, a1 = start of #parameters */
struct blitzlib *userjsr(struct blitzlib *lib, UBYTE *params)
{
    struct blitzlib *old = libisat, *r;
    UBYTE *a2;

    libisat = lib;
    a2 = params + 2 + (W_(params) & 255);
    a2 = (UBYTE *)(((ULONG)a2 + 1) & ~1UL);
    /* makelsub2 */
    makelsub3(a2);
    r = libisat;
    libisat = old;
    return r;
}

/* blitz2.s:11459 cutejsr */
struct blitzlib *cutejsr(struct blitzlib *lib, UBYTE *sub)
{
    struct blitzlib *old = libisat, *r;

    libisat = lib;
    makelsub3(sub);                 /* makelsub2 */
    r = libisat;
    libisat = old;
    return r;
}

/* blitz2.s:11466 makelibsub - lib in a2, sub offset in d1.
   blitz2.s:11471 makelsub2 (pops the caller-pushed libisat) is the
   save/restore around makelsub3 here and in userjsr/cutejsr. */
struct blitzlib *makelibsub(struct blitzlib *lib, ULONG off)
{
    struct blitzlib *old = libisat, *r;

    libisat = lib;
    makelsub3((UBYTE *)lib + 6 + off);
    r = libisat;
    libisat = old;
    return r;
}

static UBYTE *fetchregs(UBYTE *a2);
static void doajsrrout(ULONG d1);
static void doajsr(ULONG d1);
static void makebtst(UWORD tok);

/* blitz2.s:11474 makelsub3 */
static void makelsub3(UBYTE *a2)
{
    ULONG d1;

    a2 = fetchregs(a2);
    if (DEBUGGA_B) {
        d1 = L_(a2) | L_(a2 + 8);
        if (d1 & ~1UL) {
            d1 = L_(a2);
            a2 += 4;
            if (d1 & 1) {
                if (blitzmode >= 0)
                    blitzerr();
                if (DEBUGGA_B)
                    makebtst(inblitz);
                d1 &= ~1UL;         /* .skiptt2 */
            }
            /* .skiptt */
            doajsr(d1);
            d1 = L_(a2);
            a2 += 4;
            doajsrrout(d1);
            d1 = L_(a2);
            doajsr(d1);
            return;                 /* .more */
        }
    }
    /* .norerr */
    if (B_(a2 + 3) & 1) {
        /* Here, the routine can only run in Blitz Mode */
        if (blitzmode >= 0)
            blitzerr();
    }
    /* .penis */
    doajsrrout(L_(a2 + 4));
}

/* blitz2.s:11518 doajsrrout - Here, we suss out bit 0 to see if this can
   only happen in Amiga mode. */
static void doajsrrout(ULONG d1)
{
    if (d1 & 1) {
        if (blitzmode < 0)
            amigaerr();
        if (DEBUGGA_B)
            makebtst(inamiga);
        d1 &= ~1UL;                 /* .doajsr2 */
    }
    doajsr(d1);
}

static void doajsr2(ULONG d1);

/* blitz2.s:11535 doajsr - asm: Z flag from the caller's tst.l/move.l d1 */
static void doajsr(ULONG d1)
{
    if (d1)
        doajsr2(d1);
}

/* blitz2.s:11635 inline - inline code length (asm name: inline) */
static UWORD inline_;

/* blitz2.s:11541 doajsr2 - d1 is an offset from start of lib (libisat) */
static void doajsr2(ULONG d1)
{
    struct blitzlib *a0 = libisat;
    UBYTE *a1;
    UBYTE b;

    if (cfetchmode) {
        /* constant evaluation: jsr straight into the lib in memory */
        pokewd(0x4eb9);
        pokel((ULONG)a0 + d1);
        return;
    }
    /* .nfetch */
    a1 = (UBYTE *)a0 + d1;
    inline_ = 0;
    for (;;) {                      /* .cute */
        if (*a1++ != 0xa0)
            break;                  /* .ugly */
        b = *a1++;
        if (b != 1) {
            /* .iszero: here, code can be made inline. */
            inline_ = W_(a1);
            a1 += 2;
            d1 += 4;
            continue;
        }
        /* .isone: here, a blitzmode variation is available.
           Original: makebtst recurses into doajsr2, which clears the
           global inline length of this call (at worst jsr instead of inline). */
        if (blitzmode < 0) {
            if (DEBUGGA_B)
                makebtst(inblitz);
            /* .nobrerr */
            d1 = L_(a1);
            a1 = (UBYTE *)a0 + d1;
        } else {
            /* .inamiga */
            if (DEBUGGA_B)
                makebtst(inamiga);
            /* .noarerr */
            a1 += 4;
            d1 += 6;
        }
    }
    /* .ugly */
    if (inline_) {
        /* here, we copy the inline code. */
        a1--;
        pokecode2((const UWORD *)a1, (const UWORD *)(a1 + (WORD)inline_));
        return;
    }
    /* .notin */
    pokewd(0x4eb9);
    addoff();
    pokel(d1 - a0->maxinfo + a0->outaddr);
}

/* blitz2.s:11608 makebtst - emit tst.w blitzflag(a5) + jsr check tok */
static void makebtst(UWORD tok)
{
    pokewd(tstmode[0]);
    pokewd((UWORD)getbbase());
    tokejsr(tok);
}

/* blitz2.s:11619 getbbase - this return returns varoff for blitzmode flag
   in d1; it will create blitzoff if necessary */
WORD getbbase(void)
{
    if (blitzoff == -1) {
        varoff = (varoff + 1) & ~1;
        blitzoff = varoff;
        varoff += 2;
    }
    return blitzoff;
}

/* blitz2.s:11633 tstmode */
UWORD tstmode[] = {
    0x4A6D, 0x0000,                    /* +0   tstmode tst 0(a5) */
};

WORD blitzoff = -1;                 /* 11636 */
WORD blitzmode;                     /* 11637 +=amiga, -=blitz */

static void toreg(UWORD d1, UWORD d2);

/* blitz2.s:11639 fetchregs - (libnum.w, regspec.w) pairs, 0 terminated;
   returns the pointer past the terminator */
static UBYTE *fetchregs(UBYTE *a2)
{
    UWORD d1, d2;

    for (;;) {
        d1 = W_(a2);
        a2 += 2;
        if (d1 == 0)
            return a2;
        d2 = W_(a2);
        a2 += 2;
        toreg(d1, d2);
    }
}

/* blitz2.s:11803 .theretest - tst.l 0(a5) / bne.w .ttskip / moveq #1,d0 /
   trap #0 */
static UWORD theretest[] = {
    0x4AAD, 0x0000, 0x6600, 0x0006, 0x7001, 0x4E40
};

static UWORD makeregop(UWORD d2);

/* blitz2.s:11680 toreg .sendmd - move.w #(maxinfo word + add),reg */
static void sendmd(struct blitzlib *a1, UWORD d2, LONG off, UWORD add)
{
    UWORD d1 = 0x303c;              /* move.w #x,dn */

    d2 >>= 8;
    if (d2 & 0x80) {
        d1 = 0x307c;                /* move.w #x,an */
        /* original: bit 7 means An here, bit 4 in makeregop - so this
           a6 test can never be true (no library needs it). */
        if (d2 == 0x16)
            lasta6 = -1;
    }
    /* .skipad */
    d1 |= (d2 & 7) << 9;
    pokewd(d1);
    pokewd(W_((UBYTE *)a1 + a1->maxinfo + off) + add);
}

/* blitz2.s:11647 toreg - d1=lib num, d2.b=reg num */
static void toreg(UWORD d1, UWORD d2)
{
    struct blitzlib *a1;
    UBYTE *e;
    UWORD spec, dn, op;
    WORD sh;

    if (d2 < 0xff00) {
        if (d2 & 0x8000) {
            d2 &= ~0x8000;
            /* OK, blitz mode must be ON for us to bother doing this one */
            if (blitzmode >= 0)
                return;
        } else if (d2 & 0x4000) {
            /* .nobm */
            d2 &= ~0x4000;
            /* OK, must be in amiga mode */
            if (blitzmode < 0)
                return;
        }
    }
    /* .yeah */
    a1 = uselib(d1);
    switch ((UBYTE)d2) {
    case 0:
        goto norm;
    case 4:
        sendmd(a1, d2, -6, 4);
        return;
    case 3:
        /* O.K., we are going to send the current maximum settings. */
        sendmd(a1, d2, -4, 0);
        return;
    }

    /* .notnmax */
    spec = d2;
    if ((d2 >> 8) == 0x16)
        lasta6 = -1;
    /* .isnta6 */
    d1 = (UWORD)LIBMAX_VAROFF(a1);
    pokewd(makeregop(d2 >> 8));
    d2 = spec;
    if ((BYTE)d2 < 0) {
        /* <0 - the rest is a register spec.
           this reg is used to calc a max item (must be dn) */
        pokewd(d1);                 /* move.l x(a5),dn */
        e = LIBMAX_E(a1);
        d1 = d2 & 7;
        if (DEBUGGA_B) {
            pokewd((UWORD)(d1 << 9) | 0xb07c);      /* cmp #x,dn */
            pokewd(W_(e - 4));
            pokel(tl(maxchk));                      /* bcs */
            tokejsr(maxerr);
        }
        /* .norerr */
        sh = (WORD)W_(e - 2);       /* shift amount */
        while ((UWORD)sh >= 9) {    /* .shloop */
            pokewd(d1 | lsldn[0]);  /* lsl.w #8,dn (count field 0) */
            sh -= 8;
        }
        /* .simp */
        dn = d1;
        if (sh)
            pokewd(d1 | (UWORD)((sh & 7) << 9) | lsldn[0]);
        /* .simp2: Shifts are done, add'em to the opcode */
        op = (dn & 7) | maxdo[0];
        op |= (spec << 1) & 0xe00;
        pokewd(op);                 /* adda.w dn,an */
        return;
    }
    /* .skippy1 */
    if ((UBYTE)d2 != 1) {
        /* select used of a max block */
        d1 += 4;
        if (DEBUGGA_B) {
            pokewd(d1);
            pokel(((ULONG)theretest[0] << 16) | d1);
            pokel(tl(&theretest[2]));
            pokel(tl(&theretest[4]));
            return;
        }
    }
    /* .skippy: select base of a max block */
    pokewd(d1);
    return;

norm:
    d2 >>= 8;
    if ((UBYTE)d2 == 0x16) {
        if ((UWORD)a1->varoff == (UWORD)lasta6)
            return;                 /* .sk */
        lasta6 = a1->varoff;
    }
    /* .nota6 */
    op = makeregop(d2);
    if (a1->varkind < 0) {
        if (op == stackop[0])
            stackuse -= 2;
        op |= 0x1000;               /* .notst: move.w */
    }
    /* .long */
    pokewd(op);
    pokewd((UWORD)a1->varoff);
}

/* blitz2.s:11829-11839 lsldn / maxchk / maxdo / stackopuse / stackop */
UWORD lsldn[] = {
    0xE148,                            /* +0   lsldn lsl #8,d0 */
    0x6500, 0x0008,                    /* +2   maxchk bcs .ok */
    0x4EB9, 0x0000, 0x0000,            /* +6   jsr 0 */
    0xD0C0,                            /* +12  maxdo add d0,a0 */
    0x0000,                            /* +14  stackopuse dc 0 */
    0x2F2D, 0x0000,                    /* +16  stackop move.l 0(a5),-(a7) */
};

/* blitz2.s:11841 makeregop - make move x(a5),dn/an based on d2 */
static UWORD makeregop(UWORD d2)
{
    UWORD t;

    if ((UBYTE)d2 == 0xff) {
        stackuse += 4;
        return stackop[0];
    }
    /* .notst */
    t = (d2 & 0x10) ? regtransa[0] : regtransd[0];
    return (UWORD)((d2 & 7) << 9) | t;
}

/* blitz2.s:11861 ifchar - return eq=1 if <,=,> */
BOOL ifchar(UWORD *c)
{
    if (*c < 60 || *c > 62)
        return FALSE;
    *c -= 60;
    return TRUE;
}

/* ===================================================================
 * constant evaluation
 * =================================================================== */

/* blitz2.s:11871 evalconst */
ULONG evalconst(UWORD *c)
{
    bakup();
    return evalconst2(c);
}

/* blitz2.s:11880 evalconst2 - evaluate a constant at a5...  get long
   into d3 (d6=0) or ffp into d3 (d6<>0) */
ULONG evalconst2(UWORD *c)
{
    UWORD s_regat = regat;
    ULONG s_pc, s_libat, a0;
    WORD s_nomemleft, s_lasta6, s_cfetchmode;
    struct m68kregs r;
    UWORD ch;

    regat = 0;
    s_pc = pc;
    s_libat = libat;
    s_nomemleft = nomemleft;
    s_lasta6 = lasta6;
    s_cfetchmode = cfetchmode;
    if (s_cfetchmode == 0) {
        oldlibat = libat;
        pc = constpcat;
    }
    /* .something: already cfetching! continue in cmode buff */
    a0 = pc;
    libat = constlibat;             /* for overflow */
    nomemleft = 0;
    lasta6 = -1;
    cfetchmode = -1;

    arreval();

    pokewd(0x2600 | regat);         /* move.l d0,d3 */
    pokewd(0x4e75);                 /* rts! */
    if (nomemleft)
        conmemerr();
    flushc();
    /* run the generated code.  DIFF: the asm jumped in with whatever the
       compiler had in its registers; we pass the ones that mean anything:
       d3/d6 (results), a5 = source pointer (as the asm), a6 = maths. */
    memset(&r, 0, sizeof r);
    r.d[2] = 3;
    r.d[3] = numval;
    r.d[6] = (ULONG)(LONG)numfloat;
    r.a[5] = (ULONG)srcptr;
    r.a[6] = (ULONG)MathBase;
    callregs((APTR)a0, &r);
    numval = r.d[3];
    numfloat = (WORD)r.d[6];

    cfetchmode = s_cfetchmode;
    lasta6 = s_lasta6;
    nomemleft = s_nomemleft;
    libat = s_libat;
    pc = s_pc;
    regat = s_regat;
    ch = reget();
    if (c)
        *c = ch;
    return numval;
}

/* blitz2.s:11881 evalconst3 (same address as evalconst2) */
ULONG evalconst3(UWORD *c)
{
    return evalconst2(c);
}

ULONG oldlibat;                     /* 11934 */

/* ===================================================================
 * includes
 * =================================================================== */

/* blitz2.s:11936 doincdir */
void doincdir(UWORD c)
{
    char *a0 = incdir;

    for (;;) {
        if (c != '"') {
            /* BUG(orig) (#16, #17): no length check, ':' is copied too.  Kept. */
            *a0++ = (char)c;
            if (c == 0)
                return;
            if (c & 0x8000)
                syntaxerr();
        }
        c = getchar3();             /* .skip */
    }
}

/* blitz2.s:11947 makeiname - create include name */
WORD makeiname(void)
{
    UBYTE *a1 = namebuff;
    char *a0 = incdir;
    WORD d2 = -1;
    UWORD c;

    /* BUG(orig) (#325): neither loop checks the length - incdir + name beyond
       127 characters run past namebuff (namebuff2, then what follows).
       Like doincdir the rest of the line (also after ':') is copied. */
    do {
        d2++;
    } while ((*a1++ = (UBYTE)*a0++) != 0);
    a1--;
    bakup();
    for (;;) {
        c = getchar3();
        if (c & 0x8000)
            syntaxerr();
        if ((UBYTE)c == '"')
            continue;
        *a1++ = (UBYTE)c;
        if ((UBYTE)c == 0)
            break;
        d2++;
    }
    if (d2 == 0)
        syntaxerr();
    return d2;
}

/* blitz2.s:12014 imakeend - remember the start of the following line */
static void imakeend(struct include *a2, UBYTE *a5)
{
    UBYTE *a0 = a5;

    while (*a0++)
        ;
    a2->ptr = a0;
    srcptr = a5;
}

/* blitz2.s:12020 closeinc - close the current include; returns TRUE if
   there are more includes (asm: bne readinc) */
static BOOL closeinc(struct include *a2)
{
    Close(a2->fh);
    FreeMem(a2->buf, INCLEN);
    firstinc = a2->next;
    FreeMem(a2, a2->len);
    return --numincs != 0;
}

/* blitz2.s:11972 readinc - do include next line.  a5=position in buffer */
BOOL readinc(void)
{
    struct include *a2;
    UBYTE *a5, *a0, *a1;
    LONG got;

    for (;;) {
        a2 = firstinc;
        a5 = a2->ptr;
        a0 = a2->buf + (WORD)a2->nbuf;      /* end of buffer! */
        for (a1 = a5; a1 < a0; ) {
            if (*a1++ == 0) {
                imakeend(a2, a5);
                return TRUE;
            }
        }
        /* blitz2.s:11988 readpend - o.k., no 0's past where we're at! */
        a1 = a2->buf;
        while (a5 < a0)
            *a1++ = *a5++;
        a2->nbuf = (UWORD)(a1 - a2->buf);    /* chars in there */
        got = Read(a2->fh, a1, (LONG)(UWORD)((INCLEN - 1) - a2->nbuf));
        if (got < 0)
            readerr();
        a2->nbuf += (UWORD)got;
        if (a2->nbuf == 0) {
            if (closeinc(a2))
                continue;           /* bne readinc */
            return FALSE;           /* asm: Z=1 */
        }
        a5 = a2->buf;
        a5[a2->nbuf] = 0;
        imakeend(a2, a5);
        return TRUE;
    }
}

/* blitz2.s:12037 freexincs */
void freexincs(void)
{
    struct xinclude *a2 = firstxinc, *a1;

    firstxinc = NULL;
    while (a2) {
        a1 = a2;
        a2 = a2->next;
        FreeMem(a1, a1->len);
    }
}

/* blitz2.s:12050 freeincs */
void freeincs(void)
{
    struct include *a2 = firstinc, *a1;

    firstinc = NULL;
    while (a2) {
        if (a2->fh)
            Close(a2->fh);
        /* .noclose */
        if (a2->buf)
            FreeMem(a2->buf, INCLEN);
        /* .skip */
        a1 = a2;
        a2 = a2->next;
        FreeMem(a1, a1->len);
    }
}

/* ===================================================================
 * Data
 * =================================================================== */

/* blitz2.s:12074 pokedata2l - poke a long into data2! */
void pokedata2l(ULONG d0)
{
    pokedata2((UWORD)(d0 >> 16));
    pokedata2((UWORD)d0);
}

/* blitz2.s:12081 pokedata2 - poke a word into data2 (data staements) */
void pokedata2(UWORD d0)
{
    if (data2 < allat)
        W_(data2) = d0;
    else
        nomemleft = -1;             /* .over */
    data2 += 2;
}

/* blitz2.s:12094 pokedata2b */
void pokedata2b(UBYTE d0)
{
    if (data2 < allat)
        B_(data2) = d0;
    else
        nomemleft = -1;             /* .over */
    data2 += 1;
}

/* ===================================================================
 * CNIF / CSIF
 * =================================================================== */

/* blitz2.s:12104 getsp - get a string parameter for conditional stuff
   into a0 (getsp2 = loop entry) */
void getsp(UWORD c, UBYTE *buf)
{
    if (c != '"')
        syntaxerr();
    for (;;) {
        c = getchar3();
        if ((UBYTE)c == '"')
            break;
        if (c == 0)
            syntaxerr();
        *buf++ = (UBYTE)c;
    }
    *buf = 0;
}

/* blitz2.s:12118 numcom - tst.l d3 */
int numcom(void)
{
    if ((LONG)numval < 0)
        return -1;
    return numval != 0;
}

/* blitz2.s:12121 strcom - compare strings. set flags accordingly */
int strcom(void)
{
    BYTE *a0 = (BYTE *)namebuff, *a1 = (BYTE *)namebuff2;
    BYTE d0;

    for (;;) {
        d0 = *a0++;
        if (d0 == 0)
            break;
        if (d0 != *a1)              /* cmp.b: signed byte compare */
            return d0 < *a1 ? -1 : 1;
        a1++;
    }
    /* .short */
    if (*a1 == 0)
        return 0;
    /* .lt: cmp #1,d0 with d0.b = 0 -> "less".  DIFF: the asm compared the
       whole word, whose high byte was left over from earlier code. */
    return -1;
}
