/*
 * asmlibs.c - chunk 04 (blitz2.s 5000-6739): inline assembler back end,
 * library parameter fetching, library usage / token jsrs, library loading
 * and freeing, pokedata1*.  See notes/survey-04.md and asmlibs.h.
 */
#include <string.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "blitz2.h"

/* ================================================================ data */

UWORD lc = 0;                           /* blitz2.s:5026 */

/* blitz2.s:5617 ptype move #0,-(a7) (template) */
UWORD ptype[] = {
    0x3F3C, 0x0000,                     /* +0 ptype move #0,-(a7) */
};
/* blitz2.s:5618 pmode dc 0 (data word right behind ptype) */
UWORD pmode = 0;

/* blitz2.s:5818-5820 templates */
UWORD fbase[] = {
    0x202D, 0x0000,                     /* +0 fbase    move.l 0(a5),d0 */
    0xD097,                             /* +4 pushadd  add.l (a7),d0 */
    0xD080,                             /* +6 pushlast add.l d0,d0 */
};

struct tokejsrnode *tokeslist = NULL;   /* blitz2.s:6227 */
APTR deflibsat = NULL;                  /* blitz2.s:6318 */
APTR acidlibsat = NULL;                 /* blitz2.s:6319 */
struct node *blitzlibsat = NULL;        /* blitz2.s:6320 start of blitzlibs list! */
struct blitzlib *ffplib = NULL;         /* blitz2.s:6616 */

/* pmode high byte (asm `move.b x,pmode`) */
static inline void setpmodehi(UWORD b)
{
    pmode = (pmode & 0x00ff) | (UWORD)(b << 8);
}

/* ========================================================== assembler */

/* blitz2.s:5000 asmconst - store the text of the eval into asmfirst...
   text till '(','.' */
UWORD asmconst(void)
{
    UBYTE *a0 = asmbuff;
    UWORD d1 = numincs | macmask;
    UBYTE *saved;
    UWORD c;

    W_(a0) = d1;                        /* flag! */
    a0 += 2;
    if (d1)
        return copyconst(a0);           /* bne copyconst */
    saved = srcptr;                     /* move.l a5,-(a7) */
    c = copyconst(a0);
    a0 = asmbuff;
    if ((UWORD)asmlen2 <= 4 || lc) {
        /* .leave: flag becomes $ffff = direct text.  asm `addq #4,a7`
           only drops the saved a5. */
        W_(a0) = ~W_(a0);
        return c;
    }
    /* indirect: pointer into the (persistent) source text */
    L_(a0 + 2) = (ULONG)saved;
    B_(a0 + 6) = 0;
    asmlen2 = 4;                        /* pointer + 0 */
    return c;
}

/* blitz2.s:5028 copyconst */
UWORD copyconst(UBYTE *a0)
{
    UWORD d1 = 0;
    UWORD c;

    lc = 0;
    for (;;) {
        d1++;
        c = getchar3();
        if (eos(c))
            break;
        if (c & 0x8000) {               /* token: 2 bytes */
            *a0++ = (UBYTE)(c >> 8);
            *a0++ = (UBYTE)c;
            d1++;
            continue;
        }
        if (!quoteflag) {
            if (c == '(')
                break;
            /* ';cmp #'.',d0 / beq .done' commented out in the original */
            if (c == ',')
                break;
            if ((UBYTE)c == loclabch) {
                const char *a1 = lastbiglab;
                lc = c;
                if (!*a1)
                    nolocerr();
                do
                    d1++;
                while ((*a0++ = *a1++) != 0);
                d1--;
                a0--;
            }
        }
        *a0++ = (UBYTE)c;               /* .putbyte */
    }
    *a0 = 0;
    asmlen2 = d1;
    return c;
}

