/*
 * selftest_mem - the check "all memory back" of the library tests
 * (rwemem.h: rwe_mem_back(), the task's own AllocMem/FreeMem balance)
 * with broken input, no library: a block that is not freed, a free of a
 * block the window did not see allocated, and a window without an
 * allocation (the patches saw nothing).  Each must fail; the fourth
 * window frees what it allocates and must pass.  ctest libs_mem_wrong (the
 * snitch case "mem wrong" is [.selftest] and fails on purpose).
 */
/* not every helper of sysload.h is used here */
#pragma GCC diagnostic ignored "-Wunused-function"
#include "sysload.h"
#include "rwe/rwemem.h"

int selftest_mem_wrong(void)
{
    APTR kept, outside;

    /* one block of the window is not freed */
    rwe_mem_start();
    kept = AllocMem(100, MEMF_PUBLIC);
    check("leak: all memory back", rwe_mem_back());
    if (kept)
        FreeMem(kept, 100);

    /* a block allocated outside is freed inside: the balance is negative */
    outside = AllocMem(64, MEMF_PUBLIC);
    rwe_mem_start();
    if (outside)
        FreeMem(outside, 64);
    check("free of a foreign block: all memory back", rwe_mem_back());

    /* nothing allocated: a balance of 0 says nothing */
    rwe_mem_start();
    check("no allocation: all memory back", rwe_mem_back());

    /* everything freed: passes (the failure count stays 3 up to here) */
    rwe_mem_start();
    kept = AllocMem(100, MEMF_PUBLIC);
    if (kept)
        FreeMem(kept, 100);
    check("alloc and free: all memory back", rwe_mem_back());

    /* the marks inside a running measurement (the "old one freed" and
       "freed" checks of systest_runtime.c) */
    {
        struct rwe_memmark m;
        rwe_mem_start();
        m = rwe_mem_mark();
        kept = AllocMem(100, MEMF_PUBLIC);
        check("mark: block not freed: freed", rwe_mem_since_same(m));
        if (kept)
            FreeMem(kept, 100);
        check("mark: freed again: freed", rwe_mem_since_same(m));

        m = rwe_mem_mark();
        check("mark: no allocation: old one freed", rwe_mem_since_allocs(m));
        kept = AllocMem(100, MEMF_PUBLIC);
        if (kept)
            FreeMem(kept, 100);
        check("mark: alloc and free: old one freed", rwe_mem_since_allocs(m));

        m = rwe_mem_mark();
        check("mark: nothing happened: nothing allocated", rwe_mem_since_none(m));
        kept = AllocMem(100, MEMF_PUBLIC);
        if (kept)
            FreeMem(kept, 100);
        check("mark: alloc and free: nothing allocated", rwe_mem_since_none(m));
        rwe_mem_stop();
        /* no measurement running: nothing is true */
        check("mark: no measurement: freed", rwe_mem_since_same(m));
    }

    printf("mem wrong: %d checks, %d failures\n", checks, failures);
    return failures ? 5 : 0;
}
