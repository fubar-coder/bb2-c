/*
 * seriallib - command library 90: serial.device access - OpenSerial,
 * WriteSerial, WriteSerialString, ReadSerial, ReadSerialString,
 * CloseSerial, SetSerialBuffer, SetSerialLens, SetSerialParams,
 * SerialEvent, ReadSerialMem, WriteSerialMem.
 *
 * C translation of bblibsrc/seriallib ("SERIAL Version 2.0 By King
 * Fuzzy"; binary of the CD's acidlibs checked: same as the source).
 *   finit: _closeallio - AbortIO/WaitIO/CloseDevice and FreeSignal for
 *          every unit still open.
 *
 * Memory: allio, 8 units x 256 bytes inside the library.  Each unit has a
 * read half (+0) and a write half (+128), both `struct serio` (120 bytes):
 *   +0  request   pointer to the IOExtSer below (0 = not open)
 *   +4  port      MsgPort (reply port of the request)
 *   +38 io        IOExtSer
 * OpenSerial sets up the read half, opens the device and copies the 128
 * bytes to the write half, which then gets its own port and signal.
 *
 * Kept from the original (BUG(orig)):
 *   - unit index: OpenSerial, WriteSerial, WriteSerialString,
 *     WriteSerialMem, ReadSerialMem, ReadSerial and ReadSerialString use
 *     unit*256 (ASL.w #8), but CloseSerial, SetSerialBuffer,
 *     SetSerialLens, SetSerialParams and SerialEvent use unit*128
 *     (ASL.w #7): only unit 0 works with all commands; for unit n > 0
 *     those reach the write half of unit (n-1)/2 for an odd n, the read
 *     half of unit n/2 for an even n (CloseSerial 1 closes the
 *     device through unit 0's write request and frees unit 1's read
 *     signal).  Index arithmetic is word sized (ADDA.w: sign extended).
 *   - ReadSerialMem reads through the WRITE request (+128).
 *   - WriteSerialString: flags bit 1 (value 2) selects both TrueString
 *     (length from the string instead of -1 = null terminated) and DoIO;
 *     without it the write is started with SendIO and not waited for.
 *     Bit 0 ("1=DoIO" in the help text) is never tested.  The two
 *     argument form takes a #long (an address), flags 0.
 *   - ReadSerial: the "wait" flag is bit 1 (value 2), not 1 as the help
 *     says; with it the query is skipped and the read waits for a byte.
 *   - ReadSerialString reads io_Actual bytes (unlimited) to a3; d0 =
 *     number of bytes, 0 if nothing is waiting.
 *   - CloseSerial: AbortIO without WaitIO, the write half's request is
 *     neither aborted nor closed (a pending SendIO write stays pending).
 *   - OpenSerial: if the write half's AllocSignal fails, the device stays
 *     open and the write half keeps its request pointer; a second
 *     OpenSerial of an open unit that fails (exclusive access) clears the
 *     read half, so the open device is never closed.
 *   - A unit that is not open: runtime error "Serial Device Not Open"
 *     (TRAP #0, also without the debugger; the original then runs into
 *     the message text, the C version returns).  SerialEvent checks
 *     nothing.
 * Kept from the original, not a bug: ReadSerialString writes no
 * terminating 0 (the compiler appends clr.b (a3)+ itself where it passes
 * a string on, blitz2.s:8767 sbtolong).
 * DIFF: ReadSerial does not change a3 (the original leaves it on its
 * one byte buffer rb1 inside the library); a2 is never changed (the
 * original sets it to the unit's slot, WriteSerialString pops the length).
 * Registers: GCC keeps d2-d7/a2-a6; the original changes d0/d1/a0/a1 in
 * every routine that calls exec, so no thunks are needed except for
 * SerialEvent, which keeps a0/a1 in the original.
 */
#include "blitzlib.h"
#include "blitzexec.h"
#include <devices/serial.h>
#include <stddef.h>

/* entry points (defined below) */
ULONG BLITZ_ROUTINE openserial(UBYTE *name BL_REG(d0), ULONG unit BL_REG(d1),
                               ULONG baud BL_REG(d2), ULONG flag BL_REG(d3));
