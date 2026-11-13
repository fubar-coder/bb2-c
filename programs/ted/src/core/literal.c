/*
 * literal.c - literal characters (NEW): see literal.h.
 */
#include "tedcore.h"
#include "hooks.h"
#include "state.h"
#include "fileio.h"
#include "keymap.h"
#include "literal.h"

/* the room for the marks: the line buffers are 3 * (chcols + 2) bytes
   (outside, ted.s:41), chcols is 128 */
#define LITCOLS 128
#define LITBUFF (3 * (LITCOLS + 2))

/* the built-in names: 128 one-character strings, $80 to $FF */
static const char litnames[128][2] = {
#define L4(c) { (char)(c) }, { (char)((c) + 1) }, { (char)((c) + 2) }, { (char)((c) + 3) }
#define L16(c) L4(c), L4((c) + 4), L4((c) + 8), L4((c) + 12)
    L16(0x80), L16(0x90), L16(0xA0), L16(0xB0),
    L16(0xC0), L16(0xD0), L16(0xE0), L16(0xF0)
#undef L16
#undef L4
};
static_assert(sizeof litnames == 256, "128 names of 2 bytes");

BOOL littrunc;
static UBYTE litbuffmark[LITBUFF];      /* beside charbuff..charbuff4 */
UBYTE litscratch[LITCOLS + 2];
static UBYTE litscratchmark[LITCOLS + 2];
UBYTE *litmap;                          /* beside chmap (literal.h) */
static ULONG litmapsize;

const char *litname(UWORD tok)
{
    return litnames[tok & 0x7F];
}

/* the map byte of the chmap cell p, NULL if p is no cell of chmap */
static UBYTE *mapof(const UBYTE *p)
{
    if (litmap && chmap && p >= chmap && p < chmap + litmapsize)
        return &litmap[p - chmap];
    return NULL;
}

/* the mark of p, NULL if p is in no column row with marks (chmap: mapof) */
static UBYTE *markof(const UBYTE *p)
{
    if (charbuff && p >= charbuff && p < charbuff + LITBUFF)
        return &litbuffmark[p - charbuff];
    if (p >= litscratch && p < litscratch + sizeof(litscratch))
        return &litscratchmark[p - litscratch];
    return NULL;
}

UBYTE litat(const UBYTE *p)
{
    const UBYTE *m = mapof(p);

    if (m)
        return *p == TED_LITGLYPH ? *m : 0;     /* chmap: the marker byte */
    m = markof(p);
    if (!m || !*m)
        return 0;
    return *p == *m ? *m : 0;                   /* a line buffer: cc */
}

void litput(UBYTE *p, UBYTE cc)
{
    UBYTE *m = markof(p);

    *p = cc;
    if (m)
        *m = cc;
}

/* ------------------------------------------------- the literal map */

void litmap_init(UBYTE *map, ULONG size)
{
    ULONG i;

    litmap = map;
    litmapsize = map ? size : 0;
    for (i = 0; i < litmapsize; i++)
        litmap[i] = 0;
}

void litmap_put(const UBYTE *cell, UBYTE cc)
{
    UBYTE *m = mapof(cell);

    if (m)
        *m = cc;
}

/* the map offset of row `row`, or -1 if the row is not in the map (the
   products are mulu.w: no 32-bit multiply on the 68000) */
static LONG map_row(UWORD row)
{
    ULONG at = (ULONG)row * chcols;

    if (!litmap || at > litmapsize || litmapsize - at < chcols)
        return -1;
    return (LONG)at;
}

/* n rows of the map from row `from` to row `to` (one row up or down) */
static void map_rows(UWORD to, UWORD from, UWORD n)
{
    LONG d = map_row(to), s = map_row(from), last = map_row((UWORD)((to > from ? to : from) + n - 1));
    ULONG bytes = (ULONG)n * chcols;

    if (!n || d < 0 || s < 0 || last < 0)
        return;
    if (to < from) {
        UBYTE *dp = litmap + d;
        const UBYTE *sp = litmap + s;

        while (bytes--)
            *dp++ = *sp++;
    } else {
        UBYTE *dp = litmap + d + bytes;
        const UBYTE *sp = litmap + s + bytes;

        while (bytes--)
            *--dp = *--sp;
    }
}

void litmap_eraseline(UWORD row)
{
    LONG at = map_row(row);
    UWORD i;

    if (at < 0)
        return;
    for (i = 0; i < chcols; i++)
        litmap[at + i] = 0;
}

void litmap_scrollup(UWORD row)
{
    if (row < edrows)
        map_rows(row, (UWORD)(row + 1), (UWORD)(edrows - row - 1));
    if (edrows)
        litmap_eraseline((UWORD)(edrows - 1));
}

void litmap_scrolldown(UWORD row)
{
    if (row < edrows)
        map_rows((UWORD)(row + 1), row, (UWORD)(edrows - row - 1));
    litmap_eraseline(row);
}

void litmap_cls(void)
{
    UWORD r;

    for (r = 0; r < edrows; r++)
        litmap_eraseline(r);
}

UBYTE cellglyph(const UBYTE *cell, BOOL *tok)
{
    UBYTE cc = litat(cell);

    if (cc) {
        *tok = FALSE;
        return cc;
    }
    *tok = (*cell & 0x80) != 0;
    return (UBYTE)(*cell & 0x7F);
}

UBYTE litnext(const UBYTE **src, BOOL pairs)
{
    UBYTE c = *(*src)++;

    if (pairs && TED_ISLIT(c, **src))
        c = *(*src)++;
    return c;
}

void litstore(UBYTE **p, WORD c)
{
    if (c & TED_BLIT)
        litput(*p, (UBYTE)c);
    else
        **p = (UBYTE)c;
    (*p)++;
}

