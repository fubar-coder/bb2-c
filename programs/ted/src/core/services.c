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
void blockend(void)
{
    WORD d0, d2;
    UWORD d1, d3;

    d2 = bstx;                                  /* x start */
    if (d2 < 0) {
        beepem();                               /* bmi beepem */
        return;
    }
    /* BUG(orig) survey-03 I.2: `move d0,btopx` with the d0 of mainedit's
       dispatch = the jumptable code byte (130); its upper byte is 0 unless
       showthisitem ran before (undefined there) - C: the event code */
    btopx = (WORD)(UBYTE)ev_d7;
    d3 = (UWORD)bsty;                           /* y start */
    d1 = (UWORD)(curstop + cursy);              /* y end */
    d0 = (WORD)(cursoff + cursx);               /* x end */
    if (d0 == d2 && d1 == d3) {
        blokdis();
        showall();                              /* bra showall */
        return;
    }
    /* .doit (survey-03 I.3: end (0,0) gives bbotx = -1) */
    if (d0 == 0 && d1 != 0) {
        d1--;
        d0 = (WORD)chcols;
    }
    /* .skip */
    d0--;
    bbotx = d0;
    bboty = d1;
    btopx = d2;
    btopy = d3;
    sortblox(NULL);
    bloken();
    showall();                                  /* bra showall */
}

