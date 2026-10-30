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
   Here: goto enddo.  Reads beyond `last` while matching a token name and for
   the character behind it (no length check; in a chmap row that is the
   next row - BUG(orig) #386, survey-05 A/I.10). */
static UWORD tokskip(UBYTE *out, const UBYTE *src, WORD last, UBYTE **outend)
{
    UBYTE *const start = out;                   /* (a7) for enddo */
    const struct tokennode *tok;
    const UBYTE *match, *name;
    UWORD count;
    UBYTE c, namec;

/* ted.s:4319 putchar: move.b d2,(a1)+ / addq #1,d0 / cmp d1,d0 /
   bhi pchover.  ted.s:4324 pchover: addq #4,a7 (drops putchar's return),
   then pchover2 (pops a4) and enddo. */
#define putchar_() do {                                         \
        *out++ = c;                                             \
        count++;                                                \
        if (count > (UWORD)last)                                \
            goto enddo;                                         \
    } while (0)

    count = 0;
    invflag = 0;                                /* moveq #0,d0 / move d0,invflag */
loop2:
    c = *src++ & 0x7F;                          /* bclr #7,d2 */
    if (c == quote)
        goto quoteon;
    /* .hendrix */
    if (invflag)
        goto pch;
    if (alpha(c))
        goto tsttoke;
ppp2:
    if (c == '_' || c == '.' || c == '\\')
        goto unto2;
    if (c == remark) {
        for (;;) {                              /* .remloop: ends only via pchover */
            putchar_();
            c = *src++ & 0x7F;
        }
    }
    /* .notrem */
    if (c != '$')
        goto nothex;
hexloop:
    putchar_();
    c = *src++ & 0x7F;
    if (c < '0')
        goto nothex;
    if (c > 'f')
        goto nothex;
    if (c <= '9')
        goto hexloop;
    if (c < 'A')
        goto nothex;
    if (c >= 'a')
        goto hexloop;
    if (c <= 'F')
        goto hexloop;
nothex:
    /* the current character goes out unchecked (also quote/remark/_) */
    putchar_();
    c = *src++ & 0x7F;
    if (c == quote)
        goto quoteon;
    if (!alpha(c))
        goto ppp2;
tsttoke:
    src--;
    tok = tokens;
#ifdef TED_HOST
    if (!tok) {
        /* host-test-only guard.  BUG(orig) (#359) (survey-05 I.10): with tokens
           == 0 the asm walks a "node" at address 0 (name at 6, num at 4,
           next at 0); the Amiga build does the same through tok = 0.  The
           host cannot read address 0: no token matches. */
        ORIG_HAZARD("tokit: tokens == 0 (asm reads a node at address 0)");
        goto notoke;
    }
#endif
loop3:
    match = src;
    name = (const UBYTE *)tok->name;            /* addq #6,a3 */
    for (;;) {                                  /* .loop4 */
        namec = *name++;
        if (namec == 0)
            goto found;
        c = *match++ & 0x7F;
        if ((UBYTE)(namec | 32) != (UBYTE)(c | 32))
            break;
    }
next:
    tok = tok->next;
    if (!tok)
        goto notoke;
    goto loop3;
notoke:
    c = *src++ & 0x7F;
unto2:
    putchar_();
    c = *src++ & 0x7F;
    if (c == quote)
        goto quoteon;
    if (alpha(c))
        goto unto2;
    goto ppp2;

found:
    /* .found: a letter or _ behind the name -> next token */
    c = *match & 0x7F;
    if (alpha(c))
        goto next;
    if (c == '_')
        goto next;
    /* .putina3 */
    *out++ = (UBYTE)((tok->num >> 8) | 0x80);
    *out++ = (UBYTE)tok->num;
    if ((ULONG)count + (ULONG)(match - src) > (ULONG)(UWORD)last + 1)
        ORIG_HAZARD("tokit: token name matched beyond the last character");
    count = (UWORD)(count + (UWORD)(match - src));
    if (count > (UWORD)last)
        goto enddo;                             /* bhi pchover2 */
    src = match;
    goto loop2;

quoteon:
    invflag = (UWORD)~invflag;                  /* not invflag */
pch:
    putchar_();
    goto loop2;

enddo:
    /* ted.s:4325 pchover2: move.l (a7)+,a4 / bra enddo */
    *out++ = 0;
    if (outend)
        *outend = out;
    return (UWORD)(out - start);
#undef putchar_
}

/* ted.s:4198 tokit - tokenise the line!  First, find last character in
   line! (raw byte compare: $A0 is not a space) */
UWORD tokit(UBYTE *out, const UBYTE *src, WORD last, UBYTE **outend)
{
    for (;;) {
        /* ted.s:4202 tokit4 */
        if (src[last] != ' ')
            return tokskip(out, src, last, outend);
        /* tokit3 (ted.s line 4204): subq #1,d1 / bpl tokit4 */
        if (--last < 0)
            break;
    }
    /* all spaces: addq #4,a7 (start dropped), clr.b (a1), d5 = 1 -
       a1 stays ON the 0 (survey-05 I.16) */
    *out = 0;
    if (outend)
        *outend = out;
    return 1;
}

