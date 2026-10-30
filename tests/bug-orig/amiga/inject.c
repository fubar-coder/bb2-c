/*
 * inject - run a command in this process with faults injected, for the
 * bug-orig tests (tests/bug-orig, pass file keys wrap= for the compiler
 * and runwrap= for the programs): conditions the original reacts to
 * wrongly but that a normal system never shows - no free signal, an
 * allocation that fails, a read error.
 *
 * usage: inject [OPTION...] COMMAND [ARGS]
 *   signals                every free signal bit of the process allocated
 *                          before COMMAND starts (AllocSignal(-1) fails)
 *   allocfail=MIN-MAX[/chip|/f=FLAGS][/COUNT][+SKIP]
 *                          AllocMem of MIN..MAX bytes (with /chip: only
 *                          with MEMF_CHIP, with /f=FLAGS: only with exactly
 *                          these flags, hex) returns 0, the first COUNT
 *                          times (default every time); with +SKIP only
 *                          after SKIP such allocations went through
 *   readfail=NAME/N        the N-th Read on a file opened under a name that
 *                          ends in NAME (case ignored) returns -1
 *   writefail=NAME/N       the same for Write
 *   alerts                 DisplayAlert returns FALSE at once (as if the
 *                          right mouse button was pressed: a recoverable
 *                          alert does not wait), its text is written
 *   wbfrontfail            WBenchToFront returns FALSE (as with the
 *                          Workbench closed)
 *   nullnode=NUM/NAME      while COMMAND runs, the long word at address 0
 *                          (normally 0) points to a node {next 0, word NUM
 *                          (hex), NAME}: a list walked through a null
 *                          pointer (head at address 0) finds it; restored
 *                          afterwards
 *   examinefail=NAME       Examine of a lock on a name that ends in NAME
 *                          returns DOSFALSE
 *   openlibfail=NAME       OpenLibrary/OldOpenLibrary of a library whose
 *                          name ends in NAME (case ignored) returns 0
 *   opendevfail=NAME       OpenDevice of a device whose name ends in NAME
 *                          returns IOERR_OPENFAIL (io_Error set, io_Device
 *                          0)
 *   createprocfail         CreateProc returns 0 (no process: as without
 *                          memory for it or its stack)
 *   track                  the process's AllocMem/FreeMem are recorded; what
 *                          is still allocated when COMMAND ends is reported
 *                          and every FreeMem whose size differs from the
 *                          allocation's
 *   pad                    every AllocMem of the process gets 64 bytes
 *                          more, zeroed, behind the bytes asked for (also
 *                          a LoadSeg in the process: a pointer just past
 *                          the end of a hunk finds zeros); FreeMem of the
 *                          allocation frees them too.  A FreeMem inside an
 *                          allocation of the process (or in its 64 bytes),
 *                          not at its start, is not done and reported once
 *                          (the count could depend on garbage) - e.g. a
 *                          list followed past the end of a hunk.  Other
 *                          memory (allocated before COMMAND, by another
 *                          task, ...) is freed as asked.  A FreeMem whose
 *                          64 bytes behind are no longer zero is reported
 *                          (written behind the allocation)
 * What COMMAND leaves behind:
 *   opencount=NAME         how often the process opened a library whose
 *                          name ends in NAME (OpenLibrary, OldOpenLibrary)
 *                          minus how often it closed it (CloseLibrary of
 *                          that base); up to 4 times.  Only this process
 *                          counts (lib_OpenCnt would count the background
 *                          tools of the Workbench too)
 *   cd[=PATH]              COMMAND runs with a lock of its own as current
 *                          directory (a DupLock of inject's or a lock on
 *                          PATH); afterwards: is it still the current
 *                          directory, did COMMAND UnLock it (inject's own
 *                          current directory is restored in any case)
 *   handles                after COMMAND: are the process's input and
 *                          output handles (pr_CIS, pr_COS) the ones it
 *                          started with
 *   inuse=PATH             after COMMAND: is PATH still in use (an
 *                          exclusive Lock fails with ERROR_OBJECT_IN_USE,
 *                          e.g. a lock left over); up to 4 times
 * Input while COMMAND runs (by a second process, the driver):
 *   click=TITLE/TEXT       GADGETUP on the gadget with the text TEXT
 *                          (GadgetText->IText) of the window with the title
 *                          TITLE: a synthetic IntuiMessage put into the
 *                          window's UserPort, as reqclose does
 *                          (tests/corpus/amiga/reqclose.h); the requester
 *                          replies to the driver.  Up to 8, sent in this
 *                          order, each after the reply of the one before
 *                          (the first 0.5 s after its window appeared,
 *                          the others 0.2 s after the reply)
 *   doslocked              the clicks are sent while the driver holds the
 *                          DOS list locked for writing (LockDosList); a
 *                          click not answered within 1 s ends the lock and
 *                          the clicks behind it are sent without it (a
 *                          program that locks the DOS list waits for it,
 *                          one that walks it without a lock does not)
 *   cacheflush=NAME        CreateProc of a process whose name contains
 *                          NAME: did the calling task call CacheClearU or
 *                          CacheClearE after its last AllocMem (and since
 *                          CreateProc made it)?
 *                          Code written into memory and started without a
 *                          flush may run stale on a 68020+ with caches
 *   undobuf                before every click (after the first) the
 *                          UndoBuffer pointers of the string gadgets of the
 *                          window are compared with those before the click
 *                          ahead of it: did that click change them (e.g.
 *                          a requester that clears a word of a StringInfo)
 *   keys=TITLE/CODE@SECS[,CODE@SECS]...   (in TITLE of click= and keys=,
 *                          '_' also matches a blank)
 *                          an input handler (priority 100, before
 *                          Intuition and the handlers of the program)
 *                          stamps every input event with a clock of its
 *                          own instead of the real time: 1000 s, standing
 *                          still.  0.5 s after a window TITLE appeared, its
 *                          next call puts RAWKEY events (CODE hex, up to 8)
 *                          in front of the incoming events, in one list,
 *                          each stamped with the clock + its SECS; then the
 *                          clock stands at the last of them.  So a handler
 *                          behind it sees several events at controlled
 *                          times in one call (input.device's IND_WRITEEVENT
 *                          writes one event per call, with the real time)
 * COMMAND is loaded with LoadSeg and run with RunCommand in this process
 * (the signals and the recorded allocations are the process's); the
 * patches are system-wide but act only for this process.  Behind COMMAND
 * it writes (lines "inject: ..."): how often each fault was injected,
 * rc=N of COMMAND, with track the allocations left (sorted by size:
 * the addresses differ between runs), with pad the allocations written
 * behind ("pad: written behind an allocation of N bytes"), then
 *   opencount: NAME +N           opened minus closed
 *   current dir at end: start | other
 *   current dir: the start lock was unlocked   (only if so)
 *   in use: PATH | not in use: PATH | missing: PATH
 *   click TITLE/TEXT: answered | answered after the DOS list was unlocked
 *                | not answered | no gadget | no window | not sent
 *   click TITLE/TEXT: UndoBuffer of the string gadgets kept | changed
 *                (undobuf; not for the last click)
 *   keys: sent | keys: no window | keys: not sent
 *   CreateProc NAME: caches flushed before | caches not flushed before
 *                | not called          (cacheflush)
 */
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <exec/errors.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <devices/input.h>
#include <devices/inputevent.h>
#include <intuition/intuitionbase.h>
#include <intuition/screens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define LVO_ALLOCMEM (-198)
#define LVO_FREEMEM  (-210)
#define LVO_OPEN     (-30)
#define LVO_READ     (-42)
#define LVO_WRITE    (-48)
#define LVO_DISPLAYALERT (-90)
#define LVO_WBENCHTOFRONT (-342)
#define LVO_LOCK     (-84)
#define LVO_UNLOCK   (-90)
#define LVO_EXAMINE  (-102)
#define LVO_CREATEPROC (-138)
#define LVO_OLDOPENLIBRARY (-408)
#define LVO_CLOSELIBRARY   (-414)
#define LVO_OPENLIBRARY    (-552)
#define LVO_CACHECLEARU    (-636)
#define LVO_CACHECLEARE    (-642)
#define LVO_OPENDEVICE     (-444)

