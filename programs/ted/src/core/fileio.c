/*
 * fileio.c - chunk 04 core (ted.s 3063-4197): see fileio.h.
 *
 * Option file, number output, file reading into the line list, token
 * lookup and detokeniser, search, line list helpers, block cursor and
 * block state, charstobuff.  The UI half of chunk 04 is src/reqs.c.
 */
#include "tedcore.h"
#include "hooks.h"
#include "state.h"
#include "fileio.h"
#include "lines.h"
#include "loadsave.h"
#include "services.h"
#include "tokit.h"
#include "lineedit.h"
#include "keymap.h"

#ifdef TED_HOST
#include <stdio.h>
#include <stdlib.h>
#endif

const UWORD powersof10[5] = { 10000, 1000, 100, 10, 1 };   /* ted.s:3165 */
struct rdstate rd;              /* asm d7/a5/d6 of a load */
struct bcur bc;                 /* asm a5/d6/d5 of a block walk */

#ifdef TED_HOST
/* see fileio.h.  Host only: the Amiga build uses the real DIVU.W from
   src/glue.c (divu_w there), which also traps on a zero divisor like the
   original.  Pure C (shift and subtract). */
ULONG divu_w(ULONG d0, UWORD src)
{
    ULONG r, q = 0;
    int i;

    if (src == 0) {
        /* 68000: zero divide trap (guru 5).  BUG(orig) (#385) (tabstop = 0,
           decision O3); host-test-only guard: */
        ORIG_HAZARD("divu by 0 (68000 zero divide trap)");
        fprintf(stderr, "ted core: divu by zero (e.g. tabstop = 0) - the "
                "68000 traps here (zero divide guru); host build aborts\n");
        abort();
    }
    if ((d0 >> 16) >= src)
        return d0;                              /* quotient > $FFFF: V=1, unchanged */
    r = d0 >> 16;
    for (i = 15; i >= 0; i--) {
        r = (r << 1) | ((d0 >> i) & 1);
        q <<= 1;
        if (r >= src) {
            r -= src;
            q |= 1;
        }
    }
    return (r << 16) | q;
}
#endif

/* first non-space character of a 0-terminated line == `label`
   (the label test inlined in allocline/freeupa5/makeline/freethis:
   `cmp.b #32,(a1)+ / beq .loop / cmp.b -(a1),d0`) */
static BOOL islabelline(const UBYTE *p)
{
    while (*p == ' ')
        p++;
    return *p == label;
}

/* ------------------------------------------------------- option file */

/* ted.s:3063 loaddefs */
void loaddefs(void)
{
    UBYTE img[sizeof(struct defblock)];
    tedfh d7;

    d7 = ted_open(defname, TED_MODE_OLDFILE);
    if (d7) {
        /* the asm reads straight into defstart; a short read keeps the
           rest - the image starts as a copy of defstart for that */
        defs_tobytes(img, &defstart);
        ted_read(d7, img, (LONG)sizeof(img));   /* result unchecked */
        defs_frombytes(&defstart, img);
        ted_close(d7);
    }
    /* .skip */
    setdefs(&defstart);                         /* bra setdefs */
}

/* ted.s:3078 savedefs */
void savedefs(void)
{
    UBYTE img[sizeof(struct defblock)];
    tedfh d7;

    d7 = ted_open(defname, TED_MODE_NEWFILE);
    if (!d7)
        return;                                 /* .skip: errors ignored */
    defs_tobytes(img, &defstart);
    ted_write(d7, img, (LONG)sizeof(img));      /* result unchecked */
    ted_close(d7);                              /* jmp close(a6) */
}

/* NEW: defstart <-> 50 byte big-endian file image (identity on the Amiga,
   byte swap on a little-endian host) */
void defs_frombytes(struct defblock *d, const UBYTE *b)
{
    UWORD *w = (UWORD *)d;
    unsigned i;

    for (i = 0; i < sizeof(*d) / 2; i++)
        w[i] = (UWORD)((b[2 * i] << 8) | b[2 * i + 1]);
}

