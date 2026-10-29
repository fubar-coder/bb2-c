/*
 * rwe rifastseriallib - the C build of the RWE userlib rifastseriallib
 * (libs/userlibs/rwe/rifastseriallib.c) against the CD binary BSS2
 * RIFASTSERIALLIB.OBJ (rifastseriallib.orig).
 *
 *   rifastseriallib DIR     (DIR: rifastseriallib.obj and .orig)
 *
 * The library drives the serial port registers directly.  The emulator
 * has no serial connection: SERDATR always shows TBE (a byte written is
 * gone at once) and RBF never comes (Amiberry does not let a program set
 * it through INTREQ either), so everything that sends can run, nothing
 * that waits for a byte can.  Compared on both sets with the same inputs
 * (other registers = markers): d0 of the functions, every register the
 * original keeps, INTENAR (InitSerialIO switches TBE/RBF off - called
 * with both on inside Disable(); FinitSerialIO must not change it nor the
 * word at $9A, BUG(orig)) and the library's state block: mode, numbytes,
 * inbuf, outbuf, baud (= the SERPER value written, which cannot be read
 * back), inten ($F01C, BUG(orig)) and temp.  The C build keeps the
 * original's layout of these 18 bytes; the test finds them in both
 * libraries after the first InitSerialIO by its inbuf/outbuf values.
 * Covered: CheckSerial (no byte: -1), FlushSerialIO, SetSerialMode,
 * EndSerialIO, SetIOBuffers, SetIOSize, SetIOSpeed (also with garbage in
 * the upper word: DIVU.L by all of d0), PutSerial byte and address,bytes
 * before (nothing) and after InitSerialIO, InitSerialIO twice (the second
 * does nothing) and with a baud rate that gives SERPER 0 (stays "not
 * initialised"), FinitSerialIO, init/finit.
 * Not covered (they wait for a received byte, RBF): GetSerial (both
 * forms), DoSerialIO, SyncPut, SyncGet, SyncSerial, CheckSerial with a
 * byte; the bytes on the line (no remote station).
 */
#include "sysload.h"
#include <hardware/custom.h>

#define CUSTOM (*(volatile struct Custom *)0xdff000)

static const char *names[] = { "rifastseriallib", NULL };

enum { INITSERIALIO, FINITSERIALIO, SETSERIALMODE, ENDSERIALIO, PUTSERIAL, GETSERIAL,
       FLUSHSERIALIO, SYNCSERIAL, DOSERIALIO, SETIOBUFFERS, SETIOSPEED, SETIOSIZE,
       CHECKSERIAL, SYNCPUT, SYNCGET, INIT, FINIT };

static int cur;
static struct regs R[2], IN[2];
static const char *ROUT;
static char VEC[100];
static int prints;

static void fail(const char *what, ULONG o, ULONG c)
{
    char buf[200];
    if (prints++ < 60)
        printf("FAIL %s %s %s: orig $%08lX, C $%08lX\n", ROUT, VEC, what, o, c);
    snprintf(buf, sizeof buf, "%s %s %s", ROUT, VEC, what);
    check(buf, FALSE);
}

#define SAME(what, expr) do {                                               \
        ULONG v_[2];                                                        \
        for (cur = 0; cur < 2; cur++) v_[cur] = (ULONG)(expr);              \
        if (v_[0] != v_[1]) fail(what, v_[0], v_[1]); else checks++;        \
    } while (0)

static void preserve(void)
{
    struct call c[2];
    char buf[140];
    int s;
    for (s = 0; s < 2; s++) {
        c[s].in = IN[s];
        c[s].out = R[s];
    }
    sprintf(buf, "%s %s", ROUT, VEC);
    cmp_preserve(buf, &c[0], &c[1]);
}

static struct lib *thelib(void)
{
    return getlib(&sets[cur], names[0]);
}

static APTR ent(int idx, int form)
{
    return (APTR)((ULONG)entry_form(thelib(), idx, form) & ~1UL);
}

/* INTENAR after the call, made with TBE and RBF interrupts on inside
   Disable() (switched off again before Enable()) */
static UWORD intenar[2];

#define DO2(idx, form, setup) do {                                          \
        for (cur = 0; cur < 2; cur++) {                                     \
            markregs(&R[cur]);                                              \
            setup;                                                          \
            IN[cur] = R[cur];                                               \
            Disable();                                                      \
            CUSTOM.intena = 0x8801;                                         \
            callregs(ent(idx, form), &R[cur], 0, 0);                        \
            intenar[cur] = CUSTOM.intenar;                                  \
            CUSTOM.intena = 0x0801;                                         \
            Enable();                                                       \
        }                                                                   \
        preserve();                                                         \
    } while (0)

/* the state block of the library (found after the first InitSerialIO) */
struct fsstate {
    UWORD mode, numbytes;
    ULONG inbuf, outbuf;
    UWORD baud, inten, temp;
};
static struct fsstate *st[2];
/* the word at $9A (through a variable: a constant address would be a
   null pointer offset for GCC) and its value at the start */
static volatile UWORD *addr9a;
static UWORD vec9a;

#define INBUF  0x12345678UL
#define OUTBUF 0x9abcdef0UL

static void same_state(void)
{
    SAME("INTENAR", intenar[cur] & 0x0801);
    SAME("word at $9A unchanged", *addr9a == vec9a);
    if (!st[0] || !st[1])
        return;
    SAME("mode", st[cur]->mode & 0xff00);
    SAME("numbytes", st[cur]->numbytes);
    SAME("inbuf", st[cur]->inbuf);
    SAME("outbuf", st[cur]->outbuf);
    SAME("baud (SERPER)", st[cur]->baud);
    SAME("inten", st[cur]->inten);
    SAME("temp", st[cur]->temp & 0xff00);
}

