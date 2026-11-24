/*
 * libcall.c - chunk 07, blitz2.s 10684-12135:
 * number fetch, code push/pull, library call / init code generation,
 * compile-time constant evaluation, include reader, Data poking and the
 * CNIF/CSIF compare helpers.  See notes/survey-07.md and libcall.h.
 */
#include "blitz2.h"
#include "exprtype.h"
#include "exprtree.h"
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

/* asm d3/d6 of the number fetch.  They stay globals (file-local): the
   fetch routines set them at any depth of the recursive evaluator (eval ->
   getelement -> fetchnum...), and evalconst2 hands them as d3/d6 into the
   generated code and reads d6 back after it ran (the float flag of the
   constant), like the asm. */
static ULONG numval;    /* asm d3: value (int or FFP) */
static WORD numfloat;   /* asm d6: 0 int, != 0 float */

/* Fixed (#978): the integer constant constemit emitted last (move.w/.l
   #x,Dn from constat to constend; 0: none), for isbyteconst */
static ULONG constat, constend;
/* Fixed (#1179): its value (as emitted), for iswordconst */
static ULONG constval;

/* No asm counterpart (#1186, a port defect of the #1177 fix): a decimal
   literal with a point or an exponent that the byte sizing of consttype2
   made a whole number (litcut, set by consttype2; since the fix of #1188
   only a whole-valued one, 2.0 or 1e2), and where constemit emitted it
   last (litat..litend, 0: none) with its FFP value litffp and the whole
   number litint, for fraclitfloat */
static BOOL litcut;
static ULONG litat, litend, litffp, litint;

/* Fixed (#1070): the literal consttype2 sizes next is negated (litneg in
   libcall.h); consttype2 takes it and clears it */
BOOL litneg;


static ULONG fetchnum3(ULONG val, UWORD tsf, UWORD *type, BOOL data);
static ULONG consttype2(UWORD *type, ULONG val, BOOL data);
static ULONG fetchpoint2(UWORD *type, BOOL data, BOOL frac);
static void dothee(void);
static UWORD fetchdec(UWORD c, ULONG *divisorp);

/* ===================================================================
 * number fetch
 * =================================================================== */

/* blitz2.s:10701 fetchdata .datado - dispatch on the first char */
static ULONG datado(UWORD c, UWORD *type)
{
    if (c == '$')
        return fetchhex(type, TRUE);
    if (c == '%')
        return fetchbin(type, TRUE);
    /* .doit */
    if (c != '.')
        return fetchnum(c, type, TRUE);
    return fetchfrac(c, type, TRUE);
}

/* blitz2.s:10686 fetchdata - fetch number and update type if necessary! */
ULONG fetchdata(UWORD *type, ULONG *raw)
{
    UWORD c;
    ULONG val;

    /* (asm: dfetch = -1; C passes data = TRUE down the fetch routines) */
    c = getchar3();
    if (c != '-') {
        litneg = FALSE;
        val = datado(c, type);
        *raw = numval;
        return val;
    }
    c = getchar3();
    litneg = TRUE;                  /* Fixed (#1070) */
    val = datado(c, type);
    *raw = numval;
    if (*type < T_FLOAT)
        return (ULONG)-(LONG)val;
    /* .nfp: negate floating point data! */
    return val ^ 0x80;          /* bchg #7,d0 (FFP sign bit) */
}

/* blitz2.s:10709 fetchfrac */
ULONG fetchfrac(UWORD c, UWORD *type, BOOL data)
{
    (void)c;
    numval = 0;
    return fetchpoint2(type, data, TRUE);
}

/* blitz2.s:10712 fetchnum */
ULONG fetchnum(UWORD c, UWORD *type, BOOL data)
{
    ULONG divisor, val;
    UWORD tsf;

    numval = 0;
    c = fetchdec(c, &divisor);
    if (c == '.') {
        /* blitz2.s:10734 fetchpoint */
        if (numval >= 0x800000)
            overerr();
        return fetchpoint2(type, data, FALSE);
    }
    c |= 32;
    if (c != 'e')
        return fetchnum2(numval, type, data);
    if (numval >= 0x800000)
        overerr();
    numfloat = -1;
    numval = ffpcall(SPFlt, numval, 0, NULL);     /* int to ffp */
    dothee();
    c = reget();
    tsf = *type;
    val = consttype2(type, c, data);
    return fetchnum3(val, tsf, type, data);
}

/* blitz2.s:10737 fetchpoint2 - now a float.  in: numval = whole part;
   frac: the number began with the point (fetchfrac) */
static ULONG fetchpoint2(UWORD *type, BOOL data, BOOL frac)
{
    ULONG whole, divisor, val, t;
    UWORD c, tsf;

    numfloat = -1;
    whole = ffpcall(SPFlt, numval, 0, NULL);     /* save whole number */
    c = getchar3();
    if (!tstnum(c)) {
        /* Fixed (#10): "1." is the float 1, as "1.0"; the original wanted
           a digit after the point.  A point alone and a point before a
           letter or '_' ("4.w") stay syntax errors. */
        if (frac || tstalpha(c) || c == '_')
            syntaxerr();
        numval = whole;
        goto pskip2;
    }
    numval = 0;
    c = fetchdec(c, &divisor);      /* get int frac */
    if (numval == 0) {
        numval = whole;             /* .pskip */
    } else {
        if (numval >= 0x800000)
            overerr();
        divisor = ffpcall(SPFlt, divisor, 0, NULL); /* divisor to ffp */
        t = ffpcall(SPFlt, numval, 0, NULL);     /* to ffp */
        t = ffpcall(SPDiv, t, divisor, NULL);   /* num/divisor (eg .25=25/100) */
        numval = ffpcall(SPAdd, t, whole, NULL); /* add 'em */
    }
    /* Fixed (#11): the exponent is read after a fraction of value 0 too
       ("1.0e5"); the original's .pskip went past this check and left the
       'e' unread */
    c = reget();
    c |= 32;
    if (c == 'e')
        dothee();                   /* E Something! */
pskip2:
    c = reget();
    tsf = *type;
    val = consttype2(type, c, data);
    return fetchnum3(val, tsf, type, data);
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
    UWORD power;

    if (!tstnum(c))
        syntaxerr();
    power = c - 48;
    c = getchar3();
    if (tstnum(c)) {
        power = power * 10 + (UWORD)(c - 48);
        getchar3();
    }
    /* .skip: power=number E+10 */
    if (power > 18)
        overerr();
    return facts[power];
}

