/*
 * riseriallib - RWE userlib 22 (the literal of the source; RWE's list
 * gives 22 to #riblitlib, of which no source exists): the serial port
 * through the hardware with interrupts of its own ("Serial-Library v1.0
 * (7.3.1994)").
 *   SerialBaud rate              SERPER = 3546895 / rate - 1
 *   SerialOpen [baudrate]        installs the receive/transmit interrupt
 *   SerialClose                  removes it
 *   SerialRead                   the next received byte, -1 if none
 *   SerialWrite byte             into the transmit buffer
 *   SerialVersion                1
 *   SerialBuffer                 the address of the receive buffer
 *   SerialSendBlock address,length
 *   SerialReceiveBlock address,length
 *   SerialFlush                  reads until the buffer is empty
 *   SerialSendString a$          (see BUG(orig), #793, #801)
 *   SerialReadString             bytes up to a 0
 *   SerialProgress               the remaining length of a block
 *   SerialReadLong / SerialWriteLong longword
 *   SerialStop                   ends a block transfer
 *   SerialGot                    -1 if a byte is waiting, else 0
 *   SerialWait                   waits for a byte and reads it
 *   finit: SerialClose
 *
 * C translation of rwelibsrc/seriallib/Serial-Library.bb; reference: the
 * reference build of the source (REFERENCE refbuild, no binary exists;
 * docs/rwelibs/README.md 2 and 3.3).  Notes: docs/rwelibs/port/
 * riseriallib.md.
 *
 * SerialOpen writes its handlers straight into the 68000 autovectors of
 * levels 5 (RBF) and 1 (TBE) at $74 and $64 (the old ones are chained
 * when the interrupt is not the library's), sets CIA-B PRA to $3F
 * (RTS/DTR), and switches RBF, TBE and the master bit on in INTENA,
 * remembering INTENAR.  The receive interrupt puts SERDATR's byte into a
 * ring of 256 bytes (rend), SerialRead takes it (rbig); SerialWrite puts
 * the byte with a stop bit into a ring of 128 words (wrend) and raises
 * TBE when the ring was empty, the transmit interrupt sends one word
 * (wbig) when SERDATR shows TBE.  The rings are the first 256 bytes of
 * 1024-byte buffers: the indices are words whose low byte alone is
 * counted up (ADD.b on their second byte).
 *
 * The custom chip registers and CIA-B PRA are volatile accesses of the
 * original's width, at their absolute addresses (the test redirects
 * them).  rbig, rend and the receive buffer, and wbig, wrend and the
 * transmit buffer keep the original's order (SerialBuffer gives the
 * address of the first; the test reads the indices in front of it).
 *
 * Assembler: the interrupt handlers' entries (RTE, or on to the old
 * vector), the thunks, and the ends of SerialReceiveBlock's abort and
 * SerialReadString with an open port (both return to a value on the
 * stack, BUG(orig) #803, #796).
 *
 * BUG(orig):
 *   - The vectors are written at $64/$74 (the VBR of a 68010+ is not
 *     looked at) (#804), the old ones into the operands of two JMP
 *     instructions in the code without clearing the instruction cache
 *     (#791).  SerialClose switches all interrupts off (INTENA $7FFF) and
 *     then on only those that were on at SerialOpen (#790).
 *   - The open flag is a word, SerialSendBlock, SerialReceiveBlock,
 *     SerialFlush, SerialReadLong and SerialWriteLong test it as a long
 *     (with the first instruction of SerialClose behind it, $4A79: never
 *     0): the first three never open the port, the other two always call
 *     SerialOpen's code (#792).  SerialSendString and SerialReadString
 *     test the word the other way round: they call it when the port is
 *     open (it then only clears the stop flag) (#793).
 *   - SerialRead/SerialWait return the upper word of d0 at the call with
 *     the byte (#long; -1 if empty); a byte $FF with $FFFF there and a
 *     ring index with $FF in the high byte reads as -1 (#797).
 *   - SerialSendBlock with a length of 0 or less sends one byte (#800).
 *   - SerialReceiveBlock first throws away what is in the buffer, then
 *     waits (#798); a length of 1 receives without end (the count goes
 *     below 0) (#799); SerialStop during it ends it by taking its return
 *     address into d7 and returning to the long above it on the stack
 *     (#803).
 *   - SerialSendString sends the low byte of the string's address (with
 *     the stop bit) once for every long it reads from the a2 stack, down
 *     to one with a low byte 0 (DIFF: a2 unchanged, as in all C ports;
 *     the original leaves it below the longs read) (#801).
 *   - SerialReadString stores up to and including the 0 and leaves a3
 *     behind it (#795); the count of characters (d0 = count - 1) is reset
 *     only when the port is open, where the routine returns to the d0 of
 *     the call (it pushed it and never takes it back) (#796); the loop
 *     ends at a 0 only if the upper word of d0 at the call is 0 and no
 *     wait for a byte (-1) came before (#797).
 *   - SerialReadLong never returns: the counter of its loop is taken
 *     back from the stack every time (it reads bytes without end) (#794).
 *   - SerialWriteLong sends the low byte of its argument and then three
 *     times $01 (it rotates the register SerialWrite changed) (#805).
 *   - SerialGot returns -1/0, not a count (#802).
 *
 * No defects: SerialProgress gives the remaining count of the last send
 * (or receive) block, what it is meant to give; SerialBaud 0 divides by 0
 * through ldivlib, whose division by zero has a defined result.
 */
