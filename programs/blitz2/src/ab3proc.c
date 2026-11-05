/*
 * ab3proc.c - the procedures of AB3 mode (blitz2-ab3; extension points of
 * ext.h, spec ab3-compat/procedures of the change ab3-mode-g4): optional
 * parameters, up to ten parameters, FAST and NewType return types, as
 * AmiBlitz3 v3.14.0 has them (AmiBlitz3.ab3 at 58a11658).  In BB2 mode
 * none applies.
 *
 * The data AmiBlitz3 keeps in its bigger procedure node (the first
 * optional parameter, the FAST flag, the return type) are in a side table
 * here, found by the node's address (design D1): blitz2's node and its
 * chunk code stay as they are.
 *
 * The stack of a call (design D3): parameters 1-6 are in d0-d5 as in
 * blitz2.  Each string parameter has AmiBlitz3's slot of two longs: its
 * length at the higher address, below it its start for a parameter from
 * the 7th on (the registers end at d5) or a filler.  The parameters 7-10
 * that are not strings are pushed by the caller as for a library command
 * (maineval2): a word for a byte or a word, a long for the rest.
 * AmiBlitz3 passes them in four static longs (staticslib's temp1-temp4),
 * which a call inside an argument overwrites; the stack is an intended
 * deviation.  The caller pushes the slots in the order of the
 * parameters, the procedure copies them into its locals at its entry and
 * its epilogue removes them.  A slot's offset in the procedure's frame
 * (link a4) is 8 plus the sizes of the slots pushed after it.
 */
#include <exec/memory.h>
#include <proto/exec.h>
#include "blitz2.h"
#include "ab3.h"

/* AmiBlitz3's index of the first optional parameter if none is
   (AmiBlitz3.ab3:22111) */
#define NOOPT 31
/* the parameters of a procedure (ext_procmax) */
#define MAXPARS 10
/* the parameters in registers, d0-d5 */
#define REGPARS 6

/* a procedure of AB3 mode (design D1) */
struct procinfo {
    struct procinfo *next;
    const struct proc *proc;
    ULONG rettype;          /* the type code of a NewType return type, 0 */
    UBYTE firstopt;         /* the first optional parameter (0-based),
                               NOOPT if none */
    UBYTE fast;             /* FAST: the locals are not cleared */
};

#define NHASH 64
static struct procinfo *infos[NHASH];

/* the definition being read: the return type and FAST before its node is
   made (ext_procrettype, ext_procfast), its parameters 7-10 (ext_procparam) */
static ULONG defrettype;
static BOOL deffast;
static struct procinfo *definfo;
static WORD paroff[MAXPARS];

/* the argument lists being fetched (ext_procargs): the depth of the eval
   of their arguments, the innermost last */
#define MAXARGS 64
static UWORD argdepth[MAXARGS];
static UWORD nargs;

static UWORD hashof(const struct proc *p)
{
    return (UWORD)(((ULONG)p >> 2) & (NHASH - 1));
}

static struct procinfo *info(const struct proc *p)
{
    struct procinfo *i;

    for (i = infos[hashof(p)]; i; i = i->next)
        if (i->proc == p)
            return i;
    return NULL;
}

static void freeinfos(void)
{
    UWORD h;

    for (h = 0; h < NHASH; h++) {
        struct procinfo *i = infos[h];
        infos[h] = NULL;
        while (i) {
            struct procinfo *dead = i;
            i = i->next;
            FreeMem(dead, sizeof *dead);
        }
    }
}

void ab3_procpass(void)
{
    freeinfos();            /* the procedures of the pass before are freed */
    defrettype = 0;
    deffast = FALSE;
    definfo = NULL;
    nargs = 0;
}

void ab3_procfree(void)
{
    freeinfos();
}

/* the first optional parameter of p, NOOPT if none */
static UWORD firstopt(const struct proc *p)
{
    const struct procinfo *i = info(p);
    return i ? i->firstopt : NOOPT;
}

/* ============================================================ the stack */

/* the bytes of parameter k's slot of type t (0: none) */
static UWORD slotsize(UWORD k, UBYTE t)
{
    if (t == T_STRING)
        return 8;
    if (k < REGPARS)
        return 0;
    return t < T_LONG ? 2 : 4;
}

/* the frame offsets of p's slots (off[k] for parameter k, n of them);
   returns the bytes of all slots */
static UWORD layout(const struct proc *p, UWORD *off)
{
    UWORD k = p->nparams & 15, at = 8;

    while (k-- > 0) {
        off[k] = at;
        at += slotsize(k, p->params[k]);
    }
    return at - 8;
}

