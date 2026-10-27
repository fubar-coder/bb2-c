/*
 * tokit.c - chunk 05 core (ted.s 4198-5252): see tokit.h.
 *
 * The tokeniser, alpha, cursor row arithmetic, line alloc/free, donew and
 * the chmap half of the blitter routines.  The UI half of chunk 05 is
 * src/screen.c.
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

/* ---------------------------------------------------------- tokeniser */

/* ted.s:4210 tokskip - the main tokenise loop.  d1 = last char on line.
   The asm keeps [a4][start of a1][return] on the stack; it is left only
   through putchar -> pchover (two levels at once) or .found -> pchover2,
   both `bra enddo` (ted.s line 4190: clr.b (a1)+, d5 = a1 - start).
   Here: goto enddo.  Reads beyond d1 while matching a token name and for
   the character behind it (no length check; in a chmap row that is the
   next row - BUG(orig) survey-05 A/I.10). */
static UWORD tokskip(UBYTE *a1, const UBYTE *a2, WORD d1, UBYTE **a1end)
{
    UBYTE *const start = a1;                    /* (a7) for enddo */
    const struct tokennode *a3;
    const UBYTE *a4, *name;
    UWORD d0;
    UBYTE d2, d3;

/* ted.s:4319 putchar: move.b d2,(a1)+ / addq #1,d0 / cmp d1,d0 /
   bhi pchover.  ted.s:4324 pchover: addq #4,a7 (drops putchar's return),
   then pchover2 (pops a4) and enddo. */
#define putchar_() do {                                         \
        *a1++ = d2;                                             \
        d0++;                                                   \
        if (d0 > (UWORD)d1)                                     \
            goto enddo;                                         \
    } while (0)

    d0 = 0;
    invflag = 0;                                /* moveq #0,d0 / move d0,invflag */
loop2:
    d2 = *a2++ & 0x7F;                          /* bclr #7,d2 */
    if (d2 == quote)
        goto quoteon;
    /* .hendrix */
    if (invflag)
        goto pch;
    if (alpha(d2))
        goto tsttoke;
ppp2:
    if (d2 == '_' || d2 == '.' || d2 == '\\')
        goto unto2;
    if (d2 == remark) {
        for (;;) {                              /* .remloop: ends only via pchover */
            putchar_();
            d2 = *a2++ & 0x7F;
        }
    }
    /* .notrem */
    if (d2 != '$')
        goto nothex;
hexloop:
    putchar_();
    d2 = *a2++ & 0x7F;
    if (d2 < '0')
        goto nothex;
    if (d2 > 'f')
        goto nothex;
    if (d2 <= '9')
        goto hexloop;
    if (d2 < 'A')
        goto nothex;
    if (d2 >= 'a')
        goto hexloop;
    if (d2 <= 'F')
        goto hexloop;
nothex:
    /* the current character goes out unchecked (also quote/remark/_) */
    putchar_();
    d2 = *a2++ & 0x7F;
    if (d2 == quote)
        goto quoteon;
    if (!alpha(d2))
        goto ppp2;
tsttoke:
    a2--;
    a3 = tokens;
#ifdef TED_HOST
    if (!a3) {
        /* host-test-only guard.  BUG(orig) (survey-05 I.10): with tokens
           == 0 the asm walks a "node" at address 0 (name at 6, num at 4,
           next at 0); the Amiga build does the same through a3 = 0.  The
           host cannot read address 0: no token matches. */
        ORIG_HAZARD("tokit: tokens == 0 (asm reads a node at address 0)");
        goto notoke;
    }
#endif
loop3:
    a4 = a2;
    name = (const UBYTE *)a3->name;             /* addq #6,a3 */
    for (;;) {                                  /* .loop4 */
        d3 = *name++;
        if (d3 == 0)
            goto found;
        d2 = *a4++ & 0x7F;
        if ((UBYTE)(d3 | 32) != (UBYTE)(d2 | 32))
            break;
    }