/* blitz2.s:5072 asmit - assemble one instruction (c = asm token) */
void asmit(UWORD c)
{
    UWORD idx;
    struct asment *e;
    UWORD sz, nib = 0;
    WORD d2 = 0, d3 = -1, d4 = 0, d5 = -1;
    UWORD d1;
    ULONG a1, a4;
    WORD f;

    alignpc();
    idx = (UWORD)(c - (0x8000 + fnum)) << 2;
    e = asmpnts[idx >> 2];
    /* move d0,temp1: word write = high half of the long */
    temp1 = (temp1 & 0xffff) | ((ULONG)idx << 16);

    /* @5080 size */
    c = getchar3();
    if (c != '.') {
        sz = 4;                         /* .dsize: no size check */
    } else {
        c = getchar3() | 32;
        if (c == 'w') {
            sz = 4;
            nib = e->size >> 4;
        } else if (c == 'b') {
            sz = 0;
            nib = e->size;
        } else if (c == 'l') {
            sz = 8;
            nib = e->size >> 8;
        } else
            syntaxerr();
        if ((nib & 15) == 15)
            illsizeerr();
        getchar3();
    }
    asmsize = sz;

    /* @5110 operands */
    c = reget();
    if (!eos(c)) {
        UWORD m, r;
        asmbuff = namebuff;
        bakup();
        c = getea(&m, &r);
        d2 = r; d3 = m;                 /* to src */
        d4 = r;                         /* asm: d4 keeps the src reg */
        asmlen = asmlen2;
        d5 = -1;
        extraword2 = extraword;
        if (c == ',') {
            asmbuff = namebuff2;
            getea(&m, &r);              /* dest */
            d4 = r; d5 = m;
        }
    }

    /* @5129 all stuff fetched!  O.K. - Let's assemble it! */
    a1 = pc;
    a4 = a1;
    a1 += 2;
    d1 = e->opcode;
    if (d3 < 0) {                       /* .none: no src/dest ea! */
        if (e->srcmask | e->destmask)
            illeaerr();
        goto putit;
    }

    /* @5141 make into movea, addi etc. if possible */
    f = e->flag;
    if (f < 0) {
        UWORD ni = 0;
        UWORD t = (UWORD)(temp1 >> 16);
        if ((f & 1) && d5 == 1) {
            if (d3 != 14)
                ni = t + 8;
        } else if ((f & 2) && d3 == 11)
            ni = t + 4;
        if (ni) {
            e = asmpnts[ni >> 2];
            d1 = e->opcode;
        }
    }

    /* @5164 .noia */
    buff1 = namebuff;
    buff2 = namebuff2;
    if (e->chk) {
        struct asmregs r;
        WORD res;
        r.opcode = d1; r.srcreg = d2; r.srcmode = d3; r.destreg = d4; r.destmode = d5;
        r.size = asmsize;
        res = e->chk(&r);
        d1 = r.opcode; d2 = r.srcreg; d3 = r.srcmode; d4 = r.destreg; d5 = r.destmode;
        if (res < 0) {
            if (res == -2)
                illsizeerr();
            illeaerr();
        }
        if (res > 0) {
            WORD t; UWORD w;
            t = d2; d2 = d4; d4 = t;
            t = d3; d3 = d5; d5 = t;
            w = extraword; extraword = extraword2; extraword2 = w;  /* swap d6 */
            buff1 = namebuff2;
            buff2 = namebuff;
            t = asmlen; asmlen = asmlen2; asmlen2 = t;
        }
    }

    /* @5191 .nojsr */
    asmbuff = buff1;
    f = e->flag;
    if (f <= 0)
        goto nothing;
    if (f == 1) {
        if (d3 != 11)
            goto nothing;
        goto skipsrc;
    }
    if (f == 2) {                       /* Bcc: label -> d(pc) */
        if (d3 != 8)
            illeaerr();
        d3 = 9;
        goto nothing;
    }
    if (f == 3 || f == 4)               /* moveq / trap */
        goto skipsrc;
    if (f == 5) {                       /* DBcc */
        if (d5 != 8)
            illeaerr();
        d1 |= d2;
        d5 = 9;
    }
    goto nothing;

skipsrc:
    insasm(e, a1, d3);
    d2 = d4;
    d3 = d5;
    goto skipsrc2;

nothing:
    {
        WORD s4 = d4, s5 = d5;
        doea(&d1, d2, d3, extraword2, e->srcmask, e->srcshift, e, &a1); /* do src */
        d2 = s4;
        d3 = s5;
    }
skipsrc2:
    if (d3 < 0) {                       /* .one */
        if (e->destmask)
            illeaerr();
        goto putit;
    }
    asmbuff = buff2;
    asmlen = asmlen2;
    doea(&d1, d2, d3, extraword, e->destmask, e->destshift, e, &a1);

putit:
    /* @5252 put in the opcode */
    if ((e->size & 0xf000) != 0xf000) {
        UWORD sh = e->size >> 12;       /* shift for size */
        UWORD v = (e->size >> asmsize) & 15;
        d1 |= v << sh;                  /* size into opcode */
    }
    if (a4 < libat)
        W_(a4) = d1;
    else
        nomemleft = -1;
    pc = a1;
}

/* blitz2.s:5278 asmoff - add an offset */
void asmoff(ULONG a1)
{
    ULONG save = pc;
    pc = a1;
    addoff();
    pc = save;
}

/* blitz2.s:5286 asmfixer - end of pass asm filler-inner */
void asmfixer(void)
{
    struct asmnode *a2;

    if (nomemleft)
        return;
    if ((optreqga15.Flags & GFLG_SELECTED) && !smallpass)
        return;
    a2 = firstasm;
    constmode = -1;
    regat = 0;
    prepstack();
    for (;;) {
        ULONG a1;
        UWORD d1, c;
        LONG d3;

        chkstop();
        if (!a2)
            break;
        asmbuff = (UBYTE *)a2;          /* for asmerr2 (error line) */
        if (a2->type)
            srcptr = a2->text;
        else
            srcptr = *(UBYTE **)a2->text;   /* pointer to real text */
        asmtype = 0;
        d3 = (LONG)evalconst3(&c);
        a2 = (struct asmnode *)asmbuff;
        if (!eos(c) && c != '(' && c != ',')
            syntaxerr();
        a1 = a2->addr;
        d1 = a2->mode;
        switch (d1) {
        case 5:
        case 7:
            chkword(d3);
            W_(a1) = (UWORD)d3;
            break;
        case 6:
            chkbyte(d3);
            B_(a1 + 1) = (UBYTE)d3;
            break;
        case 17:
            B_(a1 + 1) = (UBYTE)d3;
            break;
        case 8:
        isabsl:
            if (asmtype)
                asmoff(a1);
            L_(a1) = d3;
            break;
        case 9:
            d3 -= a1;
            chkword(d3);
            W_(a1) = (UWORD)d3;
            break;
        case 10:
            d3 -= a1;
            chkbyte(d3);
            B_(a1 + 1) = (UBYTE)d3;
            break;
        default:                        /* .imm */
            switch (a2->flag) {
            case 1:                     /* .tocount (addq/subq/shift) */
                if ((ULONG)d3 > 8)
                    illimmerr();
                if (d3 == 0)
                    illimmerr();
                W_(a1 - 2) |= (UWORD)((d3 & 7) << 9);
                break;
            case 3:                     /* .tomoveq */
                chkbyte(d3);
                B_(a1 - 1) = (UBYTE)d3;
                break;
            case 4:                     /* .totrap */
                if ((ULONG)d3 > 15)
                    illtraperr();
                B_(a1 - 1) |= (UBYTE)d3;
                break;
            default:                    /* .noflag */
                if (d1 == 11)
                    goto isabsl;
                W_(a1) = (UWORD)d3;     /* .word2: unchecked */
                break;
            }
            break;
        }
        a2 = a2->next;
    }
    constmode = 0;
}

