/*
 * ritrackdisklib - RWE userlib 91 (#ritrackdisklib): sector access to
 * the floppy drives through trackdisk.device - OpenDisk, CloseDisk,
 * ReadSector, WriteSector, FormatTrack, WriteBoot, BootProg, MotorOn,
 * MotorOff, DiskStatus, DiskIO, DiskMotor.
 *
 * C translation of rwelibsrc/trackdisklib/trackdisk2.bb (trackdisk.bb has
 * the same code); the declarations equal the CD's BSS2 RITRACKDISKLIB.OBJ
 * (ctest rwe_declarations_ritrackdisklib), the behaviour is compared with
 * it in libs/tests/rwe/ritrackdisklib.c.  Notes:
 * docs/rwelibs/port/ritrackdisklib.md.
 *   init:  the reply port msgport, built by hand (newport(); 0 if that
 *          fails: then every command returns 0)
 *   finit: CloseDisk 0-3 and frees the port (freeport())
 *
 * Units 0-3: one IOStdReq (200 bytes, built by hand: newreq()) per unit
 * in iorequests, all with the reply port msgport.  Port and requests use
 * exec V33 functions only (AllocMem, AllocSignal, FindTask), the same path
 * on every Kickstart.  Sector commands: d1 =
 * first sector (FormatTrack: track, * 11), d3 = number of sectors
 * (default 1), length and offset = * 512; result: d0 of DoIO with
 * io_Error as its low byte (MOVE.b 31(a0),d0), 0 without port/request.
 * DiskStatus, DiskIO, BootProg and DiskMotor are Amiga mode only (odd
 * code address in !subs).
 *
 * Everything is C: exec through proto/exec.h (DoIO through BLITZ_BASEJSR,
 * the original returns all of DoIO's d0, the NDK inline only a BYTE).
 *
 * Fixed (#857): the original's DiskMotor returned SNE of bit 7 of CIAB
 * PRB ($BFD100), which trackdisk.device leaves 1 after every request:
 * always -1.  The library keeps the motor state of its units itself.
 * Fixed (#858, #864): the original's FormatTrack without a count and
 * WriteBoot sent TD_FORMAT requests of 512 and 1024 bytes, which
 * trackdisk.device refuses (-4, IOERR_BADLENGTH: only whole tracks of
 * 5632 bytes); WriteBoot unit,bootdata wrote from the caller's d2 in
 * place of bootdata.  FormatTrack's count stays in sectors (only
 * multiples of 11 work), the form without a count formats one track,
 * WriteBoot writes sectors 0-1 with CMD_WRITE from bootdata.
 * Fixed (#865): the original's WriteBoot unit wrote its 8 byte default
 * block ("DOS",0, -1) and the 1016 bytes behind it.  It writes a bootable
 * standard boot block now: "DOS" with the flag byte of the disk's boot
 * block, the checksum, root block 880, boot code of its own.
 * Fixed (#1154): the original never sent CMD_UPDATE: trackdisk.device
 * keeps written sectors in its track buffer until it changes the track,
 * so the last track written was lost when the disk was taken out.
 * MotorOff (and with it CloseDisk and finit) sends CMD_UPDATE first.
 * Fixed (#856): the original's BootProg stored the plain sum with carry
 * of the 256 longs including the old checksum long (not cleared first,
 * not complemented) and cleared the root block long after the sum.
 * Fixed (#859, #860): the original's OpenDisk unit,name$ copied the name
 * into a buffer of 58 bytes without a limit (a longer name overwrote the
 * request table and the port) and kept it there, so a later OpenDisk
 * unit opened that device too.  The name goes to OpenDevice as it is;
 * OpenDisk unit always opens trackdisk.device.
 * Fixed (#861): the original's OpenDisk let a negative unit pass its
 * check (CMP #3: BGT) and lost the request (the table ignores it).
 * Fixed (#862): the original's OpenDisk of a unit that is open replaced
 * its request (the old one stayed open, lost).
 * Fixed (#863): the original's finit closed units 0-2 only, not 3, and
 * its init cleared only the first three entries of iorequests.
 * Fixed (#941): the original called CreateMsgPort, CreateIORequest,
 * DeleteIORequest and DeleteMsgPort (exec V36) without a version check:
 * on Kickstart 1.x every program with the library crashed at start-up.
 * Registers: the original changes d0/d1/a0/a1, a6 and d4-d7 in most
 * commands, but keeps d1/a0/a1 on some paths (no port or request); the
 * thunks keep d1/a0/a1 everywhere, GCC keeps d2-d7/a2-a6.
 */