static void none(int idx, const char *name, int result)
{
    ROUT = name;
    strcpy(VEC, "");
    DO2(idx, 0, (void)0);
    if (result)
        SAME("d0", R[cur].d[0]);
    same_state();
}

static void one(int idx, const char *name, ULONG a)
{
    ROUT = name;
    sprintf(VEC, "$%lX", a);
    DO2(idx, 0, R[cur].d[0] = a);
    same_state();
}

static void initserialio(ULONG baud, ULONG n, ULONG in, ULONG out)
{
    ROUT = "InitSerialIO";
    sprintf(VEC, "%lu,%lu,$%lX,$%lX", baud, n, in, out);
    DO2(INITSERIALIO, 0, R[cur].d[0] = baud; R[cur].d[1] = 0xA5A50000UL | n;
        R[cur].d[2] = in; R[cur].d[3] = out);
    SAME("d0", R[cur].d[0]);
    same_state();
}

static void putblock(int set, UBYTE *p, ULONG n)
{
    ROUT = "PutSerial address,bytes";
    sprintf(VEC, "%lu (%d)", n, set);
    DO2(PUTSERIAL, 1, R[cur].d[0] = (ULONG)p; R[cur].d[1] = n);
    same_state();
}

/* find the state block: inbuf, outbuf at +4 */
static struct fsstate *findstate(struct lib *l)
{
    ULONG i;
    for (i = 4; i + 14 <= l->size; i += 2)
        if (*(ULONG *)(l->code + i) == INBUF && *(ULONG *)(l->code + i + 4) == OUTBUF)
            return (struct fsstate *)(l->code + i - 4);
    return NULL;
}

int main(int argc, char **argv)
{
    static UBYTE data[16] = "fast serial io!";
    UWORD ena;

    if (argc < 2) {
        printf("usage: rifastseriallib DIR\n");
        return 20;
    }
    if (!load_sets(argv[1], names))
        return 20;
    __asm__("" : "=a"(addr9a) : "0"(0x9a));
    vec9a = *addr9a;
    ena = CUSTOM.intenar;

    ROUT = "init";
    strcpy(VEC, "");
    DO2(INIT, 0, (void)0);

    /* before InitSerialIO */
    none(CHECKSERIAL, "CheckSerial", 1);
    none(FLUSHSERIALIO, "FlushSerialIO", 0);
    none(FINITSERIALIO, "FinitSerialIO (not initialised)", 0);
    one(PUTSERIAL, "PutSerial byte (not initialised)", 0x41);
    putblock(0, data, 3);
    one(SETIOSPEED, "SetIOSpeed (not initialised)", 9600);
    one(SETSERIALMODE, "SetSerialMode", 0xA5A5A5FFUL);
    one(SETIOSIZE, "SetIOSize", 0xA5A50010UL);
    none(ENDSERIALIO, "EndSerialIO", 0);

    initserialio(19200, 16, INBUF, OUTBUF);
    for (cur = 0; cur < 2; cur++)
        st[cur] = findstate(thelib());
    check("rifastseriallib: state block found (orig)", st[0] != NULL);
    check("rifastseriallib: state block found (C)", st[1] != NULL);
    same_state();
    check("rifastseriallib: inten = $F01C", st[0] && st[0]->inten == 0xf01c);

    initserialio(9600, 4, 1, 2);                  /* initialised: nothing */
    none(CHECKSERIAL, "CheckSerial", 1);
    one(PUTSERIAL, "PutSerial byte", 0xA5A5A542UL);
    putblock(1, data, 1);
    putblock(1, data, 15);
    one(SETIOSPEED, "SetIOSpeed", 2400);
    one(SETIOSPEED, "SetIOSpeed", 0xA5A52580UL);  /* DIVU.L by all of d0 */
    one(SETIOSPEED, "SetIOSpeed", 31250);
    one(SETSERIALMODE, "SetSerialMode", 0);
    one(SETIOSIZE, "SetIOSize", 0);
    ROUT = "SetIOBuffers";
    strcpy(VEC, "");
    DO2(SETIOBUFFERS, 0, R[cur].d[0] = 0x11111111; R[cur].d[1] = 0x22222222);
    same_state();
    none(FLUSHSERIALIO, "FlushSerialIO", 0);
    none(FINITSERIALIO, "FinitSerialIO", 0);
    none(FINITSERIALIO, "FinitSerialIO (again)", 0);
    one(PUTSERIAL, "PutSerial byte (finished)", 0x43);

    /* SERPER 0: InitSerialIO works, but the library stays "not
       initialised" */
    initserialio(3546895, 2, INBUF, OUTBUF);
    one(PUTSERIAL, "PutSerial byte (SERPER 0)", 0x44);
    initserialio(57600, 300, INBUF, OUTBUF);
    one(SETIOSPEED, "SetIOSpeed", 3546895);
    one(PUTSERIAL, "PutSerial byte (after SetIOSpeed to SERPER 0)", 0x45);

    initserialio(115200, 1, INBUF, OUTBUF);
    ROUT = "finit";
    strcpy(VEC, "");
    DO2(FINIT, 0, (void)0);
    same_state();

    CUSTOM.intena = 0x8000 | (ena & 0x0801);
    printf("%d checks, %d failures\n", checks, failures);
    return failures ? 5 : 0;
}