/* blitz2.s:5386 doea - d2=reg, d3=ea, d4=extraword, d6=allowable mask,
   d7=shift data; or in bits for opcode in d1 */
void doea(UWORD *d1, UWORD d2, UWORD d3, UWORD d4, UWORD d6, UBYTE d7,
          const struct asment *e, ULONG *a1)
{
    UWORD s;

    /* btst d3,d6 (register form: bit d3 mod 32; modes are 0..15 here) */
    if (d3 >= 16 || !((d6 >> d3) & 1))
        illeaerr();
    if (makemode(d2, d3, d4, e, a1))
        return;                         /* DIFF: asm addq #4,a7 in makemode */
    if (d3 >= 7) {
        d2 = d3 - 7;
        if (d2 >= 5)
            d2 = 4;
        d3 = 7;
    }
    s = d7 & 15;
    if (s != 15)
        *d1 |= d3 << s;
    s = (d7 >> 4) & 15;
    if (s != 15)
        *d1 |= d2 << s;
}

/* blitz2.s:5423 chkbyte */
void chkbyte(LONG d3)
{
    if (d3 > 127 || d3 < -128)
        illdiserr();
}

/* blitz2.s:5429 chkword */
void chkword(LONG d3)
{
    if (d3 > 32767 || d3 < -32768)
        illdiserr();
}

/* blitz2.s:5435 insasm - add asmbuff, asmlen, pc, ea to list */
void insasm(const struct asment *e, ULONG a1, UWORD d3)
{
    UWORD d2 = 16 + asmlen;
    struct asmnode *n = doallocmem(d2, MEMF_PUBLIC);
    UBYTE *s, *d;

    n->line = linenumat;
    n->next = firstasm;
    firstasm = n;
    n->addr = a1;
    n->mode = d3;
    n->flag = e->flag < 0 ? 0 : e->flag;
    /* BUG(orig) (#1): size stored as a byte - texts longer than 239 bytes give
       a wrong size for freeasms */
    n->size = (UBYTE)d2;
    s = asmbuff + 1;
    d = n->text;
    n->type = *s++;                     /* type - 0 = indirect, else direct */
    if (!n->type) {
        memcpy(d, s, 4);
    } else {
        while ((*d++ = *s++) != 0)
            ;
    }
}

/* blitz2.s:5474 makemode */
BOOL makemode(UWORD d2, UWORD d3, UWORD d4, const struct asment *e, ULONG *a1)
{
    if (d3 < 5)
        return FALSE;
    if (d3 > 11) {                      /* .done2 */
        if (d3 != 15)
            return FALSE;
        /* movem register list.  DIFF: asm `addq #4,a7` returned past doea */
        if (*a1 < libat)
            W_(*a1) = d2;
        *a1 += 2;
        return TRUE;
    }
    if (d3 == 11) {                     /* .imm */
        if ((UWORD)asmsize < 8) {
            insasm(e, *a1, 16);
            *a1 += 2;
        } else {
            insasm(e, *a1, 11);         /* .immlong */
            *a1 += 4;
        }
        return FALSE;
    }
    insasm(e, *a1, d3);
    switch (d3) {
    case 5:
    case 7:
    case 9:
        *a1 += 2;
        break;
    case 8:
        *a1 += 4;
        break;
    default:                            /* 6, 10: d(..,xi) */
        if (*a1 < libat)
            W_(*a1) = d4;
        *a1 += 2;
        break;
    }
    return FALSE;
}

/* ================================================== parameter fetching */

/* blitz2.s:5519 countpees - count up how many parameters are following,
   nest a-la {,(,},) */
UWORD countpees(UWORD c)
{
    UWORD d1 = 0, d3 = 0;

    storeloc(c);
    c = getchar3();                     /* eol? */
    if (eos(c))
        goto done;
    goto skiphelp;
loop:
    c = getchar3();
    if (eos(c)) {
        if (d3)
            syntaxerr();
        goto done2;
    }
    if (quoteflag)
        goto loop;
skiphelp:
    if (c == ')') {
        if (d3 == 0)
            goto done2;
        d3--;
        goto loop;
    }
    if (c == '}') {
        d3--;
        goto loop;
    }
    if (c == '(' || c == '{') {
        d3++;
        goto loop;
    }
    if (c == ',' && d3 == 0)
        d1++;
    goto loop;
done2:
    d1++;
done:
    resloc();
    return d1;
}

/* fetchpees with the type pointer advanced (asm a2) */
static UWORD fetchpees_p(UWORD n, const UBYTE **tp, UWORD pushmode)
{
    UWORD oldpmode = pmode;             /* in case of recursion */
    UWORD cnt = n;
    UWORD c;

    pmode = pushmode;
    for (;;) {
        UWORD t = *(*tp)++;
        if (!t)
            t = userp;                  /* user selected type */
        c = handlep(t);
        if (--cnt == 0)
            break;
        if (c != ',')
            notparerr();
    }
    pmode = oldpmode;
    return c;
}

/* blitz2.s:5563 fetchlibps - fetch library parameters */
UWORD fetchlibps(UWORD d1, const UBYTE *types)
{
    UWORD d0, d2;
    UWORD c = 0;

    numreps = 0;
    d0 = d1 & 0xff00;
    if (d0 == 0)
        goto noreps;
    d0 >>= 12;                          /* d0 = start of reps */
    if (d0) {
        d1 = (d1 & 0xff00) | (UBYTE)(d1 - d0);  /* new number to fetch */
        c = fetchpees_p(d0 & 15, &types, 0);    /* fetch first parms */
        if (c != ',')
            syntaxerr();
    }
    /* O.K., now to fetch repeats..... */
    d2 = (d1 >> 8) & 7;                 /* # repeatable */
    while ((UBYTE)d1 >= d2) {
        const UBYTE *t = types;         /* a2 restored per repeat */
        d1 = (d1 & 0xff00) | (UBYTE)(d1 - d2);
        c = fetchpees_p(d2 & 15, &t, 1);
        numreps++;
    }
    if (numreps == 0)
        syntaxerr();
    types += d2;
    d1 &= 255;
    if (d1 == 0)
        return c;
    if (c != ',')
        syntaxerr();
noreps:
    d0 = d1;
    if (d0)
        return fetchpees_p(d0 & 15, &types, 0);
    return d0;
}