next:
    a3 = a3->next;
    if (!a3)
        goto notoke;
    goto loop3;
notoke:
    d2 = *a2++ & 0x7F;
unto2:
    putchar_();
    d2 = *a2++ & 0x7F;
    if (d2 == quote)
        goto quoteon;
    if (alpha(d2))
        goto unto2;
    goto ppp2;

found:
    /* .found: a letter or _ behind the name -> next token */
    d2 = *a4 & 0x7F;
    if (alpha(d2))
        goto next;
    if (d2 == '_')
        goto next;
    /* .putina3 */
    *a1++ = (UBYTE)((a3->num >> 8) | 0x80);
    *a1++ = (UBYTE)a3->num;
    if ((ULONG)d0 + (ULONG)(a4 - a2) > (ULONG)(UWORD)d1 + 1)
        ORIG_HAZARD("tokit: token name matched beyond the last character");
    d0 = (UWORD)(d0 + (UWORD)(a4 - a2));
    if (d0 > (UWORD)d1)
        goto enddo;                             /* bhi pchover2 */
    a2 = a4;
    goto loop2;

quoteon:
    invflag = (UWORD)~invflag;                  /* not invflag */
pch:
    putchar_();
    goto loop2;

enddo:
    /* ted.s:4325 pchover2: move.l (a7)+,a4 / bra enddo */
    *a1++ = 0;
    if (a1end)
        *a1end = a1;
    return (UWORD)(a1 - start);
#undef putchar_
}

/* ted.s:4198 tokit - tokenise the line!  First, find last character in
   line! (raw byte compare: $A0 is not a space) */
UWORD tokit(UBYTE *a1, const UBYTE *a2, WORD d1, UBYTE **a1end)
{
    for (;;) {
        /* ted.s:4202 tokit4 */
        if (a2[d1] != ' ')
            return tokskip(a1, a2, d1, a1end);
        /* tokit3 (ted.s line 4204): subq #1,d1 / bpl tokit4 */
        if (--d1 < 0)
            break;
    }
    /* all spaces: addq #4,a7 (start dropped), clr.b (a1), d5 = 1 -
       a1 stays ON the 0 (survey-05 I.16) */
    *a1 = 0;
    if (a1end)
        *a1end = a1;
    return 1;
}

/* ted.s:4204 tokit3 - entry with d1 = LENGTH (tokit2) */
UWORD tokit3(UBYTE *a1, const UBYTE *a2, WORD d1, UBYTE **a1end)
{
    if (--d1 >= 0)                              /* subq #1,d1 / bpl tokit4 */
        return tokit(a1, a2, d1, a1end);
    *a1 = 0;                                    /* empty line */
    if (a1end)
        *a1end = a1;
    return 1;
}

/* ted.s:4328 alpha - return eq if d2 is alphabetic */
BOOL alpha(UBYTE d2)
{
    if (d2 < 65 || d2 > 122)
        return FALSE;
    if (d2 < 91)
        return TRUE;                            /* .yes */
    return d2 >= 97;
}

/* ted.s:4340 alphad0 */
BOOL alphad0(UBYTE d0)
{
    return alpha(d0);                           /* exg d0,d2 around alpha */
}

/* ------------------------------------------------------ cursor rows */

/* ted.s:4530 godown */
void godown(void)
{
    cursy++;
    scloc += (ULONG)fontmul;                    /* moveq #0 / move.w / add.l */
    chloc = (UWORD)(chloc + chcols);
}

/* ted.s:4538 goup - move cursor up one line */
void goup(void)
{
    cursy--;
    scloc -= (ULONG)fontmul;
    chloc = (UWORD)(chloc - chcols);
}

/* ted.s:4976 storepos */
void storepos(void)
{
    cursx2 = cursx;                             /* move.l cursx,cursx2 */
    cursy2 = cursy;
    scloc2 = scloc;
    chloc2 = chloc;
}