/* blitz2.s:10775 dothee - Suss stuff after the E (numval = FFP so far) */
static void dothee(void)
{
    UWORD c = getchar3();
    ULONG factor;

    if (c == '-') {
        c = getchar3();
        factor = getsign(c);
        /* d3=number, factor=divisor */
        numval = ffpcall(SPDiv, numval, factor, NULL);
        return;
    }
    if (c == '+')                   /* .pos */
        c = getchar3();
    /* .pos2: d3=number so far, factor=multiplier */
    factor = getsign(c);
    numval = ffpcall(SPMul, numval, factor, NULL);
}

/* blitz2.s:10833 fetchdec - numval = numval*10 + digits; *d5 = 10^digits
   (divisor for frac convert!).  Returns the next (non digit) char. */
static UWORD fetchdec(UWORD c, ULONG *divisorp)
{
    ULONG divisor = 1, twice;

    do {
        divisor = divisor * 10;     /* unchecked, as the asm */
        /* only the shifts are overflow checked (bcs overerr) */
        if (numval & 0x80000000)
            overerr();
        numval <<= 1;
        twice = numval;
        if (numval & 0x80000000)
            overerr();
        numval <<= 1;
        if (numval & 0x80000000)
            overerr();
        numval <<= 1;
        numval += twice;
        numval += (UWORD)(c - 48);
        c = getchar3();
    } while (tstnum(c));
    *divisorp = divisor;
    return c;
}

/* no asm counterpart; replaces blitz2.s:10860-10888 fetchnum3's .doit,
   .word and .long - the code of the constant's leaf l: move.w/move.l
   #value,D(regat).  It uses only the leaf and the emitter's state; it
   leaves the facts of the constant for the code after it (#978
   constat..constend and constval, #1186 litat..litend) and the code in
   l->start..l->end. */
static void constemit(struct exprnode *l)
{
    ULONG at = pc, val = l->value;

    /* Fixed (#978): remember where the last integer constant is */
    constat = constend = 0;
    if (!(l->lflags & ELF_POINT) && l->tafter != T_FLOAT)
        constat = at;
    constval = val;
    switch (l->tafter) {
    case T_BYTE:
    case T_WORD:
        /* .word */
        pokewd((UWORD)(regat << 9) | 0x303c);
        pokewd((UWORD)val);
        break;
    case T_LONG:
    case T_QUICK:
    case T_FLOAT:
        /* .long */
        pokewd((UWORD)(regat << 9) | 0x203c);
        pokel(val);
        break;
    default:
        mismatcherr();
    }
    if (constat)
        constend = pc;
    if (l->lflags & ELF_CUT) {      /* #1186 */
        litat = at;
        litend = pc;
        litffp = l->aux;
        litint = val;
    }
    l->start = at;
    l->end = pc;
}

/* blitz2.s:10856 fetchnum3 - return the value (Data), or end the
   constant's parse part: its leaf (the value, the type so far tsf it was
   read under, the type it is now and its facts) emitted at once
   (constemit) and released (the window of exprtree.h); returns the next
   char.  data: the asm flag dfetch (10854), set by fetchdata and cleared
   here; C passes it down the fetch routines. */
static ULONG fetchnum3(ULONG val, UWORD tsf, UWORD *type, BOOL data)
{
    struct exprnode *l;

    litat = 0;                      /* #1186 */
    if (data)
        return val;
    l = exprtree_new(EXN_LEAF);
    l->op = ELK_CONST;
    l->value = val;
    l->aux = numval;                /* the FFP value of a literal with a point */
    l->tbefore = tsf;
    l->own = (UBYTE)*type;
    l->tafter = *type;
    if (numfloat)
        l->lflags |= ELF_POINT;
    if (litcut)
        l->lflags |= ELF_CUT;
    if (!forthsp[-1].constgot)
        l->lflags |= ELF_FOLD;
    constemit(l);
    exprtree_release(l);
    return reget();
}

BOOL fraclitfloat(ULONG from, ULONG *to, UWORD reg, UWORD *type)
{
    ULONG end = litend, ffp = litffp;

    if (!litat || litat != from || *to > libat)
        return FALSE;
    if (litend - litat == 4 ? W_(from + 2) != (UWORD)litint
        : (W_(from + 2) != (UWORD)(litint >> 16)
           || W_(from + 4) != (UWORD)litint))
        return FALSE;               /* not the literal's code any more */
    for (; end < *to; end += 2) {
        if (W_(end) != (0x4440 | reg))  /* neg.w Dreg */
            return FALSE;
        if (ffp)
            ffp ^= 0x80;            /* the FFP sign bit */
    }
    if (end != *to)
        return FALSE;
    pc = from;
    pokewd((UWORD)(reg << 9) | 0x203c); /* move.l #ffp,Dreg */
    pokel(ffp);
    litat = constat = 0;
    *type = T_FLOAT;
    *to = pc;
    return TRUE;
}

void fraclitdrop(void)
{
    litat = 0;
}

BOOL isbyteconst(ULONG from, ULONG to)
{
    ULONG v;

    if (!constat || constat != from || constend != to || to > libat)
        return FALSE;
    if (to - from == 4)                         /* move.w #x,Dn: w */
        v = W_(from + 2);
    else {                                      /* move.l #x,Dn: l or q */
        v = ((ULONG)W_(from + 2) << 16) | W_(from + 4);
        if (!(v & 0xffff))
            v >>= 16;                           /* q: x.0 */
    }
    return v >= 128 && v <= 255;
}

