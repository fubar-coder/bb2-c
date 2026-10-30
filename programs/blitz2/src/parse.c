/*
 * parse.c - chunk 05, blitz2.s 6740-8450: static structs / data start,
 * savefile, reloc offset list, getchar family (conditional compilation,
 * `~expr~`), err + error stubs, char classes, name lists, freeing,
 * fetchvar, calcvar.  See notes/survey-05.md and parse.h.
 */
#include <string.h>
#include <stdlib.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "blitz2.h"

/* long read of a UWORD template (asm move.l label,d1); avoids L_() on
   UWORD arrays (strict aliasing) */
#define TL(p) (((ULONG)(p)[0] << 16) | (p)[1])

/* ================================================== persistent registers */

UBYTE *srcptr;                  /* asm a5 of the parser */

/* ================================================== error recovery */

jmp_buf *errjmp;                /* asm errstack/errcont */
const char *errmsg;             /* asm d0 in the quiet (direct mode) path */
WORD ezerr;                     /* blitz2.s:7636 for simple errors (ie not during compile) */

/* ================================================== getchar data */

static UBYTE cmakebuff[24];     /* blitz2.s:7037 decimal text of ~expr~ (ds 12 = 12 words)
                                   */
static UBYTE *cmakea5;          /* blitz2.s:7038 */
UWORD cmake;                    /* blitz2.s:7039 */
static UBYTE cmakebak[128];     /* blitz2.s:7040 namebuff backup */
UWORD ciflineat;                /* blitz2.s:7145 */

/* blitz2.s:7181 gs - control char -> character map inside strings:
   26 zero bytes, then ae oe ue Ae Oe Ue (Latin-1) */
static const UBYTE gs[32] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    228, 246, 252, 196, 214, 220
};

/* ================================================== code templates
   (tools/asmlisting.py, checked against the shipped binary) */

/* blitz2.s:8077-8080, 14 bytes */
static UWORD zarrchk[] = {
    0xB0AD, 0x0000,                    /* +0   zarrchk cmp.l 0(a5),d0 */
    0x6500, 0x0008,                    /* +4   arrchk2 bcs.w arrchk3 */
    0x4EB9, 0x0000, 0x0000,            /* +8   arrchkf jsr 0 */
};
#define arrchk2        (&zarrchk[2])   /* +4 */
#define arrchkf        (&zarrchk[4])   /* +8 */

/* blitz2.s:8157-8168, 30 bytes */
static UWORD arrmul1[] = {
    0x2F00,                            /* +0   arrmul1 move.l d0,-(a7) */
    0x3000,                            /* +2   arrmul2 move d0,d0 */
    0xC0ED, 0x0000,                    /* +4   arrmul3 mulu 0(a5),d0 */
    0xC0ED, 0x0000,                    /* +8   arrmul4 mulu 0(a5),d0 */
    0x4840,                            /* +12  arrmul5 swap d0 */
    0x4240,                            /* +14  arrmul6 clr d0 */
    0xD080,                            /* +16  arrmul7 add.l d0,d0 */
    0x201F,                            /* +18  arrmul8 move.l (a7)+,d0 */
    0x246D, 0x0000,                    /* +20  listcode1 move.l 0(a5),a2 */
    0x246A, 0xFFE0,                    /* +24  listcode2 move.l -32(a2),a2 */
    0x504A,                            /* +28  listarfix addq #8,a2 */
};
#define arrmul2        (&arrmul1[1])
#define arrmul3        (&arrmul1[2])
#define arrmul4        (&arrmul1[4])
#define arrmul5        (&arrmul1[6])
#define arrmul6        (&arrmul1[7])
#define arrmul7        (&arrmul1[8])
#define arrmul8        (&arrmul1[9])
#define listcode1      (&arrmul1[10])
#define listcode2      (&arrmul1[12])
#define listarfix      (&arrmul1[14])

/* blitz2.s:8423 calcvar2.pfix: move.l (a2),a2 */
static constexpr UWORD calcvar_pfix = 0x2452;

/* blitz2.s:8442-8449, 14 bytes */
static UWORD brchkcode0[] = {
    0xB07C, 0x0000,                    /* +0  brchkcode0 cmp #0,d0 */
    0xB07C, 0x0000,                    /* +4  brchkcode  cmp #0,d0 (patched) */
    0x6504,                            /* +8  bcs.s .ok */
    0x7002,                            /* +10 moveq #2,d0 */
    0x4E40,                            /* +12 trap #0 */
};
#define brchkcode      (&brchkcode0[2])

/* ================================================== object file data */

static const char infoname[] = ".info";     /* blitz2.s:6992 */
static constexpr ULONG sysdebug = 0x3f0;    /* blitz2.s:6995 HUNK_SYMBOL */

/* blitz2.s:6989 objicon incbin obj.info (383 bytes) .. objiconf */
static const UBYTE objicon[] = {
  0xe3, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x19, 0x00, 0x11,
  0x00, 0x2e, 0x00, 0x18, 0x00, 0x04, 0x00, 0x03, 0x00, 0x01, 0x00, 0x22,
  0x73, 0x68, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x03, 0x22, 0x00, 0x21, 0x3c, 0x28, 0x00, 0x22, 0x6e, 0xc0, 0x00, 0x00,
  0x00, 0x12, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2e,
  0x00, 0x17, 0x00, 0x02, 0x00, 0x01, 0x8e, 0x80, 0x03, 0x00, 0x00, 0x00,
  0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x80, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x80, 0x00, 0x00, 0x3f, 0xbb, 0x40, 0x80, 0x1f, 0xc0, 0x00,
  0x00, 0x00, 0x80, 0x3f, 0xc0, 0x0f, 0xbb, 0x40, 0x80, 0x1f, 0xc0, 0x00,
  0x00, 0x00, 0x8e, 0x00, 0xf8, 0x00, 0x00, 0x00, 0x8f, 0x01, 0xff, 0x00,
  0x3b, 0x40, 0x81, 0xe3, 0xf7, 0xc0, 0x00, 0x00, 0x80, 0x7f, 0xf8, 0x70,
  0x3b, 0x40, 0x80, 0x0f, 0x7c, 0xe0, 0x00, 0x00, 0x80, 0x00, 0xfd, 0x80,
  0x00, 0x00, 0x80, 0x01, 0xf6, 0x00, 0xfb, 0x40, 0x80, 0x03, 0xef, 0x00,
  0x00, 0x00, 0x80, 0x07, 0x87, 0x80, 0x3b, 0x40, 0x80, 0x0f, 0x01, 0xc0,
  0x00, 0x00, 0x80, 0x03, 0xf0, 0x70, 0x1d, 0x40, 0x80, 0x00, 0x18, 0x1c,
  0x00, 0x00, 0x80, 0x00, 0x10, 0x06, 0x03, 0x40, 0x80, 0x00, 0x20, 0x1f,
  0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x01, 0x40, 0x80, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x3f, 0xfc, 0x01,
  0xbb, 0x44, 0x00, 0x60, 0xfe, 0x00, 0x00, 0x04, 0x00, 0xcf, 0xff, 0x01,
  0xbb, 0x44, 0x1f, 0xef, 0xff, 0x00, 0x00, 0x04, 0x33, 0xff, 0xff, 0xe0,
  0x00, 0x04, 0x37, 0xfe, 0x7f, 0xfc, 0x1b, 0x44, 0x0f, 0x9c, 0xff, 0xff,
  0x00, 0x04, 0x03, 0xe3, 0xff, 0xff, 0xdb, 0x44, 0x00, 0x7f, 0xbf, 0xff,
  0x80, 0x04, 0x00, 0x0f, 0x3f, 0xfe, 0x00, 0x04, 0x00, 0x06, 0x7f, 0xf8,
  0x1b, 0x44, 0x00, 0x0c, 0xf7, 0xfc, 0x00, 0x04, 0x00, 0x19, 0xf9, 0xfe,
  0x1b, 0x44, 0x00, 0x33, 0xfe, 0x7f, 0x00, 0x04, 0x00, 0x0f, 0xff, 0xbf,
  0xcd, 0x44, 0x00, 0x03, 0xef, 0xef, 0xf0, 0x04, 0x00, 0x00, 0x7f, 0xdf,
  0xfb, 0x44, 0x00, 0x00, 0xff, 0xa7, 0xfc, 0x04, 0x00, 0x00, 0x7e, 0x3f,
  0xfd, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x7f, 0xff, 0xff, 0xff,
  0xff, 0xfc, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x04
};

