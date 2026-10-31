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
 *   init:  CreateMsgPort (msgport, 0 if that fails: then every command
 *          returns 0)
 *   finit: CloseDisk 0, 1, 2 and DeleteMsgPort
 *
 * Units 0-3: one IOStdReq (CreateIORequest, 200 bytes) per unit in
 * iorequests, all with the reply port msgport.  Sector commands: d1 =
 * first sector (FormatTrack: track, * 11), d3 = number of sectors
 * (default 1), length and offset = * 512; result: d0 of DoIO with
 * io_Error as its low byte (MOVE.b 31(a0),d0), 0 without port/request.
 * DiskStatus, DiskIO, BootProg and DiskMotor are Amiga mode only (odd
 * code address in !subs).
 *
 * Everything is C: exec through proto/exec.h (DoIO through BLITZ_BASEJSR,
 * the original returns all of DoIO's d0, the NDK inline only a BYTE), the
 * CIA through a volatile access.  The data (device name buffer, request
 * table, port, default boot block) keep the original's order and sizes,
 * so that a long device name overwrites the same things.
 *
 * BUG(orig):
 *   - OpenDisk unit,name$ copies the name into the buffer trackname (58
 *     bytes) without a limit - a longer name overwrites iorequests and
 *     msgport - (#860) and keeps it: a later OpenDisk without name uses
 *     it too (#859).
 *   - OpenDisk of a unit that is open replaces its request (the old one
 *     stays open, lost) (#862); OpenDisk with a negative unit passes the
 *     check (CMP #3: BGT), but putioreq/getioreq ignore it: the request
 *     is lost (OpenDevice ok) or DeleteIORequest gets 0 (OpenDevice
 *     failed) (#861).
 *   - finit closes units 0-2 only, not 3; init clears only the first
 *     three entries of iorequests (#863).
 *   - WriteBoot unit,bootdata sets d1 = 0 (sector) over bootdata and
 *     takes the buffer from d2, which is not an argument (whatever the
 *     caller left there; here a register parameter d2 for the same
 *     effect) (#864).  WriteBoot unit writes the 8 byte default boot
 *     block and the 1016 bytes behind it (the end of the library) (#865).
 *   - FormatTrack/WriteBoot (TD_FORMAT): length = number * 512 (as for
 *     sectors), not tracks * 11 * 512; WriteBoot formats 2 "sectors"
 *     (#858).
 *   - BootProg: the boot block checksum is the plain sum with carry of
 *     the 256 longs incl. the old checksum long (not cleared first, not
 *     complemented), and the root block long (+8) is cleared after it
 *     (#856).
 *   - DiskMotor: SNE on bit 7 of CIAB PRB ($BFD100), which is /MTR (active
 *     low): -1 (the byte $FF) means the motor line is high (off), the
 *     help text says "-1 if motor is on" (#857).
 *   - init calls CreateMsgPort, the commands CreateIORequest/
 *     DeleteIORequest, finit DeleteMsgPort - exec V36 - without a version
 *     check: on Kickstart 1.x every program using the library crashes at
 *     start-up (#941).
 * Registers: the original changes d0/d1/a0/a1, a6 and d4-d7 in most
 * commands, but keeps d1/a0/a1 on some paths (no port or request); the
 * thunks keep d1/a0/a1 everywhere, GCC keeps d2-d7/a2-a6.
 */
#include "blitzlib.h"
#include "blitzexec.h"
#include <exec/io.h>
#include <devices/trackdisk.h>
#include <devices/bootblock.h>
#include <stddef.h>

/* entries (thunks below) */
void libinit(void), libfinit(void);
void opentrack(void), opentrack2(void), motoron(void), closedisk(void);
void readsector1(void), readsector2(void), motoroff(void);
void writesector1(void), writesector2(void);
void formatsector1(void), formatsector2(void), install1(void), install2(void);
void iostatus(void), ioreq(void), bootprog(void), diskmotor(void);

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

BLITZ_NULLSUB_AT(finit, libfinit, 0, 0);
BLITZ_NULLSUB_AT(init, libinit, 0, 0);

BLITZ_LIBFIN();

#define LVO_DoIO (-456)
/* CIAB PRB: bit 7 = /MTR (custom.res ciabprb) */
#define CIABPRB (*(volatile UBYTE *)0xbfd100)

/* the data at the end of the original, in its order */
static struct BL_PACKED {
    char trackname[58];             /* "trackdisk.device",0 + Dcb.l 10,0, Even */
    struct IOStdReq *iorequests[4];
    struct MsgPort *msgport;
    UBYTE defaultboot[8];           /* "DOS",0 : Dc.l -1 */
} td = {
    "trackdisk.device", { 0 }, 0, { 'D', 'O', 'S', 0, 0xff, 0xff, 0xff, 0xff }
};
_Static_assert(offsetof(__typeof__(td), iorequests) == 58, "td layout");

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

/* DoIO and the result of the commands: DoIO's d0 with io_Error as the
   low byte (MOVE.b 31(a0),d0) */
static ULONG doio(struct IOStdReq *req)
{
    struct bl_regs r = { .a1 = (ULONG)req };
    ULONG result = BLITZ_BASEJSR(SysBase, LVO_DoIO, r);

    BL_SETB(result, req->io_Error);
    return result;
}

void BLITZ_ROUTINE c_libinit(void)
{
    td.iorequests[0] = NULL;            /* BUG(orig): not [3] (#863) */
    td.iorequests[1] = NULL;
    td.iorequests[2] = NULL;
    td.msgport = CreateMsgPort();       /* BUG(orig): V36, unchecked (#941) */
}

/* MotorOff unit# (TD_MOTOR, length 0) */
ULONG BLITZ_ROUTINE c_motoroff(LONG unit BL_REG(d0))
{
    struct IOStdReq *req;

    if (!td.msgport || !(req = getioreq(unit)))
        return 0;
    req->io_Command = TD_MOTOR;
    req->io_Flags = 0;
    req->io_Length = 0;
    return doio(req);
}

/* CloseDisk unit#: -1 if it was open */
BB_BOOL BLITZ_ROUTINE c_closedisk(LONG unit BL_REG(d0))
{
    struct IOStdReq *req;

    if (unit > 3 || !td.msgport || !(req = getioreq(unit)))
        return BB_FALSE;
    c_motoroff(unit);
    CloseDevice((struct IORequest *)req);
    DeleteIORequest((struct IORequest *)req);   /* V36 (#941) */
    putioreq(unit, NULL);
    return BB_TRUE;
}

void BLITZ_ROUTINE c_libfinit(void)
{
    c_closedisk(0);                     /* BUG(orig): not 3 (#863) */
    c_closedisk(1);
    c_closedisk(2);
    if (td.msgport) {
        DeleteMsgPort(td.msgport);      /* V36 (#941) */
        td.msgport = NULL;
    }
}

/* OpenDisk unit#: -1 if the device opened */
BB_BOOL BLITZ_ROUTINE c_opentrack(LONG unit BL_REG(d0))
{
    struct IOStdReq *req;

    if (unit > 3 || !td.msgport)
        return BB_FALSE;
    req = (struct IOStdReq *)CreateIORequest(td.msgport, 200); /* V36 (#941) */
    if (!req)
        return BB_FALSE;
    putioreq(unit, req);
    if (OpenDevice((CONST_STRPTR)td.trackname, unit, (struct IORequest *)req, 0)) {
        req = getioreq(unit);           /* 0 for a negative unit */
        putioreq(unit, NULL);
        DeleteIORequest((struct IORequest *)req);   /* V36 (#941) */
        return BB_FALSE;
    }
    return BB_TRUE;
}

/* OpenDisk unit#,devicename$: the name stays for later OpenDisks */
BB_BOOL BLITZ_ROUTINE c_opentrack2(LONG unit BL_REG(d0), const char *name BL_REG(d1))
{
    char *p = td.trackname;

    while ((*p++ = *name++) != 0)       /* BUG(orig): no limit (#860) */
        ;
    *p = 0;
    return c_opentrack(unit);
}

/* okok: the sector commands; d3 = number of sectors, d1 = first sector,
   both * 512 */
static ULONG sectorio(LONG unit, ULONG offset, APTR buffer, ULONG num, UWORD cmd)
{
    struct IOStdReq *req;

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
    return doio(req);
}

ULONG BLITZ_ROUTINE c_readsector2(LONG unit BL_REG(d0), ULONG sector BL_REG(d1),
                                  APTR buffer BL_REG(d2), ULONG num BL_REG(d3))
{
    return sectorio(unit, sector, buffer, num, CMD_READ);
}

ULONG BLITZ_ROUTINE c_readsector1(LONG unit BL_REG(d0), ULONG sector BL_REG(d1),
                                  APTR buffer BL_REG(d2))
{
    return sectorio(unit, sector, buffer, 1, CMD_READ);
}

ULONG BLITZ_ROUTINE c_writesector2(LONG unit BL_REG(d0), ULONG sector BL_REG(d1),
                                   APTR buffer BL_REG(d2), ULONG num BL_REG(d3))
{
    return sectorio(unit, sector, buffer, num, CMD_WRITE);
}

ULONG BLITZ_ROUTINE c_writesector1(LONG unit BL_REG(d0), ULONG sector BL_REG(d1),
                                   APTR buffer BL_REG(d2))
{
    return sectorio(unit, sector, buffer, 1, CMD_WRITE);
}

/* FormatTrack: track * 11 (MULU.w: the low word of the track) */
ULONG BLITZ_ROUTINE c_formatsector2(LONG unit BL_REG(d0), ULONG track BL_REG(d1),
                                    APTR buffer BL_REG(d2), ULONG num BL_REG(d3))
{
    return sectorio(unit, bl_mulu_w((UWORD)track, 11), buffer, num, TD_FORMAT);
}

ULONG BLITZ_ROUTINE c_formatsector1(LONG unit BL_REG(d0), ULONG track BL_REG(d1),
                                    APTR buffer BL_REG(d2))
{
    return c_formatsector2(unit, track, buffer, 1);
}

/* WriteBoot unit#: the default boot block (and what follows it) */
ULONG BLITZ_ROUTINE c_install1(LONG unit BL_REG(d0))
{
    return c_formatsector2(unit, 0, td.defaultboot, 2);
}

/* WriteBoot unit#,bootdata: BUG(orig) the buffer is the caller's d2 (#864) */
ULONG BLITZ_ROUTINE c_install2(LONG unit BL_REG(d0), APTR buffer BL_REG(d2))
{
    return c_formatsector2(unit, 0, buffer, 2);
}

/* BootProg unit#,memoryaddress: "DOS",0, checksum, write sectors 0-1 */
ULONG BLITZ_ROUTINE c_bootprog(LONG unit BL_REG(d0), UBYTE *mem BL_REG(d1))
{
    struct BootBlock *bb = (struct BootBlock *)mem;
    ULONG *p = (ULONG *)mem, sum = 0;
    int i;

    if (!td.msgport)
        return 0;
    mem[0] = 'D';                       /* bb_id, byte by byte (MOVE.b; */
    mem[1] = 'O';                       /* as bb->bb_id GCC would merge */
    mem[2] = 'S';                       /* them into words) */
    mem[3] = 0;
    for (i = 0; i < 256; i++) {         /* ADD.l (a0)+,d0:BCC:ADDQ.l #1 */
        ULONG v = p[i];
        sum += v;
        if (sum < v)
            sum++;
    }
    bb->bb_chksum = sum;                /* BUG(orig): not complemented (#856) */
    bb->bb_dosblock = 0;                /* BUG(orig): after the sum (#856) */
    return c_writesector2(unit, 0, mem, 2);
}

/* DiskStatus unit#: CheckIO */
ULONG BLITZ_ROUTINE c_iostatus(LONG unit BL_REG(d0))
{
    struct IOStdReq *req;

    if (!td.msgport || !(req = getioreq(unit)))
        return 0;
    return (ULONG)CheckIO((struct IORequest *)req);
}

/* DiskIO unit#: the IOStdReq */
ULONG BLITZ_ROUTINE c_ioreq(LONG unit BL_REG(d0))
{
    if (!td.msgport)
        return 0;
    return (ULONG)getioreq(unit);
}

/* MotorOn unit# (TD_MOTOR, length 1) */
ULONG BLITZ_ROUTINE c_motoron(LONG unit BL_REG(d0))
{
    struct IOStdReq *req;

    if (!td.msgport || !(req = getioreq(unit)))
        return 0;
    req->io_Message.mn_ReplyPort = td.msgport;
    req->io_Command = TD_MOTOR;
    req->io_Length = 1;
    req->io_Flags = 0;
    return doio(req);
}

/* DiskMotor: MOVEQ #0,d0:BTST #7,$bfd100:SNE d0 */
ULONG BLITZ_ROUTINE c_diskmotor(void)
{
    return (CIABPRB & 0x80) ? 0xff : 0;
}

/* entries: keep d1/a0/a1 (and d0 in init/finit) */
BLITZ_THUNK(libinit, c_libinit, "d0-d1/a0-a1");
BLITZ_THUNK(libfinit, c_libfinit, "d0-d1/a0-a1");
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