/* see fileio.h */
void defs_tobytes(UBYTE *b, const struct defblock *d)
{
    const UWORD *w = (const UWORD *)d;
    unsigned i;

    for (i = 0; i < sizeof(*d) / 2; i++) {
        b[2 * i] = (UBYTE)(w[i] >> 8);
        b[2 * i + 1] = (UBYTE)w[i];
    }
}

/* ------------------------------------------------------------ numbers */

/* ted.s:3140 numtoascii2 */
UBYTE *numtoascii2(UWORD d0, UBYTE *a0)
{
    a0 = numtoascii(d0, a0);
    *a0 = 0;                                    /* clr.b (a0) - a0 stays */
    return a0;
}

/* ted.s:3144 numtoascii - convert number to ascii (number in d0, buffer
   in a0); no leading zeros, no 0 byte */
UBYTE *numtoascii(UWORD d0, UBYTE *a0)
{
    ULONG v = d0;                               /* and.l #$ffff,d0 */
    BOOL d1 = FALSE;                            /* printed anything flag */
    int i;

    for (i = 0; i < 5; i++) {                   /* divu (a1)+,d0: no overflow */
        ULONG r = divu_w(v, powersof10[i]);
        UWORD q = (UWORD)r;

        if (q || d1) {
            d1 = TRUE;
            *a0++ = (UBYTE)(q + 48);
        }
        v = r >> 16;                            /* clr d0 / swap d0 */
    }
    if (!d1)
        *a0++ = '0';
    return a0;
}

/* ------------------------------------------------------ reading files */

/* ted.s:3307 doopen - open the file (name in d7, mode in d2).  The
   comment "and fill buffer" is wrong: the caller calls fillbuff. */
BOOL doopen(const UBYTE *d7, LONG d2)
{
    rd.fh = ted_open((const char *)d7, d2);
    if (!rd.fh) {
        /* addq #4,a7 / bra error: error1, then back to the CALLER's
           caller - here: FALSE, the caller returns at once */
        error(error1);
        return FALSE;
    }
    filehand = rd.fh;                           /* .skip */
    return TRUE;
}

/* ted.s:3302 doclose */
void doclose(void)
{
    filehand = 0;
    ted_close(rd.fh);
}

/* ted.s:3337 fillbuff */
LONG fillbuff(void)
{
    rd.a5 = dosbuff;
    rd.d6 = ted_read(rd.fh, dosbuff, doslen);
    if (rd.d6 < 0) {                            /* bpl .skip */
        error(error2);
        rd.d6 = 0;
    }
    return rd.d6;                               /* asm: Z=1 if 0 */
}

/* ted.s:3325 doread - next character from the file (or <0 if eof!).
   asm: N=0 with the byte in d0.b, d0 = -1 / N=1 at EOF.  d6 counts with
   subq.w; it never exceeds doslen, so a LONG gives the same result. */
WORD doread(void)
{
    for (;;) {
        /* .skip2 */
        if (--rd.d6 >= 0)
            return *rd.a5++;
        /* .skip */
        if (!fillbuff())
            return -1;
    }
}

/* ted.s:3375 readline - read in a line to charbuff2.
   asm: d0 = -1/N=1 no line to read, d0 = 0/Z=1 no eol, d0 = 1 normal;
   length (incl. the 0 byte) in d5.
   BUG(orig) (survey-04 I.1-7, kept): the last line without LF and lines
   cut at the buffer end are never tokenised (d4 is only set for LF; #376, #375);
   bytes >= $80 are token pairs even in text files (the next byte is taken
   unchecked, also LF/0/TAB; #374); a token byte right before EOF leaves a
   half token in front of the 0 (#379); TAB is expanded before the overflow test
   (may write up to tabstop-1 bytes beyond charbuff3; #378); tabstop = 0 is a
   zero divide (#385); TAB counts token pairs as 2 columns (#377); CR stays. */