/* blitz2.s:5620 fetchpees4 - always push */
UWORD fetchpees4(UWORD n, const UBYTE *types)
{
    return fetchpees_p(n & 15, &types, 1);
}

/* blitz2.s:5623 fetchpees2 - dont push unless told */
UWORD fetchpees2(UWORD n, const UBYTE *types)
{
    return fetchpees_p(n & 15, &types, 0);
}

/* blitz2.s:5624 fetchpees3 */
UWORD fetchpees3(UWORD n, const UBYTE *types, UWORD pushmode)
{
    return fetchpees_p(n & 15, &types, pushmode);
}

/* blitz2.s:5626 fetchpees - fetch a list of parameters */
UWORD fetchpees(UWORD n, const UBYTE *types, UWORD pushmode)
{
    return fetchpees_p(n, &types, pushmode);
}

/* blitz2.s:5653 handlep - d2=what to get.
   BIT 7 : The Address of a var - d2.b = type of var
   BIT 6 : push results that would normally go into D(regat)
   BIT 5 : We want array info. */
UWORD handlep(UWORD type)
{
    UWORD c, t;

    setpmodehi(type & 64);              /* move.b d1,pmode */
    if (type & 0x80)
        return handlevara(type);
    if (type & 0x20)
        return handlearr(type);
    t = type;                           /* normal p get */
    c = getap(&t);
    putap(t);                           /* falls through into putap */
    return c;
}

/* blitz2.s:5675 putap - push D(regat), type in d2 */
void putap(UWORD type)
{
    if (pmode)
        pushap(type);
    else
        putapincreg();
}

/* blitz2.s:5679 pushap - push D(regat), type in d2 */
void pushap(UWORD type)
{
    UWORD d1;

    stackuse += 4;
    d1 = pushdn[0] | regat;             /* push long */
    if (type < 3) {
        stackuse -= 2;
        d1 |= 0x1000;                   /* push word only */
    }
    pokewd(d1);
}

/* blitz2.s:5689 putapincreg */
void putapincreg(void)
{
    regat++;
}

/* blitz2.s:5695 putalp */
void putalp(void)
{
    putap(T_LONG);
}

/* blitz2.s:5698 handlevara - Here, we want the address of a var type d2 */
UWORD handlevara(UWORD type)
{
    UWORD exp = type & 7;
    UWORD d2 = exp;
    UWORD c = ampersand2(&d2);

    if ((UBYTE)d2 != 7) {               /* ignore pnt to $ */
        if (d2 & 0x4000)
            d2 = (d2 & 0xff00) | 3;     /* pointer to long */
        else if ((UBYTE)d2 == 0)
            d2 = (d2 & 0xff00) | 3;
    }
    if ((UBYTE)exp == 0) {              /* .unp */
        stackuse += 2;
        pokewd(ptype[0]);
        pokewd((UBYTE)d2);
        putalp();
        return c;
    }
    if ((UBYTE)exp != (UBYTE)d2)
        mismatcherr();
    putalp();
    return c;
}

/* blitz2.s:5730 handlearr */
UWORD handlearr(UWORD type)
{
    struct var *v;
    struct type *t;

    baseadd(&v, &t);
    if (type & 0x10)
        highadd(v);
    if (type & 0x08)
        sendtype(v, t);
    return getchar3();
}

/* blitz2.s:5742 sendtype - OK, type of array is gonna be put on stack */
void sendtype(struct var *v, struct type *t)
{
    UWORD d1 = (regat << 9) | 0x7000;

    if (v->flags & 1) {                 /* btst #0,7(a3): A Pointer */
        d1 = (d1 & 0xff00) | 3;
    } else {
        if (t->u.code >= 256)
            arrerr1();                  /* illegal type to send */
        d1 = (d1 & 0xff00) | B_((UBYTE *)t + 7);
    }
    pokewd(d1);                         /* moveq #type,d regat */
    pushap(2);                          /* just a word. */
}

/* blitz2.s:5759 baseadd - send base on stack or in reg */
void baseadd(struct var **v, struct type **t)
{
    UWORD c = getchar3();

    getvname(c);
    if (!(flagmask & 2))
        syntaxerr();
    *v = fetchvar(getvnamelen, t, NULL);
    c = getchar3();
    if (c != ')')                       /* we just want a() or whatever */
        syntaxerr();
    if (added)
        noarrayerr();
    pokewd((regat << 9) | fbase[0]);
    pokewd((*v)->offset);
    putalp();                           /* OK, we've got da base reg. */
}

/* blitz2.s:5781 highadd - send high end of array on stack */
void highadd(struct var *v)
{
    WORD d1;
    UWORD w;

    pokewd((regat << 9) | fbase[0]);
    d1 = v->ndims << 2;
    if (varmode)
        d1 = -d1;
    d1 += v->offset;
    pokewd(d1);
    w = regat << 9;
    if (pmode || regat >= 7) {
        /* we just pushed last parameter / last was d6, it must have been
           pushed too: we have to add to stack! */
        w |= pushadd[0];
    } else {
        w |= pushlast[0];
        w |= (UWORD)(regat - 1);
    }
    pokewd(w);
    setpmodehi(1);                      /* move.b #1,pmode */
    putalp();
}

/* blitz2.s:5822 getap */
UWORD getap(UWORD *type)
{
    *type &= 7;                         /* 0-7 (unknown to string) */
    if (*type == 0)
        return evalu2(type);
    return eval(type);
}