UWORD ext_procstackbytes(const struct proc *p)
{
    UWORD off[MAXPARS];

    if (progmode != XTRA_MODE_AB3)
        return (UWORD)((p->nparams >> 4) << 2);
    return layout(p, off);
}

/* ======================================================= the definition */

/* the return type of a Function: blitz2's sizespec2 (compile.c), with a
   NewType taken as a long whose type is kept (AmiBlitz3.ab3:1794
   check_FunctionReturnType, function_support_returntype) */
BOOL ext_procrettype(UWORD *mode)
{
    struct type *t;
    UWORD c, len;

    if (progmode != XTRA_MODE_AB3)
        return FALSE;
    defrettype = 0;
    prevloc = srcptr;
    c = getchar_();
    if (c == '.') {
        len = makename(NULL);
        if (!len)
            syntaxerr();
        t = findtype((struct node *)&alltypes, len, NULL);
        if (!t)
            notypeerr();
    } else {
        t = defaulttype;
        if (c == '$') {
            *mode = T_STRING;
            usertype = T_STRING;
            return TRUE;
        }
    }
    bakup();
    if (t->u.code >= 256) {
        defrettype = t->u.code;
        *mode = T_LONG;
    } else
        *mode = TYPE_CODE(t);
    usertype = *mode;
    return TRUE;
}

/* FAST in front of the name: the text "FAST" and a space, upper case only,
   not a token (AmiBlitz3.ab3:22045-22056) */
void ext_procfast(void)
{
    const UBYTE *q = prevloc;

    deffast = FALSE;
    if (progmode != XTRA_MODE_AB3)
        return;
    if (q[0] != 'F' || q[1] != 'A' || q[2] != 'S' || q[3] != 'T' || q[4] != ' ')
        return;
    srcptr = (UBYTE *)q + 4;
    getchar3();             /* prevloc on the name */
    deffast = TRUE;
}

void ext_procdef(struct proc *p)
{
    struct procinfo *i;

    definfo = NULL;
    if (progmode != XTRA_MODE_AB3)
        return;
    i = doallocmem(sizeof *i, MEMF_PUBLIC);
    i->proc = p;
    i->rettype = defrettype;
    i->firstopt = NOOPT;
    i->fast = deffast;
    i->next = infos[hashof(p)];
    infos[hashof(p)] = i;
    definfo = i;
    defrettype = 0;
    deffast = FALSE;
}

/* @ in front of a parameter: it and every parameter after it are optional
   (AmiBlitz3.ab3:22132-22139: only the first @ counts) */
UWORD ext_procoptional(const struct proc *p, UWORD npar, UWORD c)
{
    (void)p;
    if (!definfo || c != '@')
        return c;
    if (definfo->firstopt == NOOPT)
        definfo->firstopt = (UBYTE)npar;
    return getchar3();
}

UWORD ext_procmax(void)
{
    return progmode == XTRA_MODE_AB3 ? MAXPARS + 1 : 7;
}

BOOL ext_procparam(UWORD npar, UWORD partype, WORD off)
{
    (void)partype;          /* p->params[npar] when the parameters are read */
    if (!definfo || npar < REGPARS)
        return FALSE;
    paroff[npar] = off;
    return TRUE;
}

/* the entry: the parameters 7-10 from their slots into their locals, then
   the strings (blitz2's 'stloop with AmiBlitz3's slots, 22279-22322: the
   length for lastring from the slot, the start from the register, or for
   a register above d5 from the slot) */
BOOL ext_procentry(const struct proc *p, const UWORD *st)
{
    UWORD off[MAXPARS];
    UWORD n = p->nparams & 15, k, i;

    if (!definfo)
        return FALSE;
    layout(p, off);
    for (k = REGPARS; k < n; k++) {
        UBYTE t = p->params[k];

        if (t == T_STRING)
            continue;
        if (t == T_BYTE) {
            pokewd(0x196c);                 /* move.b off+1(a4),x(a4) */
            pokewd(off[k] + 1);
        } else {
            pokewd(t == T_WORD ? 0x396c : 0x296c);  /* move.w/.l */
            pokewd(off[k]);
        }
        pokewd((UWORD)paroff[k]);
    }
    for (i = p->nparams >> 4; i-- > 0;) {
        k = *st++;
        funcst[1] = off[k] + 4;             /* move.l len(a4),-(a7) */
        pokel(L_(funcst));
        if (k < REGPARS)
            pokewd(k | movedna3[0]);        /* move.l dN,a3 */
        else {
            pokewd(0x266c);                 /* movea.l ptr(a4),a3 */
            pokewd(off[k]);
        }
        least[1] = *st++;
        pokel(L_(least));
        tokejsr(lastring);
    }
    definfo = NULL;
    return TRUE;
}