void BLITZ_ROUTINE writebyte(UWORD unit BL_REG(d0), UBYTE byte BL_REG(d1));
void BLITZ_ROUTINE writestring2(UWORD unit BL_REG(d0), ULONG data BL_REG(d1));
void BLITZ_ROUTINE writestring3(UWORD unit BL_REG(d0), ULONG data BL_REG(d1),
                                UWORD flags BL_REG(d2),
                                const ULONG *lens BL_REG(a2));
ULONG BLITZ_ROUTINE readobyte(UWORD unit BL_REG(d0));
ULONG BLITZ_ROUTINE readbyte(UWORD unit BL_REG(d0), UWORD flags BL_REG(d1));
ULONG BLITZ_ROUTINE readstring(UWORD unit BL_REG(d0));
void BLITZ_ROUTINE closeserial(UWORD unit BL_REG(d0));
void BLITZ_ROUTINE serialbuffer(UWORD unit BL_REG(d0), ULONG len BL_REG(d1));
void BLITZ_ROUTINE seriallens(UWORD unit BL_REG(d0), UBYTE rlen BL_REG(d1),
                              UBYTE wlen BL_REG(d2), UBYTE stop BL_REG(d3));
void BLITZ_ROUTINE serialsetparams(UWORD unit BL_REG(d0));
void checkevent(void);
void BLITZ_ROUTINE readmem(UWORD unit BL_REG(d0), ULONG addr BL_REG(d1),
                           ULONG len BL_REG(d2));
void BLITZ_ROUTINE writemem(UWORD unit BL_REG(d0), ULONG addr BL_REG(d1),
                            ULONG len BL_REG(d2));
void BLITZ_ROUTINE closeallio(void);

BLITZ_ENTRY_DECL(finit);