BOOL islongconst(ULONG from, ULONG to)
{
    /* by the positions, not the code: the same in a pass whose code is
       not stored (nomemleft) */
    return constat && constat == from && constend == to && to - from == 6;
}

BOOL iswordconst(ULONG from, ULONG to)
{
    return islongconst(from, to) && constval <= 32767;
}

void byteconstext(ULONG at)
{
    if (W_(at) & 0x1000)                        /* move.w */
        W_(at + 2) |= 0xff00;
    else if (!W_(at + 4))                       /* q: integer part */
        W_(at + 2) |= 0xff00;
    else {                                      /* l */
        W_(at + 2) = 0xffff;
        W_(at + 4) |= 0xff00;
    }
}

/* blitz2.s:10892 makeint - *val = numval as integer.  asm: on SPFix
   overflow it drops its return address (addq #4,a7) and jumps to
   numdofloat, i.e. returns from consttype2 as a float.
   DIFF: returns FALSE instead, consttype2 then does numdofloat. */
static BOOL makeint(ULONG *val)
{
    UWORD ccr;

    *val = numval;
    if (numfloat == 0)
        return TRUE;                /* already integer */
    *val = ffpcall(SPFix, numval, 0, &ccr);
    if (ccr & CCR_V)
        return FALSE;
    return TRUE;
}

/* blitz2.s:10902 fetchnum2 - no frac */
ULONG fetchnum2(ULONG num, UWORD *type, BOOL data)
{
    ULONG val;
    UWORD tsf = *type;

    numval = num;
    numfloat = 0;
    /* d0 only matters on consttype2's float paths (numfloat != 0) */
    val = consttype2(type, lastchar, data);
    return fetchnum3(val, tsf, type, data);
}

/* blitz2.s:10906 constotype - fetch a constant of type d2, return new
   type in d2! */
ULONG constotype(UWORD *type)
{
    UWORD c;

    evalconst(&c, NULL);
    return consttype2(type, c, FALSE);
}

/* no asm counterpart: the size of a whole number (exprtype_constfacts) */
static UBYTE constsize(ULONG v)
{
    if ((LONG)v <= 127 && (LONG)v >= -128)
        return T_BYTE;
    if ((LONG)v <= 32767 && (LONG)v >= -32768)
        return T_WORD;
    return T_LONG;
}

/* blitz2.s:10913 consttype2 .quick2 - the decimal literal numval (FFP)
   with the whole part val (makeint) as a Quick in *q; FALSE when it does
   not fit (numdofloat).  neg: the literal is negated (#1070).  (a6 = maths
   assumed, as the asm) */
static BOOL quickconst(ULONG val, BOOL neg, ULONG *q)
{
    UWORD whole;
    ULONG t, frac;

    t = ffpcall(SPFlt, val, 0, NULL);                   /* back to ffp */
    frac = ffpcall(SPSub, numval, t, NULL);             /* d0=fraction! */
    /* Fixed (#1070): a decimal literal is read as an FFP value
       (fetchpoint2: 24 bits of mantissa), so a Quick literal close to
       32768 is rounded to 32768.0 first.  The original found its integer
       part 32768 out of range, kept the Float 32768.0, and the conversion
       to the Quick of the assignment wrapped it to -32768
       ("q=32767.9999" gave -32768).  The value 32768.0 is now the nearest
       Quick that fits: 32767.99998 ($7FFFFFFF), or -32768 ($80000000,
       which the negation keeps) for a negated literal ("q=-32768.00001"
       stays -32768, "Data.q -32768.0" is no bad data any more).  No other
       value changes: no literal below 32768 rounds above 32768.0.  In
       brackets the literal is not negated itself: "-(32767.9999)" is the
       negation of 32767.99998, -32767.99998 ($80000001). */
    if (val == 32768 && frac == 0) {
        *q = neg ? 0x80000000 : 0x7fffffff;
        return TRUE;
    }
    /* Fixed (#13): the integer part is rounded down (floor), so that the
       fraction is 0..65535 for a negative value too, and the range check
       is made on it; a fraction of 0 is the Quick integer (.qint), and the
       fraction is compared signed.  The original took a zero fraction
       (beq) and a negative one (unsigned bcc) for one that does not fit:
       "q=1.0" became a float constant and a conversion. */
    if (frac & 0x80) {                                  /* FFP sign bit */
        val--;
        frac = ffpcall(SPAdd, frac, 0x80000041, NULL);  /* + 1.0 */
    }
    if ((LONG)val > 32767 || (LONG)val < -32768)
        return FALSE;
    whole = (UWORD)val;                                 /* save whole part */
    frac = ffpcall(SPMul, frac, 0x80000051, NULL);      /* *65536 */
    frac = ffpcall(SPFix, frac, 0, NULL);               /* back to int */
    if (frac == 0) {
        *q = (val << 16) | (val >> 16);                 /* .qint: swap d0 */
        return TRUE;
    }
    if ((LONG)frac < 0 || (LONG)frac >= 65536)
        return FALSE;                                   /* .knob */
    *q = ((ULONG)whole << 16) | (UWORD)frac;
    return TRUE;
}

/* blitz2.s:10913 consttype2 - size the constant numval (int if numfloat == 0,
   else FFP) to *type (0 = decide).  Returns the value (asm d0); *type =
   final type.  `val` = the asm d0 on entry (normally the current char);
   every path that uses it loads it with makeint first (#12), so it is not
   read.  data: the constant is a Data value (fetchdata; asm dfetch), for
   the Byte sizing (#1188).  The type is exprtype_element's (exprtype.c)
   from the constant's facts; the facts of a literal with a point are
   computed where the rule of the type so far reads them (under a Byte and
   a Quick: makeint, the fraction), with the FFP calls of the original's
   paths. */