/* ============================================================ libraries */

/* blitz2.s:5826 resetlibs */
void resetlibs(void)
{
    struct blitzlib *l;

    for (l = firstlib; l; l = l->next) {
        l->varoff = -1;
        l->flags &= ~LIBF_USED;
    }
}

/* blitz2.s:5835 usevars */
struct blitzlib *usevars(void)
{
    return uselib(allocvars);
}

/* blitz2.s:5837 uselib - lib number in d1 - turn it on baby! */
struct blitzlib *uselib(UWORD libnum)
{
    return uselib3(findlib(libnum), libnum);
}

/* blitz2.s:5841 uselib3 */
struct blitzlib *uselib3(struct blitzlib *l, UWORD d1)
{
    UBYTE *a0;
    ULONG end, a2;

    if (cfetchmode) {
        if (!(l->flags & LIBF_CONSTOK))
            badconerr();                /* Can't use this Library! */
        return l;
    }
    if (l->flags & LIBF_USED) {         /* bset #7: already done.... */
        return l;
    }
    l->flags |= LIBF_USED;
    if (l->varoff != -1)
        return l;                       /* something already here */
    if (dirmode) {
        l->flags &= ~LIBF_USED;
        dirliberr();
    }

    /* @5866 is it a maximum type lib.... if so, create a varoff for it's
       structs..... */
    a0 = (UBYTE *)l + l->maxinfo;
    if (W_(a0 - 2)) {
        uselib(64335);
        varoff = (varoff + 1) & ~1;
        W_(a0 - 6) = varoff;
        varoff += 4;
        if (L_(a0 - 14))
            varoff += 4;
    }

    /* @5885 copy the lib code */
    l->outaddr = lib;                   /* pc of lib */
    a0 = (UBYTE *)l + l->maxinfo;       /* start of lib */
    end = (ULONG)l + l->size;           /* end of lib */
    if (l->errchkoff && !debugga)
        end = (ULONG)l + l->errchkoff;  /* copy only non-errchks */
    a2 = lib;
    for (;;) {
        if (a2 >= data1at) {
            nomemleft = -1;
            do {
                a0 += 2;
                a2 += 2;
            } while ((ULONG)a0 < end);
            break;
        }
        W_(a2) = W_(a0);
        a0 += 2;
        a2 += 2;
        if ((ULONG)a0 >= end)
            break;
    }
    lib = a2;

    /* @5911 relocate library code! */
    if (l->nrelocs) {
        ULONG n = l->nrelocs;
        const ULONG *rp = l->relocs;
        const ULONG *first = NULL;
        ULONG d3 = 0;                   /* no tokejsrs (yet!) */
        do {
            ULONG d2 = *rp++;
            LONG off;
            if (l->errchkoff && !debugga && d2 >= l->errchkoff)
                continue;               /* .next (do-while: tests --n) */
            if ((WORD)W_((UBYTE *)l + d2) < 0 && d3 == 0) {
                first = rp;
                d3 = n;                 /* number left to do! */
            }
            off = (LONG)(d2 - l->maxinfo);
            if (off < 0)
                continue;
            off += l->outaddr;
            addoff2(off - pcat);
            if (nomemleft)
                continue;
            L_(off) += l->outaddr - l->maxinfo;
        } while (--n);
        if (d3)
            handlejsrs(l, first - 1, d3);
    }

    /* @5959 var slot */
    if (l->varkind == 0) {
        l->varoff = 0;
    } else {
        varoff = (varoff + 1) & ~1;
        l->varoff = varoff;
        if (d1 == 65235) {
            movestdn[1] = l->varoff;
            pusha3[1] = l->varoff;
            pusha32[1] = l->varoff;
            pulla3[1] = l->varoff;
            geta3[1] = l->varoff;
        }
        if (d1 == 64535) {
            dataget[1] = l->varoff;
            dataput[1] = l->varoff;
            rescode[3] = l->varoff;     /* rescode+6 */
            rescode2[1] = l->varoff;
        }
        varoff += 2;
        if (l->varkind >= 0)
            varoff += 2;
        usevars();
    }
    uselib2(l, l->initsub, d1);
    uselib2(l, l->finitsub, d1);
    return l;
}

/* blitz2.s:6003 fixjsrs - OK, we have to create code and fill in offsets
   for all tokejsrs done through the program.  Lists may grow while being
   processed (tokecode -> maketjsr): done in batches between sentinels. */
void fixjsrs(void)
{
    struct tokejsrnode *d2 = NULL;      /* do list till this... */
    struct tokejsrnode *a2;

    while ((a2 = tokeslist) != d2) {
        struct tokejsrnode *stop = a2;  /* next till... */
        do {
            struct addrnode *d3 = NULL;
            struct addrnode *a3;
            while ((a3 = a2->offs) != d3) {
                ULONG d7 = tokecode(a2->token, a2->form);
                if (!nomemleft) {
                    struct addrnode *o;
                    for (o = a3; o != d3; o = o->next)
                        L_(o->addr) = d7;
                }
                d3 = a3;
            }
            a2 = a2->next;
        } while (a2 != d2);
        d2 = stop;
    }
}

/* .putcode of tokecode: generate a stub at pc */
static ULONG putcode_cute(struct blitzlib *a2, UBYTE *a3)
{
    ULONG old = pc;
    lasta6 = -1;
    cutejsr(a2, a3);
    pokewd(0x4e75);
    return old;
}

static ULONG putcode_lsub(struct blitzlib *a2, ULONG d1)
{
    ULONG old = pc;
    lasta6 = -1;
    makelibsub(a2, d1);
    pokewd(0x4e75);
    return old;
}

/* blitz2.s:6047 tokecode - d1=token number, d2=form number!
   return d7, address to jsr! */
