/*
 * ricompactdisclib - RWE userlib 14 (#ricompactdisclib): audio CDs
 * through cd.device (or a compatible device) - OpenCD, CloseCD, CDDoor,
 * CDPlayTrack, CDReadTOC, CDStatus, CDStop, CDVolume, CDNumTracks,
 * CDFirstTrack, CDLastTrack, CDTrackLength, CDUpdateInfo, CDFlush,
 * CDPause, CDRewind, CDFastForward, CDNormalSpeed, CDSpeed, CDTrackMins,
 * CDTrackSecs, CDTrackPlaying, CDMotor.
 *
 * C translation of rwelibsrc/CDlib/cdSCSI.bb (1997-01-05, the newest of
 * the four CD sources, newer than every binary: docs/rwelibs/README.md
 * 3.2); the reference is the reference build of that source by blitz2
 * 2.10 (ref/ricompactdisclib.obj): the declarations equal it (ctest
 * rwe_declarations_ricompactdisclib), the behaviour is compared with it
 * in libs/tests/rwe/ricompactdisclib.c.  Notes:
 * docs/rwelibs/port/ricompactdisclib.md.
 *   init:  device name = "cd.device", toc = 0; CreateMsgPort (if that
 *          fails: flag 0, nothing else) and five buffers through memlib's
 *          globalloc (MEMF_CLEAR): the SCSICmd (30 bytes), the READ TOC
 *          command (10), a track descriptor (8, never used), the TOC data
 *          of the SCSI path (804) and a 12 byte SCSI command; scsiflag 0
 *   finit: without port nothing; else CloseCD, DeleteMsgPort, flag 0
 *          (the buffers stay for memlib)
 *
 * OpenCD [devicename$,unit[,scsiflag]]: one IOStdReq (CreateIORequest,
 * 200 bytes) on the port, OpenDevice(name, unit, 0); flag = -1 if open.
 * The commands check flag (0: return 0) and set io_Command, io_Data,
 * io_Offset, io_Length and io_Flags (0) of the request, then DoIO (the
 * commands return DoIO's d0, the statements leave it in d0 as well).
 * With scsiflag <> 0 CDPlayTrack, CDReadTOC and CDNumTracks use
 * HD_SCSICMD (PLAY AUDIO(12), READ TOC) instead.
 *
 * Everything is C: exec through BLITZ_BASEJSR where the original passes
 * the whole d0 of an exec call on (DoIO, SendIO, AbortIO, OpenDevice,
 * DeleteIORequest), memlib through BLITZ_ALIBJSR.  The data from
 * trackname to MyCDInfo keep the original's order and sizes: a long
 * device name and CDTrackMins/CDTrackSecs offsets read and write the same
 * things.
 *
 * BUG(orig):
 *   - OpenCD name$,unit copies the name into username (40 bytes) without
 *     a limit (a longer name overwrites toc, toclen, temptoc, qcodeinfo,
 *     ioreq ...) and keeps it, the unit and scsiflag for later OpenCDs;
 *     scsiflag is only cleared by init.
 *   - OpenCD while open: the old request stays open, lost.  If OpenDevice
 *     fails the request is deleted but stays in ioreq, and OpenCD returns
 *     d0 of DeleteIORequest (not 0).
 *   - CDStop (also in CloseCD) is AbortIO only: no WaitIO, and it sets
 *     io_Command of the request first.
 *   - CDPlayTrack with scsiflag ignores its arguments: it plays the
 *     blocks 14000 to 15000 (test values), sets io_Offset = 1, and writes
 *     the two longs to odd addresses (MOVE.l #14000,(a0)+ after a byte):
 *     an address error on a 68000.
 *   - CDReadTOC with scsiflag reads into its own buffer only: CDFirstTrack,
 *     CDLastTrack and CDTrackLength still use toc (0 before a CDReadTOC
 *     without scsiflag: they read address 0).
 *   - CDReadTOC: the summary read is not checked; the globalloc of the
 *     TOC is not checked (DoIO then writes to address 0).  toclen =
 *     (last track + 2) * 8 + 32 for entries of 6 bytes.
 *   - CDTrackLength n: start of track n+1 - start of track n in seconds
 *     (minutes * 60 + seconds); for track 1 the start of track 2 (the
 *     pregap counted).  From track 44 on the offset (n-1)*6 is > 255 and
 *     its high byte stays in the low word that MULU.w multiplies: the
 *     start of track n is wrong.  The last track takes the entry behind
 *     it (CD_TOCMSF does not write it: the lead-out is in the summary),
 *     track 0 reads 65539 bytes behind toc.  No check that toc exists.
 *   - CDUpdateInfo without an open device returns the caller's a0 + 5.
 *   - CDTrackMins/CDTrackSecs offset: ASL.b #2 shifts the low byte only,
 *     the index is the whole word (high byte kept), and the high bytes of
 *     d0 stay in the result; any offset reads qcodeinfo[5 + 4 * offset]
 *     (6 + ...) without a limit.  They and CDTrackPlaying do not check
 *     flag.
 * Registers: the original changes d0/d1/a0/a1, a6 and (CDReadTOC d7,
 * CDTrackLength d1/d2) others; the thunks keep d1/a0/a1, GCC keeps
 * d2-d7/a2-a6.
 */
