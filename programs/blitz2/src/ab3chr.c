/*
 * ab3chr.c - the character literals @"..." of AB3 mode (blitz2-ab3;
 * extension point ext_element of ext.h, spec ab3-compat/strings,
 * design D1/D4 of the change ab3-mode-g3).
 *
 * With new syntax on (OPTIMIZE bit 4), @ directly followed by a string in
 * a run-time expression is an integer of the character codes, the first
 * one highest (AmiBlitz3.ab3:15149, in getelement2 behind '?').  The value
 * is built arithmetically (value << 8 | c, as fetchqasm), so it does not
 * depend on the byte order of the compiling machine; on the 68000 it is in
 * memory the characters in their written order.  It is then passed on
 * like a numeric constant (fetchnum2): a float target gets the long
 * converted.  AmiBlitz3 writes move.l #value,d0 for every target, also a
 * float one; that is taken as intended.
 *
 * In a constant expression getelement2 has stopped before (constmode:
 * syntaxerr, cfetchmode: badconerr), as AmiBlitz3 does.
 */
#include "blitz2.h"
#include "ab3.h"

/* getelement2 (eval.c) has no element for c */
BOOL ext_element(UWORD c, UWORD *type, UWORD *ret)
{
    ULONG value = 0;
    UWORD max, n = 0;

    if (c != '@' || *srcptr != '"' || progmode != XTRA_MODE_AB3 || !ext_newsyntax())
        return FALSE;
    /* the most characters of the target, AmiBlitz3's: a quick target is
       refused before the characters are read */
    switch (*type) {
    case T_BYTE:
        max = 1;
        break;
    case T_WORD:
        max = 2;
        break;
    case T_QUICK:
    case T_STRING:
        noconerr();
    default:
        max = 4;            /* long, float, not decided yet */
    }
    getchar3();                         /* the opening quote */
    for (;;) {
        c = getchar3();
        if (c == '"')
            break;
        if (c == 0)
            syntaxerr();
        if (LITCHAR(c)) {
            /* a literal character (the loader's form of a byte above $7F
               in a string, spec ab3-compat/strings): its one byte */
            n++;
            value = value << 8 | (UBYTE)c;
        } else if (c & 0x8000) {
            /* a byte above $7F: getcharb has read the next one with it
               (as a token) */
            n += 2;
            value = value << 16 | c;
        } else {
            n++;
            value = value << 8 | (UBYTE)c;
        }
        if (n > max)
            syntaxerr();
    }
    getchar3();                         /* behind the closing quote */
    *ret = (UWORD)fetchnum2(value, type, FALSE);
    return TRUE;
}
