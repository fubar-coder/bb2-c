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
 * SERPER value, 0 = not initialised), inten, bytebuf (temp: one byte
 * buffer, the first byte of a word).
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
 * SyncGet sends the byte in bytebuf (whatever was last written there) for
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
    UBYTE bytebuf[2];                   /* temp, Dc.w: the first byte is used */
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

/**
 * @brief init: nothing.
 * @return nothing
 */
void BLITZ_ROUTINE c_codeinit(void)
{
}

/**
 * @brief FinitSerialIO (and finit): end the serial I/O, switch TBE/RBF back on.
 *
 * Nothing if not initialised (baud 0); else baud = 0 and the TBE (bit 11)
 * and RBF (bit 0) bits of the saved inten are set again.  BUG(orig):
 * inten is always $F01C (#651), whose bits 11 and 0 are clear, so it
 * never writes; the write would go to the absolute address $9A, not
 * INTENA (#648).
 * @return nothing
 */
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

/**
 * @brief DoSerialIO numbytes-1: exchange the buffers byte by byte.
 *
 * Master (SetSerialMode <> 0): writes a byte of outbuf, then reads one
 * into inbuf; slave: the other way round.  n + 1 bytes (DBRA).  Does not
 * check the initialisation (BUG(orig) (#652)).
 * @param n  d0: the number of bytes - 1
 * @return nothing
 */
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

/**
 * @brief DoSerialIO: c_doserialIO() with the count of InitSerialIO/SetIOSize.
 * @return nothing
 */
void BLITZ_ROUTINE c_doserialIO2(void)
{
    c_doserialIO(fs.numbytes);
}

/**
 * @brief InitSerialIO baud,numbytes,inbuf,outbuf: set up the serial port for the transfers.
 *
 * Nothing if already initialised.  Switches the TBE and RBF interrupts
 * off (INTENA $0801), sets SERPER = 3546895 / baud - 1 with DIVU.L, a
 * 68020 instruction: an illegal instruction exception on a 68000
 * (BUG(orig) (#647)), and stores numbytes - 1 and the buffers.
 * BUG(orig): inten gets the immediate $F01C (the address of INTENAR), not
 * INTENAR's value (#651); a baud rate that gives SERPER 0 (>= 1773448)
 * leaves the library "not initialised" (#653).
 * @param baud      d0: the baud rate
 * @param numbytes  d1: the bytes of a DoSerialIO
 * @param inbuf     d2: the receive buffer
 * @param outbuf    d3: the send buffer
 * @return d0: the baud argument, also when it did nothing (BUG(orig) (#650))
 */
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

/**
 * @brief SetSerialMode mode: master (<> 0) or slave (0) for DoSerialIO.
 * @param mode  d0: the mode (a byte; the first byte of the word mode)
 * @return nothing
 */
void BLITZ_ROUTINE c_setserialmode(UBYTE mode BL_REG(d0))
{
    fs.mode = mode;
}

/**
 * @brief EndSerialIO: "disable serial read/write" - does nothing (an RTS).
 * @return nothing
 */
void BLITZ_ROUTINE c_endserialio(void)
{
}

/**
 * @brief PutSerial byte: send one byte (through bytebuf).
 *
 * Waits for TBE and writes the byte with a stop bit to SERDAT; nothing
 * if not initialised.
 * @param b  d0: the byte
 * @return nothing
 */
void BLITZ_ROUTINE c_putserial(UBYTE b BL_REG(d0))
{
    if (!fs.baud)
        return;
    fs.bytebuf[0] = b;
    writebyte(fs.bytebuf[0]);
}

/**
 * @brief PutSerial address,bytes: send a block of bytes.
 *
 * Nothing if not initialised.  BUG(orig) (#646): 0 bytes are 2^32
 * (SUBQ.l: BNE).
 * @param p  d0: the address of the data
 * @param n  d1: the number of bytes
 * @return nothing
 */
void BLITZ_ROUTINE c_putserial2(const UBYTE *p BL_REG(d0), ULONG n BL_REG(d1))
{
    if (!fs.baud)
        return;
    do
        writebyte(*p++);
    while (--n != 0);
}

/**
 * @brief CheckSerial: the received byte, without waiting.
 *
 * If RBF is set: the byte of SERDATR, RBF acknowledged in INTREQ.  Does
 * not check the initialisation (#652).
 * @return d0: the byte, -1 if there is none
 */
ULONG BLITZ_ROUTINE c_checkserial(void)
{
    UBYTE b;

    if (!(SERDATR_HI & RBF))
        return (ULONG)-1;
    b = SERDATR_LO;
    INTREQ = INTF_RBF;
    return b;
}

/**
 * @brief GetSerial: wait for a byte and return it (through bytebuf).
 *
 * Does not check the initialisation (#652).
 * @return d0: the byte
 */
ULONG BLITZ_ROUTINE c_getserial(void)
{
    fs.bytebuf[0] = readbyte();
    return fs.bytebuf[0];
}

