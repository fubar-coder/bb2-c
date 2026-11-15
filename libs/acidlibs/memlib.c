/*
 * memlib - system library 65530 ($C000-$C006): memory management of Blitz
 * programs (tokens localloc..freelast, blitz2.s equates 80-85).
 *
 * C translation of bblibsrc/memlib (v1.1).  From the original:
 *
 *   firstdata.l - points to current list header
 *   listheader - 8 bytes: 00.l next listheader, 04.l first memblock
 *   Memblocks are 8 bytes longer than normal:
 *     00.l next, 04.l length of memblock + 8, 08... mem returned to user
 *
 *   firstdata ---list---list---list...
 *                 !      !      !
 *                mem    mem    mem
 *
 *   first list is always the current local one (newmem at procedure entry
 *   adds one, freelast at the exit frees it with all its blocks); the last
 *   list is the global one.
 *
 * Every routine keeps the registers of its contract: the compiler calls
 * them in the middle of generated code (newmem at procedure entry, with
 * the parameters in d0-d5; perf-analysis thunks.md 1.4), and third-party
 * libraries call globalloc and globfree with registers live
 * (analyse-internal-token-callers.md).  The thunks save what GCC may
 * change of those.
 *
 * Fixed (intended deviation, #956): "Out of Memory!" (err1) and "Unable
 * to Free Memory" (err2) trap in the normal code, but the original has
 * their messages in the error check area (without the debugger d0 points
 * behind the library); here the messages are in the normal code.  The
 * error check area is empty.
 */
#include "blitzlib.h"
#include "blitzexec.h"

