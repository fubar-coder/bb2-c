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

/* blitz2.s:5617 ptype move #0,-(a7) (template) */
UWORD ptype[] = {
    0x3F3C, 0x0000,                     /* +0 ptype move #0,-(a7) */
};
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

/* ========================================================== assembler */

/* blitz2.s:5000 asmconst - store the text of the eval into asmfirst...
   text till '(','.' */
UWORD asmconst(void)
{
    UBYTE *buf = asmbuff;
    UWORD flag = numincs | macmask;
    UBYTE *saved;
    UWORD c, lc;

    W_(buf) = flag;                     /* flag! */
    buf += 2;
    if (flag)
        return copyconst(buf, &lc);     /* bne copyconst */
    saved = srcptr;                     /* move.l a5,-(a7) */
    c = copyconst(buf, &lc);
    buf = asmbuff;
    if ((UWORD)asmlen2 <= 4 || lc) {
        /* .leave: flag becomes $ffff = direct text.  asm `addq #4,a7`
           only drops the saved a5. */
        W_(buf) = ~W_(buf);
        return c;
    }
    /* indirect: pointer into the (persistent) source text */
    L_(buf + 2) = (ULONG)saved;
    B_(buf + 6) = 0;
    asmlen2 = 4;                        /* pointer + 0 */
    return c;
}

/* blitz2.s:5028 copyconst */
UWORD copyconst(UBYTE *dst, UWORD *lc)
{
    UWORD len = 0;
    UWORD c;

    *lc = 0;
    for (;;) {
        len++;
        c = getchar3();
        if (eos(c))
            break;
        if (c & 0x8000) {               /* token: 2 bytes */
            *dst++ = (UBYTE)(c >> 8);
            *dst++ = (UBYTE)c;
            len++;
            continue;
        }
        if (!quoteflag) {
            if (c == '(')
                break;
            /* ';cmp #'.',d0 / beq .done' commented out in the original */
            if (c == ',')
                break;
            if ((UBYTE)c == loclabch) {
                const char *lab = lastbiglab;
                *lc = c;
                if (!*lab)
                    nolocerr();
                do
                    len++;
                while ((*dst++ = *lab++) != 0);
                len--;
                dst--;
            }
        }
        *dst++ = (UBYTE)c;              /* .putbyte */
    }
    *dst = 0;
    asmlen2 = len;
    return c;
}

/* blitz2.s:5072 asmit - assemble one instruction (c = asm token) */
void asmit(UWORD c)
{
    UWORD idx;
    struct asment *e;
    UWORD sz, nib = 0;
    WORD srcreg = 0, srcmode = -1, destreg = 0, destmode = -1;
    UWORD opcode;
    ULONG at, opat;
    WORD f;
    WORD asmsize;                       /* 14917: 0/4/8 = .b/.w/.l */
    UBYTE *buff1, *buff2;               /* 14911/14912: src/dest text */

    alignpc();
    idx = (UWORD)(c - (0x8000 + fnum)) << 2;
    e = asmpnts[idx >> 2];
    /* (move d0,temp1: the asm keeps idx in temp1 for @5141) */

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
        srcreg = r; srcmode = m;        /* to src */
        destreg = r;                    /* asm: d4 keeps the src reg */
        asmlen = asmlen2;
        destmode = -1;
        extraword2 = extraword;
        if (c == ',') {
            asmbuff = namebuff2;
            getea(&m, &r);              /* dest */
            destreg = r; destmode = m;
        }
    }

    /* @5129 all stuff fetched!  O.K. - Let's assemble it! */
    at = pc;
    opat = at;
    at += 2;
    opcode = e->opcode;
    if (srcmode < 0) {                  /* .none: no src/dest ea! */
        if (e->srcmask | e->destmask)
            illeaerr();
        goto putit;
    }

    /* @5141 make into movea, addi etc. if possible */
    f = e->flag;
    if (f < 0) {
        UWORD ni = 0;
        UWORD t = idx;                  /* asm: temp1 */
        if ((f & 1) && destmode == 1) {
            if (srcmode != 14)
                ni = t + 8;
        } else if ((f & 2) && srcmode == 11)
            ni = t + 4;
        if (ni) {
            e = asmpnts[ni >> 2];
            opcode = e->opcode;
        }
    }

    /* @5164 .noia */
    buff1 = namebuff;
    buff2 = namebuff2;
    if (e->chk) {
        struct asmregs r;
        WORD res;
        r.opcode = opcode; r.srcreg = srcreg; r.srcmode = srcmode; r.destreg = destreg; r.destmode = destmode;
        r.size = asmsize;
        res = e->chk(&r);
        opcode = r.opcode; srcreg = r.srcreg; srcmode = r.srcmode; destreg = r.destreg; destmode = r.destmode;
        if (res < 0) {
            if (res == -2)
                illsizeerr();
            illeaerr();
        }
        if (res > 0) {
            WORD t; UWORD w;
            t = srcreg; srcreg = destreg; destreg = t;
            t = srcmode; srcmode = destmode; destmode = t;
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
        if (srcmode != 11)
            goto nothing;
        goto skipsrc;
    }
    if (f == 2) {                       /* Bcc: label -> d(pc) */
        if (srcmode != 8)
            illeaerr();
        srcmode = 9;
        goto nothing;
    }
    if (f == 3 || f == 4)               /* moveq / trap */
        goto skipsrc;
    if (f == 5) {                       /* DBcc */
        if (destmode != 8)
            illeaerr();
        opcode |= srcreg;
        destmode = 9;
    }
    goto nothing;

skipsrc:
    insasm(e, at, srcmode);
    goto skipsrc2;

nothing:
    doea(&opcode, srcreg, srcmode, extraword2, asmsize, e->srcmask, e->srcshift, e, &at); /* do src */
skipsrc2:
    if (destmode < 0) {                 /* .one */
        if (e->destmask)
            illeaerr();
        goto putit;
    }
    asmbuff = buff2;
    asmlen = asmlen2;
    doea(&opcode, destreg, destmode, extraword, asmsize, e->destmask, e->destshift, e, &at);

putit:
    /* @5252 put in the opcode */
    if ((e->size & 0xf000) != 0xf000) {
        UWORD sh = e->size >> 12;       /* shift for size */
        UWORD v = (e->size >> asmsize) & 15;
        opcode |= v << sh;              /* size into opcode */
    }
    if (opat < libat)
        W_(opat) = opcode;
    else
        nomemleft = -1;
    pc = at;
}