#include "blitzlib.h"

/* entries (thunks below) */
void _setbaud(void), _openserial(void), _openserial2(void), _closeserial(void);
void _readser(void), _writeser(void), info(void), _showloc(void), _sersend(void);
void _serreceive(void), _flushser(void), _sendserialstring(void);
void _readserialstring(void), _buffpos(void), _readlong(void), _writelong(void);
void _abortxfer(void), _buffcount(void), _buffcount2(void), _libfinit(void);
void readint(void), writeint(void);

BLITZ_ENTRY_DECL(finit);

/* !libheader {22,0,0,finit,0} */
BLITZ_LIBHEADER(LIB_riseriallib, 0, BL_NORET, finit, 0);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_setbaud, 0, 0);
BLITZ_NAME("SerialBaud", "(rate)");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_openserial, 0, 0);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_openserial2, 0, 0);
BLITZ_NAME("SerialOpen", "[baudrate]");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_closeserial, 0, 0);
BLITZ_NAME("SerialClose", "");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_readser, 0, 0);
BLITZ_NAME("SerialRead", "-- returns next byte, -1 if empty");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_writeser, 0, 0);
BLITZ_NAME("SerialWrite", " byte ");

BLITZ_AFUNCTION(BL_BYTE);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(info, 0, 0);
BLITZ_NAME("SerialVersion", "");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_showloc, 0, 0);
BLITZ_NAME("SerialBuffer", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_sersend, 0, 0);
BLITZ_NAME("SerialSendBlock", "address,length");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_serreceive, 0, 0);
BLITZ_NAME("SerialReceiveBlock", "address,length");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_flushser, 0, 0);
BLITZ_NAME("SerialFlush", " -- clears serial port read buffer");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(_sendserialstring, 0, 0);
BLITZ_NAME("SerialSendString", "(a$)");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_readserialstring, 0, 0);
BLITZ_NAME("SerialReadString", "");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_buffpos, 0, 0);
BLITZ_NAME("SerialProgress", "");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_readlong, 0, 0);
BLITZ_NAME("SerialReadLong", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_writelong, 0, 0);
BLITZ_NAME("SerialWriteLong", "(longword)");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_abortxfer, 0, 0);
BLITZ_NAME("SerialStop", "");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_buffcount, 0, 0);
BLITZ_NAME("SerialGot", "   -- returns num chars in serial buffer");

