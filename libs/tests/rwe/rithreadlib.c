/*
 * rwe rithreadlib - CreateThread/ActiveThreads: the C build against the
 * reference build of rwelibsrc/threadlib.bb (rithreadlib.orig), method of
 * rwecmp.h.
 *
 *   rithreadlib DIR     (DIR with memlib and rithreadlib, .obj/.orig)
 *
 * The threads are real processes; their code is thread_entry here: it
 * records the registers it starts with, its process name and the first
 * bytes of its memory, signals the test task and waits for CTRL-F, then
 * returns (the library's exit code counts down).  Each set runs a phase
 * alone, every thread started and ended before the next step, and the
 * test waits until every process is gone before it goes on (finit waits
 * for the last thread's signal, the process ends right after) - nothing
 * of a set runs while the other set is called.  Then the records of both
 * sets are compared (registers of every call, the original's kept ones
 * kept; ActiveThreads; the thread's registers: d1-d7/a1/a5/a6 the same,
 * a0 = the code, a2/a3/a4 = memory + 1024/3072/7168, the memory cleared;
 * the name; the library's signal).
 *
 * Phases:
 *   0  no free signal 17..31 (the test takes them): init gets no
 *      dos.library, CreateThread does nothing, finit nothing.
 *   1  init (the highest free signal), thread A, thread B (ActiveThreads
 *      1, 2), A ends (1, no signal), B released, finit waits for it (0).
 *   2  init again, thread C ends (0: the signal is sent and stays
 *      pending), thread D, finit: returns at once while D runs
 *      (BUG(orig), ActiveThreads still 1), D ends.
 * The signals init allocates are never freed (BUG(orig)); the test frees
 * them.  Not covered: two CreateThread before the first thread runs (the
 * shared registers, BUG(orig): depends on the scheduling).
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
} rec[2][NREC];
static int nrec[2];

/* what a thread saw */
struct seen {
    ULONG r[15];                        /* d0-d7/a0-a6 at its start */
    char name[24];
    ULONG cleared;                      /* its memory all 0 */
    struct Task *task;
};
static struct seen seen[2][4];
static volatile int slot;               /* the record of the next thread */
static struct Task *testtask;
static ULONG startsig;
static ULONG libsig[2][3];              /* the signal init took, per phase */
static ULONG active[2][NREC];
static int nactive[2];
static ULONG pending[2][4];

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
    struct seen *t = &seen[cur][slot];
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
    int n = nrec[cur]++;
    markregs(&r);
    r.d[0] = d0;
    r.d[1] = d1;
    rec[cur][n].name = name;
    rec[cur][n].in = r;
    NARGS = 0;
    CCR[cur] = callstk(ENT(LIB, idx, 0), &r, ARGS[cur], NARGS, &rec[cur][n].pop);
    rec[cur][n].out = r;
    return r.d[0];
}

static void activecount(void)
{
    active[cur][nactive[cur]++] = call(ACTIVE, "ActiveThreads", 0, 0);
}

static void initcall(int phase)
{
    ULONG before = testtask->tc_SigAlloc;
    call(INIT, "init", 0, 0);
    libsig[cur][phase] = testtask->tc_SigAlloc & ~before;
}

/* CreateThread name,thread_entry; wait for its start */
static void create(int k, const char *name)
{
    slot = k;
    seen[cur][k].task = NULL;
    call(CREATE, "CreateThread", (ULONG)name, (ULONG)thread_entry);
    check("rithreadlib: the thread started", waitsig(startsig));
}

/* release thread k, wait for its end */
static void release(int k)
{
    if (seen[cur][k].task)
        Signal(seen[cur][k].task, SIGBREAKF_CTRL_F);
}

static char names[2][4][24];

static void phase0(void)
{
    ULONG got = 0;
    int n;
    for (n = 17; n < 32; n++)
        if (AllocSignal(n) == n)
            got |= 1UL << n;
    initcall(0);
    call(CREATE, "CreateThread (no dos.library)", (ULONG)names[cur][0], (ULONG)thread_entry);
    activecount();
    call(FINIT, "finit (no dos.library)", 0, 0);
    for (n = 17; n < 32; n++)
        if (got & (1UL << n))
            FreeSignal(n);
    Forbid();
    check("rithreadlib: no thread without dos.library", !FindTask((STRPTR)names[cur][0]));
    Permit();
}

static void phase1(void)
{
    initcall(1);
    create(0, names[cur][0]);
    activecount();
    create(1, names[cur][1]);
    activecount();
    release(0);
    check("rithreadlib: thread A ended", waitgone(names[cur][0]));
    activecount();
    pending[cur][0] = SetSignal(0, 0) & libsig[cur][1];
    release(1);
    call(FINIT, "finit (waits for B)", 0, 0);
    check("rithreadlib: thread B ended", waitgone(names[cur][1]));
    activecount();
    pending[cur][1] = SetSignal(0, 0) & libsig[cur][1];
}

