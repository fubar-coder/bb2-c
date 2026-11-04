/*
 * ab3syn.c - the syntax rules of AB3 mode (blitz2-ab3; extension points
 * of ext.h, spec ab3-compat/syntax-rules of the change ab3-mode-g2): the
 * "new syntax" of OPTIMIZE bit 4 (ext_newsyntax, ab3dir.c) and the rules
 * AmiBlitz3 v3.14.0 has on in every program.  In BB2 mode none applies.
 */
#include <string.h>
#include "blitz2.h"
#include "ab3.h"

/* the variables of global SHARED statements (ext_shared): shared with
   every procedure; the variables are freed between passes */
#define MAXSHARED 256
static struct var *gshared[MAXSHARED];
static UWORD ngshared;

static BOOL cnifdef;

/* the lines read belong to a Macro definition (ext_macrodef) */
static BOOL inmacrodef;

void ab3_synpass(void)
{
    ngshared = 0;
    cnifdef = FALSE;
    inmacrodef = FALSE;     /* an error stopped a definition */
}

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

/* .name followed by a token is a label (AmiBlitz3.ab3:4284-4291
   handlemouse), the token the next statement */
void ext_dotlabel(UWORD c)
{
    if (progmode == XTRA_MODE_AB3 && (c & 0x8000))
        labelend();
}

/* '+' at the start of a statement: "no type check" for it
   (AmiBlitz3.ab3:4183, notypecheck); type checks are G8, so it is
   skipped */
UWORD ext_stmtprefix(UWORD c)
{
    if (progmode == XTRA_MODE_AB3 && c == '+')
        return getchar3();
    return c;
}

/* a Function called as a statement (AmiBlitz3.ab3:4416-4426
   handle_statement: the check of the return type is commented out,
   ";BPL.w illprocerr"): compiled as a Statement's call, the result
   discarded.  A string result is left in the string buffer, as
   AmiBlitz3's: no string base is set up for it (blitz2's afunction does
   for an expression), the Function's own string code sets it. */
BOOL ext_funcstmt(const struct proc *p)
{
    (void)p;
    return progmode == XTRA_MODE_AB3;
}

/* code after ":@" in a comment is compiled (AmiBlitz3.ab3:10593 getchar):
   the comment ends there as a ':' */
BOOL ext_comment(void)
{
    const UBYTE *p;

    if (progmode != XTRA_MODE_AB3)
        return FALSE;
    for (p = srcptr; *p; p++)
        if (p[0] == ':' && p[1] == '@') {
            srcptr = (UBYTE *)p + 2;
            return TRUE;
        }
    return FALSE;
}

/* AmiBlitz3's test_for_letter (AmiBlitz3.ab3:936-947, getelement 15139):
   every character from 'A' to 'z' starts a variable, so also
   [ \ ] ^ _ ` */
BOOL ext_letter(UWORD c)
{
    return progmode == XTRA_MODE_AB3 && c >= '[' && c <= '`';
}

/* CNIF @ (AmiBlitz3.ab3:10677-10691, flag incnif): an '@' right behind
   CNIF makes every #name of the condition a "defined?" test
   (fetch_Constant 15400-15412: findlabel2, 1 if found, else 0, never
   "Constant Not Found").  An '@' elsewhere in a condition is blitz2's
   "Illegal Constant Expression" (getelement2 in a constant), as in
   AmiBlitz3.  cnifdef: in such a condition. */

UWORD ext_cnifstart(UWORD c)
{
    if (progmode != XTRA_MODE_AB3 || c != '@')
        return c;
    cnifdef = TRUE;
    return getchar3();
}

void ext_cnifend(void)
{
    cnifdef = FALSE;
}

/* AmiBlitz3's constants of the include mechanism (AmiBlitz3.ab3:3405-3440,
   processInclude 20799, next_includefile 19534): #__include is 1 while an
   include is open, else 0.  #__debug (AmiBlitz3's debugmode) is the
   core's, in both modes (eval.c debugconst).  AmiBlitz3 has both in its
   constant list, so a "defined?" test finds them. */
BOOL ext_constant(UWORD len, ULONG *value)
{
    BOOL inc, dbg;

    if (progmode != XTRA_MODE_AB3)
        return FALSE;
    inc = len == 9 && !memcmp(namebuff, "__include", 9);
    dbg = len == 7 && !memcmp(namebuff, "__debug", 7);
    if (cnifdef)
        *value = inc || dbg || findadd(len, NULL) != NULL;
    else if (inc)
        *value = ab3_incdepth() != 0;
    else
        return FALSE;
    return TRUE;
}

/* SHARED outside a procedure (AmiBlitz3.ab3:20505 do_shared,
   proceduremode 0): the global variables named are shared with every
   procedure compiled after it (ext_sharedvar) */
BOOL ext_shared(UWORD c)
{
    struct type *t;
    struct var *v;
    UWORD len;

    if (progmode != XTRA_MODE_AB3)
        return FALSE;
    for (;;) {
        c = getvname(c, &len);
        v = fetchvar(len, &t, &c);
        if (ngshared == MAXSHARED)
            err("Too many global SHARED variables");
        gshared[ngshared++] = v;
        if (flagmask & 2) {
            c = getchar3();
            if (c != ')')
                syntaxerr();
            c = getchar3();
        }
        if (c != ',')
            break;
        c = getchar3();
    }
    return TRUE;
}

