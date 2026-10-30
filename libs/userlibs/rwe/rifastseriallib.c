/*
 * rifastseriallib - RWE userlib 36 (#rifastseriallib): byte transfers
 * straight through the serial port hardware (polling SERDATR, writing
 * SERDAT/SERPER, acknowledging RBF in INTREQ; no serial.device), for
 * links between two machines - InitSerialIO, FinitSerialIO,
 * SetSerialMode, EndSerialIO, PutSerial, GetSerial, FlushSerialIO,
 * SyncSerial, DoSerialIO, SetIOBuffers, SetIOSpeed, SetIOSize,
 * CheckSerial, SyncPut, SyncGet.
 *
 * C translation of rwelibsrc/FASTSerial/fastseriallib3.bb; the
 * declarations equal the CD's BSS2 RIFASTSERIALLIB.OBJ (ctest
 * rwe_declarations_rifastseriallib), the behaviour is compared with it in
 * libs/tests/rwe/rifastseriallib.c.  Notes:
 * docs/rwelibs/port/rifastseriallib.md.
 *   init:  nothing
 *   finit: FinitSerialIO
 *
 * The custom chip registers are volatile accesses of the original's
 * width: the TBE/RBF tests read the high byte of SERDATR (BTST #5/#6 on
 * $dff018), the data the low byte ($dff019), SERDAT/SERPER/INTENA/INTREQ
 * are written as words.  The macros !writebyte/!readbyte of the source
 * are the functions writebyte/readbyte.
 * Assembler only for DIVU.L (InitSerialIO/SetIOSpeed): the source writes
 * the 68020 instruction as data (!divul {0,1} = Dc.w $4c40,$1001, DIVU.L
 * d0,d1); a 68000 build of C would divide with a libgcc call that works
 * on every CPU, the original takes an illegal instruction exception on a
 * 68000 (BUG(orig) (#647), kept).
 *
 * State: mode (SetSerialMode writes the first byte of the word, TST.b
 * reads it), numbytes (DoSerialIO's count - 1), inbuf/outbuf, baud (the
 * SERPER value, 0 = not initialised), inten, temp (one byte buffer, the
 * first byte of a word).
 *
 * BUG(orig):
 *   - InitSerialIO/SetIOSpeed use DIVU.L, a 68020 instruction (above);
 *     SetIOSpeed divides by all of d0, also the upper word of its #word
 *     argument (#647, #654).
 *   - InitSerialIO stores the immediate #$dff000+intenar (the word $F01C)
 *     to inten instead of reading INTENAR, and FinitSerialIO writes to
 *     the absolute address $9A (intena without custom+).  With $F01C
 *     (bits 11 and 0 clear) FinitSerialIO never writes: TBE and RBF stay
 *     off (#651, #648).
 *   - InitSerialIO (#long result) returns its baud argument, also when it
 *     does nothing because it was already initialised (#650).  A baud rate
 *     that gives SERPER 0 (>= 1773448) leaves the library "not
 *     initialised" (#653).
 *   - Only FinitSerialIO, InitSerialIO, PutSerial and SetIOSpeed check
 *     the initialisation (#652); a block of 0 bytes (PutSerial/GetSerial
 *     address,0, SyncPut, SyncGet) is 2^32 bytes (SUBQ.l: BNE) (#646).
 *   - GetSerial address,bytes (#byte result) returns its address argument
 *     (#649).
 *   - SyncSerial: two machines starting at the same time both become
 *     master (each takes the other's sync byte for the echo) (#655).
 * SyncGet sends the byte in temp (whatever was last written there) for
 * every byte received: no defect, like the original (the answer only
 * paces the sender, its value does not matter).
 * Registers: the original changes d0 in every routine that sends (the
 * word $01xx of !writebyte) and d7, a0, a1 in the block routines; the
 * thunks keep all of d0/d1/a0/a1 that are not the result, GCC keeps
 * d2-d7/a2-a6.
 */
#include "blitzlib.h"

