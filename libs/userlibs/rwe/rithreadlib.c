/*
 * rithreadlib - RWE userlib 44 (#rithreadlib, Reflective Images 1995):
 * Blitz code as a separate process.
 *   CreateThread name$,address   a new DOS process (CreateNewProc) that
 *                                runs the Blitz code at `address` with
 *                                the registers of the call and memory of
 *                                its own for the stack-like registers
 *   ActiveThreads                the number of threads not yet ended
 *
 * C translation of rwelibsrc/threadlib.bb; reference: the reference
 * build of the source (REFERENCE refbuild, no binary exists;
 * docs/rwelibs/README.md 2 and 3.3).  Notes: docs/rwelibs/port/
 * rithreadlib.md.
 *
 *   init:  a signal of the calling task (the highest free of 31..17),
 *          the task, dos.library V36 (none without a free signal)
 *   finit: waits for that signal until no thread runs (the last
 *          thread to end sends it), closes dos.library, frees the signal
 *
 * CreateThread takes 10240 bytes (MEMF_CLEAR) through memlib's
 * globalloc (freed by memlib's finit), with the thread's start data
 * behind them: d0-d7/a1-a6 as they are after that call (d0 = the memory),
 * the address and the memory.  It hands the start data to the process
 * in tc_UserData, set between Forbid and Permit around CreateNewProc
 * (the process cannot run before).  The process starts at startupcode,
 * which takes them from its tc_UserData (and clears it), loads the
 * registers, then a2 = memory+1024, a3 = memory+1024+2048 (the string
 * buffer), a4 = a3+4096, a0 = address, and jumps to the address; when
 * the code returns, DOS calls the exit code (cleanup): the count goes
 * down, and the parent task gets the signal when it is 0.  The thread
 * shares the program's globals (a5).
 *
 * Assembler: startupcode (it sets every register and jumps), the
 * ALibJsr to globalloc (the registers after it are the thread's), the
 * count's ADDQ/SUBQ (one instruction: the parent and the threads change
 * it), the thunks.
 *
 * Fixed (intended deviations):
 *   - The registers, the memory and the address were single variables:
 *     a second CreateThread before the first thread had started
 *     (CreateNewProc returns before the process runs) gave both threads
 *     the second call's registers, memory and code; they are now start
 *     data of each thread (#838).
 *   - finit waited only once, so a signal left from an earlier time the
 *     count was 0 let it go on while threads ran (their exit code is in
 *     this library); it now waits until the count is 0 (#837).
 *   - finit never freed the signal and left dos.library's base set: every
 *     run in the same task (from the editor, or a CLI program in the
 *     shell's process) took one more signal; it now frees it, also when
 *     dos.library did not open, and clears the base (#839).
 *   - CreateThread counted the thread before CreateNewProc and did not
 *     test its result: a process that could not be created (too little
 *     memory) left ActiveThreads one too high, and finit waited for it
 *     forever (the original returned only on a signal left from an
 *     earlier count of 0); the count goes down again when CreateNewProc
 *     returns 0 (#1086).
 *
 * No defects: the memory is not checked, but memlib's globalloc never
 * returns 0 (it raises "Out of Memory!"); the name is the string
 * argument as it is (CreateNewProc copies it); ActiveThreads (#word)
 * returns the count as a long (the caller takes the low word, which is
 * the count).
 */
#include "blitzlib.h"
#include "blitzexec.h"
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <utility/tagitem.h>

#define LVO_CreateNewProc (-498)

/* entries (thunks below) */
void _createthread(void), _activethreads(void), rithreadlib_libinit(void);
void rithreadlib_libfinit(void), startupcode(void), cleanup(void);

BLITZ_ENTRY_DECL(finit);
BLITZ_ENTRY_DECL(init);

/* !libheader {#rithreadlib,init,0,finit,0} */
BLITZ_LIBHEADER(LIB_rithreadlib, init, BL_NORET, finit, 0);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_STRING, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_createthread), 0, 0);
BLITZ_NAME("CreateThread", "name,address");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_activethreads), 0, 0);
BLITZ_NAME("ActiveThreads", "");

BLITZ_NULLSUB_AT(finit, rithreadlib_libfinit, 0, 0);
BLITZ_NULLSUB_AT(init, rithreadlib_libinit, 0, 0);

BLITZ_LIBFIN();

#define STRINGBUFFERSIZE 2048

/* the start data of a thread, behind its memory; startupcode reads it
   through tc_UserData */