BLITZ_ACOMMAND(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_buffcount2, 0, 0);
BLITZ_NAME("SerialWait", "   -- waits for serial buffer to receive a byte");

BLITZ_NULLSUB_AT(finit, _libfinit, 0, 0);

BLITZ_LIBFIN();

/* custom chip registers (#serdat=$dff030 ...) and CIA-B PRA */
#define SERDATR_HI (*(volatile UBYTE *)0xdff018)   /* TBE = bit 5 */
#define SERDATR_LO (*(volatile UBYTE *)0xdff019)   /* the received byte */
#define INTENAR    (*(volatile UWORD *)0xdff01c)
#define INTREQR_HI (*(volatile UBYTE *)0xdff01e)   /* RBF = bit 3 (11) */
#define INTREQR_LO (*(volatile UBYTE *)0xdff01f)   /* TBE = bit 0 */
#define SERDAT     (*(volatile UWORD *)0xdff030)
#define SERPER     (*(volatile UWORD *)0xdff032)
#define INTENA     (*(volatile UWORD *)0xdff09a)
#define INTREQ     (*(volatile UWORD *)0xdff09c)
#define CIABPRA    (*(volatile UBYTE *)0xbfd000)
#define SERCLOCK   3546895UL

/* the level 1 and 5 autovectors ($64, $74): the address through an asm,
   so that GCC does not take it for a null pointer offset */
static volatile ULONG *vector(ULONG a)
{
    volatile ULONG *p;
    __asm__("" : "=a"(p) : "0"(a));
    return p;
}
#define VEC_TBE vector(0x64)
#define VEC_RBF vector(0x74)

static ULONG buffstart, bufflength;
static ULONG baudrate = 2400;
static volatile ULONG quitflag;
static UWORD openflag;
static UWORD oldint;                    /* INTENAR at SerialOpen */
/* the old vectors (the original: the operands of JMP OldInt1/OldInt5),
   read by the handlers' entries */
ULONG riser_oldint1 = 0xffffffff, riser_oldint5 = 0xffffffff;

/* the receive ring (rbig: next to read, rend: next to write) and the
   transmit ring; the indices count only in their low byte */
static struct {
    volatile UWORD big, end;
    UBYTE buf[1024];
} rd, wr;

static ULONG numchars;
static const UBYTE server = 1;
static const char version[] __attribute__((used)) = "$VER: Serial-Library v1.0 (7.3.1994)";

/* ADD.b #n on the low byte of an index */
#define BUMP(w, n) (((volatile UBYTE *)&(w))[1] += (n))

/* SerialRead (_readser): d0's upper word stays (MOVE.w rbig,d0) */
static ULONG readser(ULONG value)
{
    UWORD b = rd.big;

    if (b == rd.end)
        return (ULONG)-1;
    value = (value & 0xffff0000UL) | b;
    value = (value & 0xffffff00UL) | rd.buf[(WORD)b];
    BUMP(rd.big, 1);
    return value;
}

/* SerialWrite (_writeser): the word $01xx into the ring; TBE requested
   when the ring was empty.  Returns d0 as the original leaves it. */
static ULONG writeser(ULONG value)
{
    UWORD e = wr.end;

    value = (value & 0xffff0000UL) | 0x0100 | (value & 0xff);
    *(UWORD *)(wr.buf + (WORD)e) = (UWORD)value;
    BUMP(wr.end, 2);
    if (e == wr.big)
        INTREQ = 0x8001;
    return value;
}

/* _openserial: the stop flag cleared; if not open: the vectors, the
   rings cleared, RTS/DTR, the interrupts on.  Returns d0 (0 when it
   opened, else unchanged). */