static ULONG consttype2(UWORD *type, ULONG val, BOOL data)
{
    UWORD tsf = *type;
    struct exprtype_constfacts k = { 0 };
    struct exprtype_elem e;
    ULONG whole = numval, q = 0;
    BOOL neg = litneg;

    (void)val;
    litneg = FALSE;
    litcut = FALSE;                 /* #1186 */

    k.point = numfloat != 0;
    k.data = data;
    if (!k.point)
        k.size = constsize(numval);
    else if (tsf == T_BYTE || tsf == T_QUICK) {
        k.fix = makeint(&whole);
        if (k.fix && tsf == T_BYTE) {
            k.size = constsize(whole);
            /* Fixed (#1188): whether the literal is the whole number (a
               Data value keeps the cut: not asked) */
            k.whole = !data
                      && !ffpcall(SPSub, numval, ffpcall(SPFlt, whole, 0, NULL), NULL);
        } else if (k.fix)
            k.qfit = quickconst(whole, neg, &q);
    }
    e = exprtype_element(tsf, 0, &k);
    switch (e.result) {
    case 0:
        mismatcherr();
    case T_FLOAT:
        /* blitz2.s:11011 numdofloat; under a Float the whole number to
           FFP first (float_) */
        if (!k.point)
            numval = ffpcall(SPFlt, numval, 0, NULL);   /* int to float */
        val = numval;
        break;
    case T_QUICK:
        /* .quick2, or .qint: swap d0.  BUG(orig) (issue to be filed): the
           swap is a Quick of the whole number only for 0 to 32767; a
           negative one keeps $FFFF in the half that becomes the
           fraction, so On (-1) is $FFFFFFFF and #c=-3 is $FFFDFFFF
           (test compiler-quick-negative-const).  A literal is not
           affected: its minus sign is a neg of the positive Quick. */
        val = k.point ? q : (numval << 16) | (numval >> 16);
        break;
    default:
        /* Byte, Word, Long: the whole number (Fixed (#978): a constant
           from 128 to 255 becomes a positive word ($0090), here as in the
           original; compared with a byte value, which varcont
           sign-extends ($FF90), operate makes it the sign-extended byte:
           isbyteconst, byteconstext; every other byte context (a.b=200)
           keeps it.  Data reads the '-' of a value itself (fetchdata), so
           "Data.q -32768" sizes the integer 32768 under a Quick and leaves
           as a long; dodata takes it as the Quick -32768 (Fixed (#1074),
           tokens1.c).) */
        val = k.point ? whole : numval;
        break;
    }
    litcut = e.litcut;              /* #1186: a whole-valued literal */
    *type = e.result;
    return val;
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
    ULONG dst, src;
    UWORD n;

    pc = pushpc;                    /* New PC */
    pushclen = (UWORD)len;
    pushat = 0;                     /* no save buffer (pushput, #14) */
    if (pushclen == 0 || nomemleft)
        return;
    pushat = (ULONG)doallocerr(len, MEMF_PUBLIC);
    dst = pushat;                   /* destination */
    src = pushpc;                   /* source */
    pc = src;
    n = (pushclen >> 1) - 1;        /* to word dbf */
    do {
        W_(dst) = W_(src);
        dst += 2;
        src += 2;
    } while (n-- != 0);
    pushdooff = firstoff;
}

/* blitz2.s:11057 pushput - replace pulled out code! */
void pushput(void)
{
    ULONG len = pushclen, putpc, src, dst, delta;
    struct offnode *o;
    UWORD n;

    if (len == 0)
        return;
    putpc = pc;                     /* pc before put */
    pc += len;
    /* Fixed (#14): both exits of a full object buffer free the save
       buffer if pushdo made one (blitz2.s returns past the FreeMem) */
    if (nomemleft)
        goto skip2;
    n = (UWORD)((len >> 1) - 1);
    dst = putpc;
    src = pushat;
    do {
        if (dst >= libat) {
            nomemleft = -1;
            goto skip2;
        }
        W_(dst) = W_(src);
        dst += 2;
        src += 2;
    } while (n-- != 0);
    /* OK, now all offsets between pushdooff and pushoff must be adjusted
       by putpc-pushpc */
    delta = putpc - pushpc;
    if (delta) {
        for (o = pushdooff; o != pushoff; o = o->next) {
            /* address in prog.; leave offs >= lib */
            if (o->offset + pcat < libat)
                o->offset += delta;
        }
        /* Whew! */
    }
skip2:
    if (pushat)
        FreeMem((APTR)pushat, pushclen);
    pushat = 0;
}

/* Fixed (#994): a compile error between pushdo and pushput leaves the
   compiler past pushput (blitz2.s:11057, the only FreeMem of the save
   buffer): qfree frees it at the end of every pass */
void freepush(void)
{
    if (pushat)
        FreeMem((APTR)pushat, pushclen);
    pushat = 0;
}

/* ===================================================================
 * code generation
 * =================================================================== */

/* blitz2.s:11126 muld1 .once - lsl.l #n,D(regat) (n = 0 means 8) */
static void muld1once(UWORD n)
{
    pokewd((UWORD)(n << 9) | 0xe188 | regat);
}

/* blitz2.s:11106 muld1 - generate code for #mul.w*regat */
void muld1(UWORD mul)
{
    UWORD bits = 0, shift = 0;
    WORD bit;

    for (bit = 15; bit >= 0; bit--) { /* .shloop (dbf) */
        if (mul & (1U << bit)) {
            bits++;
            shift = bit;
        }
    }
    if (bits != 1) {
        /* .domul */
        pokewd((UWORD)(regat << 9) | 0xc0fc);
        pokewd(mul);
        return;
    }
    if (shift == 0)
        return;
    if (shift < 8) {
        muld1once(shift);
        return;
    }
    /* Fixed (#15): the original went on into a second lsl #8 after the
       first shift also for shift == 8 (x256 became x65536); the shift
       by shift - 8 is made only when it is not 0 */
    if (shift > 8)
        muld1once(shift - 8);
    muld1once(0);
}

/* blitz2.s:11280 makesize - create code for #size into d0 */
static void makesize(ULONG size)
{
    pokewd(0x203c);
    pokel(size);
}