struct thstart {
    ULONG regs[14];                     /* d0-d7/a1-a6 */
    ULONG entrypoint;
    ULONG memory;
};
_Static_assert(__builtin_offsetof(struct thstart, entrypoint) == 56, "startupcode: entrypoint");
_Static_assert(__builtin_offsetof(struct thstart, memory) == 60, "startupcode: memory");
_Static_assert(__builtin_offsetof(struct ExecBase, ThisTask) == 276, "startupcode: ThisTask");
_Static_assert(__builtin_offsetof(struct Task, tc_UserData) == 88, "startupcode: tc_UserData");

/* the state (the source's variables, in its order, without the start
   data); startupcode reads stringbuffersize by name */
struct thstate {
    ULONG thesignal;                    /* -1: none allocated */
    ULONG stringbuffersize;
    volatile ULONG threadcount;
    struct Task *parenttask;
    struct {                            /* CreateNewProc's tags */
        struct TagItem entry, name, exitcode, end;
    } threadtaglist;
    struct Library *dbase;
};
static_assert(sizeof(((struct thstate *)0)->threadtaglist) == 8 * 4);

struct thstate th = {
    .thesignal = (ULONG)-1,
    .stringbuffersize = STRINGBUFFERSIZE,
    .threadtaglist = { { NP_Entry, (ULONG)startupcode }, { NP_Name, 0 },
                       { NP_ExitCode, (ULONG)cleanup }, { TAG_END, TAG_END } },
};

static const char dosname[] = "dos.library";

/* the process's start: its start data from tc_UserData (cleared), the
   saved registers, a2/a3/a4 in its memory, a0 = the code, which it jumps
   to */
BLITZ_ASM_BLOCK(startupcode,
"   .globl  _startupcode\n"
"_startupcode:\n"
"   move.l  4.w,a1\n"
"   move.l  276(a1),a1\n"               /* ThisTask */
"   move.l  88(a1),a0\n"                /* tc_UserData: the start data */
"   clr.l   88(a1)\n"
"   movem.l (a0)+,d0-d7/a1-a6\n"
"   move.l  4(a0),a3\n"                 /* memory */
"   lea     1024(a3),a2\n"
"   lea     1024(a3),a3\n"
"   add.l   _th+4,a3\n"                 /* stringbuffersize */
"   lea     4096(a3),a4\n"
"   move.l  (a0),a0\n"                  /* entrypoint */
"   jmp     (a0)\n"
);
_Static_assert(__builtin_offsetof(struct thstate, stringbuffersize) == 4,
               "startupcode: stringbuffersize");

/**
 * @brief The exit code of every thread (NP_ExitCode; DOS calls it when the
 * thread's code returns).
 *
 * Counts the thread down (one SUBQ.l, the parent and the threads change
 * the count) and sends the parent task the init's signal when the count
 * is 0.
 */
void BLITZ_ROUTINE c_cleanup(void)
{
    __asm__ volatile("subq.l #1,%0" : "+m"(th.threadcount) : : "cc");
    if (th.threadcount == 0)
        Signal(th.parenttask, 1UL << th.thesignal);
}

/**
 * @brief ActiveThreads: the number of threads not yet ended.
 *
 * The function is a #word one; the count is returned as a long, the
 * caller takes the low word, which is the count (no defect).
 * @return d0: the count
 */
ULONG BLITZ_ROUTINE c_activethreads(void)
{
    return th.threadcount;
}

/**
 * @brief CreateThread name$,address: run the Blitz code at address as a new
 * DOS process (CreateNewProc).
 *
 * Nothing without dos.library V36 (init).  Counts the thread up, takes
 * 10240 bytes (stringbuffersize + 8192, MEMF_CLEAR) and the start data
 * behind them through memlib's globalloc (freed by memlib's finit;
 * globalloc never returns 0, it raises "Out of Memory!") and saves
 * d0-d7/a1-a6 as they are after that call (d0 = the memory), the address
 * and the memory in the start data, which the new process gets in
 * tc_UserData (set before it can run: Forbid ... Permit around
 * CreateNewProc).  startupcode loads them, sets a2/a3/a4 in the memory
 * and jumps to the address.  The thread shares the program's globals
 * (a5).  Fixed (#838): the original keeps registers, memory and address
 * in single variables - a second CreateThread before the first thread
 * has started gives both threads the second call's.  When CreateNewProc
 * returns 0 the count goes down again (Fixed (#1086): the original kept
 * it, ActiveThreads one too high and finit waiting for that thread; the
 * memory stays allocated until memlib's finit, as in the original).
 * @param r  a0: the registers of the call as the thunk pushed them,
 *           d0-d7/a0-a6 (d0 = name$, the process name as it is, which
 *           CreateNewProc copies; d1 = address)
 */
