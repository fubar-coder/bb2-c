/*
 * services.c - chunk 03 core (ted.s 1981-3062): see services.h.
 *
 * F1/F2 block marks, tokenise/detokenise all lines (comdata+16/+20/+56),
 * the token lookup of the help key, the label column list ("mice"),
 * a0toname and alloc2.  The UI part of the chunk is src/mouse.c.
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

WORD bstx = -1;                 /* ted.s:2455 F1 block start x (-1 = none) */
WORD bsty;                      /* ted.s:2456 */

/* ted.s:2458 blockstart - f1, genam style */
void blockstart(void)
{
    bstx = (WORD)(cursoff + cursx);
    bsty = (WORD)(curstop + cursy);
}

/* ted.s:2468 blockend - f2, genam style */
void blockend(const struct tedevent *ev)
{
    WORD endx, startx;
    UWORD endy, starty;

    startx = bstx;                              /* x start */
    if (startx < 0) {
        beepem();                               /* bmi beepem */
        return;
    }
    /* survey-03 I.2 (no effect): `move d0,btopx` with the d0 of mainedit's
       dispatch = the jumptable code byte (130); its upper byte is 0 unless
       showthisitem ran before (undefined there) - C: the event code */
    btopx = (WORD)(UBYTE)ev->code;
    starty = (UWORD)bsty;                       /* y start */
    endy = (UWORD)(curstop + cursy);            /* y end */
    endx = (WORD)(cursoff + cursx);             /* x end */
    if (endx == startx && endy == starty) {
        blokdis();
        showall();                              /* bra showall */
        return;
    }
    /* .doit (survey-03 I.3: end (0,0) gives bbotx = -1) */
    if (endx == 0 && endy != 0) {
        endy--;
        endx = (WORD)chcols;
    }
    /* .skip */
    endx--;
    bbotx = endx;
    bboty = endy;
    btopx = startx;
    btopy = starty;
    sortblox(NULL);
    bloken();
    showall();                                  /* bra showall */
}

/* ted.s:2636 tokone - tokenise one line - line in a0 (in place) */
UWORD tokone(UBYTE *line)
{
    const UBYTE *p = line;
    WORD last = 0;

    do                                          /* .loop */
        last++;
    while (*p++);
    last--;
    if (last >= 0)                              /* bpl tokit2 */
        return tokit2(line, line, last, NULL);
    /* lines >= 32768 bytes: rts with d5 undefined */
    return 0;
}

/* ted.s:2648 tokall - tokenise entire source */
void tokall(void)
{
    if (tokenise) {
        beepem();                               /* bne beepem */
        return;
    }
    tokenise = (UWORD)~tokenise;                /* not tokenise: $FFFF */
    tokall2();                                  /* falls through into tokall2 */
}

/* ted.s:2653 tokall2 */
void tokall2(void)
{
    struct item *item = firstitem;
    UWORD cnt = numlines;                       /* move numlines,-(a7) */
    WORD last;
    UWORD len;

    do {
        /* .loop */
        last = (WORD)(item->numchars - 1);
        if (last >= 0) {                        /* bmi .noway */
            len = tokit2(charbuff, item->chars, last, NULL);
            thisitem = item;
            item = makeline(item, len);
        }
        /* .noway */
        item = item->nextitem;
    } while (--cnt != 0);                       /* subq #1,(a7) / bne .loop */
    detokallc();                                /* bra detokallc */
}

/* ted.s:2670 detokallz */
void detokallz(void)
{
    if (!tokenise) {
        beepem();                               /* beq beepem */
        return;
    }
    tokenise = 0;
    detokallz2();                               /* falls through into detokallz2 */
}

/* ted.s:2673 detokallz2.  Fixed (#355) survey-03 I.14: the original does
   not check that the text fits into charbuff / 255 bytes (numchars is a
   byte); detok stops at 254 characters + the 0.  thisitem stays on the
   last line, no redraw. */
void detokallz2(void)
{
    struct item *item = firstitem;
    UWORD cnt = numlines;
    UBYTE *end;

    do {
        /* .loop */
        end = detok(item->chars, charbuff, charbuff + 255);
        thisitem = item;
        item = makeline(item, (UWORD)(end - charbuff + 1));
        item = item->nextitem;
    } while (--cnt != 0);
}

/* ted.s:2689 detokall (no caller) */
void detokall(void)
{
    if (!tokenise) {
        beepem();                               /* already done..... */
        return;
    }
    tokenise = 0;
    detokall2();                                /* falls through into detokall2 */
}

/* ted.s:2692 detokall2 */
void detokall2(void)
{
    detokallz2();
    detokallc();                                /* falls through into detokallc */
}

/* ted.s:2694 detokallc */
void detokallc(void)
{
    calcitem();
    showall();                                  /* bra showall */
}

/* ted.s:2697 (help, core part): walk the tokenised line (a4) up to column
   col (d3) counting screen columns (token = length of its name); the
   last token seen is the result (survey-03 I.13).  *num = helptoke. */
