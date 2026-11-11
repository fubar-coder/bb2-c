/*
 * rwe riseriallib - the serial port library with its own interrupts: the
 * C build against the reference build of rwelibsrc/seriallib/
 * Serial-Library.bb (riseriallib.orig), method of rwecmp.h.
 *
 *   riseriallib DIR     (DIR with ldivlib and riseriallib, .obj/.orig)
 *
 * The library writes the custom chip registers and CIA-B PRA itself and
 * its handlers into the autovectors $64/$74; SerialClose switches every
 * interrupt off (INTENA $7FFF).  None of that may reach the machine:
 *   - every absolute address $DFF000-$DFF1FF and $BFD000 in both loaded
 *     libraries is patched to a RAM copy per set (the custom registers
 *     filled with a fixed state before each call: SERDATR with TBE,
 *     INTENAR $2A35, INTREQR 0), compared after each call.  These are
 *     the 13 custom chip and 2 CIA-B operands in the original's code and
 *     the one long of each base in the C build (riseriallib.c riser_custom,
 *     riser_ciab), whatever code GCC makes of the accesses;
 *   - every call runs inside Disable() with the vectors $64/$74 set to
 *     the test's stubs (RTE, counting); what the library wrote there is
 *     recorded (a stub, or a handler in its code) and the system's
 *     vectors are put back before Enable().  So SerialOpen saves the
 *     stubs as the old vectors, and the handlers chain to them.
 * The handlers are called by hand through Supervisor() (RTE), with the
 * RBF/TBE bits of the fake INTREQR and the received byte in the fake
 * SERDATR: that is also how bytes arrive.  Routines that wait for bytes
 * the test cannot give beforehand (SerialReceiveBlock throws away what
 * is there first; SerialReadLong never returns) run in a task of their
 * own at a lower priority, fed byte by byte while it waits; the test
 * removes it (RemTask) when it is done or, for SerialReadLong, when every
 * byte is read - the port is open then, so the task does not touch the
 * vectors.
 *
 * Compared after every call: the fake registers, CIA-B PRA, the vectors,
 * the receive ring (rbig, rend, buffer - in front of SerialBuffer's
 * address in both builds) and the transmit ring (wbig, wrend, buffer -
 * found by the first three bytes written), d0 of the functions, and the
 * registers the original keeps.  At the end the system's vectors are
 * checked.
 * Not covered: the stale JMP operands in the original's instruction
 * cache (the test clears the cache before calling a handler), SerialBaud
 * 0 (ldivlib divides by 0), a ring index with $FF in its high byte.
 */
#include "rwecmp.h"
#include <exec/memory.h>
#include <exec/tasks.h>
#include <exec/execbase.h>

#define LIB "riseriallib"

enum { SETBAUD, OPEN, CLOSE, READ, WRITE, VERSION, BUFFER, SENDBLOCK, RECVBLOCK, FLUSH,
       SENDSTR, READSTR, PROGRESS, READLONG, WRITELONG, STOP, GOT, WAIT, FINIT };

#define FAKESZ 0x200                    /* $DFF000 ... $DFF1FF */
#define RINGSZ (4 + 1024)               /* the two indices, the buffer */

static UBYTE *fake[2], *fakecia[2];
static UBYTE *rdp[2], *wrp[2];          /* the rings */
static ULONG h1[2], h5[2];              /* the handlers the library installed */
static ULONG got64[2], got74[2];        /* the vectors after the last call */
static ULONG hits[2][2];                /* the stubs called, per set */
static ULONG hb1, hb5;
ULONG riser_hits1, riser_hits5, riser_landed;
static UBYTE *databuf[2];               /* 512 bytes per set, at a multiple of 256 */
static UBYTE *dataraw[2];
static struct Task *testtask;
static ULONG donesig;