static void phase2(void)
{
    initcall(2);
    create(2, names[cur][2]);
    release(2);
    check("rithreadlib: thread C ended", waitgone(names[cur][2]));
    activecount();
    pending[cur][2] = SetSignal(0, 0) & libsig[cur][2];
    create(3, names[cur][3]);
    call(FINIT, "finit (signal pending, D runs)", 0, 0);
    activecount();
    release(3);
    check("rithreadlib: thread D ended", waitgone(names[cur][3]));
    activecount();
    pending[cur][3] = SetSignal(0, 0) & libsig[cur][2];
}

int main(int argc, char **argv)
{
    static const char *libnames[] = { "memlib", LIB, NULL };
    int s, k, i;

    if (argc < 2) {
        printf("usage: rithreadlib DIR\n");
        return 20;
    }
    if (!load_sets(argv[1], libnames))
        return 20;
    testtask = FindTask(NULL);
    {
        BYTE b = AllocSignal(-1);
        if (b < 0) {
            check("rithreadlib: a signal for the test", FALSE);
            return rwe_done(LIB);
        }
        startsig = 1UL << b;
    }
    for (s = 0; s < 2; s++)
        for (k = 0; k < 4; k++)
            sprintf(names[s][k], "rithread test %d%c", s, 'A' + k);

    for (cur = 0; cur < 2; cur++) {
        struct regs r;
        markregs(&r);
        callstk(entry_form(getlib(&sets[cur], "memlib"), 7, 0), &r, NULL, 0, &POP[cur]);
    }

    for (cur = 0; cur < 2; cur++) {
        phase0();
        phase1();
        phase2();
        /* the library never frees its signals */
        for (i = 0; i < 32; i++)
            if ((libsig[cur][1] | libsig[cur][2]) & (1UL << i)) {
                SetSignal(0, 1UL << i);
                FreeSignal(i);
            }
    }

    for (cur = 0; cur < 2; cur++) {
        struct regs r;
        markregs(&r);
        callstk(entry_form(getlib(&sets[cur], "memlib"), 8, 0), &r, NULL, 0, &POP[cur]);
    }
    FreeSignal(__builtin_ctzl(startsig));

    /* compare */
    strcpy(VEC, "");
    SAME("calls", nrec[cur]);
    for (k = 0; k < nrec[0] && k < nrec[1]; k++) {
        ROUT = rec[0][k].name;
        sprintf(VEC, "#%d", k);
        for (s = 0; s < 2; s++) {
            IN[s] = rec[s][k].in;
            R[s] = rec[s][k].out;
            POP[s] = rec[s][k].pop;
        }
        if (k == 0 || rec[0][k].name != rec[1][k].name)
            check("rithreadlib: the same calls", rec[0][k].name == rec[1][k].name);
        DONTCARE = 0;
        preserve();
    }
    ROUT = "ActiveThreads";
    SAME("calls", nactive[cur]);
    for (k = 0; k < nactive[0]; k++) {
        sprintf(VEC, "#%d", k);
        SAME("count", active[cur][k]);
    }
    {
        static const ULONG want[] = { 0, 1, 2, 1, 0, 0, 1, 0 };
        for (k = 0; k < nactive[0] && k < 8; k++) {
            sprintf(VEC, "#%d", k);
            BOTH("expected count", active[cur][k] == want[k]);
        }
    }
    ROUT = "signals";
    strcpy(VEC, "");
    BOTH("phase 0: none", libsig[cur][0] == 0);
    BOTH("phase 1: one", libsig[cur][1] != 0 && !(libsig[cur][1] & (libsig[cur][1] - 1)));
    BOTH("phase 2: one", libsig[cur][2] != 0 && !(libsig[cur][2] & (libsig[cur][2] - 1)));
    for (k = 0; k < 4; k++) {
        sprintf(VEC, "pending %d", k);
        SAME("library signal", pending[cur][k] != 0);
    }
    BOTH("A ended: no signal", pending[cur][0] == 0);
    BOTH("finit took B's signal", pending[cur][1] == 0);
    BOTH("C ended: signal pending", pending[cur][2] != 0);
    BOTH("D ended: signal", pending[cur][3] != 0);

    ROUT = "thread";
    for (k = 0; k < 4; k++) {
        sprintf(VEC, "%c", 'A' + k);
        BOTH("started", seen[cur][k].task != NULL);
        BOTH("name", strcmp(seen[cur][k].name, names[cur][k]) == 0);
        BOTH("memory cleared", seen[cur][k].cleared);
        BOTH("a0 = the code", seen[cur][k].r[8] == (ULONG)thread_entry);
        BOTH("a2 = memory + 1024", seen[cur][k].r[10] == seen[cur][k].r[0] + 1024);
        BOTH("a3 = memory + 3072", seen[cur][k].r[11] == seen[cur][k].r[0] + 3072);
        BOTH("a4 = memory + 7168", seen[cur][k].r[12] == seen[cur][k].r[0] + 7168);
        for (i = 1; i < 15; i++) {
            char w[16];
            if (i == 8 || (i >= 10 && i <= 12))
                continue;
            sprintf(w, "%c%d", i < 8 ? 'd' : 'a', i & 7);
            SAME(w, seen[cur][k].r[i]);
        }
    }
    return rwe_done(LIB);
}