/* blitz2.s:11269 makessize */
static void makessize(void)
{
    makesize((ULONG)stringwork);
}

/* blitz2.s:11272 makevsize - create code for amount of variables in d0 */
static void makevsize(void)
{
    ULONG size = (UWORD)(varoff - 0x8000);

    if (size == 0)
        size = 8;
    makesize(size);
}

/* blitz2.s:11288 findhilib - find highest lib# under *libnum, set *libnum to
   this bl number.  asm: ne if found (a2 = bl), eq if not. */
static BOOL findhilib(UWORD *libnum, struct blitzlib **found)
{
    UWORD best = 0;
    struct blitzlib *bl;

    for (bl = firstlib; bl; bl = bl->next) {
        UWORD n = (UWORD)bl->libnum;
        if (*libnum <= n)
            continue;
        if (best >= n)
            continue;
        best = n;
        *found = bl;
    }
    *libnum = best;
    return best != 0;
}

/* blitz2.s:11307 findlolib - find lowest lib# above *libnum, set *libnum to
   this bl number.  asm: ne if found, eq if not. */
static BOOL findlolib(UWORD *libnum, struct blitzlib **found)
{
    UWORD best = 0xffff;
    struct blitzlib *bl;

    for (bl = firstlib; bl; bl = bl->next) {
        UWORD n = (UWORD)bl->libnum;
        if (*libnum >= n)
            continue;
        if (best <= n)
            continue;
        best = n;
        *found = bl;
    }
    *libnum = best;
    return best != 0xffff;
}