#include "blitzlib.h"
#include "blitzexec.h"
#include <exec/io.h>
#include <exec/memory.h>
#include <devices/cd.h>
#include <devices/scsidisk.h>
#include <stddef.h>

/* entries (thunks below) */
void libinit(void), libfinit(void);
void opencd(void), opencd2(void), opencd3(void), closecd(void), eject(void);
void playtrack(void), gettoc(void), cdstatus(void), stoptrack(void), cdvolume(void);
void cdnumtracks(void), cdfirsttrack(void), cdlasttrack(void), cdtracklength(void);
void cdtracktime(void), cdflush(void), cdpause(void), cdrewind(void);
void cdfastforward(void), cdnormalspeed(void), cdchangespeed(void);
void cdtrackmins2(void), cdtrackmins(void), cdtracksecs2(void), cdtracksecs(void);
void cdtrackplaying(void), cdmotor(void);

BLITZ_ENTRY_DECL(finit);
BLITZ_ENTRY_DECL(init);

/* !libheader {#ricompactdisclib,init,0,finit,0} */
BLITZ_LIBHEADER(LIB_ricompactdisclib, init, BL_NORET, finit, 0);

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(opencd, 0, 0);
BLITZ_ARGS(BL_STRING, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(opencd2, 0, 0);
BLITZ_ARGS(BL_STRING, BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(opencd3, 0, 0);
BLITZ_NAME("OpenCD", "[devicename,unit,scsiflag] - open a CD compatible device");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(closecd, 0, 0);
BLITZ_NAME("CloseCD", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(eject), 0, 0);
BLITZ_NAME("CDDoor", "on/off");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(playtrack), 0, 0);
BLITZ_NAME("CDPlayTrack", "track#,numtracks");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(gettoc), 0, 0);
BLITZ_NAME("CDReadTOC", "");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(cdstatus), 0, 0);
BLITZ_NAME("CDStatus", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(stoptrack), 0, 0);
BLITZ_NAME("CDStop", "");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(cdvolume), 0, 0);
BLITZ_NAME("CDVolume", "volume,lengthoffade");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(cdnumtracks), 0, 0);
BLITZ_NAME("CDNumTracks", "");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(cdfirsttrack), 0, 0);
BLITZ_NAME("CDFirstTrack", "");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(cdlasttrack), 0, 0);
BLITZ_NAME("CDLastTrack", "");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(cdtracklength), 0, 0);
BLITZ_NAME("CDTrackLength", "track#");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(cdtracktime), 0, 0);
BLITZ_NAME("CDUpdateInfo", "- update track playing/time information");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(cdflush), 0, 0);
BLITZ_NAME("CDFlush", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(cdpause), 0, 0);
BLITZ_NAME("CDPause", "on/off");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(cdrewind), 0, 0);
BLITZ_NAME("CDRewind", "");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(cdfastforward), 0, 0);
BLITZ_NAME("CDFastForward", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(cdnormalspeed), 0, 0);
BLITZ_NAME("CDNormalSpeed", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(cdchangespeed), 0, 0);
BLITZ_NAME("CDSpeed", "speed");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(cdtrackmins2, 0, 0);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(cdtrackmins, 0, 0);
BLITZ_NAME("CDTrackMins", "[offset] - mins for current track");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(cdtracksecs2, 0, 0);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(cdtracksecs, 0, 0);
BLITZ_NAME("CDTrackSecs", "[offset] - secs for current track");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(cdtrackplaying, 0, 0);
BLITZ_NAME("CDTrackPlaying", "- current track being played");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(cdmotor), 0, 0);
BLITZ_NAME("CDMotor", "on|off");