static ULONG openserial(ULONG value)
{
    quitflag = 0;
    if (openflag)
        return value;
    riser_oldint5 = *VEC_RBF;
    riser_oldint1 = *VEC_TBE;
    *VEC_RBF = (ULONG)readint;
    *VEC_TBE = (ULONG)writeint;
    rd.big = rd.end = 0;                /* CLR.l rbig */
    wr.big = wr.end = 0;
    CIABPRA = 0x3f;
    oldint = INTENAR;
    INTENA = 0xc801;
    openflag = 0xffff;
    return 0;
}

/**
 * @brief SerialBaud rate: sets the baud rate (SERPER = 3546895 / rate - 1,
 * bit 15 cleared).
 *
 * Rate 0 divides by 0 through ldivlib (a defined result, no defect).
 * @param rate  d0: the baud rate (also kept for SerialOpen)
 */
void BLITZ_ROUTINE c_setbaud(ULONG rate BL_REG(d0))
{
    struct bl_regs r = { .d0 = SERCLOCK, .d1 = rate };

    baudrate = rate;
    BLITZ_ALIBJSR(TOK_ldivide, r);
    SERPER = (UWORD)(r.d0 - 1) & 0x7fff;        /* SUBQ.w, BCLR #15 */
}

/**
 * @brief SerialOpen: installs the library's receive and transmit
 * interrupts (see openserial).
 *
 * BUG(orig) (#804): the vectors at $64/$74 (the VBR is not looked at);
 * BUG(orig) (#791): the old ones go into the operands of two JMPs
 * without clearing the instruction cache.
 */
void BLITZ_ROUTINE c_openserial(void)
{
    openserial(0);
}

/**
 * @brief SerialOpen baudrate: SerialBaud baudrate, then SerialOpen.
 * @param rate  d0: the baud rate
 */
void BLITZ_ROUTINE c_openserial2(ULONG rate BL_REG(d0))
{
    baudrate = rate;
    c_setbaud(rate);
    openserial(0);
}

/**
 * @brief SerialClose (and finit): restores the vectors, CIA-B PRA $FF and
 * the interrupts.
 *
 * BUG(orig) (#790): switches all interrupts off (INTENA $7FFF) and then
 * on only those that were on at SerialOpen.  Nothing if not open.
 */
void BLITZ_ROUTINE c_closeserial(void)
{
    if (!openflag)
        return;
    *VEC_RBF = riser_oldint5;
    *VEC_TBE = riser_oldint1;
    INTENA = 0x7fff;
    CIABPRA = 0xff;
    INTENA = oldint | 0xc000;
    openflag = 0;
}

/**
 * @brief SerialRead: the next received byte.
 *
 * BUG(orig) (#797): the upper word of d0 at the call stays in the result;
 * a byte $FF with $FFFF there reads as -1.
 * @param value  d0: d0 at the call
 * @return d0: the byte (with the upper word and the ring index's high
 *         byte of d0), -1 if the buffer is empty
 */
ULONG BLITZ_ROUTINE c_readser(ULONG value BL_REG(d0))
{
    return readser(value);
}

/**
 * @brief SerialWrite byte: puts the byte (with a stop bit) into the
 * transmit ring; TBE is requested when the ring was empty.
 * @param value  d0: the byte (low byte)
 */
void BLITZ_ROUTINE c_writeser(ULONG value BL_REG(d0))
{
    writeser(value);
}

/**
 * @brief SerialVersion: the version of the library.
 * @return d0: 1
 */
ULONG BLITZ_ROUTINE c_info(void)
{
    return server;
}

/**
 * @brief SerialBuffer: the address of the receive ring (256 bytes; its
 * two index words lie in front of it).
 * @return d0: the address
 */
ULONG BLITZ_ROUTINE c_showloc(void)
{
    return (ULONG)rd.buf;
}

/**
 * @brief SerialSendBlock address,length: sends a block byte by byte.
 *
 * SerialStop ends it; SerialProgress gives the remaining count.
 * BUG(orig) (#792): never opens the port; BUG(orig) (#800): a length of
 * 0 or less sends one byte.
 * @param addr  d0: the address
 * @param len   d1: the length
 */
