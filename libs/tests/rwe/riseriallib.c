/*
 * rwe riseriallib - the serial port library with its own interrupts: the
 * C build or the reference build of rwelibsrc/seriallib/
 * Serial-Library.bb (riseriallib.orig), one set as LIBSET says, observed
 * as cases (rwecmp.h; the expected data
 * libs/tests/expected/rwe_riseriallib.txt).
 *
 *   riseriallib DIR     (DIR with ldivlib and riseriallib, .obj/.orig)
 *
 * The library writes the custom chip registers and CIA-B PRA itself and
 * its handlers into the autovectors $64/$74; SerialClose switches every
 * interrupt off (INTENA $7FFF).  None of that may reach the machine:
 *   - every absolute address $DFF000-$DFF1FF and $BFD000 in the loaded
 *     library is patched to a RAM copy (the custom registers filled with
 *     a fixed state before each call: SERDATR with TBE, INTENAR $2A35,
 *     INTREQR 0), observed after each call.  These are the 13 custom chip
 *     and 2 CIA-B operands in the original's code and the one long of
 *     each base in the C build (riseriallib.c riser_custom, riser_ciab),
 *     whatever code GCC makes of the accesses: the numbers of the run's
 *     set are checked;
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
 * Observed after every call: the fake registers, CIA-B PRA, the vectors,
 * the receive ring (rbig, rend, buffer - in front of SerialBuffer's
 * address in both builds) and the transmit ring (wbig, wrend, buffer -
 * found by the first three bytes written), d0 of the functions, and the
 * kept registers.  At the end the system's vectors are checked.
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

static UBYTE *fake, *fakecia;
static UBYTE *rdp, *wrp;                /* the rings */
static ULONG h1, h5;                    /* the handlers the library installed */
static ULONG got64, got74;              /* the vectors after the last call */
static ULONG hits[2];                   /* the stubs called */
static ULONG hb1, hb5;
ULONG riser_hits1, riser_hits5, riser_landed;
static UBYTE *databuf;                  /* 512 bytes at a multiple of 256 */
static UBYTE *dataraw;
static struct Task *testtask;
static ULONG donesig;
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

static void fillfake(void)
{
    memset(fake, 0, FAKESZ);
    fake[0x18] = 0x20;               /* SERDATR: TBE */
    fake[0x1c] = 0x2a;               /* INTENAR (INTEN off: SerialClose sets it) */
    fake[0x1d] = 0x35;
    fakecia[0] = 0x5a;
}

static int incode(ULONG a)
{
    struct lib *l = getlib(&loaded, LIB);
    return a >= (ULONG)l->code && a < (ULONG)l->code + l->size;
}

/* a vector as seen after a call: 0 the stub, 1 in the library, 2 else */
static ULONG norm(ULONG v, APTR stub)
{
    return v == (ULONG)stub ? 0 : incode(v) ? 1 : 2;
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

static void vecs_out(void)
{
    got64 = *vec(0x64);
    got74 = *vec(0x74);
    *vec(0x64) = sys64;
    *vec(0x74) = sys74;
    Enable();
}

static void hits_in(void)
{
    hb1 = riser_hits1;
    hb5 = riser_hits5;
}

static void hits_out(void)
{
    hits[0] = riser_hits1 - hb1;
    hits[1] = riser_hits5 - hb5;
}

static void compare(void)
{
    OBSBYTES("custom chip registers", fake, FAKESZ);
    OBS("CIA-B PRA", fakecia[0]);
    OBS("vector $64", norm(got64, riser_stub1));
    OBS("vector $74", norm(got74, riser_stub5));
    if (rdp)
        OBSBYTES("receive ring", rdp, RINGSZ);
    if (wrp)
        OBSBYTES("transmit ring", wrp, RINGSZ);
    OBS("stub $64 called", hits[0]);
    OBS("stub $74 called", hits[1]);
}

/* one call, inside Disable() with the test's vectors */
#define CALL(name, idx, form, setup) do {                                   \
        APTR fn_ = ENT(LIB, (idx), (form));                                 \
        ROUT = (name);                                                      \
        markregs(&R);                                                       \
        NARGS = 0;                                                          \
        setup;                                                              \
        IN = R;                                                             \
        fillfake();                                                         \
        hits_in();                                                          \
        if (!fn_) {                                                         \
            check("entry missing", FALSE);                                  \
            break;                                                          \
        }                                                                   \
        vecs_in();                                                          \
        CCR = callstk(fn_, &R, ARGS, NARGS, &POP);                          \
        vecs_out();                                                         \
        hits_out();                                                         \
        CacheClearU();                                                      \
        preserve();                                                         \
        compare();                                                          \
    } while (0)