struct IntuitionBase *IntuitionBase;

static BPTR out;
static struct Task *me;
static volatile int active;             /* COMMAND runs */

static void put(const char *s) { Write(out, (APTR)s, strlen(s)); }

/* -------------------------------------------------------------- options */

static int o_signals, o_track, o_pad, o_allocfail, o_createprocfail, o_alerts;
static int o_wbfrontfail, o_handles;
static ULONG nn_num;
static char nn_name[40];
static ULONG wbf_done;
static ULONG af_min, af_max, af_chip, af_count, af_done, af_skip, af_seen, af_flags;
static int af_flags_set;
static char rf_name[108], wf_name[108], ef_name[108], olf_name[108], odf_name[108];
static ULONG rf_n, rf_done, wf_n, wf_done, ef_done, olf_done, cpf_done, alerts_done, odf_done;

/* the patches: asm entry, C decision (only for this process while COMMAND
   runs); the original is called with the registers it got */
APTR old_allocmem, old_freemem, old_open, old_read, old_lock, old_examine;
APTR old_oldopenlib, old_openlib, old_createproc, old_closelib, old_unlock;
APTR old_cacheclearu, old_cachecleare;
void cacheclearu_patch(void), cachecleare_patch(void);
APTR old_write, old_alert, old_opendev, old_wbfront;
void write_patch(void), alert_patch(void), opendev_patch(void), wbfront_patch(void);
void allocmem_patch(void), freemem_patch(void), open_patch(void), read_patch(void);
void lock_patch(void), examine_patch(void), oldopenlib_patch(void), openlib_patch(void);
void createproc_patch(void), closelib_patch(void), unlock_patch(void), keys_stub(void);