/* !libheader {#seriallib,0,0,finit,0} */
BLITZ_LIBHEADER(LIB_seriallib, 0, BL_NORET, finit, 0);

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_STRING, BL_LONG, BL_LONG, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(openserial, 0, 0);
BLITZ_NAME("OpenSerial", "device$,unit#,baud,io_serflags");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(writebyte, 0, 0);
BLITZ_NAME("WriteSerial", "unit#,byte");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(writestring2, 0, 0);
BLITZ_ARGS(BL_WORD, BL_STRING, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(writestring3, 0, 0);
BLITZ_NAME("WriteSerialString", "unit#,string[,Flags 1=DoIO 2=TrueString] ");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(readobyte, 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(readbyte, 0, 0);
BLITZ_NAME("ReadSerial", "(unit#[,flags 1=wait on])  returns -1 if nothing waiting");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(readstring, 0, 0);
BLITZ_NAME("ReadSerialString", "(unit#)  returns null if nothing waiting");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(closeserial, 0, 0);
BLITZ_NAME("CloseSerial", "unit#");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(serialbuffer, 0, 0);
BLITZ_NAME("SetSerialBuffer", "unit#,bufferlength");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_BYTE, BL_BYTE, BL_BYTE);
BLITZ_LIBS0();
BLITZ_SUBS(seriallens, 0, 0);
BLITZ_NAME("SetSerialLens", "unit#,readlen,writelen,stopbits");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(serialsetparams, 0, 0);
BLITZ_NAME("SetSerialParams", "unit#");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_utillib, BL_LA(0));
BLITZ_SUBS(checkevent, 0, 0);
BLITZ_NAME("SerialEvent", "(unit#)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(readmem, 0, 0);
BLITZ_NAME("ReadSerialMem", "unit#,Address,Length");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(writemem, 0, 0);
BLITZ_NAME("WriteSerialMem", "unit#,Address,Length");

BLITZ_NULLSUB_AT(finit, closeallio, 0, 0);

BLITZ_LIBFIN();

/* one half of a unit (NEWTYPE .serialio of the original) */
struct serio {
    struct IOExtSer *request;           /* +0  0 = not open */
    struct MsgPort port;                /* +4  */
    struct IOExtSer io;                 /* +38 */
    UBYTE pad[8];                       /* to 128 bytes */
};
static_assert(offsetof(struct serio, io) == 38, "serio layout");
static_assert(sizeof(struct serio) == 128, "serio size");

/* allio: Dcb.b 2048,0 - unit n: read half at 2n, write half at 2n+1 */
static struct serio allio[16];

/* name/unit/baud/flag: the arguments of the last OpenSerial (Dc.l) */
static ULONG oname, ounit, obaud, oflag;
static UBYTE wb1[4];                    /* WriteSerial's byte */
static UBYTE rb1[4];                    /* ReadSerial's byte */

static const char error[] = "Serial Device Not Open";

/* ASL.w #shift,d0:(ADD.w #add,d0):LEA allio,a2:ADDA.w d0,a2 */
static struct serio *slot(UWORD unit, int shift, UWORD add)
{
    return (struct serio *)((UBYTE *)allio
                            + (WORD)(UWORD)((UWORD)(unit << shift) + add));
}

/* noser: MOVE.l #error,d0:TRAP #0
   DIFF (K): if the trap returns, the original runs into the message text
   behind it (with the saved a6 still on the stack); here noser returns and
   the caller returns without I/O */
static void noser(void)
{
    BLITZ_RUNERR(error);
}

/* FreeSignal with the signal zero extended (MOVEQ #0,d0:MOVE.b 19(a2),d0;
   the NDK inline takes a BYTE and would pass $FF as -1) */
static void freesig(struct serio *s)
{
    struct bl_regs r = { .d0 = s->port.mp_SigBit };
    BLITZ_BASEJSR(SysBase, -336, r);
}

/* set up request pointer, port and message of one half (FindTask and
   AllocSignal are done by the caller) */
static void initport(struct serio *s)
{
    struct List *l = &s->port.mp_MsgList;

    s->request = &s->io;
    s->port.mp_Node.ln_Type = NT_MSGPORT;
    l->lh_TailPred = (struct Node *)l;          /* lh_Tail is not touched */
    l->lh_Head = (struct Node *)&l->lh_Tail;
    s->io.IOSer.io_Message.mn_ReplyPort = &s->port;
    s->io.IOSer.io_Message.mn_Node.ln_Type = NT_MESSAGE;
}

/* _openserial - d0 = device name, d1 = unit, d2 = baud, d3 = serflags;
   result: address of the unit's read half, 0 on failure */
ULONG BLITZ_ROUTINE openserial(UBYTE *name BL_REG(d0), ULONG unit BL_REG(d1),
                               ULONG baud BL_REG(d2), ULONG flag BL_REG(d3))
{
    struct serio *r, *w;
    ULONG *src, *dst;
    BYTE sig;
    int i;

    oname = (ULONG)name;
    ounit = unit;
    obaud = baud;
    oflag = flag;
    r = slot((UWORD)unit, 8, 0);

    /* setup read port and open device */
    initport(r);
    r->port.mp_SigTask = FindTask(NULL);
    sig = AllocSignal(-1);
    r->port.mp_SigBit = (UBYTE)sig;
    if (sig < 0)
        goto fail;
    r->io.IOSer.io_Message.mn_Length = sizeof(struct IOExtSer);  /* 82 */
    r->io.io_SerFlags = (UBYTE)oflag;
    if (OpenDevice((CONST_STRPTR)oname, ounit, (struct IORequest *)r->request, 0))
        goto fail2;
    r->io.io_Baud = obaud;
    r->io.io_SerFlags = (UBYTE)oflag;
    r->io.IOSer.io_Command = SDCMD_SETPARAMS;
    DoIO((struct IORequest *)r->request);

    /* copy read iostdreq stuff and init write port */
    src = (ULONG *)r;
    dst = (ULONG *)(r + 1);
    for (i = 0; i < 32; i++)
        *dst++ = *src++;
    w = r + 1;
    initport(w);
    sig = AllocSignal(-1);
    w->port.mp_SigBit = (UBYTE)sig;
    if (sig < 0)
        goto fail2;                     /* fail1: the device stays open */
    return (ULONG)r;

fail2:
    freesig(r);
fail:
    r->request = NULL;
    return 0;
}

/* _closeserial - BUG(orig): unit*128, see the head comment */
void BLITZ_ROUTINE closeserial(UWORD unit BL_REG(d0))
{
    struct serio *s = slot(unit, 7, 0);

    if (!s->request)
        return;
    AbortIO((struct IORequest *)s->request);
    CloseDevice((struct IORequest *)s->request);
    freesig(s);
    s->request = NULL;
    s++;
    if (!s->request)
        return;
    freesig(s);
    s->request = NULL;
}

/* _closeallio (finit) */
void BLITZ_ROUTINE closeallio(void)
{
    struct serio *s = allio;
    int n;

    for (n = 0; n < 8; n++, s += 2) {
        if (s->request) {
            AbortIO((struct IORequest *)s->request);
            WaitIO((struct IORequest *)s->request);
            CloseDevice((struct IORequest *)s->request);
            freesig(s);
            s->request = NULL;
        }
        if (s[1].request) {
            freesig(&s[1]);
            s[1].request = NULL;
        }
    }
}

/* _writebyte - one byte through the write half, DoIO */
void BLITZ_ROUTINE writebyte(UWORD unit BL_REG(d0), UBYTE byte BL_REG(d1))
{
    struct serio *s = slot(unit, 8, 128);

    if (!s->request) {
        noser();
        return;
    }
    wb1[0] = byte;
    s->io.IOSer.io_Command = CMD_WRITE;
    s->io.IOSer.io_Length = 1;
    s->io.IOSer.io_Data = wb1;
    DoIO((struct IORequest *)s->request);
}

/* _writestring3 - d1 = string, d2 = flags, -4(a2) = its length */
void BLITZ_ROUTINE writestring3(UWORD unit BL_REG(d0), ULONG data BL_REG(d1),
                                UWORD flags BL_REG(d2),
                                const ULONG *lens BL_REG(a2))
{
    const ULONG *sp = lens;
    ULONG len = BL_POPL(sp);            /* MOVE.l -(a2),d3 */
    struct serio *s = slot(unit, 8, 128);

    if (!s->request) {
        noser();
        return;
    }
    s->io.IOSer.io_Command = CMD_WRITE;
    s->io.IOSer.io_Length = (ULONG)-1;
    if (flags & 2)                      /* trueString */
        s->io.IOSer.io_Length = len;
    s->io.IOSer.io_Data = (APTR)data;
    if (flags & 2)                      /* BUG(orig): DoIO on bit 1 too */
        DoIO((struct IORequest *)s->request);
    else
        SendIO((struct IORequest *)s->request);
}

/* _writestring2 - MOVEQ #0,d2 then _writestring3 (the length popped there
   is not used with flags 0, so it is not read here) */
void BLITZ_ROUTINE writestring2(UWORD unit BL_REG(d0), ULONG data BL_REG(d1))
{
    struct serio *s = slot(unit, 8, 128);

    if (!s->request) {
        noser();
        return;
    }
    s->io.IOSer.io_Command = CMD_WRITE;
    s->io.IOSer.io_Length = (ULONG)-1;
    s->io.IOSer.io_Data = (APTR)data;
    SendIO((struct IORequest *)s->request);
}

/* command `cmd` with length/data through the write half, DoIO */
static void memio(UWORD unit, UWORD cmd, ULONG addr, ULONG len)
{
    struct serio *s = slot(unit, 8, 128);

    if (!s->request) {
        noser();
        return;
    }
    s->io.IOSer.io_Command = cmd;
    s->io.IOSer.io_Length = len;
    s->io.IOSer.io_Data = (APTR)addr;
    DoIO((struct IORequest *)s->request);
}

/* _writemem */
void BLITZ_ROUTINE writemem(UWORD unit BL_REG(d0), ULONG addr BL_REG(d1),
                            ULONG len BL_REG(d2))
{
    memio(unit, CMD_WRITE, addr, len);
}

/* _readmem - BUG(orig): CMD_READ through the write half */
void BLITZ_ROUTINE readmem(UWORD unit BL_REG(d0), ULONG addr BL_REG(d1),
                           ULONG len BL_REG(d2))
{
    memio(unit, CMD_READ, addr, len);
}

/* _readbyte - d1 bit 1 = wait (no query); -1 if nothing is waiting */
ULONG BLITZ_ROUTINE readbyte(UWORD unit BL_REG(d0), UWORD flags BL_REG(d1))
{
    struct serio *s = slot(unit, 8, 0);

    if (!s->request) {
        noser();
        return 0;
    }
    if (!(flags & 2)) {
        s->io.IOSer.io_Command = SDCMD_QUERY;
        DoIO((struct IORequest *)s->request);
        if ((LONG)s->io.IOSer.io_Actual <= 0)
            return (ULONG)-1;           /* nobytes */
    }
    /* ral - DIFF: the original reads through a3 = rb1 and leaves it there */
    s->io.IOSer.io_Command = CMD_READ;
    s->io.IOSer.io_Length = 1;
    s->io.IOSer.io_Data = rb1;
    DoIO((struct IORequest *)s->request);
    return rb1[0];
}

/* _readobyte - MOVEQ #0,d1 then _readbyte */
ULONG BLITZ_ROUTINE readobyte(UWORD unit BL_REG(d0))
{
    return readbyte(unit, 0);
}

/* _readstring - everything waiting, to a3 (no terminating 0) */
ULONG BLITZ_ROUTINE readstring(UWORD unit BL_REG(d0))
{
    struct serio *s = slot(unit, 8, 0);
    ULONG n;

    if (!s->request) {
        noser();
        return 0;
    }
    s->io.IOSer.io_Command = SDCMD_QUERY;
    DoIO((struct IORequest *)s->request);
    n = s->io.IOSer.io_Actual;
    if ((LONG)n <= 0)
        return 0;                       /* nobytes2 */
    s->io.IOSer.io_Command = CMD_READ;
    s->io.IOSer.io_Length = n;
    s->io.IOSer.io_Data = blitz_string;
    DoIO((struct IORequest *)s->request);
    blitz_string += n;
    return n;
}

/* SDCMD_SETPARAMS through the half at unit*128 (BUG(orig), see above) */
static struct serio *paramslot(UWORD unit)
{
    struct serio *s = slot(unit, 7, 0);

    if (!s->request) {
        noser();
        return NULL;
    }
    return s;
}

static void setparams(struct serio *s)
{
    s->io.IOSer.io_Command = SDCMD_SETPARAMS;
    DoIO((struct IORequest *)s->request);
}

/* _serialbuffer */
void BLITZ_ROUTINE serialbuffer(UWORD unit BL_REG(d0), ULONG len BL_REG(d1))
{
    struct serio *s = paramslot(unit);

    if (s) {
        s->io.io_RBufLen = len;
        setparams(s);
    }
}

/* _seriallens */
void BLITZ_ROUTINE seriallens(UWORD unit BL_REG(d0), UBYTE rlen BL_REG(d1),
                              UBYTE wlen BL_REG(d2), UBYTE stop BL_REG(d3))
{
    struct serio *s = paramslot(unit);

    if (s) {
        s->io.io_ReadLen = rlen;
        s->io.io_WriteLen = wlen;
        s->io.io_StopBits = stop;
        setparams(s);
    }
}

/* _serialsetparams */
void BLITZ_ROUTINE serialsetparams(UWORD unit BL_REG(d0))
{
    struct serio *s = paramslot(unit);

    if (s)
        setparams(s);
}

/* _checkevent - a0 = utillib's `signals` (result of its Wait); 1 if the
   write port's signal is set, 2 if the read port's (2 wins); no check
   whether the unit is open.  BUG(orig): unit*128. */
static ULONG BLITZ_ROUTINE checkevent_c(UWORD unit BL_REG(d0),
                                        const ULONG *sigs BL_REG(a0))
{
    ULONG d7 = *sigs;
    struct serio *s = slot(unit, 7, 0);
    ULONG r = 0;

    if (d7 & (1UL << (s[1].port.mp_SigBit & 31)))     /* port+15+128 */
        r = 1;
    if (d7 & (1UL << (s->port.mp_SigBit & 31)))       /* port+15 */
        r = 2;
    return r;
}
BLITZ_THUNK(checkevent, checkevent_c, "a0-a1");