ULONG tokecode(UWORD tok, UWORD form)
{
    struct blitzlib *a2;
    UBYTE *a3 = findtoke(tok, &a2);
    WORD d2;
    ULONG d7;

    blitzmode &= ~0x8000;
    if (form & 0x4000) {
        form &= ~0x4000;
        blitzmode |= 0x8000;
    }
    d2 = form;
    if (W_(a3) == 0)
        goto simple;
    if (B_(a3 + 1) & 0x04)
        tokeerr();
    if (B_(a3 + 1) & 0x08)
        tokeerr();

    /* OK, it's a complex sorta token... */
    a3 += 6;                            /* skip node */
    for (;;) {
        UWORD d0 = W_(a3);
        a3 += 2;
        if ((WORD)d0 < 0)
            tokeerr();
        a3 = (UBYTE *)(((ULONG)a3 + (d0 & 255) + 1) & ~1UL);
        if (--d2 < 0)
            break;
        a3 = skiplibreg(a3) + 12;       /* .loop2 / .skip */
    }
    if (W_(a3) != 0)
        goto go4it;                     /* libs to fetch.... */
    if ((L_(a3 + 2) | L_(a3 + 10)) != 0)
        goto go4it;                     /* error chx... */
    if (B_(a3 + 9) & 1)
        goto go4it;                     /* ditto... */
    d7 = L_(a3 + 6);                    /* sub offset */
    if ((W_((UBYTE *)a2 + d7) & 0xf000) == 0xa000)
        goto go4it;
penis:
    return d7 - a2->maxinfo + a2->outaddr;
go4it:
    return putcode_cute(a2, a3);

simple:
    if (d2)
        tokeerr();
    if (W_(a3 + 6))
        goto be4it;
    if ((L_(a3 + 8) | L_(a3 + 16)) != 0)
        goto be4it;
    if (B_(a3 + 15) & 1)
        goto be4it;
    d7 = L_(a3 + 12);
    if ((W_((UBYTE *)a2 + d7) & 0xf000) != 0xa000)
        goto penis;
be4it:
    return putcode_lsub(a2, (ULONG)(a3 - (UBYTE *)a2));
}

/* blitz2.s:6138 freetlist */
void freetlist(void)
{
    struct tokejsrnode *a2 = tokeslist;

    tokeslist = NULL;
    while (a2) {
        struct tokejsrnode *nx;
        struct addrnode *a3 = a2->offs;
        while (a3) {
            struct addrnode *o = a3;
            a3 = a3->next;
            FreeMem(o, 8);
        }
        nx = a2->next;
        FreeMem(a2, 12);
        a2 = nx;
    }
}

/* blitz2.s:6158 handlejsrs - a2=first, d2=how many to do. */
void handlejsrs(struct blitzlib *l, const ULONG *relocs, ULONG count)
{
    do {
        ULONG d3 = *relocs++;
        UWORD d5 = W_((UBYTE *)l + d3);
        UWORD d4;
        if ((WORD)d5 >= 0)
            continue;
        if (l->errchkoff && !debugga && d3 >= l->errchkoff)
            continue;
        d5 &= 0x7fff;                   /* to offset number */
        d4 = W_((UBYTE *)l + d3 + 2);
        d3 = d3 - l->maxinfo + l->outaddr;
        maketjsr(d3, d4, d5, NULL);
    } while (--count);
}

/* blitz2.s:6189 maketjsr - d3=address to poke jsr into, d4=token number,
   d5=offset number; bit 14 of d5=1 if BLITZ type. */
UBYTE *maketjsr(ULONG addr, UWORD tok, UWORD form, struct blitzlib **libp)
{
    struct tokejsrnode *t;
    struct addrnode *o;
    struct blitzlib *l;
    UBYTE *sub;

    for (t = tokeslist; t; t = t->next)
        if (t->token == tok && t->form == form)
            break;
    if (!t) {                           /* None done yet...make one up! */
        t = doallocmem(12, MEMF_PUBLIC);
        t->next = tokeslist;
        tokeslist = t;
        t->token = tok;
        t->form = form;
        t->offs = NULL;
    }
    o = doallocmem(8, MEMF_PUBLIC);
    o->addr = addr;
    o->next = t->offs;
    t->offs = o;
    sub = findtoke(tok, &l);            /* use this library - NOW! */
    if (libp)
        *libp = l;
    return sub;
}

/* blitz2.s:6237 uselib2 */
void uselib2(struct blitzlib *l, ULONG off, UWORD self)
{
    const UWORD *a1;

    if (!off)
        return;
    a1 = (const UWORD *)((UBYTE *)l + off + 6);
    for (;;) {
        UWORD d1 = *a1++;
        if (!d1)
            break;
        if (d1 != self)
            uselib(d1);
        a1++;
    }
}

/* blitz2.s:6253 findlibnoerr */
struct blitzlib *findlibnoerr(UWORD libnum)
{
    struct blitzlib *l;

    for (l = firstlib; l; l = l->next)
        if (l->libnum == (WORD)libnum)
            return l;
    return NULL;
}

/* blitz2.s:6264 findlib - find lib number d1 - return in a1 */
struct blitzlib *findlib(UWORD libnum)
{
    struct blitzlib *l;

    for (l = firstlib; l; l = l->next)
        if (l->libnum == (WORD)libnum)
            return l;
    noliberr(libnum);
}

/* blitz2.s:6277 findtoke - d1=token. return a2.l=lib, a3.l=sub */
UBYTE *findtoke(UWORD tok, struct blitzlib **libp)
{
    struct blitzlib *l;
    UBYTE *a3;
    UWORD d1 = tok;

    for (l = firstlib; ; l = l->next) {
        if (!l)
            notokerr(tok);
        if (d1 >= l->firsttoken && d1 < l->lasttoken)
            break;
    }
    /* lib found!  (system and user libs are walked the same way; the
       original's user lib variant has its token check commented out) */
    a3 = l->subs;
    while (d1 != l->firsttoken) {
        d1--;
        a3 = *(UBYTE **)(a3 + 2);
    }
    uselib3(l, l->libnum);              /* set lib# */
    if (libp)
        *libp = l;
    return a3;
}

