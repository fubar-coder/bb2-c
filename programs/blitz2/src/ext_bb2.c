/*
 * ext_bb2.c - blitz2's implementation of the extension points (ext.h):
 * BB2 mode only.  No logic: every point handles nothing, so the chunk
 * files do what blitz2.s does; the data are BB2's.  blitz2-ab3 links the
 * AB3 modules (ab3*.c) instead.
 */
#include "blitz2.h"

struct token *ext_toklist = (struct token *)&tokimg;
UWORD ext_mathq = 247-55;

BOOL ext_makelibs(BOOL *ok)
{
    return FALSE;
}

BOOL ext_loadlibs(struct node **tail)
{
    return FALSE;
}

BOOL ext_loaduserlibs(struct node *tail)
{
    return FALSE;
}

void ext_freelibs(void)
{
}

void ext_resbegin(void)
{
}

BOOL ext_loadres(struct resentry *res)
{
    return FALSE;
}

void ext_compilestart(void)
{
}

void ext_incopen(struct include *inc)
{
}

BOOL ext_readinc(struct include *inc, UBYTE *dst, LONG len, LONG *got)
{
    return FALSE;
}

void ext_incline(struct include *inc)
{
}

void ext_incdone(struct include *inc)
{
}

void ext_errpos(void)
{
}

void ext_newtoken(UWORD tok)
{
}

void ext_passstart(void)
{
}

BOOL ext_token(UWORD c)
{
    return FALSE;
}

BOOL ext_newsyntax(void)
{
    return FALSE;
}

BOOL ext_macargs(struct macro *m, UBYTE **pp, WORD *narg)
{
    return FALSE;
}

BOOL ext_namelabel(UWORD c)
{
    return FALSE;
}

void ext_dotlabel(UWORD c)
{
}

UWORD ext_stmtprefix(UWORD c)
{
    return c;
}

BOOL ext_comment(void)
{
    return FALSE;
}

BOOL ext_letter(UWORD c)
{
    return FALSE;
}

BOOL ext_shared(UWORD c)
{
    return FALSE;
}

struct var *ext_sharedvar(UWORD len)
{
    return NULL;
}

BOOL ext_cmdline(int argc, char **argv, int *rc)
{
    return FALSE;
}

const char *ext_editor(const char *name)
{
    return name;
}

APTR ext_editorarg(APTR arg)
{
    return arg;
}

BOOL ext_result(int *rc)
{
    return FALSE;
}
