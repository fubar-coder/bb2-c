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
 * Every routine keeps the registers the original keeps: the compiler calls
 * them in the middle of generated code (e.g. newmem at procedure entry,
 * with the parameters in d0-d5).  The thunks save what GCC may change.
 */
#include "blitzlib.h"
#include "blitzexec.h"

/* entry points (thunks / asm, defined below) */
void localloc(void), locfree(void), globalloc(void), globfree(void);
void newmem(void), freelast(void), codeinit(void), codefinit(void);
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
BLITZ_NULLSUB(codefinit, 0, 0);         /* $C006 - free ALL memory allocated! */
BLITZ_NULLSUB_AT(init, codeinit, 0, 0);
BLITZ_NULLSUB_AT(finit, codefinit, 0, 0);
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

/* only used with the debugger: after `errors` */
static const char mess1[] BLITZ_ERRCHK = "Out of Memory!";
static const char mess2[] BLITZ_ERRCHK = "Unable to Free Memory";

static ULONG freelast_c(void);

/**
 * @brief Library finit and token $C006 - free ALL memory allocated: every
 * list (local and global) with all its blocks.
 *
 * Keeps all registers.
 */
static void BLITZ_ROUTINE codefinit_c(void)
{
    while (freelast_c())
        ;
}
BLITZ_THUNK(codefinit, codefinit_c, "d0-d1/a0-a1");

/* err1 - out of memory: free everything, runtime error.
   DIFF: the original jumps here from localloc/newmem with their saved
   registers still on the stack and never returns (the runtime error ends
   the program); here err1 returns and the caller returns 0. */
static void err1(void)
{
    codefinit_c();
    BLITZ_RUNERR(mess1);
}

/**
 * @brief newmem ($C004, at procedure entry) - create a new node (for
 * local stuff): a new current local list.
 *
 * Keeps all registers (the compiler calls it with the parameters in
 * d0-d5).
 *
 * Errors: no memory for the list header - err1: all memory is freed and
 * the runtime error "Out of Memory!" raised.
 */
static void BLITZ_ROUTINE newmem_c(void)
{
    struct memlist *l = AllocMem(sizeof *l, MEMF_PUBLIC);

    if (!l) {
        err1();
        return;
    }
    l->next = firstdata;
    firstdata = l;
    l->first = NULL;
}
BLITZ_THUNK(newmem, newmem_c, "d0-d1/a0-a1");

/**
 * @brief Library init - start with one list (the global one).
 *
 * @return d0: the init value, the jump table membase (localloc, locfree,
 *         globalloc, globfree; !libs 65530,#la6 of other libraries).
 */
static ULONG BLITZ_ROUTINE codeinit_c(void)
{
    firstdata = NULL;
    newmem_c();
    memerr = 0;
    return (ULONG)membase;
}
BLITZ_THUNK(codeinit, codeinit_c, "d1/a0-a1");

/* membase - the init value: BRA localloc / locfree / globalloc / globfree
   (4 bytes each, called through the table by other code) */
__asm__(
"   .text\n"
"   .even\n"
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
 * @return d0: the memory.
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
BLITZ_THUNK(localloc, localloc_c, "a0-a1");

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
 * @param size d0: the size in bytes.
 * @param flags d1: the AllocMem flags.
 * @return d0: the memory.
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
BLITZ_THUNK(globalloc, globalloc_c, "a0-a1");

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
 * Keeps all registers.
 *
 * @return Z = 1 if there was none (result 0, tested as long; an exec
 *         BOOL, internal, not a Blitz truth value).
 */
static ULONG BLITZ_ROUTINE freelast_c(void)
{
    struct memlist *l = firstdata;
    struct memblock *b;

    if (!l)
        return FALSE;
    firstdata = l->next;
    b = l->first;
    FreeMem(l, sizeof *l);
    while (b) {
        struct memblock *next = b->next;
        FreeMem(b, b->len);
        b = next;
    }
    return TRUE;
}
BLITZ_THUNK_Z(freelast, freelast_c, "d0-d1/a0-a1");