__asm__(
"   .text\n"
"   .even\n"
/* AllocMem(d0 size, d1 flags): alloc_pre != 0 -> 0, else the original and
   alloc_post(result, size, flags) */
"_allocmem_patch:\n"
"   movem.l d0-d1/a0-a1,-(sp)\n"
"   move.l  d1,-(sp)\n"
"   move.l  d0,-(sp)\n"
"   jsr     _alloc_pre\n"
"   addq.l  #8,sp\n"
"   tst.l   d0\n"
"   movem.l (sp)+,d0-d1/a0-a1\n"     /* movem leaves the flags */
"   bne.s   1f\n"
"   movem.l d0-d1,-(sp)\n"
"   movem.l d1/a0-a1,-(sp)\n"
"   move.l  d0,-(sp)\n"
"   jsr     _alloc_size\n"            /* d0 = the size allocated */
"   addq.l  #4,sp\n"
"   movem.l (sp)+,d1/a0-a1\n"
"   move.l  _old_allocmem,a0\n"
"   jsr     (a0)\n"
"   move.l  d0,-(sp)\n"
"   jsr     _alloc_post\n"
"   move.l  (sp)+,d0\n"
"   addq.l  #8,sp\n"
"   rts\n"
"1: moveq   #0,d0\n"
"   rts\n"
/* FreeMem(a1 memory, d0 size): d0 = free_pre(memory, size), the original
   with that size (0: not done) */
"   .even\n"
"_freemem_patch:\n"
"   movem.l d1/a0-a1,-(sp)\n"
"   move.l  d0,-(sp)\n"
"   move.l  a1,-(sp)\n"
"   jsr     _free_pre\n"
"   addq.l  #8,sp\n"
"   movem.l (sp)+,d1/a0-a1\n"
"   tst.l   d0\n"
"   beq.s   7f\n"
"   move.l  _old_freemem,-(sp)\n"
"7: rts\n"
/* Open(d1 name, d2 mode) / Lock(d1 name, d2 mode): the original, then
   named_post(result, name, which) */
"   .even\n"
"_open_patch:\n"
"   move.l  #0,-(sp)\n"
"   move.l  _old_open,a0\n"
"   bra.s   2f\n"
"   .even\n"
"_lock_patch:\n"
"   move.l  #1,-(sp)\n"
"   move.l  _old_lock,a0\n"
"2: move.l  d1,-(sp)\n"
"   jsr     (a0)\n"
"   move.l  (sp)+,d1\n"
"   movem.l d0-d1/a0-a1,-(sp)\n"
"   move.l  16(sp),-(sp)\n"          /* which */
"   move.l  d1,-(sp)\n"
"   move.l  d0,-(sp)\n"
"   jsr     _named_post\n"
"   lea     12(sp),sp\n"
"   movem.l (sp)+,d0-d1/a0-a1\n"
"   addq.l  #4,sp\n"
"   rts\n"
/* Read(d1 file, d2 buffer, d3 length): read_pre != 0 -> -1 */
"   .even\n"
"_read_patch:\n"
"   movem.l d1/a0-a1,-(sp)\n"
"   move.l  d1,-(sp)\n"
"   jsr     _read_pre\n"
"   addq.l  #4,sp\n"
"   tst.l   d0\n"
"   movem.l (sp)+,d1/a0-a1\n"
"   bne.s   3f\n"
"   move.l  _old_read,-(sp)\n"
"   rts\n"
"3: moveq   #-1,d0\n"
"   rts\n"
/* Write(d1 file, d2 buffer, d3 length): write_pre != 0 -> -1 */
"   .even\n"
"_write_patch:\n"
"   movem.l d1/a0-a1,-(sp)\n"
"   move.l  d1,-(sp)\n"
"   jsr     _write_pre\n"
"   addq.l  #4,sp\n"
"   tst.l   d0\n"
"   movem.l (sp)+,d1/a0-a1\n"
"   bne.s   3b\n"
"   move.l  _old_write,-(sp)\n"
"   rts\n"
/* OpenDevice(a0 name, d0 unit, a1 ioreq, d1 flags): opendev_pre(name,
   ioreq) != 0 -> IOERR_OPENFAIL (set there) */
"   .even\n"
"_opendev_patch:\n"
"   movem.l d0-d1/a0-a1,-(sp)\n"
"   move.l  a1,-(sp)\n"
"   move.l  a0,-(sp)\n"
"   jsr     _opendev_pre\n"
"   addq.l  #8,sp\n"
"   tst.l   d0\n"
"   movem.l (sp)+,d0-d1/a0-a1\n"
"   bne.s   9f\n"
"   move.l  _old_opendev,-(sp)\n"
"   rts\n"
"9: moveq   #-1,d0\n"
"   rts\n"
/* WBenchToFront(): wbfront_pre != 0 -> FALSE (Z=1) */
"   .even\n"
"_wbfront_patch:\n"
"   movem.l d0-d1/a0-a1,-(sp)\n"
"   jsr     _wbfront_pre\n"
"   tst.l   d0\n"
"   movem.l (sp)+,d0-d1/a0-a1\n"
"   bne.s   10f\n"
"   move.l  _old_wbfront,-(sp)\n"
"   rts\n"
"10: moveq  #0,d0\n"
"   rts\n"
/* DisplayAlert(d0 number, a0 string, d1 height): alert_pre != 0 -> FALSE */
"   .even\n"
"_alert_patch:\n"
"   movem.l d0-d1/a0-a1,-(sp)\n"
"   move.l  a0,-(sp)\n"
"   jsr     _alert_pre\n"
"   addq.l  #4,sp\n"
"   tst.l   d0\n"
"   movem.l (sp)+,d0-d1/a0-a1\n"
"   bne.s   8f\n"
"   move.l  _old_alert,-(sp)\n"
"   rts\n"
"8: moveq   #0,d0\n"
"   rts\n"
/* Examine(d1 lock, d2 fib): examine_pre != 0 -> DOSFALSE */
"   .even\n"
"_examine_patch:\n"
"   movem.l d1/a0-a1,-(sp)\n"
"   move.l  d1,-(sp)\n"
"   jsr     _examine_pre\n"
"   addq.l  #4,sp\n"
"   tst.l   d0\n"
"   movem.l (sp)+,d1/a0-a1\n"
"   bne.s   4f\n"
"   move.l  _old_examine,-(sp)\n"
"   rts\n"
"4: moveq   #0,d0\n"
"   rts\n"
/* OldOpenLibrary(a1 name) / OpenLibrary(a1 name, d0 version):
   openlib_pre != 0 -> 0, else the original and openlib_post(result,
   name) */
"   .even\n"
"_oldopenlib_patch:\n"
"   move.l  _old_oldopenlib,a0\n"
"   bra.s   5f\n"
"   .even\n"
"_openlib_patch:\n"
"   move.l  _old_openlib,a0\n"
"5: movem.l d0/a0-a1,-(sp)\n"
"   move.l  a1,-(sp)\n"
"   jsr     _openlib_pre\n"
"   addq.l  #4,sp\n"
"   tst.l   d0\n"
"   movem.l (sp)+,d0/a0-a1\n"
"   bne.s   6f\n"
"   move.l  a1,-(sp)\n"
"   jsr     (a0)\n"                  /* the original */
"   move.l  (sp)+,a1\n"
"   move.l  d0,-(sp)\n"
"   move.l  a1,-(sp)\n"
"   move.l  d0,-(sp)\n"
"   jsr     _openlib_post\n"
"   addq.l  #8,sp\n"
"   move.l  (sp)+,d0\n"
"   rts\n"
"6: moveq   #0,d0\n"
"   rts\n"
/* CloseLibrary(a1 library): closelib_pre(library), the original */
"   .even\n"
"_closelib_patch:\n"
"   movem.l d0-d1/a0-a1,-(sp)\n"
"   move.l  a1,-(sp)\n"
"   jsr     _closelib_pre\n"
"   addq.l  #4,sp\n"
"   movem.l (sp)+,d0-d1/a0-a1\n"
"   move.l  _old_closelib,-(sp)\n"
"   rts\n"
/* UnLock(d1 lock): unlock_pre(lock), the original */
"   .even\n"
"_unlock_patch:\n"
"   movem.l d0-d1/a0-a1,-(sp)\n"
"   move.l  d1,-(sp)\n"
"   jsr     _unlock_pre\n"
"   addq.l  #4,sp\n"
"   movem.l (sp)+,d0-d1/a0-a1\n"
"   move.l  _old_unlock,-(sp)\n"
"   rts\n"
/* the input handler of keys=: a0 events -> d0 events */
"   .even\n"
"_keys_stub:\n"
"   move.l  a0,-(sp)\n"
"   jsr     _keys_handler\n"
"   addq.l  #4,sp\n"
"   rts\n"
/* CreateProc(d1 name, d2 pri, d3 seglist, d4 stack): createproc_pre(name)
   != 0 -> 0, else the original and createproc_post(result) */
"   .even\n"
"_createproc_patch:\n"
"   movem.l d0-d1/a0-a1,-(sp)\n"
"   move.l  d1,-(sp)\n"
"   jsr     _createproc_pre\n"
"   addq.l  #4,sp\n"
"   tst.l   d0\n"
"   movem.l (sp)+,d0-d1/a0-a1\n"
"   bne.s   7f\n"
"   move.l  _old_createproc,a0\n"
"   jsr     (a0)\n"
"   movem.l d0-d1/a0-a1,-(sp)\n"
"   move.l  d0,-(sp)\n"
"   jsr     _createproc_post\n"
"   addq.l  #4,sp\n"
"   movem.l (sp)+,d0-d1/a0-a1\n"
"   rts\n"
"7: moveq   #0,d0\n"
"   rts\n"
/* CacheClearU() / CacheClearE(a0, d0, d1): cacheclear_pre(), the
   original */
"   .even\n"
"_cacheclearu_patch:\n"
"   move.l  _old_cacheclearu,-(sp)\n"
"   bra.s   8f\n"
"   .even\n"
"_cachecleare_patch:\n"
"   move.l  _old_cachecleare,-(sp)\n"
"8: tst.l   (sp)\n"                 /* SetFunction flushes the caches */
"   bne.s   9f\n"                   /* itself, before the old vector */
"   addq.l  #4,sp\n"                /* is stored: nothing to call */
"   rts\n"
"9: movem.l d0-d1/a0-a1,-(sp)\n"
"   jsr     _cacheclear_pre\n"
"   movem.l (sp)+,d0-d1/a0-a1\n"
"   rts\n"
);

static int ours(void) { return active && SysBase->ThisTask == me; }

/* ----------------------------------------------------- allocations */

#define SLOTS 16384                     /* power of 2 */
static struct { ULONG addr, size, flags; } slot[SLOTS];
static ULONG nalloc, tbl_full;
static ULONG nwrong, wrong_alloc[16], wrong_free[16];   /* FreeMem of another size */
#define TOMB 1UL                        /* removed (odd: no allocation) */
#define PAD 64                          /* pad: the bytes behind */
static ULONG bad_free;                  /* pad: a FreeMem not done */
static ULONG npadw, padw_size[16];      /* pad: written behind */

static ULONG hash(ULONG a) { return ((a >> 3) * 2654435761UL) >> 18 & (SLOTS - 1); }

void cf_alloc(void);

ULONG alloc_pre(ULONG size, ULONG flags)
{
    cf_alloc();
    if (!o_allocfail || !ours() || size < af_min || size > af_max
        || (af_chip && !(flags & MEMF_CHIP)) || (af_flags_set && flags != af_flags)
        || (af_count && af_done >= af_count))
        return 0;
    if (af_seen < af_skip) {
        af_seen++;
        return 0;
    }
    af_done++;
    return 1;
}

/* the size AllocMem allocates (pad: 64 bytes more) */
ULONG alloc_size(ULONG size)
{
    return o_pad && ours() ? size + PAD : size;
}

void alloc_post(ULONG mem, ULONG size, ULONG flags)
{
    ULONG h, i;

    if ((!o_track && !o_pad) || !mem || !ours())
        return;
    if (o_pad)
        memset((UBYTE *)mem + size, 0, PAD);
    for (h = hash(mem), i = 0; i < SLOTS; i++, h = (h + 1) & (SLOTS - 1))
        if (slot[h].addr == 0 || slot[h].addr == TOMB) {
            slot[h].addr = mem;
            slot[h].size = size;
            slot[h].flags = flags;
            nalloc++;
            return;
        }
    tbl_full++;
}