/* =================================================================
 * static structures / data start
 * ================================================================= */

/* blitz2.s:6740 calcstatic - calculate static structures stuff - put num of
   structs d4.w, address of data1 table in d3.l
   data1: .w=structsize,.w=offset */
UWORD calcstatic(ULONG *tab)
{
    return calcstatic2(firstvar, tab);      /* falls through into calcstatic2 */
}

/* blitz2.s:6746 calcstatic2 - same for the var list `list` (asm a2) */
UWORD calcstatic2(struct var *list, ULONG *tab)
{
    struct var *v;
    UWORD count = 0;

    *tab = data1;                           /* asm d3 */
    for (v = list; v; v = v->next) {
        struct type *t;
        if (v->flags != 0)                  /* tst 6(a2): pointer/array */
            continue;
        t = v->type;
        if (t->u.code < 256)                /* not a NewType */
            continue;
        pokedata1(v->offset);
        pokedata1(t->size);
        count++;
    }
    if (count) {
        uselib(65335);                      /* use statics lib */
        uselib(65530);                      /* use memlib (asm: tail bra uselib) */
    }
    return count;
}

/* blitz2.s:6770 datastart - set up a2 (d3.l) and d4 (d4.w) */
void datastart(ULONG tab, UWORD n)
{
    pokewd(*numtoa2);                       /* move.l #tab,a2 */
    addoff();
    pokel(tab);
    endarray[1] = n - 1;                    /* move #n-1,d4 (patched in place) */
    pokel(TL(endarray));
}

/* blitz2.s:6782 nextopt is an asm macro (ring buffer advance of the optimizer
   buffer: x += 8; if x == optend x = optbuff).  Not used in this chunk. */

/* =================================================================
 * executable file
 * ================================================================= */

/* blitz2.s:6790 mywrite - Write, on error close the file and execerr.
   asm: d1 = fh, d2 = buf, d3 = len, closes d7 (always == fh here). */
void mywrite(BPTR fh, APTR buf, LONG len)
{
    if (Write(fh, buf, len) == len)
        return;
    Close(fh);
    execerr();                              /* falls into execerr */
}

/* blitz2.s:6797 execerr */
void execerr(void)
{
    makeexecerr();
}

/* blitz2.s:6800 savefile - make all abs refs pcat relative, write the
   executable (hunk header, code, reloc32, symbols, end) and the icon.
   Assembled because cripple == 0. */
void savefile(void)
{
    jmp_buf jb, *olderr = errjmp;
    BPTR fh;
    struct offnode *off;
    ULONG longs;

    errjmp = &jb;
    if (setjmp(jb)) {                       /* asm: errcont = .rts */
        errjmp = olderr;                    /* DIFF: asm never restored errstack */
        return;
    }

    sleepon();
    anyerrs = -1;                           /* forces a recompile before Run */
    if (!nomemleft) {
        UBYTE *code = (UBYTE *)pcat;
        ULONG base = pcat + noinits;
        for (off = firstoff; off; off = off->next) {
            if ((WORD)W_(code + off->offset) < 0) /* tst 0(a1,d0.l) / bmi .noway */
                continue;
            L_(code + off->offset) -= base;
        }
    }
    /* @6824 .fixdone */
    fh = Open((STRPTR)execname, MODE_NEWFILE);
    if (!fh)
        execerr();

    longs = (objlen - noinits + 3) >> 2;
    *tstsize1 = longs;
    *tstsize2 = longs;
    mywrite(fh, tsthead, 32);

    mywrite(fh, (APTR)(pcat + noinits), (objlen - noinits + 3) & 0xfffffffc);

    *tstsize3 = numoffs;
    if (numoffs) {
        ULONG skip = noinits;
        mywrite(fh, tstend, 12);
        for (off = firstoff; off; off = off->next) {
            off->offset -= skip;
            mywrite(fh, &off->offset, 4);
            off->offset += skip;
        }
        *tstsize3 = 0;
        mywrite(fh, tstsize3, 4);
    }

    /* @6889 .skip: debug info */
    if (optreqga16.Flags & GFLG_SELECTED) {
        struct label *l;
        mywrite(fh, (APTR)&sysdebug, 4);
        for (l = firstlabel; l; l = l->next) {
            if (((ULONG)l->refs & 1) && (ULONG)l->refs != 1)
                continue;                   /* used as constant, not #const */
            {
                UWORD padlen = ((UWORD)l->len - 19 + 3) & 0xfffc; /* long align */
                UWORD nbytes = (padlen >> 2) << 2;              /* #bytes */
                UBYTE nb[256 + 4];
                ULONG val = padlen >> 2;    /* asm: via temp1 */
                mywrite(fh, &val, 4);
                /* DIFF: the asm wrote d4 bytes straight from the node (up to
                   3 bytes past the allocation); C pads a copy with zeros */
                memset(nb, 0, sizeof(nb));
                memcpy(nb, l->name, l->len - 19);
                mywrite(fh, nb, padlen);
                if (nbytes - padlen)        /* never true (as in the asm) */
                    mywrite(fh, &zero, nbytes - padlen);
                val = l->pc - pcat - noinits;
                mywrite(fh, &val, 4);
            }
        }
        mywrite(fh, &zero, 4);
    }
    /* @6950 .nodebug */
    mywrite(fh, tstdone, 4);
    Close(fh);

    if (optreqga9.Flags & GFLG_SELECTED) {
        /* OK, create an icon for the object code! */
        char *ext = execname + strlen(execname);
        char *end = ext;                    /* to later null out! */
        BPTR lk;
        strcpy(ext, infoname);
        lk = Lock((STRPTR)execname, ACCESS_READ);
        if (lk)
            UnLock(lk);                     /* exists: don't overwrite */
        else {
            fh = Open((STRPTR)execname, MODE_NEWFILE);
            if (fh) {
                Write(fh, (APTR)objicon, sizeof(objicon));
                Close(fh);
            }
        }
        *end = 0;
    }
    pointeron();
    errjmp = olderr;                        /* DIFF: see above */
}