/* blitz2.s:6322 loadlibgroup - load a group of libraries!
   d0=len, d1=name; return d0=address */
APTR loadlibgroup(ULONG len, STRPTR name, struct node **tail)
{
    BPTR fh;
    UBYTE *buf, *a0;

    if (!len)
        return NULL;
    fh = Open(name, MODE_OLDFILE);
    if (!fh)
        return NULL;
    buf = doallocmem(len, MEMF_PUBLIC);
    Read(fh, buf, len);
    Close(fh);

    a0 = buf;
    for (;;) {
        ULONG d0 = L_(a0);
        struct blitzlib *s;
        UBYTE *r;
        a0 += 4;
        if (!d0)
            break;
        s = (struct blitzlib *)(a0 + 32);
        (*tail)->next = (struct node *)s;   /* link in. */
        *tail = (struct node *)s;
        a0 += d0;
        /* fix stuff - done by loadalib */
        d0 = L_((UBYTE *)s - 4) * 4;    /* number of longwords -> bytes */
        s->size = d0;
        s->nrelocs = 0;                 /* no relocs */
        r = (UBYTE *)s + d0 + 12;       /* reloc info. */
        if (r >= a0)
            continue;                   /* is it over? */
        s->nrelocs = L_(r - 8);         /* number of relocs. */
        s->relocs = (ULONG *)r;         /* pointer to relocs. */
    }
    return buf;
}

/* blitz2.s:6374 loadlibs - use libslist to load in used libs dir contents */
void loadlibs(void)
{
    struct node *a3 = (struct node *)&firstlib;

    acidlibsat = loadlibgroup(acidlibs, acidname, &a3);
    deflibsat = loadlibgroup(deflibs, defname, &a3);
    /* all libs read in, now to link 'em up. */
    a3->next = NULL;
    blitzlibsat = a3;
    loadblitzlibs();                    /* falls through */
}

/* blitz2.s:6392 loadblitzlibs - load in blitz libs only! */
void loadblitzlibs(void)
{
    UBYTE *a0 = undobuffer;
    const char *a1 = libsdir;
    UBYTE *namepos;
    struct node *a2 = (struct node *)&libslist;  /* lib names! */
    struct node *a3 = blitzlibsat;

    while ((*a0++ = *a1++) != 0)
        ;
    if (!libsdir[0]) {
        /* asm: a0 == undobuffer after reading undobuffer[-1] */
        a0 = undobuffer;
    } else {
        a0 -= 2;
        /* DIFF: '/' as in the compiler of the CD (2.10, docs/v2.10, #2);
           blitz2.s tests ':', and the default libsdir already ends with
           '/': "blitz2:userlibs//dir" (AmigaDOS: parent directory) */
        if (*a0++ != '/')
            *a0++ = '/';
    }
    namepos = a0;                       /* store first pos for chars */

    for (;;) {
        struct libsnode *ln;
        UBYTE *fpos;
        BPTR lk;

        a2 = a2->next;
        if (!a2) {
            a3->next = NULL;
            fixlibs();
            return;
        }
        ln = (struct libsnode *)a2;
        if (!ln->used)
            continue;
        a0 = namepos;                   /* add dir name to libsdir */
        a1 = ln->name;
        while ((*a0++ = *a1++) != 0)
            ;
        fpos = a0;                      /* pointer for lib name */
        lk = Lock((STRPTR)undobuffer, ACCESS_READ);
        if (!lk)
            continue;                   /* .exdone2 */
        fpos[-1] = '/';
        if (Examine(lk, namebufffib)) {
            /* O.K. for exnext loop.... */
            while (ExNext(lk, namebufffib)) {
                const UBYTE *s;
                UBYTE *d;
                if (namebufffib->fib_DirEntryType >= 0)
                    continue;
                if (endininfo((char *)namebuff + 8))
                    continue;
                s = namebuff + 8;
                d = fpos;
                while ((*d++ = *s++) != 0)
                    ;
                loadalib((STRPTR)undobuffer, &a3);
            }
        }
        UnLock(lk);                     /* .exdone */
    }
}

/* blitz2.s:6470 loadalib - filename pointed to by d1, dos base in a6,
   lib in a3 */
void loadalib(STRPTR name, struct node **tail)
{
    BPTR fh = Open(name, MODE_OLDFILE);
    ULONG d6;
    struct blitzlib *a5;
    LONG got;

    if (!fh)
        return;                         /* .openerr */
    Read(fh, optstuff, 32);
    d6 = L_(optstuff + 28) << 2;        /* long words to read */
    a5 = doallocmem(d6, MEMF_PUBLIC);
    Read(fh, a5, d6);                   /* read in struct */
    (*tail)->next = (struct node *)a5;
    *tail = (struct node *)a5;
    a5->size = d6;
    a5->nrelocs = 0;
    got = Read(fh, optstuff, 12);
    /* BUG(orig) (#3): unsigned compare - a read error (-1) passes */
    if ((ULONG)got >= 12) {
        ULONG cnt = L_(optstuff + 4);
        ULONG d5 = cnt << 2;            /* length of reloc info */
        a5->nrelocs = cnt;
        a5->relocs = doallocmem(d5, MEMF_PUBLIC);   /* pointer to reloc mem */
        Read(fh, a5->relocs, d5);
    }
    Close(fh);                          /* .closeup */
}