void BLITZ_ROUTINE c_sersend(ULONG addr BL_REG(d0), ULONG len BL_REG(d1))
{
    LONG n = (LONG)(len - 1);

    buffstart = addr;
    for (;;) {
        const UBYTE *p = (const UBYTE *)buffstart;
        buffstart++;
        writeser(*p);
        n--;
        if (quitflag) {                 /* SerialStop */
            quitflag = 0;
            return;
        }
        bufflength = (ULONG)n;
        if (n < 0)
            return;
    }
}

/**
 * @brief SerialReceiveBlock address,length: receives a block byte by byte.
 *
 * BUG(orig) (#792): never opens the port; BUG(orig) (#798): first throws
 * away what is in the buffer, then waits; BUG(orig) (#799): a length of
 * 1 receives without end; BUG(orig) (#803): SerialStop ends it by
 * returning to the long above the return address (assembler thunk
 * __serreceive).
 * @param addr  d0: the address
 * @param len   d1: the length
 * @return d0: 1 = stopped (for the thunk), 0 = done
 */
ULONG BLITZ_ROUTINE c_serreceive(ULONG addr BL_REG(d0), ULONG len BL_REG(d1))
{
    ULONG value;

    buffstart = addr;
    bufflength = len - 1;
    while (readser(0) != (ULONG)-1)     /* throw away what is there */
        ;
    while ((value = readser(0)) == (ULONG)-1)
        ;
    *(UBYTE *)buffstart = (UBYTE)value;
    buffstart++;
    for (;;) {
        while ((value = readser(0)) == (ULONG)-1)
            ;
        *(UBYTE *)buffstart = (UBYTE)value;
        buffstart++;
        bufflength--;
        if (quitflag) {
            quitflag = 0;
            return 1;
        }
        if (!bufflength)
            return 0;
    }
}

/**
 * @brief SerialFlush: reads until the receive buffer is empty.
 *
 * BUG(orig) (#792): never opens the port.
 * @param value  d0: d0 at the call (goes through SerialRead)
 */
void BLITZ_ROUTINE c_flushser(ULONG value BL_REG(d0))
{
    do
        value = readser(value);
    while (value != (ULONG)-1);
}

/**
 * @brief SerialSendString a$: meant to send a string.
 *
 * BUG(orig) (#801): sends the low byte of the string's address (with the
 * stop bit) once for every long read from the a2 stack, down to one with
 * a low byte 0 (DIFF: a2 unchanged).  BUG(orig) (#793): calls SerialOpen's
 * code only when the port is open.
 * @param str       d0: the string
 * @param argstack  a2: the argument stack
 */
void BLITZ_ROUTINE c_sendserialstring(ULONG str BL_REG(d0), const ULONG *argstack BL_REG(a2))
{
    ULONG value = str;                  /* d0: as openserial and writeser leave it (#801) */
    ULONG l;

    if (openflag)
        value = openserial(value);
    do {
        l = *--argstack;
        value = writeser(value);
    } while (l & 0xff);
}

/**
 * @brief SerialReadString: received bytes up to a 0, at a3.
 *
 * BUG(orig) (#795): stores up to and including the 0 and leaves a3
 * behind it; BUG(orig) (#796): the count is reset only when the port is
 * open, and then the routine returns to the d0 of the call (assembler
 * thunk __readserialstring, riser_rsreturn); BUG(orig) (#797): the loop
 * ends at a 0 only if the upper word of d0 at the call is 0 and no wait
 * came before; BUG(orig) (#793): calls SerialOpen's code when the port
 * is open.
 * @param callerd0  d0: d0 at the call
 * @return d0: the count of characters - 1
 */
volatile UBYTE riser_rsreturn;

