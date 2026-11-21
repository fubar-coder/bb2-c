/*
 * patchblock.h - system-wide SetFunction patches that can be removed
 * safely, for the test tools reqtrap, teddrive, inject and wbrun (one source,
 * as reqclose.h).
 *
 * Removing a patch while another task is inside it is a race: a `jsr`
 * patch (CloseWindow) returns into the stub, a `jmp` patch (Open,
 * AutoRequest) can be interrupted between the library vector and the
 * stub.  The tool's own memory (its code and data) is freed when it
 * ends, so such a task would run into freed code.  Here the stub and the
 * data it uses live in an AllocMem block that is never freed (about
 * 100 bytes per run; the stub is position independent and the template in
 * the program's text is copied there).  The block is chip memory, not
 * the public memory (fast or slow) that the programs under the tool use:
 * a block there shifts every address of the program, and with them the
 * cycle counts of the perf tests (cycle-exact; code and data in memory
 * that shares the DMA slots run up to 20 % slower) and what the recorded
 * original bugs depend on (the order of the allocations, garbage read
 * from memory):
 *
 *   +0  count    calls inside the stub (a task that entered it)
 *   +4  active   1 while the handler is to be called
 *   +8  old      the function the patch replaced
 *   +12 handler  the C function of the program (called only while the
 *                call is counted and active)
 *   +16 ...      data of the stub (the ring of the CloseWindow stub)
 *
 * Every call counts itself in before it looks at `active`.  The removal
 * (pb_remove) clears `active` and restores the vector under Forbid(), then
 * waits until `count` is 0 (bounded): a task that has entered the stub
 * after that finds `active` clear and jumps to the old function, using
 * nothing but the block; a task that is in the handler or in the old
 * function of a `jsr` patch is waited for.  The block itself stays valid
 * for a task that returns to it later.
 *
 * The program defines the templates it needs before it includes the
 * header: PB_OPEN (dos Open: handler(d1 name), then the old function),
 * PB_CLOSE (intuition CloseWindow: the closed windows go to a ring in the
 * block), PB_AUTO (AutoRequest: handler(body, pos, neg), returns FALSE),
 * PB_EASY (EasyRequestArgs: handler(easystruct) returns the answer: 0 =
 * the rightmost gadget, 1 = the leftmost, ...),
 * PB_THRU (any function: the program's own stub, see below).
 */
#ifndef PATCHBLOCK_H
#define PATCHBLOCK_H
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <intuition/intuition.h>
#include <proto/exec.h>
#include <proto/dos.h>

struct pbhead {
    volatile LONG count;
    volatile LONG active;
    APTR old;
    APTR handler;
};
#define PB_NRING 16             /* the CloseWindow stub's ring: a power of two */
struct pbclose {
    struct pbhead h;
    struct Window *ring[PB_NRING]; /* entry k is ring[k % PB_NRING] */
    volatile ULONG ncl;         /* closes entered */
};

/* the template: the data (initial values 0), then the code from `entry` */
#define PB_DECLARE(name) \
    extern const UBYTE pb_##name##_tmpl[], pb_##name##_entry[], pb_##name##_end[]

#define PB_ASM_HEAD(name, extra) \
"   .text\n" \
"   .even\n" \
"_pb_" #name "_tmpl:\n" \
"   .long 0,0,0,0\n" \
extra \
"_pb_" #name "_entry:\n"

#ifdef PB_OPEN
__asm__(
PB_ASM_HEAD(open, "")
"   movem.l d0-d1/a0-a1,-(sp)\n"
"   lea     _pb_open_tmpl(pc),a0\n"
"   addq.l  #1,(a0)\n"
"   tst.l   4(a0)\n"
"   beq.s   1f\n"
"   move.l  4(sp),-(sp)\n"          /* d1: the name */
"   move.l  12(a0),a1\n"
"   jsr     (a1)\n"
"   addq.l  #4,sp\n"
"   lea     _pb_open_tmpl(pc),a0\n"
"1: subq.l  #1,(a0)\n"
"   movem.l (sp)+,d0-d1/a0-a1\n"
"   move.l  _pb_open_tmpl+8(pc),-(sp)\n"
"   rts\n"
"_pb_open_end:\n"
);
PB_DECLARE(open);
#endif

