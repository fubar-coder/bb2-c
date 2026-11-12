/*
 * rwe rithreadlib - CreateThread/ActiveThreads: the C build or the
 * reference build of rwelibsrc/threadlib.bb (rithreadlib.orig), one set as
 * LIBSET says, observed as cases (rwecmp.h; the expected
 * data libs/tests/expected/rwe_rithreadlib.txt).
 *
 *   rithreadlib DIR     (DIR with memlib and rithreadlib, .obj/.orig)
 *
 * The threads are real processes; their code is thread_entry here: it
 * records the registers it starts with, its process name and the first
 * bytes of its memory, signals the test task and waits for CTRL-F, then
 * returns (the library's exit code counts down).  Every thread is
 * started and ended before the next step, and the test waits until every
 * process is gone before it goes on (finit waits for the last thread's
 * signal, the process ends right after).  Then the records are observed
 * (the kept registers of every call; ActiveThreads; the thread's
 * registers: d1-d7/a1/a5/a6, a0 = the code, a2/a3/a4 = memory +
 * 1024/3072/7168, the memory cleared; the name; the library's signal).
 *
 * Phases:
 *   0  no free signal 17..31 (the test takes them): init gets no
 *      dos.library, CreateThread does nothing, finit nothing.
 *   1  init (the highest free signal), thread A, thread B (ActiveThreads
 *      1, 2), A ends (1, no signal), B released, finit waits for it (0).
 *   2  init again, thread C ends (0: the signal is sent and stays
 *      pending), thread D, finit: returns at once while D runs
 *      (BUG(orig) #837, ActiveThreads still 1), D ends.
 * The signals init allocates are never freed (BUG(orig), #839); the test frees
 * them.  Not covered: two CreateThread before the first thread runs (the
 * shared registers, BUG(orig) #838: depends on the scheduling).
 */
#include "rwecmp.h"
#include <exec/memory.h>
#include <exec/tasks.h>
#include <dos/dos.h>

#define LIB "rithreadlib"

enum { CREATE, ACTIVE, FINIT, INIT };

#define NREC 32
#define MEMSIZE 10240                   /* 2048 + 8192 */

static struct {
    const char *name;
    struct regs in, out;
    ULONG pop;
} rec[NREC];
static int nrec;

/* what a thread saw */
struct seen {
    ULONG r[15];                        /* d0-d7/a0-a6 at its start */
    char name[24];
    ULONG cleared;                      /* its memory all 0 */
    struct Task *task;
};
static struct seen seen[4];
static volatile int slot;               /* the record of the next thread */
static struct Task *testtask;
static ULONG startsig;
static ULONG libsig[3];                 /* the signal init took, per phase */
static ULONG active[NREC];
static int nactive;
static ULONG pending[4];

/* the thread's code: all registers to thread_c */
void thread_entry(void);
__asm__(
"   .text\n"
"   .even\n"
"   .globl  _thread_entry\n"
"_thread_entry:\n"
"   movem.l d0-d7/a0-a6,-(sp)\n"
"   move.l  sp,-(sp)\n"
"   jsr     _thread_c\n"
"   addq.l  #4,sp\n"
"   movem.l (sp)+,d0-d7/a0-a6\n"
"   rts\n"
);

void thread_c(const ULONG *r);
void thread_c(const ULONG *r)
{
    struct seen *t = &seen[slot];
    struct Task *me = FindTask(NULL);
    const UBYTE *m = (const UBYTE *)r[0];
    ULONG i;

    memcpy(t->r, r, sizeof t->r);
    strncpy(t->name, me->tc_Node.ln_Name ? me->tc_Node.ln_Name : "", sizeof t->name - 1);
    t->cleared = 1;
    if (TypeOfMem((APTR)m))
        for (i = 0; i < MEMSIZE; i++)
            if (m[i])
                t->cleared = 0;
    t->task = me;
    Signal(testtask, startsig);
    Wait(SIGBREAKF_CTRL_F);
}