BLITZ_NULLSUB_AT(finit, libfinit, 0, 0);
BLITZ_NULLSUB_AT(init, libinit, 0, 0);

BLITZ_LIBFIN();

#define LVO_OpenDevice      (-444)
#define LVO_DoIO            (-456)
#define LVO_SendIO          (-462)
#define LVO_AbortIO         (-480)
#define LVO_DeleteIORequest (-660)

#define SCSI_CMD_PLAYAUDIO12 0xa5
#define SCSI_CMD_READTOC     0x43
#define SIZEOF_READTOC       10         /* NEWTYPE .ReadTOC */
#define SIZEOF_TRACKDESC     8          /* NEWTYPE .TrackDesc */
#define SIZEOF_TOCDATA       804        /* NEWTYPE .TOCData: 4 + 100 * 8 */
#define SIZEOF_SCSICMD12     12         /* NEWTYPE .SCSICMD12 */

/* the buffers of init (globalloc) and scsiflag, in the code of the
   original */
static struct SCSICmd *Cmd;
static UBYTE *ReadTOC, *TrackDesc, *TOCData, *SCSICMD12;
static ULONG scsiflag;

/* the data at the end of the original, in its order */
static struct BL_PACKED {
    char *trackname;                    /* devicename or username */
    ULONG trackversion;                 /* the unit */
    char devicename[10];                /* "cd.device",0 */
    char username[40];                  /* Dcb.b 40,0 : Even */
    UBYTE *toc;
    ULONG toclen;
    UBYTE temptoc[12];
    UBYTE qcodeinfo[12];
    struct IOStdReq *ioreq;
    struct MsgPort *msgport;
    UBYTE flag;                         /* Even */
    UBYTE pad;
    UWORD mycdinfo[13];                 /* .MyCDInfo, Status = [12] */
} cd = { cd.devicename, 0, "cd.device" };
_Static_assert(offsetof(__typeof__(cd), toc) == 0x3a, "cd layout");
_Static_assert(offsetof(__typeof__(cd), ioreq) == 0x5a, "cd layout");
_Static_assert(offsetof(__typeof__(cd), mycdinfo) == 0x64, "cd layout");

static ULONG globalloc(ULONG size, ULONG flags)
{
    struct bl_regs r = { .d0 = size, .d1 = flags };

    return BLITZ_ALIBJSR(TOK_globalloc, r);
}

/* an exec call (constant LVO) with the request in a1: its whole d0 */
#define reqcall(lvo, req) ({                                                \
    struct bl_regs r_ = { .a1 = (ULONG)(req) };                             \
    BLITZ_BASEJSR(SysBase, lvo, r_); })

/* !DoIO / !SendIO {command,data,offset,length,flags} */
enum { DOIO, SENDIO };
static ULONG cdio(int how, UWORD cmd, APTR data, ULONG offset, ULONG length)
{
    struct IOStdReq *req = cd.ioreq;

    req->io_Command = cmd;
    req->io_Data = data;
    req->io_Offset = offset;
    req->io_Length = length;
    req->io_Flags = 0;
    if (how == SENDIO)
        return reqcall(LVO_SendIO, req);
    return reqcall(LVO_DoIO, req);
}