#ifdef PB_CLOSE
__asm__(
PB_ASM_HEAD(close, "   .fill 68,1,0\n")    /* ring (64) and ncl (4) */
"   lea     _pb_close_tmpl(pc),a1\n"
"   addq.l  #1,(a1)\n"
"   tst.l   4(a1)\n"
"   beq.s   1f\n"
"   move.l  a0,-(sp)\n"             /* the window: closed first, then */
"   move.l  8(a1),a1\n"             /* entered in the ring */
"   jsr     (a1)\n"
"   move.l  (sp)+,a0\n"
"   lea     _pb_close_tmpl(pc),a1\n"
"   move.l  80(a1),d0\n"
"   and.w   #15,d0\n"               /* PB_NRING - 1 */
"   lsl.w   #2,d0\n"
"   move.l  a0,16(a1,d0.w)\n"
"   addq.l  #1,80(a1)\n"
"   subq.l  #1,(a1)\n"
"   rts\n"
"1: subq.l  #1,(a1)\n"
"   move.l  8(a1),-(sp)\n"
"   rts\n"
"_pb_close_end:\n"
);
PB_DECLARE(close);
#endif

#ifdef PB_AUTO
/* AutoRequest(a0 win, a1 body, a2 pos, a3 neg, d0, d1, d2 w, d3 h) */
__asm__(
PB_ASM_HEAD(auto, "")
"   movem.l d0-d1/a0-a1,-(sp)\n"
"   lea     _pb_auto_tmpl(pc),a0\n"
"   addq.l  #1,(a0)\n"
"   tst.l   4(a0)\n"
"   beq.s   1f\n"
"   move.l  a3,-(sp)\n"
"   move.l  a2,-(sp)\n"
"   move.l  20(sp),-(sp)\n"         /* a1: the body (saved at 12(sp)) */
"   move.l  12(a0),a1\n"
"   jsr     (a1)\n"
"   lea     12(sp),sp\n"
"   lea     _pb_auto_tmpl(pc),a0\n"
"   subq.l  #1,(a0)\n"
"   movem.l (sp)+,d0-d1/a0-a1\n"
"   moveq   #0,d0\n"                /* FALSE = negative gadget */
"   rts\n"
"1: subq.l  #1,(a0)\n"
"   movem.l (sp)+,d0-d1/a0-a1\n"
"   move.l  _pb_auto_tmpl+8(pc),-(sp)\n"
"   rts\n"
"_pb_auto_end:\n"
);
PB_DECLARE(auto);
#endif

#ifdef PB_EASY
/* EasyRequestArgs(a0 win, a1 easy, a2 idcmp, a3 args) */
__asm__(
PB_ASM_HEAD(easy, "")
"   movem.l d0-d1/a0-a1,-(sp)\n"
"   lea     _pb_easy_tmpl(pc),a0\n"
"   addq.l  #1,(a0)\n"
"   tst.l   4(a0)\n"
"   beq.s   1f\n"
"   move.l  12(sp),-(sp)\n"         /* a1: the EasyStruct */
"   move.l  12(a0),a1\n"
"   jsr     (a1)\n"
"   addq.l  #4,sp\n"
"   lea     _pb_easy_tmpl(pc),a0\n"
"   subq.l  #1,(a0)\n"
"   move.l  d0,(sp)\n"              /* the handler's answer: the saved d0 */
"   movem.l (sp)+,d0-d1/a0-a1\n"
"   rts\n"
"1: subq.l  #1,(a0)\n"
"   movem.l (sp)+,d0-d1/a0-a1\n"
"   move.l  _pb_easy_tmpl+8(pc),-(sp)\n"
"   rts\n"
"_pb_easy_end:\n"
);
PB_DECLARE(easy);
#endif