/* the size FreeMem frees (pad: with the 64 bytes; 0: not done) */
ULONG free_pre(ULONG mem, ULONG size)
{
    ULONG h, i;

    if ((!o_track && !o_pad) || !mem || !ours())
        return size;
    for (h = hash(mem), i = 0; i < SLOTS && slot[h].addr; i++, h = (h + 1) & (SLOTS - 1))
        if (slot[h].addr == mem) {
            if (slot[h].size != size && nwrong < 16) {
                wrong_alloc[nwrong] = slot[h].size;
                wrong_free[nwrong++] = size;
            }
            if (o_pad) {
                const UBYTE *p = (const UBYTE *)mem + slot[h].size;

                for (i = 0; i < PAD && !p[i]; i++)
                    ;
                if (i < PAD && npadw < 16)
                    padw_size[npadw++] = slot[h].size;
            }
            slot[h].addr = TOMB;
            nalloc--;
            return o_pad ? size + PAD : size;
        }
    /* not the start of an allocation of the process: inside one or in
       its pad not done, else (memory from elsewhere) as it is */
    if (o_pad)
        for (i = 0; i < SLOTS; i++)
            if (slot[i].addr > TOMB && mem > slot[i].addr && mem < slot[i].addr + slot[i].size + PAD) {
                bad_free = 1;
                return 0;
            }
    return size;
}

/* ---------------------------------------------------- dos.library */

#define NAMED 8
static ULONG rf_fh[NAMED], rf_reads[NAMED], ef_lock[NAMED], wf_fh[NAMED], wf_writes[NAMED];

static int lower(int c) { return c >= 'A' && c <= 'Z' ? c + 32 : c; }

/* does name end in suffix (case ignored) */
static int ends(const char *name, const char *suffix)
{
    ULONG n = strlen(name), k = strlen(suffix), i;

    if (!k || k > n)
        return 0;
    for (i = 0; i < k; i++)
        if (lower(name[n - k + i]) != lower(suffix[i]))
            return 0;
    return 1;
}

void named_post(ULONG result, const char *name, ULONG which)
{
    ULONG *list = which ? ef_lock : rf_fh;
    const char *want = which ? ef_name : rf_name;
    int i;

    if (!which && result && name && wf_name[0] && ours() && ends(name, wf_name))
        for (i = 0; i < NAMED; i++)
            if (!wf_fh[i]) {
                wf_fh[i] = result;
                wf_writes[i] = 0;
                break;
            }
    if (!result || !name || !*want || !ours() || !ends(name, want))
        return;
    for (i = 0; i < NAMED; i++)
        if (!list[i]) {
            list[i] = result;
            if (!which)
                rf_reads[i] = 0;
            return;
        }
}

ULONG read_pre(ULONG fh)
{
    int i;

    if (!rf_name[0] || !ours() || !fh)
        return 0;
    for (i = 0; i < NAMED; i++)
        if (rf_fh[i] == fh) {
            if (++rf_reads[i] != rf_n)
                return 0;
            rf_fh[i] = 0;               /* once per open */
            rf_done++;
            return 1;
        }
    return 0;
}

ULONG write_pre(ULONG fh)
{
    int i;

    if (!wf_name[0] || !ours() || !fh)
        return 0;
    for (i = 0; i < NAMED; i++)
        if (wf_fh[i] == fh) {
            if (++wf_writes[i] != wf_n)
                return 0;
            wf_fh[i] = 0;               /* once per open */
            wf_done++;
            return 1;
        }
    return 0;
}

/* DisplayAlert: the text (behind the x position and y byte of each line:
   lines of 0-terminated text with a continuation byte) */
ULONG alert_pre(const UBYTE *s)
{
    if (!o_alerts || !ours())
        return 0;
    alerts_done++;
    put("inject: alert:");
    while (s) {
        s += 3;
        put(" ");
        put((const char *)s);
        s += strlen((const char *)s) + 1;
        if (!*s)
            break;
        s++;
    }
    put("\n");
    return 1;
}

ULONG wbfront_pre(void)
{
    if (!o_wbfrontfail || !ours())
        return 0;
    wbf_done++;
    return 1;
}

ULONG opendev_pre(const char *name, struct IORequest *io)
{
    if (!odf_name[0] || !ours() || !name || !ends(name, odf_name))
        return 0;
    odf_done++;
    io->io_Device = NULL;
    io->io_Error = IOERR_OPENFAIL;
    return 1;
}

ULONG examine_pre(ULONG lock)
{
    int i;

    if (!ef_name[0] || !ours() || !lock)
        return 0;
    for (i = 0; i < NAMED; i++)
        if (ef_lock[i] == lock) {
            ef_lock[i] = 0;             /* once per lock */
            ef_done++;
            return 1;
        }
    return 0;
}

/* ---------------------------------------------------- exec libraries */

#define MAXOC 4
static char oc_name[MAXOC][40];
static ULONG oc_base[MAXOC];
static LONG oc_count[MAXOC];
static int noc;
static ULONG ol_depth;                  /* OldOpenLibrary calls OpenLibrary */

ULONG openlib_pre(const char *name)
{
    if (!ours())
        return 0;
    if (!olf_name[0] || !name || !ends(name, olf_name)) {
        ol_depth++;
        return 0;
    }
    olf_done++;
    return 1;
}

/* cacheflush=NAME: the tasks that called CacheClearU/CacheClearE since
   their last AllocMem (any task while COMMAND runs; a task leaves the list
   with its next AllocMem - new memory, e.g. for code to be written - and
   when CreateProc makes a new process at its address) */
#define CF_TASKS 16
static struct Task *volatile cf_task[CF_TASKS];
static volatile int cf_next, cf_result;
static int o_cacheflush;
static char cf_name[40];
enum { CF_NOTCALLED, CF_FLUSHED, CF_NOTFLUSHED };

void cacheclear_pre(void)
{
    struct Task *t = SysBase->ThisTask;
    int i;

    if (!o_cacheflush || !active)
        return;
    for (i = 0; i < CF_TASKS; i++)
        if (cf_task[i] == t)
            return;
    cf_task[cf_next++ % CF_TASKS] = t;
}

void cf_alloc(void)
{
    struct Task *t = SysBase->ThisTask;
    int i;

    if (!o_cacheflush || !active)
        return;
    for (i = 0; i < CF_TASKS; i++)
        if (cf_task[i] == t)
            cf_task[i] = NULL;
}

void createproc_post(struct MsgPort *port)
{
    struct Task *t;
    int i;

    if (!o_cacheflush || !port)
        return;
    t = (struct Task *)((UBYTE *)port - offsetof(struct Process, pr_MsgPort));
    for (i = 0; i < CF_TASKS; i++)
        if (cf_task[i] == t)
            cf_task[i] = NULL;
}

ULONG createproc_pre(const char *name)
{
    if (o_cacheflush && active && name && strstr(name, cf_name)) {
        struct Task *t = SysBase->ThisTask;
        int i;

        cf_result = CF_NOTFLUSHED;
        for (i = 0; i < CF_TASKS; i++)
            if (cf_task[i] == t)
                cf_result = CF_FLUSHED;
    }
    if (!o_createprocfail || !ours())
        return 0;
    cpf_done++;
    return 1;
}

void openlib_post(ULONG base, const char *name)
{
    int i;

    if (!ours() || !ol_depth || --ol_depth || !base || !name)
        return;
    for (i = 0; i < noc; i++)
        if (ends(name, oc_name[i])) {
            oc_base[i] = base;
            oc_count[i]++;
        }
}

void closelib_pre(ULONG base)
{
    int i;

    if (!ours() || !base)
        return;
    for (i = 0; i < noc; i++)
        if (base == oc_base[i])
            oc_count[i]--;
}