/* _DoSCSI: d0 command, d1 its length, d2 buffer, d3 its length, d4 flags */
static ULONG doscsi(UBYTE *command, UWORD cmdlen, APTR buf, ULONG len, UBYTE flags)
{
    struct IOStdReq *req = cd.ioreq;
    struct SCSICmd *c = Cmd;

    req->io_Data = Cmd;
    req->io_Command = HD_SCSICMD;
    req->io_Length = sizeof(struct SCSICmd);
    req->io_Flags = 0;
    c->scsi_Command = command;
    c->scsi_Flags = flags;
    c->scsi_Flags |= SCSIF_AUTOSENSE;
    c->scsi_CmdLength = cmdlen;
    c->scsi_Data = buf;
    c->scsi_Length = len;
    c->scsi_Actual = 0;
    c->scsi_CmdActual = 0;
    c->scsi_SenseActual = 0;
    return reqcall(LVO_DoIO, req);
}

void BLITZ_ROUTINE c_libinit(void)
{
    struct MsgPort *mp;

    cd.trackname = cd.devicename;
    cd.toc = NULL;
    cd.toclen = 0;
    mp = CreateMsgPort();
    if (!mp) {
        cd.flag = 0;
        return;
    }
    cd.msgport = mp;
    Cmd = (struct SCSICmd *)globalloc(sizeof(struct SCSICmd), MEMF_CLEAR);
    ReadTOC = (UBYTE *)globalloc(SIZEOF_READTOC, MEMF_CLEAR);
    TrackDesc = (UBYTE *)globalloc(SIZEOF_TRACKDESC, MEMF_CLEAR);
    TOCData = (UBYTE *)globalloc(SIZEOF_TOCDATA, MEMF_CLEAR);
    SCSICMD12 = (UBYTE *)globalloc(SIZEOF_SCSICMD12, MEMF_CLEAR);
    scsiflag = 0;
}

/* CDStop: AbortIO only (BUG(orig): no WaitIO) */
ULONG BLITZ_ROUTINE c_stoptrack(void)
{
    if (!cd.flag)
        return 0;
    cd.ioreq->io_Command = CD_PLAYTRACK;
    return reqcall(LVO_AbortIO, cd.ioreq);
}

/* CloseCD: -1 if it was open */
ULONG BLITZ_ROUTINE c_closecd(void)
{
    if (!cd.flag)
        return 0;
    c_stoptrack();                      /* ALibJsr CDStop */
    CloseDevice((struct IORequest *)cd.ioreq);
    DeleteIORequest((struct IORequest *)cd.ioreq);
    cd.ioreq = NULL;
    cd.flag = 0;
    return (ULONG)-1;
}

void BLITZ_ROUTINE c_libfinit(void)
{
    if (!cd.msgport)
        return;
    c_closecd();
    DeleteMsgPort(cd.msgport);
    cd.msgport = NULL;
    cd.flag = 0;
}

/* OpenCD: -1 if the device opened */
ULONG BLITZ_ROUTINE c_opencd(void)
{
    struct IOStdReq *req;
    struct bl_regs r;

    if (!cd.msgport)
        return 0;
    req = (struct IOStdReq *)CreateIORequest(cd.msgport, 200);
    if (!req) {
        cd.flag = 0;
        return 0;
    }
    cd.ioreq = req;                     /* BUG(orig): an open one is lost */
    r = (struct bl_regs){ .d0 = cd.trackversion, .d1 = 0, .a0 = (ULONG)cd.trackname,
                          .a1 = (ULONG)req };
    if (BLITZ_BASEJSR(SysBase, LVO_OpenDevice, r)) {
        /* BUG(orig): ioreq keeps the deleted request, the result is d0
           of DeleteIORequest */
        r = (struct bl_regs){ .a0 = (ULONG)cd.ioreq };
        r.d0 = BLITZ_BASEJSR(SysBase, LVO_DeleteIORequest, r);
        cd.flag = 0;
        return r.d0;
    }
    cd.flag = 0xff;
    return (ULONG)-1;
}