int readline(UWORD *d5)
{
    UBYTE *a0;
    WORD d0;
    BOOL d4;                                    /* flag for no toke on load! */
    ULONG len;
    int ret;

    d0 = doread();
    if (d0 < 0)
        return RL_EOF;                          /* .skip */
    a0 = charbuff2;
    d4 = FALSE;
    for (;;) {
        /* .loop */
        if (d0 & 0x80) {                        /* token pair, copied unchecked */
            *a0++ = (UBYTE)d0;
            d0 = doread();
            if (d0 < 0)
                goto oops;
            *a0++ = (UBYTE)d0;
            goto nottab;
        }
        /* .tryout */
        if (d0 == 10) {
            d0 = 0;                             /* file is in ascii format */
            d4 = TRUE;                          /* flag it for a toke-up */
        }
        /* .skip2 */
        *a0++ = (UBYTE)d0;
        if (d0 == 0)
            goto skip3;
        if (d0 == 9) {
            ULONG col;

            a0--;
            col = (ULONG)(a0 - charbuff2);
            do {                                /* .tabloop */
                if (a0 >= charbuff4)
                    ORIG_HAZARD("readline: TAB expansion beyond charbuff4");
                *a0++ = ' ';
                col = (col & 0xFFFF0000) | (UWORD)(col + 1);    /* addq.w */
            } while ((UWORD)(divu_w(col, tabstop) >> 16) != 0);   /* swap / tst */
        }
nottab:
        /* .nottab */
        if (a0 >= charbuff3) {                  /* bcs .ok */
            a0 = charbuff3 - 3;
            goto oops;
        }
        /* .ok */
        d0 = doread();
        if (d0 < 0)
            goto oops;
    }
oops:
    /* .oops: EOF in the line or buffer full - "no eol" */
    *a0++ = 0;
    len = (ULONG)(a0 - charbuff2);
    ret = RL_NOEOL;
    goto sit;
skip3:
    /* .skip3 */
    len = (ULONG)(a0 - charbuff2);
    ret = RL_OK;
sit:
    /* .sit: should i toke it? */
    if (d4 && tokenise) {
        /* Yup! */
        len = (len & 0xFFFF0000) | (UWORD)(len - 1);    /* subq.w #1,d5 */
        if ((WORD)len < 0)
            len = 1;                            /* (never: d5 >= 1) */
        else
            len = mytoke((UWORD)len);           /* .oktt */
    }
    /* .no */
    *d5 = (UWORD)len;
    return ret;
}

/* ted.s:3447 mytoke */
UWORD mytoke(UWORD d5)
{
    /* d1 = d5 (length), a1 = a2 = charbuff2: in place */
    return tokit2(charbuff2, charbuff2, (WORD)d5, NULL);
}

/* ted.s:3349 allocline */
struct item *allocline(UWORD d5)
{
    struct item *a0;
    const UBYTE *a1;
    UBYTE *p;

    /* move.l d5,d0 / add.w #lenitem,d0 */
    a0 = alloc2((UWORD)(d5 + lenitem), TED_MEMF_PUBLIC);
    a0->nextitem = NULL;
    /* lastitem stays uninitialised (survey-04 I.13; no defect): the
       callers set it (fileopen explicitly, linka0) */
    a0->numchars = (UBYTE)d5;
    a1 = charbuff2;
    p = a0->chars;
    while ((*p++ = *a1++) != 0)
        ;
    if (islabelline(a0->chars))
        remember(a0);                           /* beq remember */
    return a0;
}

/* ted.s:3277 linka0 - add a0 after a4 */
struct item *linka0(struct item *a0, struct item *a4)
{
    struct item *a1;

    numlines++;
    a1 = a4->nextitem;
    if (a1) {
        a0->nextitem = a1;
        a1->lastitem = a0;
    } else {
        /* .end: a0->nextitem is NOT cleared (allocline/allocnull did) */
        enditem = a0;
    }
    /* .end2 */
    a0->lastitem = a4;
    a4->nextitem = a0;
    a4 = a0;
    thisitem = a4;
    return a4;
}

/* ted.s:3293 bloadadd */
UBYTE *bloadadd(UBYTE *a3)
{
    const UBYTE *a0 = charbuff2;

    for (;;) {
        UBYTE c = *a0++;

        *a3++ = c;
        if (c == 0)
            break;                              /* .done */
        if (c & 0x80)
            *a3++ = *a0++;                      /* second byte of a token */
    }
    return a3 - 1;                              /* subq #1,a3: ON the 0 */
}

/* ---------------------------------------------- tokens / detokeniser */