/* ted.s:4981 restorepos */
void restorepos(void)
{
    cursx = cursx2;
    cursy = cursy2;
    scloc = scloc2;
    chloc = chloc2;
}

/* ted.s:5234 calcpos - calculate pos of cursor in bitmaps */
void calcpos(void)
{
    scloc = (LONG)((ULONG)cursy * fontmul);     /* mulu */
    chloc = (UWORD)((ULONG)cursy * chcols);
}

/* ---------------------------------------------------------- line list */

/* ted.s:4547 allocnull */
struct item *allocnull(void)
{
    struct item *a0;

    memalt = (UWORD)-1;
    a0 = ted_alloc(lenitem + 1, TED_MEMF_PUBLIC);
    if (!a0)
        return NULL;                            /* Z=1 */
    a0->nextitem = NULL;
    a0->lastitem = NULL;
    a0->numchars = 1;                           /* move #$100,numchars(a0) */
    a0->chars[0] = 0;
    return a0;
}

/* ted.s:4560 donew - erase all.  The comment "return z=1 (beq) if a
   problem" is wrong: d0 = -1 always, allocnull failing is ignored
   (BUG(orig) survey-05 I.6). */
void donew(void)
{
    color = normcol;
    blokdis();
    cursx = 0;                                  /* clr.l cursx */
    cursy = 0;
    scloc = 0;
    chloc = 0;
    curstop = 0;
    cursoff = 0;
    allalt = 0;
    filepath[0] = 0;
    filename[0] = 0;
    numlines = 1;
    allocfirst();
    showhprop();
    showvprop();
}

/* ted.s:4582 freethis - free up line at this item (not unlinked) */
void freethis(void)
{
    struct item *a1 = thisitem;
    const UBYTE *p = a1->chars;
    UWORD d0;

    while (*p == ' ')
        p++;
    if (*p == label)
        mforget2();
    /* .skip: moveq #lenitem,d0 / add.b numchars(a1),d0 - BYTE add:
       wrong FreeMem size for numchars >= 247 (BUG(orig) survey-05 I.5;
       the host's ted_free reports it) */
    d0 = (UBYTE)(lenitem + a1->numchars);
    memalt = d0;
    ted_free(a1, d0);                           /* jmp freemem(a6) */
}

/* ted.s:4602 freeall - free all items in editor.  The list globals stay
   (dangling); numlines == 0 would be 65536 rounds (dbf). */
void freeall(void)
{
    struct item *a2 = firstitem, *a1;
    UWORD d2 = (UWORD)(numlines - 1);
    UWORD d0;

    do {                                        /* .loop */
        a1 = a2;
#ifdef TED_HOST
        if (!a1) {
            /* host-test-only guard.  BUG(orig): with numlines > list
               length (numlines == 0: 65536 rounds) the asm reads the
               "item" at address 0 and calls FreeMem(0, ...) - the Amiga
               build does the same through a1 = 0; the host stops. */
            ORIG_HAZARD("freeall: more rounds than lines");
            return;
        }
#endif
        d0 = (UBYTE)(lenitem + a1->numchars);   /* add.b, as freethis */
        a2 = a1->nextitem;
        memalt = d0;
        ted_free(a1, d0);
    } while (d2-- != 0);                        /* dbf d2,.loop */
}

/* --------------------------------------- chmap half of the blitter ops */

/* The asm blits chmap with BLTSIZE = height << 6 | width (words); height
   0 = 1024 rows, width 0 = 64 words (chcols = 128 -> chcols/2 & 63 = 0 is
   relied upon).  Modulo 0 for every chmap blit that has more than one row
   (cls, scrollup, scrolldown); the one-row fills have BLTDMOD = blitmod,
   which does not matter for a single row. */
static ULONG blt_rows(UWORD size)
{
    return (size >> 6) ? (ULONG)(size >> 6) : 1024;
}

