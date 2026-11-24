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
 *   init:  the state as at the load (fixed, #1159)
 *   finit: FinitSerialIO
 *
 * The custom chip registers are volatile accesses of the original's
 * width: the TBE/RBF tests read the high byte of SERDATR (BTST #5/#6 on
 * $dff018), the data the low byte ($dff019), SERDAT/SERPER/INTENA/INTREQ
 * are written as words.  The macros !writebyte/!readbyte of the source
 * are the functions writebyte/readbyte.
 * No assembler but the thunks.  The source divides with DIVU.L d0,d1, a
 * 68020 instruction written as data (!divul {0,1} = Dc.w $4c40,$1001):
 * an illegal instruction exception on a 68000.  Fixed (#647): a C
 * division bit by bit (udiv), which works on every CPU.
 *
 * State: mode (SetSerialMode writes the first byte of the word, TST.b
 * reads it), numbytes (DoSerialIO's count - 1), inbuf/outbuf, baud (the
 * SERPER value), inten, bytebuf (temp: one byte buffer, the first byte
 * of a word), and behind them ready (initialised, #653).
 *
 * Fixed (intended deviations):
 *   - InitSerialIO/SetIOSpeed divide in C, not with the 68020 instruction
 *     DIVU.L (above) (#647).
 *   - A block of 0 bytes (PutSerial/GetSerial address,0, SyncPut,
 *     SyncGet) transfers nothing; the original sent or received one byte
 *     and counted on with SUBQ.l: BNE, 2^32 bytes (#646).
 *   - SetIOSpeed divides by its #word argument alone; the original
 *     divided by all of d0, also its undefined upper word (#654).
 *   - InitSerialIO saves INTENAR's value, and FinitSerialIO switches the
 *     RBF and TBE interrupts that were on back on in INTENA.  The original
 *     stored the immediate #$dff000+intenar (the word $F01C, bits 11 and 0
 *     clear: FinitSerialIO never wrote, RBF and TBE stayed off) (#651),
 *     and its write went to the absolute address $9A (intena without
 *     custom+) (#648).
 *   - InitSerialIO (#long result) returns -1 when it set the port up, 0
 *     when it did nothing because it was already initialised; the
 *     original returned its baud argument in both cases (#650).
 *   - The library keeps a flag of its own for "initialised"; the original
 *     took SERPER 0 (3546895 baud) for "not initialised", so after it
 *     PutSerial, SetIOSpeed and FinitSerialIO did nothing.  InitSerialIO
 *     and SetIOSpeed refuse a baud rate outside 109-3546895, whose SERPER
 *     does not fit (the original divided by 0 for baud 0) (#653).
 *   - Every command that works on the port returns at once without
 *     InitSerialIO (CheckSerial -1, GetSerial and SyncSerial 0); the
 *     original tested it only in FinitSerialIO, InitSerialIO, PutSerial
 *     and SetIOSpeed, DoSerialIO ran with the buffers 0 (#652).
 *   - GetSerial address,bytes (#byte result) returns -1 when it received
 *     the block, 0 when it did nothing (not initialised, 0 bytes); the
 *     original returned the low byte of its address argument (#649).
 *   - SetIOSize stores its count - 1, as InitSerialIO does; the original
 *     stored the count as it is, so DoSerialIO after SetIOSize n
 *     transferred n + 1 bytes, one past buffers of n (#1158).
 *   - DoSerialIO without argument transfers nothing after a count of 0
 *     (InitSerialIO baud,0,... or SetIOSize 0); the original's DBRA loop
 *     ran over the stored $FFFF, 65536 bytes into and out of the buffers
 *     (#1164; SetIOSize 0 gave it since the port's fix of #1158, the
 *     original's SetIOSize 0 gave 1 byte).  DoSerialIO n with an explicit
 *     argument takes the count - 1 as documented and is not changed.
 *   - init sets the state as at the load (slave, not initialised, no
 *     buffers); the original's init was empty, a second run from the
 *     editor started with the state of the run before (#1159).
 * Kept on purpose (not fixed, decision):
 * BUG(orig) (#655), wontfix: SyncSerial - two machines that start at the
 * same time both become master (each takes the other's sync byte for the
 * echo).  Not changed (R10 decision Q5 (a)): the handshake would need a
 * new protocol on both ends; a program avoids the race by starting one
 * side first or by choosing the roles itself (SetSerialMode).
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
void rifastseriallib_codeinit(void), rifastseriallib_codefinit(void);
void initserialio(void), finitserialio(void), setserialmode(void);
void endserialio(void), putserial(void), putserial2(void), getserial(void);
void getserial2(void), flushserialio(void), syncserial(void);
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

BLITZ_NULLSUB_AT(init, rifastseriallib_codeinit, 0, 0);
BLITZ_NULLSUB_AT(finit, rifastseriallib_codefinit, 0, 0);

BLITZ_LIBFIN();

/* custom chip registers (custom.res, #custom=$dff000) */
#define SERDATR_HI (*(volatile UBYTE *)(BL_CUSTOM + 0x018))  /* TBE = bit 5, RBF = bit 6 */
#define SERDATR_LO (*(volatile UBYTE *)(BL_CUSTOM + 0x019))  /* the data byte */
#define SERDAT     BL_CUSTOMW(BL_CUSTOM, 0x030)
#define SERPER     BL_CUSTOMW(BL_CUSTOM, 0x032)
#define INTENA     BL_CUSTOMW(BL_CUSTOM, 0x09a)
#define INTREQ     BL_CUSTOMW(BL_CUSTOM, 0x09c)
#define INTENAR    BL_CUSTOMW(BL_CUSTOM, 0x01c)
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
    /* behind the original's 18 bytes: initialised (fixed (#653): the
       original took SERPER 0 for "not initialised") */
    UBYTE ready;
} fs;

/* numbytes of a count of 0 (0 - 1): the word arguments of InitSerialIO
   and SetIOSize give it for no other count (fixed (#1164): DoSerialIO
   without argument transfers nothing) */
#define NOBYTES 0xFFFF

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

/* n / d, unsigned, bit by bit on every CPU (the library links no
   division routine; not a hot path: once per InitSerialIO/SetIOSpeed) */
static ULONG udiv(ULONG n, ULONG d)
{
    ULONG q = 0, r = 0;
    int i;

    for (i = 31; i >= 0; i--) {
        r = r << 1 | (n >> i & 1);
        if (r >= d) {
            r -= d;
            q |= 1UL << i;
        }
    }
    return q;
}

/* a baud rate whose SERPER fits the 15 bits of the register (109 ...
   3546895; fixed (#653): the original divided by 0 for baud 0 and took
   every rate) */
#define BAUD_OK(b) ((b) >= 109 && (b) <= SERCLOCK)

/* the baud rate as SERPER: 3546895 / baud - 1 (SUBQ.w); fixed (#647):
   a division that works on a 68000, the original's DIVU.L is a 68020
   instruction */
static void setbaud(ULONG baud)
{
    fs.baud = (UWORD)(udiv(SERCLOCK, baud) - 1);
    SERPER = fs.baud;
}

/**
 * @brief init: the state as at the load: slave, not initialised, no
 * buffers.
 *
 * Fixed (#1159): the original's init was empty, so a second run of a
 * program with the library still loaded (from the editor) started with
 * the mode, count, buffers and saved INTENAR of the run before (a
 * program that never calls SetSerialMode was the master in its second
 * run if the first ended as master).
 */
void BLITZ_ROUTINE c_codeinit(void)
{
    fs.mode = fs.mode_lo = 0;
    fs.numbytes = 0;
    fs.inbuf = fs.outbuf = NULL;
    fs.baud = fs.inten = 0;
    fs.bytebuf[0] = fs.bytebuf[1] = 0;
    fs.ready = 0;
}

/**
 * @brief FinitSerialIO (and finit): end the serial I/O, switch TBE/RBF back on.
 *
 * Nothing if not initialised; else baud = 0 and the RBF (bit 11)
 * and TBE (bit 0) interrupts that were on at InitSerialIO (the saved
 * inten) are switched on again in INTENA.  Fixed (#651, #648): the
 * original saved the address of INTENAR ($F01C, bits 11 and 0 clear),
 * so it never wrote, and its write went to the absolute address $9A
 * instead of INTENA.
 */
void BLITZ_ROUTINE c_finitserialio(void)
{
    UWORD on = 0;

    if (!fs.ready)                      /* fixed (#653): not baud */
        return;
    fs.ready = 0;
    fs.baud = 0;
    if (fs.inten & 0x0800)
        on |= 0x0800;
    if (fs.inten & 0x0001)
        on |= 0x0001;
    if (on)
        INTENA = (UWORD)(on | 0x8000);  /* fixed (#648): INTENA, not $9A */
}

/**
 * @brief DoSerialIO numbytes-1: exchange the buffers byte by byte.
 *
 * Master (SetSerialMode <> 0): writes a byte of outbuf, then reads one
 * into inbuf; slave: the other way round.  n + 1 bytes (DBRA).  Nothing
 * if not initialised (fixed (#652): the original ran with the buffers 0,
 * reading into and sending from address 0).
 * @param n  d0: the number of bytes - 1
 */
void BLITZ_ROUTINE c_doserialIO(UWORD n BL_REG(d0))
{
    UBYTE *out = fs.outbuf, *in = fs.inbuf;

    if (!fs.ready)                      /* fixed (#652) */
        return;
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
 *
 * Nothing for a count of 0 (numbytes NOBYTES).  Fixed (#1164): the
 * original ran its DBRA loop over $FFFF, 65536 bytes into and out of
 * buffers sized for none.
 */
void BLITZ_ROUTINE c_doserialIO2(void)
{
    if (fs.numbytes == NOBYTES)         /* fixed (#1164) */
        return;
    c_doserialIO(fs.numbytes);
}

/**
 * @brief InitSerialIO baud,numbytes,inbuf,outbuf: set up the serial port for the transfers.
 *
 * Nothing if already initialised or the baud rate is not between 109
 * and 3546895 (fixed (#653): the original divided by 0 for baud 0, a rate
 * above 3546895 gave SERPER $FFFF, one below 109 one in the 16th bit, the
 * LONG flag).  Switches the TBE and RBF interrupts
 * off (INTENA $0801), sets SERPER = 3546895 / baud - 1 (fixed (#647): a
 * division that works on a 68000, the original's DIVU.L is a 68020
 * instruction), and stores numbytes - 1 and the buffers (numbytes 0
 * gives NOBYTES: DoSerialIO without argument transfers nothing, fixed
 * (#1164)).
 * Fixed (#651): inten gets INTENAR's value, the original the immediate
 * $F01C (the address of INTENAR).  Fixed (#653): the library is
 * initialised also with SERPER 0 (3546895 baud), which the original took
 * for "not initialised".
 * @param baud      d0: the baud rate
 * @param numbytes  d1: the bytes of a DoSerialIO
 * @param inbuf     d2: the receive buffer
 * @param outbuf    d3: the send buffer
 * @return d0: -1 when it set the port up, 0 when it did nothing (fixed
 *         (#650): the original returned the baud argument in both cases)
 */
ULONG BLITZ_ROUTINE c_initserialio(ULONG baud BL_REG(d0), UWORD numbytes BL_REG(d1),
                                   UBYTE *inbuf BL_REG(d2), UBYTE *outbuf BL_REG(d3))
{
    if (fs.ready || !BAUD_OK(baud))     /* fixed (#653) */
        return 0;                       /* fixed (#650) */
    fs.inten = INTENAR;                 /* fixed (#651): the value */
    INTENA = 0x0801;                    /* TBE and RBF interrupts off */
    fs.numbytes = (UWORD)(numbytes - 1);
    setbaud(baud);
    fs.inbuf = inbuf;
    fs.outbuf = outbuf;
    fs.ready = 1;
    return (ULONG)-1;                   /* fixed (#650) */
}

/**
 * @brief SetSerialMode mode: master (<> 0) or slave (0) for DoSerialIO.
 * @param mode  d0: the mode (a byte; the first byte of the word mode)
 */
void BLITZ_ROUTINE c_setserialmode(UBYTE mode BL_REG(d0))
{
    fs.mode = mode;
}

/**
 * @brief EndSerialIO: "disable serial read/write" - does nothing (an RTS).
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
 */
void BLITZ_ROUTINE c_putserial(UBYTE b BL_REG(d0))
{
    if (!fs.ready)                      /* fixed (#653) */
        return;
    fs.bytebuf[0] = b;
    writebyte(fs.bytebuf[0]);
}

/**
 * @brief PutSerial address,bytes: send a block of bytes.
 *
 * Nothing if not initialised.  Fixed (#646): 0 bytes send nothing (the
 * original sent one byte and counted on with SUBQ.l: BNE, 2^32 bytes).
 * @param p  d0: the address of the data
 * @param n  d1: the number of bytes
 */
void BLITZ_ROUTINE c_putserial2(const UBYTE *p BL_REG(d0), ULONG n BL_REG(d1))
{
    if (!fs.ready)                      /* fixed (#653) */
        return;
    while (n--)                         /* fixed (#646) */
        writebyte(*p++);
}

/**
 * @brief CheckSerial: the received byte, without waiting.
 *
 * If RBF is set: the byte of SERDATR, RBF acknowledged in INTREQ.  -1
 * if not initialised (fixed (#652)).
 * @return d0: the byte, -1 if there is none
 */
ULONG BLITZ_ROUTINE c_checkserial(void)
{
    UBYTE b;

    if (!fs.ready || !(SERDATR_HI & RBF))       /* fixed (#652) */
        return (ULONG)-1;
    b = SERDATR_LO;
    INTREQ = INTF_RBF;
    return b;
}

/**
 * @brief GetSerial: wait for a byte and return it (through bytebuf).
 *
 * 0 if not initialised (fixed (#652): the original waited for a byte).
 * @return d0: the byte
 */
ULONG BLITZ_ROUTINE c_getserial(void)
{
    if (!fs.ready)                      /* fixed (#652) */
        return 0;
    fs.bytebuf[0] = readbyte();
    return fs.bytebuf[0];
}

/**
 * @brief GetSerial address,bytes: receive a block of bytes.
 *
 * Fixed (#646): 0 bytes receive nothing (the original: 2^32).  Fixed
 * (#652): nothing if not initialised.
 * @param p  d0: the buffer
 * @param n  d1: the number of bytes
 * @return d0: -1 when it received the block, 0 when it did nothing (a
 *         #byte result; fixed (#649): the original returned its address
 *         argument)
 */
ULONG BLITZ_ROUTINE c_getserial2(UBYTE *p BL_REG(d0), ULONG n BL_REG(d1))
{
    if (!fs.ready || !n)                /* fixed (#652, #646) */
        return 0;                       /* fixed (#649) */
    while (n--)
        *p++ = readbyte();
    return (ULONG)-1;                   /* fixed (#649) */
}

/**
 * @brief FlushSerialIO: "make sure serial port is empty" - acknowledge RBF in INTREQ.
 */
void BLITZ_ROUTINE c_flushserialio(void)
{
    INTREQ = INTF_RBF;
}

/**
 * @brief SyncPut address,bytes: send bytes, each paced by the other side's answer.
 *
 * For every byte: send it, wait for a byte back (into bytebuf).
 * Fixed (#646): 0 bytes send nothing (the original: 2^32).  Fixed
 * (#652): nothing if not initialised.
 * @param p  d0: the data
 * @param n  d1: the number of bytes
 */
void BLITZ_ROUTINE c_syncput(const UBYTE *p BL_REG(d0), ULONG n BL_REG(d1))
{
    if (!fs.ready)                      /* fixed (#652) */
        return;
    while (n--) {                       /* fixed (#646) */
        writebyte(*p++);
        fs.bytebuf[0] = readbyte();
    }
}

/**
 * @brief SyncGet address,bytes: receive bytes, answering each one.
 *
 * For every byte received it sends the byte in bytebuf (whatever was
 * last written there: no defect, the answer only paces the sender).
 * Fixed (#646): 0 bytes receive nothing (the original: 2^32).  Fixed
 * (#652): nothing if not initialised.
 * @param p  d0: the buffer
 * @param n  d1: the number of bytes
 */
void BLITZ_ROUTINE c_syncget(UBYTE *p BL_REG(d0), ULONG n BL_REG(d1))
{
    if (!fs.ready)                      /* fixed (#652) */
        return;
    while (n--) {                       /* fixed (#646) */
        *p++ = readbyte();
        writebyte(fs.bytebuf[0]);
    }
}

/**
 * @brief SyncSerial syncbyte: synchronise with the other machine, deciding master and slave.
 *
 * A byte already waiting makes this side the slave: it waits for the
 * sync byte, echoes it and reads until a 0 arrives.  Otherwise it is the
 * master: it sends the sync byte until it comes back, then sends 0 and
 * flushes.  BUG(orig) (#655), wontfix: two machines starting at the same
 * time both become master (each takes the other's sync byte for the
 * echo); see the file's head.
 * BB_FALSE if not initialised (fixed (#652): the original ran on the
 * port).
 * @param sync  d0: the sync byte
 * @return d0: BB_TRUE = master, BB_FALSE = slave (a #word result)
 */
BB_BOOL BLITZ_ROUTINE c_syncserial(UBYTE sync BL_REG(d0))
{
    UBYTE b;

    if (!fs.ready)                      /* fixed (#652) */
        return BB_FALSE;
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
 * Fixed (#653): nothing for a rate below 109 (the original wrote a SERPER
 * with the LONG flag, or divided by 0), and the library is initialised
 * also after SERPER 0.
 * The division as in InitSerialIO (fixed, #647).  Fixed (#654): it
 * divides by the word argument alone, the original by all of d0, also
 * its undefined upper word.
 * @param baud  d0.w: the baud rate
 */
void BLITZ_ROUTINE c_setiospeed(UWORD baud BL_REG(d0))
{
    if (!fs.ready || !BAUD_OK(baud))    /* fixed (#653) */
        return;
    setbaud(baud);
}

/**
 * @brief SetIOSize bytes: the count of DoSerialIO without argument.
 *
 * Stored as count - 1 for DoSerialIO's DBRA loop, as InitSerialIO stores
 * its numbytes.  Fixed (#1158): the original stored the count as it is,
 * so DoSerialIO transferred one byte more than SetIOSize said.  A count
 * of 0 gives NOBYTES: DoSerialIO without argument transfers nothing
 * (fixed, #1164).
 * @param n  d0: the count
 */
void BLITZ_ROUTINE c_setiosize(UWORD n BL_REG(d0))
{
    fs.numbytes = (UWORD)(n - 1);       /* fixed (#1158) */
}

/**
 * @brief SetIOBuffers inbuf,outbuf: the buffers of DoSerialIO.
 * @param in   d0: the receive buffer
 * @param out  d1: the send buffer
 */
void BLITZ_ROUTINE c_setiobuf(UBYTE *in BL_REG(d0), UBYTE *out BL_REG(d1))
{
    fs.inbuf = in;
    fs.outbuf = out;
}

/* entries: keep every one of d0/d1/a0/a1 that is not the result */
BLITZ_THUNK(rifastseriallib_codeinit, c_codeinit, "d0-d1/a0-a1");
BLITZ_THUNK(rifastseriallib_codefinit, c_finitserialio, "d0-d1/a0-a1");
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