/* blitz2.s:6997 addoff - add pc to list of offsets */
void addoff(void)
{
    addoff2(pc - pcat);
}

/* blitz2.s:7006 addoff2 - add d2 to linked list of all offsets */
void addoff2(ULONG off)
{
    struct offnode *n = doallocmem(8, 1);
    numoffs++;
    n->next = firstoff;
    firstoff = n;
    n->offset = off;
}

/* blitz2.s:7022 killoffs - kill all offsets */
void killoffs(void)
{
    struct offnode *o = firstoff, *dead;
    firstoff = NULL;
    while (o) {
        dead = o;
        o = o->next;
        FreeMem(dead, 8);
    }
    numoffs = 0;
}

/* =================================================================
 * character fetch
 * ================================================================= */

/* blitz2.s:7188 gchar2 - classification tail of getchar3/reget.  Only the
   in-quote mapping changes the value; the Z flag is eos(). */
static UWORD gchar2(UWORD c)
{
    if (c == 0)
        return 0;
    if (quoteflag) {
        if ((UBYTE)c < 32) {                /* cmp.b #32,d0 (byte compare) */
            if (c < 32)
                c = gs[c];                  /* move.b gs(pc,d0),d0 */
            /* DIFF: a token with low byte < 32 inside quotes indexed gs with a
               negative word (reads code bytes); cannot happen with tokenised
               source (strings are not tokenised), C leaves it unchanged */
        }
        return c;
    }
    return c;
}

/* parse.h eos - asm Z=1 after getchar3/reget */
BOOL eos(UWORD c)
{
    if (c == 0)
        return TRUE;
    if (quoteflag)
        return FALSE;
    return c == 0x8000 + 34 || c == ':';    /* else token, ':' */
}

/* blitz2.s:7042 getchar */
UWORD getchar_(void)
{
    UWORD c;

    for (;;) {
        prevloc = srcptr;
        oldqflag = quoteflag;
        c = getcharb();
        lastchar = c;
        if (c == 0)
            return 0;
        if (c & 0x8000) {
            if (c == 0x801c) {              /* .cnif */
                ciflineat = linenumat;
                if (!comflag) { connest++; continue; }     /* .incnest */
                docnif(getchar3());
                continue;
            }
            if (c == 0x801d) {              /* .csif */
                ciflineat = linenumat;
                if (!comflag) { connest++; continue; }
                docsif(getchar3());
                continue;
            }
            if (c == 0x801e) {              /* .celse */
                if (connest == 0)
                    comflag = ~comflag;
                continue;
            }
            if (c == 0x801f) {              /* .cend */
                if (--connest >= 0)
                    continue;
                connest++;
                if (concomsp == concomstack)
                    nociferr();
                concomsp--;
                comflag = *concomsp;
                continue;
            }
        }
        /* .done2 */
        if (!comflag)
            continue;
        if (c == '~') {                     /* .cmake */
            UWORD term;
            ULONG val;
            char *end;
            if (cmake)
                return c;
            cmake = ~cmake;
            /* asm: movem.l d0-d7/a0-a6 - only srcptr matters in C and it is
               set to cmakebuff below anyway */
            memcpy(cmakebak, namebuff, 128);
            val = evalconst2(&term);        /* get constant into d3 */
            if (term != '~')
                syntaxerr();                /* cmake stays set (cleared per pass) */
            cmakea5 = srcptr;
            end = makelong((char *)cmakebuff, val);
            (*(UBYTE *)end)++;              /* NUL -> 1 = end of cmake chars */
            cmake = ~cmake;
            memcpy(namebuff, cmakebak, 128);
            srcptr = cmakebuff;
            continue;
        }
        if (c == 2) {                       /* end of UsePath text */
            srcptr = usedfrom;
            continue;
        }
        if (c != 1)
            return c;
        /* End of Cmake chars got */
        srcptr = cmakea5;
    }
}

/* blitz2.s:7147 getcharb - get next character into d0... a5=charpointer
   set hi bit if token, and lo 15 bits to token number (0-32767) */
UWORD getcharb(void)
{
    UWORD c = *srcptr++;
    if (c == 0)
        return 0;
    if (c & 0x80) {
        c = (c << 8) | *srcptr++;
        return c;
    }
    if (c == 34) {                          /* swapquote */
        quoteflag = ~quoteflag;
        return c;
    }
    if (c == ';' && !quoteflag) {           /* comment: skip line */
        while (*srcptr++)
            ;
        lastchar = 0;
        return 0;
    }
    return c;
}

/* blitz2.s:7172 getchar2 - as above and skip spaces */
UWORD getchar2(void)
{
    UWORD c;
    do
        c = getchar_();
    while (!quoteflag && c == 32);
    return c;
}

/* blitz2.s:7184 getchar3 - as above. return z=1 if 0 or ':' (eos()) */
UWORD getchar3(void)
{
    return gchar2(getchar2());
}

/* blitz2.s:7202 reget */
UWORD reget(void)
{
    return gchar2(lastchar);
}

/* blitz2.s:7205 bakup */
void bakup(void)
{
    quoteflag = oldqflag;
    srcptr = prevloc;
}

/* blitz2.s:7209 storeloc - remember character pointer */
void storeloc(UWORD c)
{
    locchar = c;
    locloc = srcptr;
}

/* blitz2.s:7215 resloc - recall char pointer.... */
UWORD resloc(void)
{
    srcptr = locloc;
    lastchar = locchar;
    return locchar;
}

/* =================================================================
 * errors
 * ================================================================= */

/* blitz2.s:7641 err / comperror - compile time error... */
void err(const char *msg)
{
    if (!dirmode) {
        pointeron();
        if (!ezerr) {
            if (cfetchmode)
                libat = oldlibat;           /* fix up lib thing. */
            if (constmode)
                asmerr2();
            if (firstlocal || firstglob)
                droplocals();
            anyerrs = -1;
            menuret = linenumat;
            endstop();
        }
        ed_error(msg);                      /* comdata+12, a0 = msg */
    }
    /* .quiet: d0 = msg */
    errmsg = msg;
    if (!errjmp)                            /* DIFF: see parse.h */
        exit(RETURN_FAIL);
    longjmp(*errjmp, 1);                    /* a7 = errstack, jmp (errcont) */
}

/* blitz2.s:7360 asmerr2 / 7361 asmerr - linenumat = 12(a2).w */
void asmerr2(void)
{
    asmerr((struct asmnode *)asmbuff);      /* move.l asmbuff,a2 */
}

void asmerr(struct asmnode *a)
{
    linenumat = a->line;
}

/* blitz2.s:7294 opterr - opchars/opchars2 are never patched in blitz2.s */
NORETURN void opterr(void) { err("Optimizer Error! - $ffff,$ffff."); }

/* blitz2.s:7317 interr2 */
NORETURN void interr2(void)
{
    linenumat = intline;
    err("SetInt without End SetInt");
}

