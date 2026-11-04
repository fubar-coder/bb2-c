/*
 * ab3str.c - the escape sequences of string literals in AB3 mode
 * (blitz2-ab3; extension point ext_strescape of ext.h, spec
 * ab3-compat/strings, design D1-D3 of the change ab3-mode-g3).
 *
 * With new syntax on (OPTIMIZE bit 4), "\\" in a string literal of a
 * run-time expression starts an escape sequence (AmiBlitz3.ab3:16210
 * litstring, 26915 check_newconstants).  AmiBlitz3's rules are followed,
 * its bugs implemented as intended (its Compiler-Spec 04-lexical.md
 * section 4.5.2, issues I-12 to I-15): an unknown form is an error, a name
 * matches only in full, \\Rddc repeats c exactly dd times, unchanged.
 * Data strings take no escapes (dodata does not call litstring).  Dc.b
 * strings take the date and version names in AB3 mode, also without new
 * syntax (ext_dcbescape).
 */
#include <proto/dos.h>
#include "blitz2.h"
#include "ab3.h"
#include "ab3date.h"

/* the kinds of the names after "\\" */
enum esckind {
    ESC_DATE_US,                        /* __DATE__      mm.dd.yyyy */
    ESC_DATE_GER,                       /* __DATE_GER__  dd.mm.yyyy */
    ESC_VER_MAJOR,                      /* __VER_*       the header's Version */
    ESC_VER_MINOR,
    ESC_VER_PATCH,
    ESC_VER_BUILD,                      /*               and NumberOfBuilds */
    ESC_THIS_FUNCTION,                  /* __THIS_FUNCTION */
    ESC_THIS_INCLUDE,                   /* __THIS_INCLUDE */
    ESC_CODELINE,                       /* __CODELINE */
    ESC_THIS_POINTER                    /* __THIS_POINTER: refused */
};

/* the names, upper case; compared case-insensitively, the longest one
   that matches in full wins (design D2) */
static const struct escname {
    const char *name;
    UBYTE kind;
} escnames[] = {
    { "__DATE__", ESC_DATE_US },
    { "__DATE_GER__", ESC_DATE_GER },
    { "__VER_MAJOR", ESC_VER_MAJOR },
    { "__VER_MINOR", ESC_VER_MINOR },
    { "__VER_PATCH", ESC_VER_PATCH },
    { "__VER_BUILD", ESC_VER_BUILD },
    { "__THIS_FUNCTION", ESC_THIS_FUNCTION },
    { "__THIS_INCLUDE", ESC_THIS_INCLUDE },
    { "__CODELINE", ESC_CODELINE },
    { "__THIS_POINTER", ESC_THIS_POINTER },
};

static NORETURN void unknownerr(void)
{
    err("Unknown Escape Sequence");
}

static UWORD upper(UWORD c)
{
    return c >= 'a' && c <= 'z' ? c - 'a' + 'A' : c;
}