/* OpenCD devicename$,unit: the name and unit stay for later OpenCDs */
ULONG BLITZ_ROUTINE c_opencd2(const char *name BL_REG(d0), ULONG unit BL_REG(d1))
{
    char *p = cd.username;

    cd.trackname = p;
    while ((*p++ = *name++) != 0)       /* BUG(orig): no limit */
        ;
    cd.trackversion = unit;
    return c_opencd();
}

/* OpenCD devicename$,unit,scsiflag */
ULONG BLITZ_ROUTINE c_opencd3(const char *name BL_REG(d0), ULONG unit BL_REG(d1),
                              ULONG flag BL_REG(d2))
{
    scsiflag = flag;
    return c_opencd2(name, unit);
}

/* CDStatus: CD_INFO, the Status word */
ULONG BLITZ_ROUTINE c_cdstatus(void)
{
    if (!cd.flag)
        return 0;
    cdio(DOIO, CD_INFO, cd.mycdinfo, 0, 26);
    return BL_EXTL(cd.mycdinfo[12]);
}

/* CDDoor on/off: CD_EJECT */
ULONG BLITZ_ROUTINE c_eject(ULONG on BL_REG(d0))
{
    if (!cd.flag)
        return 0;
    return cdio(DOIO, CD_EJECT, NULL, 0, on);
}

/* CDPlayTrack track#,numtracks: CD_PLAYTRACK through SendIO */
ULONG BLITZ_ROUTINE c_playtrack(ULONG track BL_REG(d0), ULONG num BL_REG(d1))
{
    UBYTE *c;

    if (!cd.flag)
        return 0;
    if (!scsiflag)
        return cdio(SENDIO, CD_PLAYTRACK, NULL, track, num);
    /* _playtrackscsi - BUG(orig): the arguments are ignored */
    cd.ioreq->io_Offset = 1;
    c = SCSICMD12;
    c[0] = SCSI_CMD_PLAYAUDIO12;
    /* BUG(orig): MOVE.l to odd addresses - an address error on a 68000,
       here as well (volatile: GCC keeps the long writes) */
    *(volatile ULONG *)(c + 1) = 14000;
    *(volatile ULONG *)(c + 5) = 15000;
    *(volatile UWORD *)(c + 9) = 0;
    c[11] = 0;
    return doscsi(SCSICMD12, SIZEOF_SCSICMD12, NULL, 0, 0);
}

/* CDReadTOC without scsiflag: CD_TOCMSF into toc */
ULONG BLITZ_ROUTINE c_gettoc(void)
{
    struct bl_regs r;
    ULONG n;

    if (!cd.flag)
        return 0;
    if (scsiflag) {
        /* _gettocscsi: READ TOC into TOCData */
        UBYTE *c = ReadTOC;

        c[0] = SCSI_CMD_READTOC;
        c[1] = 0;
        *(volatile ULONG *)(c + 2) = 0;
        c[6] = 0;
        c[7] = SIZEOF_TOCDATA >> 8;
        c[8] = SIZEOF_TOCDATA & 0xff;
        c[9] = 0;
        return doscsi(ReadTOC, SIZEOF_READTOC, TOCData, SIZEOF_TOCDATA, SCSIF_READ);
    }
    if (cd.toc) {
        r = (struct bl_regs){ .d0 = cd.toclen, .a1 = (ULONG)cd.toc };
        BLITZ_ALIBJSR(TOK_globfree, r);
    }
    cdio(DOIO, CD_TOCMSF, cd.temptoc, 0, 1);    /* the summary */
    n = (ULONG)cd.temptoc[1] + 2;
    cd.toclen = bl_mulu_w((UWORD)n, 8) + 32;
    cd.toc = (UBYTE *)globalloc(cd.toclen, 0);      /* BUG(orig): unchecked */
    if (cdio(DOIO, CD_TOCMSF, cd.toc, 0, n))
        return 0;
    return (ULONG)-1;
}