/* blitz2.s:7488 noliberr - liberrnum patched in place by hexascii */
static char noliberrmsg[] = "Library not Found : $ffff.";
NORETURN void noliberr(UWORD libnum)
{
    hexascii(libnum, (UBYTE *)noliberrmsg + 21);   /* liberrnum */
    err(noliberrmsg);
}

/* blitz2.s:7494 notokerr - tokerrnum patched in place */
static char notokerrmsg[] = "Token Not Found : $ffff.";
NORETURN void notokerr(UWORD token)
{
    hexascii(token, (UBYTE *)notokerrmsg + 19);    /* tokerrnum */
    err(notokerrmsg);
}

/* blitz2.s:7523 macenderr */
NORETURN void macenderr(void)
{
    linenumat = macline;
    err("Macro without End Macro");
}

/* blitz2.s:7221 errs: the plain `bsr err / dc.b 'text'` stubs */
NORETURN void badstoperr(void) { err("Debugger must be enabled for STOP"); }                 /* blitz2.s:7222 */
NORETURN void makeexecerr(void) { err("Error creating Executable"); }                        /* blitz2.s:7225 */
NORETURN void fornexterr(void) { err("Duplicate For...Next Error"); }                        /* blitz2.s:7228 */
NORETURN void excerr2(void) { err("Can't Exchange different types"); }                       /* blitz2.s:7231 */
NORETURN void excerr(void) { err("Can't Exchange NewTypes"); }                               /* blitz2.s:7234 */
NORETURN void arrerr1(void) { err("Illegal Array type"); }                                   /* blitz2.s:7237 */
NORETURN void nocenderr(void) { err("CNIF/CSIF without CEND"); }                             /* blitz2.s:7240 */
NORETURN void referr(void) { err("Label reference out of context"); }                        /* blitz2.s:7243 */
NORETURN void illlaberr(void) { err("Label has been used as a Constant"); }                  /* blitz2.s:7246 */
NORETURN void selerrz(void) { err("Select without End Select"); }                            /* blitz2.s:7249 */
NORETURN void unterr1(void) { err("Until without Repeat"); }                                 /* blitz2.s:7252 */
NORETURN void unterr2(void) { err("Repeat Block too large"); }                               /* blitz2.s:7255 */
NORETURN void unterr3(void) { err("Repeat without Until"); }                                 /* blitz2.s:7258 */
NORETURN void vbokerr1(void) { err("AbortVBOK MUST be in a SETINT 5"); }                     /* blitz2.s:7261 */
NORETURN void errerr1(void) { err("SetErr not allowed in Procedures"); }                     /* blitz2.s:7264 */
NORETURN void errerr2(void) { err("Can't nest SetErr"); }                                    /* blitz2.s:7267 */
NORETURN void errerr3(void) { err("End SetErr without SetErr"); }                            /* blitz2.s:7270 */
NORETURN void noarrayerr(void) { err("Array not found"); }                                   /* blitz2.s:7273 */
NORETURN void nolisterr(void) { err("Array is not a List"); }                                /* blitz2.s:7276 */
NORETURN void defaerr(void) { err("Default without Select"); }                               /* blitz2.s:7279 */
NORETURN void eselerr(void) { err("End Select without Select"); }                            /* blitz2.s:7282 */
NORETURN void cbraerr(void) { err("Previous Case Block too Large"); }                        /* blitz2.s:7285 */
NORETURN void caseerr(void) { err("Case Without Select"); }                                  /* blitz2.s:7288 */
NORETURN void tokeerr(void) { err("Illegal TokeJsr token number"); }                         /* blitz2.s:7291 */
NORETURN void blitzerr(void) { err("Only Available in Blitz mode"); }                        /* blitz2.s:7299 */
NORETURN void amigaerr(void) { err("Only Available in Amiga mode"); }                        /* blitz2.s:7302 */
NORETURN void interr4(void) { err("Illegal Interrupt Number"); }                             /* blitz2.s:7305 */
NORETURN void interr5(void) { err("Illegally nested Interrupts"); }                          /* blitz2.s:7308 */
NORETURN void interr3(void) { err("Can't use Set/ClrInt in Local Mode"); }                   /* blitz2.s:7311 */
NORETURN void interr(void) { err("End SetInt without SetInt"); }                             /* blitz2.s:7314 */
NORETURN void conmemerr(void) { err("Expression too Complex"); }                             /* blitz2.s:7321 */
NORETURN void pserr(void) { err("Precedence Stack Overflow"); }                              /* blitz2.s:7324 */
NORETURN void clasherr(void) { err("Clash in Residents"); }                                  /* blitz2.s:7327 */
NORETURN void reserr(void) { err("Can't Load Resident"); }                                   /* blitz2.s:7330 */
NORETURN void dupofferr(void) { err("Duplicate Offset Error"); }                             /* blitz2.s:7333 */
NORETURN void notparerr(void) { err("Not Enough Parameters"); }                              /* blitz2.s:7336 */
NORETURN void badconterr(void) { err("Cont not Currently Available"); }                      /* blitz2.s:7339 */
NORETURN void dirliberr(void) { err("Library not Available in Direct Mode"); }               /* blitz2.s:7342 */
NORETURN void baddirerr(void) { err("Illegal direct mode command"); }                        /* blitz2.s:7345 */
NORETURN void nodirmem(void) { err("Not enough room in direct mode buffer"); }               /* blitz2.s:7348 */
NORETURN void illdirerr(void) { err("Can't Create in Direct Mode"); }                        /* blitz2.s:7351 */
NORETURN void nosuperr(void) { err("Not Supported"); }                                       /* blitz2.s:7354 */
NORETURN void freeerr(void) { err("Illegal Token"); }                                        /* blitz2.s:7357 */
NORETURN void mbovererr(void) { err("Macro Buffer Overflow"); }                              /* blitz2.s:7365 */
NORETURN void illvgerr(void) { err("Illegal VGoto/VGosub Parameter"); }                      /* blitz2.s:7368 */
NORETURN void notqerr(void) { err("No Terminating Quote"); }                                 /* blitz2.s:7371 */
NORETURN void illcgeterr(void) { err("Can't Use Constant"); }                                /* blitz2.s:7374 */
NORETURN void illconerr(void) { err("Can't Assign Constant"); }                              /* blitz2.s:7377 */
NORETURN void redimerr(void) { err("Illegal number of Dimensions"); }                        /* blitz2.s:7380 */
NORETURN void illeolerr(void) { err("Garbage at End of Line"); }                             /* blitz2.s:7383 */
NORETURN void illelseerr(void) { err("Illegal Else in While Block"); }                       /* blitz2.s:7386 */
NORETURN void illgotoerr(void) { err("Can't Goto/Gosub a Procedure"); }                      /* blitz2.s:7389 */
NORETURN void illtraperr(void) { err("Illegal Trap Vector"); }                               /* blitz2.s:7395 */
NORETURN void illimmerr(void) { err("Illegal Immediate Value"); }                            /* blitz2.s:7398 */
NORETURN void illabserr(void) { err("Illgeal Absolute"); }                                   /* blitz2.s:7401 */
NORETURN void illdiserr(void) { err("Illegal Displacement"); }                               /* blitz2.s:7404 */
NORETURN void illsizeerr(void) { err("Illegal Assembler Instruction Size"); }                /* blitz2.s:7407 */
NORETURN void illeaerr(void) { err("Illegal Assembler Addressing Mode"); }                   /* blitz2.s:7410 */
NORETURN void nolocerr(void) { err("Illegal Local Name"); }                                  /* blitz2.s:7416 */
NORETURN void fpconerr(void) { err("Fractions Not allowed in Constants"); }                  /* blitz2.s:7419 */
NORETURN void syntaxerr(void) { err("Syntax Error"); }                                       /* blitz2.s:7422 */
NORETURN void typeerr(void) { err("Duplicated Type"); }                                      /* blitz2.s:7425 */
NORETURN void laberr(void) { err("Illegal Label Name"); }                                    /* blitz2.s:7428 */
NORETURN void modeerr(void) { err("End NewType without NewType"); }                          /* blitz2.s:7431 */
NORETURN void notypeerr(void) { err("Type Not Found"); }                                     /* blitz2.s:7434 */
NORETURN void illtypeerr(void) { err("Illegal Type"); }                                      /* blitz2.s:7437 */
NORETURN void offerr(void) { err("Duplicate Offset"); }                                      /* blitz2.s:7440 */
NORETURN void overerr(void) { err("Numeric Over Flow"); }                                    /* blitz2.s:7443 */
NORETURN void toolongerr(void) { err("Type too Big"); }                                      /* blitz2.s:7446 */
NORETURN void rongtypeerr(void) { err("Mismatched Types"); }                                 /* blitz2.s:7449 */
NORETURN void noarrerr(void) { err("Array not yet Dim'd"); }                                 /* blitz2.s:7452 */
NORETURN void noofferr(void) { err("Offset not Found"); }                                    /* blitz2.s:7455 */
NORETURN void pointerr(void) { err("Element isn't a pointer"); }                             /* blitz2.s:7458 */
NORETURN void noleterr(void) { err("Can't Assign Expression"); }                             /* blitz2.s:7461 */
NORETURN void illoperr(void) { err("Illegal Operator for Type"); }                           /* blitz2.s:7464 */
NORETURN void mismatcherr(void) { err("Type Mismatch"); }                                    /* blitz2.s:7467 */
NORETURN void arraldimerr(void) { err("Array already Dim'd"); }                              /* blitz2.s:7470 */
NORETURN void nonewerr(void) { err("Can't Create Variable inside Dim"); }                    /* blitz2.s:7473 */
NORETURN void arrnotdimd(void) { err("Array not Dim'd"); }                                   /* blitz2.s:7476 */
NORETURN void noconsterr(void) { err("Constant not defined"); }                              /* blitz2.s:7479 */
NORETURN void alconsterr(void) { err("Constant already defined"); }                          /* blitz2.s:7482 */
NORETURN void ilconsterr(void) { err("Illegal Constant"); }                                  /* blitz2.s:7485 */
NORETURN void duplaberr(void) { err("Duplicate Label"); }                                    /* blitz2.s:7499 */
NORETURN void nolaberr(void) { err("Label not Found"); }                                     /* blitz2.s:7502 */
NORETURN void typeiferr(void) { err("Can't Compare Types"); }                                /* blitz2.s:7505 */
NORETURN void noiferr(void) { err("End Condition without Condition"); }                      /* blitz2.s:7508 */
NORETURN void bigiferr(void) { err("If Block too Large"); }                                  /* blitz2.s:7511 */
NORETURN void badenderr(void) { err("Illegaly nested Condition"); }                          /* blitz2.s:7514 */
NORETURN void dupmacerr(void) { err("Macro already Defined"); }                              /* blitz2.s:7517 */
NORETURN void macnesterr(void) { err("Can't create Macro inside Macro"); }                   /* blitz2.s:7520 */
NORETURN void macbigerr(void) { err("Macro too Big"); }                                      /* blitz2.s:7527 */
NORETURN void tmmacerr(void) { err("Macros Nested too Deep"); }                              /* blitz2.s:7530 */
NORETURN void nomacerr(void) { err("Macro not Found"); }                                     /* blitz2.s:7533 */
NORETURN void comerr(void) { err("Too many comma's in Let"); }                               /* blitz2.s:7536 */
NORETURN void nocomerr(void) { err("Can't use comma in Let"); }                              /* blitz2.s:7539 */
NORETURN void conintype(void) { err("Can't use a Constant in New Type"); }                   /* blitz2.s:7542 */
NORETURN void alincerr(void) { err("Already Included"); }                                    /* blitz2.s:7545 */
NORETURN void noincerr(void) { err("Can't open Include"); }                                  /* blitz2.s:7548 */
NORETURN void readerr(void) { err("Error Reading File"); }                                   /* blitz2.s:7551 */
NORETURN void toifmacerr(void) { err("If Macro's Nested too Deep"); }                        /* blitz2.s:7554 */
NORETURN void noenderr(void) { err("If Without End If"); }                                   /* blitz2.s:7557 */
NORETURN void dupprocerr(void) { err("Duplicate Procedure name"); }                          /* blitz2.s:7560 */
NORETURN void duplocerr(void) { err("Duplicate parameter variable"); }                       /* blitz2.s:7563 */
NORETURN void noprocerr(void) { err("Procedure not found"); }                                /* blitz2.s:7566 */
NORETURN void toovarerr(void) { err("Too many parameters"); }                                /* blitz2.s:7569 */
NORETURN void illvarerr(void) { err("Illegal Parameter Type"); }                             /* blitz2.s:7572 */
NORETURN void nopenderr(void) { err("Unterminated Procedure"); }                             /* blitz2.s:7575 */
NORETURN void illprocerr(void) { err("Illegal Procedure Call"); }                            /* blitz2.s:7578 */
NORETURN void functypeerr(void) { err("Illegal Function Type"); }                            /* blitz2.s:7581 */
NORETURN void illprocperr(void) { err("Illegal Parameter Type"); }                           /* blitz2.s:7584 */
NORETURN void extlaberr(void) { err("Can't Access Label"); }                                 /* blitz2.s:7587 */
NORETURN void sreterr(void) { err("Illegal Procedure return"); }                             /* blitz2.s:7590 */
NORETURN void sharederr(void) { err("Shared outside of Procedure"); }                        /* blitz2.s:7593 */
NORETURN void dupsherr(void) { err("Variable already Shared"); }                             /* blitz2.s:7596 */
NORETURN void badforerr(void) { err("Bad Type for For...Next"); }                            /* blitz2.s:7599 */
NORETURN void noforerr(void) { err("Next without For"); }                                    /* blitz2.s:7602 */
NORETURN void bigforerr(void) { err("For...Next Block to Long"); }                           /* blitz2.s:7605 */
NORETURN void nonexterr(void) { err("For Without Next"); }                                   /* blitz2.s:7608 */
NORETURN void zprocerr(void) { err("Can't Nest Procedures"); }                               /* blitz2.s:7611 */
NORETURN void nodimerr(void) { err("Can't Dim Globals in Procedures"); }                     /* blitz2.s:7614 */
NORETURN void noconerr(void) { err("Can't Convert Types"); }                                 /* blitz2.s:7617 */
NORETURN void baddaterr(void) { err("Bad Data"); }                                           /* blitz2.s:7620 */
NORETURN void nociferr(void) { err("CEND without CNIF/CSIF..."); }                           /* blitz2.s:7623 */
NORETURN void cnferr(void) { err("Constant Not Found"); }                                    /* blitz2.s:7626 */
NORETURN void badconerr(void) { err("Illegal Constant Expression"); }                        /* blitz2.s:7629 */
NORETURN void badpenderr(void) { err("Illegal End Procedure"); }                             /* blitz2.s:7632 */

