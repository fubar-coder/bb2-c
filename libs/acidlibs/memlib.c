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
 * Intended deviation (kind B): the list headers are not allocated one by
 * one but taken from buckets of BUCKET_K entries (see there), so a
 * procedure call makes no exec call for its header; up to one empty
 * bucket stays allocated until the depth goes down further or the program
 * ends.
 *
 * Every routine keeps the registers of its contract: the compiler calls
 * them in the middle of generated code (newmem at procedure entry, with
 * the parameters in d0-d5; perf-analysis thunks.md 1.4), and third-party
 * libraries call globalloc and globfree with registers live
 * (analyse-internal-token-callers.md).  The entries $C000-$C005 are
 * assembler in the original's form (a thunk around a C function saved
 * the registers a second time, behind a second call level); the thunks of
 * init and finit (also $C006) save what GCC may change of those.
 *
 * Fixed (intended deviation, #956): "Out of Memory!" (err1) and "Unable
 * to Free Memory" (err2) trap in the normal code, but the original has
 * their messages in the error check area (without the debugger d0 points
 * behind the library); here the messages are in the normal code.  The
 * error check area is empty.
 */
#include "blitzlib.h"
#include "blitzexec.h"

/* entry points (the tokens in assembler, init/finit thunks; defined below) */
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

/* The list headers live in buckets of BUCKET_K entries (one AllocMem
   per bucket, not per header): BUCKET_SIZE bytes, 00.l up (the empty
   spare bucket above this one, or 0), 04... the entries (8 bytes each,
   struct memlist).  Headers are taken and given back in stack order
   (freelast frees the newest list), so the used entries are the first
   ones of the buckets, the newest is firstdata, and the `next` of an
   entry that is not the first of its bucket is the entry right below it.
   bklast is the address of the last entry of firstdata's bucket (0: no
   list); newmem compares firstdata with it.  At most one empty bucket
   stays allocated: when a bucket becomes empty, a spare above it is
   freed; when the last list is freed, its bucket is too.  The strings
   are for the assembler of newmem and freelast; the static assertions
   tie them to BUCKET_K. */
#define BUCKET_K                8
#define BUCKET_SIZE             (4 + 8 * BUCKET_K)
#define BUCKET_SIZE_S           "68"    /* BUCKET_SIZE */
#define BUCKET_LAST_S           "60"    /* the last entry, from the bucket */
#define BUCKET_UP_OF_LAST       "-60"   /* the bucket's up, from its last entry */
_Static_assert(BUCKET_SIZE == 68 && BUCKET_SIZE <= 127, "BUCKET_SIZE_S, moveq");
_Static_assert(4 + 8 * (BUCKET_K - 1) == 60, "BUCKET_LAST_S, BUCKET_UP_OF_LAST");

/* firstdata, bklast, memerr and mess2 are also read by the assembler
   entries (used: GCC does not see those references) */
static struct memlist *firstdata __attribute__((used));
static struct memlist *bklast __attribute__((used));
static UWORD memerr __attribute__((used));

/* the messages of err1/err2 in the normal code (fixed, #956: in the
   original they lie after `errors`, in the error check area, although
   err1/err2 trap in the normal code) */
static const char mess1[] = "Out of Memory!";
static const char mess2[] __attribute__((used)) = "Unable to Free Memory";

/**
 * @brief Library finit and token $C006 - free ALL memory allocated: every
 * list (local and global) with all its blocks.
 *
 * freelast until there is no list (freelast's Z is firstdata = 0); the
 * freelast of the last list frees the buckets too.  Keeps all registers (the thunk; freelast keeps d0/d2-d7/a2-a6).
 */
static void BLITZ_ROUTINE codefinit_c(void)
{
    while (firstdata)
        __asm volatile("jsr     _freelast(pc)" : : : "d1", "a0", "a1", "cc", "memory");
}
BLITZ_THUNK(memlib_codefinit, codefinit_c, "d0-d1/a0-a1");

/* err1 - out of memory: free every block of every list, runtime error.
   Fixed (#1071): the global list header stays, empty, as the only list,
   so a program that catches the error (SetErr) and goes on still has a
   list; the original frees the headers too (codefinit), and the next
   localloc or globalloc linked its block into the list at address 0,
   writing over ExecBase at address 4.  The local lists are freed with
   freelast down to the global one; the global list is the first entry of
   the first bucket (from init), so its blocks are freed here, every
   bucket above the first one is freed through the `up` chain and bklast
   is the first bucket's last entry.  Called from globalloc's localloc
   (firstdata is the global list then, and the local lists, abandoned by
   the error, stay allocated as in the original) the `up` chain frees the
   buckets of the local headers too.  Nothing is allocated, so this cannot
   fail; only a failure of init's first bucket leaves no list (no program
   code has run then).
   DIFF: the original jumps here from localloc/newmem with their saved
   registers still on the stack and never returns (the runtime error ends
   the program, or SetErr continues at its handler with its own stack);
   here err1 returns if the trap does, and the caller returns 0.  Called
   from the assembler of newmem too (BLITZ_ROUTINE: GCC emits it). */
static void BLITZ_ROUTINE err1(void)
{
    struct memlist *g;
    struct memblock *b, *nb;
    ULONG *bk, *up;

    if (firstdata) {
        while (firstdata->next)
            __asm volatile("jsr     _freelast(pc)" : : : "d1", "a0", "a1", "cc", "memory");
        g = firstdata;
        for (b = g->first; b; b = nb) {
            nb = b->next;
            FreeMem(b, b->len);
        }
        g->first = NULL;
        bk = (ULONG *)g - 1;            /* the first bucket */
        for (up = (ULONG *)*bk; up; up = bk) {
            bk = (ULONG *)*up;
            FreeMem(up, BUCKET_SIZE);
        }
        *((ULONG *)g - 1) = 0;
        bklast = g + BUCKET_K - 1;
    }
    BLITZ_RUNERR(mess1);
}

/**
 * @brief newmem ($C004, at procedure entry) - create a new node (for
 * local stuff): a new current local list.
 *
 * The list header (8 bytes, no blocks) in front of firstdata.  It is the
 * next entry of the top bucket (see the buckets above); only when that
 * bucket is full and has no spare is a new bucket allocated
 * (AllocMem(BUCKET_SIZE, MEMF_PUBLIC)).  Keeps d0-d7/a2-a6: the compiler
 * calls it in the procedure prologue with the parameters in d0-d5
 * (perf-analysis thunks.md 1.4); a0 and a1 are free (the original keeps
 * them), no third-party library calls it.
 *
 * An assembler entry: the fast path uses a0/a1 only; the bucket change
 * saves d0-d1/a6 around AllocMem.
 *
 * Errors: no memory for a bucket - err1: all blocks are freed (the empty
 * global list stays) and the runtime error "Out of Memory!" raised
 * (DIFF: err1 returns, see there).
 */
BLITZ_ASM_ROUTINE(newmem,
        "   move.l  _firstdata(pc),a1\n"
        "   cmpa.l  _bklast(pc),a1\n"
        "   beq.s   2f\n"                   /* the bucket is full, or none */
        "   lea     8(a1),a0\n"             /* the next entry */
        "   move.l  a1,(a0)\n"              /* next */
        "   clr.l   4(a0)\n"                /* first: no blocks */
        "   move.l  a0,_firstdata\n"
        "   rts\n"
        "2: movem.l d0-d1/a6,-(sp)\n"
        "   move.l  a1,d0\n"
        "   beq.s   3f\n"                   /* no list: a first bucket */
        "   move.l  " BUCKET_UP_OF_LAST "(a1),d0\n"   /* the full bucket's spare */
        "   bne.s   4f\n"
        "3: moveq   #" BUCKET_SIZE_S ",d0\n"
        "   moveq   #1,d1\n"                /* MEMF_PUBLIC */
        "   move.l  4.w,a6\n"
        "   jsr     -198(a6)\n"             /* AllocMem */
        "   tst.l   d0\n"
        "   beq.s   1f\n"
        "   move.l  d0,a0\n"
        "   clr.l   (a0)\n"                 /* up: no spare */
        "   move.l  _firstdata(pc),d1\n"
        "   beq.s   4f\n"
        "   move.l  d1,a1\n"
        "   move.l  d0," BUCKET_UP_OF_LAST "(a1)\n"   /* above the full one */
        "4: move.l  d0,a0\n"                /* the bucket (empty) */
        "   lea     " BUCKET_LAST_S "(a0),a1\n"
        "   move.l  a1,_bklast\n"
        "   move.l  _firstdata(pc),4(a0)\n" /* its first entry: next */
        "   clr.l   8(a0)\n"                /* first: no blocks */
        "   addq.l  #4,a0\n"
        "   move.l  a0,_firstdata\n"
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
    bklast = NULL;
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

/**
 * @brief localloc ($C000) - allocate some memory in the local region!
 *
 * The block (8 bytes longer: next, length + 8) goes into the current
 * (first) list.  Keeps d2-d7/a0-a3/a6 (exec keeps d2-d7/a2-a5).
 *
 * An assembler entry, the original's code: a thunk around a C function
 * saved a0/a1 and the C function a6 and d2 again, behind a second call
 * level.
 *
 * Errors: out of memory - err1: all blocks are freed (the empty global
 * list stays) and the runtime error "Out of Memory!" raised.  DIFF: the original never returns from err1
 * (its saved registers stay on the stack); here the result is 0.
 *
 * @param size d0: the size in bytes.
 * @param flags d1: the AllocMem flags.
 * @return d0: the memory; Z clear after a successful allocation (the
 *         flags of the final ADDQ, as in the original, whose failure does
 *         not return; the same as globalloc), Z set with 0.
 */
BLITZ_ASM_ROUTINE(localloc,
        "   movem.l a0-a1/a6,-(sp)\n"
        "   addq.l  #8,d0\n"                /* we need this extra */
        "   move.l  d0,-(sp)\n"
        "   move.l  4.w,a6\n"
        "   jsr     -198(a6)\n"             /* AllocMem */
        "   tst.l   d0\n"
        "   beq.s   1f\n"
        "   move.l  d0,a0\n"
        "   move.l  _firstdata,a1\n"
        "   move.l  4(a1),(a0)\n"           /* next = the list's first */
        "   move.l  a0,4(a1)\n"
        "   move.l  (sp)+,4(a0)\n"          /* length + 8 */
        "   addq.l  #8,d0\n"                /* the user memory: Z = 0 */
        "   movem.l (sp)+,a0-a1/a6\n"       /* the CCR stays */
        "   rts\n"
        "1: addq.l  #4,sp\n"
        "   jsr     _err1(pc)\n"
        "   moveq   #0,d0\n"                /* Z = 1 */
        "   movem.l (sp)+,a0-a1/a6\n"
        "   rts");

/**
 * @brief locfree ($C001) - free memory of the current local list.
 *
 * The block is looked up in the list and freed with its stored length.
 * Keeps d1-d7/a0/a2-a3/a6.
 *
 * An assembler entry, the original's code (a thunk around a C function
 * saved d1/a0 and the C function a6 again).
 *
 * Errors: a block that is not in the list - the runtime error "Unable to
 * Free Memory", reported only once (memerr).  DIFF (K): if the trap
 * returns, the original runs on into newmem with locfree's saved
 * registers on the stack; here locfree returns.
 *
 * @param mem a1: the memory (d0 = length, ignored!).
 */
BLITZ_ASM_ROUTINE(locfree,
        "   movem.l d1/a0/a6,-(sp)\n"
        "   move.l  _firstdata,d1\n"
        "   beq.s   3f\n"
        "   subq.l  #8,a1\n"                /* the block */
        "   move.l  d1,a0\n"
        "   addq.l  #4,a0\n"                /* first: a node whose next is at 0 */
        "1: cmp.l   (a0),a1\n"
        "   bne.s   2f\n"
        "   move.l  (a1),(a0)\n"            /* unlink */
        "   move.l  4(a1),d0\n"             /* its length + 8 */
        "   move.l  4.w,a6\n"
        "   jsr     -210(a6)\n"             /* FreeMem */
        "3: movem.l (sp)+,d1/a0/a6\n"
        "   rts\n"
        "2: move.l  (a0),d1\n"
        "   move.l  d1,a0\n"
        "   bne.s   1b\n"
        "   tst.w   _memerr\n"              /* report only once */
        "   bne.s   3b\n"
        "   not.w   _memerr\n"
        "   move.l  #_mess2,d0\n"           /* err2 */
        "   trap    #0\n"
        "   bra.s   3b");

/**
 * @brief globalloc ($C002) - like localloc, but in the global (last)
 * list; globfree ($C003) - like locfree, in the global list.
 *
 * The original's frame: firstdata is saved, set to the global list (the
 * last one, the original's findglob, here inline) and restored behind the
 * call of localloc / locfree.  globalloc keeps d2-d7/a0-a3/a6, globfree
 * d1-d7/a0/a2-a3/a6.  Errors as localloc / locfree.
 *
 * Third-party libraries (XBCiaTrackerLib, XBSoundSystemLib, RIARexxLib,
 * EFMUILib, and banklib AllocMem's callers) test the result of globalloc
 * with `beq` right after the call, without a `tst`: the original leaves Z
 * clear (its last flag-setting instruction moves the list head, never 0,
 * and a failure does not return).  Fixed (#1059): globalloc ends with
 * `tst.l d0`, so the flags do not depend on the caller's registers.
 * Third-party callers keep a0/a1 across globalloc and d1/a0 across
 * globfree (analyse-internal-token-callers.md).
 *
 * globalloc: @param size d0, @param flags d1; @return d0: the memory, the
 * flags of `tst.l d0` (Z clear after a successful allocation).
 * globfree: @param mem a1: the memory (d0 = length, ignored).
 */
BLITZ_ASM_ROUTINE(globalloc,
        "   move.l  a0,-(sp)\n"
        "   move.l  _firstdata,a0\n"
        "   move.l  a0,-(sp)\n"
        "1: tst.l   (a0)\n"                 /* findglob */
        "   beq.s   2f\n"
        "   move.l  (a0),a0\n"
        "   bra.s   1b\n"
        "2: move.l  a0,_firstdata\n"
        "   bsr.w   _localloc\n"
        "   move.l  (sp)+,_firstdata\n"
        "   move.l  (sp)+,a0\n"
        "   tst.l   d0\n"
        "   rts");

BLITZ_ASM_ROUTINE(globfree,
        "   move.l  a0,-(sp)\n"
        "   move.l  _firstdata,a0\n"
        "   move.l  a0,-(sp)\n"
        "1: tst.l   (a0)\n"                 /* findglob */
        "   beq.s   2f\n"
        "   move.l  (a0),a0\n"
        "   bra.s   1b\n"
        "2: move.l  a0,_firstdata\n"
        "   bsr.w   _locfree\n"
        "   move.l  (sp)+,_firstdata\n"
        "   move.l  (sp)+,a0\n"
        "   rts");

/**
 * @brief freelast ($C005, at procedure exit) - free up most recent node
 * with all its blocks.
 *
 * The list header goes back to its bucket, then every block is freed with
 * its stored length (FreeMem).  When the header was the first entry of
 * its bucket, that bucket is empty: the spare above it, if any, is freed
 * (the last two buckets were empty), and the bucket itself stays as the
 * spare, unless it held the last list (then it is freed too: no list, no
 * bucket).  Keeps d0 and d2-d7/a2-a6: the compiler calls it at the end of
 * a procedure with the result of a Function in d0 (perf-analysis
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
        "   move.l  _firstdata(pc),d0\n"
        "   beq.s   2f\n"                   /* none: Z = 1 */
        "   move.l  d0,a1\n"
        "   move.l  (a1)+,d0\n"             /* next */
        "   move.l  d0,_firstdata\n"
        "   move.l  (a1),a2\n"              /* the first block */
        "   lea     -12(a1),a0\n"           /* the entry below in the bucket */
        "   cmpa.l  d0,a0\n"
        "   bne.s   4f\n"                   /* the first entry: the bucket is empty */
        "1: move.l  a2,d0\n"
        "   beq.s   3f\n"
        "   move.l  a2,a1\n"
        "   move.l  4(a2),d0\n"             /* its length + 8 */
        "   move.l  (a2),a2\n"
        "   jsr     -210(a6)\n"             /* FreeMem */
        "   bra.s   1b\n"
        "3: moveq   #-1,d0\n"               /* a list: Z = 0 */
        "2: movem.l (sp)+,d0/a2/a6\n"       /* the CCR stays */
        "   rts\n"
        "4: move.l  d0,_bklast\n"           /* the full one below, or 0 */
        "   subq.l  #8,a1\n"                /* this bucket */
        "   move.l  (a1),d1\n"              /* its spare */
        "   beq.s   5f\n"
        "   clr.l   (a1)\n"
        "   move.l  a1,-(sp)\n"
        "   move.l  d1,a1\n"
        "   moveq   #" BUCKET_SIZE_S ",d0\n"
        "   jsr     -210(a6)\n"             /* FreeMem the spare */
        "   move.l  (sp)+,a1\n"
        "5: move.l  _firstdata(pc),d0\n"
        "   bne.s   1b\n"                   /* this one is the spare now */
        "   moveq   #" BUCKET_SIZE_S ",d0\n"
        "   jsr     -210(a6)\n"             /* the last list: FreeMem it */
        "   bra.s   1b");