/* blitz2.s:11140 makeinits - make any initialing jsr's */
void makeinits(void)
{
    ULONG start = pc;
    UWORD libnum = 0xffff, n;
    struct blitzlib *bl = NULL;

    while (findhilib(&libnum, &bl)) {
        if (!(bl->flags & LIBF_USED))
            continue;
        if (bl->initsub == 0)
            continue;
        n = (UWORD)bl->libnum;
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
        makelibsub(bl, bl->initsub);
        if ((UWORD)bl->libnum == allocvars && intdata1) {
            pokewd(putidata1[0]);
            addoff();
            pokel(intdata1);
        }
        /* .notvar2 */
        if (bl->varkind == 0)
            continue;
        pokewd(bl->varkind < 0 ? 0x3b40 : 0x2b40); /* move.w/.l d0,x(a5) */
        pokewd((UWORD)bl->varoff);
        if ((UWORD)bl->libnum != 65530)
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
    UWORD libnum = 0, n, size, sh;
    struct blitzlib *bl = NULL;
    UBYTE *e, *regs;
    ULONG loop;

    varoff = (varoff + 1) & ~1;
    while (findlolib(&libnum, &bl)) {
        if (!(bl->flags & LIBF_USED))
            continue;
        /* .skipvar */
        e = LIBMAX_E(bl);
        if (W_(e - 2)) {
            /* A max lib, possible multiple frees! */
            regs = (UBYTE *)bl + 6 + L_(e - 10);
            /* .plop: skip the reg pairs, 0 terminated */
            for (;;) {
                UWORD w = W_(regs);
                regs += 2;
                if (w == 0)
                    break;
                regs += 2;
            }
            if (L_(regs + 4)) {
                /* Something to Do! */
                pokewd(freemax[0]);                 /* move.w #x,-(a7) */
                pokewd(W_(e - 4));                  /* x (max) */
                pokel(((ULONG)free_[0] << 16) | W_(e - 6)); /* move.l x(a5),a3 */
                loop = pc;
                sh = W_(e - 2) & 63;                /* lsl d2,d1 (mod 64) */
                size = sh < 16 ? (UWORD)(1 << sh) : 0;
                makelibsub(bl, L_(e - 10));
                pokewd(freemax3[0]);                /* lea x(a3),a3 */
                pokewd(size);
                pokel(tl(freemax2));                /* subq #1,(a7) / bne.w */
                pokewd((UWORD)(loop - pc));
                pokewd(freemax4[0]);                /* addq #2,a7 */
            }
        }
        /* .nomax */
        if (bl->finitsub == 0)
            continue;
        /* check special cases! */
        n = (UWORD)bl->libnum;
        if (n == allocvars)
            makevsize();
        else if (n == 65235)
            makessize();
        /* .more */
        lasta6 = -1;
        makelibsub(bl, bl->finitsub);
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
    struct blitzlib *bl;
    UBYTE *sub = findtoke(token, &bl);

    return makelibsub(bl, (ULONG)(sub - (UBYTE *)bl));
}

/* blitz2.s:11464 libisat - lib of the call being generated.  DIFF: not a
   global any more; makelibsub/userjsr/cutejsr pass it down to doajsr2 as
   the parameter `bl` (the asm pushed and restored the global around
   every nested call, so the value seen is the same). */

static void makelsub3(struct blitzlib *bl, UBYTE *sub);

/* blitz2.s:11444 userjsr - a2 = lib base, a1 = start of #parameters */
struct blitzlib *userjsr(struct blitzlib *bl, UBYTE *params)
{
    UBYTE *sub;

    sub = params + 2 + (W_(params) & 255);
    sub = (UBYTE *)(((ULONG)sub + 1) & ~1UL);
    /* makelsub2 */
    makelsub3(bl, sub);
    return bl;
}

/* blitz2.s:11459 cutejsr */
struct blitzlib *cutejsr(struct blitzlib *bl, UBYTE *sub)
{
    makelsub3(bl, sub);             /* makelsub2 */
    return bl;
}

/* blitz2.s:11466 makelibsub - lib in a2, sub offset in d1.
   blitz2.s:11471 makelsub2 (pops the caller-pushed libisat) is the
   save/restore around makelsub3 here and in userjsr/cutejsr. */
struct blitzlib *makelibsub(struct blitzlib *bl, ULONG off)
{
    makelsub3(bl, (UBYTE *)bl + 6 + off);
    return bl;
}

static UBYTE *fetchregs(UBYTE *p);
static void doajsrrout(struct blitzlib *bl, ULONG off);
static void doajsr(struct blitzlib *bl, ULONG off);
static void makebtst(UWORD tok);

/* blitz2.s:11474 makelsub3 */
static void makelsub3(struct blitzlib *bl, UBYTE *sub)
{
    ULONG before, rout, after;

    sub = fetchregs(sub);
    if (DEBUGGA_B) {
        if ((L_(sub) | L_(sub + 8)) & ~1UL) {
            before = L_(sub);       /* error check before the routine */
            sub += 4;
            if (before & 1) {
                if (blitzmode >= 0)
                    blitzerr();
                if (DEBUGGA_B)
                    makebtst(inblitz);
                before &= ~1UL;     /* .skiptt2 */
            }
            /* .skiptt */
            doajsr(bl, before);
            rout = L_(sub);
            sub += 4;
            doajsrrout(bl, rout);
            after = L_(sub);        /* error check after the routine */
            doajsr(bl, after);
            return;                 /* .more */
        }
    }
    /* .norerr */
    if (B_(sub + 3) & 1) {
        /* Here, the routine can only run in Blitz Mode */
        if (blitzmode >= 0)
            blitzerr();
    }
    /* .penis */
    doajsrrout(bl, L_(sub + 4));
}

/* blitz2.s:11518 doajsrrout - Here, we suss out bit 0 to see if this can
   only happen in Amiga mode. */
static void doajsrrout(struct blitzlib *bl, ULONG off)
{
    if (off & 1) {
        if (blitzmode < 0)
            amigaerr();
        if (DEBUGGA_B)
            makebtst(inamiga);
        off &= ~1UL;                /* .doajsr2 */
    }
    doajsr(bl, off);
}

static void doajsr2(struct blitzlib *bl, ULONG off);

/* blitz2.s:11535 doajsr - asm: Z flag from the caller's tst.l/move.l d1 */
static void doajsr(struct blitzlib *bl, ULONG off)
{
    if (off)
        doajsr2(bl, off);
}

/* blitz2.s:11635 inline - inline code length (asm name: inline).  Kept
   global on purpose: a nested makebtst (below) recurses into doajsr2 and
   clears it, as in the original (a local would change the generated
   code in debug mode). */
static UWORD inline_;

/* blitz2.s:11541 doajsr2 - d1 is an offset from start of lib (libisat) */
static void doajsr2(struct blitzlib *bl, ULONG off)
{
    UBYTE *p;
    UBYTE b;

    if (cfetchmode) {
        /* constant evaluation: jsr straight into the lib in memory */
        pokewd(0x4eb9);
        pokel((ULONG)bl + off);
        return;
    }
    /* .nfetch */
    p = (UBYTE *)bl + off;
    inline_ = 0;
    for (;;) {                      /* .cute */
        if (*p++ != 0xa0)
            break;                  /* .ugly */
        b = *p++;
        if (b != 1) {
            /* .iszero: here, code can be made inline. */
            inline_ = W_(p);
            p += 2;
            off += 4;
            continue;
        }
        /* .isone: here, a blitzmode variation is available.
           Original: makebtst recurses into doajsr2, which clears the
           global inline length of this call (at worst jsr instead of inline). */
        if (blitzmode < 0) {
            if (DEBUGGA_B)
                makebtst(inblitz);
            /* .nobrerr */
            off = L_(p);
            p = (UBYTE *)bl + off;
        } else {
            /* .inamiga */
            if (DEBUGGA_B)
                makebtst(inamiga);
            /* .noarerr */
            p += 4;
            off += 6;
        }
    }
    /* .ugly */
    if (inline_) {
        /* here, we copy the inline code. */
        p--;
        pokecode2((const UWORD *)p, (const UWORD *)(p + (WORD)inline_));
        return;
    }
    /* .notin */
    pokewd(0x4eb9);
    addoff();
    pokel(off - bl->maxinfo + bl->outaddr);
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

static void toreg(UWORD libnum, UWORD regspec);

/* blitz2.s:11639 fetchregs - (libnum.w, regspec.w) pairs, 0 terminated;
   returns the pointer past the terminator */
static UBYTE *fetchregs(UBYTE *p)
{
    UWORD libnum, regspec;

    for (;;) {
        libnum = W_(p);
        p += 2;
        if (libnum == 0)
            return p;
        regspec = W_(p);
        p += 2;
        toreg(libnum, regspec);
    }
}

/* blitz2.s:11803 .theretest - tst.l 0(a5) / bne.w .ttskip / moveq #1,d0 /
   trap #0 */
static UWORD theretest[] = {
    0x4AAD, 0x0000, 0x6600, 0x0006, 0x7001, 0x4E40
};

static UWORD makeregop(UWORD reg);

/* blitz2.s:11680 toreg .sendmd - move.w #(maxinfo word + add),reg */
static void sendmd(struct blitzlib *bl, UWORD regspec, LONG off, UWORD add)
{
    UWORD reg = regspec >> 8;
    UWORD op = 0x303c;              /* move.w #x,dn */

    if (reg & 0x80) {
        op = 0x307c;                /* move.w #x,an */
        /* original: bit 7 means An here, bit 4 in makeregop - so this
           a6 test can never be true (no library needs it). */
        if (reg == 0x16)
            lasta6 = -1;
    }
    /* .skipad */
    op |= (reg & 7) << 9;
    pokewd(op);
    pokewd(W_((UBYTE *)bl + bl->maxinfo + off) + add);
}

/* blitz2.s:11647 toreg - libnum=lib num, regspec.b=reg num */
static void toreg(UWORD libnum, UWORD regspec)
{
    struct blitzlib *bl;
    UBYTE *e;
    UWORD reg, maxoff, dn, op;
    WORD sh;

    if (regspec < 0xff00) {
        if (regspec & 0x8000) {
            regspec &= ~0x8000;
            /* OK, blitz mode must be ON for us to bother doing this one */
            if (blitzmode >= 0)
                return;
        } else if (regspec & 0x4000) {
            /* .nobm */
            regspec &= ~0x4000;
            /* OK, must be in amiga mode */
            if (blitzmode < 0)
                return;
        }
    }
    /* .yeah */
    bl = uselib(libnum);
    reg = regspec >> 8;
    switch ((UBYTE)regspec) {
    case 0:
        goto norm;
    case 4:
        sendmd(bl, regspec, -6, 4);
        return;
    case 3:
        /* O.K., we are going to send the current maximum settings. */
        sendmd(bl, regspec, -4, 0);
        return;
    }

    /* .notnmax */
    if (reg == 0x16)
        lasta6 = -1;
    /* .isnta6 */
    maxoff = (UWORD)LIBMAX_VAROFF(bl);
    pokewd(makeregop(reg));
    if ((BYTE)regspec < 0) {
        /* <0 - the rest is a register spec.
           this reg is used to calc a max item (must be dn) */
        pokewd(maxoff);             /* move.l x(a5),dn */
        e = LIBMAX_E(bl);
        dn = regspec & 7;
        if (DEBUGGA_B) {
            pokewd((UWORD)(dn << 9) | 0xb07c);      /* cmp #x,dn */
            pokewd(W_(e - 4));
            pokel(tl(maxchk));                      /* bcs */
            tokejsr(maxerr);
        }
        /* .norerr */
        sh = (WORD)W_(e - 2);       /* shift amount */
        if (objlong(e)) {
            /* Fixed (#939): a long offset (see objlong, tokens1.c) */
            objlongcode(dn, (UWORD)sh);
            op = (dn & 7) | maxdo[0] | 0x100;
            op |= (regspec << 1) & 0xe00;
            pokewd(op);             /* adda.l dn,an */
            return;
        }
        while ((UWORD)sh >= 9) {    /* .shloop */
            pokewd(dn | lsldn[0]);  /* lsl.w #8,dn (count field 0) */
            sh -= 8;
        }
        /* .simp */
        if (sh)
            pokewd(dn | (UWORD)((sh & 7) << 9) | lsldn[0]);
        /* .simp2: Shifts are done, add'em to the opcode. */
        op = (dn & 7) | maxdo[0];
        op |= (regspec << 1) & 0xe00;
        pokewd(op);                 /* adda.w dn,an */
        return;
    }
    /* .skippy1 */
    if ((UBYTE)regspec != 1) {
        /* select used of a max block */
        maxoff += 4;
        if (DEBUGGA_B) {
            pokewd(maxoff);
            pokel(((ULONG)theretest[0] << 16) | maxoff);
            pokel(tl(&theretest[2]));
            pokel(tl(&theretest[4]));
            return;
        }
    }
    /* .skippy: select base of a max block */
    pokewd(maxoff);
    return;

norm:
    if ((UBYTE)reg == 0x16) {
        if ((UWORD)bl->varoff == (UWORD)lasta6)
            return;                 /* .sk */
        lasta6 = bl->varoff;
    }
    /* .nota6 */
    op = makeregop(reg);
    if (bl->varkind < 0) {
        if (op == stackop[0])
            stackuse -= 2;
        op |= 0x1000;               /* .notst: move.w */
    }
    /* .long */
    pokewd(op);
    pokewd((UWORD)bl->varoff);
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
static UWORD makeregop(UWORD reg)
{
    UWORD t;

    if ((UBYTE)reg == 0xff) {
        stackuse += 4;
        return stackop[0];
    }
    /* .notst */
    t = (reg & 0x10) ? regtransa[0] : regtransd[0];
    return (UWORD)((reg & 7) << 9) | t;
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
ULONG evalconst(UWORD *c, WORD *isfloat)
{
    bakup();
    return evalconst2(c, isfloat);
}

/* blitz2.s:11880 evalconst2 - evaluate a constant at a5...  get long
   into d3 (d6=0) or ffp into d3 (d6<>0) */
ULONG evalconst2(UWORD *c, WORD *isfloat)
{
    UWORD s_regat = regat;
    ULONG s_pc, s_libat, code;
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
    code = pc;
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
    callregs((APTR)code, &r);
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
    if (isfloat)
        *isfloat = numfloat;
    return numval;
}

/* blitz2.s:11881 evalconst3 (same address as evalconst2) */
ULONG evalconst3(UWORD *c, WORD *isfloat)
{
    return evalconst2(c, isfloat);
}

ULONG oldlibat;                     /* 11934 */

/* ===================================================================
 * includes
 * =================================================================== */

/* Fixed (#17): what may follow the closing quote of a quoted IncDir path
   or include name - the line end or a comment, and with colon a ':' or
   Else that ends the statement (doline goes on with it, lastchar).
   Anything else is a syntax error.  blitz2.s copies the rest of the line, also
   after ':', into the path or name. */
static void quoteend(BOOL colon)
{
    UWORD c = getchar3();

    if (c == 0 || (colon && eos(c)))   /* ':' or Else end the statement */
        return;
    syntaxerr();
}

/* blitz2.s:11936 doincdir */
void doincdir(UWORD c)
{
    char *dst = incdir;
    BOOL quoted = c == '"';

    for (;;) {
        if (c != '"') {
            /* Fixed (#16): a path that does not fit into incdir (95
               characters and the 0) is refused; blitz2.s writes on into
               the data behind it */
            if (c && dst == incdir + sizeof incdir - 1)
                pathlongerr();
            *dst++ = (char)c;
            if (c == 0)
                return;
            if (c & 0x8000)
                syntaxerr();
        } else if (quoted && !quoteflag) {
            *dst = 0;               /* Fixed (#17): the closing quote */
            quoteend(TRUE);
            return;
        }
        c = getchar3();             /* .skip */
    }
}

/* blitz2.s:11947 makeiname - create include name */
WORD makeiname(BOOL colon)
{
    UBYTE *dst = namebuff;
    char *src = incdir;
    WORD len = -1;
    WORD dirlen;
    BOOL device = FALSE;
    BOOL first = TRUE;
    BOOL quoted = FALSE;
    UWORD c;

    /* Fixed (#325): blitz2.s copies incdir and the name without a limit,
       beyond 127 characters past namebuff (into namebuff2, then what
       follows).  Here the copy stops at the end of namebuff2 (incdir has
       at most 95 characters, #16), and a name that is longer than 127
       characters without the incdir a device drops (#341) is "Path too
       Long".  Fixed (#17): a quoted name ends at its closing quote
       (quoteend); an unquoted one is the rest of the line, as in
       blitz2.s. */
    do {
        len++;
    } while ((*dst++ = (UBYTE)*src++) != 0);
    dst--;
    dirlen = len;
    bakup();
    for (;; first = FALSE) {
        c = getchar3();
        if (c & 0x8000)
            syntaxerr();
        if ((UBYTE)c == '"') {
            if (first)
                quoted = TRUE;
            else if (quoted && !quoteflag) {
                *dst = 0;
                quoteend(colon);
                break;
            }
            continue;
        }
        if ((UBYTE)c && dst == namebuff + sizeof namebuff - 1)
            pathlongerr();
        *dst++ = (UBYTE)c;
        if ((UBYTE)c == 0)
            break;
        if ((UBYTE)c == ':')
            device = TRUE;
        len++;
    }
    /* Fixed (#341): a name with a device or assign (':') is used as it
       is - blitz2.s puts incdir in front of every name (AB3 mode keeps
       that: ext_incdevice) */
    if (device && dirlen && ext_incdevice()) {
        memmove(namebuff, namebuff + dirlen, (size_t)(len - dirlen) + 1);
        len -= dirlen;
    }
    if (len > 127)
        pathlongerr();              /* Fixed (#325) */
    if (len == 0)
        syntaxerr();
    return len;
}

/* blitz2.s:12014 imakeend - remember the start of the following line */
static void imakeend(struct include *inc, UBYTE *line)
{
    UBYTE *p = line;

    while (*p++)
        ;
    inc->ptr = p;
    srcptr = line;
    ext_incline(inc);
}

/* blitz2.s:12020 closeinc - close the current include; returns TRUE if
   there are more includes (asm: bne readinc) */
static BOOL closeinc(struct include *inc)
{
    ext_incdone(inc);
    Close(inc->fh);
    FreeMem(inc->buf, INCLEN);
    firstinc = inc->next;
    FreeMem(inc, inc->len);
    return --numincs != 0;
}

/* blitz2.s:11972 readinc - do include next line.  a5=position in buffer */
BOOL readinc(void)
{
    struct include *inc;
    UBYTE *line, *end, *p, *src, *dst;
    LONG got;

    for (;;) {
        inc = firstinc;
        line = inc->ptr;
        end = inc->buf + (WORD)inc->nbuf;   /* end of buffer! */
        for (p = line; p < end; ) {
            if (*p++ == 0) {
                imakeend(inc, line);
                return TRUE;
            }
        }
        /* blitz2.s:11988 readpend - o.k., no 0's past where we're at! */
        dst = inc->buf;
        for (src = line; src < end; )
            *dst++ = *src++;
        inc->nbuf = (UWORD)(dst - inc->buf); /* chars in there */
        if (!ext_readinc(inc, dst, (LONG)(UWORD)((INCLEN - 1) - inc->nbuf), &got))
            got = Read(inc->fh, dst, (LONG)(UWORD)((INCLEN - 1) - inc->nbuf));
        if (got < 0)
            readerr();
        inc->nbuf += (UWORD)got;
        if (inc->nbuf == 0) {
            if (closeinc(inc))
                continue;           /* bne readinc */
            return FALSE;           /* asm: Z=1 */
        }
        line = inc->buf;
        line[inc->nbuf] = 0;
        imakeend(inc, line);
        return TRUE;
    }
}

/* blitz2.s:12037 freexincs */
void freexincs(void)
{
    struct xinclude *cur = firstxinc, *node;

    firstxinc = NULL;
    while (cur) {
        node = cur;
        cur = cur->next;
        FreeMem(node, node->len);
    }
}

/* blitz2.s:12050 freeincs */
void freeincs(void)
{
    struct include *cur = firstinc, *node;

    firstinc = NULL;
    while (cur) {
        ext_incdone(cur);
        if (cur->fh)
            Close(cur->fh);
        /* .noclose */
        if (cur->buf)
            FreeMem(cur->buf, INCLEN);
        /* .skip */
        node = cur;
        cur = cur->next;
        FreeMem(node, node->len);
    }
}

/* ===================================================================
 * Data
 * =================================================================== */

/* blitz2.s:12074 pokedata2l - poke a long into data2! */
void pokedata2l(ULONG l)
{
    pokedata2((UWORD)(l >> 16));
    pokedata2((UWORD)l);
}

/* blitz2.s:12081 pokedata2 - poke a word into data2 (data staements) */
void pokedata2(UWORD w)
{
    if (data2 < allat)
        W_(data2) = w;
    else
        nomemleft = -1;             /* .over */
    data2 += 2;
}

/* blitz2.s:12094 pokedata2b */
void pokedata2b(UBYTE b)
{
    if (data2 < allat)
        B_(data2) = b;
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
int numcom(ULONG value)
{
    if ((LONG)value < 0)
        return -1;
    return value != 0;
}

/* blitz2.s:12121 strcom - compare strings. set flags accordingly */
int strcom(void)
{
    BYTE *s1 = (BYTE *)namebuff, *s2 = (BYTE *)namebuff2;
    BYTE ch;

    for (;;) {
        ch = *s1++;
        if (ch == 0)
            break;
        if (ch != *s2)              /* cmp.b: signed byte compare */
            return ch < *s2 ? -1 : 1;
        s2++;
    }
    /* .short */
    if (*s2 == 0)
        return 0;
    /* .lt: cmp #1,d0 with d0.b = 0 -> "less".  DIFF: the asm compared the
       whole word, whose high byte was left over from earlier code. */
    return -1;
}
