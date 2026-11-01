/*
 * ab3syn.c - the syntax rules of AB3 mode (blitz2-ab3; extension points
 * of ext.h, spec ab3-compat/syntax-rules of the change ab3-mode-g2): the
 * "new syntax" of OPTIMIZE bit 4 (ext_newsyntax, ab3dir.c) and the rules
 * AmiBlitz3 v3.14.0 has on in every program.  In BB2 mode none applies.
 */
#include "blitz2.h"
#include "ab3.h"

/* AmiBlitz3's chartab (AmiBlitz3.ab3:1161-1191): the characters that end
   a name; the bytes from 128 are tokens */
static BOOL isterm(UBYTE c)
{
    static const char term[] = ",;:()=+-*/$%.[]{}\\><&|^";
    const char *t;

    if (c <= 2 || c >= 127)
        return TRUE;
    for (t = term; *t; t++)
        if ((UBYTE)*t == c)
            return TRUE;
    return FALSE;
}

/* AmiBlitz3's findit (AmiBlitz3.ab3:3186): the end of the parameter at
   *pp of a macro call.  As blitz2's (compile.c findit) up to a ',' or '}'
   outside brackets, but the end of the line, a ':' or a ';' outside a
   string end the parameter (and the call) too: *pp is then on it, the
   result 0.  The length of the parameter goes to *len. */
static UWORD findit3(UBYTE **pp, UWORD *len)
{
    UBYTE *p = *pp, *start = p;
    BOOL instr = FALSE;
    WORD nest = 0;
    UBYTE c;

    for (;;) {
        c = *p++;
        if (!c)
            break;
        if (c & 0x80) {
            p++;
            continue;
        }
        if (!instr && (c == ';' || c == ':'))
            break;
        if (c == '"') {
            instr = !instr;
            continue;
        }
        if (instr)
            continue;
        if (c == ',' || c == '}') {
            if (nest == 0) {
                *len = (UWORD)(p - start - 1);
                *pp = p;
                return c;
            }
            if (c == '}')
                nest--;
            continue;
        }
        if (c == '{' || c == '(')
            nest++;
        else if (c == ')')
            nest--;
    }
    p--;                                /* on the end */
    *len = (UWORD)(p - start);
    *pp = p;
    return 0;
}

/* New syntax: the parameters of the call !name of the macro m at *pp
   (after the name and its spaces) without braces, as AmiBlitz3's
   handle_macro (AmiBlitz3.ab3:2853-2930).  A macro whose text has no `
   takes none; else the first character decides: name characters, '#',
   '$', '-' and '{' (skipped) start the parameters, the other ends of a
   name ('(', ':', ';', the end of the line, ...) mean none; a ',' right
   after the name means none.  They go to macrobuff as blitz2's; *pp is
   behind them. */
BOOL ext_macargs(struct macro *m, UBYTE **pp, WORD *narg)
{
    struct macparam *mp = (struct macparam *)macrobuff;
    UBYTE *p = *pp, c;
    UWORD i, t;

    if (!ext_newsyntax())
        return FALSE;
    *narg = 0;
    if (*p == ',')
        return TRUE;
    for (i = 0; i < m->textlen && m->text[i] != '`'; i++)
        ;
    if (i == m->textlen)
        return TRUE;
    while (*p == ' ')
        p++;
    c = *p;
    if (!(c & 0x80) && isterm(c)) {
        if (c == '{')
            p++;
        else if (c != '#' && c != '$' && c != '-' && c != ',')
            return TRUE;
    }
    if (!*p)
        return TRUE;
    do {
        mp->start = p;
        t = findit3(&p, &mp->len);
        mp++;
        ++*narg;
    } while (t == ',');
    *pp = p;
    return TRUE;
}

/* the end of a label at the start of a statement before the token c: the
   source back at the token, as if a ':' had come (AmiBlitz3.ab3:4219,
   4289: subq.l #2,a5, lastchar ':') */
static void labelend(void)
{
    srcptr -= 2;
    lastchar = ':';
}

/* New syntax: a name at the start of a statement followed by a token is
   a label without colon (AmiBlitz3.ab3:4214-4225), unless the low byte of
   the token is one of allowtab (1198-1215): LSL, LSR, ASL, ASR, MOD, NOT,
   BitTst, BitSet, BitClr, BitChg - the BB2 shortcut "a LSL 2" */
BOOL ext_namelabel(UWORD c)
{
    static const UBYTE allowtab[] = {
        0x7d, 0x7c, 0x44, 0x45, 0xb9, 0x89, 0xce, 0xcf, 0xd0, 0xcd
    };
    unsigned i;

    if (!ext_newsyntax() || !(c & 0x8000))
        return FALSE;
    for (i = 0; i < sizeof allowtab; i++)
        if ((UBYTE)c == allowtab[i])
            return FALSE;
    labelend();
    return TRUE;
}