/* ------------------------------------------------- current directory */

static int o_cd;
static char cd_path[108];
static BPTR cd_lock;                    /* COMMAND's current dir at its start */
static volatile int cd_unlocked;
static int cd_end_start;                /* still the current dir at the end */

void unlock_pre(ULONG lock)
{
    if (ours() && cd_lock && lock == (ULONG)cd_lock)
        cd_unlocked = 1;
}

#define MAXINUSE 4
static const char *inuse[MAXINUSE];
static int ninuse;

/* --------------------------------------------- input: the driver */

#define MAXCLICK 8
static struct {
    char title[40], text[40];
    int result;
    int undo;                           /* undobuf: UB_... */
} click[MAXCLICK];
static int nclick, o_doslocked, o_undobuf;
enum { UB_NOTCHECKED, UB_KEPT, UB_CHANGED };

static struct Window *findwin(const char *title);

/* undobuf: the UndoBuffer pointers of the string gadgets of a window */
#define MAXSTR 8
static APTR undo_before[MAXSTR];

static int undobufs(const char *title, APTR *u)
{
    struct Window *w;
    struct Gadget *g;
    ULONG ib = LockIBase(0);
    int n = 0;

    w = findwin(title);
    if (w)
        for (g = w->FirstGadget; g && n < MAXSTR; g = g->NextGadget)
            if ((g->GadgetType & GTYP_GTYPEMASK) == GTYP_STRGADGET && g->SpecialInfo)
                u[n++] = ((struct StringInfo *)g->SpecialInfo)->UndoBuffer;
    UnlockIBase(ib);
    return w ? n : -1;
}
enum { CL_NOTSENT, CL_ANSWERED, CL_LATE, CL_NOANSWER, CL_NOGADGET, CL_NOWINDOW };
static const char *const cl_text[] = {
    "not sent", "answered", "answered after the DOS list was unlocked",
    "not answered", "no gadget", "no window"
};

#define MAXKEYS 8
static char k_title[40];
static UWORD k_code[MAXKEYS];
static ULONG k_secs[MAXKEYS];
static int nkeys;
static struct InputEvent k_ev[MAXKEYS];
static volatile ULONG k_clock = 1000;
static volatile int k_fire, k_sent;
enum { KS_NOTSENT, KS_SENT, KS_NOWINDOW };
static int k_result;

static volatile int cmd_done, drv_done;
static BPTR h_cis, h_cos;              /* handles: at the start */
static ULONG nn_old;                    /* nullnode: the long word at 0 */
static int h_kept;

/* the input handler (input.device's task) */
struct InputEvent *keys_handler(struct InputEvent *ev)
{
    struct InputEvent *e, *head = ev;
    int i;

    if (k_fire) {
        for (i = 0; i < nkeys; i++) {
            k_ev[i].ie_NextEvent = i + 1 < nkeys ? &k_ev[i + 1] : ev;
            k_ev[i].ie_Class = IECLASS_RAWKEY;
            k_ev[i].ie_Code = k_code[i];
            k_ev[i].ie_TimeStamp.tv_secs = k_clock + k_secs[i];
            k_ev[i].ie_TimeStamp.tv_micro = 0;
        }
        k_clock += k_secs[nkeys - 1];
        head = k_ev;
        k_fire = 0;
        k_sent = 1;
    }
    for (e = ev; e; e = e->ie_NextEvent) {
        e->ie_TimeStamp.tv_secs = k_clock;
        e->ie_TimeStamp.tv_micro = 0;
    }
    return head;
}

/* window title t = want; a '_' in want also matches a blank (the
   arguments cannot carry blanks: no quotes in the argument parsing) */
static int sametitle(const char *t, const char *want)
{
    for (; *t && *want; t++, want++)
        if (*t != *want && !(*want == '_' && *t == ' '))
            return 0;
    return *t == *want;
}

/* the window with that title (IBase locked) */
static struct Window *findwin(const char *title)
{
    struct Screen *s;
    struct Window *w;

    for (s = IntuitionBase->FirstScreen; s; s = s->NextScreen)
        for (w = s->FirstWindow; w; w = w->NextWindow)
            if (w->Title && sametitle((const char *)w->Title, title))
                return w;
    return NULL;
}

/* wait up to 20 s (or the end of COMMAND) for the window: 0 = none */
static int waitwin(const char *title)
{
    int t;

    for (t = 0; t < 500 && !cmd_done; t++) {
        ULONG ib = LockIBase(0);
        struct Window *w = findwin(title);

        UnlockIBase(ib);
        if (w)
            return 1;
        Delay(2);
    }
    return 0;
}

/* GADGETUP on the gadget: CL_ANSWERED when sent (the reply is waited for
   by the caller), else CL_NOWINDOW / CL_NOGADGET */
static int sendclick(int n, struct IntuiMessage *m, struct MsgPort *reply)
{
    struct Window *w;
    struct Gadget *g = NULL;
    struct MsgPort *port = NULL;
    ULONG ib;

    Forbid();
    ib = LockIBase(0);
    w = findwin(click[n].title);
    if (w) {
        port = w->UserPort;
        for (g = w->FirstGadget; g; g = g->NextGadget)
            if (g->GadgetText && g->GadgetText->IText
                && !strcmp((const char *)g->GadgetText->IText, click[n].text))
                break;
    }
    UnlockIBase(ib);
    if (!w || !port || !g) {
        Permit();
        return w && port ? CL_NOGADGET : CL_NOWINDOW;
    }
    memset(m, 0, sizeof *m);
    m->ExecMessage.mn_Node.ln_Type = NT_MESSAGE;
    m->ExecMessage.mn_ReplyPort = reply;
    m->ExecMessage.mn_Length = sizeof *m;
    m->Class = IDCMP_GADGETUP;
    m->IAddress = g;
    m->IDCMPWindow = w;
    PutMsg(port, &m->ExecMessage);
    Permit();
    return CL_ANSWERED;
}

static void driver_clicks(void)
{
    static struct IntuiMessage m;
    struct MsgPort *reply = CreateMsgPort();
    int n, t, locked = 0;

    if (!reply)
        return;
    for (n = 0; n < nclick; n++) {
        if (!waitwin(click[n].title)) {
            click[n].result = CL_NOWINDOW;
            break;
        }
        Delay(n ? 10 : 25);
        if (o_undobuf) {
            static APTR now[MAXSTR];
            int m = undobufs(click[n].title, now);

            if (n && m >= 0)
                click[n - 1].undo = memcmp(now, undo_before, sizeof now) ? UB_CHANGED : UB_KEPT;
            memcpy(undo_before, now, sizeof now);
        }
        if (o_doslocked && n == 0) {
            LockDosList(LDF_ALL | LDF_WRITE);
            locked = 1;
        }
        click[n].result = sendclick(n, &m, reply);
        if (click[n].result != CL_ANSWERED)
            break;
        for (t = 0; !GetMsg(reply); t++) {
            if (locked && t == 50) {
                UnLockDosList(LDF_ALL | LDF_WRITE);
                locked = 0;
                click[n].result = CL_LATE;
            }
            if (cmd_done && t >= 100) {         /* COMMAND ended: none */
                click[n].result = CL_NOANSWER;
                break;
            }
            Delay(1);
        }
        if (click[n].result == CL_NOANSWER)
            break;
    }
    if (locked)
        UnLockDosList(LDF_ALL | LDF_WRITE);
    if (n == nclick || click[n].result != CL_NOANSWER)
        DeleteMsgPort(reply);           /* (else a reply may still come) */
}