/* blitz2.s:5278 asmoff - add an offset */
void asmoff(ULONG addr)
{
    ULONG save = pc;
    pc = addr;
    addoff();
    pc = save;
}

/* blitz2.s:5286 asmfixer - end of pass asm filler-inner */
void asmfixer(void)
{
    struct asmnode *node;

    if (nomemleft)
        return;
    if ((optreqga15.Flags & GFLG_SELECTED) && !smallpass)
        return;
    node = firstasm;
    constmode = -1;
    regat = 0;
    prepstack();
    for (;;) {
        ULONG at;
        UWORD mode, c;
        LONG val;

        chkstop();
        if (!node)
            break;
        asmbuff = (UBYTE *)node;        /* for asmerr2 (error line) */
        if (node->type)
            srcptr = node->text;
        else
            srcptr = *(UBYTE **)node->text; /* pointer to real text */
        asmtype = 0;
        val = (LONG)evalconst3(&c, NULL);
        node = (struct asmnode *)asmbuff;
        if (!eos(c) && c != '(' && c != ',')
            syntaxerr();
        at = node->addr;
        mode = node->mode;
        switch (mode) {
        case 5:
        case 7:
            chkword(val);
            W_(at) = (UWORD)val;
            break;
        case 6:
            chkbyte(val);
            B_(at + 1) = (UBYTE)val;
            break;
        case 17:
            B_(at + 1) = (UBYTE)val;
            break;
        case 8:
        isabsl:
            if (asmtype)
                asmoff(at);
            L_(at) = val;
            break;
        case 9:
            val -= at;
            chkword(val);
            W_(at) = (UWORD)val;
            break;
        case 10:
            val -= at;
            chkbyte(val);
            B_(at + 1) = (UBYTE)val;
            break;
        default:                        /* .imm */
            switch (node->flag) {
            case 1:                     /* .tocount (addq/subq/shift) */
                if ((ULONG)val > 8)
                    illimmerr();
                if (val == 0)
                    illimmerr();
                W_(at - 2) |= (UWORD)((val & 7) << 9);
                break;
            case 3:                     /* .tomoveq */
                chkbyte(val);
                B_(at - 1) = (UBYTE)val;
                break;
            case 4:                     /* .totrap */
                if ((ULONG)val > 15)
                    illtraperr();
                B_(at - 1) |= (UBYTE)val;
                break;
            default:                    /* .noflag */
                if (mode == 11)
                    goto isabsl;
                W_(at) = (UWORD)val;    /* .word2: unchecked */
                break;
            }
            break;
        }
        node = node->next;
    }
    constmode = 0;
}

/* blitz2.s:5386 doea - d2=reg, d3=ea, d4=extraword, d6=allowable mask,
   d7=shift data; or in bits for opcode in d1; size = asmsize */