/* in a procedure: a global SHARED variable of the name and kind, as
   AmiBlitz3's fvaradd looks in sharebase before it adds a local one */
struct var *ext_sharedvar(UWORD len)
{
    UWORD i;
    struct var *v;

    for (i = 0; i < ngshared; i++) {
        v = gshared[i];
        if ((UBYTE)v->flags == (UBYTE)flagmask && v->len - 16 == len
            && !memcmp(v->name, namebuff, len)) {
            varmode = 0;
            return v;
        }
    }
    return NULL;
}

/* ------------------------------------------------- joining lines: @@ */

/* AmiBlitz3 joins the next line at "@@" in a parameter list, a macro and
   a multiple assignment (AmiBlitz3.ab3:8116, 8283, 8299 fetchpees, 3110,
   3222 macros, 20885 do_Macro, 23658 do_let), the rest of the line after
   it ignored.  Here a line with "@@" outside strings and comments is
   joined with the following ones when it is read, so every place of the
   compiler sees one line. */
#define JOINLEN 2048
static UBYTE joinbuf[JOINLEN];
static BOOL joining;
/* the lines of the joined line in joinbuf (\\__CODELINE): where each one
   starts in joinbuf, their number, the bytes in joinbuf */
#define JOINLINES 512
static UWORD joinat[JOINLINES];
static UWORD joinlines, joinlen;

/* the "@@" of the line at p outside strings and comments, NULL if none */
static const UBYTE *findjoin(const UBYTE *p)
{
    BOOL instr = FALSE;

    for (; *p; p++) {
        if (*p & 0x80) {
            if (!*++p)
                break;
            continue;
        }
        if (*p == '"')
            instr = !instr;
        else if (!instr && *p == ';')
            break;
        else if (!instr && p[0] == '@' && p[1] == '@')
            return p;
    }
    return NULL;
}

/* does the line at p start with Macro (a definition, not End Macro)? */
static BOOL ismacroline(const UBYTE *p)
{
    while (*p == ' ')
        p++;
    return p[0] == 0x80 && p[1] == 0x0d;        /* Macro ($800D) */
}

void ext_nextline(void)
{
    const UBYTE *p, *at, *b;
    ULONG n = 0;
    BOOL incrule;

    /* \\__CODELINE (ab3str.c): every line read, also each line joined
       here (AmiBlitz3 counts a joined line once - an intended deviation:
       the number finds the line in the source) */
    ab3_strline();
    if (progmode != XTRA_MODE_AB3 || joining || !(at = findjoin(srcptr)))
        return;
    joining = TRUE;
    joinlines = 0;
    joinlen = 0;
    p = srcptr;
    /* inside an include only a ',' (and spaces) before the "@@": the
       join of AmiBlitz3's mergeline (8299), the others stop there
       (errorjoin, 3113, 3224, 8117, 8286); not in a Macro definition
       (ext_macrodef), which joins every form */
    incrule = numincs && !inmacrodef && !ismacroline(p);
    do {
        if (incrule) {
            for (b = at; b > p && b[-1] == ' '; b--)
                ;
            if (b == p || b[-1] != ',') {
                joining = FALSE;
                err("Joining lines with @@ does not work in includes");
            }
        }
        if (n + (ULONG)(at - p) >= JOINLEN || joinlines == JOINLINES - 1) {
            joining = FALSE;
            err("Joined line too long");
        }
        joinat[joinlines++] = (UWORD)n;
        memcpy(joinbuf + n, p, (size_t)(at - p));
        n += (ULONG)(at - p);
        if (!nextline()) {
            joining = FALSE;
            err("No line to join with @@");
        }
        p = srcptr;
    } while ((at = findjoin(p)));
    at = p + strlen((const char *)p) + 1;
    if (n + (ULONG)(at - p) > JOINLEN) {
        joining = FALSE;
        err("Joined line too long");
    }
    joinat[joinlines++] = (UWORD)n;
    memcpy(joinbuf + n, p, (size_t)(at - p));
    joinlen = (UWORD)(n + (ULONG)(at - p));
    joining = FALSE;
    srcptr = joinbuf;
}

/* AmiBlitz3's do_Macro gathers the rest of the Macro line and the lines
   of the definition up to End Macro, joining at "@@" without the include
   check of the parameter lists (AmiBlitz3.ab3:20885, 20986-20991) */
void ext_macrodef(BOOL on)
{
    inmacrodef = on;
}

ULONG ab3_joinback(const UBYTE *p)
{
    UWORD off, k;

    if (p < joinbuf || p >= joinbuf + joinlen)
        return 0;
    off = (UWORD)(p - joinbuf);
    /* the last line starting at or before p (a line that added no bytes
       starts where the next one does: the next one has p) */
    for (k = joinlines; k > 1 && joinat[k - 1] > off; k--)
        ;
    return (ULONG)(joinlines - k);
}