BOOL ext_procclrloc(const struct proc *p)
{
    const struct procinfo *i;

    if (progmode != XTRA_MODE_AB3)
        return TRUE;
    i = info(p);
    return !(i && i->fast);
}

/* a FAST procedure must not need local memory: AmiBlitz3's procfixer
   (22361-22420) refuses it, string literals included */
void ext_procstatics(const struct proc *p)
{
    const struct procinfo *i;

    if (progmode != XTRA_MODE_AB3)
        return;
    i = info(p);
    if (i && i->fast)
        err("FAST directive is not allowed if local strings or newtypes are used");
}

/* ============================================================ the calls */

ULONG ext_proctype(const struct proc *p)
{
    const struct procinfo *i;

    if (progmode == XTRA_MODE_AB3 && (i = info(p)) && i->rettype)
        return i->rettype;
    return (UBYTE)p->rettype;
}

/* the defaults of the omitted parameters k and up (design D4); empty: the
   call has no arguments (name{}, AmiBlitz3.ab3:4446-4495), else they end
   after k (the tail, 8339-8466) */
static void defaults(const struct proc *p, UWORD k, BOOL empty)
{
    UWORD n = p->nparams & 15;

    for (; k < n; k++) {
        UBYTE t = p->params[k];
        ULONG v = t == T_QUICK ? 0xffff0000 : t == T_FLOAT ? 0x800000c1 : 0xffffffff;

        if (k < REGPARS) {
            if (empty && t != T_QUICK && t != T_FLOAT)
                pokewd(0x70ff | k << 9);    /* moveq #-1,dN */
            else {
                pokewd(0x203c | k << 9);    /* move.l #v,dN */
                pokel(v);
            }
            if (t != T_STRING)
                continue;
            stackuse += 8;
            if (empty) {
                pokel(0x2f3c0000);          /* move.l #0,-(a7) (length) */
                pokewd(0);
                pokel(0x2f3c0000);          /* move.l #0,-(a7) */
                pokewd(0);
            } else {
                pokel(0x48780000);          /* pea 0 (length) */
                pokewd(0x594f);             /* subq.w #4,a7 */
            }
        } else if (t == T_STRING) {
            stackuse += 8;
            pokel(0x48780000);              /* pea 0 (length) */
            pokewd(0x2f3c);                 /* move.l #-1,-(a7) (start) */
            pokel(0xffffffff);
        } else if (t < T_LONG) {
            stackuse += 2;
            pokel(0x3f3cffff);              /* move.w #-1,-(a7) */
        } else if (t == T_LONG) {
            stackuse += 4;
            pokel(0x4878ffff);              /* pea -1.w */
        } else {
            stackuse += 4;
            pokewd(0x2f3c);                 /* move.l #v,-(a7) */
            pokel(v);
        }
    }
}

BOOL ext_procnoargs(const struct proc *p)
{
    const UBYTE *q = srcptr;

    if (progmode != XTRA_MODE_AB3 || firstopt(p) != 0)
        return FALSE;
    while (*q == ' ')
        q++;
    if (*q != '}')
        return FALSE;
    defaults(p, 0, TRUE);
    return TRUE;
}

BOOL ext_procdefaults(const struct proc *p, UWORD n)
{
    if (progmode != XTRA_MODE_AB3 || n < firstopt(p))
        return FALSE;
    defaults(p, n, FALSE);
    return TRUE;
}

void ext_procargs(const struct proc *p)
{
    if (progmode != XTRA_MODE_AB3)
        return;
    if (!p) {
        nargs--;
        return;
    }
    if (nargs == MAXARGS)
        conmemerr();
    argdepth[nargs++] = evaldepth;
}

/* a string argument of a procedure in d0-d5: the filler of its slot below
   the length (AmiBlitz3's sbtolong 13012-13034: pea 0) */
void ext_strpointer(void)
{
    if (!nargs || argdepth[nargs - 1] != evaldepth || regat >= REGPARS || pushflag)
        return;
    stackuse += 4;
    pokel(0x48780000);                      /* pea 0 */
}

/* the right length of a string addition at d6 (an argument from the 7th
   on, of a procedure or a library command): getelement's spill has taken
   it off the stack, the saved d6 is removed (AmiBlitz3.ab3:14334
   add_strings: addq.l #4,a7) - blitz2 adds the saved d6 to the left
   length (BUG(orig), eval.c addstrings) */
UWORD ext_straddpop(UWORD op)
{
    if (progmode != XTRA_MODE_AB3 || regat != REGPARS)
        return op;
    return 0x588f;                          /* addq.l #4,a7 */
}