/* ted.s:3455 detok - detokenise a0 to a1.  No length limit (survey-04
   I.8).  A token byte in front of the 0 (readline I.3) takes the 0 as its
   low byte and runs on behind the line, as in the asm. */
UBYTE *detok(const UBYTE *a0, UBYTE *a1)
{
    for (;;) {
        UWORD d0 = *a0++;
        const char *a3;

        if (d0 == 0)
            break;                              /* .done */
        if (!(d0 & 0x80)) {
            *a1++ = (UBYTE)d0;
            continue;
        }
        /* .skip */
        d0 = (UWORD)((d0 << 8) | *a0++);
        a3 = findtoke(d0);
        while ((*a1++ = (UBYTE)*a3++) != 0)     /* .skip3 */
            ;
        a1--;
    }
    *a1 = 0;
    return a1;
}

/* ted.s:3472 findtoke */
const char *findtoke(UWORD d0)
{
    const struct tokennode *a3 = tokens;

    d0 &= 0x7FFF;                               /* bclr #15,d0 */
#ifdef TED_HOST
    if (!a3) {
        /* host-test-only guard.  BUG(orig) (#359): with tokens == 0 (standalone
           TED loading a tokenised file) the asm walks a "node" at address
           0: compares the word at 4, follows the long at 0 and may return
           the "name" at address 6.  The Amiga build does exactly that (the
           loop below reads through a3 = 0); the host cannot read address
           0 and treats it as an unknown token. */
        ORIG_HAZARD("findtoke: tokens == 0 (asm reads a node at address 0)");
        return tokeerr;
    }
#endif
    for (;;) {
        /* .loop */
        if (a3->num == d0)
            return a3->name;                    /* .got: addq #6,a3 */
        if (!a3->next)
            return tokeerr;                     /* .notoke */
        a3 = a3->next;
    }
}

/* ted.s:3759 detoka5 */
void detoka5(struct item *a5)
{
    UBYTE *a0 = charbuff3 + 1;
    const UBYTE *a1 = a5->chars;

    /* bsr .loop (subroutine in the subroutine) */
    for (;;) {
        UWORD d0 = *a1++;
        const char *a3;

        if (d0 == 0)
            break;                              /* .done */
        if (!(d0 & 0x80)) {
            if (a0 >= charbuff4)
                ORIG_HAZARD("detoka5 writes beyond charbuff3 (allocation end)");
            *a0++ = (UBYTE)d0;
            continue;
        }
        /* .skip */
        d0 = (UWORD)((d0 << 8) | *a1++);
        a3 = findtoke(d0);
        for (;;) {                              /* .found */
            if (a0 >= charbuff4)
                ORIG_HAZARD("detoka5 writes beyond charbuff3 (allocation end)");
            if ((*a0++ = (UBYTE)*a3++) == 0)
                break;
        }
        a0--;
    }
    /* length byte = n + 1, no 0 terminator; overflows beyond 255 */
    charbuff3[0] = (UBYTE)(a0 - charbuff3);
}

/* ------------------------------------------------------------- search */

/* ted.s:3523 isithere */
WORD isithere(struct item *a4, UBYTE **a1)
{
    return isithere2(a4, 0, a1);                /* moveq #0,d5, falls through */
}

/* ted.s:3530 isithere2 - test if find buffer is in line at a4; return
   d0 < 0 if not, else the start position; d5 = start position */
WORD isithere2(struct item *a4, UWORD d5, UBYTE **a1)
{
    UBYTE *a5 = a4->chars;
    UBYTE *base, *a3;
    const UBYTE *a2;

    if (tokenise) {
        a5 = charbuff;
        detok(a4->chars, charbuff);
    }
    /* .skip */
    base = a5;                                  /* asm a1 */
    if (a1)
        *a1 = base;
    {
        const UBYTE *e = base;

        while (*e)
            e++;
        if ((LONG)(WORD)d5 > (LONG)(e - base))
            ORIG_HAZARD("isithere2: start column behind the line end");
    }
    a5 += (WORD)d5;                             /* add.w d5,a5 */
    for (;;) {
        /* .loop0 */
        a3 = a5;
        a2 = findreqla16;
        for (;;) {
            /* .loop */
            UBYTE d0 = *a2++, d2;

            if (d0 == 0)
                return (WORD)(a5 - base);       /* .found: move a5,d0 */
            if (!findcase() && alphad0(d0))
                d0 &= 255 - 32;
            /* .skip2 */
            d2 = *a3++;
            if (d2 == 0)
                return -1;                      /* .notfound */
            if (!findcase() && alpha(d2))
                d2 &= 255 - 32;
            /* .skip3 */
            if (d0 != d2)
                break;
        }
        a5++;
    }
}