static ULONG blt_bytes(UWORD size)
{
    return 2 * ((size & 63) ? (ULONG)(size & 63) : 64);
}

/* fill with BLTADAT = $2020 (BLTCON0 $1F0, A DMA off) */
static void blt_fill(UBYTE *d, UWORD size)
{
    ULONG n = blt_rows(size) * blt_bytes(size);

    while (n--)
        *d++ = ' ';
}

/* NEW: chmap part of scrollup (ted.s:4346): rows row+1..edrows-1 one up
   (BLTCON0 $9F0 ascending, A = D + chcols, only if d6 != 0), then the last
   row filled with spaces (BLTSIZE chcols/2 | 64 without `and #63`: right
   only because 64|64 = 64, survey-05 I.4) */
void chmap_scrollup(UWORD row)
{
    WORD d6 = (WORD)(edrows - row - 1);         /* rows to scroll */
    UBYTE *a0;

    if (d6 != 0) {
        UBYTE *d = chmap + (ULONG)row * chcols;
        const UBYTE *s = d + chcols;
        UWORD size = (UWORD)((UWORD)((UWORD)d6 << 6) | ((chcols >> 1) & 63));
        ULONG n = blt_rows(size) * blt_bytes(size);

        if (d6 < 0)     /* row >= edrows: huge blit (survey-05 I.13) */
            ORIG_HAZARD("scrollup: row >= edrows");
        while (n--)
            *d++ = *s++;
    }
    /* .skip ... last row: chmap + (edrows-1)*chcols (add.w to a0) */
    a0 = chmap + (WORD)(UWORD)((edrows - 1) * chcols);
    blt_fill(a0, (UWORD)((chcols >> 1) | 64));
}

/* NEW: chmap part of scrolldown (ted.s:4434) INCLUDING the chmap part of
   eraseline (ted.s:4479), which scrolldown falls into: rows row..edrows-2
   one down (descending from the last word, A = D - chcols), then row
   `row` filled with spaces.  BLTSIZE d6 << 6 | chcols/2 without `and #63`
   copies one row more for even d6 (survey-05 I.4): that extra row lands
   on row `row` (erased right after) and is read from the row before -
   before chmap for row 0 (BUG(orig), reproduced; the host build stops
   there, host-test-only guard). */
void chmap_scrolldown(UWORD row)
{
    WORD d6 = (WORD)(edrows - row - 1);         /* rows to scroll */

    if (d6 != 0) {
        UBYTE *d = chmap + (ULONG)chcols * edrows - 1;  /* last byte */
        const UBYTE *s;
        UWORD size = (UWORD)((UWORD)((UWORD)d6 << 6) | (chcols >> 1));
        ULONG n = blt_rows(size) * blt_bytes(size);

        if (d6 < 0)
            ORIG_HAZARD("scrolldown: row >= edrows");
        s = d - chcols;
        while (n--) {
#ifdef TED_HOST
            /* host-test-only guard: the Amiga build reads the bytes in
               front of chmap like the blit (BUG(orig), written to row
               `row`, which eraseline clears right after) */
            if (s < chmap) {
                ORIG_HAZARD("scrolldown: blit reads before chmap");
                break;
            }
#endif
            *d-- = *s--;
        }
    }
    chmap_eraseline(row);                       /* falls through into eraseline */
}

/* NEW: chmap part of eraseline (ted.s:4479): row = spaces */
void chmap_eraseline(UWORD row)
{
    blt_fill(chmap + (ULONG)row * chcols,       /* mulu chcols,d7 */
             (UWORD)(((chcols >> 1) & 63) | 64));
}

/* NEW: chmap part of cls (ted.s:4986): edrows rows of spaces */
void chmap_cls(void)
{
    blt_fill(chmap, (UWORD)((UWORD)(edrows << 6) | ((chcols >> 1) & 63)));
}