void litclear(void)
{
    UWORD i;

    for (i = 0; i < LITBUFF; i++)
        litbuffmark[i] = 0;
    for (i = 0; i < sizeof(litscratchmark); i++)
        litscratchmark[i] = 0;
}

/* ------------------------------------------------- the cursor row */

void litrow_sync(void)
{
    const struct item *line = thisitem;
    const UBYTE *src, *row;
    UWORD col = 0, i;
    UBYTE c;

    if (!chmap)
        return;
    row = chmap + (WORD)chloc;
    for (i = 0; i < chcols; i++)
        litmap_put(row + i, 0);
    if (!line)
        return;
    /* the columns as chmap_showa2item gives them (fileio.c) */
    src = line->chars;
    c = *src++;
    if (!c)
        return;
    if (c == label) {
        col++;                                  /* no token at column 0 */
        c = *src++;
    }
    while (c && col < chcols && col < LITCOLS) {
        if (TED_ISLIT(c, *src)) {
            litmap_put(row + col, *src++);
            col++;
        } else if (tokenise && (c & 0x80)) {
            const char *name = findtoke((UWORD)((c << 8) | *src++));

            while (*name++)
                col++;
        } else {
            col++;
        }
        c = *src++;
    }
}

/* the cursor row's part of the map, NULL without one */
static UBYTE *maprow(void)
{
    LONG at = (WORD)chloc;

    if (!litmap || !chmap || at < 0 || (ULONG)at + chcols > litmapsize)
        return NULL;
    return litmap + at;
}

void litrow_shift(UWORD from, WORD n)
{
    UBYTE *m = maprow();
    WORD i, last = (WORD)chcols - 1;

    if (!m)
        return;
    if (n > 0) {
        for (i = last; i >= (WORD)from + n; i--)
            m[i] = m[i - n];
        for (i = (WORD)from; i < (WORD)from + n && i <= last; i++)
            m[i] = 0;
    } else if (n < 0) {
        for (i = (WORD)from; i <= last; i++)
            m[i] = i - n <= last ? m[i - n] : 0;
    }
}

void litrow_set(UWORD col, UBYTE cc)
{
    UBYTE *m = maprow();

    if (m && col < chcols)
        m[col] = cc;
}

/* ------------------------------------------------- the scratch row */

void litrow_copy(const UBYTE *row, UWORD n)
{
    UWORD i;

    for (i = 0; i < n && i < sizeof(litscratch); i++) {
        UBYTE l = litat(row + i);

        litscratch[i] = l ? l : row[i];
        litscratchmark[i] = l;
    }
}

UWORD litrow_expand(const UBYTE *line)
{
    UWORD i = 0;

    for (;;) {
        UBYTE c = *line++;

        if (i >= sizeof(litscratch) - 1)
            c = 0;                              /* (never: a line has 129 bytes) */
        if (TED_ISLIT(c, *line)) {
            c = *line++;
            litscratch[i] = c;
            litscratchmark[i++] = c;
            continue;
        }
        litscratch[i] = c;
        litscratchmark[i] = 0;
        if (c == 0)
            return i;
        i++;
    }
}

/* ------------------------------------------------- text */

UBYTE unmapgerms(UBYTE c)
{
    UWORD i;

    for (i = 0; i < ngerms; i++)
        if (germs[2 * i + 1] == c)
            return germs[2 * i];
    return c;
}

BOOL haslit(const UBYTE *line)
{
    for (;;) {
        UBYTE c = *line++;

        if (c == 0)
            return FALSE;
        if (TED_ISLIT(c, *line))
            return TRUE;
        if ((c & 0x80) && *line++ == 0)
            return FALSE;
    }
}

UBYTE *detolit(const UBYTE *src, UBYTE *dst, const UBYTE *end)
{
    const UBYTE *last = end - 1;                /* room for the 0 */

    for (;;) {
        UBYTE c = *src++;

        if (c == 0 || dst >= last)
            break;
        if (TED_ISLIT(c, *src)) {
            litput(dst++, *src++);
            continue;
        }
        *dst++ = c;
    }
    *dst = 0;
    return dst;
}

UBYTE *detokpairs(const UBYTE *src, UBYTE *dst, const UBYTE *end)
{
    const UBYTE *last = end - 1;                /* room for the 0 */

    for (;;) {
        UBYTE c = *src++;
        const char *name;

        if (c == 0 || dst >= last)
            break;
        if (!(c & 0x80)) {
            *dst++ = c;
            continue;
        }
        if (TED_ISLIT(c, *src)) {
            if (dst + 2 > last)
                break;
            *dst++ = c;
            *dst++ = *src++;
            continue;
        }
        name = findtoke((UWORD)((c << 8) | *src++));
        while (*name && dst < last)
            *dst++ = (UBYTE)*name++;
    }
    *dst = 0;
    return dst;
}

UBYTE *textline(const UBYTE *src, UBYTE *dst, const UBYTE *end)
{
    for (;;) {
        UBYTE c = *src++;

        if (c == 0)
            break;
        if (c & 0x80) {
            const char *name;
            UBYTE lo = *src++;

            if (TED_ISLIT(c, lo)) {
                if (dst >= end)
                    break;
                *dst++ = lo;
                continue;
            }
            name = findtoke((UWORD)((c << 8) | lo));
            while (*name && dst < end)
                *dst++ = (UBYTE)*name++;
            if (lo == 0)
                break;                          /* (a half pair: never stored) */
            continue;
        }
        c = unmapgerms(c);
        if (dst >= end)
            break;
        *dst++ = c;
    }
    return dst;
}
