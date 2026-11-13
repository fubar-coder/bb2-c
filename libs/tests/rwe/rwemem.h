/*
 * rwemem.h - the memory balance of the test task for the RWE userlib
 * tests (header only, after sysload.h; rwecmp.h includes it).
 */
#ifndef RWEMEM_H
#define RWEMEM_H

#include <exec/execbase.h>

/* rwe_mem_start() ... rwe_mem_stop(): the memory balance of this task.
   AvailMem() counts the whole system, and other tasks allocate while a
   test runs (a test that does DOS I/O cannot hold Forbid() for its
   measurement, see systest_cmdlibs.c); so exec's AllocMem/FreeMem are
   patched (SetFunction, every allocation of exec's goes through them)
   to count only what this task allocates and frees, in MEM_BLOCKSIZE
   units.  rwe_mem_stop() takes the patches out and returns the
   balance (0: everything freed); rwe_memallocs counts the allocations
   (a balance of 0 with none says nothing).  rwe_done() (rwecmp.h) takes
   them out too, so that no early return leaves them in place.
   rwe_memothers set before rwe_mem_start(): the balance also counts
   what another task frees of the blocks this task allocated (intuition
   allocates in the caller's task and frees some of it later in
   input.device); the blocks are kept in rwe_memblk (rwe_memfull: not
   all of them). */
static struct Task *rwe_memtask;
static LONG rwe_membal, rwe_memotherfree;
static ULONG rwe_memallocs;
static APTR rwe_oldalloc, rwe_oldfree;
static BOOL rwe_memothers, rwe_memfull;
#define RWE_MEMBLKS 256
static APTR rwe_memblk[RWE_MEMBLKS];
#define RWE_MEMSIZE(n) (((n) + 7) & ~7UL)

/* the slot of block `mem` in rwe_memblk (NULL: a free slot), -1: none;
   inline, so that the patches below call nothing */
static inline __attribute__((always_inline)) int rwe_memslot(APTR mem)
{
    int i;
    for (i = 0; i < RWE_MEMBLKS; i++)
        if (rwe_memblk[i] == mem)
            return i;
    return -1;
}

static __attribute__((unused)) APTR rwe_allocmem(ULONG size __asm("d0"), ULONG req __asm("d1"),
                                                 struct ExecBase *sb __asm("a6"))
{
    register ULONG d0 __asm("d0") = size;
    register ULONG d1 __asm("d1") = req;
    register struct ExecBase *a6 __asm("a6") = sb;
    __asm volatile("jsr (%3)" : "+d"(d0), "+d"(d1) : "a"(a6), "a"(rwe_oldalloc)
                   : "a0", "a1", "cc", "memory");
    if (d0 && sb->ThisTask == rwe_memtask) {
        rwe_membal += RWE_MEMSIZE(size);
        rwe_memallocs++;
        if (rwe_memothers) {
            int i = rwe_memslot(NULL);
            if (i >= 0)
                rwe_memblk[i] = (APTR)d0;
            else
                rwe_memfull = TRUE;
        }
    }
    return (APTR)d0;
}

static __attribute__((unused)) void rwe_freemem(APTR mem __asm("a1"), ULONG size __asm("d0"),
                                                struct ExecBase *sb __asm("a6"))
{
    register APTR a1 __asm("a1") = mem;
    register ULONG d0 __asm("d0") = size;
    register struct ExecBase *a6 __asm("a6") = sb;
    if (mem && rwe_memtask && rwe_memothers) {
        int i = rwe_memslot(mem);
        if (i >= 0) {
            rwe_memblk[i] = NULL;
            if (sb->ThisTask != rwe_memtask)
                rwe_memotherfree += RWE_MEMSIZE(size);
        }
    }
    if (mem && sb->ThisTask == rwe_memtask)
        rwe_membal -= RWE_MEMSIZE(size);
    __asm volatile("jsr (%3)" : "+a"(a1), "+d"(d0) : "a"(a6), "a"(rwe_oldfree)
                   : "d1", "a0", "cc", "memory");
}

static __attribute__((unused)) void rwe_mem_start(void)
{
    if (rwe_memtask)
        return;
    rwe_memtask = FindTask(NULL);
    rwe_membal = 0;
    rwe_memotherfree = 0;
    rwe_memallocs = 0;
    rwe_memfull = FALSE;
    memset(rwe_memblk, 0, sizeof rwe_memblk);
    Forbid();
    rwe_oldalloc = SetFunction((struct Library *)SysBase, -198, (APTR)rwe_allocmem);
    rwe_oldfree = SetFunction((struct Library *)SysBase, -210, (APTR)rwe_freemem);
    Permit();
}

static __attribute__((unused)) LONG rwe_mem_stop(void)
{
    if (rwe_memtask) {
        Forbid();
        rwe_memtask = NULL;
        SetFunction((struct Library *)SysBase, -198, rwe_oldalloc);
        SetFunction((struct Library *)SysBase, -210, rwe_oldfree);
        Permit();
    }
    return rwe_membal - rwe_memotherfree;
}

/* the end of a measurement begun by rwe_mem_start() for a library test of
   the sysload.h programs (systest*.c): TRUE if this task freed everything
   it allocated and allocated something at all (a balance of 0 without an
   allocation would say nothing: the patches did not see the library) */
static __attribute__((unused)) BOOL rwe_mem_back(void)
{
    LONG bal = rwe_mem_stop();
    return bal == 0 && rwe_memallocs > 0;
}

/* the end of a measurement begun by rwe_mem_start(): the check
   "LIB: all memory back" (balance 0; with `allocs` at least one
   allocation counted, for a measurement in which the library allocates);
   avail = AvailMem(MEMF_ANY) at rwe_mem_start(), to print what other
   tasks allocated meanwhile */
static __attribute__((unused)) void rwe_mem_check(const char *lib, ULONG avail, BOOL allocs)
{
    char buf[80];
    LONG bal = rwe_mem_stop();
    LONG other = (LONG)(avail - AvailMem(MEMF_ANY)) - bal;
    if (other)
        printf("%s: %ld bytes allocated by other tasks meanwhile\n", lib, other);
    if (bal)
        printf("%s: %ld bytes not freed\n", lib, bal);
    snprintf(buf, sizeof buf, "%s: all memory back", lib);
    check(buf, bal == 0 && (!allocs || rwe_memallocs > 0));
}

#endif