static int hexdigit(UWORD c)
{
    c = upper(c);
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

static BOOL decdigit(UWORD c)
{
    return c >= '0' && c <= '9';
}

/* n characters of the source, matched by peeking at srcptr, are read
   (ASCII: one byte each) */
static void skip(UWORD n)
{
    while (n--)
        getchar3();
}

/* a byte of the string: of a string literal (its length *len), or of a
   Dc.b string (len NULL, into the code) */
static void put(UBYTE b, UWORD *len)
{
    if (!len) {
        pokebyte(b);
        return;
    }
    pokedata1b(b);
    (*len)++;
}

static void putstr(const char *s, UWORD *len)
{
    while (*s)
        put((UBYTE)*s++, len);
}

static void putnum(ULONG v, UWORD *len)
{
    char buf[12], *p = buf + sizeof buf;

    *--p = 0;
    do
        *--p = (char)('0' + v % 10);
    while (v /= 10);
    putstr(p, len);
}

/* the name at p (behind "\\"): the longest of escnames matching in full,
   NULL if none */
static const struct escname *findname(const UBYTE *p, UWORD *n)
{
    const struct escname *e, *best = NULL;
    UWORD i, bestn = 0;

    for (e = escnames; e < escnames + sizeof escnames / sizeof *escnames; e++) {
        for (i = 0; e->name[i] && upper(p[i]) == (UBYTE)e->name[i]; i++)
            ;
        if (!e->name[i] && i > bestn) {
            best = e;
            bestn = i;
        }
    }
    *n = bestn;
    return best;
}

/* ------------- the line counter of \\__CODELINE (design D5, ab3-mode-g3) */

/* the lines of the program read in this pass: main source and includes in
   compile order, from 1 (AmiBlitz3's globallinecount, AmiBlitz3.ab3:2697,
   reset per pass at 27227).  The empty lines ab3cli made of an embedded
   header are not counted: AmiBlitz3 has no such lines (it removes the
   header of the main source and of a text include). */
static ULONG codeline;

void ab3_strpass(void)
{
    codeline = 0;
}

void ab3_strline(void)
{
    if (numincs ? ab3_incheaderline() : (UWORD)linenumat < ab3hdrlines)
        return;
    codeline++;
}

/* the date of the compile, dos.library's DateStamp, as AmiBlitz3 gives
   it (design D6): mm.dd.yyyy, ger dd.mm.yyyy */
static void putdate(int ger, UWORD *len)
{
    struct DateStamp ds;
    char buf[AB3DATE_TEXTLEN];

    DateStamp(&ds);
    ab3_datetext((uint32_t)ds.ds_Days, ger, buf);
    putstr(buf, len);
}

/* the value of the name e */
static void putname(const struct escname *e, UWORD *len)
{
    switch (e->kind) {
    case ESC_DATE_US:
    case ESC_DATE_GER:
        putdate(e->kind == ESC_DATE_GER, len);
        return;
    case ESC_VER_MAJOR:
    case ESC_VER_MINOR:
    case ESC_VER_PATCH:
        putnum(ab3version[e->kind - ESC_VER_MAJOR], len);
        return;
    case ESC_VER_BUILD:
        putnum(ab3builds, len);
        return;
    case ESC_THIS_FUNCTION:
        /* the procedure being compiled - in a macro the one it is
           expanded in - as written in its definition */
        putstr(thisproc ? thisproc->name : "GLOBAL", len);
        return;
    case ESC_THIS_INCLUDE:
        /* the include whose line is read, "" in the main source (AmiBlitz3:
           the include opened last, I-59 - taken as intended) */
        putstr(ab3_incfile(), len);
        return;
    case ESC_CODELINE:
        putnum(codeline, len);
        return;
    case ESC_THIS_POINTER:
        /* AmiBlitz3 inserts an address of its own compile-time memory */
        err("Not supported: \\\\__THIS_POINTER");
    }
}

/* litstring (eval.c) read a '\' of a string literal */
BOOL ext_strescape(UWORD *len)
{
    const UBYTE *p;
    const struct escname *e;
    UWORD c, n, i;
    int hi, lo;

    if (progmode != XTRA_MODE_AB3 || !ext_newsyntax() || *srcptr != '\\')
        return FALSE;
    getchar3();                         /* the second '\' */
    p = srcptr;
    c = upper(p[0]);
    if (p[0] == '\\' && p[1] == '\\') {
        /* \\\\: the two characters \\ */
        skip(2);
        put('\\', len);
        put('\\', len);
    } else if (c == 'N') {
        /* \\N: a line feed (before the hex digits, as \\R) */
        skip(1);
        put(10, len);
    } else if (c == 'R' && decdigit(p[1]) && decdigit(p[2]) && p[3]
               && p[3] != '"') {
        /* \\Rddc: c dd times, dd decimal */
        n = (UWORD)((p[1] - '0') * 10 + p[2] - '0');
        skip(3);
        c = getchar3();
        for (i = 0; i < n; i++)
            put((UBYTE)c, len);
    } else if ((hi = hexdigit(p[0])) >= 0 && (lo = hexdigit(p[1])) >= 0) {
        /* \\hh: one byte */
        skip(2);
        put((UBYTE)(hi << 4 | lo), len);
    } else if ((e = findname(p, &n))) {
        skip(n);
        putname(e, len);
    } else
        unknownerr();
    return TRUE;
}

/* ----------------------------------- Dc.b strings (ab3-mode-g3 task 3.1) */

/* dodc (tokens1.c) read a '\' of a Dc.b string: in AB3 mode, with or
   without new syntax, "\\" followed by a date or version name gives its
   value as in a string literal (AmiBlitz3.ab3:20285, compiledatedc,
   version_string_dc); every other '\' is written as it is.  The names as
   in string literals, case-insensitive and only in full (AmiBlitz3 compares
   upper case prefixes, I-15 - taken as intended). */
BOOL ext_dcbescape(void)
{
    const struct escname *e;
    UWORD n;

    if (progmode != XTRA_MODE_AB3 || *srcptr != '\\'
        || !(e = findname(srcptr + 1, &n))
        || e->kind > ESC_VER_BUILD)     /* only the date and version kinds */
        return FALSE;
    skip((UWORD)(n + 1));               /* the second '\' and the name */
    putname(e, NULL);
    return TRUE;
}