const char *help_core(struct item *line, UWORD col, UWORD *num)
{
    const UBYTE *p = line->chars;               /* a4 = start of line */
    const char *result = NULL;                  /* no token found yet */
    const char *name;
    UWORD x = 0;                                /* d2: chk x */

    for (;;) {
        /* .loop */
        UBYTE c = *p++;                         /* d0 */

        if (c == 0)
            break;                              /* .done */
        if (c & 0x80) {
            UWORD tok = (UWORD)((c << 8) | *p++);   /* d0 */

            *num = tok;                         /* move d0,helptoke */
            name = findtoke(tok);
            result = name;
            do                                  /* .loop2 */
                x++;
            while (*name++);
            x -= 2;
        }
        /* .next */
        x++;
        if (x > col)                            /* cmp d3,d2 / bls .loop */
            break;
    }
    return result;
}

/* ------------------------------------------------- label column (mice) */

/* ted.s:2850 makemice - scan entire program for mousables! */
void makemice(void)
{
    struct item *item = firstitem;
    UWORD count = (UWORD)(numlines - 1);
    UBYTE labelc = label;
    const UBYTE *p;

    do {
        /* .loop */
        p = item->chars;
        while (*p == 32)                        /* .loop2 */
            p++;
        if (*p == labelc) {
            /* move d1,mousealt: the upper byte of d1 is undefined in the
               asm (survey-03 I.19) - only != 0 counts */
            mousealt = labelc;
            remember2(item);
        }
        /* .skip */
        item = item->nextitem;
    } while (count-- != 0);                     /* dbf d0,.loop */
}

/* ted.s:2869 freemice */
void freemice(void)
{
    UWORD count = nummice;
    struct micenode *node, *next;

    if (!count)
        return;
    mousealt = count;
    count--;
    next = mice;
    do {
        /* .loop - reads `next` of the last node too (uninitialised,
           survey-03 I.4), but does not use it */
        node = next;
        next = next->next;
        ted_free(node, 8);
    } while (count-- != 0);                     /* dbf d2,.loop */
    nummice = 0;                                /* mice stays dangling */
}

/* ted.s:2883 remember - remember a0 line for mousables */
void remember(struct item *line)
{
    (void)line;
    mousealt = (UWORD)-1;
}

/* ted.s:2887 remember2 */
void remember2(struct item *line)
{
    struct micenode *node, *newnode;
    WORD count;

    newnode = ted_alloc(8, TED_MEMF_PUBLIC);
    if (!newnode)
        outmem();                               /* beq outmem */
    /* BUG(orig) (#381) survey-03 I.4: a2->next stays uninitialised (not
       MEMF_CLEAR) */
    if (nummice == 0) {
        mice = newnode;                         /* .skip */
    } else {
        node = mice;
        count = (WORD)(nummice - 2);
        if (count >= 0) {                       /* bmi .skip3 */
            do                                  /* .loop */
                node = node->next;
            while (count-- != 0);
        }
        /* .skip3 */
        node->next = newnode;
    }
    /* .skip2 */
    newnode->line = line;
    nummice++;
}

/* ted.s:2912 mforget2 - forget a1 if in mouse list */
void mforget2(void)
{
    mousealt = (UWORD)-1;
}

/* ted.s:2990 a0toname - convert a0 to a path/filename.
   Fixed (#340) survey-03 I.1: the original compares the caller's d0
   (blitz2's LoadSeg result) instead of the copied character, so the
   split depended on the memory layout.  Here the name is never split:
   filepath = "" and filename = the whole string, what the original does
   in the normal case (screen title and file requester as before). */
void a0toname(const UBYTE *src)
{
    UBYTE *dst = (UBYTE *)filepath, *namestart = dst, *pathend, c, last;

    if (*src == '"')
        src++;
    for (;;) {
        /* .loop */
        c = *dst++ = *src++;
        if (!c)
            break;                              /* .done */
    }
    /* .done */
    dst = (UBYTE *)filename;
    pathend = namestart;                        /* move.l a2,a0 */
    if (*namestart == '/')
        namestart++;
    do {                                        /* .loop2 */
        c = *dst++ = *namestart++;
#ifdef TED_HOST
        if (c && dst == (UBYTE *)filename + sizeof(filename)) {
            /* host-only guard: the asm runs on into filetext */
            ORIG_HAZARD("a0toname: name longer than filename");
            dst[-1] = 0;
            break;
        }
#endif
    } while (c);
    if (dst - 2 >= (UBYTE *)filename) {
        last = dst[-2];
    } else {
        /* empty name: -2(a1) is the last byte of filepath */
        ORIG_HAZARD("a0toname: reads the byte in front of filename");
        last = (UBYTE)filepath[sizeof(filepath) - 1];
    }
    if (last == '"') {
        if (dst - 2 >= (UBYTE *)filename)
            dst[-2] = 0;
        else
            filepath[sizeof(filepath) - 1] = 0;
    }
    /* .sskip */
    *pathend = 0;
}

/* ted.s:3033 alloc2 (a6 = ExecBase) */
APTR alloc2(ULONG size, ULONG flags)
{
    APTR p;

    memalt = (UWORD)-1;
    p = ted_alloc(size, flags);
    if (!p)
        outmem();                               /* beq outmem */
    return p;
}
