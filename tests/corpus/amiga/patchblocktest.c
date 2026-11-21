/*
 * patchblocktest - proves patchblock.h: a patch is removed while another
 * task is still inside it.
 *
 *   patchblocktest [nowait]
 *
 * dos Delay() is patched with the template of the CloseWindow stub (a
 * `jsr` patch: the call returns into the stub; the template records a0
 * of every call in its ring).  A helper process of a higher priority
 * calls Delay(50) and is inside the stub, blocked, when the main process
 * removes the patch.  The removal must wait for it (PB_REMOVE, wait): the
 * stub's count is 0 on return, the ring has the call, and the helper
 * returns to its stub (in the block, not in this program's code) and
 * goes on.  One line per check ("patchblocktest: NAME: expected" or
 * "MISMATCH"), then "patchblocktest: OK" (exit status 0) or "FAIL" (5):
 *
 *   inside   the helper is inside the patch (count 1)
 *   removed  no call inside after the removal (count 0)
 *   helper   the helper returned from Delay
 *   ring     the helper's call is in the ring (1 entry, a0 as set)
 *   vector   the vector is the original again
 *
 * With `nowait` the removal does not wait (the broken input of the test):
 * "removed" must fail ("MISMATCH - count 1"), the rest still holds.
 */
#include <exec/types.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>

#define PB_CLOSE
#include "patchblock.h"

/* the vector's target: the jump table entry is jmp (4EF9) and the address */
static APTR vector_of(struct Library *lib, WORD lvo)
{
    return *(APTR *)((UBYTE *)lib + lvo + 2);
}

static struct pbclose *blk;
static volatile int helper_done;
static int bad;

static void line(const char *a, const char *b, int ok)
{
    BPTR o = Output();
    Write(o, (APTR)a, strlen(a));
    Write(o, (APTR)(ok ? ": expected" : ": MISMATCH"), 10);
    if (b) { Write(o, (APTR)" - ", 3); Write(o, (APTR)b, strlen(b)); }
    Write(o, (APTR)"\n", 1);
    if (!ok) bad = 1;
}

/* Delay(50) with a0 set to a known value, as CloseWindow's caller has its
   window in a0 */
static void helper(void)
{
    register ULONG ticks __asm__("d1") = 50;
    register APTR w __asm__("a0") = (APTR)0x00C0FFE0;
    register struct Library *d __asm__("a6") = (struct Library *)DOSBase;
    __asm__ volatile ("jsr -198(%%a6)" : "+r"(ticks), "+r"(w), "+r"(d) : : "d0", "a1", "cc", "memory");
    helper_done = 1;
}

int main(int argc, char **argv)
{
    int wait = !(argc > 1 && !strcmp(argv[1], "nowait")), i;
    APTR orig_vec, vec;
    LONG left;

    orig_vec = vector_of((struct Library *)DOSBase, -198);
    blk = (struct pbclose *)PB_INSTALL((struct Library *)DOSBase, -198, close,
                                       sizeof(struct pbclose), 0);
    if (!blk) { line("patchblocktest: install", "no stub", 0); return 20; }
    /* the helper (priority 1) runs at once and blocks inside the stub */
    if (!CreateNewProcTags(NP_Entry, (ULONG)helper, NP_Name, (ULONG)"patchblock helper",
                           NP_Priority, 1, TAG_END)) {
        line("patchblocktest: helper", "no process", 0);
        return 20;
    }
    for (i = 0; i < 20000000 && !blk->h.count; i++) ;
    line("patchblocktest: inside", "count 1", blk->h.count == 1);
    left = PB_REMOVE(&blk->h, (struct Library *)DOSBase, -198, close, wait);
    line("patchblocktest: removed", left ? "count 1" : "count 0", left == 0);
    for (i = 0; !helper_done && i < 500; i++) Delay(1);
    line("patchblocktest: helper", NULL, helper_done);
    line("patchblocktest: ring", NULL, blk->ncl == 1 && blk->ring[0] == (struct Window *)0x00C0FFE0);
    vec = vector_of((struct Library *)DOSBase, -198);
    line("patchblocktest: vector", NULL, vec == orig_vec);
    {
        const char *end = bad ? "patchblocktest: FAIL\n" : "patchblocktest: OK\n";
        Write(Output(), (APTR)end, strlen(end));
    }
    return bad ? 5 : 0;
}
