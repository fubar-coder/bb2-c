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
#include "literal.h"

#ifdef TED_HOST
#include <stdio.h>
#include <stdlib.h>
#endif

const UWORD powersof10[5] = { 10000, 1000, 100, 10, 1 };   /* ted.s:3165 */

#ifdef TED_HOST
/* see fileio.h.  Host only: the Amiga build uses the real DIVU.W from
   src/glue.c (divu_w there), which also traps on a zero divisor like the
   original.  Pure C (shift and subtract). */
ULONG divu_w(ULONG dividend, UWORD divisor)
{
    ULONG r, q = 0;
    int i;

    if (divisor == 0) {
        /* 68000: zero divide trap (guru 5).  The divisors are constants
           and tabstop, which is never 0 (fixed #385: loaddefs clamps it,
           defaults keeps the previous value); host-test-only guard: */
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
        /* Fixed (#385): the original takes the tab size over unchecked; 0
           made every divu tabstop trap, one above 133 overran the line
           buffers (#378).  Clamped to the range of the requester. */
        if (tabstop < 1)
            tabstop = 1;
        else if (tabstop > 79)
            tabstop = 79;
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
BOOL doopen(struct rdstate *rd, const UBYTE *name, LONG mode)
{
    rd->fh = ted_open((const char *)name, mode);
    if (!rd->fh) {
        /* addq #4,a7 / bra error: error1, then back to the CALLER's
           caller - here: FALSE, the caller returns at once */
        error(error1);
        return FALSE;
    }
    rd->carry = -1;                             /* NEW (#375, readline) */
    filehand = rd->fh;                          /* .skip */
    rd->textfile = FALSE;                       /* NEW (#376, readline) */
    rd->looked = FALSE;                         /* NEW (#374, fillbuff) */
    rd->rawnoeol = FALSE;                       /* NEW (#1013, readline) */
    return TRUE;
}

/* ted.s:3302 doclose */
void doclose(struct rdstate *rd)
{
    filehand = 0;
    ted_close(rd->fh);
}

/* NEW (fixed #374): the first buffer of a file holds a LF and no 0 byte -
   a text file (a tokenised source ends every line with a 0; a LF in it
   is the low byte of a token pair) */
static BOOL istext(const UBYTE *p, LONG n)
{
    BOOL lf = FALSE;

    while (n-- > 0) {
        if (*p == 0)
            return FALSE;
        if (*p++ == 10)
            lf = TRUE;
    }
    return lf;
}

/* ted.s:3337 fillbuff.  NEW (#374): the first buffer decides whether the
   file is a text file (rd->textfile, see readline). */
LONG fillbuff(struct rdstate *rd)
{
    rd->ptr = dosbuff;
    rd->left = ted_read(rd->fh, dosbuff, doslen);
    if (rd->left < 0) {                         /* bpl .skip */
        error(error2);
        rd->left = 0;
    }
    if (!rd->looked) {                          /* fixed #374 */
        rd->looked = TRUE;
        if (istext(dosbuff, rd->left))
            rd->textfile = TRUE;
    }
    return rd->left;                            /* asm: Z=1 if 0 */
}

/* ted.s:3325 doread - next character from the file (or <0 if eof!).
   asm: N=0 with the byte in d0.b, d0 = -1 / N=1 at EOF.  d6 counts with
   subq.w; it never exceeds doslen, so a LONG gives the same result. */
WORD doread(struct rdstate *rd)
{
    if (rd->carry >= 0) {                       /* NEW (#375) */
        WORD c = rd->carry;

        rd->carry = -1;
        return c;
    }
    for (;;) {
        /* .skip2 */
        if (--rd->left >= 0)
            return *rd->ptr++;
        /* .skip */
        if (!fillbuff(rd))
            return -1;
    }
}

/* NEW (fixed #377, #375): the number of columns a token pair shows (the
   length of its name: findtoke, as detok shows it). */
static ULONG pairwidth(UBYTE hi, UBYTE lo)
{
    const char *name = findtoke((UWORD)((hi << 8) | lo));
    ULONG w = 0;

    while (*name++)
        w++;
    return w;
}

/* ted.s:3375 readline - read in a line to charbuff2.
   asm: d0 = -1/N=1 no line to read, d0 = 0/Z=1 no eol, d0 = 1 normal;
   length (incl. the 0 byte) in d5.
   Fixed (#376): the original never tokenises the last line without LF
   and lines cut at the buffer end (d4 is only set for LF); here such a
   line is tokenised as the LF lines are when the file is a text file (an
   earlier line ended by LF: rd->textfile) or the line has no byte >= $80
   (no token pair the tokeniser could break).  Fixed (#1013): a line
   without end stays text when the caller asks for it (rd->rawnoeol:
   blokload, which tokenises that line once more with the cursor line's
   tail; tokit clears bit 7 of every byte and destroyed the token pairs).
   Fixed (#375): the original cuts a line that does not fit charbuff2
   (130 bytes) back to charbuff3 - 3 - the three bytes behind the cut are
   lost - and returns "no eol", which blokload takes for the end of the
   file.  The line now ends in front of the character, token pair or TAB
   that would take it beyond 128 shown columns (chcols, a row of the
   character map; a token pair counts with the length of its name) or
   beyond charbuff3 - 1 (129 bytes and the 0); the next call reads that
   byte (or pair) again and goes on with the rest of the physical line as
   a line of its own; the result is RL_FULL.
   Fixed (#374): the original takes every byte >= $80 as the first byte
   of a token pair and the next byte unchecked as its second, also in a
   text file, where it is a Latin-1 character: a LF, TAB or 0 behind it
   was swallowed.  In a text file (rd->textfile: its first buffer has a
   LF and no 0 byte, or an earlier line ended by LF) such a byte is one
   character (one column; in front of the end of the file it stays).
   NEW (literal characters, design D4/D6): with and without tokenise the
   umlauts of the germs table are stored as typing stores them (26-31,
   blitz2 maps them back in strings) and every other such byte as the
   literal character $90 cc (two bytes, one column); a text save writes
   both back as the bytes (textline), so the file is saved as it was
   loaded.  CR stays.
   Fixed (#378): the TAB expansion stops at the end of charbuff2.
   Fixed (#377): a TAB is expanded from the column the line shows (col: a
   token pair counts with the length of its name), not from the number of
   bytes stored.
   Fixed (#379): a token start byte right before the end of the file is
   dropped (the original left a half token in front of the 0). */
int readline(struct rdstate *rd, UWORD *length)
{
    UBYTE *dst;
    UBYTE *const lim = charbuff3 - 1;           /* NEW (#375): last place of the 0 */
    ULONG col = 0;                              /* NEW (#377, #375): shown column */
    WORD c;
    BOOL ascii;                                 /* flag for no toke on load! */
    BOOL pairs = FALSE;                         /* NEW (#376): a byte >= $80 */
    ULONG len;
    int ret;

    c = doread(rd);
    if (c < 0)
        return RL_EOF;                          /* .skip */
    dst = charbuff2;
    ascii = FALSE;
    for (;;) {
        /* .loop (dst <= lim here: the 0 always fits) */
        if ((c & 0x80) && rd->textfile) {
            /* fixed #374; literal characters (design D4, D6) */
            c = (WORD)(UBYTE)germs_map(TED_LITIN(c));   /* $80: '?' */
            if (c & 0x80) {
                if (dst + 2 > lim || (dst > charbuff2 && col >= chcols))
                    goto full;                  /* fixed #375 */
                *dst++ = TED_LITHI;
                *dst++ = (UBYTE)c;
                col++;
                goto nottab;
            }
        }
        if ((c & 0x80) && !rd->textfile) {      /* token pair, copied unchecked */
            WORD lo;
            ULONG w;

            if (dst + 2 > lim)
                goto full;                      /* fixed #375 */
            lo = doread(rd);
            if (lo < 0) {
                /* Fixed (#379): the original keeps the token start byte
                   in front of the 0 - a half token whose low byte is the
                   0, detok reads on behind the line.  It is dropped. */
                goto oops;
            }
            w = pairwidth((UBYTE)c, (UBYTE)lo);
            if (dst > charbuff2 && col + w > chcols) {
                /* fixed #375: the pair starts the next line; lo (doread
                   just took it from dosbuff) goes back behind c */
                rd->ptr--;
                rd->left++;
                goto full;
            }
            pairs = TRUE;
            *dst++ = (UBYTE)c;
            *dst++ = (UBYTE)lo;
            col += w;
            goto nottab;
        }
        /* .tryout */
        if (c == 10) {
            c = 0;                              /* file is in ascii format */
            ascii = TRUE;                       /* flag it for a toke-up */
        }
        /* .skip2 */
        if (c == 0) {
            *dst++ = 0;
            goto skip3;
        }
        if (c == 9) {
            ULONG stop = col;                   /* fixed #377 */

            /* Fixed (#375): a TAB whose blanks do not fit goes to the next
               line (where it starts at column 0), unless nothing is
               stored yet */
            do
                stop = (stop & 0xFFFF0000) | (UWORD)(stop + 1);
            while ((UWORD)(divu_w(stop, tabstop) >> 16) != 0);
            if (dst > charbuff2
                && ((ULONG)(lim - dst) < (UWORD)(stop - col) || stop > chcols))
                goto full;
            do {                                /* .tabloop */
                /* Fixed (#378): the original tests the end of the line
                   buffer only after the expansion (.nottab) and writes up
                   to tabstop-1 blanks behind charbuff2 - with a tab size
                   above 133 from the options file behind TED's buffer
                   block.  The expansion stops at the end of charbuff2
                   (and at column 128, #375). */
                if (dst >= lim || col >= chcols)
                    break;
                *dst++ = ' ';
                col = (col & 0xFFFF0000) | (UWORD)(col + 1);    /* addq.w */
            } while ((UWORD)(divu_w(col, tabstop) >> 16) != 0);   /* swap / tst */
            goto nottab;
        }
        if (dst >= lim || col >= chcols)
            goto full;                          /* fixed #375 */
        *dst++ = (UBYTE)c;
        col++;
nottab:
        /* .nottab / .ok: the test of the buffer end is above (#375) */
        c = doread(rd);
        if (c < 0)
            goto oops;
    }
full:
    /* NEW (#375): the line is full; c is read again by the next call */
    rd->carry = c;
    *dst++ = 0;
    len = (ULONG)(dst - charbuff2);
    ret = RL_FULL;
    ascii = rd->textfile || !pairs;             /* fixed #376 */
    goto sit;
oops:
    /* .oops: EOF in the line - "no eol" */
    *dst++ = 0;
    len = (ULONG)(dst - charbuff2);
    ret = RL_NOEOL;
    ascii = (rd->textfile || !pairs) && !rd->rawnoeol;  /* fixed #376, #1013 */
    goto sit;
skip3:
    /* .skip3 */
    len = (ULONG)(dst - charbuff2);
    ret = RL_OK;
    if (ascii)
        rd->textfile = TRUE;                    /* ended by LF (#376) */
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

/* ted.s:3447 mytoke.  NEW (literal characters): the line is tokenised
   from the scratch row, where each pair $90 cc readline stored is one
   marked character (litrow_expand), into charbuff2 (the original works in
   place; a line without a pair gives the same bytes). */
UWORD mytoke(UWORD len)
{
    /* d1 = d5 (length), a1 = a2 = charbuff2: in place */
    UWORD n = litrow_expand(charbuff2);

    (void)len;                                  /* = n without pairs */
    return tokit2(charbuff2, litscratch, (WORD)n, NULL);
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

/* ted.s:3293 bloadadd.  NEW (#374): a byte >= $80 in front of the 0 is a
   character of a text file (readline, fixed #374), not a token start: the
   copy ends at the 0 (the original went on behind it). */
UBYTE *bloadadd(UBYTE *dst)
{
    const UBYTE *src = charbuff2;

    for (;;) {
        UBYTE c = *src++;

        *dst++ = c;
        if (c == 0)
            break;                              /* .done */
        if (c & 0x80) {
            c = *src++;                         /* second byte of a token */
            *dst++ = c;
            if (c == 0)
                break;                          /* fixed #374 */
        }
    }
    return dst - 1;                             /* subq #1,a3: ON the 0 */
}

/* NEW: see fileio.h */
UBYTE *bloadaddrow(UBYTE *dst)
{
    const UBYTE *src = charbuff2;

    for (;;) {
        UBYTE c = *src++;

        if (TED_ISLIT(c, *src)) {
            litput(dst++, *src++);
            continue;
        }
        *dst++ = c;
        if (c == 0)
            break;
        if (c & 0x80) {
            c = *src++;                         /* second byte of a token */
            *dst++ = c;
            if (c == 0)
                break;
        }
    }
    return dst - 1;
}

/* ---------------------------------------------- tokens / detokeniser */

/* ted.s:3455 detok - detokenise a0 to a1.  Fixed (#355) survey-04 I.8:
   the original has no length limit; the text stops silently where only
   the byte for the 0 is left before end.  A token byte in front of the 0
   takes the 0 as its low byte and runs on behind the line, as in the asm
   (readline no longer stores such a half token, fixed #379). */
UBYTE *detok(const UBYTE *src, UBYTE *dst, const UBYTE *end)
{
    const UBYTE *last = end - 1;                /* room for the 0 */

    for (;;) {
        UBYTE c = *src++;
        const char *name;

        if (c == 0 || dst >= last)
            break;                              /* .done */
        if (!(c & 0x80)) {
            *dst++ = c;
            continue;
        }
        /* .skip */
        if (TED_ISLIT(c, *src)) {               /* NEW: marked (literal.h) */
            litput(dst++, *src++);
            continue;
        }
        name = findtoke((UWORD)((c << 8) | *src++));
        while (*name && dst < last)             /* .skip3 */
            *dst++ = (UBYTE)*name++;
    }
    *dst = 0;
    return dst;
}

/* ted.s:3472 findtoke.  NEW (literal characters): a word $9080-$90FF
   that the token list does not name (no list, AB3 mode, a library set
   without literallib) gets its one-character name from the built-in table
   (litname); the list's entry wins when it has one. */
const char *findtoke(UWORD tok)
{
    const struct tokennode *node = tokens;

    tok &= 0x7FFF;                              /* bclr #15,d0 */
    /* Fixed (#359) survey-05 I.10: with tokens == 0 (standalone TED) the
       original walks a "node" at address 0: compares the word at 4,
       follows the long at 0 and may return the "name" at address 6.  No
       token list: every token is unknown. */
    if (!node)
        goto notoke;
    for (;;) {
        /* .loop */
        if (node->num == tok)
            return node->name;                  /* .got: addq #6,a3 */
        if (!node->next)
            goto notoke;                        /* .notoke */
        node = node->next;
    }
notoke:
    if (TED_ISLITWORD(tok))
        return litname(tok);
    return tokeerr;
}

/* ted.s:3759 detoka5.  Fixed (#355) survey-04 I.8: the original has no
   length limit and writes behind charbuff3, the end of TED's buffer
   block; the text stops silently where one byte is left before
   charbuff4 (bloksave puts the LF there) */
void detoka5(struct item *line)
{
    UBYTE *dst = charbuff3 + 1;
    UBYTE *const last = charbuff4 - 1;
    const UBYTE *src = line->chars;

    /* bsr .loop (subroutine in the subroutine) */
    for (;;) {
        UBYTE c = *src++;
        const char *name;

        if (c == 0 || dst >= last)
            break;                              /* .done */
        if (!(c & 0x80)) {
            *dst++ = c;
            continue;
        }
        /* .skip */
        if (TED_ISLIT(c, *src)) {               /* NEW: marked (literal.h) */
            litput(dst++, *src++);
            continue;
        }
        name = findtoke((UWORD)((c << 8) | *src++));
        while (*name && dst < last)             /* .found */
            *dst++ = (UBYTE)*name++;
    }
    /* length byte = n + 1 (at most chcols + 1), no 0 terminator */
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
        detok(line->chars, charbuff, charbuff4);
    } else if (haslit(line->chars)) {
        /* NEW: the literal characters as the characters (shown columns) */
        pos = charbuff;
        detolit(line->chars, charbuff, charbuff4);
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
            /* NEW (literal characters): an umlaut of the find text is
               compared in the form the text stores it (26-31, germs) */
            want = (UBYTE)germs_map(want);
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
        /* .freeonly.  Fixed (#361): when allocnull fails, outmem reports it
           and the line stays (the original freed it anyway, and
           first/end/thisitem kept pointing at it, survey-04 I.11) */
        if (!afirst2())
            outmem();
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

/* ted.s:4041 allocfirst (afirst2, then cls): no C function - its callers
   allocate the line before they free the old one (donew_with, freeonly;
   fixed #356, #1006) */

/* ------------------------------------------------------- block cursor */

/* ted.s:3727 bloksetup2 - find first item of block
   (a5 = btopy line item, d6 = btop line; a4 = this item, d7 = this line) */
UWORD bloksetup2(struct bcur *bc)
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
    bc->item = item;
    bc->line = line;
    return cur;
}

/* NEW (literal characters): with tokenise off getbchar2 reads the line
   from charbuff3 when it holds a literal character (blokline) */
static BOOL bloklit;
static const struct item *bloklitline;

/* NEW: the text of a block line for getbchar2: detoka5 with tokenise on;
   without tokenise, a line with a literal character in charbuff3 as
   detoka5 writes it (detolit: one marked byte per literal, every other
   byte as stored), another line read from chars as in the original */
void blokline(struct item *line)
{
    if (tokenise) {
        detoka5(line);
        return;
    }
    bloklit = line && haslit(line->chars);
    bloklitline = line;
    if (bloklit)
        charbuff3[0] = (UBYTE)(detolit(line->chars, charbuff3 + 1, charbuff4) - charbuff3);
}

/* ted.s:3752 bloksetup */
void bloksetup(struct bcur *bc)
{
    bloksetup2(bc);
    bc->col = btopx;
    blokline(bc->item);                         /* bra detoka5 (tokenise) */
    /* .hendrix */
}

/* ted.s:3849 getbchar - get next char from block: N (-1) at the end of
   the block, Z (0) at the end of a line; a5 = item, d6 = y, d5 = x */
WORD getbchar(struct bcur *bc)
{
    if (bc->col >= (WORD)chcols)                /* bge newline (signed) */
        goto newline;
    if (bc->line < bboty)                       /* bcs getbchar2 (unsigned) */
        return getbchar2(bc);
    if (bc->line == bboty && (UWORD)bc->col <= (UWORD)bbotx) /* bls: unsigned */
        return getbchar2(bc);
    /* .eob */
    return -1;

newline:
    /* ted.s:3884 newline */
    bc->item = bc->item->nextitem;
    bc->line++;
    bc->col = 0;
    return 0;
}

/* ted.s:3865 getbchar2 */
WORD getbchar2(struct bcur *bc)
{
    bc->col++;
    if (tokenise || (bloklit && bc->item == bloklitline)) {
        const UBYTE *text = charbuff3;          /* a3: [0] = length + 1 */
        UBYTE l;

        if ((UBYTE)bc->col >= text[0])          /* cmp.b (a3),d5 / bcc .space */
            goto space;
        l = litat(&text[bc->col]);
        if (l)
            return (WORD)(TED_BLIT | l);        /* NEW: a literal character */
        return text[bc->col];                   /* tst d0: make plus! */
    }
    /* .skipz */
    if ((UBYTE)bc->col >= bc->item->numchars)   /* cmp.b numchars(a5),d5 */
        goto space;
    /* Fixed (#362) survey-04 I.9: the original's move.b chars-1(a5,d5),d0
       sets N for bytes >= $80, so the callers (bmi) took a Latin-1
       character for the end of the block; zero-extended as in the token
       path */
    return (WORD)bc->item->chars[bc->col - 1];

space:
    /* .space */
    if (blokop) {
        /* bne newline (ted.s:3884, in getbchar) */
        bc->item = bc->item->nextitem;
        bc->line++;
        bc->col = 0;
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
    BOOL full;                  /* NEW (#1011): the row is full (chcols) */
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
    litmap_put(&chmap[(WORD)chloc + cursoff + (WORD)cursx], 0);   /* NEW: no literal */
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

/* normprint (ted.s line 4142).  Fixed (#1011): the original stores at any
   column - a line wider than the row (chcols) ran on into the next row
   and from the last row behind the character map; a character from
   column chcols on is neither stored nor drawn, and the row is full. */
static void sa_normprint(struct showa2 *sa, UBYTE c)
{
    if (sa->col >= chcols) {                    /* fixed #1011 */
        sa->full = TRUE;
        return;
    }
    sa->row[(WORD)sa->col] = c;
    litmap_put(&sa->row[(WORD)sa->col], 0);     /* NEW: no literal */
    sa_normprint2(sa, c);                       /* falls through */
}

/* NEW (literal characters): normprint for the literal character cc - the
   cell holds TED_LITGLYPH, the literal map cc, and cc is drawn (the glyph
   of its code, spec editor/screen-font) in the colour of the text */
static void sa_litprint(struct showa2 *sa, UBYTE cc)
{
    UBYTE *cell;

    if (sa->col >= chcols) {                    /* as normprint (#1011) */
        sa->full = TRUE;
        return;
    }
    sa->row[(WORD)sa->col] = TED_LITGLYPH;
    litmap_put(&sa->row[(WORD)sa->col], cc);
    if (sa->col < (UWORD)cursoff || sa->scrcol >= edcols)
        return;                                 /* as normprint2 */
    cursx = sa->scrcol;
    cell = &chmap[(WORD)chloc + cursoff + (WORD)cursx];   /* as print2 */
    *cell = TED_LITGLYPH;
    litmap_put(cell, cc);
    if (sa->draw)
        sa->draw(cc);
    sa->scrcol++;
}

/* tokprint (ted.s line 4152); the asm destroys a5 (exg trick).  Fixed
   (#1011): a keyword name ends at column chcols, as normprint's
   characters do. */
static void sa_tokprint(struct showa2 *sa, UBYTE c)
{
    const char *name;

    if (TED_ISLIT(c, *sa->src)) {
        /* NEW (literal characters): one cell, drawn as the character in
           the colour of the text, no token bit; with and without tokenise */
        sa_litprint(sa, *sa->src++);
        return;
    }
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
        if (sa->col >= chcols) {                /* fixed #1011 */
            sa->full = TRUE;
            break;
        }
        sa->row[(WORD)sa->col] = ch;
        sa->row[(WORD)sa->col] |= 0x80;         /* bset.b #7 */
        litmap_put(&sa->row[(WORD)sa->col], 0); /* NEW: no literal */
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
    sa->full = FALSE;
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
            if (sa->full)
                break;                          /* fixed #1011 */
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
   (charbuff); null terminate and put length into d5.
   NEW (literal characters): the row is first copied to the scratch row
   with its literal characters (litrow_copy: the cursor row of chmap, a
   line buffer); the tokeniser writes each as the pair $90 cc, and so does
   the copy without tokenise (design D6: one stored form), which ends in
   front of a pair that would take the line beyond TED_LITLIMIT bytes.
   Without a literal character the bytes are the original's (the copy
   also makes charstobuff(charbuff) in place, blalldone, safe when a pair
   is longer than its column). */
UWORD charstobuff(const UBYTE *row)
{
    UBYTE *end = charbuff, *dst;
    UWORD n = (UWORD)(chcols - 1);

    litrow_copy(row, chcols);
    row = litscratch;
    littrunc = FALSE;
    if (tokenise)
        return tokit(charbuff, row, (WORD)n, NULL);  /* bne tokit */
    dst = end;
    do {                                        /* .loop */
        UBYTE l = litat(row);

        if (dst + (l ? 2 : 1) > charbuff + TED_LITLIMIT) {
            /* (only with a literal character): cut, unless only blanks
               are left */
            do
                if (*row++ != ' ')
                    littrunc = TRUE;
            while (n-- != 0);
            break;
        }
        if (l)
            *dst++ = TED_LITHI;
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