/* =================================================================
 * char classes (TRUE = asm Z=1)
 * ================================================================= */

/* blitz2.s:7681 tstalpha - tst if d0 is alphabetic */
BOOL tstalpha(UWORD c)
{
    if (c > 'z' || c < 'A')
        return FALSE;
    if (c >= 'a')
        return TRUE;
    return c <= 'Z';
}

/* blitz2.s:7694 tstnum - tst if d0 is numeric, eq=1 if yes */
BOOL tstnum(UWORD c)
{
    return tstnum2(c);
}

/* blitz2.s:7697 tstnum2 */
BOOL tstnum2(UWORD c)
{
    return c >= '0' && c <= '9';
}

/* blitz2.s:7704 tstlab - tst if d0 is a lable character */
BOOL tstlab(UWORD c)
{
    return tstalpha(c) || tstnum2(c) || c == '_' || c == loclabch;
}

/* =================================================================
 * names
 * ================================================================= */

/* blitz2.s:7715 makename2 */
UWORD makename2(UWORD *c)
{
    bakup();
    return makename(c);                     /* falls through into makename */
}

/* blitz2.s:7716 makename - pick up label name from a5... put into name buff,
   return len in d2 (eq if zero) */
UWORD makename(UWORD *c)
{
    return makename3((char *)namebuff, c);
}

