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
ULONG divu_w(ULONG dividend, UWORD divisor)
{
    ULONG r, q = 0;
    int i;

    if (divisor == 0) {
        /* 68000: zero divide trap (guru 5).  BUG(orig) (#385) (tabstop = 0,
           decision O3); host-test-only guard: */
        ORIG_HAZARD("divu by 0 (68000 zero divide trap)");
        fprintf(stderr, "ted core: divu by zero (e.g. tabstop = 0) - the "
                "68000 traps here (zero divide guru); host build aborts\n");
        abort();
    }
    if ((dividend >> 16) >= divisor)
        return dividend;                        /* quotient > $FFFF: V=1, unchanged */
    r = dividend >> 16;
    for (i = 15; i >= 0; i--) {
        r = (r << 1) | ((dividend >> i) & 1);
        q <<= 1;
        if (r >= divisor) {
            r -= divisor;
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
    tedfh fh;

    fh = ted_open(defname, TED_MODE_OLDFILE);
    if (fh) {
        /* the asm reads straight into defstart; a short read keeps the
           rest - the image starts as a copy of defstart for that */
        defs_tobytes(img, &defstart);
        ted_read(fh, img, (LONG)sizeof(img));   /* result unchecked */
        defs_frombytes(&defstart, img);
        ted_close(fh);
    }
    /* .skip */
    setdefs(&defstart);                         /* bra setdefs */
}

/* ted.s:3078 savedefs */
void savedefs(void)
{
    UBYTE img[sizeof(struct defblock)];
    tedfh fh;

    fh = ted_open(defname, TED_MODE_NEWFILE);
    if (!fh)
        return;                                 /* .skip: errors ignored */
    defs_tobytes(img, &defstart);
    ted_write(fh, img, (LONG)sizeof(img));      /* result unchecked */
    ted_close(fh);                              /* jmp close(a6) */
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
UBYTE *numtoascii2(UWORD num, UBYTE *buf)
{
    buf = numtoascii(num, buf);
    *buf = 0;                                   /* clr.b (a0) - a0 stays */
    return buf;
}

/* ted.s:3144 numtoascii - convert number to ascii (number in d0, buffer
   in a0); no leading zeros, no 0 byte */
UBYTE *numtoascii(UWORD num, UBYTE *buf)
{
    ULONG v = num;                              /* and.l #$ffff,d0 */
    BOOL printed = FALSE;                       /* printed anything flag */
    int i;

    for (i = 0; i < 5; i++) {                   /* divu (a1)+,d0: no overflow */
        ULONG r = divu_w(v, powersof10[i]);
        UWORD q = (UWORD)r;

        if (q || printed) {
            printed = TRUE;
            *buf++ = (UBYTE)(q + 48);
        }
        v = r >> 16;                            /* clr d0 / swap d0 */
    }
    if (!printed)
        *buf++ = '0';
    return buf;
}

/* ------------------------------------------------------ reading files */

/* ted.s:3307 doopen - open the file (name in d7, mode in d2).  The
   comment "and fill buffer" is wrong: the caller calls fillbuff. */
BOOL doopen(const UBYTE *name, LONG mode)
{
    rd.fh = ted_open((const char *)name, mode);
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
    rd.ptr = dosbuff;
    rd.left = ted_read(rd.fh, dosbuff, doslen);
    if (rd.left < 0) {                          /* bpl .skip */
        error(error2);
        rd.left = 0;
    }
    return rd.left;                             /* asm: Z=1 if 0 */
}

/* ted.s:3325 doread - next character from the file (or <0 if eof!).
   asm: N=0 with the byte in d0.b, d0 = -1 / N=1 at EOF.  d6 counts with
   subq.w; it never exceeds doslen, so a LONG gives the same result. */
WORD doread(void)
{
    for (;;) {
        /* .skip2 */
        if (--rd.left >= 0)
            return *rd.ptr++;
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
int readline(UWORD *length)
{
    UBYTE *dst;
    WORD c;
    BOOL ascii;                                 /* flag for no toke on load! */
    ULONG len;
    int ret;

    c = doread();
    if (c < 0)
        return RL_EOF;                          /* .skip */
    dst = charbuff2;
    ascii = FALSE;
    for (;;) {
        /* .loop */
        if (c & 0x80) {                         /* token pair, copied unchecked */
            *dst++ = (UBYTE)c;
            c = doread();
            if (c < 0)
                goto oops;
            *dst++ = (UBYTE)c;
            goto nottab;
        }
        /* .tryout */
        if (c == 10) {
            c = 0;                              /* file is in ascii format */
            ascii = TRUE;                       /* flag it for a toke-up */
        }
        /* .skip2 */
        *dst++ = (UBYTE)c;
        if (c == 0)
            goto skip3;
        if (c == 9) {
            ULONG col;

            dst--;
            col = (ULONG)(dst - charbuff2);
            do {                                /* .tabloop */
                if (dst >= charbuff4)
                    ORIG_HAZARD("readline: TAB expansion beyond charbuff4");
                *dst++ = ' ';
                col = (col & 0xFFFF0000) | (UWORD)(col + 1);    /* addq.w */
            } while ((UWORD)(divu_w(col, tabstop) >> 16) != 0);   /* swap / tst */
        }
nottab:
        /* .nottab */
        if (dst >= charbuff3) {                 /* bcs .ok */
            dst = charbuff3 - 3;
            goto oops;
        }
        /* .ok */
        c = doread();
        if (c < 0)
            goto oops;
    }
oops:
    /* .oops: EOF in the line or buffer full - "no eol" */
    *dst++ = 0;
    len = (ULONG)(dst - charbuff2);
    ret = RL_NOEOL;
    goto sit;
skip3:
    /* .skip3 */
    len = (ULONG)(dst - charbuff2);
    ret = RL_OK;
sit:
    /* .sit: should i toke it? */
    if (ascii && tokenise) {
        /* Yup! */
        len = (len & 0xFFFF0000) | (UWORD)(len - 1);    /* subq.w #1,d5 */
        if ((WORD)len < 0)
            len = 1;                            /* (never: d5 >= 1) */
        else
            len = mytoke((UWORD)len);           /* .oktt */
    }
    /* .no */
    *length = (UWORD)len;
    return ret;
}

/* ted.s:3447 mytoke */
UWORD mytoke(UWORD len)
{
    /* d1 = d5 (length), a1 = a2 = charbuff2: in place */
    return tokit2(charbuff2, charbuff2, (WORD)len, NULL);
}

/* ted.s:3349 allocline */
struct item *allocline(UWORD len)
{
    struct item *item;
    const UBYTE *src;
    UBYTE *dst;

    /* move.l d5,d0 / add.w #lenitem,d0 */
    item = alloc2((UWORD)(len + lenitem), TED_MEMF_PUBLIC);
    item->nextitem = NULL;
    /* lastitem stays uninitialised (survey-04 I.13; no defect): the
       callers set it (fileopen explicitly, linka0) */
    item->numchars = (UBYTE)len;
    src = charbuff2;
    dst = item->chars;
    while ((*dst++ = *src++) != 0)
        ;
    if (islabelline(item->chars))
        remember(item);                         /* beq remember */
    return item;
}

/* ted.s:3277 linka0 - add a0 after a4 */
struct item *linka0(struct item *item, struct item *prev)
{
    struct item *next;

    numlines++;
    next = prev->nextitem;
    if (next) {
        item->nextitem = next;
        next->lastitem = item;
    } else {
        /* .end: a0->nextitem is NOT cleared (allocline/allocnull did) */
        enditem = item;
    }
    /* .end2 */
    item->lastitem = prev;
    prev->nextitem = item;
    thisitem = item;
    return item;
}

/* ted.s:3293 bloadadd */
UBYTE *bloadadd(UBYTE *dst)
{
    const UBYTE *src = charbuff2;

    for (;;) {
        UBYTE c = *src++;

        *dst++ = c;
        if (c == 0)
            break;                              /* .done */
        if (c & 0x80)
            *dst++ = *src++;                    /* second byte of a token */
    }
    return dst - 1;                             /* subq #1,a3: ON the 0 */
}

/* ---------------------------------------------- tokens / detokeniser */

/* ted.s:3455 detok - detokenise a0 to a1.  No length limit (survey-04
   I.8).  A token byte in front of the 0 (readline I.3) takes the 0 as its
   low byte and runs on behind the line, as in the asm. */
UBYTE *detok(const UBYTE *src, UBYTE *dst)
{
    for (;;) {
        UBYTE c = *src++;
        const char *name;

        if (c == 0)
            break;                              /* .done */
        if (!(c & 0x80)) {
            *dst++ = c;
            continue;
        }
        /* .skip */
        name = findtoke((UWORD)((c << 8) | *src++));
        while ((*dst++ = (UBYTE)*name++) != 0)  /* .skip3 */
            ;
        dst--;
    }
    *dst = 0;
    return dst;
}

/* ted.s:3472 findtoke */
const char *findtoke(UWORD tok)
{
    const struct tokennode *node = tokens;

    tok &= 0x7FFF;                              /* bclr #15,d0 */
#ifdef TED_HOST
    if (!node) {
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
        if (node->num == tok)
            return node->name;                  /* .got: addq #6,a3 */
        if (!node->next)
            return tokeerr;                     /* .notoke */
        node = node->next;
    }
}

/* ted.s:3759 detoka5 */
void detoka5(struct item *line)
{
    UBYTE *dst = charbuff3 + 1;
    const UBYTE *src = line->chars;

    /* bsr .loop (subroutine in the subroutine) */
    for (;;) {
        UBYTE c = *src++;
        const char *name;

        if (c == 0)
            break;                              /* .done */
        if (!(c & 0x80)) {
            if (dst >= charbuff4)
                ORIG_HAZARD("detoka5 writes beyond charbuff3 (allocation end)");
            *dst++ = c;
            continue;
        }
        /* .skip */
        name = findtoke((UWORD)((c << 8) | *src++));
        for (;;) {                              /* .found */
            if (dst >= charbuff4)
                ORIG_HAZARD("detoka5 writes beyond charbuff3 (allocation end)");
            if ((*dst++ = (UBYTE)*name++) == 0)
                break;
        }
        dst--;
    }
    /* length byte = n + 1, no 0 terminator; overflows beyond 255 */
    charbuff3[0] = (UBYTE)(dst - charbuff3);
}

/* ------------------------------------------------------------- search */

/* ted.s:3523 isithere */
WORD isithere(struct item *line, UBYTE **text)
{
    return isithere2(line, 0, text);            /* moveq #0,d5, falls through */
}

/* ted.s:3530 isithere2 - test if find buffer is in line at a4; return
   d0 < 0 if not, else the start position; d5 = start position */
WORD isithere2(struct item *line, UWORD col, UBYTE **text)
{
    UBYTE *pos = line->chars;
    UBYTE *base, *p;
    const UBYTE *find;

    if (tokenise) {
        pos = charbuff;
        detok(line->chars, charbuff);
    }
    /* .skip */
    base = pos;                                 /* asm a1 */
    if (text)
        *text = base;
    {
        const UBYTE *e = base;

        while (*e)
            e++;
        if ((LONG)(WORD)col > (LONG)(e - base))
            ORIG_HAZARD("isithere2: start column behind the line end");
    }
    pos += (WORD)col;                           /* add.w d5,a5 */
    for (;;) {
        /* .loop0 */
        p = pos;
        find = findreqla16;
        for (;;) {
            /* .loop */
            UBYTE want = *find++, got;

            if (want == 0)
                return (WORD)(pos - base);      /* .found: move a5,d0 */
            if (!findcase() && alphad0(want))
                want &= 255 - 32;
            /* .skip2 */
            got = *p++;
            if (got == 0)
                return -1;                      /* .notfound */
            if (!findcase() && alpha(got))
                got &= 255 - 32;
            /* .skip3 */
            if (want != got)
                break;
        }
        pos++;
    }
}

/* --------------------------------------------------------- line list */

/* ted.s:3684 freeupa5 - unlink a5 from list */
void freeupa5(struct item *line)
{
    UWORD size;

    if (numlines == 1) {
        /* .freeonly: BUG(orig) (#361) (survey-04 I.11): if allocnull fails,
           first/end/thisitem keep pointing at the freed line */
        afirst2();
    } else {
        struct item *next = line->nextitem, *prev = line->lastitem;

        numlines--;
        if (!prev) {
            /* .first */
            firstitem = next;
#ifdef TED_HOST
            /* host-test-only guard (no known path): with an inconsistent list
               (numlines > 1 but a single line) the asm clears the long at
               address 4 (`clr.l lastitem(a0)`, a0 = 0); the Amiga build
               does the same. */
            if (!next) {
                ORIG_HAZARD("freeupa5: numlines > 1 but single line");
            } else
#endif
            next->lastitem = NULL;
        } else if (!next) {
            /* .last */
            enditem = prev;
            prev->nextitem = NULL;
        } else {
            prev->nextitem = next;
            next->lastitem = prev;
        }
    }
    /* .free */
    if (islabelline(line->chars))
        mforget2();
    /* .fa5s */
    size = (UWORD)(line->numchars + lenitem);
    memalt = size;
    ted_free(line, size);                       /* jmp freemem(a6) */
}

/* ted.s:3783 calcitem - work out item address */
struct item *calcitem(void)
{
    struct item *item = firstitem;
    UWORD n = (UWORD)(curstop + cursy);

    while (n--)                                 /* beq .done / dbf */
        item = item->nextitem;
    /* .done */
    thisitem = item;
    return item;
}

/* ted.s:3794 makeline2 */
struct item *makeline2(UBYTE *end, struct item *line)
{
    return makeline(line, (UWORD)(end - charbuff + 1));
}

/* ted.s:3799 makefirst */
struct item *makefirst(struct item *line)
{
    UWORD len = charstobuff(charbuff2);

    return makeline(line, len);                 /* falls through into makeline */
}

/* ted.s:3803 makeline - make charbuff current line (this item - a4, len
   in d5).  survey-04 I.12: freethis frees THISITEM (all callers: a4 == thisitem). */
struct item *makeline(struct item *line, UWORD len)
{
    struct item *item, *prev, *next;
    const UBYTE *src;
    UBYTE *dst;

    item = alloc2((UWORD)(len + lenitem), TED_MEMF_PUBLIC);
    if (islabelline(charbuff))
        remember(item);
    /* .skipz */
    if (line == firstitem)
        firstitem = item;
    /* .skip2 */
    if (line == enditem)
        enditem = item;
    /* .skip3 */
    item->numchars = (UBYTE)len;
    prev = line->lastitem;
    if (prev)
        prev->nextitem = item;
    /* .skip4 */
    item->lastitem = prev;
    next = line->nextitem;
    if (next)
        next->lastitem = item;
    /* .skip5 */
    item->nextitem = next;
    src = charbuff;
    dst = item->chars;
    while ((*dst++ = *src++) != 0)              /* .loop2 */
        ;
    freethis();
    thisitem = item;
    return item;
}

/* ted.s:4043 afirst2 */
struct item *afirst2(void)
{
    struct item *item = allocnull();

    if (!item)
        return NULL;                            /* .done, Z=1 */
    firstitem = item;
    enditem = item;
    thisitem = item;
    return item;
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
    UWORD cur = (UWORD)(curstop + cursy);
    struct item *item = thisitem;               /* asm: a4 (= thisitem at all callers) */
    UWORD line = cur;

    if (line != btopy) {
        if (line >= btopy) {                    /* bcc .up (unsigned) */
            do {                                /* .up */
                item = item->lastitem;
                line--;
            } while (line != btopy);
        } else {
            do {                                /* .down */
                item = item->nextitem;
                line++;
            } while (line != btopy);
        }
    }
    /* ted.s:3750 found */
    bc.item = item;
    bc.line = line;
    return cur;
}

/* ted.s:3752 bloksetup */
void bloksetup(void)
{
    bloksetup2();
    bc.col = btopx;
    if (tokenise)
        detoka5(bc.item);                       /* bra detoka5 */
    /* .hendrix */
}

/* ted.s:3849 getbchar - get next char from block: N (-1) at the end of
   the block, Z (0) at the end of a line; a5 = item, d6 = y, d5 = x */
WORD getbchar(void)
{
    if (bc.col >= (WORD)chcols)                 /* bge newline (signed) */
        goto newline;
    if (bc.line < bboty)                        /* bcs getbchar2 (unsigned) */
        return getbchar2();
    if (bc.line == bboty && (UWORD)bc.col <= (UWORD)bbotx) /* bls: unsigned */
        return getbchar2();
    /* .eob */
    return -1;

newline:
    /* ted.s:3884 newline */
    bc.item = bc.item->nextitem;
    bc.line++;
    bc.col = 0;
    return 0;
}

/* ted.s:3865 getbchar2 */
WORD getbchar2(void)
{
    bc.col++;
    if (tokenise) {
        const UBYTE *a3 = charbuff3;

        if ((UBYTE)bc.col >= a3[0])             /* cmp.b (a3),d5 / bcc .space */
            goto space;
        return a3[bc.col];                      /* tst d0: make plus! */
    }
    /* .skipz */
    if ((UBYTE)bc.col >= bc.item->numchars)     /* cmp.b numchars(a5),d5 */
        goto space;
    /* move.b chars-1(a5,d5),d0 sets N for bytes >= $80: the callers see the
       end of the block (bmi) - BUG(orig) (#362) survey-04 I.9, kept as a negative
       return value */
    return (WORD)(BYTE)bc.item->chars[bc.col - 1];

space:
    /* .space */
    if (blokop) {
        /* bne newline (ted.s:3884, in getbchar) */
        bc.item = bc.item->nextitem;
        bc.line++;
        bc.col = 0;
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
void blokadj(UWORD line)
{
    if (bboty == (UWORD)-1)
        return;
    if (line > bboty)                           /* bhi (unsigned) */
        return;
    bboty++;
    if (line > btopy)
        return;
    btopy++;
}

/* ted.s:3947 sortblox.  *r = d0-d3 at the rts: btopx/btopy/bbotx/bboty
   as loaded, exchanged in the swap path - i.e. always the ordered values
   (= the globals afterwards). */
void sortblox(struct quad *r)
{
    WORD topx = btopx, botx = bbotx, t;
    UWORD topy = btopy, boty = bboty, u;

    if (topy < boty)                            /* bcs .ok */
        goto ok;
    if (topy == boty && !(topx > botx))         /* .skip: bgt .swap (signed) */
        goto ok;
    /* .swap */
    t = topx; topx = botx; botx = t;
    u = topy; topy = boty; boty = u;
    btopx = topx;
    btopy = topy;
    bbotx = botx;
    bboty = boty;
ok:
    if (r) {
        r->topx = topx;
        r->topy = (WORD)topy;
        r->botx = botx;
        r->boty = (WORD)boty;
    }
}

/* ted.s:3965 orderem */
void orderem(struct quad *r)
{
    WORD t;

    if ((UWORD)r->topy < (UWORD)r->boty)        /* bcs .noswap */
        return;
    if (r->topy == r->boty && r->topx <= r->botx) /* ble .noswap (signed) */
        return;
    /* .swap */
    t = r->topx; r->topx = r->botx; r->botx = t;
    t = r->topy; r->topy = r->boty; r->boty = t;
}

/* ------------------------------------------------ line output (chmap) */

/* register state of showa2item and its helpers (asm a2/a3/d2/d3); lives
   on chmap_showa2item's stack, the helpers get a pointer to it */
struct showa2 {
    const UBYTE *src;           /* a2: source (chars) */
    UBYTE *row;                 /* a3: chmap row */
    UWORD col;                  /* d2: chmap column */
    UWORD scrcol;               /* d3: screen column */
    void (*draw)(UBYTE);
};

/* the chmap part of print (ted.s line 5132 -> print2 5167): c (bit 7
   if color != normcol) to chmap + chloc + cursoff + cursx (word adds),
   then draw (printa0/print4 on the Amiga) */
static void sa_print(struct showa2 *sa, UBYTE c)
{
    UBYTE cell = c;

    if (color != normcol)
        cell |= 128;
    chmap[(WORD)chloc + cursoff + (WORD)cursx] = cell;
    if (sa->draw)
        sa->draw(c);
}

/* normprint2 (ted.s line 4143, below in chmap_showa2item's family) */
static void sa_normprint2(struct showa2 *sa, UBYTE c)
{
    if (sa->col < (UWORD)cursoff)               /* bcs .skip (unsigned) */
        return;
    if (sa->scrcol >= edcols)
        return;
    cursx = sa->scrcol;
    sa_print(sa, c);
    sa->scrcol++;
}

/* normprint (ted.s line 4142) */
static void sa_normprint(struct showa2 *sa, UBYTE c)
{
    sa->row[(WORD)sa->col] = c;
    sa_normprint2(sa, c);                       /* falls through */
}

/* tokprint (ted.s line 4152); the asm destroys a5 (exg trick) */
static void sa_tokprint(struct showa2 *sa, UBYTE c)
{
    const char *name;

    if (!tokenise || !(c & 0x80)) {
        sa_normprint(sa, c);
        return;
    }
    name = findtoke((UWORD)((c << 8) | *sa->src++));
    color = tokcol;                             /* .doit */
    for (;;) {                                  /* .doit2 */
        UBYTE ch = (UBYTE)*name++;

        if (!ch)
            break;
        sa->row[(WORD)sa->col] = ch;
        sa->row[(WORD)sa->col] |= 0x80;         /* bset.b #7 */
        sa_normprint2(sa, ch);
        sa->col++;
    }
    /* .pdone */
    color = normcol;
    sa->col--;
}

/* ted.s:4104 (showa2item, core part).  Labels of the asm routine family:
   ted.s:4124 endofline, ted.s:4142 normprint, ted.s:4143 normprint2,
   ted.s:4152 tokprint (helpers sa_* above). */
void chmap_showa2item(struct item *line, void (*draw)(UBYTE c))
{
    UWORD savex = cursx;                        /* move cursx,-(a7) */
    struct showa2 state, *sa = &state;
    UBYTE c;

    sa->src = line->chars;
    sa->row = chmap + (WORD)chloc;              /* add.w chloc,a3 */
    sa->scrcol = 0;
    sa->col = 0;
    sa->draw = draw;
    c = *sa->src++;
    if (c) {
        if (c == label)
            sa_normprint(sa, c);                /* label line: no tokens at col 0 */
        else
            goto skip;
        for (;;) {                              /* .loop */
            sa->col++;
            c = *sa->src++;
            if (!c)
                break;
skip:
            sa_tokprint(sa, c);                 /* .skip */
        }
    }
    /* endofline: block line -> pad the screen row with spaces */
    if (bboty != (UWORD)-1) {
        UWORD y = (UWORD)(curstop + cursy);

        if (y >= btopy && y <= bboty) {
            while (sa->scrcol < edcols) {       /* .done3 */
                cursx = sa->scrcol;
                sa_print(sa, 32);
                sa->scrcol++;
            }
        }
    }
    /* .done2 */
    cursx = savex;
}

/* ted.s:4050 (showall, core part) */
void chmap_showall(void (*draw)(UBYTE c))
{
    struct item *item, *next;
    UWORD row, n;

    storepos();
    cls();
    item = thisitem;
    for (row = cursy; row; row--)               /* .loop: no NULL check */
        item = item->lastitem;
    /* .skip */
    cursy = row;
    scloc = 0;
    chloc = 0;
    n = (UWORD)(edrows - 1);
    do {                                        /* .loop2 */
        next = item->nextitem;
        chmap_showa2item(item, draw);
        if (!next)
            break;                              /* beq restorepos */
        item = next;
        godown();
    } while (n-- != 0);                         /* dbf d7,.loop2 */
    restorepos();
}

/* ted.s:4073 (showtopitem, core part) */
void chmap_showtopitem(void (*draw)(UBYTE c))
{
    struct item *item = thisitem;
    UWORD row;

    storepos();
    scloc = 0;
    chloc = 0;
    for (row = cursy; row; row--) {             /* .loop */
        if (!item->lastitem)
            goto skip2;
        item = item->lastitem;
    }
    /* .skip */
    cursy = row;
    chmap_showa2item(item, draw);
skip2:
    restorepos();
}

/* ted.s:4088 (showbotitem, core part) */
void chmap_showbotitem(void (*draw)(UBYTE c))
{
    struct item *item = thisitem;

    storepos();
    for (;;) {                                  /* .loop */
        if ((UWORD)(cursy + 1) >= edrows) {     /* bcc .skip */
            chmap_showa2item(item, draw);
            break;
        }
        if (!item->nextitem)
            break;                              /* .skip2 */
        item = item->nextitem;
        godown();
    }
    restorepos();
}

/* --------------------------------------------------------- chmap line */

/* ted.s:4173 charstobuff - transfer on screen chars to character buffer
   (charbuff); null terminate and put length into d5 */
UWORD charstobuff(const UBYTE *row)
{
    UBYTE *end = charbuff, *dst;
    UWORD n = (UWORD)(chcols - 1);

    if (tokenise)
        return tokit(charbuff, row, (WORD)n, NULL);  /* bne tokit */
    dst = end;
    do {                                        /* .loop */
        *dst++ = *row++;
        if (dst[-1] != ' ')
            end = dst;
    } while (n-- != 0);                         /* dbf d1,.loop */
    /* ted.s:4190 enddo */
    *end++ = 0;
    return (UWORD)(end - charbuff);             /* sub.l (a7)+,d5 */
}

/* ted.s:4195 tokit2 */
UWORD tokit2(UBYTE *dst, const UBYTE *src, WORD len, UBYTE **dstend)
{
    return tokit3(dst, src, len, dstend);       /* move.l a1,-(a7) / bra tokit3 */
}
