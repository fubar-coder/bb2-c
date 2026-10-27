/*
 * intlib - system library 65520 ($C100-$C107): interrupt handlers of Blitz
 * programs (SetInt/ClrInt ... End SetInt, VWait).
 *
 * C translation of bblibsrc/intlib (V1.1 - "blitz mode stuff removed");
 * ref/intlib.obj matches the source.  Compiler tokens (blitz2.s equates):
 * setint/clrint ($C100/$C101), newint/oldint ($C102/$C103, start and end
 * of an interrupt handler's code), vwait ($C104), the cleanup at program
 * end ($C105); $C106/$C107 are called by switchlib when the program goes
 * to Blitz / Amiga mode.
 *
 * Every SetInt allocates a node (memlib globalloc, MEMF_PUBLIC|MEMF_CLEAR):
 *     0.l next node of the same level
 *     4.w level as given in d0 (low 4 bits = interrupt number, the other
 *         bits allow selective ClrInt)
 *     6   struct Interrupt (is_Code = handler, is_Data = 0, pri 0)
 * kept in 16 lists per mode: _fints (Amiga mode, servers added at once)
 * and _bints (Blitz mode, servers added by goblitz while in Blitz mode).
 *
 * SetInt/ClrInt/VWait are Blitz/Amiga variants (`Dc $a001`, see
 * BLITZ_AB_ROUTINE): the Blitz mode version comes first in the prefix.
 * Everything is C; the prefixes are the only assembler.
 */
#include "blitzlib.h"
#include "blitzexec.h"
#include <exec/interrupts.h>
#define __NOLIBBASE__
#define GRAPHICS_BASE_NAME gfx
#include <proto/graphics.h>
#undef __NOLIBBASE__

/* entry points (thunks / asm, defined below) */
void setint(void), clrint(void), newint(void), oldint(void), dovwait(void);
void cleanup(void), goblitz(void), goamiga(void), codeinit(void);

BLITZ_ENTRY_DECL(init);

/* !syslibheader{#intlib,$c100,$c108,_init,0,0,0} */
BLITZ_SYSLIBHEADER7(LIB_intlib, 0xc100, 0xc108, init, BL_NORET, 0, 0);
BLITZ_NULLSUB(setint, 0, 0);                    /* $C100 */
BLITZ_NULLSUB(clrint, 0, 0);                    /* $C101 */
BLITZ_NULLSUB(newint, 0, 0);                    /* $C102 */
BLITZ_NULLSUB(oldint, 0, 0);                    /* $C103 */
/* !nullsub{dovwait,0,0,#graphicslib,$5600}: $5600 = #la6|$4000 as in the
   original (graphics base in a6) */
BLITZ_NULLSUB(dovwait, 0, 0, LIB_graphicslib, 0x5600); /* $C104 */
BLITZ_NULLSUB(cleanup, 0, 0);                   /* $C105 */
BLITZ_NULLSUB(goblitz, 0, 0);                   /* $C106 */
BLITZ_NULLSUB(goamiga, 0, 0);                   /* $C107 */
BLITZ_NULLSUB_AT(init, codeinit, 0, 0);
BLITZ_LIBFIN();

struct intnode {
    struct intnode *next;                       /* 0.l */
    UWORD level;                                /* 4.w */
    struct Interrupt is;                        /* 6 */
};
#define NODESIZE (sizeof(struct Interrupt) + 6) /* SizeOf.Interrupt+6 = 28 */

static UWORD dontdo;                            /* don't AddIntServer now */
static UBYTE *inta5;                            /* a5 of the program */
static struct intnode *allints[32];             /* _fints: Amiga mode lists, */
#define fints (allints)                         /* _bints: Blitz mode lists */
#define bints (allints + 16)                    /* (cleanup frees all 32) */
static struct intnode **ints;                   /* list set of setint2/clrint2 */
static UWORD blitzintson;                       /* are blitz ints on? */

/* dovwait - VWait [n]: wait for n+1 vertical blanks (n = d0.w, DBF
   count: 0 = once, $FFFF = 65536 times).
   Amiga mode: WaitTOF with the graphics base in a6 (!libs); the original
   (MOVE d0,d2) keeps no register but a6 and d3-d7/a2-a5. */
static void BLITZ_ROUTINE dovwait_c(UWORD n BL_REG(d0), struct GfxBase *gfx BL_REG(a6))
{
    do
        WaitTOF();
    while (n-- != 0);
}

/* Blitz mode: poll VPOSR bit 0 (V8, vertical position >= 256) - wait for
   it to become 1, then 0 again = one frame.  Only d0 is changed by the
   original (DBF), so the thunk keeps d1/a0-a1. */
static void BLITZ_ROUTINE b_dovwait_c(UWORD n BL_REG(d0))
{
    volatile UBYTE *vposr = (volatile UBYTE *)0xdff005;

    do {
        while (!(*vposr & 1))
            ;
        while (*vposr & 1)
            ;
    } while (n-- != 0);
}
BLITZ_THUNK(b_dovwait, b_dovwait_c, "d1/a0-a1");
BLITZ_AB_ROUTINE(dovwait, b_dovwait,
"   jmp     _dovwait_c");

/* codeinit - zero out all ints (both sets, 32 lists), remember a5 */
static void BLITZ_ROUTINE codeinit_c(void)
{
    int i;

    blitzintson = 0;
    for (i = 0; i < 32; i++)
        allints[i] = NULL;
    inta5 = blitz_globals;
}
BLITZ_THUNK(codeinit, codeinit_c, "d0-d1/a0-a1");