/* the test's vectors and landing places */
void riser_stub1(void), riser_stub5(void), riser_landing(void), riser_landpop(void);
__asm__(
"   .text\n"
"   .even\n"
"   .globl  _riser_stub1\n"
"_riser_stub1:\n"
"   addq.l  #1,_riser_hits1\n"
"   rte\n"
"   .globl  _riser_stub5\n"
"_riser_stub5:\n"
"   addq.l  #1,_riser_hits5\n"
"   rte\n"
/* SerialReadString with the port open returns to the d0 of the call */
"   .globl  _riser_landing\n"
"_riser_landing:\n"
"   addq.l  #1,_riser_landed\n"
"   rts\n"
/* SerialReceiveBlock stopped: its return address is in d7, the long
   above it (callregs' argument) is where it returns to */
"   .globl  _riser_landpop\n"
"_riser_landpop:\n"
"   addq.l  #1,_riser_landed\n"
"   subq.l  #4,sp\n"
"   move.l  d7,-(sp)\n"
"   rts\n"
);

static volatile ULONG *vec(ULONG a)
{
    volatile ULONG *p;
    __asm__("" : "=a"(p) : "0"(a));
    return p;
}

static ULONG getl(const UBYTE *p) { return (ULONG)p[0] << 24 | (ULONG)p[1] << 16 | p[2] << 8 | p[3]; }
static void putl(UBYTE *p, ULONG v) { p[0] = v >> 24; p[1] = v >> 16; p[2] = v >> 8; p[3] = v; }

/* every absolute $DFF000-$DFF1FF and $BFD000 in the library (the code
   of the original, the base longs of the C build) to the set's
   copies */
static int patch(struct lib *l, UBYTE *f, UBYTE *cia)
{
    ULONG i, v;
    int n = 0;
    for (i = 0; i + 4 <= l->size; i += 2) {
        v = getl(l->code + i);
        if (v >= 0xdff000 && v < 0xdff000 + FAKESZ) {
            putl(l->code + i, (ULONG)f + v - 0xdff000);
            n++;
            i += 2;
        } else if (v == 0xbfd000) {
            putl(l->code + i, (ULONG)cia);
            n += 0x100;
            i += 2;
        }
    }
    return n;
}

static void fillfake(int s)
{
    memset(fake[s], 0, FAKESZ);
    fake[s][0x18] = 0x20;               /* SERDATR: TBE */
    fake[s][0x1c] = 0x2a;               /* INTENAR (INTEN off: SerialClose sets it) */
    fake[s][0x1d] = 0x35;
    fakecia[s][0] = 0x5a;
}

static int incode(int s, ULONG a)
{
    struct lib *l = getlib(&sets[s], LIB);
    return a >= (ULONG)l->code && a < (ULONG)l->code + l->size;
}

/* a vector as seen after a call: 0 the stub, 1 in the library, 2 else */
static ULONG norm(int s, ULONG v, APTR stub)
{
    return v == (ULONG)stub ? 0 : incode(s, v) ? 1 : 2;
}

static ULONG sys64, sys74;

static void vecs_in(void)
{
    Disable();
    sys64 = *vec(0x64);
    sys74 = *vec(0x74);
    *vec(0x64) = (ULONG)riser_stub1;
    *vec(0x74) = (ULONG)riser_stub5;
}

static void vecs_out(int s)
{
    got64[s] = *vec(0x64);
    got74[s] = *vec(0x74);
    *vec(0x64) = sys64;
    *vec(0x74) = sys74;
    Enable();
}

static void hits_in(void)
{
    hb1 = riser_hits1;
    hb5 = riser_hits5;
}

static void hits_out(int s)
{
    hits[s][0] = riser_hits1 - hb1;
    hits[s][1] = riser_hits5 - hb5;
}

static void compare(void)
{
    SAMEBYTES("custom chip registers", fake, FAKESZ);
    SAME("CIA-B PRA", fakecia[cur][0]);
    SAME("vector $64", norm(cur, got64[cur], riser_stub1));
    SAME("vector $74", norm(cur, got74[cur], riser_stub5));
    if (rdp[0] && rdp[1])
        SAMEBYTES("receive ring", rdp, RINGSZ);
    if (wrp[0] && wrp[1])
        SAMEBYTES("transmit ring", wrp, RINGSZ);
    SAME("stub $64 called", hits[cur][0]);
    SAME("stub $74 called", hits[cur][1]);
}