/* ted.s:2636 tokone - tokenise one line - line in a0 (in place) */
UWORD tokone(UBYTE *a0)
{
    const UBYTE *p = a0;
    WORD d1 = 0;

    do                                          /* .loop */
        d1++;
    while (*p++);
    d1--;
    if (d1 >= 0)                                /* bpl tokit2 */
        return tokit2(a0, a0, d1, NULL);
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
    struct item *a4 = firstitem;
    UWORD cnt = numlines;                       /* move numlines,-(a7) */
    WORD d1;
    UWORD d5;

    do {
        /* .loop */
        d1 = (WORD)(a4->numchars - 1);
        if (d1 >= 0) {                          /* bmi .noway */
            d5 = tokit2(charbuff, a4->chars, d1, NULL);
            thisitem = a4;
            a4 = makeline(a4, d5);
        }
        /* .noway */
        a4 = a4->nextitem;
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

/* ted.s:2673 detokallz2.  survey-03 I.14: no check that the text fits
   into charbuff / 255 bytes (numchars is a byte); thisitem stays on the
   last line, no redraw. */
void detokallz2(void)
{
    struct item *a4 = firstitem;
    UWORD cnt = numlines;
    UBYTE *a1;

    do {
        /* .loop */
        a1 = detok(a4->chars, charbuff);
        thisitem = a4;
        a4 = makeline(a4, (UWORD)(a1 - charbuff + 1));
        a4 = a4->nextitem;
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

/* ted.s:2697 (help, core part): walk the tokenised line a4 up to column
   col (d3) counting screen columns (token = length of its name); the
   last token seen is the result (survey-03 I.13).  *num = helptoke. */
const char *help_core(struct item *a4, UWORD col, UWORD *num)
{
    const UBYTE *p = a4->chars;                 /* a4 = start of line */
    const char *d1 = NULL;                      /* no token found yet */
    const char *a3;
    UWORD d0, d2 = 0;                           /* chk x */

    for (;;) {
        /* .loop */
        d0 = *p++;
        if (d0 == 0)
            break;                              /* .done */
        if (d0 & 0x80) {
            d0 = (UWORD)((d0 << 8) | *p++);
            *num = d0;                          /* move d0,helptoke */
            a3 = findtoke(d0);
            d1 = a3;
            do                                  /* .loop2 */
                d2++;
            while (*a3++);
            d2 -= 2;
        }
        /* .next */
        d2++;
        if (d2 > col)                           /* cmp d3,d2 / bls .loop */
            break;
    }
    return d1;
}

/* ------------------------------------------------- label column (mice) */

/* ted.s:2850 makemice - scan entire program for mousables! */
void makemice(void)
{
    struct item *a0 = firstitem;
    UWORD d0 = (UWORD)(numlines - 1);
    UBYTE d1 = label;
    const UBYTE *a1;

    do {
        /* .loop */
        a1 = a0->chars;
        while (*a1 == 32)                       /* .loop2 */
            a1++;
        if (*a1 == d1) {
            /* move d1,mousealt: the upper byte of d1 is undefined in the
               asm (survey-03 I.19) - only != 0 counts */
            mousealt = d1;
            remember2(a0);
        }
        /* .skip */
        a0 = a0->nextitem;
    } while (d0-- != 0);                        /* dbf d0,.loop */
}

/* ted.s:2869 freemice */
void freemice(void)
{
    UWORD d2 = nummice;
    struct micenode *a1, *a2;

    if (!d2)
        return;
    mousealt = d2;
    d2--;
    a2 = mice;
    do {
        /* .loop - reads `next` of the last node too (uninitialised,
           survey-03 I.4), but does not use it */
        a1 = a2;
        a2 = a2->next;
        ted_free(a1, 8);
    } while (d2-- != 0);                        /* dbf d2,.loop */
    nummice = 0;                                /* mice stays dangling */
}

/* ted.s:2883 remember - remember a0 line for mousables */
void remember(struct item *a0)
{
    (void)a0;
    mousealt = (UWORD)-1;
}

/* ted.s:2887 remember2 */
void remember2(struct item *a0)
{
    struct micenode *a1, *a2;
    WORD d1;

    a2 = ted_alloc(8, TED_MEMF_PUBLIC);
    if (!a2)
        outmem();                               /* beq outmem */
    /* BUG(orig) survey-03 I.4: a2->next stays uninitialised (not
       MEMF_CLEAR) */
    if (nummice == 0) {
        mice = a2;                              /* .skip */
    } else {
        a1 = mice;
        d1 = (WORD)(nummice - 2);
        if (d1 >= 0) {                          /* bmi .skip3 */
            do                                  /* .loop */
                a1 = a1->next;
            while (d1-- != 0);
        }
        /* .skip3 */
        a1->next = a2;
    }
    /* .skip2 */
    a2->line = a0;
    nummice++;
}

/* ted.s:2912 mforget2 - forget a1 if in mouse list */
void mforget2(void)
{
    mousealt = (UWORD)-1;
}

/* ted.s:2990 a0toname - convert a0 to a path/filename.
   BUG(orig) survey-03 I.1: the tests compare d0 (the caller's register),
   not the copied character - normally filepath = "" and filename = the
   whole string. */
void a0toname(const UBYTE *a0, ULONG d0)
{
    UBYTE *a1 = (UBYTE *)filepath, *a2 = a1, *z, c, last;
    const UBYTE c0 = (UBYTE)d0;

    if (*a0 == '"')
        a0++;
    for (;;) {
        /* .loop */
        c = *a1++ = *a0++;
        if (!c)
            break;                              /* .done */
        if (c0 == ':')
            a2 = a1;                            /* .skip2 */
        else if (c0 == '/')
            a2 = a1 - 1;
    }
    /* .done */
    a1 = (UBYTE *)filename;
    z = a2;                                     /* move.l a2,a0 */
    if (*a2 == '/')
        a2++;
    do {                                        /* .loop2 */
        c = *a1++ = *a2++;
#ifdef TED_HOST
        if (c && a1 == (UBYTE *)filename + sizeof(filename)) {
            /* host-only guard: the asm runs on into filetext */
            ORIG_HAZARD("a0toname: name longer than filename");
            a1[-1] = 0;
            break;
        }
#endif
    } while (c);
    if (a1 - 2 >= (UBYTE *)filename) {
        last = a1[-2];
    } else {
        /* empty name: -2(a1) is the last byte of filepath */
        ORIG_HAZARD("a0toname: reads the byte in front of filename");
        last = (UBYTE)filepath[sizeof(filepath) - 1];
    }
    if (last == '"') {
        if (a1 - 2 >= (UBYTE *)filename)
            a1[-2] = 0;
        else
            filepath[sizeof(filepath) - 1] = 0;
    }
    /* .sskip */
    *z = 0;
}

/* ted.s:3033 alloc2 (a6 = ExecBase) */
APTR alloc2(ULONG d0, ULONG d1)
{
    APTR p;

    memalt = (UWORD)-1;
    p = ted_alloc(d0, d1);
    if (!p)
        outmem();                               /* beq outmem */
    return p;
}