#ifdef PB_THRU
/* A patch whose real code stays in the program (the stub of inject: it
   decides in C, calls the old function with the registers it got): the
   block holds only this trampoline.  While `active` it calls the stub of
   the program (head field `handler`) with its own return address on the
   stack, so the old function's return comes back here and the call is
   counted until it has left the stub's code; the stub must behave as a
   called function (all of inject's do: they end in the old function or
   an rts).  Once `active` is clear it jumps to the old function without
   touching the program.  All registers are kept (a0 is saved on the
   stack around the count); only the condition codes change. */
__asm__(
PB_ASM_HEAD(thru, "")
"   move.l  a0,-(sp)\n"
"   lea     _pb_thru_tmpl(pc),a0\n"
"   addq.l  #1,(a0)\n"
"   tst.l   4(a0)\n"
"   beq.s   1f\n"
"   move.l  (sp)+,a0\n"
"   pea     2f(pc)\n"
"   move.l  _pb_thru_tmpl+12(pc),-(sp)\n"
"   rts\n"
"2: move.l  a0,-(sp)\n"
"   lea     _pb_thru_tmpl(pc),a0\n"
"   subq.l  #1,(a0)\n"
"   move.l  (sp)+,a0\n"
"   rts\n"
"1: subq.l  #1,(a0)\n"
"   move.l  (sp)+,a0\n"
"   move.l  _pb_thru_tmpl+8(pc),-(sp)\n"
"   rts\n"
"_pb_thru_end:\n"
);
PB_DECLARE(thru);
#endif

/* The patch of vector lvo of lib from the template NAME: the block (never
   freed), or NULL if there is no memory or the template's data is not
   datasz bytes (struct and asm differ).  handler: see above. */
#define PB_INSTALL(lib, lvo, name, datasz, handler) \
    pb_install(lib, lvo, pb_##name##_tmpl, pb_##name##_entry, pb_##name##_end, datasz, (APTR)(handler))
static struct pbhead *pb_install(struct Library *lib, WORD lvo, const UBYTE *tmpl,
                                 const UBYTE *entry, const UBYTE *end, ULONG datasz,
                                 APTR handler)
{
    struct pbhead *b;

    if ((ULONG)(entry - tmpl) != datasz) return NULL;
    b = AllocMem(end - tmpl, MEMF_CHIP | MEMF_PUBLIC | MEMF_CLEAR);   /* never freed */
    if (!b) return NULL;
    CopyMem((APTR)tmpl, b, end - tmpl);
    b->active = 1;
    b->handler = handler;
    Forbid();
    b->old = SetFunction(lib, lvo, (APTR)((UBYTE *)b + datasz));
    if (SysBase->LibNode.lib_Version >= 37) CacheClearU();
    Permit();
    return b;
}

/* Remove the patch: the stub passes on to the old function from now on,
   the vector is restored (if a later patch sits on top of ours, theirs
   stays in place and ours remains in their chain), then wait (up to
   10 s, wait = 0: not at all, for the tests) for the calls inside.
   Returns the count of calls still inside. */
#define PB_REMOVE(b, lib, lvo, name, wait) \
    pb_remove(b, lib, lvo, pb_##name##_tmpl, pb_##name##_entry, wait)
static LONG pb_remove(struct pbhead *b, struct Library *lib, WORD lvo, const UBYTE *tmpl,
                      const UBYTE *entry, int wait)
{
    APTR mine = (APTR)((UBYTE *)b + (entry - tmpl)), cur;
    int n;

    Forbid();
    b->active = 0;
    cur = SetFunction(lib, lvo, b->old);
    if (cur != mine) SetFunction(lib, lvo, cur);
    if (SysBase->LibNode.lib_Version >= 37) CacheClearU();
    Permit();
    for (n = 0; wait && b->count && n < 500; n++) Delay(1);
    return b->count;
}
#endif