static void driver_keys(void)
{
    int t;

    if (!waitwin(k_title)) {
        k_result = KS_NOWINDOW;
        return;
    }
    Delay(25);
    k_fire = 1;
    for (t = 0; t < 250 && !k_sent; t++)
        Delay(1);
    k_result = k_sent ? KS_SENT : KS_NOTSENT;
}

static void driver(void)
{
    if (nclick)
        driver_clicks();
    if (nkeys)
        driver_keys();
    Forbid();
    drv_done = 1;
}

/* ------------------------------------------------------------ main */

static int number(const char **s, ULONG *v)
{
    const char *p = *s;

    *v = 0;
    if (*p < '0' || *p > '9')
        return 0;
    while (*p >= '0' && *p <= '9')
        *v = *v * 10 + (ULONG)(*p++ - '0');
    *s = p;
    return 1;
}

/* 1 = an option, 0 = the command, -1 = a bad option */
static int option(const char *a)
{
    const char *s;

    if (!strcmp(a, "signals"))
        return o_signals = 1;
    if (!strcmp(a, "track"))
        return o_track = 1;
    if (!strcmp(a, "pad"))
        return o_pad = 1;
    if (!strcmp(a, "createprocfail"))
        return o_createprocfail = 1;
    if (!strncmp(a, "allocfail=", 10)) {
        s = a + 10;
        if (!number(&s, &af_min) || *s++ != '-' || !number(&s, &af_max) || af_min > af_max)
            return -1;
        if (!strncmp(s, "/chip", 5)) {
            af_chip = 1;
            s += 5;
        } else if (!strncmp(s, "/f=", 3)) {
            int d, n = 0;

            for (s += 3, af_flags = 0; (d = *s >= '0' && *s <= '9' ? *s - '0'
                                          : *s >= 'a' && *s <= 'f' ? *s - 'a' + 10
                                          : *s >= 'A' && *s <= 'F' ? *s - 'A' + 10 : -1) >= 0
                                       && n < 8; s++, n++)
                af_flags = af_flags * 16 + (ULONG)d;
            if (!n)
                return -1;
            af_flags_set = 1;
        }
        if (*s == '/' && (s++, !number(&s, &af_count) || !af_count))
            return -1;
        if (*s == '+' && (s++, !number(&s, &af_skip)))
            return -1;
        return *s ? -1 : (o_allocfail = 1);
    }
    if (!strncmp(a, "readfail=", 9)) {
        const char *slash = strrchr(a, '/');

        if (!slash || slash == a + 9 || (ULONG)(slash - a - 9) >= sizeof rf_name)
            return -1;
        s = slash + 1;
        if (!number(&s, &rf_n) || *s || !rf_n)
            return -1;
        memcpy(rf_name, a + 9, (ULONG)(slash - a - 9));
        return 1;
    }
    if (!strncmp(a, "writefail=", 10)) {
        const char *slash = strrchr(a, '/');

        if (!slash || slash == a + 10 || (ULONG)(slash - a - 10) >= sizeof wf_name)
            return -1;
        s = slash + 1;
        if (!number(&s, &wf_n) || *s || !wf_n)
            return -1;
        memcpy(wf_name, a + 10, (ULONG)(slash - a - 10));
        return 1;
    }
    if (!strcmp(a, "alerts"))
        return o_alerts = 1;
    if (!strcmp(a, "wbfrontfail"))
        return o_wbfrontfail = 1;
    if (!strncmp(a, "nullnode=", 9)) {
        const char *slash = strchr(a + 9, '/');

        s = a + 9;
        if (!slash || slash == s || !slash[1] || strlen(slash + 1) >= sizeof nn_name)
            return -1;
        for (nn_num = 0; s < slash; s++) {
            int d = *s >= '0' && *s <= '9' ? *s - '0' : *s >= 'a' && *s <= 'f' ? *s - 'a' + 10
                    : *s >= 'A' && *s <= 'F' ? *s - 'A' + 10 : -1;

            if (d < 0 || nn_num > 0xfff)
                return -1;
            nn_num = nn_num * 16 + d;
        }
        strcpy(nn_name, slash + 1);
        return 1;
    }
    if (!strcmp(a, "handles"))
        return o_handles = 1;
    if (!strncmp(a, "opendevfail=", 12)) {
        if (!a[12] || strlen(a + 12) >= sizeof odf_name)
            return -1;
        strcpy(odf_name, a + 12);
        return 1;
    }
    if (!strncmp(a, "examinefail=", 12)) {
        if (!a[12] || strlen(a + 12) >= sizeof ef_name)
            return -1;
        strcpy(ef_name, a + 12);
        return 1;
    }
    if (!strncmp(a, "openlibfail=", 12)) {
        if (!a[12] || strlen(a + 12) >= sizeof olf_name)
            return -1;
        strcpy(olf_name, a + 12);
        return 1;
    }
    if (!strncmp(a, "opencount=", 10)) {
        if (!a[10] || strlen(a + 10) >= sizeof oc_name[0] || noc >= MAXOC)
            return -1;
        strcpy(oc_name[noc++], a + 10);
        return 1;
    }
    if (!strcmp(a, "cd"))
        return o_cd = 1;
    if (!strncmp(a, "cd=", 3)) {
        if (!a[3] || strlen(a + 3) >= sizeof cd_path)
            return -1;
        strcpy(cd_path, a + 3);
        return o_cd = 1;
    }
    if (!strncmp(a, "inuse=", 6)) {
        if (!a[6] || strlen(a + 6) > 100 || ninuse >= MAXINUSE)
            return -1;
        inuse[ninuse++] = a + 6;
        return 1;
    }
    if (!strcmp(a, "doslocked"))
        return o_doslocked = 1;
    if (!strcmp(a, "undobuf"))
        return o_undobuf = 1;
    if (!strncmp(a, "cacheflush=", 11)) {
        if (!a[11] || strlen(a + 11) >= sizeof cf_name)
            return -1;
        strcpy(cf_name, a + 11);
        return o_cacheflush = 1;
    }
    if (!strncmp(a, "click=", 6)) {
        const char *slash = strchr(a + 6, '/');

        if (!slash || slash == a + 6 || !slash[1] || nclick >= MAXCLICK
            || (ULONG)(slash - a - 6) >= sizeof click[0].title
            || strlen(slash + 1) >= sizeof click[0].text)
            return -1;
        memcpy(click[nclick].title, a + 6, (ULONG)(slash - a - 6));
        strcpy(click[nclick++].text, slash + 1);
        return 1;
    }
    if (!strncmp(a, "keys=", 5)) {
        const char *slash = strchr(a + 5, '/');

        if (nkeys || !slash || slash == a + 5 || (ULONG)(slash - a - 5) >= sizeof k_title)
            return -1;
        memcpy(k_title, a + 5, (ULONG)(slash - a - 5));
        s = slash;
        do {
            ULONG code = 0;
            int digits = 0;

            for (s++; ; s++, digits++)
                if (*s >= '0' && *s <= '9')
                    code = code * 16 + (ULONG)(*s - '0');
                else if (lower(*s) >= 'a' && lower(*s) <= 'f')
                    code = code * 16 + (ULONG)(lower(*s) - 'a' + 10);
                else
                    break;
            if (!digits || code > 0xffff || *s++ != '@' || nkeys >= MAXKEYS
                || !number(&s, &k_secs[nkeys]))
                return -1;
            k_code[nkeys++] = (UWORD)code;
        } while (*s == ',');
        return *s ? -1 : 1;
    }
    return 0;
}