/* entries (thunks below) */
void codeinit(void), codefinit(void);
void initserialio(void), finitserialio(void), setserialmode(void);
void endserialio(void), putserial(void), putserial2(void);
void getserial(void), getserial2(void), flushserialio(void), syncserial(void);
void doserialIO2(void), doserialIO(void), setiobuf(void), setiospeed(void);
void setiosize(void), checkserial(void), syncput(void), syncget(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !libheader{#rifastseriallib,init,0,finit,0} */
BLITZ_LIBHEADER(LIB_rifastseriallib, init, BL_NORET, finit, 0);

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG, BL_WORD, BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(initserialio, 0, 0);
BLITZ_NAME("InitSerialIO", "baud,numbytes,inbuf,outbuf");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(finitserialio, 0, 0);
BLITZ_NAME("FinitSerialIO", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_BYTE);
BLITZ_LIBS0();
BLITZ_SUBS(setserialmode, 0, 0);
BLITZ_NAME("SetSerialMode", "mode   -- determine master/slave");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(endserialio, 0, 0);
BLITZ_NAME("EndSerialIO", "- disable serial read/write");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_BYTE);
BLITZ_LIBS0();
BLITZ_SUBS(putserial, 0, 0);
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(putserial2, 0, 0);
BLITZ_NAME("PutSerial", "address,bytes | byte");

BLITZ_ACOMMAND(BL_BYTE);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(getserial, 0, 0);
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(getserial2, 0, 0);
BLITZ_NAME("GetSerial", "[address,bytes]");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(flushserialio, 0, 0);
BLITZ_NAME("FlushSerialIO", "- make sure serial port is empty");

BLITZ_ACOMMAND(BL_WORD);
BLITZ_ARGS(BL_BYTE);
BLITZ_LIBS0();
BLITZ_SUBS(syncserial, 0, 0);
BLITZ_NAME("SyncSerial", "syncbyte");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(doserialIO2, 0, 0);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(doserialIO, 0, 0);
BLITZ_NAME("DoSerialIO", "[numbytes-1]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(setiobuf, 0, 0);
BLITZ_NAME("SetIOBuffers", "inbuf,outbuf");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(setiospeed, 0, 0);
BLITZ_NAME("SetIOSpeed", "baudrate");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(setiosize, 0, 0);
BLITZ_NAME("SetIOSize", "bytes");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(checkserial, 0, 0);
BLITZ_NAME("CheckSerial", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(syncput, 0, 0);
BLITZ_NAME("SyncPut", "address,bytes");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(syncget, 0, 0);
BLITZ_NAME("SyncGet", "address,bytes");

BLITZ_NULLSUB_AT(init, codeinit, 0, 0);
BLITZ_NULLSUB_AT(finit, codefinit, 0, 0);

BLITZ_LIBFIN();

/* custom chip registers (custom.res, #custom=$dff000) */
#define SERDATR_HI (*(volatile UBYTE *)(BL_CUSTOM + 0x018))  /* TBE = bit 5, RBF = bit 6 */
#define SERDATR_LO (*(volatile UBYTE *)(BL_CUSTOM + 0x019))  /* the data byte */
#define SERDAT     BL_CUSTOMW(BL_CUSTOM, 0x030)
#define SERPER     BL_CUSTOMW(BL_CUSTOM, 0x032)
#define INTENA     BL_CUSTOMW(BL_CUSTOM, 0x09a)
#define INTREQ     BL_CUSTOMW(BL_CUSTOM, 0x09c)
#define INTENAR    0x01c
#define TBE  0x20
#define RBF  0x40
#define INTF_RBF 0x0800
/* PAL colour clock / baud = SERPER + 1 */
#define SERCLOCK 3546895UL

static struct {
    UBYTE mode, mode_lo;                /* Dc.w: the first byte is used */
    UWORD numbytes;
    UBYTE *inbuf;
    UBYTE *outbuf;
    UWORD baud;
    UWORD inten;
    UBYTE temp[2];                      /* Dc.w: the first byte is used */
} fs;

/* !divul {0,1}: DIVU.L d0,d1 (68020) - see the head comment */
static ULONG divul(ULONG divisor, ULONG dividend)
{
    register ULONG d0 __asm("d0") = divisor;
    register ULONG d1 __asm("d1") = dividend;

    __asm__ volatile(".word 0x4c40,0x1001" : "+d"(d1) : "d"(d0) : "cc");
    return d1;
}

/* !writebyte: wait for TBE, send the byte with a stop bit */
static void writebyte(UBYTE b)
{
    while (!(SERDATR_HI & TBE))
        ;
    SERDAT = (UWORD)(0x100 | b);
}

/* !readbyte: wait for RBF, take the byte, acknowledge RBF */
static UBYTE readbyte(void)
{
    UBYTE b;

    while (!(SERDATR_HI & RBF))
        ;
    b = SERDATR_LO;
    INTREQ = INTF_RBF;
    return b;
}

/* the baud rate as SERPER: 3546895 / baud - 1 (SUBQ.w) */
static void setbaud(ULONG baud)
{
    fs.baud = (UWORD)(divul(baud, SERCLOCK) - 1);
    SERPER = fs.baud;
}

void BLITZ_ROUTINE c_codeinit(void)
{
}

/* FinitSerialIO (and finit) */
void BLITZ_ROUTINE c_finitserialio(void)
{
    UWORD on = 0;

    if (!fs.baud)
        return;
    fs.baud = 0;
    if (fs.inten & 0x0800)
        on |= 0x0800;
    if (fs.inten & 0x0001)
        on |= 0x0001;
    if (on) {
        /* BUG(orig) (#648): MOVE.w d1,intena - the absolute address $9A (never
           reached, inten is always $F01C); the address goes through an
           asm so that GCC does not take it for a null pointer offset */
        volatile UWORD *intena;
        __asm__("" : "=a"(intena) : "0"(0x9a));
        *intena = (UWORD)(on | 0x8000);
    }
}

/* DoSerialIO [numbytes-1]: exchange the buffers byte by byte */
void BLITZ_ROUTINE c_doserialIO(UWORD n BL_REG(d0))
{
    UBYTE *out = fs.outbuf, *in = fs.inbuf;

    if (fs.mode) {                      /* master: write, then read */
        do {
            writebyte(*out++);
            *in++ = readbyte();
        } while (n-- != 0);             /* DBRA */
    } else {                            /* slave: read, then write */
        do {
            *in++ = readbyte();
            writebyte(*out++);
        } while (n-- != 0);
    }
}

void BLITZ_ROUTINE c_doserialIO2(void)
{
    c_doserialIO(fs.numbytes);
}

/* InitSerialIO baud,numbytes,inbuf,outbuf: d0 = baud (BUG(orig) (#650)) */
ULONG BLITZ_ROUTINE c_initserialio(ULONG baud BL_REG(d0), UWORD numbytes BL_REG(d1),
                                   UBYTE *inbuf BL_REG(d2), UBYTE *outbuf BL_REG(d3))
{
    if (fs.baud)
        return baud;
    fs.inten = (UWORD)(0xdff000 + INTENAR);     /* BUG(orig) (#651): the address */
    INTENA = 0x0801;                    /* TBE and RBF interrupts off */
    fs.numbytes = (UWORD)(numbytes - 1);
    setbaud(baud);
    fs.inbuf = inbuf;
    fs.outbuf = outbuf;
    return baud;
}

void BLITZ_ROUTINE c_setserialmode(UBYTE mode BL_REG(d0))
{
    fs.mode = mode;
}

void BLITZ_ROUTINE c_endserialio(void)
{
}

/* PutSerial byte */
void BLITZ_ROUTINE c_putserial(UBYTE b BL_REG(d0))
{
    if (!fs.baud)
        return;
    fs.temp[0] = b;
    writebyte(fs.temp[0]);
}

/* PutSerial address,bytes (0 = 2^32) */
void BLITZ_ROUTINE c_putserial2(const UBYTE *p BL_REG(d0), ULONG n BL_REG(d1))
{
    if (!fs.baud)
        return;
    do
        writebyte(*p++);
    while (--n != 0);
}

/* CheckSerial: the received byte, -1 if there is none */
ULONG BLITZ_ROUTINE c_checkserial(void)
{
    UBYTE b;

    if (!(SERDATR_HI & RBF))
        return (ULONG)-1;
    b = SERDATR_LO;
    INTREQ = INTF_RBF;
    return b;
}

/* GetSerial: wait for a byte */
ULONG BLITZ_ROUTINE c_getserial(void)
{
    fs.temp[0] = readbyte();
    return fs.temp[0];
}

/* GetSerial address,bytes (0 = 2^32): d0 = address (BUG(orig) (#649)) */
ULONG BLITZ_ROUTINE c_getserial2(UBYTE *p BL_REG(d0), ULONG n BL_REG(d1))
{
    UBYTE *a = p;

    do
        *a++ = readbyte();
    while (--n != 0);
    return (ULONG)p;
}

void BLITZ_ROUTINE c_flushserialio(void)
{
    INTREQ = INTF_RBF;
}

/* SyncPut address,bytes: send a byte, wait for the echo (into temp) */
void BLITZ_ROUTINE c_syncput(const UBYTE *p BL_REG(d0), ULONG n BL_REG(d1))
{
    do {
        writebyte(*p++);
        fs.temp[0] = readbyte();
    } while (--n != 0);
}

/* SyncGet address,bytes: receive a byte, answer with temp (like the original) */
void BLITZ_ROUTINE c_syncget(UBYTE *p BL_REG(d0), ULONG n BL_REG(d1))
{
    do {
        *p++ = readbyte();
        writebyte(fs.temp[0]);
    } while (--n != 0);
}

/* SyncSerial syncbyte: -1 = master, 0 = slave.  A byte already waiting
   makes this side the slave: it waits for the sync byte, echoes it and
   reads until a 0 arrives.  Otherwise it is the master: it sends the
   sync byte until it comes back, then sends 0. */
ULONG BLITZ_ROUTINE c_syncserial(UBYTE sync BL_REG(d0))
{
    UBYTE b;

    fs.temp[0] = sync;
    if (SERDATR_HI & RBF) {
        for (;;) {
            while (!(SERDATR_HI & RBF))
                ;
            b = SERDATR_LO;
            INTREQ = INTF_RBF;
            if (b == fs.temp[0])
                break;
        }
        writebyte(fs.temp[0]);
        do
            fs.temp[0] = readbyte();
        while (fs.temp[0] != 0);
        return 0;
    }
    for (;;) {
        writebyte(fs.temp[0]);
        if (!(SERDATR_HI & RBF))
            continue;
        b = SERDATR_LO;
        INTREQ = INTF_RBF;
        if (b == fs.temp[0])
            break;
    }
    fs.temp[0] = 0;                     /* MOVE.w #0,(a0) */
    fs.temp[1] = 0;
    writebyte(fs.temp[0]);
    c_flushserialio();
    return (ULONG)-1;
}

/* SetIOSpeed baudrate: BUG(orig) (#654) divides by all of d0 */
void BLITZ_ROUTINE c_setiospeed(ULONG baud BL_REG(d0))
{
    if (!fs.baud)
        return;
    setbaud(baud);
}

void BLITZ_ROUTINE c_setiosize(UWORD n BL_REG(d0))
{
    fs.numbytes = n;
}

void BLITZ_ROUTINE c_setiobuf(UBYTE *in BL_REG(d0), UBYTE *out BL_REG(d1))
{
    fs.inbuf = in;
    fs.outbuf = out;
}

/* entries: keep every one of d0/d1/a0/a1 that is not the result */
BLITZ_THUNK(codeinit, c_codeinit, "d0-d1/a0-a1");
BLITZ_THUNK(codefinit, c_finitserialio, "d0-d1/a0-a1");
BLITZ_THUNK(finitserialio, c_finitserialio, "d0-d1/a0-a1");
BLITZ_THUNK(doserialIO2, c_doserialIO2, "d0-d1/a0-a1");
BLITZ_THUNK(doserialIO, c_doserialIO, "d0-d1/a0-a1");
BLITZ_THUNK(initserialio, c_initserialio, "d1/a0-a1");
BLITZ_THUNK(setserialmode, c_setserialmode, "d0-d1/a0-a1");
BLITZ_THUNK(endserialio, c_endserialio, "d0-d1/a0-a1");
BLITZ_THUNK(putserial, c_putserial, "d0-d1/a0-a1");
BLITZ_THUNK(putserial2, c_putserial2, "d0-d1/a0-a1");
BLITZ_THUNK(checkserial, c_checkserial, "d1/a0-a1");
BLITZ_THUNK(getserial, c_getserial, "d1/a0-a1");
BLITZ_THUNK(getserial2, c_getserial2, "d1/a0-a1");
BLITZ_THUNK(flushserialio, c_flushserialio, "d0-d1/a0-a1");
BLITZ_THUNK(syncput, c_syncput, "d0-d1/a0-a1");
BLITZ_THUNK(syncget, c_syncget, "d0-d1/a0-a1");
BLITZ_THUNK(syncserial, c_syncserial, "d1/a0-a1");
BLITZ_THUNK(setiospeed, c_setiospeed, "d0-d1/a0-a1");
BLITZ_THUNK(setiosize, c_setiosize, "d0-d1/a0-a1");
BLITZ_THUNK(setiobuf, c_setiobuf, "d0-d1/a0-a1");