/* blitz2.s:7722 makename3 */
UWORD makename3(char *buf, UWORD *c)
{
    return makename4(buf, 0, c);
}

/* blitz2.s:7723 makename4 */
UWORD makename4(char *at, UWORD len, UWORD *c)
{
    UWORD ch = getchar3();
    if (ch == '_' || tstalpha(ch)) {
        do {
            *at++ = (char)ch;               /* BUG(orig) (#29): no length check */
            len++;
            ch = getchar_();
        } while (tstlab(ch));
        if (ch == 32)
            ch = getchar3();
    }
    *at = 0;
    if (c)
        *c = ch;
    return len;
}

/* =================================================================
 * name lists
 * ================================================================= */

/* blitz2.s:7746 findlast - look for last entry in list from a2.
   review: the asm loops back to `beq` with the flags of the first
   move.l (movea sets none), so it hangs on a non-empty list; never called
   in blitz2.s.  C walks the list (DIFF, dead code). */
struct node *findlast(struct node *link)
{
    while (link->next)
        link = link->next;
    return link;                            /* asm: bra reget (no side effects) */
}

/* blitz2.s:7754 addhere2 - insert at beginning (after link) */
struct node *addhere2(struct node *link, UWORD nameoff, UWORD len)
{
    struct node *old = link->next;
    struct node *n = addhere(link, nameoff, len);
    n->next = old;
    return n;
}

/* blitz2.s:7761 addhere - add namebuff after a2,d7; d2=len of name */
struct node *addhere(struct node *link, UWORD nameoff, UWORD len)
{
    UWORD size;
    struct node *n;
    UBYTE *p;
    const UBYTE *src;

    if (dirmode)
        illdirerr();
    size = nameoff + len + 1;
    n = doallocmem(size, 1);
    link->next = n;
    n->next = NULL;                         /* clear next */
    p = (UBYTE *)n + nameoff - 1;
    *p++ = (UBYTE)size;                     /* put in struct length */
    src = namebuff;
    while ((*p++ = *src++))
        ;
    return n;                               /* asm: bra reget */
}

/* blitz2.s:7822 findlab - see if namebuff is in list at a2, char offset in d7.
   z=1 if yes, a2=item loc; if no, a2=last for link */
struct node *findlab(struct node *link, UWORD nameoff, UWORD len,
                     struct node **last)
{
    struct node *n = link;

    for (;;) {
        UWORD nlen, cnt;
        const UBYTE *name, *want;
        if (!n->next) {
            if (last)
                *last = n;
            return NULL;                    /* asm: reget, d1=-1, Z=0 */
        }
        n = n->next;
        nlen = *((UBYTE *)n + nameoff - 1) - nameoff - 1;
        if (nlen != len)
            continue;
        name = (UBYTE *)n + nameoff;
        want = namebuff;
        cnt = nlen - 1;                     /* dbf: BUG(orig) (#30) len==0 runs on */
        for (;;) {
            if (*name++ != *want++)
                goto next;
            if (cnt-- == 0)
                break;
        }
        return n;                           /* asm: reget, Z=1 */
    next:;
    }
}

/* blitz2.s:7789-7820 find wrappers */
struct include *findinc(UWORD len, struct node **last)
{
    return (struct include *)findlab((struct node *)&firstinc, NAMEOFF_INC, len, last);
}

struct xinclude *findxinc(UWORD len, struct node **last)
{
    return (struct xinclude *)findlab((struct node *)&firstxinc, NAMEOFF_XINC, len, last);
}

struct constant *findconst(UWORD len, struct node **last)
{
    return (struct constant *)findlab((struct node *)&firstconst, NAMEOFF_CONST, len, last);
}

struct macro *findmac(UWORD len, struct node **last)
{
    return (struct macro *)findlab((struct node *)&firstmacro, NAMEOFF_MACRO, len, last);
}

struct label *findadd(UWORD len, struct node **last)
{
    return (struct label *)findlab((struct node *)&firstlabel, NAMEOFF_LABEL, len, last);
}

struct type *findtype(struct node *link, UWORD len, struct node **last)
{
    return (struct type *)findlab(link, NAMEOFF_TYPE, len, last);
}

struct proc *findproc(UWORD len, struct node **last)
{
    return (struct proc *)findlab((struct node *)&firstproc, NAMEOFF_PROC, len, last);
}

struct fornode *findfor(UWORD len, struct node **last)
{
    return (struct fornode *)findlab((struct node *)&firstfor, NAMEOFF_FOR, len, last);
}

struct var *findvar(struct node *link, UWORD len, struct node **last)
{
    return (struct var *)findlab(link, NAMEOFF_VAR, len, last);
}

/* =================================================================
 * freeing
 * ================================================================= */

/* blitz2.s:7850 freeprocvs */
void freeprocvs(void)
{
    struct alllocal *al = alllocals, *dead;
    alllocals = NULL;
    while (al) {
        freevars(al->locals);
        freevars(al->globs);
        dead = al;
        al = al->next;
        FreeMem(dead, 12);
    }
}

/* blitz2.s:7866 freevars / 7867 freevars2 - free up var structs from a3 on */
void freevars(struct var *v)
{
    struct var *dead;
    while (v) {
        dead = v;
        v = v->next;
        FreeMem(dead, dead->len);
    }
}