void BLITZ_ROUTINE c_createthread(const ULONG *r BL_REG(a0))
{
    struct thstart *st;

    if (!th.dbase)
        return;
    __asm__ volatile("addq.l #1,%0" : "+m"(th.threadcount) : : "cc");
    th.threadtaglist.name.ti_Data = r[0];
    {
        /* ALibJsr globalloc: d0 = size, d1 = flags; the registers after
           it (d0 = the memory) are the thread's (memlib keeps d2-d7/
           a0-a6, so these are the call's) */
        register ULONG d0 __asm("d0") = th.stringbuffersize + 8192
                                        + sizeof(struct thstart);
        register ULONG d1 __asm("d1") = MEMF_CLEAR;
        register ULONG a0 __asm("a0") = r[8];
        register ULONG a1 __asm("a1") = r[9];
        register ULONG a6 __asm("a6") = r[14];
        int i;

        __asm__ volatile("jsr _blitz_tok+%c5"
            : "+d"(d0), "+d"(d1), "+a"(a0), "+a"(a1), "+a"(a6)
            : "i"(BL_TOKVAL(TOK_globalloc, 0))
            : "d2", "d3", "d4", "d5", "d6", "d7", "a2", "cc", "memory");
        st = (struct thstart *)(d0 + th.stringbuffersize + 8192);
        st->memory = d0;
        st->entrypoint = r[1];
        st->regs[0] = d0;
        st->regs[1] = d1;
        for (i = 2; i < 8; i++)         /* d2-d7 */
            st->regs[i] = r[i];
        st->regs[8] = a1;
        for (i = 10; i < 14; i++)       /* a2-a5 */
            st->regs[i - 1] = r[i];
        st->regs[13] = a6;
    }
    /* the process cannot run before Permit: it finds its start data */
    Forbid();
    {
        struct bl_regs cr = { .d1 = (ULONG)&th.threadtaglist };
        struct Process *p = (struct Process *)
            BLITZ_BASEJSR(th.dbase, LVO_CreateNewProc, cr);

        if (p)
            p->pr_Task.tc_UserData = st;
        else                            /* Fixed (#1086): no thread to count */
            __asm__ volatile("subq.l #1,%0" : "+m"(th.threadcount) : : "cc");
    }
    Permit();
}

/**
 * @brief init: a signal of the calling task, the task, dos.library V36.
 *
 * The highest free signal of 31 down to 17; without one, dos.library is
 * not opened (dbase = 0, every CreateThread does nothing).
 */
static void BLITZ_ROUTINE c_libinit(void)
{
    LONG sig, n;

    for (n = 31;; ) {
        sig = AllocSignal(n);
        if (sig != -1)
            break;
        if (--n == 16) {
            th.thesignal = (ULONG)-1;
            th.dbase = 0;
            return;
        }
    }
    th.thesignal = (ULONG)sig;
    th.parenttask = FindTask(0);
    th.dbase = OpenLibrary((CONST_STRPTR)dosname, 36);
}

/**
 * @brief finit: wait for the last thread, close dos.library, free the
 * signal.
 *
 * Waits for the signal until no thread runs (the last one to end sends
 * it).  Fixed (#837): the original waits only once, so a signal left from
 * an earlier time the count was 0 lets it go on while threads run.
 * Fixed (#839): the original never frees the signal and leaves the base
 * set; the signal is freed (cleared first) also when dos.library did not
 * open.
 */
static void BLITZ_ROUTINE c_libfinit(void)
{
    if (th.dbase) {
        while (th.threadcount)
            Wait(1UL << th.thesignal);
        CloseLibrary(th.dbase);
        th.dbase = 0;
    }
    if (th.thesignal != (ULONG)-1) {
        SetSignal(0, 1UL << th.thesignal);
        FreeSignal((LONG)th.thesignal);
        th.thesignal = (ULONG)-1;
    }
}

/* entries: CreateThread gets the registers of its call (and keeps them
   all), the others keep d0-d1/a0-a1 that are not the result */
BLITZ_THUNK_PRE(_createthread, c_createthread, "d0-d7/a0-a6", "move.l  sp,a0");
BLITZ_THUNK(_activethreads, c_activethreads, "d1/a0-a1");
BLITZ_THUNK(rithreadlib_libinit, c_libinit, "d0-d1/a0-a1");
BLITZ_THUNK(rithreadlib_libfinit, c_libfinit, "d0-d1/a0-a1");
BLITZ_THUNK(cleanup, c_cleanup, "d0-d1/a0-a1");