/* wait for `sig` (at most 5 seconds) */
static BOOL waitsig(ULONG sig)
{
    int i;
    for (i = 0; i < 250; i++) {
        if (SetSignal(0, sig) & sig)
            return TRUE;
        Delay(1);
    }
    return FALSE;
}

/* wait until the process `name` is gone (at most 5 seconds) */
static BOOL waitgone(const char *name)
{
    int i;
    struct Task *t;
    for (i = 0; i < 250; i++) {
        Forbid();
        t = FindTask((STRPTR)name);
        Permit();
        if (!t)
            return TRUE;
        Delay(1);
    }
    return FALSE;
}

static ULONG call(int idx, const char *name, ULONG d0, ULONG d1)
{
    struct regs r;
    int n = nrec++;
    markregs(&r);
    r.d[0] = d0;
    r.d[1] = d1;
    rec[n].name = name;
    rec[n].in = r;
    NARGS = 0;
    CCR = callstk(ENT(LIB, idx, 0), &r, ARGS, NARGS, &rec[n].pop);
    rec[n].out = r;
    return r.d[0];
}

static void activecount(void)
{
    active[nactive++] = call(ACTIVE, "ActiveThreads", 0, 0);
}

static void initcall(int phase)
{
    ULONG before = testtask->tc_SigAlloc;
    call(INIT, "init", 0, 0);
    libsig[phase] = testtask->tc_SigAlloc & ~before;
}

/* CreateThread name,thread_entry; wait for its start */
static void create(int k, const char *name)
{
    slot = k;
    seen[k].task = NULL;
    call(CREATE, "CreateThread", (ULONG)name, (ULONG)thread_entry);
    check("rithreadlib: the thread started", waitsig(startsig));
}

/* release thread k, wait for its end */
static void release(int k)
{
    if (seen[k].task)
        Signal(seen[k].task, SIGBREAKF_CTRL_F);
}

static char names[4][24];

static void phase0(void)
{
    ULONG got = 0;
    int n;
    for (n = 17; n < 32; n++)
        if (AllocSignal(n) == n)
            got |= 1UL << n;
    initcall(0);
    call(CREATE, "CreateThread (no dos.library)", (ULONG)names[0], (ULONG)thread_entry);
    activecount();
    call(FINIT, "finit (no dos.library)", 0, 0);
    for (n = 17; n < 32; n++)
        if (got & (1UL << n))
            FreeSignal(n);
    Forbid();
    check("rithreadlib: no thread without dos.library", !FindTask((STRPTR)names[0]));
    Permit();
}

static void phase1(void)
{
    initcall(1);
    create(0, names[0]);
    activecount();
    create(1, names[1]);
    activecount();
    release(0);
    check("rithreadlib: thread A ended", waitgone(names[0]));
    activecount();
    pending[0] = SetSignal(0, 0) & libsig[1];
    release(1);
    call(FINIT, "finit (waits for B)", 0, 0);
    check("rithreadlib: thread B ended", waitgone(names[1]));
    activecount();
    pending[1] = SetSignal(0, 0) & libsig[1];
}

static void phase2(void)
{
    initcall(2);
    create(2, names[2]);
    release(2);
    check("rithreadlib: thread C ended", waitgone(names[2]));
    activecount();
    pending[2] = SetSignal(0, 0) & libsig[2];
    create(3, names[3]);
    call(FINIT, "finit (signal pending, D runs)", 0, 0);
    activecount();
    release(3);
    check("rithreadlib: thread D ended", waitgone(names[3]));
    activecount();
    pending[3] = SetSignal(0, 0) & libsig[2];
}

