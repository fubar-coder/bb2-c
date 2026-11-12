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
static UBYTE litrowmark[LITCOLS];       /* beside the cursor row of chmap */
static const UBYTE *litrowat;           /* the row litrowmark belongs to */

const char *litname(UWORD tok)
{
    return litnames[tok & 0x7F];
}

/* the mark of p, NULL if p is in no column row with marks */
static UBYTE *markof(const UBYTE *p)
{
    if (charbuff && p >= charbuff && p < charbuff + LITBUFF)
        return &litbuffmark[p - charbuff];
    if (p >= litscratch && p < litscratch + sizeof(litscratch))
        return &litscratchmark[p - litscratch];
    if (litrowat && p >= litrowat && p < litrowat + LITCOLS && p < litrowat + chcols)
        return &litrowmark[p - litrowat];
    return NULL;
}

UBYTE litat(const UBYTE *p)
{
    const UBYTE *m = markof(p);

    if (!m || !*m)
        return 0;
    if (m >= litrowmark && m < litrowmark + LITCOLS)
        return *p == TED_LITGLYPH ? *m : 0;     /* chmap: the glyph */
    return *p == *m ? *m : 0;                   /* a line buffer: cc */
}

void litput(UBYTE *p, UBYTE cc)
{
    UBYTE *m = markof(p);

    *p = cc;
    if (m && !(m >= litrowmark && m < litrowmark + LITCOLS))
        *m = cc;
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
    const UBYTE *src;
    UWORD col = 0, i;
    UBYTE c;

    for (i = 0; i < LITCOLS; i++)
        litrowmark[i] = 0;
    litrowat = chmap ? chmap + (WORD)chloc : NULL;
    if (!line || !litrowat)
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
            litrowmark[col] = *src++;
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

void litrow_shift(UWORD from, WORD n)
{
    WORD i, last = (WORD)(chcols < LITCOLS ? chcols : LITCOLS) - 1;

    if (n > 0) {
        for (i = last; i >= (WORD)from + n; i--)
            litrowmark[i] = litrowmark[i - n];
        for (i = (WORD)from; i < (WORD)from + n && i <= last; i++)
            litrowmark[i] = 0;
    } else if (n < 0) {
        for (i = (WORD)from; i <= last; i++)
            litrowmark[i] = i - n <= last ? litrowmark[i - n] : 0;
    }
}

void litrow_set(UWORD col, UBYTE cc)
{
    if (col < LITCOLS)
        litrowmark[col] = cc;
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