ULONG BLITZ_ROUTINE c_readserialstring(ULONG callerd0 BL_REG(d0))
{
    ULONG value = callerd0;             /* d0: openserial's, then each byte read (#797) */

    riser_rsreturn = 0;
    if (openflag) {
        riser_rsreturn = 1;
        value = openserial(0);
        numchars = 0;
    }
    do {
        while ((value = readser(value)) == (ULONG)-1)
            ;
        numchars++;
        *blitz_string++ = (UBYTE)value;
    } while (value);
    return numchars - 1;
}

/**
 * @brief SerialProgress: the remaining count of the last send (or
 * receive) block.
 * @return d0: the count
 */
ULONG BLITZ_ROUTINE c_buffpos(void)
{
    return bufflength;
}

/**
 * @brief SerialReadLong: meant to read a long.
 *
 * BUG(orig) (#794): never returns (the loop counter is taken back from
 * the stack every time: it reads bytes without end); BUG(orig) (#792):
 * always calls SerialOpen's code.
 * @param longword  d1: what the original builds from the bytes
 */
void BLITZ_ROUTINE c_readlong(ULONG longword BL_REG(d1))
{
    ULONG value = openserial(0);

    for (;;) {
        while ((value = readser(value)) == (ULONG)-1)
            ;
        longword = (longword & 0xffffff00UL) | (value & 0xff);
        longword = longword << 8 | longword >> 24;
    }
}

/**
 * @brief SerialWriteLong longword: meant to send a long.
 *
 * BUG(orig) (#805): sends the low byte, then three times $01 (it rotates
 * the register SerialWrite changed); BUG(orig) (#792): always calls
 * SerialOpen's code.
 * @param longword  d0: the long
 */
void BLITZ_ROUTINE c_writelong(ULONG longword BL_REG(d0))
{
    int i;

    openserial(longword);               /* d0 saved around it */
    for (i = 0; i < 4; i++) {
        longword = writeser(longword);
        longword = longword >> 8 | longword << 24; /* ROR.l #8 */
    }
}

/**
 * @brief SerialStop: ends a block transfer (sets the stop flag).
 */
void BLITZ_ROUTINE c_abortxfer(void)
{
    quitflag = 1;
}

/**
 * @brief SerialGot: whether a byte is waiting.
 *
 * BUG(orig) (#802): -1/0, not the count the help text promises.
 * @return d0: $FFFF (the word -1) if a byte is waiting, else 0
 */
ULONG BLITZ_ROUTINE c_buffcount(void)
{
    return rd.big != rd.end ? 0xffff : 0;
}

/**
 * @brief SerialWait: waits for a byte and reads it (SerialRead).
 * @param value  d0: d0 at the call
 * @return d0: as SerialRead (BUG(orig) (#797))
 */
ULONG BLITZ_ROUTINE c_buffcount2(ULONG value BL_REG(d0))
{
    while (rd.big == rd.end)
        ;
    return readser(value);
}

/**
 * @brief The receive interrupt (level 5): SERDATR's byte into the ring.
 * @return d0: 1 = RBF handled (RTE), 0 = not ours (on to the old vector)
 */
ULONG BLITZ_ROUTINE c_readint(void)
{
    if (!(INTREQR_HI & 0x08))
        return 0;
    rd.buf[(WORD)rd.end] = SERDATR_LO;
    BUMP(rd.end, 1);
    INTREQ = 0x0800;
    return 1;
}

/**
 * @brief The transmit interrupt (level 1): one word of the ring when
 * SERDATR shows TBE.
 * @return d0: 1 = TBE handled (RTE), 0 = not ours (on to the old vector)
 */
ULONG BLITZ_ROUTINE c_writeint(void)
{
    UWORD b;

    if (!(INTREQR_LO & 0x01))
        return 0;
    b = wr.big;
    if (b != wr.end) {
        while (!(SERDATR_HI & 0x20))
            ;
        SERDAT = *(UWORD *)(wr.buf + (WORD)b);
        BUMP(wr.big, 2);
    }
    INTREQ = 0x0001;
    return 1;
}