/* blitz2.s:7878 freelabels - free up label structs from firstlabel on... */
void freelabels(void)
{
    struct label *last = lastrescon;
    struct label *l = last->next, *dead;
    last->next = NULL;
    while (l) {
        ULONG refs = (ULONG)l->refs;
        if (!(refs & 1)) {
            struct labref *ref = (struct labref *)refs, *deadref;
            while (ref) {
                deadref = ref;
                ref = ref->next;
                FreeMem(deadref, 12);
            }
        }
        dead = l;
        l = l->next;
        FreeMem(dead, dead->len);
    }
}

/* blitz2.s:7906 freepends / 7909 fpnd2 */
void freepends(void)
{
    struct addrnode *p = firstpend, *dead;
    firstpend = NULL;
    while (p) {
        dead = p;
        p = p->next;
        FreeMem(dead, 8);
    }
}

/* blitz2.s:7918 freeprocs - free up all proc structs */
void freeprocs(void)
{
    struct proc *p = firstproc, *dead;
    firstproc = NULL;
    while (p) {
        dead = p;
        p = p->next;
        FreeMem(dead, dead->len);
    }
}

/* blitz2.s:7933 freetypes - free up type structs from a2 on */
void freetypes(struct type *t)
{
    struct type *dead;
    while (t) {
        if (t->u.code != TYPE_FORWARD)      /* cmp #$ff,a3 */
            freevars(t->u.fields);          /* free offsets too */
        dead = t;
        t = t->next;
        FreeMem(dead, dead->len);
    }
}

/* =================================================================
 * variables
 * ================================================================= */

static struct var *fvarback(struct node *head, UWORD len, struct type **type,
                            UWORD *c);

/* blitz2.s:8034 fvarfound - var found, establish if type is correct! */
static struct var *fvarfound(struct var *v, struct type **type, UWORD *c)
{
    UWORD ch = reget();                     /* d0 left by findlab */

    if (ch == '.') {
        struct type *t;
        UWORD len = makename(&ch);
        t = findtype((struct node *)&alltypes, len, NULL);
        if (!t)
            notypeerr();
        ch = reget();
        if (v->type != t)
            rongtypeerr();
    } else if (ch == '$') {
        if (v->type != &stringtype)
            rongtypeerr();
        ch = getchar3();
    }
    if (flagmask & 1) {
        if (!(v->flags & 1))
            pointerr();
    }
    *type = v->type;
    if (c)
        *c = ch;
    return v;
}

/* blitz2.s:7973 fvaradd - create var after `last` */
static struct var *fvaradd(struct node *last, UWORD len, struct type **type,
                           UWORD *c)
{
    struct var *v;
    struct type *t;
    UWORD ch, size;

    if (nonew)
        nonewerr();
    v = (struct var *)addhere(last, NAMEOFF_VAR, len);   /* create var */
    added = ~added;                         /* set added flag */
    ch = reget();
    t = defaulttype;
    if (ch == '.') {                        /* .clapton */
        UWORD l2 = makename(&ch);
        t = findtype((struct node *)&alltypes, l2, NULL);
        if (!t)
            notypeerr();
        ch = reget();
    } else if (ch == '$') {
        t = &stringtype;
        ch = getchar3();
    }
    /* .usedef */
    if (t != &bytetype) {
        if (!varmode)
            varoff++;
        varoff &= ~1;                       /* .pit: bclr #0,varoff+1 */
    }
    v->type = t;
    v->flags = flagmask;
    size = 4;
    if (!(flagmask & 1) && !(flagmask & 2) && t->u.code < 256)
        size = t->size;
    if (varmode) {
        varoff -= size;
        v->offset = varoff;                 /* loc offset */
    } else {
        v->offset = varoff;                 /* global */
        varoff += size;
    }
    *type = t;
    if (c)
        *c = ch;
    return v;
}

/* blitz2.s:7965 fvarback - search list head, flags must match */
static struct var *fvarback(struct node *head, UWORD len, struct type **type,
                            UWORD *c)
{
    struct node *last;
    struct var *v = findvar(head, len, &last);
    while (v) {                             /* .more */
        if ((UBYTE)flagmask == (UBYTE)v->flags)
            return fvarfound(v, type, c);
        v = (struct var *)findlab((struct node *)v, NAMEOFF_VAR, len, &last);
    }
    return fvaradd(last, len, type, c);
}

/* blitz2.s:8016 fvarlocal */
static struct var *fvarlocal(UWORD len, struct type **type, UWORD *c)
{
    struct var *v = findvar((struct node *)&firstglob, len, NULL);
    struct var *r;
    WORD save;

    while (v) {
        if ((UBYTE)flagmask == (UBYTE)v->flags)
            return fvarfound(v, type, c);
        v = (struct var *)findlab((struct node *)v, NAMEOFF_VAR, len, NULL);
    }
    /* .hendrix */
    varmode = -1;
    save = varoff;
    varoff = locvaroff;
    r = fvarback((struct node *)&firstlocal, len, type, c);
    locvaroff = varoff;
    varoff = save;
    return r;
}

/* blitz2.s:7953 fetchvar - find/create variable/struct from namebuff
   flagmask.0=1 if pointer; get pointer to variable in a3, pointer to type
   in a2 */
struct var *fetchvar(UWORD len, struct type **type, UWORD *c)
{
    struct type *dummy;
    if (!type)
        type = &dummy;
    usevars();
    added = 0;
    varmode = 0;                            /* local mode */
    if (procmode)
        return fvarlocal(len, type, c);
    return fvarback((struct node *)&firstvar, len, type, c);
}

/* blitz2.s:8061 addd0a0 - emit code adding d3 to a2; returns the (possibly
   clobbered) d3 like the asm */
static WORD addd0a0(WORD off)
{
    if (off == 0)
        return off;
    if ((UWORD)off < 9) {
        WORD quick = (off & 7) << 9;        /* addq data field (8 -> 0) */
        *addqa0 = (*addqa0 & 0xf1ff) | quick;
        pokewd(*addqa0);
        return quick;                       /* asm: d3 clobbered */
    }
    leaa0a0[1] = off;
    pokel(TL(leaa0a0));
    return off;
}

/* blitz2.s:8082 makeamul / 8084 makelmul - make long multiply for array
   calc.  DIFF: the asm read the dimension offset from its caller's stack
   frame (4(a7)); here it is the parameter `off`.
   code: do a x(a5).l * d1 + d0.l  with d1 = D(regat), d0 = D(regat-1) */
static void makelmul(WORD off)
{
    UWORD r = regat;

    pokewd((arrmul1[0] | r) - 1);                           /* move.l d(r-1),-(a7) */
    pokewd((*arrmul2 | r) | ((r - 1) << 9));                /* move dr,d(r-1) */
    pokewda5s(*arrmul3 | (r << 9));                         /* mulu x+2(a5),dr */
    pokewd(off + 2);
    pokewda5s(*arrmul4 | ((r - 1) << 9));                   /* mulu x(a5),d(r-1) */
    pokewd(off);
    pokewd((*arrmul5 | r) - 1);                             /* swap d(r-1) */
    pokewd((*arrmul6 | r) - 1);                             /* clr d(r-1) */
    pokewd(((*arrmul7 | r) - 1) | (r << 9));                /* add.l d(r-1),dr */
    pokewd(*arrmul8 | ((r - 1) << 9));                      /* move.l (a7)+,d(r-1) */
    pokewd(((regat - 1) << 9) | 0xd080 | regat);            /* add.l dr,d(r-1) */
}
#define makeamul makelmul