static void patch(int on)
{
    struct Library *dos = (struct Library *)DOSBase, *ex = (struct Library *)SysBase;

    Forbid();
    if (on) {
        old_allocmem = SetFunction(ex, LVO_ALLOCMEM, (APTR)allocmem_patch);
        old_freemem = SetFunction(ex, LVO_FREEMEM, (APTR)freemem_patch);
        old_open = SetFunction(dos, LVO_OPEN, (APTR)open_patch);
        old_read = SetFunction(dos, LVO_READ, (APTR)read_patch);
        old_lock = SetFunction(dos, LVO_LOCK, (APTR)lock_patch);
        old_examine = SetFunction(dos, LVO_EXAMINE, (APTR)examine_patch);
        old_oldopenlib = SetFunction(ex, LVO_OLDOPENLIBRARY, (APTR)oldopenlib_patch);
        old_openlib = SetFunction(ex, LVO_OPENLIBRARY, (APTR)openlib_patch);
        old_createproc = SetFunction(dos, LVO_CREATEPROC, (APTR)createproc_patch);
        old_closelib = SetFunction(ex, LVO_CLOSELIBRARY, (APTR)closelib_patch);
        old_unlock = SetFunction(dos, LVO_UNLOCK, (APTR)unlock_patch);
        old_cacheclearu = SetFunction(ex, LVO_CACHECLEARU, (APTR)cacheclearu_patch);
        old_cachecleare = SetFunction(ex, LVO_CACHECLEARE, (APTR)cachecleare_patch);
        old_write = SetFunction(dos, LVO_WRITE, (APTR)write_patch);
        old_opendev = SetFunction(ex, LVO_OPENDEVICE, (APTR)opendev_patch);
        if (o_alerts)
            old_alert = SetFunction((struct Library *)IntuitionBase, LVO_DISPLAYALERT,
                                    (APTR)alert_patch);
        if (o_wbfrontfail)
            old_wbfront = SetFunction((struct Library *)IntuitionBase, LVO_WBENCHTOFRONT,
                                      (APTR)wbfront_patch);
    } else {
        SetFunction(ex, LVO_CACHECLEARU, old_cacheclearu);
        SetFunction(ex, LVO_CACHECLEARE, old_cachecleare);
        SetFunction(dos, LVO_WRITE, old_write);
        SetFunction(ex, LVO_OPENDEVICE, old_opendev);
        if (o_alerts)
            SetFunction((struct Library *)IntuitionBase, LVO_DISPLAYALERT, old_alert);
        if (o_wbfrontfail)
            SetFunction((struct Library *)IntuitionBase, LVO_WBENCHTOFRONT, old_wbfront);
        SetFunction(ex, LVO_CLOSELIBRARY, old_closelib);
        SetFunction(dos, LVO_UNLOCK, old_unlock);
        SetFunction(ex, LVO_ALLOCMEM, old_allocmem);
        SetFunction(ex, LVO_FREEMEM, old_freemem);
        SetFunction(dos, LVO_OPEN, old_open);
        SetFunction(dos, LVO_READ, old_read);
        SetFunction(dos, LVO_LOCK, old_lock);
        SetFunction(dos, LVO_EXAMINE, old_examine);
        SetFunction(ex, LVO_OLDOPENLIBRARY, old_oldopenlib);
        SetFunction(ex, LVO_OPENLIBRARY, old_openlib);
        SetFunction(dos, LVO_CREATEPROC, old_createproc);
    }
    CacheClearU();
    Permit();
}

/* what is left with track, sorted by size and flags */
static void report_track(void)
{
    static ULONG sz[64], fl[64];
    static char line[100];
    ULONG bytes = 0, n = 0, i, j, t;

    for (i = 0; i < SLOTS; i++)
        if (slot[i].addr > TOMB) {
            bytes += slot[i].size;
            if (n < 64) {
                sz[n] = slot[i].size;
                fl[n++] = slot[i].flags;
            }
        }
    for (i = 1; i < n; i++)
        for (j = i; j > 0 && (sz[j - 1] > sz[j] || (sz[j - 1] == sz[j] && fl[j - 1] > fl[j])); j--) {
            t = sz[j], sz[j] = sz[j - 1], sz[j - 1] = t;
            t = fl[j], fl[j] = fl[j - 1], fl[j - 1] = t;
        }
    sprintf(line, "inject: track: %lu allocations left, %lu bytes%s\n", nalloc, bytes,
            tbl_full ? " (table full: more not recorded)" : "");
    put(line);
    for (i = 0; i < n; i++) {
        sprintf(line, "inject:   %lu bytes, flags $%lx\n", sz[i], fl[i]);
        put(line);
    }
    for (i = 0; i < nwrong; i++) {
        sprintf(line, "inject: track: FreeMem of %lu bytes for an allocation of %lu bytes\n",
                wrong_free[i], wrong_alloc[i]);
        put(line);
    }
}

static const char usage[] =
    "usage: inject [OPTION...] COMMAND [ARGS]\n"
    "  run COMMAND in this process with faults injected (bug-orig tests)\n"
    "  options: signals allocfail=MIN-MAX[/chip|/f=FLAGS][/COUNT][+SKIP] readfail=NAME/N\n"
    "    writefail=NAME/N alerts wbfrontfail examinefail=NAME openlibfail=NAME\n"
    "    opendevfail=NAME createprocfail handles nullnode=NUM/NAME\n"
    "    track pad\n"
    "    opencount=NAME cd[=PATH] inuse=PATH click=TITLE/TEXT doslocked\n"
    "    undobuf cacheflush=NAME\n"
    "    keys=TITLE/CODE@SECS[,CODE@SECS]...  (described in inject.c)\n";