/* --------------------------------------------------------- line list */

/* ted.s:3684 freeupa5 - unlink a5 from list */
void freeupa5(struct item *a5)
{
    UWORD d0;

    if (numlines == 1) {
        /* .freeonly: BUG(orig) (#361) (survey-04 I.11): if allocnull fails,
           first/end/thisitem keep pointing at the freed line */
        afirst2();
    } else {
        struct item *d1 = a5->nextitem, *d2 = a5->lastitem;

        numlines--;
        if (!d2) {
            /* .first */
            firstitem = d1;
#ifdef TED_HOST
            /* host-test-only guard (no known path): with an inconsistent list
               (numlines > 1 but a single line) the asm clears the long at
               address 4 (`clr.l lastitem(a0)`, a0 = 0); the Amiga build
               does the same. */
            if (!d1) {
                ORIG_HAZARD("freeupa5: numlines > 1 but single line");
            } else
#endif
            d1->lastitem = NULL;
        } else if (!d1) {
            /* .last */
            enditem = d2;
            d2->nextitem = NULL;
        } else {
            d2->nextitem = d1;
            d1->lastitem = d2;
        }
    }
    /* .free */
    if (islabelline(a5->chars))
        mforget2();
    /* .fa5s */
    d0 = (UWORD)(a5->numchars + lenitem);
    memalt = d0;
    ted_free(a5, d0);                           /* jmp freemem(a6) */
}

/* ted.s:3783 calcitem - work out item address */
struct item *calcitem(void)
{
    struct item *a4 = firstitem;
    UWORD d0 = (UWORD)(curstop + cursy);

    while (d0--)                                /* beq .done / dbf */
        a4 = a4->nextitem;
    /* .done */
    thisitem = a4;
    return a4;
}

/* ted.s:3794 makeline2 */
struct item *makeline2(UBYTE *a3, struct item *a4)
{
    return makeline(a4, (UWORD)(a3 - charbuff + 1));
}

/* ted.s:3799 makefirst */
struct item *makefirst(struct item *a4)
{
    UWORD d5 = charstobuff(charbuff2);

    return makeline(a4, d5);                    /* falls through into makeline */
}

/* ted.s:3803 makeline - make charbuff current line (this item - a4, len
   in d5).  survey-04 I.12: freethis frees THISITEM (all callers: a4 == thisitem). */
struct item *makeline(struct item *a4, UWORD d5)
{
    struct item *a0, *d1;
    const UBYTE *a1;
    UBYTE *p;

    a0 = alloc2((UWORD)(d5 + lenitem), TED_MEMF_PUBLIC);
    if (islabelline(charbuff))
        remember(a0);
    /* .skipz */
    if (a4 == firstitem)
        firstitem = a0;
    /* .skip2 */
    if (a4 == enditem)
        enditem = a0;
    /* .skip3 */
    a0->numchars = (UBYTE)d5;
    d1 = a4->lastitem;
    if (d1)
        d1->nextitem = a0;
    /* .skip4 */
    a0->lastitem = d1;
    d1 = a4->nextitem;
    if (d1)
        d1->lastitem = a0;
    /* .skip5 */
    a0->nextitem = d1;
    a4 = a0;
    a1 = charbuff;
    p = a0->chars;
    while ((*p++ = *a1++) != 0)                 /* .loop2 */
        ;
    freethis();
    thisitem = a4;
    return a4;
}

/* ted.s:4043 afirst2 */
struct item *afirst2(void)
{
    struct item *a0 = allocnull();

    if (!a0)
        return NULL;                            /* .done, Z=1 */
    firstitem = a0;
    enditem = a0;
    thisitem = a0;
    return a0;
}

