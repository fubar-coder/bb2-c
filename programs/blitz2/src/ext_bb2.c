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

BOOL ext_incdevice(void)
{
    return TRUE;
}

UWORD ext_incname(UWORD len)
{
    return len;
}

BOOL ext_xinclude(UWORD len)
{
    return FALSE;
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

BOOL ext_funcstmt(const struct proc *p)
{
    return FALSE;
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

UWORD ext_cnifstart(UWORD c)
{
    return c;
}

void ext_cnifend(void)
{
}

BOOL ext_constant(UWORD len, ULONG *value)
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

void ext_nextline(void)
{
}

void ext_passend(void)
{
}

BOOL ext_savefile(void)
{
    return FALSE;
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

/* AB3 mode's strings (change ab3-mode-g3) */

BOOL ext_strescape(UWORD *len)
{
    return FALSE;
}

/* Dc.b strings (ab3-mode-g3 task 3.1) */

BOOL ext_dcbescape(void)
{
    return FALSE;
}

/* AB3 mode's type rules (change ab3-pointer-assign) */

void ext_assignstart(void)
{
}

void ext_evalvalue(ULONG code, UWORD flags)
{
}

void ext_assigncheck(UWORD vartype)
{
}

/* AB3 mode's character literals (change ab3-mode-g3) */

BOOL ext_element(UWORD c, UWORD *type, UWORD *ret)
{
    return FALSE;
}

/* AB3 mode's macros (change ab3-mode-g3) */

void ext_macrodef(BOOL on)
{
}

void ext_macrotoken(UWORD c, UWORD next)
{
}

UWORD ext_macrolineend(void)
{
    return 0;
}

BOOL ext_macroend(UWORD next)
{
    return FALSE;
}

/* AB3 mode's procedures (change ab3-mode-g4) */

BOOL ext_procrettype(UWORD *mode)
{
    return FALSE;
}

void ext_procfast(void)
{
}

void ext_procdef(struct proc *p)
{
}

UWORD ext_procoptional(const struct proc *p, UWORD npar, UWORD c)
{
    return c;
}

UWORD ext_procmax(void)
{
    return 7;
}

BOOL ext_procparam(UWORD npar, UWORD partype, WORD off)
{
    return FALSE;
}

BOOL ext_procentry(const struct proc *p, const UWORD *st)
{
    return FALSE;
}

UWORD ext_procstackbytes(const struct proc *p)
{
    return (UWORD)((p->nparams >> 4) << 2);
}

BOOL ext_procclrloc(const struct proc *p)
{
    return TRUE;
}

void ext_procstatics(const struct proc *p)
{
}

ULONG ext_proctype(const struct proc *p)
{
    return (UBYTE)p->rettype;
}

BOOL ext_procnoargs(const struct proc *p)
{
    return FALSE;
}

void ext_procargs(const struct proc *p)
{
}

BOOL ext_procdefaults(const struct proc *p, UWORD n)
{
    return FALSE;
}

void ext_strpointer(void)
{
}