int main(int argc, char **argv)
{
    static char args[512], line[200];
    static struct Interrupt kint;
    struct CommandLineInterface *cli = Cli();
    ULONG stack = cli ? cli->cli_DefaultStack * 4 : 4096, sigs = 0;
    struct MsgPort *iport = NULL;
    struct IOStdReq *io = NULL;
    BPTR seg, old_cd = 0;
    LONG rc;
    int i, k, r = 0;

    if (argc == 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
        Write(Output(), (APTR)usage, sizeof usage - 1);
        return 0;
    }
    out = Output();
    me = FindTask(NULL);
    for (i = 1; i < argc && (r = option(argv[i])) != 0; i++)
        if (r < 0) {
            put("inject: bad option ");
            put(argv[i]);
            put("\n");
            return 20;
        }
    if (i >= argc) {
        put("usage: inject [signals] [allocfail=MIN-MAX[/chip|/f=FLAGS][/COUNT][+SKIP]]\n"
            "              [readfail=NAME/N] [writefail=NAME/N] [alerts] [wbfrontfail]\n"
            "              [handles] [nullnode=NUM/NAME]\n"
            "              [examinefail=NAME] [openlibfail=NAME] [opendevfail=NAME]\n"
            "              [createprocfail] [track] [pad]\n"
            "              [opencount=NAME]... [cd[=PATH]] [inuse=PATH]...\n"
            "              [click=TITLE/TEXT]... [doslocked] [undobuf] [cacheflush=NAME]\n"
            "              [keys=TITLE/CODE@SECS[,CODE@SECS]...] COMMAND [ARGS]\n");
        return 20;
    }
    for (k = i + 1; k < argc; k++) {
        if (strlen(args) + strlen(argv[k]) + 3 >= sizeof args)
            return 20;
        if (k > i + 1)
            strcat(args, " ");
        strcat(args, argv[k]);
    }
    strcat(args, "\n");
    if (!(seg = LoadSeg(argv[i]))) {
        put("inject: cannot load ");
        put(argv[i]);
        put("\n");
        return 20;
    }
    if (nclick || nkeys || o_alerts || o_wbfrontfail) {
        IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 36);
        if (!IntuitionBase)
            return 20;
    }
    if (nkeys) {
        if (!(iport = CreateMsgPort())
            || !(io = (struct IOStdReq *)CreateIORequest(iport, sizeof *io))
            || OpenDevice("input.device", 0, (struct IORequest *)io, 0)) {
            put("inject: cannot open input.device\n");
            return 20;
        }
        kint.is_Node.ln_Type = NT_INTERRUPT;
        kint.is_Node.ln_Pri = 100;
        kint.is_Node.ln_Name = "inject";
        kint.is_Code = keys_stub;
        io->io_Command = IND_ADDHANDLER;
        io->io_Data = &kint;
        DoIO((struct IORequest *)io);
    }
    if ((nclick || nkeys)
        && !CreateNewProcTags(NP_Entry, (ULONG)driver, NP_Name, (ULONG)"inject driver",
                              NP_StackSize, 4096, NP_CurrentDir, 0,
                              NP_Input, 0, NP_CloseInput, FALSE,
                              NP_Output, 0, NP_CloseOutput, FALSE, TAG_DONE)) {
        put("inject: cannot start the driver\n");
        return 20;
    }
    if (o_cd) {
        cd_lock = cd_path[0] ? Lock(cd_path, SHARED_LOCK)
            : DupLock(((struct Process *)me)->pr_CurrentDir);
        if (!cd_lock) {
            put("inject: cannot lock the current directory for COMMAND\n");
            return 20;
        }
        old_cd = CurrentDir(cd_lock);
    }
    if (o_signals)
        for (;;) {
            BYTE s = AllocSignal(-1);

            if (s < 0)
                break;
            sigs |= 1UL << s;
        }
    {
        struct Process *pr = (struct Process *)me;

        h_cis = pr->pr_CIS;
        h_cos = pr->pr_COS;
    }
    if (nn_name[0]) {
        /* the node: next (long), number (word), name; address 0 points to it */
        static UBYTE node[48] __attribute__((aligned(4)));

        memset(node, 0, sizeof node);
        node[4] = (UBYTE)(nn_num >> 8);
        node[5] = (UBYTE)nn_num;
        strcpy((char *)node + 6, nn_name);
        __asm__ volatile("move.l 0.w,%0" : "=d"(nn_old));
        __asm__ volatile("move.l %0,0.w" : : "d"((ULONG)node) : "memory");
    }
    patch(1);
    active = 1;
    rc = RunCommand(seg, stack, args, strlen(args));
    active = 0;
    patch(0);
    if (nn_name[0])
        __asm__ volatile("move.l %0,0.w" : : "d"(nn_old) : "memory");
    {
        struct Process *pr = (struct Process *)me;

        h_kept = pr->pr_CIS == h_cis && pr->pr_COS == h_cos;
    }
    cmd_done = 1;
    for (k = 0; k < 32; k++)
        if (sigs & (1UL << k))
            FreeSignal(k);
    UnLoadSeg(seg);
    if (nclick || nkeys)
        while (!drv_done)
            Delay(2);
    if (nkeys) {
        io->io_Command = IND_REMHANDLER;
        io->io_Data = &kint;
        DoIO((struct IORequest *)io);
        CloseDevice((struct IORequest *)io);
        DeleteIORequest((struct IORequest *)io);
        DeleteMsgPort(iport);
    }
    if (o_cd) {
        BPTR now = CurrentDir(old_cd);

        cd_end_start = now == cd_lock && !cd_unlocked;
        if (!cd_unlocked)
            UnLock(cd_lock);
    }
    if (o_allocfail) {
        sprintf(line, "inject: allocfail: %lu allocations failed\n", af_done);
        put(line);
    }
    if (rf_name[0]) {
        sprintf(line, "inject: readfail: %lu reads failed\n", rf_done);
        put(line);
    }
    if (wf_name[0]) {
        sprintf(line, "inject: writefail: %lu writes failed\n", wf_done);
        put(line);
    }
    if (o_alerts) {
        sprintf(line, "inject: alerts: %lu alerts answered\n", alerts_done);
        put(line);
    }
    if (o_wbfrontfail) {
        sprintf(line, "inject: wbfrontfail: %lu calls failed\n", wbf_done);
        put(line);
    }
    if (ef_name[0]) {
        sprintf(line, "inject: examinefail: %lu examines failed\n", ef_done);
        put(line);
    }
    if (olf_name[0]) {
        sprintf(line, "inject: openlibfail: %lu opens failed\n", olf_done);
        put(line);
    }
    if (odf_name[0]) {
        sprintf(line, "inject: opendevfail: %lu opens failed\n", odf_done);
        put(line);
    }
    if (o_createprocfail) {
        sprintf(line, "inject: createprocfail: %lu processes not created\n", cpf_done);
        put(line);
    }
    sprintf(line, "inject: rc=%ld\n", rc);
    put(line);
    if (o_handles)
        put(h_kept ? "inject: handles: kept\n" : "inject: handles: changed\n");
    if (o_track)
        report_track();
    if (bad_free)
        put("inject: pad: FreeMem inside an allocation (not done)\n");
    for (k = 0; k < (int)npadw; k++) {
        sprintf(line, "inject: pad: written behind an allocation of %lu bytes\n", padw_size[k]);
        put(line);
    }
    for (k = 0; k < noc; k++) {
        sprintf(line, "inject: opencount: %s %+ld\n", oc_name[k], (long)oc_count[k]);
        put(line);
    }
    if (o_cd) {
        put(cd_end_start ? "inject: current dir at end: start\n"
            : "inject: current dir at end: other\n");
        if (cd_unlocked)
            put("inject: current dir: the start lock was unlocked\n");
    }
    for (k = 0; k < ninuse; k++) {
        BPTR l = Lock(inuse[k], EXCLUSIVE_LOCK);

        if (l) {
            UnLock(l);
            sprintf(line, "inject: not in use: %.100s\n", inuse[k]);
        } else
            sprintf(line, IoErr() == ERROR_OBJECT_IN_USE ? "inject: in use: %.100s\n"
                    : "inject: missing: %.100s\n", inuse[k]);
        put(line);
    }
    for (k = 0; k < nclick; k++) {
        sprintf(line, "inject: click %.39s/%.39s: %s\n", click[k].title, click[k].text,
                cl_text[click[k].result]);
        put(line);
        if (click[k].undo != UB_NOTCHECKED) {
            sprintf(line, "inject: click %.39s/%.39s: UndoBuffer of the string gadgets %s\n",
                    click[k].title, click[k].text,
                    click[k].undo == UB_KEPT ? "kept" : "changed");
            put(line);
        }
    }
    if (o_cacheflush) {
        static const char *const cf_text[] = {
            "not called", "caches flushed before", "caches not flushed before"
        };

        sprintf(line, "inject: CreateProc %.39s: %s\n", cf_name, cf_text[cf_result]);
        put(line);
    }
    if (nkeys)
        put(k_result == KS_SENT ? "inject: keys: sent\n"
            : k_result == KS_NOWINDOW ? "inject: keys: no window\n" : "inject: keys: not sent\n");
    if (IntuitionBase)
        CloseLibrary((struct Library *)IntuitionBase);
    return 0;
}