/* one call on both sets, inside Disable() with the test's vectors */
#define CALL(name, idx, form, setup) do {                                   \
        ROUT = (name);                                                      \
        for (cur = 0; cur < 2; cur++) {                                     \
            APTR fn_ = ENT(LIB, (idx), (form));                             \
            markregs(&R[cur]);                                              \
            NARGS = 0;                                                      \
            setup;                                                          \
            IN[cur] = R[cur];                                               \
            fillfake(cur);                                                  \
            hits_in();                                                      \
            if (!fn_) {                                                     \
                check("entry missing", FALSE);                              \
                continue;                                                   \
            }                                                               \
            vecs_in();                                                      \
            CCR[cur] = callstk(fn_, &R[cur], ARGS[cur], NARGS, &POP[cur]);  \
            vecs_out(cur);                                                  \
            hits_out(cur);                                                  \
            CacheClearU();                                                  \
        }                                                                   \
        preserve();                                                         \
        compare();                                                          \
    } while (0)

/* a handler by hand (Supervisor, RTE) on set s: `level` 1 or 5, the
   fake INTREQR and the byte in SERDATR */
static void inthandler(int s, int level, UBYTE reqhi, UBYTE reqlo, UBYTE data)
{
    ULONG h = level == 5 ? h5[s] : h1[s];
    fillfake(s);
    fake[s][0x1e] = reqhi;
    fake[s][0x1f] = reqlo;
    fake[s][0x19] = data;
    got64[s] = (ULONG)riser_stub1;
    got74[s] = (ULONG)riser_stub5;
    hits_in();
    if (h) {
        Disable();
        CacheClearU();
        Supervisor((ULONG (*)())h);
        Enable();
    }
    hits_out(s);
}

static void interrupt2(const char *what, int level, UBYTE reqhi, UBYTE reqlo, UBYTE data)
{
    ROUT = what;
    sprintf(VEC, "$%02X%02X, $%02X", reqhi, reqlo, data);
    for (cur = 0; cur < 2; cur++)
        inthandler(cur, level, reqhi, reqlo, data);
    compare();
}

/* a byte arrives on set s (the receive interrupt) */
static void feed(int s, UBYTE b)
{
    inthandler(s, 5, 0x08, 0, b);
}

/* ------------------------------------------------- a task of its own */

#define CHSTACK 4096
struct child {
    struct Task task;
    APTR fn;
    struct regs r;
    ULONG arg, push;
    UBYTE stack[CHSTACK];
};
static struct child *ch;

static void childentry(void)
{
    callregs(ch->fn, &ch->r, ch->arg, ch->push);
    Signal(testtask, donesig);
    Wait(0);
}

static void child_start(int s, int idx, ULONG d0, ULONG d1, ULONG arg, ULONG push)
{
    memset(ch, 0, sizeof *ch);
    ch->fn = ENT(LIB, idx, 0);
    markregs(&ch->r);
    ch->r.d[0] = d0;
    ch->r.d[1] = d1;
    ch->arg = arg;
    ch->push = push;
    IN[s] = ch->r;
    POP[s] = 0;
    CCR[s] = 0;
    ch->task.tc_Node.ln_Type = NT_TASK;
    ch->task.tc_Node.ln_Pri = -1;
    ch->task.tc_Node.ln_Name = (char *)"riseriallib test";
    ch->task.tc_SPLower = ch->stack;
    ch->task.tc_SPUpper = ch->stack + CHSTACK;
    ch->task.tc_SPReg = ch->stack + CHSTACK;
    SetSignal(0, donesig);
    AddTask(&ch->task, (APTR)childentry, NULL);
}

/* the task's end (waiting at most 2 seconds if `wait`); TRUE if it
   returned */
static BOOL child_end(int s, int wait)
{
    BOOL done = FALSE;
    int i;
    for (i = 0; i < (wait ? 100 : 1) && !done; i++) {
        if (SetSignal(0, donesig) & donesig)
            done = TRUE;
        else
            Delay(1);
    }
    Forbid();
    RemTask(&ch->task);
    Permit();
    R[s] = ch->r;
    return done;
}