/* intson/intsoff - add/remove the servers of all lists of a set */
static void intson(struct intnode **set)
{
    int i;
    struct intnode *n;

    for (i = 0; i < 16; i++)
        for (n = set[i]; n; n = n->next)
            AddIntServer(n->level & 15, &n->is);
}

static void intsoff(struct intnode **set)
{
    int i;
    struct intnode *n;

    for (i = 0; i < 16; i++)
        for (n = set[i]; n; n = n->next)
            RemIntServer(n->level & 15, &n->is);
}

/* cleanup - at program end: remove the servers, free all nodes of both
   sets.  BUG(orig): blitzintson is not reset (a program ends in Amiga
   mode anyway).  The original changes d0-d2/a0-a3/a6. */
void BLITZ_ROUTINE cleanup(void)
{
    struct intnode **l, *n, *next;
    int i;

    intsoff(fints);
    if (blitzintson)
        intsoff(bints);
    l = allints;
    for (i = 0; i < 32; i++, l++) {     /* fints and bints */
        n = *l;
        if (!n)
            continue;
        *l = NULL;
        do {
            struct bl_regs r = { .d0 = NODESIZE, .a1 = (ULONG)n };
            next = n->next;
            BLITZ_ALIBJSR(TOK_globfree, r);
            n = next;
        } while (n);
    }
}

/* newint - start of an interrupt handler: a5 = the program's globals */
static void BLITZ_ROUTINE newint_c(void)
{
    blitz_globals = inta5;
}
BLITZ_THUNK(newint, newint_c, "d0-d1/a0-a1");

/* oldint - end of an interrupt handler: nothing to do */
void BLITZ_ROUTINE oldint(void)
{
}

/* setint2 - d0 = level of interrupt to install (ANDed with 15, the other
   bits may be used for selective ClrInt), d1 = code.  The node goes to
   the front of list `level & 15` of `ints`.
   No check of the allocation: in the original globalloc never returns 0
   (memlib err1: codefinit + TRAP #0 "Out of Memory!", also without the
   debugger); only the C memlib returns 0 (K). */
static void setint2(UWORD level, ULONG code)
{
    struct bl_regs r = { .d0 = NODESIZE, .d1 = MEMF_PUBLIC | MEMF_CLEAR };
    struct intnode *n, **l;

    n = (struct intnode *)BLITZ_ALIBJSR(TOK_globalloc, r);
    l = ints + (level & 15);
    n->next = *l;
    *l = n;
    n->level = level;
    n->is.is_Code = (void (*)())code;
    n->is.is_Node.ln_Type = NT_INTERRUPT;
    if (!dontdo)
        AddIntServer(level & 15, &n->is);
}

/* setint (Amiga mode): servers are added at once */
static void BLITZ_ROUTINE setint_c(ULONG level BL_REG(d0), ULONG code BL_REG(d1))
{
    ints = fints;
    dontdo = 0;
    setint2(level, code);
}

/* b_setint (Blitz mode): blitz list; added now only if the blitz servers
   are on (SEQ dontdo: $FF00 = don't) */
static void BLITZ_ROUTINE b_setint_c(ULONG level BL_REG(d0), ULONG code BL_REG(d1))
{
    ints = bints;
    dontdo = blitzintson ? 0 : 0xff00;
    setint2(level, code);
}

/* the original keeps d2/a0-a1/a6, and d0-d1 when no server is added */
BLITZ_THUNK(b_setint, b_setint_c, "d0-d1/a0-a1");
BLITZ_AB_ROUTINE(setint, b_setint,
"   movem.l d0-d1/a0-a1,-(sp)\n"
"   jsr     _setint_c\n"
"   movem.l (sp)+,d0-d1/a0-a1\n"
"   rts");

/* clrint2 - d0 = level of interrupt to clear: removes every node of list
   `d0 & 15` whose level word equals d0 */
static void clrint2(UWORD level)
{
    struct intnode **link = ints + (level & 15), *n;

    while ((n = *link) != NULL) {
        if (n->level != level) {
            link = &n->next;
            continue;
        }
        if (!dontdo)
            RemIntServer(level & 15, &n->is);
        *link = n->next;
        {
            struct bl_regs r = { .d0 = NODESIZE, .a1 = (ULONG)n };
            BLITZ_ALIBJSR(TOK_globfree, r);
        }
    }
}

static void BLITZ_ROUTINE clrint_c(ULONG level BL_REG(d0))
{
    ints = fints;
    dontdo = 0;
    clrint2(level);
}

static void BLITZ_ROUTINE b_clrint_c(ULONG level BL_REG(d0))
{
    ints = bints;
    dontdo = blitzintson ? 0 : 0xff00;
    clrint2(level);
}

/* the original keeps d1-d2/a0-a3/a6 */
BLITZ_THUNK(b_clrint, b_clrint_c, "d0-d1/a0-a1");
BLITZ_AB_ROUTINE(clrint, b_clrint,
"   movem.l d0-d1/a0-a1,-(sp)\n"
"   jsr     _clrint_c\n"
"   movem.l (sp)+,d0-d1/a0-a1\n"
"   rts");

/* goblitz - (switchlib) going to Blitz mode: add the blitz servers */
static void BLITZ_ROUTINE goblitz_c(void)
{
    if (blitzintson)
        return;
    blitzintson = 0xff00;               /* ST */
    intson(bints);
}
BLITZ_THUNK(goblitz, goblitz_c, "d0-d1/a0-a1");

/* goamiga - (switchlib) back to Amiga mode: remove the blitz servers */
static void BLITZ_ROUTINE goamiga_c(void)
{
    if (!blitzintson)
        return;
    blitzintson = 0;                    /* SF */
    intsoff(bints);
}
BLITZ_THUNK(goamiga, goamiga_c, "d0-d1/a0-a1");