/* CDVolume volume,lengthoffade: CD_ATTENUATE */
ULONG BLITZ_ROUTINE c_cdvolume(ULONG vol BL_REG(d0), ULONG fade BL_REG(d1))
{
    if (!cd.flag)
        return 0;
    return cdio(DOIO, CD_ATTENUATE, NULL, fade, vol);
}

/* CDNumTracks: last - first + 1 of toc (scsiflag: of TOCData) */
ULONG BLITZ_ROUTINE c_cdnumtracks(void)
{
    if (!cd.flag)
        return 0;
    if (scsiflag)
        return (UBYTE)(TOCData[3] - TOCData[2] + 1);
    return (UBYTE)(cd.toc[1] - cd.toc[0] + 1);
}

ULONG BLITZ_ROUTINE c_cdfirsttrack(void)
{
    if (!cd.flag)
        return 0;
    return cd.toc[0];
}

ULONG BLITZ_ROUTINE c_cdlasttrack(void)
{
    if (!cd.flag)
        return 0;
    return cd.toc[1];
}

/* CDTrackLength track#: in seconds, see BUG(orig) above */
ULONG BLITZ_ROUTINE c_cdtracklength(ULONG track BL_REG(d0))
{
    const UBYTE *p;
    ULONG off, start, end;

    if (!cd.flag)
        return 0;
    track--;
    if (!track)
        return (UWORD)(bl_mulu_w(cd.toc[15], 60) + cd.toc[16]);
    off = bl_mulu_w((UWORD)track, 6);   /* AND.l #$ffff: a word already */
    p = cd.toc + 9 + off;
    /* MOVE.b (a0),d0: the high byte of the offset stays in the word */
    start = (UWORD)(bl_mulu_w((UWORD)((off & 0xff00) | p[0]), 60) + p[1]);
    end = (UWORD)(bl_mulu_w(p[6], 60) + p[7]);
    return end - start;
}

/* CDUpdateInfo (doinfo: CD_QCODEMSF into qcodeinfo): the address of the track position's minutes;
   BUG(orig) without device the caller's a0 + 5 */
ULONG BLITZ_ROUTINE c_cdtracktime(ULONG a0 BL_REG(a0))
{
    if (!cd.flag)
        return a0 + 5;
    cdio(DOIO, CD_QCODEMSF, cd.qcodeinfo, 0, 0);
    return (ULONG)cd.qcodeinfo + 5;
}

/* CDTrackMins/CDTrackSecs: ASL.b #2,d0 : MOVE.b n(a0,d0.w),d0 */
static ULONG qbyte(ULONG d0, int n)
{
    ULONG i = (d0 & 0xffffff00UL) | (UBYTE)(d0 << 2);

    return (d0 & 0xffffff00UL) | ((const UBYTE *)cd.qcodeinfo)[n + (WORD)i];
}

ULONG BLITZ_ROUTINE c_cdtrackmins(ULONG d0 BL_REG(d0))
{
    return qbyte(d0, 5);
}

ULONG BLITZ_ROUTINE c_cdtrackmins2(void)
{
    return qbyte(0, 5);
}

ULONG BLITZ_ROUTINE c_cdtracksecs(ULONG d0 BL_REG(d0))
{
    return qbyte(d0, 6);
}

ULONG BLITZ_ROUTINE c_cdtracksecs2(void)
{
    return qbyte(0, 6);
}

/* CDTrackPlaying: the track of the last CDUpdateInfo */
ULONG BLITZ_ROUTINE c_cdtrackplaying(void)
{
    return cd.qcodeinfo[1];
}

/* CDFlush: CD_FLUSH */
ULONG BLITZ_ROUTINE c_cdflush(void)
{
    if (!cd.flag)
        return 0;
    return cdio(DOIO, CD_FLUSH, NULL, 0, 0);
}