/* a call on set s alone, in the test task (while the other task runs) */
static void call1(int s, int idx)
{
    struct regs r;
    ULONG pop;
    markregs(&r);
    callstk(ENT(LIB, idx, 0), &r, ARGS[s], 0, &pop);
}

/* --------------------------------------------------------------- tests */

static void readfunc(const char *name, int idx, ULONG d0)
{
    sprintf(VEC, "d0 $%08lX", d0);
    CALL(name, idx, 0, R[cur].d[0] = d0);
    SAME("d0", R[cur].d[0]);
}

static void stmt(const char *name, int idx, int form, ULONG d0, ULONG d1)
{
    sprintf(VEC, "$%lX,$%lX", d0, d1);
    CALL(name, idx, form, R[cur].d[0] = d0; R[cur].d[1] = d1);
}

static void sendblock(const char *what, ULONG off, ULONG len)
{
    strcpy(VEC, what);
    CALL("SerialSendBlock", SENDBLOCK, 0,
         R[cur].d[0] = (ULONG)databuf[cur] + off; R[cur].d[1] = len);
    readfunc("SerialProgress", PROGRESS, 0);
}

/* 200 bytes in the 128-word transmit ring, no interrupt in between.
   BUG(orig) (#983): _writeser never tests whether the ring is full, byte 129
   overwrites the word of byte 1 (the oldest unsent one), and the index wrend
   runs on; in a real run the transmit interrupt sends a byte or two meanwhile
   and the receiver gets the end of the block, not the start.  Both builds. */
static void ringfull(void)
{
    UWORD wbig0[2], wrend0[2];

    for (cur = 0; cur < 2; cur++) {
        wbig0[cur] = *(UWORD *)wrp[cur];
        wrend0[cur] = *(UWORD *)(wrp[cur] + 2);
    }
    sendblock("200 bytes (ring wraps)", 0, 200);
    BOTH("200 bytes: wbig unchanged", *(UWORD *)wrp[cur] == wbig0[cur]);
    BOTH("200 bytes: wrend ran 400 on (BUG(orig), #983)",
         *(UWORD *)(wrp[cur] + 2) == (UWORD)((wrend0[cur] + 400) & 0xff));
    /* the slot byte 1 went to holds byte 129 (0xC1), not byte 1 (0x41) */
    BOTH("200 bytes: slot of byte 1 holds byte 129 (BUG(orig), #983)",
         *(UWORD *)(wrp[cur] + 4 + wrend0[cur]) == (UWORD)(0x0100 | databuf[cur][128]));
}

/* SerialSendString: d0 = a string with the same low address byte in
   both sets, a2 = longs to read downwards */
static void sendstring(const char *what)
{
    static ULONG stk[2][4];
    int s;
    for (s = 0; s < 2; s++) {
        stk[s][0] = 0xabcdef00;
        stk[s][1] = 0x12345681;
        stk[s][2] = 7;
        stk[s][3] = 0;
    }
    strcpy(VEC, what);
    CALL("SerialSendString", SENDSTR, 0,
         R[cur].d[0] = (ULONG)databuf[cur] + 0x141; R[cur].a[2] = (ULONG)&stk[cur][3]);
    BOTH("a2 not below the stack", R[cur].a[2] <= (ULONG)&stk[cur][3]
                                   && R[cur].a[2] >= (ULONG)&stk[cur][0]);
}

/* SerialReadString into databuf + 0x180 */
static void readstring(const char *what, ULONG d0)
{
    ULONG l0 = riser_landed;
    int s;
    strcpy(VEC, what);
    for (s = 0; s < 2; s++)
        memset(databuf[s] + 0x180, 0xee, 32);
    CALL("SerialReadString", READSTR, 0,
         R[cur].d[0] = d0 == 1 ? (ULONG)riser_landing : d0;
         R[cur].a[3] = (ULONG)databuf[cur] + 0x180);
    SAME("d0", R[cur].d[0]);
    SAME("a3", R[cur].a[3] - (ULONG)databuf[cur]);
    {
        const UBYTE *b[2] = { databuf[0] + 0x180, databuf[1] + 0x180 };
        SAMEBYTES("string", b, 32);
    }
    check("riseriallib: SerialReadString returned to d0 (open) / normally",
          riser_landed - l0 == (d0 == 1 ? 2 : 0));
}