void doea(UWORD *opcode, UWORD reg, UWORD mode, UWORD extra, WORD size,
          UWORD mask, UBYTE shifts, const struct asment *e, ULONG *at)
{
    UWORD s;

    /* btst d3,d6 (register form: bit d3 mod 32; modes are 0..15 here) */
    if (mode >= 16 || !((mask >> mode) & 1))
        illeaerr();
    if (makemode(reg, mode, extra, size, e, at))
        return;                         /* DIFF: asm addq #4,a7 in makemode */
    if (mode >= 7) {
        reg = mode - 7;
        if (reg >= 5)
            reg = 4;
        mode = 7;
    }
    s = shifts & 15;
    if (s != 15)
        *opcode |= mode << s;
    s = (shifts >> 4) & 15;
    if (s != 15)
        *opcode |= reg << s;
}

/* blitz2.s:5423 chkbyte */
void chkbyte(LONG val)
{
    if (val > 127 || val < -128)
        illdiserr();
}

/* blitz2.s:5429 chkword */
void chkword(LONG val)
{
    if (val > 32767 || val < -32768)
        illdiserr();
}

/* blitz2.s:5435 insasm - add asmbuff, asmlen, pc, ea to list */
void insasm(const struct asment *e, ULONG addr, UWORD mode)
{
    UWORD size = 16 + asmlen;
    struct asmnode *n = doallocmem(size, MEMF_PUBLIC);
    UBYTE *s, *d;

    n->line = linenumat;
    n->next = firstasm;
    firstasm = n;
    n->addr = addr;
    n->mode = mode;
    n->flag = e->flag < 0 ? 0 : e->flag;
    /* BUG(orig) (#1): size stored as a byte - texts longer than 239 bytes give
       a wrong size for freeasms */
    n->size = (UBYTE)size;
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
BOOL makemode(UWORD reg, UWORD mode, UWORD extra, WORD size,
              const struct asment *e, ULONG *at)
{
    if (mode < 5)
        return FALSE;
    if (mode > 11) {                    /* .done2 */
        if (mode != 15)
            return FALSE;
        /* movem register list.  DIFF: asm `addq #4,a7` returned past doea */
        if (*at < libat)
            W_(*at) = reg;
        *at += 2;
        return TRUE;
    }
    if (mode == 11) {                   /* .imm */
        if ((UWORD)size < 8) {
            insasm(e, *at, 16);
            *at += 2;
        } else {
            insasm(e, *at, 11);         /* .immlong */
            *at += 4;
        }
        return FALSE;
    }
    insasm(e, *at, mode);
    switch (mode) {
    case 5:
    case 7:
    case 9:
        *at += 2;
        break;
    case 8:
        *at += 4;
        break;
    default:                            /* 6, 10: d(..,xi) */
        if (*at < libat)
            W_(*at) = extra;
        *at += 2;
        break;
    }
    return FALSE;
}

/* ================================================== parameter fetching */

/* blitz2.s:5519 countpees - count up how many parameters are following,
   nest a-la {,(,},) */
UWORD countpees(UWORD c)
{
    UWORD count = 0, depth = 0;

    storeloc(c);
    c = getchar3();                     /* eol? */
    if (eos(c))
        goto done;
    goto skiphelp;
loop:
    c = getchar3();
    if (eos(c)) {
        if (depth)
            syntaxerr();
        goto done2;
    }
    if (quoteflag)
        goto loop;
skiphelp:
    if (c == ')') {
        if (depth == 0)
            goto done2;
        depth--;
        goto loop;
    }
    if (c == '}') {
        depth--;
        goto loop;
    }
    if (c == '(' || c == '{') {
        depth++;
        goto loop;
    }
    if (c == ',' && depth == 0)
        count++;
    goto loop;
done2:
    count++;
done:
    resloc();
    return count;
}

/* fetchpees with the type pointer advanced (asm a2).  The asm kept the
   push mode in the global pmode and saved/restored it around the loop (in
   case of recursion); C hands it down to handlep.  proc: the procedure
   called (fetchprocps), NULL for a library. */
static UWORD fetchpees_p(UWORD n, const UBYTE **tp, UWORD pushmode,
                         const struct proc *proc)
{
    UWORD cnt = n;
    UWORD c;

    for (;;) {
        UWORD t = *(*tp)++;
        if (!t)
            t = userp;                  /* user selected type */
        c = handlep(t, pushmode);
        if (--cnt == 0)
            break;
        if (c != ',') {
            if (proc && ext_procdefaults(proc, n - cnt))
                break;
            notparerr();
        }
    }
    return c;
}

/* the arguments of a procedure call (asmlibs.h) */
UWORD fetchprocps(const struct proc *p)
{
    const UBYTE *types = p->params;
    UWORD c;

    if (ext_procnoargs(p))
        return getchar3();
    ext_procargs(p);
    c = fetchpees_p(p->nparams & 15, &types, 0, p);
    ext_procargs(NULL);
    return c;
}

/* blitz2.s:5563 fetchlibps - fetch library parameters */
UWORD fetchlibps(UWORD spec, const UBYTE *types)
{
    UWORD first, nreps;
    UWORD c = 0;

    numreps = 0;
    if ((spec & 0xff00) == 0)
        goto noreps;
    first = spec >> 12;                 /* d0 = start of reps */
    if (first) {
        spec = (spec & 0xff00) | (UBYTE)(spec - first); /* new number to fetch */
        c = fetchpees_p(first & 15, &types, 0, NULL); /* fetch first parms */
        if (c != ',')
            syntaxerr();
    }
    /* O.K., now to fetch repeats..... */
    nreps = (spec >> 8) & 7;            /* # repeatable */
    while ((UBYTE)spec >= nreps) {
        const UBYTE *t = types;         /* a2 restored per repeat */
        spec = (spec & 0xff00) | (UBYTE)(spec - nreps);
        c = fetchpees_p(nreps & 15, &t, 1, NULL);
        numreps++;
    }
    if (numreps == 0)
        syntaxerr();
    types += nreps;
    spec &= 255;
    if (spec == 0)
        return c;
    if (c != ',')
        syntaxerr();
noreps:
    if (spec)
        return fetchpees_p(spec & 15, &types, 0, NULL);
    return 0;
}

/* blitz2.s:5620 fetchpees4 - always push */
UWORD fetchpees4(UWORD n, const UBYTE *types)
{
    return fetchpees_p(n & 15, &types, 1, NULL);
}

/* blitz2.s:5623 fetchpees2 - dont push unless told */
UWORD fetchpees2(UWORD n, const UBYTE *types)
{
    return fetchpees_p(n & 15, &types, 0, NULL);
}

/* blitz2.s:5624 fetchpees3 */
UWORD fetchpees3(UWORD n, const UBYTE *types, UWORD pushmode)
{
    return fetchpees_p(n & 15, &types, pushmode, NULL);
}

/* blitz2.s:5626 fetchpees - fetch a list of parameters */
UWORD fetchpees(UWORD n, const UBYTE *types, UWORD pushmode)
{
    return fetchpees_p(n, &types, pushmode, NULL);
}

/* blitz2.s:5653 handlep - d2=what to get.
   BIT 7 : The Address of a var - d2.b = type of var
   BIT 6 : push results that would normally go into D(regat)
   BIT 5 : We want array info.
   pmode: the asm's global pmode (5618): the low byte is the push mode of
   fetchpees, handlep writes the high byte (move.b); tested as a word. */
UWORD handlep(UWORD type, UWORD pmode)
{
    UWORD c, t;

    pmode = (pmode & 0x00ff) | (UWORD)((type & 64) << 8);   /* move.b d1,pmode */
    if (type & 0x80)
        return handlevara(type, pmode);
    if (type & 0x20)
        return handlearr(type, pmode);
    t = type;                           /* normal p get */
    c = getap(&t);
    putap(t, pmode);                    /* falls through into putap */
    return c;
}

/* blitz2.s:5675 putap - push D(regat), type in d2 */
void putap(UWORD type, UWORD pmode)
{
    if (pmode)
        pushap(type);
    else
        putapincreg();
}

/* blitz2.s:5679 pushap - push D(regat), type in d2 */
void pushap(UWORD type)
{
    UWORD opcode;

    stackuse += 4;
    opcode = pushdn[0] | regat;         /* push long */
    if (type < 3) {
        stackuse -= 2;
        opcode |= 0x1000;               /* push word only */
    }
    pokewd(opcode);
}

/* blitz2.s:5689 putapincreg */
void putapincreg(void)
{
    regat++;
}

/* blitz2.s:5695 putalp */
void putalp(UWORD pmode)
{
    putap(T_LONG, pmode);
}

/* blitz2.s:5698 handlevara - Here, we want the address of a var type d2 */
UWORD handlevara(UWORD type, UWORD pmode)
{
    UWORD exp = type & 7;
    UWORD vtype = exp;
    UWORD c = ampersand2(&vtype);

    if ((UBYTE)vtype != 7) {            /* ignore pnt to $ */
        if (vtype & 0x4000)
            vtype = (vtype & 0xff00) | 3; /* pointer to long */
        else if ((UBYTE)vtype == 0)
            vtype = (vtype & 0xff00) | 3;
    }
    if ((UBYTE)exp == 0) {              /* .unp */
        stackuse += 2;
        pokewd(ptype[0]);
        pokewd((UBYTE)vtype);
        putalp(pmode);
        return c;
    }
    if ((UBYTE)exp != (UBYTE)vtype)
        mismatcherr();
    putalp(pmode);
    return c;
}

/* blitz2.s:5730 handlearr */
UWORD handlearr(UWORD type, UWORD pmode)
{
    struct var *v;
    struct type *t;

    baseadd(&v, &t, pmode);
    if (type & 0x10)
        highadd(v, pmode);
    if (type & 0x08)
        sendtype(v, t);
    return getchar3();
}

/* blitz2.s:5742 sendtype - OK, type of array is gonna be put on stack */
void sendtype(struct var *v, struct type *t)
{
    UWORD opcode = (regat << 9) | 0x7000;

    if (v->flags & 1) {                 /* btst #0,7(a3): A Pointer */
        opcode = (opcode & 0xff00) | 3;
    } else {
        if (t->u.code >= 256)
            arrerr1();                  /* illegal type to send */
        opcode = (opcode & 0xff00) | B_((UBYTE *)t + 7);
    }
    pokewd(opcode);                     /* moveq #type,d regat */
    pushap(2);                          /* just a word. */
}

/* blitz2.s:5759 baseadd - send base on stack or in reg */
void baseadd(struct var **v, struct type **t, UWORD pmode)
{
    UWORD namelen;          /* asm d2 after getvname */
    UWORD c = getchar3();

    getvname(c, &namelen);
    if (!(flagmask & 2))
        syntaxerr();
    *v = fetchvar(namelen, t, NULL);
    c = getchar3();
    if (c != ')')                       /* we just want a() or whatever */
        syntaxerr();
    if (added)
        noarrayerr();
    pokewd((regat << 9) | fbase[0]);
    pokewd((*v)->offset);
    putalp(pmode);                      /* OK, we've got da base reg. */
}

/* blitz2.s:5781 highadd - send high end of array on stack */
void highadd(struct var *v, UWORD pmode)
{
    WORD off;
    UWORD w;

    pokewd((regat << 9) | fbase[0]);
    off = v->ndims << 2;
    if (varmode)
        off = -off;
    off += v->offset;
    pokewd(off);
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
    pmode = (pmode & 0x00ff) | 0x100;   /* move.b #1,pmode */
    putalp(pmode);
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
struct blitzlib *uselib3(struct blitzlib *l, UWORD libnum)
{
    UBYTE *p;
    ULONG end, dest;

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
    p = (UBYTE *)l + l->maxinfo;
    if (W_(p - 2)) {
        uselib(64335);
        varoff = (varoff + 1) & ~1;
        W_(p - 6) = varoff;
        varoff += 4;
        if (L_(p - 14))
            varoff += 4;
    }

    /* @5885 copy the lib code */
    l->outaddr = lib;                   /* pc of lib */
    p = (UBYTE *)l + l->maxinfo;        /* start of lib */
    end = (ULONG)l + l->size;           /* end of lib */
    if (l->errchkoff && !debugga)
        end = (ULONG)l + l->errchkoff;  /* copy only non-errchks */
    dest = lib;
    for (;;) {
        if (dest >= data1at) {
            nomemleft = -1;
            do {
                p += 2;
                dest += 2;
            } while ((ULONG)p < end);
            break;
        }
        W_(dest) = W_(p);
        p += 2;
        dest += 2;
        if ((ULONG)p >= end)
            break;
    }
    lib = dest;

    /* @5911 relocate library code! */
    if (l->nrelocs) {
        ULONG n = l->nrelocs;
        const ULONG *rp = l->relocs;
        const ULONG *first = NULL;
        ULONG jsrcount = 0;             /* no tokejsrs (yet!) */
        do {
            ULONG reloc = *rp++;
            LONG off;
            if (l->errchkoff && !debugga && reloc >= l->errchkoff)
                continue;               /* .next (do-while: tests --n) */
            if ((WORD)W_((UBYTE *)l + reloc) < 0 && jsrcount == 0) {
                first = rp;
                jsrcount = n;           /* number left to do! */
            }
            off = (LONG)(reloc - l->maxinfo);
            if (off < 0)
                continue;
            off += l->outaddr;
            addoff2(off - pcat);
            if (nomemleft)
                continue;
            L_(off) += l->outaddr - l->maxinfo;
        } while (--n);
        if (jsrcount)
            handlejsrs(l, first - 1, jsrcount);
    }

    /* @5959 var slot */
    if (l->varkind == 0) {
        l->varoff = 0;
    } else {
        varoff = (varoff + 1) & ~1;
        l->varoff = varoff;
        if (libnum == 65235) {
            movestdn[1] = l->varoff;
            pusha3[1] = l->varoff;
            pusha32[1] = l->varoff;
            pulla3[1] = l->varoff;
            geta3[1] = l->varoff;
        }
        if (libnum == 64535) {
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
    uselib2(l, l->initsub, libnum);
    uselib2(l, l->finitsub, libnum);
    return l;
}

/* blitz2.s:6003 fixjsrs - OK, we have to create code and fill in offsets
   for all tokejsrs done through the program.  Lists may grow while being
   processed (tokecode -> maketjsr): done in batches between sentinels. */
void fixjsrs(void)
{
    struct tokejsrnode *till = NULL;    /* do list till this... */
    struct tokejsrnode *node;

    while ((node = tokeslist) != till) {
        struct tokejsrnode *stop = node; /* next till... */
        do {
            struct addrnode *otill = NULL;
            struct addrnode *ofirst;
            while ((ofirst = node->offs) != otill) {
                ULONG addr = tokecode(node->token, node->form);
                if (!nomemleft) {
                    struct addrnode *o;
                    for (o = ofirst; o != otill; o = o->next)
                        L_(o->addr) = addr;
                }
                otill = ofirst;
            }
            node = node->next;
        } while (node != till);
        till = stop;
    }
}

/* .putcode of tokecode: generate a stub at pc */
static ULONG putcode_cute(struct blitzlib *lib, UBYTE *sub)
{
    ULONG old = pc;
    lasta6 = -1;
    cutejsr(lib, sub);
    pokewd(0x4e75);
    return old;
}

static ULONG putcode_lsub(struct blitzlib *lib, ULONG suboff)
{
    ULONG old = pc;
    lasta6 = -1;
    makelibsub(lib, suboff);
    pokewd(0x4e75);
    return old;
}

/* blitz2.s:6047 tokecode - d1=token number, d2=form number!
   return d7, address to jsr! */
ULONG tokecode(UWORD tok, UWORD form)
{
    struct blitzlib *lib;
    UBYTE *sub = findtoke(tok, &lib);
    WORD formsleft;
    ULONG off;

    blitzmode &= ~0x8000;
    if (form & 0x4000) {
        form &= ~0x4000;
        blitzmode |= 0x8000;
    }
    formsleft = form;
    if (W_(sub) == 0)
        goto simple;
    if (B_(sub + 1) & 0x04)
        tokeerr();
    if (B_(sub + 1) & 0x08)
        tokeerr();

    /* OK, it's a complex sorta token... */
    sub += 6;                           /* skip node */
    for (;;) {
        UWORD info = W_(sub);
        sub += 2;
        if ((WORD)info < 0)
            tokeerr();
        sub = (UBYTE *)(((ULONG)sub + (info & 255) + 1) & ~1UL);
        if (--formsleft < 0)
            break;
        sub = skiplibreg(sub) + 12;     /* .loop2 / .skip */
    }
    if (W_(sub) != 0)
        goto go4it;                     /* libs to fetch.... */
    if ((L_(sub + 2) | L_(sub + 10)) != 0)
        goto go4it;                     /* error chx... */
    if (B_(sub + 9) & 1)
        goto go4it;                     /* ditto... */
    off = L_(sub + 6);                  /* sub offset */
    if ((W_((UBYTE *)lib + off) & 0xf000) == 0xa000)
        goto go4it;
penis:
    return off - lib->maxinfo + lib->outaddr;
go4it:
    return putcode_cute(lib, sub);

simple:
    if (formsleft)
        tokeerr();
    if (W_(sub + 6))
        goto be4it;
    if ((L_(sub + 8) | L_(sub + 16)) != 0)
        goto be4it;
    if (B_(sub + 15) & 1)
        goto be4it;
    off = L_(sub + 12);
    if ((W_((UBYTE *)lib + off) & 0xf000) != 0xa000)
        goto penis;
be4it:
    return putcode_lsub(lib, (ULONG)(sub - (UBYTE *)lib));
}

/* blitz2.s:6138 freetlist */
void freetlist(void)
{
    struct tokejsrnode *t = tokeslist;

    tokeslist = NULL;
    while (t) {
        struct tokejsrnode *nx;
        struct addrnode *offs = t->offs;
        while (offs) {
            struct addrnode *o = offs;
            offs = offs->next;
            FreeMem(o, 8);
        }
        nx = t->next;
        FreeMem(t, 12);
        t = nx;
    }
}

/* blitz2.s:6158 handlejsrs - a2=first, d2=how many to do. */
void handlejsrs(struct blitzlib *l, const ULONG *relocs, ULONG count)
{
    do {
        ULONG reloc = *relocs++;
        UWORD form = W_((UBYTE *)l + reloc);
        UWORD tok;
        if ((WORD)form >= 0)
            continue;
        if (l->errchkoff && !debugga && reloc >= l->errchkoff)
            continue;
        form &= 0x7fff;                 /* to offset number */
        tok = W_((UBYTE *)l + reloc + 2);
        reloc = reloc - l->maxinfo + l->outaddr;
        maketjsr(reloc, tok, form, NULL);
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
    const UWORD *deps;

    if (!off)
        return;
    deps = (const UWORD *)((UBYTE *)l + off + 6);
    for (;;) {
        UWORD libnum = *deps++;
        if (!libnum)
            break;
        if (libnum != self)
            uselib(libnum);
        deps++;
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
    UBYTE *sub;
    UWORD cur = tok;

    for (l = firstlib; ; l = l->next) {
        if (!l)
            notokerr(tok);
        if (cur >= l->firsttoken && cur < l->lasttoken)
            break;
    }
    /* lib found!  (system and user libs are walked the same way; the
       original's user lib variant has its token check commented out) */
    sub = l->subs;
    while (cur != l->firsttoken) {
        cur--;
        sub = *(UBYTE **)(sub + 2);
    }
    uselib3(l, l->libnum);              /* set lib# */
    if (libp)
        *libp = l;
    return sub;
}

/* blitz2.s:6322 loadlibgroup - load a group of libraries!
   d0=len, d1=name; return d0=address */
APTR loadlibgroup(ULONG len, STRPTR name, struct node **tail)
{
    BPTR fh;
    UBYTE *buf, *p;

    if (!len)
        return NULL;
    fh = Open(name, MODE_OLDFILE);
    if (!fh)
        return NULL;
    buf = doallocmem(len, MEMF_PUBLIC);
    Read(fh, buf, len);
    Close(fh);

    p = buf;
    for (;;) {
        ULONG filelen = L_(p);
        ULONG size;
        struct blitzlib *s;
        UBYTE *r;
        p += 4;
        if (!filelen)
            break;
        s = (struct blitzlib *)(p + 32);
        (*tail)->next = (struct node *)s;   /* link in. */
        *tail = (struct node *)s;
        p += filelen;
        /* fix stuff - done by loadalib */
        size = L_((UBYTE *)s - 4) * 4;  /* number of longwords -> bytes */
        s->size = size;
        s->nrelocs = 0;                 /* no relocs */
        r = (UBYTE *)s + size + 12;     /* reloc info. */
        if (r >= p)
            continue;                   /* is it over? */
        s->nrelocs = L_(r - 8);         /* number of relocs. */
        s->relocs = (ULONG *)r;         /* pointer to relocs. */
    }
    return buf;
}

/* blitz2.s:6374 loadlibs - use libslist to load in used libs dir contents */
void loadlibs(void)
{
    struct node *tail = (struct node *)&firstlib;

    if (!ext_loadlibs(&tail)) {
        acidlibsat = loadlibgroup(acidlibs, acidname, &tail);
        deflibsat = loadlibgroup(deflibs, defname, &tail);
    }
    /* all libs read in, now to link 'em up. */
    tail->next = NULL;
    blitzlibsat = tail;
    loadblitzlibs();                    /* falls through */
}

/* blitz2.s:6392 loadblitzlibs - load in blitz libs only! */
void loadblitzlibs(void)
{
    UBYTE *dst = undobuffer;
    const char *src = libsdir;
    UBYTE *namepos;
    struct node *node = (struct node *)&libslist; /* lib names! */
    struct node *tail = blitzlibsat;

    if (ext_loaduserlibs(tail)) {
        fixlibs();
        return;
    }
    while ((*dst++ = *src++) != 0)
        ;
    if (!libsdir[0]) {
        /* asm: a0 == undobuffer after reading undobuffer[-1] */
        dst = undobuffer;
    } else {
        dst -= 2;
        /* DIFF: '/' as in the compiler of the CD (2.10, docs/v2.10, #2);
           blitz2.s tests ':', and the default libsdir already ends with
           '/': "blitz2:userlibs//dir" (AmigaDOS: parent directory) */
        if (*dst++ != '/')
            *dst++ = '/';
    }
    namepos = dst;                      /* store first pos for chars */

    for (;;) {
        struct libsnode *ln;
        UBYTE *fpos;
        BPTR lk;

        node = node->next;
        if (!node) {
            tail->next = NULL;
            fixlibs();
            return;
        }
        ln = (struct libsnode *)node;
        if (!ln->used)
            continue;
        dst = namepos;                  /* add dir name to libsdir */
        src = ln->name;
        while ((*dst++ = *src++) != 0)
            ;
        fpos = dst;                     /* pointer for lib name */
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
                loadalib((STRPTR)undobuffer, &tail);
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
    ULONG size;
    struct blitzlib *l;
    LONG got;

    if (!fh)
        return;                         /* .openerr */
    /* Fixed (#3): a header or structure that cannot be read completely
       leaves the library out (blitz2.s reads on with what the buffers
       held) */
    if (Read(fh, optstuff, 32) != 32) {
        Close(fh);
        return;
    }
    size = L_(optstuff + 28) << 2;      /* long words to read */
    l = doallocmem(size, MEMF_PUBLIC);
    if ((ULONG)Read(fh, l, size) != size) { /* read in struct */
        FreeMem(l, size);
        Close(fh);
        return;
    }
    (*tail)->next = (struct node *)l;
    *tail = (struct node *)l;
    l->size = size;
    l->nrelocs = 0;
    got = Read(fh, optstuff, 12);
    /* Fixed (#3): signed compare - a read error (-1) ends the reading like
       a short file (blitz2.s: cmp.l #12,d0 / bcs, unsigned) */
    if (got >= 12) {
        ULONG cnt = L_(optstuff + 4);
        ULONG rsize = cnt << 2;         /* length of reloc info */
        l->nrelocs = cnt;
        l->relocs = doallocmem(rsize, MEMF_PUBLIC); /* pointer to reloc mem */
        Read(fh, l->relocs, rsize);
    }
    Close(fh);                          /* .closeup */
}

/* blitz2.s:6525 fixlibs - fill in blanks in lib list: tokens, links etc */
void fixlibs(void)
{
    struct node *tail = (struct node *)lasttoken;
    struct blitzlib *l;
    jmp_buf jb;
    jmp_buf *volatile olderr;

    nummaxs = 0;
    for (l = firstlib; l; l = l->next) {
        WORD token = l->libnum;
        UBYTE *p;
        ULONG link, dummy;

        if (token >= 0) {               /* user lib */
            token = (token << 7) + 1;
            l->firsttoken = token;
        }
        /* .syslib: system libs (negative lib number) keep the token range
           stored in the lib file (e.g. memlib $c000.., see the equates at
           blitz2.s 72-250); only user libs get libnum*128+1 above */
        p = l->subs;
        link = (ULONG)&dummy;           /* dummy first link (asm: temp1) */
        for (;;) {
            UWORD kind = W_(p);
            if ((WORD)kind < 0)
                break;                  /* .next */
            L_(link) = (ULONG)p;
            link = (ULONG)p + 2;        /* last link for subs */
            p += 6;
            if (kind == 8)
                goto addtoke;
            if ((kind & 15) == 0) {
                /* skip sys sub */
                p = skiplibreg(p) + 12;
                token++;                /* in case it's a user lib */
                continue;
            }
            if (kind & 4) {             /* amiga lib */
                p += 4;
                while (!(*p++ & 0x80))
                    ;
                p = aligna1(p);
                goto addtoke;
            }
            /* process standard function or statement */
            for (;;) {
                UWORD f = W_(p);
                p += 2;
                if ((WORD)f < 0)
                    break;
                p += f & 255;
                p = aligna1(p);
                p = skiplibreg(p) + 12;
            }
        addtoke:
            tail->next = (struct node *)p;
            tail = (struct node *)p;
            p += 4;
            W_(p) = token;
            p += 2;
            while (*p++)
                ;
            while (*p++)
                ;
            p = aligna1(p);
            token++;
        }
        if (l->libnum >= 0)
            l->lasttoken = token;
        p += 2;
        if (L_(p)) {
            p += 4;
            nummaxs++;
            p += 22;
        } else
            p += 4;
        l->maxinfo = p - (UBYTE *)l;
    }
    tail->next = NULL;

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
    struct blitzlib *l = (struct blitzlib *)blitzlibsat->next;

    blitzlibsat->next = NULL;
    while (l) {
        struct blitzlib *nx;
        if (l->nrelocs)
            FreeMem(l->relocs, l->nrelocs << 2);
        nx = l->next;
        FreeMem(l, l->size);
        l = nx;
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
    ext_freelibs();
}

/* blitz2.s:6680 freemacs */
void freemacs(void)
{
    struct macro *last = lastresmac;
    struct macro *p = last->next;

    last->next = NULL;
    while (p) {
        struct macro *m = p;
        if (m->textlen)
            FreeMem(m->text, m->textlen);
        p = p->next;
        FreeMem(m, m->len);
    }
}

/* blitz2.s:6701 freeasms */
void freeasms(void)
{
    struct asmnode *p = firstasm;

    firstasm = NULL;
    while (p) {
        struct asmnode *n = p;
        p = p->next;
        FreeMem(n, n->size);
    }
}

/* ============================================================ data1 */

/* blitz2.s:6714 pokedata1b - byte into data1 */
void pokedata1b(UBYTE b)
{
    if (data1 >= data2at) {
        nomemleft = -1;
        data1 += 1;
        return;
    }
    B_(data1) = b;
    data1 += 1;
}

/* blitz2.s:6725 pokedata1l */
void pokedata1l(ULONG l)
{
    pokedata1((UWORD)(l >> 16));
    pokedata1((UWORD)l);                /* falls through into pokedata1 */
}

/* blitz2.s:6729 pokedata1 - poke word in d1 into data block */
void pokedata1(UWORD w)
{
    if (data1 >= data2at) {
        nomemleft = -1;
        data1 += 2;
        return;
    }
    W_(data1) = w;
    data1 += 2;
}
