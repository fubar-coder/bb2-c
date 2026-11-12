/*
 * rwe rifastseriallib - the RWE userlib rifastseriallib: the C build
 * (libs/userlibs/rwe/rifastseriallib.c) or the CD binary BSS2
 * RIFASTSERIALLIB.OBJ, one set as LIBSET says, observed as cases
 * (sysload.h, LIBS_ONE_SET; the expected data
 * libs/tests/expected/rwe_rifastseriallib.txt).
 *
 *   rifastseriallib DIR     (DIR: rifastseriallib.obj or .orig)
 *
 * The library drives the serial port registers directly.  The emulator
 * has no serial connection: SERDATR always shows TBE (a byte written is
 * gone at once) and RBF never comes (Amiberry does not let a program set
 * it through INTREQ either), so everything that sends can run, nothing
 * that waits for a byte can.  Observed with the inputs (other registers =
 * markers): d0 of the functions, the kept registers, INTENAR
 * (InitSerialIO switches TBE/RBF off - called with both on inside
 * Disable(); FinitSerialIO must not change it nor the word at $9A,
 * BUG(orig) (#648, #651)) and the library's state block: mode, numbytes,
 * inbuf, outbuf, baud (= the SERPER value written, which cannot be read
 * back), inten ($F01C, BUG(orig) (#651)) and temp.  The C build keeps the
 * original's layout of these 18 bytes; the test finds them in the
 * library after the first InitSerialIO by its inbuf/outbuf values.
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
#define LIBS_ONE_SET
#include "sysload.h"
#include <hardware/custom.h>

#define CUSTOM (*(volatile struct Custom *)0xdff000)

static const char *names[] = { "rifastseriallib", NULL };

enum { INITSERIALIO, FINITSERIALIO, SETSERIALMODE, ENDSERIALIO, PUTSERIAL, GETSERIAL,
       FLUSHSERIALIO, SYNCSERIAL, DOSERIALIO, SETIOBUFFERS, SETIOSPEED, SETIOSIZE,
       CHECKSERIAL, SYNCPUT, SYNCGET, INIT, FINIT };

static struct regs R, IN;

static struct lib *thelib(void)
{
    return getlib(&loaded, names[0]);
}

static APTR ent(int idx, int form)
{
    return (APTR)((ULONG)entry_form(thelib(), idx, form) & ~1UL);
}

/* INTENAR after the call, made with TBE and RBF interrupts on inside
   Disable() (switched off again before Enable()) */
static UWORD intenar;

#define DO1(idx, form, setup) do {                                          \
        markregs(&R);                                                       \
        setup;                                                              \
        IN = R;                                                             \
        Disable();                                                          \
        CUSTOM.intena = 0x8801;                                             \
        callregs(ent(idx, form), &R, 0, 0);                                 \
        intenar = CUSTOM.intenar;                                           \
        CUSTOM.intena = 0x0801;                                             \
        Enable();                                                           \
        obs_kept(&IN, &R);                                                  \
    } while (0)

/* the state block of the library (found after the first InitSerialIO) */
struct fsstate {
    UWORD mode, numbytes;
    ULONG inbuf, outbuf;
    UWORD baud, inten, temp;
};
static struct fsstate *st;
/* the word at $9A (through a variable: a constant address would be a
   null pointer offset for GCC) and its value at the start */
static volatile UWORD *addr9a;
static UWORD vec9a;

#define INBUF  0x12345678UL
#define OUTBUF 0x9abcdef0UL

static void same_state(void)
{
    OBS("INTENAR", intenar & 0x0801);
    OBS("word at $9A unchanged", *addr9a == vec9a);
    if (!st)
        return;
    OBS("mode", st->mode & 0xff00);
    OBS("numbytes", st->numbytes);
    OBS("inbuf", st->inbuf);
    OBS("outbuf", st->outbuf);
    OBS("baud (SERPER)", st->baud);
    OBS("inten", st->inten);
    OBS("temp", st->temp & 0xff00);
}

static void none(int idx, const char *name, int result)
{
    ROUT = name;
    strcpy(VEC, "");
    DO1(idx, 0, (void)0);
    if (result)
        OBS("d0", R.d[0]);
    same_state();
}

static void one(int idx, const char *name, ULONG a)
{
    ROUT = name;
    sprintf(VEC, "$%lX", a);
    DO1(idx, 0, R.d[0] = a);
    same_state();
}

static void initserialio(ULONG baud, ULONG n, ULONG in, ULONG out)
{
    ROUT = "InitSerialIO";
    sprintf(VEC, "%lu,%lu,$%lX,$%lX", baud, n, in, out);
    DO1(INITSERIALIO, 0, R.d[0] = baud; R.d[1] = 0xA5A50000UL | n;
        R.d[2] = in; R.d[3] = out);
    OBS("d0", R.d[0]);
    same_state();
}

static void putblock(int set, UBYTE *p, ULONG n)
{
    ROUT = "PutSerial address,bytes";
    sprintf(VEC, "%lu (%d)", n, set);
    DO1(PUTSERIAL, 1, R.d[0] = (ULONG)p; R.d[1] = n);
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
    if (!load_set(argv[1], names))
        return 20;
    KEEPS("init", d0-d7|a0-a3|a6);
    KEEPS("CheckSerial", d1-d7|a0-a3|a6);
    KEEPS("FlushSerialIO", d0-d7|a0-a3|a6);
    KEEPS("FinitSerialIO (not initialised)", d0-d7|a0-a3|a6);
    KEEPS("PutSerial byte (not initialised)", d0-d7|a0-a3|a6);
    KEEPS("PutSerial address,bytes", d2-d7|a1-a3|a6);
    KEEPS("SetIOSpeed (not initialised)", d0-d7|a0-a3|a6);
    KEEPS("SetSerialMode", d0-d7|a0-a3|a6);
    KEEPS("SetIOSize", d0-d7|a0-a3|a6);
    KEEPS("EndSerialIO", d0-d7|a0-a3|a6);
    KEEPS("InitSerialIO", d0|d2-d7|a0-a3|a6);
    KEEPS("PutSerial byte", d1-d7|a1-a3|a6);
    KEEPS("SetIOSpeed", d0|d2-d7|a0-a3|a6);
    KEEPS("SetIOBuffers", d0-d7|a0-a3|a6);
    KEEPS("FinitSerialIO", d2-d7|a0-a3|a6);
    KEEPS("FinitSerialIO (again)", d0-d7|a0-a3|a6);
    KEEPS("PutSerial byte (finished)", d0-d7|a0-a3|a6);
    KEEPS("PutSerial byte (SERPER 0)", d0-d7|a0-a3|a6);
    KEEPS("PutSerial byte (after SetIOSpeed to SERPER 0)", d0-d7|a0-a3|a6);
    KEEPS("finit", d2-d7|a0-a3|a6);
    __asm__("" : "=a"(addr9a) : "0"(0x9a));
    vec9a = *addr9a;
    ena = CUSTOM.intenar;

    ROUT = "init";
    strcpy(VEC, "");
    DO1(INIT, 0, (void)0);

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
    st = findstate(thelib());
    check("rifastseriallib: state block found", st != NULL);
    same_state();                       /* inten: $F01C (BUG(orig) #651) */

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
    DO1(SETIOBUFFERS, 0, R.d[0] = 0x11111111; R.d[1] = 0x22222222);
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
    DO1(FINIT, 0, (void)0);
    same_state();

    CUSTOM.intena = 0x8000 | (ena & 0x0801);
    printf("%d checks, %d failures\n", checks, failures);
    kept_summary();
    return failures ? 5 : 0;
}