/* until the task has read every byte of set s (at most a second), then
   one tick more: it then waits in its read loop (it runs only while the
   test task waits, at a lower priority) */
static BOOL settle(int s)
{
    int i;
    for (i = 0; i < 50; i++) {
        if (getl(rdp[s]) >> 16 == (getl(rdp[s]) & 0xffff))
            break;
        Delay(1);
    }
    Delay(1);
    return i < 50;
}

/* SerialReceiveBlock in its own task: `pre` bytes there before (thrown
   away), then `n` bytes one by one; stop: SerialStop after 3 */
static void receive(const char *what, ULONG len, int n, int stop)
{
    static const UBYTE bytes[] = { 0x52, 0x00, 0xff, 0x13, 0x37, 0x80, 0x01, 0x7f };
    BOOL ret[2], ok[2];
    int s, i;
    strcpy(VEC, what);
    ROUT = "SerialReceiveBlock";
    for (s = 0; s < 2; s++) {
        cur = s;
        memset(databuf[s] + 0x100, 0xee, 64);
        feed(s, 0x11);
        feed(s, 0x22);
        hits_in();
        child_start(s, RECVBLOCK, (ULONG)databuf[s] + 0x100, len, (ULONG)riser_landpop, stop);
        ok[s] = settle(s);              /* thrown away */
        for (i = 0; i < n; i++) {
            if (stop && i == 3)
                call1(s, STOP);
            feed(s, bytes[i % sizeof bytes]);
            if (!stop || i <= 3)        /* the 5th stays after the stop */
                ok[s] &= settle(s);
        }
        ret[s] = child_end(s, 1);
        hits_out(s);
        got64[s] = (ULONG)riser_stub1;
        got74[s] = (ULONG)riser_stub5;
        fillfake(s);
    }
    BOTH("every byte read in time", ok[cur]);
    BOTH("returned", ret[cur]);
    preserve();
    compare();
    {
        const UBYTE *b[2] = { databuf[0] + 0x100, databuf[1] + 0x100 };
        SAMEBYTES("bytes received", b, 64);
    }
    if (stop)
        SAME("d7 (its return address)", R[cur].d[7]);
    readfunc("SerialProgress", PROGRESS, 0);
}

/* SerialReadLong (never returns) in its own task, n bytes */
static void readlong(int n)
{
    static const UBYTE bytes[] = { 0x10, 0x20, 0x30, 0x40, 0x50, 0x60 };
    BOOL ret[2], empty[2];
    int s, i;
    strcpy(VEC, "");
    ROUT = "SerialReadLong";
    for (s = 0; s < 2; s++) {
        cur = s;
        hits_in();
        child_start(s, READLONG, 0, 0, 0, 0);
        Delay(2);
        for (i = 0; i < n; i++) {
            feed(s, bytes[i]);
            empty[s] = settle(s);
        }
        ret[s] = child_end(s, 0);
        hits_out(s);
        got64[s] = (ULONG)riser_stub1;
        got74[s] = (ULONG)riser_stub5;
        fillfake(s);
    }
    BOTH("never returns", !ret[cur]);
    BOTH("every byte read", empty[cur]);
    compare();
}