/* ted.s:4041 allocfirst */
void allocfirst(void)
{
    afirst2();
    cls();                                      /* bra cls */
}

/* ------------------------------------------------------- block cursor */

/* ted.s:3727 bloksetup2 - find first item of block
   (a5 = btopy line item, d6 = btop line; a4 = this item, d7 = this line) */
UWORD bloksetup2(void)
{
    UWORD d7 = (UWORD)(curstop + cursy);
    struct item *a5 = thisitem;                 /* asm: a4 (= thisitem at all callers) */
    UWORD d6 = d7;

    if (d6 != btopy) {
        if (d6 >= btopy) {                      /* bcc .up (unsigned) */
            do {                                /* .up */
                a5 = a5->lastitem;
                d6--;
            } while (d6 != btopy);
        } else {
            do {                                /* .down */
                a5 = a5->nextitem;
                d6++;
            } while (d6 != btopy);
        }
    }
    /* ted.s:3750 found */
    bc.a5 = a5;
    bc.d6 = d6;
    return d7;
}

/* ted.s:3752 bloksetup */
void bloksetup(void)
{
    bloksetup2();
    bc.d5 = btopx;
    if (tokenise)
        detoka5(bc.a5);                         /* bra detoka5 */
    /* .hendrix */
}

/* ted.s:3849 getbchar - get next char from block: N (-1) at the end of
   the block, Z (0) at the end of a line; a5 = item, d6 = y, d5 = x */
WORD getbchar(void)
{
    if (bc.d5 >= (WORD)chcols)                  /* bge newline (signed) */
        goto newline;
    if (bc.d6 < bboty)                          /* bcs getbchar2 (unsigned) */
        return getbchar2();
    if (bc.d6 == bboty && (UWORD)bc.d5 <= (UWORD)bbotx)  /* bls: unsigned */
        return getbchar2();
    /* .eob */
    return -1;

newline:
    /* ted.s:3884 newline */
    bc.a5 = bc.a5->nextitem;
    bc.d6++;
    bc.d5 = 0;
    return 0;
}

/* ted.s:3865 getbchar2 */
WORD getbchar2(void)
{
    bc.d5++;
    if (tokenise) {
        const UBYTE *a3 = charbuff3;

        if ((UBYTE)bc.d5 >= a3[0])              /* cmp.b (a3),d5 / bcc .space */
            goto space;
        return a3[bc.d5];                       /* tst d0: make plus! */
    }
    /* .skipz */
    if ((UBYTE)bc.d5 >= bc.a5->numchars)        /* cmp.b numchars(a5),d5 */
        goto space;
    /* move.b chars-1(a5,d5),d0 sets N for bytes >= $80: the callers see the
       end of the block (bmi) - BUG(orig) (#362) survey-04 I.9, kept as a negative
       return value */
    return (WORD)(BYTE)bc.a5->chars[bc.d5 - 1];

space:
    /* .space */
    if (blokop) {
        /* bne newline (ted.s:3884, in getbchar) */
        bc.a5 = bc.a5->nextitem;
        bc.d6++;
        bc.d5 = 0;
        return 0;
    }
    return 32;
}

/* ------------------------------------------------------- block state */

/* ted.s:3890 bloken - enable block menu items */
void bloken(void)
{
    blokmenus(TRUE);                            /* or #16,menu4item*+12 */
    bltext[0] = 'b';                            /* move.l #'bloc' / 'k' */
    bltext[1] = 'l';
    bltext[2] = 'o';
    bltext[3] = 'c';
    bltext[4] = 'k';
    blokprnt2();                                /* falls through into blokprnt2 */
}

/* ted.s:3903 blokdis - disable above */
void blokdis(void)
{
    bstx = -1;
    blokmenus(FALSE);                           /* and #$ffff-16,menu4item*+12 */
    bboty = (UWORD)-1;
    bltext[0] = ' ';
    bltext[1] = ' ';
    bltext[2] = ' ';
    bltext[3] = ' ';
    bltext[4] = ' ';
    blokprnt2();                                /* (asm saves d0/a0-a2 around it) */
}