/* CDPause on/off: CD_PAUSE with bit 0 */
ULONG BLITZ_ROUTINE c_cdpause(ULONG on BL_REG(d0))
{
    if (!cd.flag)
        return 0;
    return cdio(DOIO, CD_PAUSE, NULL, 0, on & 1);
}

/* CDMotor on|off: CD_MOTOR with bit 0 */
ULONG BLITZ_ROUTINE c_cdmotor(ULONG on BL_REG(d0))
{
    if (!cd.flag)
        return 0;
    return cdio(DOIO, CD_MOTOR, NULL, 0, on & 1);
}

/* cdchangespeed (CDSpeed speed): CD_SEARCH with the mode in io_Length */
ULONG BLITZ_ROUTINE c_cdchangespeed(ULONG mode BL_REG(d0))
{
    if (!cd.flag)
        return 0;
    return cdio(DOIO, CD_SEARCH, NULL, 0, mode);
}

ULONG BLITZ_ROUTINE c_cdrewind(void)
{
    return c_cdchangespeed(CDMODE_FREV);
}

ULONG BLITZ_ROUTINE c_cdfastforward(void)
{
    return c_cdchangespeed(CDMODE_FFWD);
}

ULONG BLITZ_ROUTINE c_cdnormalspeed(void)
{
    return c_cdchangespeed(CDMODE_NORMAL);
}

/* entries: keep d1/a0/a1 (and d0 in init/finit) */
BLITZ_THUNK(libinit, c_libinit, "d0-d1/a0-a1");
BLITZ_THUNK(libfinit, c_libfinit, "d0-d1/a0-a1");
BLITZ_THUNK(opencd, c_opencd, "d1/a0-a1");
BLITZ_THUNK(opencd2, c_opencd2, "d1/a0-a1");
BLITZ_THUNK(opencd3, c_opencd3, "d1/a0-a1");
BLITZ_THUNK(closecd, c_closecd, "d1/a0-a1");
BLITZ_THUNK(eject, c_eject, "d1/a0-a1");
BLITZ_THUNK(playtrack, c_playtrack, "d1/a0-a1");
BLITZ_THUNK(gettoc, c_gettoc, "d1/a0-a1");
BLITZ_THUNK(cdstatus, c_cdstatus, "d1/a0-a1");
BLITZ_THUNK(stoptrack, c_stoptrack, "d1/a0-a1");
BLITZ_THUNK(cdvolume, c_cdvolume, "d1/a0-a1");
BLITZ_THUNK(cdnumtracks, c_cdnumtracks, "d1/a0-a1");
BLITZ_THUNK(cdfirsttrack, c_cdfirsttrack, "d1/a0-a1");
BLITZ_THUNK(cdlasttrack, c_cdlasttrack, "d1/a0-a1");
BLITZ_THUNK(cdtracklength, c_cdtracklength, "d1/a0-a1");
BLITZ_THUNK(cdtracktime, c_cdtracktime, "d1/a0-a1");
BLITZ_THUNK(cdflush, c_cdflush, "d1/a0-a1");
BLITZ_THUNK(cdpause, c_cdpause, "d1/a0-a1");
BLITZ_THUNK(cdrewind, c_cdrewind, "d1/a0-a1");
BLITZ_THUNK(cdfastforward, c_cdfastforward, "d1/a0-a1");
BLITZ_THUNK(cdnormalspeed, c_cdnormalspeed, "d1/a0-a1");
BLITZ_THUNK(cdchangespeed, c_cdchangespeed, "d1/a0-a1");
BLITZ_THUNK(cdtrackmins2, c_cdtrackmins2, "d1/a0-a1");
BLITZ_THUNK(cdtrackmins, c_cdtrackmins, "d1/a0-a1");
BLITZ_THUNK(cdtracksecs2, c_cdtracksecs2, "d1/a0-a1");
BLITZ_THUNK(cdtracksecs, c_cdtracksecs, "d1/a0-a1");
BLITZ_THUNK(cdtrackplaying, c_cdtrackplaying, "d1/a0-a1");
BLITZ_THUNK(cdmotor, c_cdmotor, "d1/a0-a1");