int main(int argc, char **argv)
{
    static const char *names[] = { "ldivlib", LIB, NULL };
    ULONG v64, v74;
    int s, n[2], i;

    if (argc < 2) {
        printf("usage: riseriallib DIR\n");
        return 20;
    }
    if (!load_sets(argv[1], names))
        return 20;
    testtask = FindTask(NULL);
    {
        BYTE b = AllocSignal(-1);
        if (b < 0) {
            check("riseriallib: a signal", FALSE);
            return rwe_done(LIB);
        }
        donesig = 1UL << b;
    }
    ch = AllocMem(sizeof *ch, MEMF_PUBLIC | MEMF_CLEAR);
    for (s = 0; s < 2; s++) {
        fake[s] = AllocMem(FAKESZ, MEMF_PUBLIC | MEMF_CLEAR);
        fakecia[s] = AllocMem(4, MEMF_PUBLIC | MEMF_CLEAR);
        dataraw[s] = AllocMem(512 + 256, MEMF_PUBLIC | MEMF_CLEAR);
        databuf[s] = dataraw[s] ? (UBYTE *)(((ULONG)dataraw[s] + 255) & ~255UL) : NULL;
        n[s] = fake[s] && fakecia[s] ? patch(getlib(&sets[s], LIB), fake[s], fakecia[s]) : 0;
    }
    CacheClearU();
    printf("riseriallib: patched custom/CIA addresses: orig %d/%d, C %d/%d\n",
           n[0] & 0xff, n[0] >> 8, n[1] & 0xff, n[1] >> 8);
    check("riseriallib: custom chip addresses patched", (n[0] & 0xff) == 13 && (n[1] & 0xff) == 1);
    check("riseriallib: CIA-B PRA patched", (n[0] >> 8) == 2 && (n[1] >> 8) == 1);
    if (!ch || !databuf[0] || !databuf[1] || n[0] != 0x20d || n[1] != 0x101)
        return rwe_done(LIB);
    v64 = *vec(0x64);
    v74 = *vec(0x74);
    for (s = 0; s < 2; s++)
        for (i = 0; i < 16; i++)
            databuf[s][i] = (UBYTE)(0x41 + i * 7);

    /* the receive ring: in front of SerialBuffer's address */
    strcpy(VEC, "");
    CALL("SerialBuffer", BUFFER, 0, (void)0);
    for (s = 0; s < 2; s++)
        if (incode(s, R[s].d[0]))
            rdp[s] = (UBYTE *)R[s].d[0] - 4;
    BOTH("SerialBuffer in the library", rdp[cur] != NULL);
    if (!rdp[0] || !rdp[1])
        return rwe_done(LIB);
    readfunc("SerialVersion", VERSION, 0);
    readfunc("SerialGot", GOT, 0);
    readfunc("SerialRead", READ, 0x12345678);
    readfunc("SerialProgress", PROGRESS, 0);
    stmt("SerialStop", STOP, 0, 0, 0);
    stmt("SerialBaud", SETBAUD, 0, 9600, 0);
    stmt("SerialBaud", SETBAUD, 0, 1, 0);
    stmt("SerialBaud", SETBAUD, 0, 2 * 3546895, 0);

    /* open: the handlers */
    stmt("SerialOpen", OPEN, 0, 0, 0);
    for (s = 0; s < 2; s++) {
        h1[s] = got64[s];
        h5[s] = got74[s];
    }
    BOTH("SerialOpen: handler $64", incode(cur, h1[cur]));
    BOTH("SerialOpen: handler $74", incode(cur, h5[cur]));
    if (!incode(0, h1[0]) || !incode(1, h1[1]) || !incode(0, h5[0]) || !incode(1, h5[1]))
        return rwe_done(LIB);

    /* the transmit ring: wbig 0, wrend 6, $01A1 $01B2 $01C3 */
    stmt("SerialWrite", WRITE, 0, 0x5a5a00a1, 0);
    stmt("SerialWrite", WRITE, 0, 0xb2, 0);
    stmt("SerialWrite", WRITE, 0, 0xffffffc3, 0);
    for (s = 0; s < 2; s++) {
        static const UBYTE sig[10] = { 0, 0, 0, 6, 1, 0xa1, 1, 0xb2, 1, 0xc3 };
        struct lib *l = getlib(&sets[s], LIB);
        ULONG o;
        for (o = 0; o + sizeof sig <= l->size; o += 2)
            if (!memcmp(l->code + o, sig, sizeof sig)) {
                wrp[s] = l->code + o;
                break;
            }
    }
    BOTH("transmit ring found", wrp[cur] != NULL);
    compare();

    /* the transmit interrupt: three words, then an empty ring, then not
       TBE (chained) */
    for (i = 0; i < 4; i++)
        interrupt2("transmit interrupt", 1, 0, 0x01, 0);
    interrupt2("transmit interrupt (not TBE)", 1, 0x08, 0x00, 0);
    /* the receive interrupt: bytes, and not RBF (chained) */
    interrupt2("receive interrupt", 5, 0x08, 0, 'H');
    interrupt2("receive interrupt", 5, 0x08, 0, 0);
    interrupt2("receive interrupt", 5, 0x08, 0, 0xff);
    interrupt2("receive interrupt (not RBF)", 5, 0x00, 0x01, 0x55);

    readfunc("SerialGot", GOT, 0);
    readfunc("SerialRead", READ, 0x12345678);
    readfunc("SerialRead", READ, 0xffff0000);
    readfunc("SerialWait", WAIT, 0x0000ffff);
    readfunc("SerialRead", READ, 0);
    readfunc("SerialGot", GOT, 0);
    for (s = 0; s < 2; s++)
        for (i = 0; i < 3; i++)
            feed(s, (UBYTE)(0x30 + i));
    stmt("SerialFlush", FLUSH, 0, 0x12345678, 0);
    readfunc("SerialGot", GOT, 0);

    /* blocks */
    sendblock("5 bytes", 0, 5);
    sendblock("length 1", 5, 1);
    sendblock("length 0", 6, 0);
    sendblock("length -3", 7, (ULONG)-3);
    stmt("SerialStop", STOP, 0, 0, 0);
    sendblock("stopped", 8, 5);
    ringfull();

    /* strings and longs with the port open */
    stmt("SerialStop", STOP, 0, 0, 0);
    sendstring("open");
    sendblock("after SerialSendString (stop flag cleared)", 2, 2);
    for (s = 0; s < 2; s++) {
        feed(s, 'O');
        feed(s, 'K');
        feed(s, 0);
        feed(s, 'x');
    }
    readstring("open: returns to d0", 1);
    stmt("SerialWriteLong", WRITELONG, 0, 0x11223344, 0);
    stmt("SerialStop", STOP, 0, 0, 0);
    stmt("SerialOpen", OPEN, 1, 19200, 0);      /* open: only SERPER, the stop flag */
    sendblock("after SerialOpen", 3, 2);
    stmt("SerialFlush", FLUSH, 0, 0, 0);

    /* in a task of their own */
    receive("6 bytes", 6, 6, 0);
    receive("2 bytes", 2, 2, 0);
    receive("length 1, SerialStop", 1, 5, 1);
    readlong(5);

    /* close */
    stmt("SerialClose", CLOSE, 0, 0, 0);
    stmt("SerialClose", CLOSE, 0, 0, 0);
    interrupt2("receive interrupt (closed)", 5, 0x08, 0, 0x99);
    stmt("SerialFlush", FLUSH, 0, 0, 0);

    /* closed */
    for (s = 0; s < 2; s++) {
        feed(s, 'A');
        feed(s, 'B');
        feed(s, 0);
        feed(s, 'C');
        feed(s, 0);
    }
    readstring("closed", 0);
    readstring("closed, again (the count goes on)", 0);
    sendstring("closed");
    stmt("SerialStop", STOP, 0, 0, 0);
    sendstring("closed, stop flag set");
    sendblock("after SerialSendString (stop flag still set)", 4, 3);
    stmt("SerialWriteLong", WRITELONG, 0, 0xa1b2c3d4, 0);     /* opens */
    stmt("finit", FINIT, 0, 0, 0);
    stmt("finit", FINIT, 0, 0, 0);
    stmt("SerialOpen", OPEN, 1, 2400, 0);
    for (s = 0; s < 2; s++)
        for (i = 0; i < 300; i++)
            feed(s, (UBYTE)(i * 13));
    CALL("receive ring wrapped", GOT, 0, (void)0);
    stmt("SerialFlush", FLUSH, 0, 0, 0);
    stmt("SerialClose", CLOSE, 0, 0, 0);

    check("riseriallib: the system's vectors unchanged", *vec(0x64) == v64 && *vec(0x74) == v74);
    FreeSignal(__builtin_ctzl(donesig));
    FreeMem(ch, sizeof *ch);
    for (s = 0; s < 2; s++) {
        FreeMem(fake[s], FAKESZ);
        FreeMem(fakecia[s], 4);
        FreeMem(dataraw[s], 512 + 256);
    }
    return rwe_done(LIB);
}