#include "blitzlib.h"
#include "blitzexec.h"
#include <exec/io.h>
#include <devices/trackdisk.h>
#include <devices/bootblock.h>

/* entries (thunks below) */
void ritrackdisklib_libinit(void), ritrackdisklib_libfinit(void);
void opentrack(void), opentrack2(void), motoron(void), closedisk(void);
void readsector1(void), readsector2(void), motoroff(void), writesector1(void);
void writesector2(void), formatsector1(void), formatsector2(void);
void install1(void), install2(void), iostatus(void), ioreq(void);
void bootprog(void), diskmotor(void);

BLITZ_ENTRY_DECL(finit);
BLITZ_ENTRY_DECL(init);

/* !libheader {#ritrackdisklib,init,0,finit,0} */
BLITZ_LIBHEADER(LIB_ritrackdisklib, init, BL_NORET, finit, 0);

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(opentrack, 0, 0);
BLITZ_ARGS(BL_LONG, BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(opentrack2, 0, 0);
BLITZ_NAME("OpenDisk", "unit#[,devicename$]");

BLITZ_ACOMMAND(BL_BYTE);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(motoron, 0, 0);
BLITZ_NAME("MotorOn", "unit#");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(closedisk, 0, 0);
BLITZ_NAME("CloseDisk", "unit#");

BLITZ_ACOMMAND(BL_BYTE);
BLITZ_ARGS(BL_LONG, BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(readsector1, 0, 0);
BLITZ_ARGS(BL_LONG, BL_LONG, BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(readsector2, 0, 0);
BLITZ_NAME("ReadSector", "unit#,sector#,buffer[,numsectors]");

BLITZ_ACOMMAND(BL_BYTE);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(motoroff, 0, 0);
BLITZ_NAME("MotorOff", "unit#");

BLITZ_ACOMMAND(BL_BYTE);
BLITZ_ARGS(BL_LONG, BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(writesector1, 0, 0);
BLITZ_ARGS(BL_LONG, BL_LONG, BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(writesector2, 0, 0);
BLITZ_NAME("WriteSector", "unit#,sector#,buffer[,numsectors]");

BLITZ_ACOMMAND(BL_BYTE);
BLITZ_ARGS(BL_LONG, BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(formatsector1, 0, 0);
BLITZ_ARGS(BL_LONG, BL_LONG, BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(formatsector2, 0, 0);
BLITZ_NAME("FormatTrack", "unit#,track#,buffer[,numtracks]");

BLITZ_ACOMMAND(BL_BYTE);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(install1, 0, 0);
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(install2, 0, 0);
BLITZ_NAME("WriteBoot", "unit#[,bootdata]");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(iostatus), 0, 0);
BLITZ_NAME("DiskStatus", "unit#");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(ioreq), 0, 0);
BLITZ_NAME("DiskIO", "unit#");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(bootprog), 0, 0);
BLITZ_NAME("BootProg", "unit#,memoryaddress");

BLITZ_AFUNCTION(BL_BYTE);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(diskmotor), 0, 0);
BLITZ_NAME("DiskMotor", "  ; Returns -1 if motor is on");

BLITZ_NULLSUB_AT(finit, ritrackdisklib_libfinit, 0, 0);
BLITZ_NULLSUB_AT(init, ritrackdisklib_libinit, 0, 0);

BLITZ_LIBFIN();

#define LVO_DoIO (-456)

/* the library's data: the requests of units 0-3, the reply port, the
   motor state of units 0-3 as the library knows it (#857) */
static struct {
    struct IOStdReq *iorequests[4];
    struct MsgPort *msgport;
    UBYTE motor[4];
} td;

/* getioreq: the request of a unit, NULL for units outside 0-3 */
static struct IOStdReq *getioreq(LONG unit)
{
    if (unit < 0 || unit > 3)
        return NULL;
    return td.iorequests[unit];
}

/* putioreq: ignores units outside 0-3 */
static void putioreq(LONG unit, struct IOStdReq *req)
{
    if (unit < 0 || unit > 3)
        return;
    td.iorequests[unit] = req;
}

/*
 * Fixed (#941): reply port and requests built by hand with exec V33
 * functions, one path for every Kickstart (in place of exec V36's
 * CreateMsgPort/DeleteMsgPort and CreateIORequest/DeleteIORequest).
 */

/* newport: a signal port of the calling task (as CreateMsgPort): the
   MsgPort cleared, a signal bit, PA_SIGNAL, an empty message list; NULL
   without memory or a free signal */
static struct MsgPort *newport(void)
{
    struct MsgPort *mp = AllocMem(sizeof *mp, MEMF_PUBLIC | MEMF_CLEAR);
    BYTE sig;

    if (!mp)
        return NULL;
    sig = AllocSignal(-1);
    if (sig < 0) {
        FreeMem(mp, sizeof *mp);
        return NULL;
    }
    mp->mp_Node.ln_Type = NT_MSGPORT;
    mp->mp_Flags = PA_SIGNAL;
    mp->mp_SigBit = sig;
    mp->mp_SigTask = FindTask(NULL);
    mp->mp_MsgList.lh_Head = (struct Node *)&mp->mp_MsgList.lh_Tail;
    mp->mp_MsgList.lh_TailPred = (struct Node *)&mp->mp_MsgList.lh_Head;
    return mp;
}

/* freeport: frees the signal bit and the port of newport() */
static void freeport(struct MsgPort *mp)
{
    FreeSignal(mp->mp_SigBit);
    FreeMem(mp, sizeof *mp);
}

/* the size of a request, as the original's CreateIORequest(port, 200) */
#define REQSIZE 200

/* newreq: a cleared request of REQSIZE bytes on msgport with
   mn_ReplyPort and mn_Length set (as CreateIORequest) and NT_REPLYMSG:
   CheckIO takes a request that was never sent for done, as one of
   CreateIORequest (DiskStatus after OpenDisk); NULL without memory */
static struct IOStdReq *newreq(void)
{
    struct IOStdReq *req = AllocMem(REQSIZE, MEMF_PUBLIC | MEMF_CLEAR);

    if (req) {
        req->io_Message.mn_Node.ln_Type = NT_REPLYMSG;
        req->io_Message.mn_ReplyPort = td.msgport;
        req->io_Message.mn_Length = REQSIZE;
    }
    return req;
}

/* freereq: frees a request of newreq(); NULL is ignored */
static void freereq(struct IOStdReq *req)
{
    if (req)
        FreeMem(req, REQSIZE);
}

/* DoIO and the result of the commands: DoIO's d0 with io_Error as the
   low byte (MOVE.b 31(a0),d0) */
static ULONG doio(struct IOStdReq *req)
{
    struct bl_regs r = { .a1 = (ULONG)req };
    ULONG result = BLITZ_BASEJSR(SysBase, LVO_DoIO, r);

    BL_SETB(result, req->io_Error);
    return result;
}

/**
 * @brief init: clear the request table and create the reply port.
 *
 * Fixed (#863): all four entries (the original cleared 0-2 only).
 * Without a port (msgport 0) every command returns 0.
 */
static void BLITZ_ROUTINE c_libinit(void)
{
    int u;

    for (u = 0; u < 4; u++) {           /* Fixed (#863): also [3] */
        td.iorequests[u] = NULL;
        td.motor[u] = 0;                /* Fixed (#857) */
    }
    td.msgport = newport();             /* Fixed (#941): by hand, V33 */
}

/**
 * @brief MotorOff unit#: write the track buffer to the disk (CMD_UPDATE), then switch the drive motor off (TD_MOTOR, length 0).
 *
 * Fixed (#1154): CMD_UPDATE first (the original only switched the motor
 * off: the last track written stayed in trackdisk.device's buffer).
 * Its result is not looked at; the motor goes off in any case.
 * CloseDisk and finit switch the motor off through this routine.
 * @param unit  d0: unit# (0-3)
 * @return d0: DoIO's d0 of TD_MOTOR with io_Error as its low byte, 0
 *         without port or request
 */
ULONG BLITZ_ROUTINE c_motoroff(LONG unit BL_REG(d0))
{
    struct IOStdReq *req;
    ULONG r;

    if (!td.msgport || !(req = getioreq(unit)))
        return 0;
    req->io_Command = CMD_UPDATE;       /* Fixed (#1154) */
    req->io_Flags = 0;
    doio(req);
    req->io_Command = TD_MOTOR;
    req->io_Flags = 0;
    req->io_Length = 0;
    r = doio(req);
    if (!(UBYTE)r)                      /* Fixed (#857): the library's state */
        td.motor[unit] = 0;
    return r;
}

/**
 * @brief CloseDisk unit#: switch the motor off, close the device, free the request.
 *
 * @param unit  d0: unit# (0-3)
 * @return d0: BB_TRUE if the unit was open, else BB_FALSE
 */
BB_BOOL BLITZ_ROUTINE c_closedisk(LONG unit BL_REG(d0))
{
    struct IOStdReq *req;

    if (unit > 3 || !td.msgport || !(req = getioreq(unit)))
        return BB_FALSE;
    c_motoroff(unit);
    CloseDevice((struct IORequest *)req);
    freereq(req);
    putioreq(unit, NULL);
    td.motor[unit] = 0;                 /* Fixed (#857) */
    return BB_TRUE;
}

/**
 * @brief finit: CloseDisk 0-3 and free the reply port.
 *
 * Fixed (#863): unit 3 too (the original closed 0-2 only).
 */
static void BLITZ_ROUTINE c_libfinit(void)
{
    int u;

    for (u = 0; u < 4; u++)             /* Fixed (#863): also 3 */
        c_closedisk(u);
    if (td.msgport) {
        freeport(td.msgport);
        td.msgport = NULL;
    }
}

/* opendisk: OpenDisk of a unit with the device name (both forms).
   Fixed (#862): a unit that is open is left alone, the result is 0 (the
   original made a new request, the old one stayed open and was lost).
   Fixed (#861): a unit outside 0-3 is refused before the request is made
   (the original's signed CMP #3: BGT let negative units pass; the table
   ignored them, so the request was lost or 0 was freed). */
static BB_BOOL opendisk(LONG unit, const char *name)
{
    struct IOStdReq *req;

    if ((ULONG)unit > 3 || !td.msgport) /* Fixed (#861): unsigned */
        return BB_FALSE;
    if (td.iorequests[unit])            /* Fixed (#862): open, left alone */
        return BB_FALSE;
    req = newreq();
    if (!req)
        return BB_FALSE;
    putioreq(unit, req);
    if (OpenDevice((CONST_STRPTR)name, unit, (struct IORequest *)req, 0)) {
        putioreq(unit, NULL);
        freereq(req);
        return BB_FALSE;
    }
    return BB_TRUE;
}

/**
 * @brief OpenDisk unit#: open trackdisk.device for a unit with a new request.
 *
 * A new request (newreq(), 200 bytes) on the reply port, then OpenDevice
 * of "trackdisk.device".  Fixed (#859): always trackdisk.device (the
 * original opened the device of the last OpenDisk unit,name$).  A unit
 * that is open, or outside 0-3, gives 0 (#862, #861).
 * @param unit  d0: unit# (0-3)
 * @return d0: BB_TRUE if the device opened, else BB_FALSE
 */
BB_BOOL BLITZ_ROUTINE c_opentrack(LONG unit BL_REG(d0))
{
    return opendisk(unit, "trackdisk.device");
}

/**
 * @brief OpenDisk unit#,devicename$: c_opentrack() with another device.
 *
 * Fixed (#860, #859): the name goes to OpenDevice as it is (the original
 * copied it into a buffer of 58 bytes without a limit, a longer name
 * overwrote the request table and the port, and kept it for later
 * OpenDisks without a name).
 * @param unit  d0: unit# (0-3)
 * @param name  d1: the device name
 * @return d0: as c_opentrack()
 */
BB_BOOL BLITZ_ROUTINE c_opentrack2(LONG unit BL_REG(d0), const char *name BL_REG(d1))
{
    return opendisk(unit, name);
}

/* the sectors of a track of a DD disk */
#define NUMSECS 11

/* okok: the sector commands; d3 = number of sectors, d1 = first sector,
   both * 512 */
static ULONG sectorio(LONG unit, ULONG offset, APTR buffer, ULONG num, UWORD cmd)
{
    struct IOStdReq *req;
    ULONG r;

    if (!td.msgport || !(req = getioreq(unit)))
        return 0;
    req->io_Message.mn_ReplyPort = td.msgport;
    req->io_Command = cmd;
    req->io_Flags = 0;
    req->io_Error = 0;
    req->io_Actual = 0;
    req->io_Length = num << 9;
    req->io_Data = buffer;
    req->io_Offset = offset << 9;
    req->io_Flags = 0;
    r = doio(req);
    if (!(UBYTE)r)                      /* Fixed (#857): the device switched */
        td.motor[unit] = 1;             /* the motor on for the request */
    return r;
}

/**
 * @brief ReadSector unit#,sector#,buffer,numsectors: read sectors (CMD_READ).
 *
 * Offset sector# * 512, length numsectors * 512.
 * @param unit    d0: unit# (0-3)
 * @param sector  d1: the first sector
 * @param buffer  d2: the buffer
 * @param num     d3: the number of sectors
 * @return d0: DoIO's d0 with io_Error as its low byte, 0 without port or
 *         request
 */
ULONG BLITZ_ROUTINE c_readsector2(LONG unit BL_REG(d0), ULONG sector BL_REG(d1),
                                  APTR buffer BL_REG(d2), ULONG num BL_REG(d3))
{
    return sectorio(unit, sector, buffer, num, CMD_READ);
}

/**
 * @brief ReadSector unit#,sector#,buffer: read one sector.
 * @param unit    d0: unit# (0-3)
 * @param sector  d1: the sector
 * @param buffer  d2: the buffer
 * @return d0: as c_readsector2()
 */
ULONG BLITZ_ROUTINE c_readsector1(LONG unit BL_REG(d0), ULONG sector BL_REG(d1),
                                  APTR buffer BL_REG(d2))
{
    return sectorio(unit, sector, buffer, 1, CMD_READ);
}

/**
 * @brief WriteSector unit#,sector#,buffer,numsectors: write sectors (CMD_WRITE).
 * @param unit    d0: unit# (0-3)
 * @param sector  d1: the first sector
 * @param buffer  d2: the data
 * @param num     d3: the number of sectors
 * @return d0: DoIO's d0 with io_Error as its low byte, 0 without port or
 *         request
 */
ULONG BLITZ_ROUTINE c_writesector2(LONG unit BL_REG(d0), ULONG sector BL_REG(d1),
                                   APTR buffer BL_REG(d2), ULONG num BL_REG(d3))
{
    return sectorio(unit, sector, buffer, num, CMD_WRITE);
}

/**
 * @brief WriteSector unit#,sector#,buffer: write one sector.
 * @param unit    d0: unit# (0-3)
 * @param sector  d1: the sector
 * @param buffer  d2: the data
 * @return d0: as c_writesector2()
 */
ULONG BLITZ_ROUTINE c_writesector1(LONG unit BL_REG(d0), ULONG sector BL_REG(d1),
                                   APTR buffer BL_REG(d2))
{
    return sectorio(unit, sector, buffer, 1, CMD_WRITE);
}

/**
 * @brief FormatTrack unit#,track#,buffer,numsectors: format tracks (TD_FORMAT).
 *
 * The first sector is track * 11 (MULU.w: the low word of the track),
 * the length numsectors * 512, as in the original: the count is in
 * sectors, though the declaration calls it numtracks.  trackdisk.device
 * formats whole tracks only (5632 bytes = 11 sectors on a DD disk) and
 * refuses another length with -4 (IOERR_BADLENGTH): 11 formats one
 * track, 22 two (#858, decided: the count stays in sectors).
 * @param unit    d0: unit# (0-3)
 * @param track   d1: the first track
 * @param buffer  d2: the data (numsectors * 512 bytes)
 * @param num     d3: the number of sectors (a multiple of 11)
 * @return d0: DoIO's d0 with io_Error as its low byte, 0 without port or
 *         request
 */
ULONG BLITZ_ROUTINE c_formatsector2(LONG unit BL_REG(d0), ULONG track BL_REG(d1),
                                    APTR buffer BL_REG(d2), ULONG num BL_REG(d3))
{
    return sectorio(unit, bl_mulu_w((UWORD)track, 11), buffer, num, TD_FORMAT);
}

/**
 * @brief FormatTrack unit#,track#,buffer: format one track.
 *
 * Fixed (#858): 11 sectors (5632 bytes), one track of a DD disk; the
 * original asked for 1 sector, which trackdisk.device refuses (-4).
 * @param unit    d0: unit# (0-3)
 * @param track   d1: the track
 * @param buffer  d2: the data (5632 bytes)
 * @return d0: as c_formatsector2()
 */
ULONG BLITZ_ROUTINE c_formatsector1(LONG unit BL_REG(d0), ULONG track BL_REG(d1),
                                    APTR buffer BL_REG(d2))
{
    return c_formatsector2(unit, track, buffer, NUMSECS); /* Fixed (#858) */
}

/* the boot code of the standard boot block, written for this library
   (Fixed, #865): Strap calls it with a6 = SysBase; it returns the
   initialisation of the dos.library resident in a0 with d0 = 0, or
   d0 = -1 if there is none.  At offset 12 of the block:
       lea     dosname(pc),a1          43FA 0018
       jsr     -96(a6)                 4EAE FFA0   FindResident
       tst.l   d0                      4A80
       beq.s   fail                    670A
       movea.l d0,a0                   2040
       movea.l 22(a0),a0               2068 0016   rt_Init
       moveq   #0,d0                   7000
       rts                             4E75
   fail:
       moveq   #-1,d0                  70FF
       rts                             4E75
   dosname: "dos.library",0 */
static const UBYTE bootcode[] = {
    0x43, 0xfa, 0x00, 0x18, 0x4e, 0xae, 0xff, 0xa0, 0x4a, 0x80, 0x67, 0x0a,
    0x20, 0x40, 0x20, 0x68, 0x00, 0x16, 0x70, 0x00, 0x4e, 0x75, 0x70, 0xff,
    0x4e, 0x75, 'd', 'o', 's', '.', 'l', 'i', 'b', 'r', 'a', 'r', 'y', 0
};

/* the root block of a DD disk */
#define ROOTBLOCK 880

static void checksum(UBYTE *mem);

/**
 * @brief WriteBoot unit#: write a bootable standard boot block (sectors 0-1).
 *
 * Fixed (#865): reads sectors 0-1 first and keeps the flag byte of the
 * disk's boot block (byte 3, the file system: 0 OFS, 1 FFS, ...) if it
 * starts with "DOS", else 0; then writes "DOS" and that byte, root block
 * 880, the boot code above and the checksum (checksum()).  The buffer is
 * chip memory (1024 bytes, freed again).  The original wrote its 8 byte
 * block ("DOS",0, -1) and the 1016 bytes behind it.  Fixed (#858): with
 * CMD_WRITE (the original's TD_FORMAT of 1024 bytes was refused).
 * @param unit  d0: unit# (0-3)
 * @return d0: as c_writesector2(); 0 without port or request,
 *         TDERR_NoMem (31) without chip memory for the buffer
 */
ULONG BLITZ_ROUTINE c_install1(LONG unit BL_REG(d0))
{
    UBYTE *mem, flag = 0;
    ULONG r;
    int i;

    if (!td.msgport || !getioreq(unit))
        return 0;
    mem = AllocMem(1024, MEMF_CHIP | MEMF_CLEAR);
    if (!mem)
        return TDERR_NoMem;
    if (!(UBYTE)c_readsector2(unit, 0, mem, 2) &&
        mem[0] == 'D' && mem[1] == 'O' && mem[2] == 'S')
        flag = mem[3];
    for (i = 0; i < 1024; i++)
        mem[i] = 0;
    mem[0] = 'D';
    mem[1] = 'O';
    mem[2] = 'S';
    mem[3] = flag;
    ((struct BootBlock *)mem)->bb_dosblock = ROOTBLOCK;
    for (i = 0; i < (int)sizeof bootcode; i++)
        mem[12 + i] = bootcode[i];
    checksum(mem);
    r = c_writesector2(unit, 0, mem, 2);
    FreeMem(mem, 1024);
    return r;
}

/**
 * @brief WriteBoot unit#,bootdata: write 1024 bytes of bootdata to the boot block (sectors 0-1).
 *
 * The block is written as it is (no checksum: BootProg makes one).
 * Fixed (#864): from bootdata (the original set d1 = 0, the sector, over
 * bootdata and wrote from the caller's d2, which is no argument).
 * Fixed (#858): with CMD_WRITE (the original's TD_FORMAT of 1024 bytes
 * was refused).
 * @param unit    d0: unit# (0-3)
 * @param buffer  d1: bootdata, 1024 bytes
 * @return d0: as c_writesector2()
 */
ULONG BLITZ_ROUTINE c_install2(LONG unit BL_REG(d0), APTR buffer BL_REG(d1))
{
    return c_writesector2(unit, 0, buffer, 2);
}

/* checksum: the boot block checksum of the 1024 bytes at mem, as
   Kickstart checks it: the checksum long cleared, the 256 longs added
   with end-around carry, the complement stored (the sum of the block is
   then $FFFFFFFF).  Fixed (#856). */
static void checksum(UBYTE *mem)
{
    struct BootBlock *bb = (struct BootBlock *)mem;
    const ULONG *p = (const ULONG *)mem;
    ULONG sum = 0;
    int i;

    bb->bb_chksum = 0;
    for (i = 0; i < 256; i++) {         /* ADD.l (a0)+,d0:BCC:ADDQ.l #1 */
        ULONG v = p[i];
        sum += v;
        if (sum < v)
            sum++;
    }
    bb->bb_chksum = ~sum;
}

/**
 * @brief BootProg unit#,memoryaddress: make a boot block of 1024 bytes in memory bootable and write it.
 *
 * Writes "DOS",0, the root block long 0 and the checksum (checksum()),
 * then sectors 0-1 (WriteSector).  Fixed (#856): the checksum of the
 * block as Kickstart checks it (the original stored the plain sum with
 * carry including the old checksum long, and cleared the root block long
 * after the sum).
 * @param unit  d0: unit# (0-3)
 * @param mem   d1: the boot block (1024 bytes)
 * @return d0: as c_writesector2(), 0 without port
 */
ULONG BLITZ_ROUTINE c_bootprog(LONG unit BL_REG(d0), UBYTE *mem BL_REG(d1))
{
    if (!td.msgport)
        return 0;
    mem[0] = 'D';                       /* bb_id, byte by byte (MOVE.b; */
    mem[1] = 'O';                       /* as bb->bb_id GCC would merge */
    mem[2] = 'S';                       /* them into words) */
    mem[3] = 0;
    ((struct BootBlock *)mem)->bb_dosblock = 0; /* Fixed (#856): before the sum */
    checksum(mem);
    return c_writesector2(unit, 0, mem, 2);
}

/**
 * @brief DiskStatus unit#: CheckIO of the unit's request.
 * @param unit  d0: unit# (0-3)
 * @return d0: CheckIO's result (the request if it is done, else 0), 0
 *         without port or request
 */
ULONG BLITZ_ROUTINE c_iostatus(LONG unit BL_REG(d0))
{
    struct IOStdReq *req;

    if (!td.msgport || !(req = getioreq(unit)))
        return 0;
    return (ULONG)CheckIO((struct IORequest *)req);
}

/**
 * @brief DiskIO unit#: the unit's IOStdReq.
 * @param unit  d0: unit# (0-3)
 * @return d0: the request, 0 without port or request
 */
ULONG BLITZ_ROUTINE c_ioreq(LONG unit BL_REG(d0))
{
    if (!td.msgport)
        return 0;
    return (ULONG)getioreq(unit);
}

/**
 * @brief MotorOn unit#: switch the drive motor on (TD_MOTOR, length 1).
 * @param unit  d0: unit# (0-3)
 * @return d0: DoIO's d0 with io_Error as its low byte (a #byte result), 0
 *         without port or request
 */
ULONG BLITZ_ROUTINE c_motoron(LONG unit BL_REG(d0))
{
    struct IOStdReq *req;
    ULONG r;

    if (!td.msgport || !(req = getioreq(unit)))
        return 0;
    req->io_Message.mn_ReplyPort = td.msgport;
    req->io_Command = TD_MOTOR;
    req->io_Length = 1;
    req->io_Flags = 0;
    r = doio(req);
    if (!(UBYTE)r)                      /* Fixed (#857): the library's state */
        td.motor[unit] = 1;
    return r;
}

/**
 * @brief DiskMotor: -1 if the motor of a unit the library has open is on, as the library knows it.
 *
 * Fixed (#857): the state is the library's own knowledge: on after a
 * MotorOn or a ReadSector, WriteSector, FormatTrack, WriteBoot or
 * BootProg that worked (trackdisk.device switches the motor on for
 * them and leaves it on), off after a MotorOff that worked and after
 * CloseDisk.  What other programs do with the drives is not seen.  The
 * original returned SNE of bit 7 of CIAB PRB ($BFD100, /MTR), which
 * trackdisk.device leaves 1 after every request (it deselects the
 * drives): always -1.
 * @return d0: $FF (-1) if a unit's motor is on, else 0 (a #byte result)
 */
ULONG BLITZ_ROUTINE c_diskmotor(void)
{
    int u;

    for (u = 0; u < 4; u++)
        if (td.iorequests[u] && td.motor[u])
            return 0xff;
    return 0;
}

/* entries: keep d1/a0/a1 (and d0 in init/finit) */
BLITZ_THUNK(ritrackdisklib_libinit, c_libinit, "d0-d1/a0-a1");
BLITZ_THUNK(ritrackdisklib_libfinit, c_libfinit, "d0-d1/a0-a1");
BLITZ_THUNK(opentrack, c_opentrack, "d1/a0-a1");
BLITZ_THUNK(opentrack2, c_opentrack2, "d1/a0-a1");
BLITZ_THUNK(motoron, c_motoron, "d1/a0-a1");
BLITZ_THUNK(closedisk, c_closedisk, "d1/a0-a1");
BLITZ_THUNK(readsector1, c_readsector1, "d1/a0-a1");
BLITZ_THUNK(readsector2, c_readsector2, "d1/a0-a1");
BLITZ_THUNK(motoroff, c_motoroff, "d1/a0-a1");
BLITZ_THUNK(writesector1, c_writesector1, "d1/a0-a1");
BLITZ_THUNK(writesector2, c_writesector2, "d1/a0-a1");
BLITZ_THUNK(formatsector1, c_formatsector1, "d1/a0-a1");
BLITZ_THUNK(formatsector2, c_formatsector2, "d1/a0-a1");
BLITZ_THUNK(install1, c_install1, "d1/a0-a1");
BLITZ_THUNK(install2, c_install2, "d1/a0-a1");
BLITZ_THUNK(iostatus, c_iostatus, "d1/a0-a1");
BLITZ_THUNK(ioreq, c_ioreq, "d1/a0-a1");
BLITZ_THUNK(bootprog, c_bootprog, "d1/a0-a1");
BLITZ_THUNK(diskmotor, c_diskmotor, "d1/a0-a1");
