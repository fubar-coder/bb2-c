/*
 * blitzexec.h - exec calls from library code.  There is no C startup in a
 * library, so no SysBase variable: like the originals (MOVE.l 4,a6) the
 * base is read from address 4 at every call.
 */
#ifndef BLITZEXEC_H
#define BLITZEXEC_H

#include <exec/io.h>
#include <exec/memory.h>
#include <exec/ports.h>
#define __NOLIBBASE__
#include <proto/exec.h>
#undef __NOLIBBASE__
#define SysBase (*(struct ExecBase **)4L)

/* BL_FREESIGNAL(sig) - FreeSignal with the signal number zero extended,
   like the originals (MOVEQ #0,d0:MOVE.b sigbit,d0:JSR _FreeSignal(a6)).
   The NDK inline takes a BYTE and would pass a bit number $FF as -1.
   `sig` is passed as its (unsigned) value: a UBYTE mp_SigBit, or a UWORD
   for MOVE.w (brexxlib mybit = $FFFF).  Needs blitzlib.h (BLITZ_SYSJSR1). */
#define BL_FREESIGNAL(sig) ((void)BLITZ_SYSJSR1(SysBase, -336, (sig)))

/* ------------------------------------ exec V36 calls built with V33 ones

   Exec V36's CreateMsgPort/DeleteMsgPort, CreateIORequest/DeleteIORequest
   and AllocVec/FreeVec do not exist on Kickstart 1.x; these helpers do
   the same with exec V33 functions (AllocMem, FreeMem, AllocSignal,
   FreeSignal, FindTask), one path on every Kickstart.  They are static
   inline and always inlined (the libraries are built without GCC's own
   inlining of small functions, which would leave each an out-of-line copy
   or not, by its heuristics): a library that calls one from several
   places and wants a single copy wraps it in a static function of its
   own (ricompactdisclib's and ritrackdisklib's newport). */
#define BL_EXEC_INLINE static inline __attribute__((always_inline))

/* bl_newport: a signal port of the calling task (as CreateMsgPort): the
   MsgPort cleared, a signal bit, PA_SIGNAL, an empty message list; NULL
   without memory or a free signal */
BL_EXEC_INLINE struct MsgPort *bl_newport(void)
{
    struct MsgPort *mp = AllocMem(sizeof *mp, MEMF_PUBLIC | MEMF_CLEAR);
    BYTE sig;

    if (!mp)
        return NULL;
    sig = AllocSignal(-1);
    if (sig < 0) {
        FreeMem(mp, sizeof *mp);
        return NULL;
    }
    mp->mp_Node.ln_Type = NT_MSGPORT;
    mp->mp_Flags = PA_SIGNAL;
    mp->mp_SigBit = sig;
    mp->mp_SigTask = FindTask(NULL);
    mp->mp_MsgList.lh_Head = (struct Node *)&mp->mp_MsgList.lh_Tail;
    mp->mp_MsgList.lh_TailPred = (struct Node *)&mp->mp_MsgList.lh_Head;
    return mp;
}

/* bl_freeport: frees the signal bit and the port of bl_newport() */
BL_EXEC_INLINE void bl_freeport(struct MsgPort *mp)
{
    FreeSignal(mp->mp_SigBit);
    FreeMem(mp, sizeof *mp);
}

/* bl_newreq: a cleared request of `size` bytes with mn_ReplyPort `port`
   and mn_Length set (as CreateIORequest) and NT_REPLYMSG: CheckIO takes a
   request that was never sent for done, as one of CreateIORequest; NULL
   without memory */
BL_EXEC_INLINE void *bl_newreq(struct MsgPort *port, ULONG size)
{
    struct IORequest *req = AllocMem(size, MEMF_PUBLIC | MEMF_CLEAR);

    if (req) {
        req->io_Message.mn_Node.ln_Type = NT_REPLYMSG;
        req->io_Message.mn_ReplyPort = port;
        req->io_Message.mn_Length = size;
    }
    return req;
}

/* bl_freereq: frees a request of bl_newreq() of `size` bytes; NULL is
   ignored */
BL_EXEC_INLINE void bl_freereq(void *req, ULONG size)
{
    if (req)
        FreeMem(req, size);
}

/* bl_allocvec: a block of `size` bytes in AllocVec's layout: AllocMem of
   size + 4 bytes, the whole size in the first long, the block behind it.
   exec sees the sizes AllocVec would allocate, and a block of it may be
   freed by exec's FreeVec on V36+ (and the other way round).  NULL
   without memory */
BL_EXEC_INLINE void *bl_allocvec(ULONG size, ULONG flags)
{
    ULONG *p;

    size += 4;
    p = AllocMem(size, flags);
    if (!p)
        return NULL;
    *p = size;
    return p + 1;
}

/* bl_freevec: frees a block of bl_allocvec() (or AllocVec); NULL is
   ignored, as by FreeVec */
BL_EXEC_INLINE void bl_freevec(void *block)
{
    ULONG *p = block;

    if (p) {
        p--;
        FreeMem(p, *p);
    }
}

#endif