/* ted.s:3919 blokadj */
void blokadj(UWORD d0)
{
    if (bboty == (UWORD)-1)
        return;
    if (d0 > bboty)                             /* bhi (unsigned) */
        return;
    bboty++;
    if (d0 > btopy)
        return;
    btopy++;
}

/* ted.s:3947 sortblox.  *r = d0-d3 at the rts: btopx/btopy/bbotx/bboty
   as loaded, exchanged in the swap path - i.e. always the ordered values
   (= the globals afterwards). */
void sortblox(struct quad *r)
{
    WORD d0 = btopx, d2 = bbotx, t;
    UWORD d1 = btopy, d3 = bboty, u;

    if (d1 < d3)                                /* bcs .ok */
        goto ok;
    if (d1 == d3 && !(d0 > d2))                 /* .skip: bgt .swap (signed) */
        goto ok;
    /* .swap */
    t = d0; d0 = d2; d2 = t;
    u = d1; d1 = d3; d3 = u;
    btopx = d0;
    btopy = d1;
    bbotx = d2;
    bboty = d3;
ok:
    if (r) {
        r->d0 = d0;
        r->d1 = (WORD)d1;
        r->d2 = d2;
        r->d3 = (WORD)d3;
    }
}

/* ted.s:3965 orderem */
void orderem(struct quad *r)
{
    WORD t;

    if ((UWORD)r->d1 < (UWORD)r->d3)            /* bcs .noswap */
        return;
    if (r->d1 == r->d3 && r->d0 <= r->d2)       /* ble .noswap (signed) */
        return;
    /* .swap */
    t = r->d0; r->d0 = r->d2; r->d2 = t;
    t = r->d1; r->d1 = r->d3; r->d3 = t;
}

/* ------------------------------------------------ line output (chmap) */

/* register state of showa2item and its helpers (asm a2/a3/d2/d3) */
static struct {
    const UBYTE *a2;            /* source (chars) */
    UBYTE *a3;                  /* chmap row */
    UWORD d2;                   /* chmap column */
    UWORD d3;                   /* screen column */
    void (*draw)(UBYTE);
} sa;

/* the chmap part of print (ted.s line 5132 -> print2 5167): d0 (bit 7
   if color != normcol) to chmap + chloc + cursoff + cursx (word adds),
   then draw (printa0/print4 on the Amiga) */
static void sa_print(UBYTE d0)
{
    UBYTE d2 = d0;

    if (color != normcol)
        d2 |= 128;
    chmap[(WORD)chloc + cursoff + (WORD)cursx] = d2;
    if (sa.draw)
        sa.draw(d0);
}

/* normprint2 (ted.s line 4143, below in chmap_showa2item's family) */
static void sa_normprint2(UBYTE d0)
{
    if (sa.d2 < (UWORD)cursoff)                 /* bcs .skip (unsigned) */
        return;
    if (sa.d3 >= edcols)
        return;
    cursx = sa.d3;
    sa_print(d0);
    sa.d3++;
}

/* normprint (ted.s line 4142) */
static void sa_normprint(UBYTE d0)
{
    sa.a3[(WORD)sa.d2] = d0;
    sa_normprint2(d0);                          /* falls through */
}

/* tokprint (ted.s line 4152); the asm destroys a5 (exg trick) */
static void sa_tokprint(UBYTE d0)
{
    const char *a5;

    if (!tokenise || !(d0 & 0x80)) {
        sa_normprint(d0);
        return;
    }
    a5 = findtoke((UWORD)((d0 << 8) | *sa.a2++));
    color = tokcol;                             /* .doit */
    for (;;) {                                  /* .doit2 */
        UBYTE c = (UBYTE)*a5++;

        if (!c)
            break;
        sa.a3[(WORD)sa.d2] = c;
        sa.a3[(WORD)sa.d2] |= 0x80;             /* bset.b #7 */
        sa_normprint2(c);
        sa.d2++;
    }
    /* .pdone */
    color = normcol;
    sa.d2--;
}

/* ted.s:4104 (showa2item, core part).  Labels of the asm routine family:
   ted.s:4124 endofline, ted.s:4142 normprint, ted.s:4143 normprint2,
   ted.s:4152 tokprint (helpers sa_* above). */