/* ted.s:4204 tokit3 - entry with d1 = LENGTH (tokit2) */
UWORD tokit3(UBYTE *out, const UBYTE *src, WORD len, UBYTE **outend)
{
    if (--len >= 0)                             /* subq #1,d1 / bpl tokit4 */
        return tokit(out, src, len, outend);
    *out = 0;                                   /* empty line */
    if (outend)
        *outend = out;
    return 1;
}

/* ted.s:4328 alpha - return eq if d2 is alphabetic */
BOOL alpha(UBYTE c)
{
    if (c < 65 || c > 122)
        return FALSE;
    if (c < 91)
        return TRUE;                            /* .yes */
    return c >= 97;
}

/* ted.s:4340 alphad0 */
BOOL alphad0(UBYTE c)
{
    return alpha(c);                            /* exg d0,d2 around alpha */
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
    struct item *item;

    memalt = (UWORD)-1;
    item = ted_alloc(lenitem + 1, TED_MEMF_PUBLIC);
    if (!item)
        return NULL;                            /* Z=1 */
    item->nextitem = NULL;
    item->lastitem = NULL;
    item->numchars = 1;                         /* move #$100,numchars(a0) */
    item->chars[0] = 0;
    return item;
}

/* ted.s:4560 donew - erase all.  The comment "return z=1 (beq) if a
   problem" is wrong: d0 = -1 always, allocnull failing is ignored
   (BUG(orig) #356, survey-05 I.6). */
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
    struct item *item = thisitem;
    const UBYTE *p = item->chars;
    UWORD size;

    while (*p == ' ')
        p++;
    if (*p == label)
        mforget2();
    /* .skip: moveq #lenitem,d0 / add.b numchars(a1),d0 - BYTE add:
       wrong FreeMem size for numchars >= 247 (survey-05 I.5, unreachable: numchars <= 130;
       the host's ted_free reports it) */
    size = (UBYTE)(lenitem + item->numchars);
    memalt = size;
    ted_free(item, size);                       /* jmp freemem(a6) */
}

/* ted.s:4602 freeall - free all items in editor.  The list globals stay
   (dangling); numlines == 0 would be 65536 rounds (dbf). */
void freeall(void)
{
    struct item *next = firstitem, *item;
    UWORD count = (UWORD)(numlines - 1);
    UWORD size;

    do {                                        /* .loop */
        item = next;
#ifdef TED_HOST
        if (!item) {
            /* host-test-only guard (unreachable): with numlines > list
               length (numlines == 0: 65536 rounds) the asm reads the
               "item" at address 0 and calls FreeMem(0, ...) - the Amiga
               build does the same through item = 0; the host stops. */
            ORIG_HAZARD("freeall: more rounds than lines");
            return;
        }
#endif
        size = (UBYTE)(lenitem + item->numchars); /* add.b, as freethis */
        next = item->nextitem;
        memalt = size;
        ted_free(item, size);
    } while (count-- != 0);                     /* dbf d2,.loop */
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
    WORD nrows = (WORD)(edrows - row - 1);      /* rows to scroll */
    UBYTE *last;

    if (nrows != 0) {
        UBYTE *d = chmap + (ULONG)row * chcols;
        const UBYTE *s = d + chcols;
        UWORD size = (UWORD)((UWORD)((UWORD)nrows << 6) | ((chcols >> 1) & 63));
        ULONG n = blt_rows(size) * blt_bytes(size);

        if (nrows < 0)  /* row >= edrows: huge blit (survey-05 I.13) */
            ORIG_HAZARD("scrollup: row >= edrows");
        while (n--)
            *d++ = *s++;
    }
    /* .skip ... last row: chmap + (edrows-1)*chcols (add.w to a0) */
    last = chmap + (WORD)(UWORD)((edrows - 1) * chcols);
    blt_fill(last, (UWORD)((chcols >> 1) | 64));
}

/* NEW: chmap part of scrolldown (ted.s:4434) INCLUDING the chmap part of
   eraseline (ted.s:4479), which scrolldown falls into: rows row..edrows-2
   one down (descending from the last word, A = D - chcols), then row
   `row` filled with spaces.  BLTSIZE d6 << 6 | chcols/2 without `and #63`
   copies one row more for even d6 (survey-05 I.4): that extra row lands
   on row `row` (erased right after) and is read from the row before -
   before chmap for row 0 (reproduced, no visible effect; the host build stops
   there, host-test-only guard). */
void chmap_scrolldown(UWORD row)
{
    WORD nrows = (WORD)(edrows - row - 1);      /* rows to scroll */

    if (nrows != 0) {
        UBYTE *d = chmap + (ULONG)chcols * edrows - 1;  /* last byte */
        const UBYTE *s;
        UWORD size = (UWORD)((UWORD)((UWORD)nrows << 6) | (chcols >> 1));
        ULONG n = blt_rows(size) * blt_bytes(size);

        if (nrows < 0)
            ORIG_HAZARD("scrolldown: row >= edrows");
        s = d - chcols;
        while (n--) {
#ifdef TED_HOST
            /* host-test-only guard: the Amiga build reads the bytes in
               front of chmap like the blit (as the original, written to row
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