/* a handler by hand (Supervisor, RTE): `level` 1 or 5, the fake INTREQR
   and the byte in SERDATR */
static void inthandler(int level, UBYTE reqhi, UBYTE reqlo, UBYTE data)
{
    ULONG h = level == 5 ? h5 : h1;
    fillfake();
    fake[0x1e] = reqhi;
    fake[0x1f] = reqlo;
    fake[0x19] = data;
    got64 = (ULONG)riser_stub1;
    got74 = (ULONG)riser_stub5;
    hits_in();
    if (h) {
        Disable();
        CacheClearU();
        Supervisor((ULONG (*)())h);
        Enable();
    }
    hits_out();
}

static void interrupt1(const char *what, int level, UBYTE reqhi, UBYTE reqlo, UBYTE data)
{
    ROUT = what;
    sprintf(VEC, "$%02X%02X, $%02X", reqhi, reqlo, data);
    inthandler(level, reqhi, reqlo, data);
    compare();
}


/* a byte arrives (the receive interrupt) */
static void feed(UBYTE b)
{
    inthandler(5, 0x08, 0, b);
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

static void child_start(int idx, ULONG d0, ULONG d1, ULONG arg, ULONG push)
{
    memset(ch, 0, sizeof *ch);
    ch->fn = ENT(LIB, idx, 0);
    markregs(&ch->r);
    ch->r.d[0] = d0;
    ch->r.d[1] = d1;
    ch->arg = arg;
    ch->push = push;
    IN = ch->r;
    POP = 0;
    CCR = 0;
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
static BOOL child_end(int wait)
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
    R = ch->r;
    return done;
}

/* a call in the test task (while the other task runs) */
static void call_here(int idx)
{
    struct regs r;
    ULONG pop;
    markregs(&r);
    callstk(ENT(LIB, idx, 0), &r, ARGS, 0, &pop);
}

/* --------------------------------------------------------------- tests */

static void readfunc(const char *name, int idx, ULONG d0)
{
    sprintf(VEC, "d0 $%08lX", d0);
    CALL(name, idx, 0, R.d[0] = d0);
    OBS("d0", R.d[0]);
}

static void stmt(const char *name, int idx, int form, ULONG d0, ULONG d1)
{
    sprintf(VEC, "$%lX,$%lX", d0, d1);
    CALL(name, idx, form, R.d[0] = d0; R.d[1] = d1);
}

static void sendblock(const char *what, ULONG off, ULONG len)
{
    strcpy(VEC, what);
    CALL("SerialSendBlock", SENDBLOCK, 0,
         R.d[0] = (ULONG)databuf + off; R.d[1] = len);
    readfunc("SerialProgress", PROGRESS, 0);
}

/* 200 bytes in the 128-word transmit ring, no interrupt in between.
   BUG(orig) (#983): _writeser never tests whether the ring is full, byte 129
   overwrites the word of byte 1 (the oldest unsent one), and the index wrend
   runs on; in a real run the transmit interrupt sends a byte or two meanwhile
   and the receiver gets the end of the block, not the start.  Both builds. */
static void ringfull(void)
{
    UWORD wbig0 = *(UWORD *)wrp, wrend0 = *(UWORD *)(wrp + 2);

    sendblock("200 bytes (ring wraps)", 0, 200);
    BOTH("200 bytes: wbig unchanged", *(UWORD *)wrp == wbig0);
    BOTH("200 bytes: wrend ran 400 on (BUG(orig), #983)",
         *(UWORD *)(wrp + 2) == (UWORD)((wrend0 + 400) & 0xff));
    /* the slot byte 1 went to holds byte 129 (0xC1), not byte 1 (0x41) */
    BOTH("200 bytes: slot of byte 1 holds byte 129 (BUG(orig), #983)",
         *(UWORD *)(wrp + 4 + wrend0) == (UWORD)(0x0100 | databuf[128]));
}

/* SerialSendString: d0 = a string at a fixed low address byte, a2 =
   longs to read downwards */
static void sendstring(const char *what)
{
    static ULONG stk[4];
    stk[0] = 0xabcdef00;
    stk[1] = 0x12345681;
    stk[2] = 7;
    stk[3] = 0;
    strcpy(VEC, what);
    CALL("SerialSendString", SENDSTR, 0,
         R.d[0] = (ULONG)databuf + 0x141; R.a[2] = (ULONG)&stk[3]);
    BOTH("a2 not below the stack", R.a[2] <= (ULONG)&stk[3] && R.a[2] >= (ULONG)&stk[0]);
}

/* SerialReadString into databuf + 0x180 */
static void readstring(const char *what, ULONG d0)
{
    ULONG l0 = riser_landed;
    strcpy(VEC, what);
    memset(databuf + 0x180, 0xee, 32);
    CALL("SerialReadString", READSTR, 0,
         R.d[0] = d0 == 1 ? (ULONG)riser_landing : d0;
         R.a[3] = (ULONG)databuf + 0x180);
    OBS("d0", R.d[0]);
    OBS("a3", R.a[3] - (ULONG)databuf);
    OBSBYTES("string", databuf + 0x180, 32);
    check("riseriallib: SerialReadString returned to d0 (open) / normally",
          riser_landed - l0 == (d0 == 1 ? 1 : 0));
}

/* until the task has read every byte (at most a second), then one tick
   more: it then waits in its read loop (it runs only while the test task
   waits, at a lower priority) */
static BOOL settle(void)
{
    int i;
    for (i = 0; i < 50; i++) {
        if (getl(rdp) >> 16 == (getl(rdp) & 0xffff))
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
    BOOL ret, ok;
    int i;
    strcpy(VEC, what);
    ROUT = "SerialReceiveBlock";
    memset(databuf + 0x100, 0xee, 64);
    feed(0x11);
    feed(0x22);
    hits_in();
    child_start(RECVBLOCK, (ULONG)databuf + 0x100, len, (ULONG)riser_landpop, stop);
    ok = settle();                      /* thrown away */
    for (i = 0; i < n; i++) {
        if (stop && i == 3)
            call_here(STOP);
        feed(bytes[i % sizeof bytes]);
        if (!stop || i <= 3)            /* the 5th stays after the stop */
            ok &= settle();
    }
    ret = child_end(1);
    hits_out();
    got64 = (ULONG)riser_stub1;
    got74 = (ULONG)riser_stub5;
    fillfake();
    BOTH("every byte read in time", ok);
    BOTH("returned", ret);
    preserve();
    compare();
    OBSBYTES("bytes received", databuf + 0x100, 64);
    if (stop)
        OBSPTR("d7 (its return address)", R.d[7], NULL, 0);
    readfunc("SerialProgress", PROGRESS, 0);
}

/* SerialReadLong (never returns) in its own task, n bytes */
static void readlong(int n)
{
    static const UBYTE bytes[] = { 0x10, 0x20, 0x30, 0x40, 0x50, 0x60 };
    BOOL ret, empty = FALSE;
    int i;
    strcpy(VEC, "");
    ROUT = "SerialReadLong";
    hits_in();
    child_start(READLONG, 0, 0, 0, 0);
    Delay(2);
    for (i = 0; i < n; i++) {
        feed(bytes[i]);
        empty = settle();
    }
    ret = child_end(0);
    hits_out();
    got64 = (ULONG)riser_stub1;
    got74 = (ULONG)riser_stub5;
    fillfake();
    BOTH("never returns", !ret);
    BOTH("every byte read", empty);
    compare();
}

int main(int argc, char **argv)
{
    static const char *names[] = { "ldivlib", LIB, NULL };
    ULONG v64, v74;
    int n, n0, i;

    if (argc < 2) {
        printf("usage: riseriallib DIR\n");
        return 20;
    }
    if (!load_set(argv[1], names))
        return 20;
    KEEPS("SerialBuffer", d1-d7|a1-a3|a6);
    KEEPS("SerialVersion", d1-d7|a0-a3|a6);
    KEEPS("SerialGot", d1-d7|a0-a3|a6);
    KEEPS("SerialRead", d1-d7|a0-a3|a6);
    KEEPS("SerialProgress", d1-d7|a0-a3|a6);
    KEEPS("SerialStop", d0-d7|a0-a3|a6);
    KEEPS("SerialBaud", d1-d7|a0-a3|a6);
    KEEPS("SerialOpen", d1-d7|a0-a3|a6);
    KEEPS("SerialWrite", d2-d7|a1-a3|a6);
    KEEPS("SerialWait", d1-d7|a0-a3|a6);
    KEEPS("SerialFlush", d1-d7|a0-a3|a6);
    KEEPS("SerialSendBlock", d2-d7|a1-a3|a6);
    KEEPS("SerialSendString", d2-d7|a1|a3|a6);
    KEEPS("SerialReadString", d1-d7|a0-a2|a6);
    KEEPS("SerialWriteLong", d3-d7|a1-a3|a6);
    KEEPS("SerialReceiveBlock", d2-d6|a1-a3|a6);
    KEEPS("SerialClose", d1-d7|a0-a3|a6);
    KEEPS("finit", d1-d7|a0-a3|a6);
    KEEPS("receive ring wrapped", d1-d7|a0-a3|a6);
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
    fake = AllocMem(FAKESZ, MEMF_PUBLIC | MEMF_CLEAR);
    fakecia = AllocMem(4, MEMF_PUBLIC | MEMF_CLEAR);
    dataraw = AllocMem(512 + 256, MEMF_PUBLIC | MEMF_CLEAR);
    databuf = dataraw ? (UBYTE *)(((ULONG)dataraw + 255) & ~255UL) : NULL;
    n = fake && fakecia ? patch(getlib(&loaded, LIB), fake, fakecia) : 0;
    CacheClearU();
    printf("riseriallib: patched custom/CIA addresses: %d/%d\n", n & 0xff, n >> 8);
    /* the operands of the set: 13 custom chip and 2 CIA-B ones in the
       original's code, the one long of each base in the C build */
    n0 = !strcmp(bl_libset, "orig") ? 0x20d : 0x101;
    check("riseriallib: custom chip addresses patched", (n & 0xff) == (n0 & 0xff));
    check("riseriallib: CIA-B PRA patched", (n >> 8) == n0 >> 8);
    if (!ch || !databuf || n != n0)
        return rwe_done(LIB);
    v64 = *vec(0x64);
    v74 = *vec(0x74);
    for (i = 0; i < 16; i++)
        databuf[i] = (UBYTE)(0x41 + i * 7);

    /* the receive ring: in front of SerialBuffer's address */
    strcpy(VEC, "");
    CALL("SerialBuffer", BUFFER, 0, (void)0);
    if (incode(R.d[0]))
        rdp = (UBYTE *)R.d[0] - 4;
    BOTH("SerialBuffer in the library", rdp != NULL);
    if (!rdp)
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
    h1 = got64;
    h5 = got74;
    BOTH("SerialOpen: handler $64", incode(h1));
    BOTH("SerialOpen: handler $74", incode(h5));
    if (!incode(h1) || !incode(h5))
        return rwe_done(LIB);

    /* the transmit ring: wbig 0, wrend 6, $01A1 $01B2 $01C3 */
    stmt("SerialWrite", WRITE, 0, 0x5a5a00a1, 0);
    stmt("SerialWrite", WRITE, 0, 0xb2, 0);
    stmt("SerialWrite", WRITE, 0, 0xffffffc3, 0);
    {
        static const UBYTE sig[10] = { 0, 0, 0, 6, 1, 0xa1, 1, 0xb2, 1, 0xc3 };
        struct lib *l = getlib(&loaded, LIB);
        ULONG o;
        for (o = 0; o + sizeof sig <= l->size; o += 2)
            if (!memcmp(l->code + o, sig, sizeof sig)) {
                wrp = l->code + o;
                break;
            }
    }
    BOTH("transmit ring found", wrp != NULL);
    compare();

    /* the transmit interrupt: three words, then an empty ring, then not
       TBE (chained) */
    for (i = 0; i < 4; i++)
        interrupt1("transmit interrupt", 1, 0, 0x01, 0);
    interrupt1("transmit interrupt (not TBE)", 1, 0x08, 0x00, 0);
    /* the receive interrupt: bytes, and not RBF (chained) */
    interrupt1("receive interrupt", 5, 0x08, 0, 'H');
    interrupt1("receive interrupt", 5, 0x08, 0, 0);
    interrupt1("receive interrupt", 5, 0x08, 0, 0xff);
    interrupt1("receive interrupt (not RBF)", 5, 0x00, 0x01, 0x55);

    readfunc("SerialGot", GOT, 0);
    readfunc("SerialRead", READ, 0x12345678);
    readfunc("SerialRead", READ, 0xffff0000);
    readfunc("SerialWait", WAIT, 0x0000ffff);
    readfunc("SerialRead", READ, 0);
    readfunc("SerialGot", GOT, 0);
    for (i = 0; i < 3; i++)
        feed((UBYTE)(0x30 + i));
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
    feed('O');
    feed('K');
    feed(0);
    feed('x');
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
    interrupt1("receive interrupt (closed)", 5, 0x08, 0, 0x99);
    stmt("SerialFlush", FLUSH, 0, 0, 0);

    /* closed */
    feed('A');
    feed('B');
    feed(0);
    feed('C');
    feed(0);
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
    for (i = 0; i < 300; i++)
        feed((UBYTE)(i * 13));
    CALL("receive ring wrapped", GOT, 0, (void)0);
    stmt("SerialFlush", FLUSH, 0, 0, 0);
    stmt("SerialClose", CLOSE, 0, 0, 0);

    check("riseriallib: the system's vectors unchanged", *vec(0x64) == v64 && *vec(0x74) == v74);
    FreeSignal(__builtin_ctzl(donesig));
    FreeMem(ch, sizeof *ch);
    FreeMem(fake, FAKESZ);
    FreeMem(fakecia, 4);
    FreeMem(dataraw, 512 + 256);
    return rwe_done(LIB);
}