void chmap_showa2item(struct item *a2, void (*draw)(UBYTE d0))
{
    UWORD savex = cursx;                        /* move cursx,-(a7) */
    UBYTE d0;

    sa.a2 = a2->chars;
    sa.a3 = chmap + (WORD)chloc;                /* add.w chloc,a3 */
    sa.d3 = 0;
    sa.d2 = 0;
    sa.draw = draw;
    d0 = *sa.a2++;
    if (d0) {
        if (d0 == label)
            sa_normprint(d0);                   /* label line: no tokens at col 0 */
        else
            goto skip;
        for (;;) {                              /* .loop */
            sa.d2++;
            d0 = *sa.a2++;
            if (!d0)
                break;
skip:
            sa_tokprint(d0);                    /* .skip */
        }
    }
    /* endofline: block line -> pad the screen row with spaces */
    if (bboty != (UWORD)-1) {
        UWORD y = (UWORD)(curstop + cursy);

        if (y >= btopy && y <= bboty) {
            while (sa.d3 < edcols) {            /* .done3 */
                cursx = sa.d3;
                sa_print(32);
                sa.d3++;
            }
        }
    }
    /* .done2 */
    cursx = savex;
}

/* ted.s:4050 (showall, core part) */
void chmap_showall(void (*draw)(UBYTE d0))
{
    struct item *a2, *next;
    UWORD d1, d7;

    storepos();
    cls();
    a2 = thisitem;
    for (d1 = cursy; d1; d1--)                  /* .loop: no NULL check */
        a2 = a2->lastitem;
    /* .skip */
    cursy = d1;
    scloc = 0;
    chloc = 0;
    d7 = (UWORD)(edrows - 1);
    do {                                        /* .loop2 */
        next = a2->nextitem;
        chmap_showa2item(a2, draw);
        if (!next)
            break;                              /* beq restorepos */
        a2 = next;
        godown();
    } while (d7-- != 0);                        /* dbf d7,.loop2 */
    restorepos();
}

/* ted.s:4073 (showtopitem, core part) */
void chmap_showtopitem(void (*draw)(UBYTE d0))
{
    struct item *a2 = thisitem;
    UWORD d1;

    storepos();
    scloc = 0;
    chloc = 0;
    for (d1 = cursy; d1; d1--) {                /* .loop */
        if (!a2->lastitem)
            goto skip2;
        a2 = a2->lastitem;
    }
    /* .skip */
    cursy = d1;
    chmap_showa2item(a2, draw);
skip2:
    restorepos();
}

/* ted.s:4088 (showbotitem, core part) */
void chmap_showbotitem(void (*draw)(UBYTE d0))
{
    struct item *a2 = thisitem;

    storepos();
    for (;;) {                                  /* .loop */
        if ((UWORD)(cursy + 1) >= edrows) {     /* bcc .skip */
            chmap_showa2item(a2, draw);
            break;
        }
        if (!a2->nextitem)
            break;                              /* .skip2 */
        a2 = a2->nextitem;
        godown();
    }
    restorepos();
}

/* --------------------------------------------------------- chmap line */

/* ted.s:4173 charstobuff - transfer on screen chars to character buffer
   (charbuff); null terminate and put length into d5 */
UWORD charstobuff(const UBYTE *a2)
{
    UBYTE *a1 = charbuff, *a0;
    UWORD d1 = (UWORD)(chcols - 1);

    if (tokenise)
        return tokit(charbuff, a2, (WORD)d1, NULL);  /* bne tokit */
    a0 = a1;
    do {                                        /* .loop */
        *a0++ = *a2++;
        if (a0[-1] != ' ')
            a1 = a0;
    } while (d1-- != 0);                        /* dbf d1,.loop */
    /* ted.s:4190 enddo */
    *a1++ = 0;
    return (UWORD)(a1 - charbuff);              /* sub.l (a7)+,d5 */
}

/* ted.s:4195 tokit2 */
UWORD tokit2(UBYTE *a1, const UBYTE *a2, WORD d1, UBYTE **a1end)
{
    return tokit3(a1, a2, d1, a1end);           /* move.l a1,-(a7) / bra tokit3 */
}