/* blitz2.s:8170 calcvar / 8172 calcvar2 - use a3 = var, a2 = type,
   flagmask = flags.
   if bit 15 of d2 then code has been generated for address of thing in a2.
   else d3=offset from a5 for simple variable.
   d2 & ff=type. 0=struct address; bit 14 of d2=1 if result is a pointer */
UWORD calcvar(struct var **pv, struct type **pt, UWORD *pc0, WORD *offset)
{
    /* .listoff dc 0 (data in code).  Kept static, not a local: it is set
       before arreval, which recurses into calcvar for an array element in
       the index expression and resets it there (as in the asm) - a local
       would change the element size of a List array indexed that way. */
    static UWORD listoff;
    struct var *v = *pv;
    struct type *t = *pt;
    UWORD ch = *pc0;
    UWORD rtype;
    WORD off;

    lastoffset = 0;
    if (v->flags & 2) {                     /* btst #1,7(a3): array */
        if (added)
            noarrerr();
        ch = getchar3();
        if (ch == ')') {
            /* list array: current item */
            if (!(v->flags & 0x100))        /* btst #0,6(a3) */
                nolisterr();
            listcode1[1] = v->offset;
            pokela5s(TL(listcode1));
            pokel(TL(listcode2));
            if (debugga)
                tokejsr(currentchk);        /* a2=0 error check */
            pokewd(*listarfix);             /* addq #8,a2: skip node. */
        } else {
            /* .notalist */
            struct type *ftype;
            WORD fvarmode, fndims, foff;
            UWORD fmask, elsize;

            listoff = 0;
            if (v->flags & 0x100)
                listoff = 8;                /* adjust size of one element if list. */
            /* .knob: asm stack frame -> locals */
            fvarmode = varmode;
            fndims = v->ndims;              /* number of dims */
            foff = v->offset;               /* varoff */
            ftype = t;
            fmask = flagmask;
            bakup();
            ch = arreval();
            flagmask = fmask;
            varmode = fvarmode;
            if (debugga)
                arrszchk();
            t = ftype;
            elsize = 4;
            if (!(flagmask & 1))            /* pointer? then use 4 byte mul... */
                elsize = t->size;
            muld1(elsize + listoff);
            regat++;
            for (;;) {                      /* .arloop */
                foff += 4;
                if (fvarmode)
                    foff -= 8;
                if (debugga) {
                    pokewda5s(*zarrchk | ((regat - 1) << 9));   /* cmp.l x(a5),D regat */
                    pokewd(foff);
                    pokel(TL(arrchk2));
                    tokejsr(arrerr);
                }
                if (--fndims == 0)
                    break;
                if (ch != ',')
                    syntaxerr();
                ch = arrevalchk();
                varmode = fvarmode;
                makeamul(foff);
            }
            /* .ardone */
            if (ch != ')')
                syntaxerr();
            regat--;
            movea5a0[1] = v->offset;
            pokela5s(TL(movea5a0));
            pokewd(regat | 0xd5c0);         /* add.l Dr,a2 */
            if (v->flags & 0x100)
                pokewd(*listarfix);         /* addq #8,a2 to skip node. */
        }
        /* .normalar */
        ch = getchar3();
        /* this shit added lately - after the major fucking crash! */
        if ((v->flags & 1) && ch != '\\')
            rtype = 0xc000;                 /* .pmskip */
        else {
            if (v->flags & 1)
                pokewd(calcvar_pfix);       /* move.l (a2),a2 */
            rtype = 0x8000;                 /* .noppnt */
        }
    } else {
        /* .notarr */
        if (ch == '(')
            noarrerr();
        if (t->u.code < 256) {
            /* .simpvar */
            rtype = TYPE_CODE(t);
            goto simpvar2;
        }
        if ((v->flags & 1) && ch != '\\') {
            /* it's a pointer - is it a simple one? */
            rtype = 0;
            goto simpvar2;
        }
        /* .notap */
        rtype = 0x8000;
        movea5a0[1] = v->offset;
        pokela5s(TL(movea5a0));             /* move.l x(a5),a2 */
    }

    /* .loopm1 */
    off = 0;
    for (;;) {
        /* .loop0 */
        if (ch != '\\')
            break;
        if (t->u.code < 256)
            rongtypeerr();
        {
            UWORD len = makename(&ch);
            struct var *f;
            if (!len)
                syntaxerr();
            f = findvar((struct node *)&t->u.fields, len, NULL);   /* addq #4,a2 */
            if (!f)
                noofferr();
            ch = reget();
            v = f;                          /* pointer to offset */
            t = v->type;                    /* pointer to type */
            off += v->offset;
            lastoffset = (ULONG)v;
        }
        if (ch == '[') {
            struct type *st;
            struct var *sv;
            WORD svm;
            ULONG slo;
            UWORD ty, elsize;

            if (!(v->flags & 2))
                rongtypeerr();
            addd0a0(off);
            st = t; sv = v; svm = varmode; slo = lastoffset;
            pokewd(*pusha2);
            ty = 2;
            ch = eval(&ty);
            if (ch != ']')
                syntaxerr();
            pokewd(*pulla2);
            lastoffset = slo; varmode = svm; t = st; v = sv;
            /* [] error checking: 8(a3) = how many we can handle.
               special case for 0! */
            if (debugga && v->ndims) {
                brchkcode[1] = v->ndims;
                brchkcode[0] = brchkcode0[0] | (regat << 9);
                pokel(TL(brchkcode));
                pokel(TL(brchkcode + 2));
                pokewd(brchkcode[4]);
            }
            elsize = 4;
            if (!(v->flags & 1))
                elsize = t->size;
            muld1(elsize);
            pokewd(*addrega2 | regat);
            ch = getchar3();
            off = 0;
            rtype = 0x8000;
        }
        /* .realdone */
        if (!(v->flags & 1))
            continue;                       /* bra .loop0 */
        /* .pointer */
        if (ch != '\\') {
            /* .done3 */
            rtype |= 0x4000;
            if (!(v->flags & 4))
                rtype |= TF_PTRNOAT;
            break;
        }
        if (off == 0)
            pokewd(*movea0);
        else {
            movea0a0[1] = off;
            pokel(TL(movea0a0));
        }
        if (v->flags & 4)
            pokel(TL(adda0a0));
        off = 0;                            /* .loopm1 */
    }
    /* .done */
    if (t->u.code < 256)
        rtype = (rtype & 0xff00) | (UBYTE)TYPE_CODE(t); /* move.b 7(a2),d2 */
    off = addd0a0(off);                     /* .done2: bra addd0a0 */
    goto out;

simpvar2:
    off = v->offset;
    if (v->flags & 1)
        rtype |= 0x4000;
out:
    *pv = v;
    *pt = t;
    *pc0 = ch;
    if (offset)
        *offset = off;
    return rtype;
}