/* the handlers: RTE, or on to the old vector */
#define BL_INTHANDLER(entry, cfunc, old)                                    \
    __asm__("   .text\n   .even\n   .globl  _" #entry "\n"                  \
            "_" #entry ":\n"                                                \
            "   movem.l d0-d1/a0-a1,-(sp)\n"                                \
            "   jsr     _" #cfunc "\n"                                      \
            "   tst.l   d0\n"                                               \
            "   movem.l (sp)+,d0-d1/a0-a1\n"                                \
            "   beq.s   1f\n"                                               \
            "   rte\n"                                                      \
            "1: move.l  _" #old ",-(sp)\n"                                  \
            "   rts\n")
BL_INTHANDLER(readint, c_readint, riser_oldint5);
BL_INTHANDLER(writeint, c_writeint, riser_oldint1);

/* entries: keep every one of d0/d1/a0/a1 that is not the result */
BLITZ_THUNK(_setbaud, c_setbaud, "d0-d1/a0-a1");
BLITZ_THUNK(_openserial, c_openserial, "d0-d1/a0-a1");
BLITZ_THUNK(_openserial2, c_openserial2, "d0-d1/a0-a1");
BLITZ_THUNK(_closeserial, c_closeserial, "d0-d1/a0-a1");
BLITZ_THUNK(_libfinit, c_closeserial, "d0-d1/a0-a1");
BLITZ_THUNK(_readser, c_readser, "d1/a0-a1");
BLITZ_THUNK(_writeser, c_writeser, "d0-d1/a0-a1");
BLITZ_THUNK(info, c_info, "d1/a0-a1");
BLITZ_THUNK(_showloc, c_showloc, "d1/a0-a1");
BLITZ_THUNK(_sersend, c_sersend, "d0-d1/a0-a1");
BLITZ_THUNK(_flushser, c_flushser, "d0-d1/a0-a1");
BLITZ_THUNK(_sendserialstring, c_sendserialstring, "d0-d1/a0-a1");
BLITZ_THUNK(_buffpos, c_buffpos, "d1/a0-a1");
BLITZ_THUNK(_readlong, c_readlong, "d1/a0-a1");
BLITZ_THUNK(_writelong, c_writelong, "d0-d1/a0-a1");
BLITZ_THUNK(_abortxfer, c_abortxfer, "d0-d1/a0-a1");
BLITZ_THUNK(_buffcount, c_buffcount, "d1/a0-a1");
BLITZ_THUNK(_buffcount2, c_buffcount2, "d1/a0-a1");

/* SerialReceiveBlock: stopped, the original takes its return address
   into d7 and returns to the long above it (BUG(orig) #803) */
__asm__(
"   .text\n"
"   .even\n"
"   .globl  __serreceive\n"
"__serreceive:\n"
"   movem.l d0-d1/a0-a1,-(sp)\n"
"   jsr     _c_serreceive\n"
"   tst.l   d0\n"
"   movem.l (sp)+,d0-d1/a0-a1\n"
"   beq.s   1f\n"
"   move.l  (sp)+,d7\n"
"1: rts\n"
);

/* SerialReadString: with the port open the original pushed the d0 of
   the call and returns to it (BUG(orig) #796) */
__asm__(
"   .text\n"
"   .even\n"
"   .globl  __readserialstring\n"
"__readserialstring:\n"
"   move.l  d0,-(sp)\n"
"   movem.l d1/a0-a1,-(sp)\n"
"   jsr     _c_readserialstring\n"
"   movem.l (sp)+,d1/a0-a1\n"
"   tst.b   _riser_rsreturn\n"
"   bne.s   1f\n"
"   addq.l  #4,sp\n"
"1: rts\n"
);