/* entry points (thunks, newmem and freelast assembler; defined below) */
void localloc(void), locfree(void), globalloc(void), globfree(void);
void newmem(void), freelast(void), memlib_codeinit(void);
void memlib_codefinit(void);
extern char membase[];

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !syslibheader{#memlib,$c000,$c007,init,1,finit,errors} */
BLITZ_SYSLIBHEADER7(LIB_memlib, 0xc000, 0xc007, init, BL_LONGRET, finit, BLITZ_ERROR_AREA);
BLITZ_NULLSUB(localloc, 0, 0);          /* $C000 */
BLITZ_NULLSUB(locfree, 0, 0);           /* $C001 */
BLITZ_NULLSUB(globalloc, 0, 0);         /* $C002 */
BLITZ_NULLSUB(globfree, 0, 0);          /* $C003 */
BLITZ_NULLSUB(newmem, 0, 0);            /* $C004 */
BLITZ_NULLSUB(freelast, 0, 0);          /* $C005 */
BLITZ_NULLSUB(memlib_codefinit, 0, 0);  /* $C006 - free ALL memory allocated! */
BLITZ_NULLSUB_AT(init, memlib_codeinit, 0, 0);
BLITZ_NULLSUB_AT(finit, memlib_codefinit, 0, 0);
BLITZ_LIBFIN();

struct memblock {
    struct memblock *next;              /* 00.l */
    ULONG len;                          /* 04.l length + 8 */
};                                      /* 08... user memory */

struct memlist {
    struct memlist *next;               /* 00.l next listheader */
    struct memblock *first;             /* 04.l first memblock */
};

static struct memlist *firstdata;
static UWORD memerr;

/* the messages of err1/err2 in the normal code (fixed, #956: in the
   original they lie after `errors`, in the error check area, although
   err1/err2 trap in the normal code) */
static const char mess1[] = "Out of Memory!";
static const char mess2[] = "Unable to Free Memory";

/**
 * @brief Library finit and token $C006 - free ALL memory allocated: every
 * list (local and global) with all its blocks.
 *
 * freelast until there is no list (freelast's Z is firstdata = 0).
 * Keeps all registers (the thunk; freelast keeps d0/d2-d7/a2-a6).
 */
static void BLITZ_ROUTINE codefinit_c(void)
{
    while (firstdata)
        __asm volatile("jsr     _freelast(pc)" : : : "d1", "a0", "a1", "cc", "memory");
}
BLITZ_THUNK(memlib_codefinit, codefinit_c, "d0-d1/a0-a1");

/* err1 - out of memory: free everything, runtime error.
   DIFF: the original jumps here from localloc/newmem with their saved
   registers still on the stack and never returns (the runtime error ends
   the program); here err1 returns and the caller returns 0.  Called from
   the assembler of newmem too (BLITZ_ROUTINE: GCC emits it). */
static void BLITZ_ROUTINE err1(void)
{
    codefinit_c();
    BLITZ_RUNERR(mess1);
}

/**
 * @brief newmem ($C004, at procedure entry) - create a new node (for
 * local stuff): a new current local list.
 *
 * AllocMem(8, MEMF_PUBLIC), the list header in front of firstdata, no
 * blocks.  Keeps d0-d7/a2-a6: the compiler calls it in the procedure
 * prologue with the parameters in d0-d5 (perf-analysis thunks.md 1.4);
 * a0 and a1 are free (the original keeps them), no third-party library
 * calls it.
 *
 * An assembler entry, the original's code with d0-d1/a6 saved: C cannot
 * keep d0/d1 across AllocMem, and a thunk around the C function saved
 * them a second time.
 *
 * Errors: no memory for the list header - err1: all memory is freed and
 * the runtime error "Out of Memory!" raised (DIFF: err1 returns, see
 * there).
 */
BLITZ_ASM_ROUTINE(newmem,
        "   movem.l d0-d1/a6,-(sp)\n"
        "   moveq   #8,d0\n"                /* sizeof (struct memlist) */
        "   moveq   #1,d1\n"                /* MEMF_PUBLIC */
        "   move.l  4.w,a6\n"
        "   jsr     -198(a6)\n"             /* AllocMem */
        "   tst.l   d0\n"
        "   beq.s   1f\n"
        "   move.l  d0,a0\n"
        "   move.l  _firstdata,(a0)\n"      /* next */
        "   move.l  a0,_firstdata\n"
        "   clr.l   4(a0)\n"                /* first: no blocks */
        "   movem.l (sp)+,d0-d1/a6\n"
        "   rts\n"
        "1: jsr     _err1(pc)\n"
        "   movem.l (sp)+,d0-d1/a6\n"
        "   rts");

/**
 * @brief Library init - start with one list (the global one).
 *
 * @return d0: the init value, the jump table membase (localloc, locfree,
 *         globalloc, globfree; !libs 65530,#la6 of other libraries).
 */
static ULONG BLITZ_ROUTINE codeinit_c(void)
{
    firstdata = NULL;
    __asm volatile("jsr     _newmem(pc)" : : : "a0", "a1", "cc", "memory");
    memerr = 0;
    return (ULONG)membase;
}
BLITZ_THUNK(memlib_codeinit, codeinit_c, "d1/a0-a1");

/* membase - the init value: BRA localloc / locfree / globalloc / globfree
   (4 bytes each, called through the table by other code) */
BLITZ_ASM_BLOCK(membase,
"   .globl  _membase\n"
"_membase:\n"
"   bra.w   _localloc\n"
"   bra.w   _locfree\n"
"   bra.w   _globalloc\n"
"   bra.w   _globfree\n"
);

/* findglob - the global list is the last one */
static struct memlist *findglob(struct memlist *l)
{
    while (l->next)
        l = l->next;
    return l;
}

/**
 * @brief localloc ($C000) - allocate some memory in the local region!
 *
 * The block (8 bytes longer: next, length + 8) goes into the current
 * (first) list.  Keeps a0-a1/a6.
 *
 * Errors: out of memory - err1: all memory is freed and the runtime error
 * "Out of Memory!" raised.  DIFF: the original never returns from err1
 * (its saved registers stay on the stack); here the result is 0.
 *
 * @param size d0: the size in bytes.
 * @param flags d1: the AllocMem flags.
 * @return d0: the memory; the flags of `tst.l d0` (Z clear after a
 *         successful allocation, as in the original, whose failure does
 *         not return; the same as globalloc).
 */
static ULONG BLITZ_ROUTINE localloc_c(ULONG size BL_REG(d0), ULONG flags BL_REG(d1))
{
    struct memblock *b;

    size += 8;                          /* we need this extra */
    b = AllocMem(size, flags);
    if (!b) {
        err1();
        return 0;
    }
    b->next = firstdata->first;
    firstdata->first = b;
    b->len = size;
    return (ULONG)(b + 1);
}
BLITZ_THUNK_Z(localloc, localloc_c, "a0-a1");

/**
 * @brief locfree ($C001) - free memory of the current local list.
 *
 * The block is looked up in the list and freed with its stored length.
 * Keeps a0/d1/a6.
 *
 * Errors: a block that is not in the list - the runtime error "Unable to
 * Free Memory", reported only once (memerr).  DIFF (K): if the trap
 * returns, the original runs on into newmem; here locfree returns.
 *
 * @param mem a1: the memory (d0 = length, ignored!).
 */
static void BLITZ_ROUTINE locfree_c(APTR mem BL_REG(a1))
{
    struct memblock *b = (struct memblock *)mem - 1;      /* SUBQ #8,a1 */
    struct memblock **link;

    if (!firstdata)
        return;
    /* a0 = firstdata+4: the `first` field works as a node whose next is
       at offset 0 */
    for (link = &firstdata->first; *link; link = &(*link)->next) {
        if (*link == b) {
            *link = b->next;
            FreeMem(b, b->len);
            return;
        }
    }
    if (memerr)                         /* report only once */
        return;
    memerr = ~memerr;                   /* NOT memerr */
    BLITZ_RUNERR(mess2);                /* err2 */
    /* DIFF (K): if the trap returns, the original runs on into newmem
       with locfree's saved registers on the stack; here locfree returns */
}
BLITZ_THUNK(locfree, locfree_c, "d1/a0");

/**
 * @brief globalloc ($C002) - like localloc, but in the global (last) list.
 *
 * Keeps a0-a1.  Errors as localloc.
 *
 * Third-party libraries (XBCiaTrackerLib, XBSoundSystemLib, RIARexxLib,
 * EFMUILib, and banklib AllocMem's callers) test the result with `beq`
 * right after the call, without a `tst`: the original leaves Z clear (its
 * last flag-setting instruction moves the list head, never 0, and a
 * failure does not return).  Fixed (#1059): the thunk ends with
 * `tst.l d0` (BLITZ_THUNK_Z), so the flags do not depend on the caller's
 * registers.
 *
 * @param size d0: the size in bytes.
 * @param flags d1: the AllocMem flags.
 * @return d0: the memory; the flags of `tst.l d0` (Z clear after a
 *         successful allocation).
 */
static ULONG BLITZ_ROUTINE globalloc_c(ULONG size BL_REG(d0), ULONG flags BL_REG(d1))
{
    struct memlist *save = firstdata;
    ULONG mem;

    firstdata = findglob(firstdata);
    mem = localloc_c(size, flags);
    firstdata = save;
    return mem;
}
BLITZ_THUNK_Z(globalloc, globalloc_c, "a0-a1");

/**
 * @brief globfree ($C003) - like locfree, in the global list.
 *
 * Keeps a0/d1.  Errors as locfree.
 *
 * @param mem a1: the memory (d0 = length, ignored).
 */
static void BLITZ_ROUTINE globfree_c(APTR mem BL_REG(a1))
{
    struct memlist *save = firstdata;

    firstdata = findglob(firstdata);
    locfree_c(mem);
    firstdata = save;
}
BLITZ_THUNK(globfree, globfree_c, "d1/a0");

/**
 * @brief freelast ($C005, at procedure exit) - free up most recent node
 * with all its blocks.
 *
 * The list header (8 bytes), then every block with its stored length
 * (FreeMem).  Keeps d0 and d2-d7/a2-a6: the compiler calls it at the end
 * of a procedure with the result of a Function in d0 (perf-analysis
 * thunks.md 1.4); d1, a0 and a1 are free (the original keeps them), no
 * third-party library calls it.
 *
 * An assembler entry, the original's code with d0/a2/a6 saved: C cannot
 * keep d0 nor return a flag, and the thunk around the C function saved
 * d0-d1/a0-a1.
 *
 * @return Z = 1 if there was none (the original's MOVEQ #-1,d0 / the
 *         BEQ on firstdata; internal, not a Blitz truth value).
 */
BLITZ_ASM_ROUTINE(freelast,
        "   movem.l d0/a2/a6,-(sp)\n"
        "   move.l  4.w,a6\n"
        "   move.l  _firstdata,d0\n"
        "   beq.s   2f\n"                   /* none: Z = 1 */
        "   move.l  d0,a1\n"
        "   move.l  (a1),_firstdata\n"
        "   move.l  4(a1),a2\n"             /* the first block */
        "   moveq   #8,d0\n"
        "   jsr     -210(a6)\n"             /* FreeMem the list header */
        "1: move.l  a2,d0\n"
        "   beq.s   3f\n"
        "   move.l  a2,a1\n"
        "   move.l  4(a2),d0\n"             /* its length + 8 */
        "   move.l  (a2),a2\n"
        "   jsr     -210(a6)\n"             /* FreeMem */
        "   bra.s   1b\n"
        "3: moveq   #-1,d0\n"               /* a list: Z = 0 */
        "2: movem.l (sp)+,d0/a2/a6\n"       /* the CCR stays */
        "   rts");