/* blitz2.s:6525 fixlibs - fill in blanks in lib list: tokens, links etc */
void fixlibs(void)
{
    struct node *a3 = (struct node *)lasttoken;
    struct blitzlib *a2;
    jmp_buf jb;
    jmp_buf *volatile olderr;

    nummaxs = 0;
    for (a2 = firstlib; a2; a2 = a2->next) {
        WORD d7 = a2->libnum;
        UBYTE *a1;
        ULONG a4;

        if (d7 >= 0) {                  /* user lib */
            d7 = (d7 << 7) + 1;
            a2->firsttoken = d7;
        }
        /* .syslib: system libs (negative lib number) keep the token range
           stored in the lib file (e.g. memlib $c000.., see the equates at
           blitz2.s 72-250); only user libs get libnum*128+1 above */
        a1 = a2->subs;
        a4 = (ULONG)&temp1;             /* dummy first link */
        for (;;) {
            UWORD d0 = W_(a1);
            if ((WORD)d0 < 0)
                break;                  /* .next */
            L_(a4) = (ULONG)a1;
            a4 = (ULONG)a1 + 2;         /* last link for subs */
            a1 += 6;
            if (d0 == 8)
                goto addtoke;
            if ((d0 & 15) == 0) {
                /* skip sys sub */
                a1 = skiplibreg(a1) + 12;
                d7++;                   /* in case it's a user lib */
                continue;
            }
            if (d0 & 4) {               /* amiga lib */
                a1 += 4;
                while (!(*a1++ & 0x80))
                    ;
                a1 = aligna1(a1);
                goto addtoke;
            }
            /* process standard function or statement */
            for (;;) {
                UWORD f = W_(a1);
                a1 += 2;
                if ((WORD)f < 0)
                    break;
                a1 += f & 255;
                a1 = aligna1(a1);
                a1 = skiplibreg(a1) + 12;
            }
        addtoke:
            a3->next = (struct node *)a1;
            a3 = (struct node *)a1;
            a1 += 4;
            W_(a1) = d7;
            a1 += 2;
            while (*a1++)
                ;
            while (*a1++)
                ;
            a1 = aligna1(a1);
            d7++;
        }
        if (a2->libnum >= 0)
            a2->lasttoken = d7;
        a1 += 2;
        if (L_(a1)) {
            a1 += 4;
            nummaxs++;
            a1 += 22;
        } else
            a1 += 4;
        a2->maxinfo = a1 - (UBYTE *)a2;
    }
    a3->next = NULL;

    /* asm: errstack=a7, errcont=.cont, ezerr=-1: a missing memlib/ffplib
       resumes at .cont (rts) */
    olderr = errjmp;
    errjmp = &jb;
    ezerr = -1;
    if (setjmp(jb)) {
        errjmp = olderr;
        return;                         /* .cont */
    }
    memlib = findlib(65530);            /* get address of mem lib */
    ffplib = findlib(64935);
    errjmp = olderr;                    /* DIFF: asm never restored errstack */
}

/* blitz2.s:6618 skiplibreg */
UBYTE *skiplibreg(const UBYTE *p)
{
    for (;;) {
        UWORD w = W_(p);
        p += 2;
        if (!w)
            break;
        p += 2;
    }
    return (UBYTE *)p;
}

/* blitz2.s:6624 aligna0 */
UBYTE *aligna0(const void *p)
{
    return aligna1(p);
}

/* blitz2.s:6629 aligna1 */
UBYTE *aligna1(const void *p)
{
    return (UBYTE *)(((ULONG)p + 1) & ~1UL);
}

/* blitz2.s:6635 freeblitzlibs */
void freeblitzlibs(void)
{
    struct blitzlib *a2 = (struct blitzlib *)blitzlibsat->next;

    blitzlibsat->next = NULL;
    while (a2) {
        struct blitzlib *nx;
        if (a2->nrelocs)
            FreeMem(a2->relocs, a2->nrelocs << 2);
        nx = a2->next;
        FreeMem(a2, a2->size);
        a2 = nx;
    }
    frlibslist();                       /* tail: beq frlibslist */
}

/* blitz2.s:6653 freelibs - free up all memory used by libs */
void freelibs(void)
{
    freeblitzlibs();
    firstlib = NULL;
    L_(lasttoken) = 0;
    if (deflibs && deflibsat) {
        APTR p = deflibsat;
        deflibsat = NULL;
        FreeMem(p, deflibs);
    }
    if (acidlibs && acidlibsat) {
        APTR p = acidlibsat;
        acidlibsat = NULL;
        FreeMem(p, acidlibs);
    }
}

/* blitz2.s:6680 freemacs */
void freemacs(void)
{
    struct macro *a0 = lastresmac;
    struct macro *a2 = a0->next;

    a0->next = NULL;
    while (a2) {
        struct macro *m = a2;
        if (m->textlen)
            FreeMem(m->text, m->textlen);
        a2 = a2->next;
        FreeMem(m, m->len);
    }
}

/* blitz2.s:6701 freeasms */
void freeasms(void)
{
    struct asmnode *a2 = firstasm;

    firstasm = NULL;
    while (a2) {
        struct asmnode *n = a2;
        a2 = a2->next;
        FreeMem(n, n->size);
    }
}

/* ============================================================ data1 */

/* blitz2.s:6714 pokedata1b - byte into data1 */
void pokedata1b(UBYTE d1)
{
    if (data1 >= data2at) {
        nomemleft = -1;
        data1 += 1;
        return;
    }
    B_(data1) = d1;
    data1 += 1;
}

/* blitz2.s:6725 pokedata1l */
void pokedata1l(ULONG d1)
{
    pokedata1((UWORD)(d1 >> 16));
    pokedata1((UWORD)d1);               /* falls through into pokedata1 */
}

/* blitz2.s:6729 pokedata1 - poke word in d1 into data block */
void pokedata1(UWORD d1)
{
    if (data1 >= data2at) {
        nomemleft = -1;
        data1 += 2;
        return;
    }
    W_(data1) = d1;
    data1 += 2;
}