int main(int argc, char **argv)
{
    static const char *libnames[] = { "memlib", LIB, NULL };
    int k, i;

    if (argc < 2) {
        printf("usage: rithreadlib DIR\n");
        return 20;
    }
    if (!load_set(argv[1], libnames))
        return 20;
    KEEPS("init", d3-d7|a2|a3);
    KEEPS("CreateThread (no dos.library)", d0-d7|a0-a3|a6);
    KEEPS("ActiveThreads", d1-d7|a0-a3|a6);
    KEEPS("finit (no dos.library)", d0-d7|a0-a3|a6);
    KEEPS("CreateThread", d2-d7|a2|a3);
    KEEPS("finit (waits for B)", d0-d7|a2|a3);
    KEEPS("finit (signal pending, D runs)", d0-d7|a2|a3);
    testtask = FindTask(NULL);
    {
        BYTE b = AllocSignal(-1);
        if (b < 0) {
            check("rithreadlib: a signal for the test", FALSE);
            return rwe_done(LIB);
        }
        startsig = 1UL << b;
    }
    for (k = 0; k < 4; k++)
        sprintf(names[k], "rithread test %c", 'A' + k);

    {
        struct regs r;
        markregs(&r);
        callstk(entry_form(getlib(&loaded, "memlib"), 7, 0), &r, NULL, 0, &POP);
    }

    phase0();
    phase1();
    phase2();
    /* the library never frees its signals */
    for (i = 0; i < 32; i++)
        if ((libsig[1] | libsig[2]) & (1UL << i)) {
            SetSignal(0, 1UL << i);
            FreeSignal(i);
        }

    {
        struct regs r;
        markregs(&r);
        callstk(entry_form(getlib(&loaded, "memlib"), 8, 0), &r, NULL, 0, &POP);
    }
    FreeSignal(__builtin_ctzl(startsig));

    /* the observations */
    strcpy(VEC, "");
    OBS("calls", nrec);
    for (k = 0; k < nrec; k++) {
        ROUT = rec[k].name;
        sprintf(VEC, "#%d", k);
        IN = rec[k].in;
        R = rec[k].out;
        POP = rec[k].pop;
        DONTCARE = 0;
        preserve();
    }
    ROUT = "ActiveThreads";
    OBS("calls", nactive);
    for (k = 0; k < nactive; k++) {
        sprintf(VEC, "#%d", k);
        OBS("count", active[k]);
    }
    {
        static const ULONG want[] = { 0, 1, 2, 1, 0, 0, 1, 0 };
        for (k = 0; k < nactive && k < 8; k++) {
            sprintf(VEC, "#%d", k);
            BOTH("expected count", active[k] == want[k]);
        }
    }
    ROUT = "signals";
    strcpy(VEC, "");
    BOTH("phase 0: none", libsig[0] == 0);
    BOTH("phase 1: one", libsig[1] != 0 && !(libsig[1] & (libsig[1] - 1)));
    BOTH("phase 2: one", libsig[2] != 0 && !(libsig[2] & (libsig[2] - 1)));
    for (k = 0; k < 4; k++) {
        sprintf(VEC, "pending %d", k);
        OBS("library signal", pending[k] != 0);
    }
    BOTH("A ended: no signal", pending[0] == 0);
    BOTH("finit took B's signal", pending[1] == 0);
    BOTH("C ended: signal pending", pending[2] != 0);
    BOTH("D ended: signal", pending[3] != 0);

    ROUT = "thread";
    for (k = 0; k < 4; k++) {
        sprintf(VEC, "%c", 'A' + k);
        BOTH("started", seen[k].task != NULL);
        BOTH("name", strcmp(seen[k].name, names[k]) == 0);
        BOTH("memory cleared", seen[k].cleared);
        BOTH("a0 = the code", seen[k].r[8] == (ULONG)thread_entry);
        BOTH("a2 = memory + 1024", seen[k].r[10] == seen[k].r[0] + 1024);
        BOTH("a3 = memory + 3072", seen[k].r[11] == seen[k].r[0] + 3072);
        BOTH("a4 = memory + 7168", seen[k].r[12] == seen[k].r[0] + 7168);
        for (i = 1; i < 15; i++) {
            char w[16];
            if (i == 8 || (i >= 10 && i <= 12))
                continue;
            sprintf(w, "%c%d", i < 8 ? 'd' : 'a', i & 7);
            OBS(w, seen[k].r[i]);
        }
    }
    return rwe_done(LIB);
}