/**
 * @brief GetSerial address,bytes: receive a block of bytes.
 *
 * BUG(orig): 0 bytes are 2^32 (#646); no initialisation check (#652).
 * @param p  d0: the buffer
 * @param n  d1: the number of bytes
 * @return d0: the address argument (a #byte result, BUG(orig) (#649))
 */
ULONG BLITZ_ROUTINE c_getserial2(UBYTE *p BL_REG(d0), ULONG n BL_REG(d1))
{
    UBYTE *a = p;

    do
        *a++ = readbyte();
    while (--n != 0);
    return (ULONG)p;
}

/**
 * @brief FlushSerialIO: "make sure serial port is empty" - acknowledge RBF in INTREQ.
 * @return nothing
 */
void BLITZ_ROUTINE c_flushserialio(void)
{
    INTREQ = INTF_RBF;
}

/**
 * @brief SyncPut address,bytes: send bytes, each paced by the other side's answer.
 *
 * For every byte: send it, wait for a byte back (into bytebuf).
 * BUG(orig): 0 bytes are 2^32 (#646); no initialisation check (#652).
 * @param p  d0: the data
 * @param n  d1: the number of bytes
 * @return nothing
 */
void BLITZ_ROUTINE c_syncput(const UBYTE *p BL_REG(d0), ULONG n BL_REG(d1))
{
    do {
        writebyte(*p++);
        fs.bytebuf[0] = readbyte();
    } while (--n != 0);
}

/**
 * @brief SyncGet address,bytes: receive bytes, answering each one.
 *
 * For every byte received it sends the byte in bytebuf (whatever was
 * last written there: no defect, the answer only paces the sender).
 * BUG(orig): 0 bytes are 2^32 (#646); no initialisation check (#652).
 * @param p  d0: the buffer
 * @param n  d1: the number of bytes
 * @return nothing
 */
void BLITZ_ROUTINE c_syncget(UBYTE *p BL_REG(d0), ULONG n BL_REG(d1))
{
    do {
        *p++ = readbyte();
        writebyte(fs.bytebuf[0]);
    } while (--n != 0);
}

/**
 * @brief SyncSerial syncbyte: synchronise with the other machine, deciding master and slave.
 *
 * A byte already waiting makes this side the slave: it waits for the
 * sync byte, echoes it and reads until a 0 arrives.  Otherwise it is the
 * master: it sends the sync byte until it comes back, then sends 0 and
 * flushes.  BUG(orig) (#655): two machines starting at the same time
 * both become master (each takes the other's sync byte for the echo).
 * @param sync  d0: the sync byte
 * @return d0: BB_TRUE = master, BB_FALSE = slave (a #word result)
 */
BB_BOOL BLITZ_ROUTINE c_syncserial(UBYTE sync BL_REG(d0))
{
    UBYTE b;

    fs.bytebuf[0] = sync;
    if (SERDATR_HI & RBF) {
        for (;;) {
            while (!(SERDATR_HI & RBF))
                ;
            b = SERDATR_LO;
            INTREQ = INTF_RBF;
            if (b == fs.bytebuf[0])
                break;
        }
        writebyte(fs.bytebuf[0]);
        do
            fs.bytebuf[0] = readbyte();
        while (fs.bytebuf[0] != 0);
        return BB_FALSE;
    }
    for (;;) {
        writebyte(fs.bytebuf[0]);
        if (!(SERDATR_HI & RBF))
            continue;
        b = SERDATR_LO;
        INTREQ = INTF_RBF;
        if (b == fs.bytebuf[0])
            break;
    }
    fs.bytebuf[0] = 0;                  /* MOVE.w #0,(a0) */
    fs.bytebuf[1] = 0;
    writebyte(fs.bytebuf[0]);
    c_flushserialio();
    return BB_TRUE;
}

/**
 * @brief SetIOSpeed baudrate: a new baud rate (SERPER) if initialised.
 *
 * DIVU.L as in InitSerialIO (68020, #647); BUG(orig) (#654): it divides
 * by all of d0, also the upper word of its #word argument.
 * @param baud  d0: the baud rate
 * @return nothing
 */
void BLITZ_ROUTINE c_setiospeed(ULONG baud BL_REG(d0))
{
    if (!fs.baud)
        return;
    setbaud(baud);
}

/**
 * @brief SetIOSize bytes: the count of DoSerialIO without argument.
 *
 * Stored as it is (DoSerialIO transfers count + 1 bytes); InitSerialIO
 * stores numbytes - 1.
 * @param n  d0: the count
 * @return nothing
 */
void BLITZ_ROUTINE c_setiosize(UWORD n BL_REG(d0))
{
    fs.numbytes = n;
}

/**
 * @brief SetIOBuffers inbuf,outbuf: the buffers of DoSerialIO.
 * @param in   d0: the receive buffer
 * @param out  d1: the send buffer
 * @return nothing
 */
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
