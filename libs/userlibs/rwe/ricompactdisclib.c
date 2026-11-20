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
 *   init:  device name = "cd.device", toc = 0; a reply port (newport; if
 *          that fails: flag 0, nothing else) and five buffers through
 *          memlib's globalloc (MEMF_CLEAR): the SCSICmd (30 bytes), the
 *          READ TOC command (10), a track descriptor (8, never used), the
 *          TOC data of the SCSI path (804) and a 12 byte SCSI command;
 *          scsiflag 0
 *   finit: without port nothing; else CloseCD, freeport, flag 0 (the
 *          buffers stay for memlib)
 *
 * OpenCD [devicename$,unit[,scsiflag]]: one IOStdReq (newreq, 200 bytes)
 * on the port, OpenDevice(name, unit, 0); flag = -1 if open.
 * The commands check flag (0: return 0) and set io_Command, io_Data,
 * io_Offset, io_Length and io_Flags (0) of the request, then DoIO (the
 * commands return DoIO's d0, the statements leave it in d0 as well).
 * With scsiflag <> 0 CDPlayTrack, CDReadTOC and CDNumTracks use
 * HD_SCSICMD (PLAY AUDIO(12), READ TOC) instead.
 *
 * Everything is C: exec through BLITZ_BASEJSR where the original passes
 * the whole d0 of an exec call on (DoIO, SendIO, AbortIO, OpenDevice),
 * memlib through BLITZ_ALIBJSR.  The data from
 * trackname to MyCDInfo keep the original's order and sizes: a long
 * device name and CDTrackMins/CDTrackSecs offsets read and write the same
 * things.
 *
 * Fixed (#576): OpenCD name$,unit refuses a name of 40 characters or
 *   more (0, nothing copied); the original copies it into username (40
 *   bytes) without a limit (over toc, toclen, temptoc, qcodeinfo, ioreq
 *   ...).
 * Fixed (#579): CDPlayTrack with scsiflag writes its command block byte
 *   by byte; the original writes two longs and a word to odd addresses
 *   (MOVE.l #14000,(a0)+ after a byte): an address error on a 68000.
 * Fixed (#582): CDNumTracks, CDFirstTrack, CDLastTrack and CDTrackLength
 *   return 0 without a TOC; with scsiflag they use the SCSI TOC of
 *   CDReadTOC (first and last track, the lengths from the LBAs; 0 for a
 *   track it does not hold).  The original reads toc without a test
 *   (address 0 before a CDReadTOC) and with scsiflag counts only
 *   CDNumTracks from the SCSI TOC.
 * Fixed (#581): CDReadTOC checks the summary read and the globalloc of
 *   the TOC (0, no TOC) and leaves no TOC behind a failed read; the
 *   original reads the entries with an earlier summary, and keeps the
 *   uncleared buffer of a failed read.  toclen = (last track + 2) * 8 +
 *   32 for entries of 6 bytes, as in the original.
 * Fixed (#1138): the reply port and the request are built by hand with
 *   exec V33 functions (newport, newreq), one path for all Kickstarts;
 *   the original calls CreateMsgPort in init (exec V36, no version
 *   check) and crashes on Kickstart 1.x before the program's first line.
 * Fixed (#575): OpenCD returns 0 when OpenDevice fails and forgets the
 *   freed request; the original keeps the deleted request in ioreq and
 *   returns the d0 of DeleteIORequest (200 on Kickstart 3.1).
 * Fixed (#577): OpenCD name$,unit clears scsiflag, OpenCD without
 *   arguments opens cd.device unit 0 without scsiflag (the manual); the
 *   original keeps the name, the unit and scsiflag of the OpenCD before
 *   (only init clears scsiflag and sets the name back, not the unit).
 * Fixed (#584): CDTrackLength 1 is the start of track 2 minus the start
 *   of track 1, as for every other track; the original returns the start
 *   of track 2 (the 2 seconds of the pregap counted).
 * Fixed (#585): CDTrackLength takes the start's minutes alone; from track
 *   44 on the entry offset (n-1)*6 is > 255, and the original's MOVE.b
 *   (a0),d0 leaves its high byte in the word MULU.w multiplies: the start
 *   of the track was wrong.
 * Fixed (#583): CDTrackLength of the last track ends at the lead-out of
 *   the summary; the original takes the entry behind it, which CD_TOCMSF
 *   does not write.  With runtime error checking a track outside the
 *   TOC's first to last track is refused ("Track number out of range");
 *   in the release code such a track still reads entries that do not
 *   exist (track 0: 65539 bytes behind toc), as in the original (D-005).
 * Fixed (#586): with runtime error checking CDTrackMins offset and
 *   CDTrackSecs offset refuse an offset other than 0 or 1 ("Offset must
 *   be 0 or 1"; the manual allows those two).  The release code is the
 *   original's: ASL.b #2 shifts the low byte only, the index is the whole
 *   word (high byte kept), the high bytes of d0 stay in the result, and
 *   another offset reads qcodeinfo[5 + 4 * offset] (6 + ...) without a
 *   limit (D-005).  They and CDTrackPlaying read the Q-code without an
 *   open device: the data of the last CDUpdateInfo (0 before one).
 * Fixed (#578): with runtime error checking OpenCD (all three forms)
 *   refuses to open while a device is open ("CD device already open:
 *   CloseCD first"; the manual asks for CloseCD between them).  The
 *   release code is the original's: the open request is lost and stays
 *   open (D-005).
 * Fixed (#587): CDUpdateInfo returns 0 without an open device; the
 *   original returns the caller's a0 + 5.
 *
 * Fixed (#982): CDVolume volume,fade puts the volume into io_Offset and
 *   the fade into io_Length, as cd.device's CD_ATTENUATE wants them; the
 *   original swaps them.
 * Fixed (#948): CDStatus asks for a whole struct CDInfo (34 bytes) into
 *   MyCDInfo of that size; the original asks for 26 bytes into 13 words,
 *   and a device that writes the whole struct (uaescsi.device) writes 8
 *   bytes behind MyCDInfo, the end of the library.  (The port had 8 bytes
 *   of room there since #946, a DIFF; it is the struct's end now.)
 * Fixed (#580): CDPlayTrack track#,numtracks with scsiflag sends PLAY
 *   AUDIO(12) from the SCSI TOC of CDReadTOC (the LBA of track# and the
 *   length to the start of the track behind the last one played, the
 *   lead-out at the most) and returns -1 when it worked, else 0 (also
 *   without a TOC, for a track the TOC does not hold, for 0 tracks); the
 *   original ignores its arguments, plays the author's test values
 *   (blocks 14000 to 15000) and returns DoIO's error code.
 * Fixed (#1156, #574): CDPlayTrack sends CD_PLAYTRACK with a request of
 *   its own (playreq, built after OpenDevice from the device and unit of
 *   ioreq); CDStop, CloseCD and the next CDPlayTrack abort a play that is
 *   out and take it back with WaitIO (stopplay); io_Command is not
 *   written.  The original sends the library's one request with SendIO:
 *   every other command fills it and sends it again while the play is
 *   out, CDStop writes CD_PLAYTRACK into it and calls AbortIO without
 *   WaitIO, and the reply stays at the port (the next play links the
 *   same message again).
 * Fixed (#1157): CDPlayTrack without scsiflag returns -1 when the play
 *   was sent (0 without a device); the original returns the d0 SendIO
 *   leaves (SendIO has no result).
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
void ricompactdisclib_libinit(void), ricompactdisclib_libfinit(void);
void opencd(void), opencd2(void), opencd3(void), closecd(void), eject(void);
void playtrack(void), gettoc(void), cdstatus(void), stoptrack(void);
void cdvolume(void), cdnumtracks(void), cdfirsttrack(void), cdlasttrack(void);
void cdtracklength(void), cdtracktime(void), cdflush(void), cdpause(void);
void cdrewind(void), cdfastforward(void), cdnormalspeed(void);
void cdchangespeed(void), cdtrackmins2(void), cdtrackmins(void);
void cdtracksecs2(void), cdtracksecs(void), cdtrackplaying(void);
void cdmotor(void);
void _cdtracklengthchk(void), _offsetchk(void), _opencdchk(void);

BLITZ_ENTRY_DECL(finit);
BLITZ_ENTRY_DECL(init);

/* !libheader {#ricompactdisclib,init,0,finit,0} */
/* error field: the error check area (fixed #583; the original has none) */
BLITZ_LIBHEADER(LIB_ricompactdisclib, init, BL_NORET, finit, BLITZ_ERROR_AREA);

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(opencd, _opencdchk, 0);                      /* check: fixed #578 */
BLITZ_ARGS(BL_STRING, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(opencd2, _opencdchk, 0);                     /* check: fixed #578 */
BLITZ_ARGS(BL_STRING, BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(opencd3, _opencdchk, 0);                     /* check: fixed #578 */
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
BLITZ_SUBS(BLITZ_AMIGAONLY(cdtracklength), _cdtracklengthchk, 0);  /* check: fixed #583 */
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
BLITZ_SUBS(cdtrackmins, _offsetchk, 0);                 /* check: fixed #586 */
BLITZ_NAME("CDTrackMins", "[offset] - mins for current track");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(cdtracksecs2, 0, 0);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(cdtracksecs, _offsetchk, 0);                 /* check: fixed #586 */
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

BLITZ_NULLSUB_AT(finit, ricompactdisclib_libfinit, 0, 0);
BLITZ_NULLSUB_AT(init, ricompactdisclib_libinit, 0, 0);

BLITZ_LIBFIN();

#define LVO_OpenDevice      (-444)
#define LVO_DoIO            (-456)
#define LVO_SendIO          (-462)

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
static UBYTE scsitoc;                   /* TOCData read (fixed #582) */
/* the request of CDPlayTrack's CD_PLAYTRACK (SendIO) and whether it is
   out, not yet taken back with WaitIO (fixed #1156, #574) */
static struct IOStdReq *playreq;
static UBYTE playing;

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
    /* .MyCDInfo, Status = [12].  Fixed (#948): a whole struct CDInfo (17
       words), asked for with its size; the original has 13 words and asks
       for 26 bytes, and a device that writes the whole struct
       (uaescsi.device does) writes 8 bytes behind the library */
    UWORD mycdinfo[17];
} cd = { cd.devicename, 0, "cd.device" };
_Static_assert(offsetof(__typeof__(cd), toc) == 0x3a, "cd layout");
_Static_assert(offsetof(__typeof__(cd), ioreq) == 0x5a, "cd layout");
_Static_assert(offsetof(__typeof__(cd), mycdinfo) == 0x64, "cd layout");
_Static_assert(sizeof cd.mycdinfo == sizeof(struct CDInfo), "CD_INFO size");
_Static_assert(offsetof(struct CDInfo, Status) == 24, "CDInfo Status");

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

/* a long big-endian at any address (a SCSI command block) */
static void putlong(UBYTE *p, ULONG v)
{
    p[0] = (UBYTE)(v >> 24);
    p[1] = (UBYTE)(v >> 16);
    p[2] = (UBYTE)(v >> 8);
    p[3] = (UBYTE)v;
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

/* Fixed (#1138): the reply port and the request are built by hand with
   exec V33 functions (AllocMem, FreeMem, AllocSignal, FreeSignal,
   FindTask), one path for all Kickstarts, as ritrackdisklib's (#941);
   the original calls CreateMsgPort, CreateIORequest, DeleteIORequest and
   DeleteMsgPort (V36) without a version check and crashes in init on
   Kickstart 1.x.  The helpers are those of blitzexec.h; the wrappers
   below keep them out of line (one copy each). */

/* newport: the reply port (bl_newport: a signal port of the calling
   task, as CreateMsgPort); NULL without memory or a free signal */
static struct MsgPort *newport(void)
{
    return bl_newport();
}

/* freeport: frees the port of newport() */
static void freeport(struct MsgPort *mp)
{
    bl_freeport(mp);
}

#define REQSIZE 200                     /* the original's CreateIORequest size */

/* newreq: a cleared request of REQSIZE bytes on the library's port with
   mn_ReplyPort and mn_Length set (as CreateIORequest) and NT_REPLYMSG
   (CheckIO takes a request that was never sent for done); NULL without
   memory.  Not blitzexec.h's bl_newreq: that one reads the port before
   AllocMem, this one after it (other code, the library would change) */
static struct IOStdReq *newreq(void)
{
    struct IOStdReq *req = AllocMem(REQSIZE, MEMF_PUBLIC | MEMF_CLEAR);

    if (req) {
        req->io_Message.mn_Node.ln_Type = NT_REPLYMSG;
        req->io_Message.mn_ReplyPort = cd.msgport;
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

static void BLITZ_ROUTINE c_libinit(void)
{
    struct MsgPort *mp;

    cd.trackname = cd.devicename;
    cd.toc = NULL;
    cd.toclen = 0;
    mp = newport();                     /* fixed #1138: built by hand */
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
    scsitoc = 0;
    playreq = NULL;
    playing = 0;
}

/* stopplay: a play that is out (playing) is aborted and taken back
   (AbortIO, WaitIO: also one the device has replied by itself) */
static void stopplay(void)
{
    if (!playing)
        return;
    AbortIO((struct IORequest *)playreq);
    WaitIO((struct IORequest *)playreq);
    playing = 0;
}

/* CDStop.  Fixed (#574, #1156): the play request is aborted and taken
   back with WaitIO, only when a play is out, and its io_Command is not
   written; the original writes CD_PLAYTRACK into io_Command of the one
   request and calls AbortIO without WaitIO (the reply stays at the port)
   and returns AbortIO's d0 */
ULONG BLITZ_ROUTINE c_stoptrack(void)
{
    if (!cd.flag)
        return 0;
    stopplay();
    return 0;
}

/* CloseCD: -1 if it was open */
ULONG BLITZ_ROUTINE c_closecd(void)
{
    if (!cd.flag)
        return 0;
    c_stoptrack();                      /* ALibJsr CDStop */
    CloseDevice((struct IORequest *)cd.ioreq);
    freereq(cd.ioreq);
    freereq(playreq);
    cd.ioreq = NULL;
    playreq = NULL;
    cd.flag = 0;
    return (ULONG)-1;
}

static void BLITZ_ROUTINE c_libfinit(void)
{
    if (!cd.msgport)
        return;
    c_closecd();
    freeport(cd.msgport);
    cd.msgport = NULL;
    cd.flag = 0;
}

/* the open of the three forms: -1 if the device opened */
static ULONG doopen(void)
{
    struct IOStdReq *req, *preq;
    struct bl_regs r;

    if (!cd.msgport)
        return 0;
    /* OpenCD while open (#578, refused only by the error check): the open
       request stays lost as in the original, but the play request of
       #1156 is the library's own: its play stopped, it is freed */
    if (playreq) {
        stopplay();
        freereq(playreq);
        playreq = NULL;
    }
    req = newreq();                     /* fixed #1138: built by hand */
    if (!req) {
        cd.flag = 0;
        return 0;
    }
    /* fixed #1156: a request of its own for CD_PLAYTRACK */
    preq = newreq();
    if (!preq) {
        freereq(req);
        cd.flag = 0;
        return 0;
    }
    cd.ioreq = req;                     /* an open one is lost (#578: refused by opencdchk) */
    playreq = preq;
    playing = 0;
    r = (struct bl_regs){ .d0 = cd.trackversion, .d1 = 0, .a0 = (ULONG)cd.trackname,
                          .a1 = (ULONG)req };
    if (BLITZ_BASEJSR(SysBase, LVO_OpenDevice, r)) {
        /* Fixed (#575): the request freed and forgotten, 0; the original
           keeps the deleted request in ioreq and returns the d0 of
           DeleteIORequest (200 on Kickstart 3.1) */
        freereq(req);
        freereq(preq);
        cd.ioreq = NULL;
        playreq = NULL;
        cd.flag = 0;
        return 0;
    }
    preq->io_Device = req->io_Device;
    preq->io_Unit = req->io_Unit;
    cd.flag = 0xff;
    return (ULONG)-1;
}

/* a device name that fits username with its 0 (the manual: "less than
   forty characters") */
static int namefits(const char *name)
{
    int n = 0;

    while (name[n])
        if (++n >= (int)sizeof cd.username)
            return 0;
    return 1;
}

/* OpenCD: cd.device, unit 0 (the manual: "ommiting the optional
   parameter of this command will cause the default cd.device unit 0 to
   be opened").  Fixed (#577): the name, the unit and scsiflag are set
   back; the original opens the name and unit of the OpenCD before, with
   its scsiflag */
ULONG BLITZ_ROUTINE c_opencd(void)
{
    cd.trackname = cd.devicename;
    cd.trackversion = 0;
    scsiflag = 0;
    return doopen();
}

/* OpenCD devicename$,unit[,scsiflag]: the name, the unit and scsiflag.
   Fixed (#576): a name of 40 characters or more is refused (0, nothing
   copied, nothing opened, scsiflag kept); the original copies it without
   a limit over toc, toclen, temptoc, qcodeinfo, ioreq, msgport ... */
static ULONG openname(const char *name, ULONG unit, ULONG flag)
{
    char *p = cd.username;

    if (!namefits(name))
        return 0;
    cd.trackname = p;
    while ((*p++ = *name++) != 0)
        ;
    cd.trackversion = unit;
    scsiflag = flag;
    return doopen();
}

/* OpenCD devicename$,unit.  Fixed (#577): scsiflag 0; the original keeps
   the scsiflag of an earlier OpenCD devicename$,unit,scsiflag */
ULONG BLITZ_ROUTINE c_opencd2(const char *name BL_REG(d0), ULONG unit BL_REG(d1))
{
    return openname(name, unit, 0);
}

/* OpenCD devicename$,unit,scsiflag */
ULONG BLITZ_ROUTINE c_opencd3(const char *name BL_REG(d0), ULONG unit BL_REG(d1),
                              ULONG flag BL_REG(d2))
{
    return openname(name, unit, flag);
}

/* CDStatus: CD_INFO, the Status word */
ULONG BLITZ_ROUTINE c_cdstatus(void)
{
    if (!cd.flag)
        return 0;
    cdio(DOIO, CD_INFO, cd.mycdinfo, 0, sizeof cd.mycdinfo);  /* fixed #948: 34, not 26 */
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
static const UBYTE *scsidesc(ULONG t);
static ULONG getlong(const UBYTE *p);

ULONG BLITZ_ROUTINE c_playtrack(ULONG track BL_REG(d0), ULONG num BL_REG(d1))
{
    UBYTE *c;

    if (!cd.flag)
        return 0;
    if (!scsiflag) {
        /* Fixed (#1156): CD_PLAYTRACK goes out with a request of its own
           (a play that is out first stopped and taken back), so that the
           other commands do not reuse a request in flight, and its reply
           is taken off the port; the original sends the library's one
           request, which the next command fills and sends again while the
           play is out, and leaves its reply at the port */
        struct IOStdReq *req = playreq;

        stopplay();
        req->io_Command = CD_PLAYTRACK;
        req->io_Data = NULL;
        req->io_Offset = track;
        req->io_Length = num;
        req->io_Flags = 0;
        SendIO((struct IORequest *)req);
        playing = 1;
        /* Fixed (#1157): -1, the play was sent (the manual: "A return
           value of true means that the command succeeded"); the original
           returns the d0 SendIO leaves, which has no result */
        return (ULONG)-1;
    }
    /* _playtrackscsi.  Fixed (#580): PLAY AUDIO(12) from the SCSI TOC of
       CDReadTOC: the LBA of track# and the length to the start of the
       track behind the last one played (the lead-out at the most); 0
       without a TOC, for a track it does not hold or for 0 tracks; -1
       when DoIO worked, else 0.  The original ignores its arguments and
       sends the author's test values (blocks 14000 to 15000), and returns
       DoIO's error code.  io_Offset = 1 as in the original (HD_SCSICMD
       does not look at it). */
    {
        const UBYTE *d, *e;
        ULONG lastt, endt, start;

        if (!scsitoc || !num)
            return 0;
        lastt = TOCData[3];
        if (track < TOCData[2] || track > lastt)
            return 0;
        endt = num > lastt + 1 - track ? lastt + 1 : track + num;
        d = scsidesc(track);
        e = scsidesc(endt);
        if (!d || !e)
            return 0;
        start = getlong(d + 4);
        cd.ioreq->io_Offset = 1;
        c = SCSICMD12;
        /* Fixed (#579): the bytes one by one; the original writes two
           longs and a word with MOVE to the odd addresses 1, 5 and 9 (an
           address error on a 68000) */
        c[0] = SCSI_CMD_PLAYAUDIO12;
        c[1] = 0;
        putlong(c + 2, start);
        putlong(c + 6, getlong(e + 4) - start);
        c[10] = 0;
        c[11] = 0;
        return doscsi(SCSICMD12, SIZEOF_SCSICMD12, NULL, 0, 0) ? 0 : (ULONG)-1;
    }
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
        r.d0 = doscsi(ReadTOC, SIZEOF_READTOC, TOCData, SIZEOF_TOCDATA, SCSIF_READ);
        scsitoc = !r.d0;                /* fixed #582: the TOC functions use it */
        return r.d0;
    }
    /* Fixed (#581): the old TOC is gone before the reads (toc 0), a
       failing summary read or globalloc returns 0 without a TOC, and a
       failing read of the entries frees the new one; the original reads
       the entries with the summary of an earlier CDReadTOC and keeps the
       buffer of a failed read */
    if (cd.toc) {
        r = (struct bl_regs){ .d0 = cd.toclen, .a1 = (ULONG)cd.toc };
        BLITZ_ALIBJSR(TOK_globfree, r);
        cd.toc = NULL;
    }
    if (cdio(DOIO, CD_TOCMSF, cd.temptoc, 0, 1))    /* the summary */
        return 0;
    n = (ULONG)cd.temptoc[1] + 2;
    cd.toclen = bl_mulu_w((UWORD)n, 8) + 32;
    cd.toc = (UBYTE *)globalloc(cd.toclen, 0);
    if (!cd.toc)
        return 0;
    if (cdio(DOIO, CD_TOCMSF, cd.toc, 0, n)) {
        r = (struct bl_regs){ .d0 = cd.toclen, .a1 = (ULONG)cd.toc };
        BLITZ_ALIBJSR(TOK_globfree, r);
        cd.toc = NULL;
        return 0;
    }
    return (ULONG)-1;
}

/* CDVolume volume,lengthoffade: CD_ATTENUATE.
   Fixed (#982): io_Offset = volume, io_Length = fade in frames, as
   cd.device wants them; the original swaps the two fields */
ULONG BLITZ_ROUTINE c_cdvolume(ULONG vol BL_REG(d0), ULONG fade BL_REG(d1))
{
    if (!cd.flag)
        return 0;
    return cdio(DOIO, CD_ATTENUATE, NULL, vol, fade);
}

/* the SCSI TOC (READ TOC of a CDReadTOC with scsiflag, fixed #582): the
   descriptor of track t (t = last + 1: the lead-out, the descriptor
   behind the last track), NULL without a TOC or for a track it does not
   hold */
static const UBYTE *scsidesc(ULONG t)
{
    ULONG len, n, i;

    if (!scsitoc)
        return NULL;
    len = ((ULONG)TOCData[0] << 8) | TOCData[1];
    n = len < 2 ? 0 : (len - 2) / 8;
    if (n > (SIZEOF_TOCDATA - 4) / 8)
        n = (SIZEOF_TOCDATA - 4) / 8;
    if (t < TOCData[2] || t > (ULONG)TOCData[3] + 1)
        return NULL;
    i = t - TOCData[2];
    if (i >= n)
        return NULL;
    return TOCData + 4 + 8 * i;
}

static ULONG getlong(const UBYTE *p)
{
    return ((ULONG)p[0] << 24) | ((ULONG)p[1] << 16) | ((ULONG)p[2] << 8) | p[3];
}

/* CDNumTracks: last - first + 1 of toc (scsiflag: of TOCData).  Fixed
   (#582): 0 without a TOC (the original reads address 0, or the cleared
   TOCData: 1) */
ULONG BLITZ_ROUTINE c_cdnumtracks(void)
{
    if (!cd.flag)
        return 0;
    if (scsiflag)
        return scsitoc ? (UBYTE)(TOCData[3] - TOCData[2] + 1) : 0;
    if (!cd.toc)
        return 0;
    return (UBYTE)(cd.toc[1] - cd.toc[0] + 1);
}

/* CDFirstTrack, CDLastTrack.  Fixed (#582): 0 without a TOC; with
   scsiflag from TOCData (the original reads toc, address 0 before a
   CDReadTOC without scsiflag) */
ULONG BLITZ_ROUTINE c_cdfirsttrack(void)
{
    if (!cd.flag)
        return 0;
    if (scsiflag)
        return scsitoc ? TOCData[2] : 0;
    return cd.toc ? cd.toc[0] : 0;
}

ULONG BLITZ_ROUTINE c_cdlasttrack(void)
{
    if (!cd.flag)
        return 0;
    if (scsiflag)
        return scsitoc ? TOCData[3] : 0;
    return cd.toc ? cd.toc[1] : 0;
}

/* CDTrackLength track#: in seconds, start of track n+1 - start of track n
   (minutes * 60 + seconds; fixed #583-#585, see above).
   Fixed (#582): 0 without a TOC; with scsiflag from the LBAs of TOCData
   (the start of the track to the start of the next one or of the
   lead-out, in frames / 75), 0 for a track it does not hold */
ULONG BLITZ_ROUTINE c_cdtracklength(ULONG track BL_REG(d0))
{
    const UBYTE *p;
    ULONG off, start, end;

    if (!cd.flag)
        return 0;
    if (scsiflag) {
        const UBYTE *d = track <= 99 ? scsidesc(track) : NULL;
        const UBYTE *e = d && track <= TOCData[3] ? scsidesc(track + 1) : NULL;

        if (!e)
            return 0;
        off = getlong(e + 4) - getlong(d + 4);
        if (off >= 75UL * 65536)        /* a broken TOC: no 16-bit quotient */
            return 0;
        return (UWORD)bl_divu_w(off, 75);
    }
    if (!cd.toc)
        return 0;
    /* Fixed (#584): track 1 takes the general path (entries 1 and 2); the
       original returns the start of track 2 for it (the pregap counted) */
    track--;
    off =bl_mulu_w((UWORD)track, 6);   /* AND.l #$ffff: a word already */
    p = cd.toc + 9 + off;
    /* Fixed (#585): the minutes alone; the original's MOVE.b (a0),d0
       leaves the high byte of the offset (track 44 on) in the word that
       MULU.w multiplies */
    start = (UWORD)(bl_mulu_w(p[0], 60) + p[1]);
    /* Fixed (#583): the last track ends at the lead-out of the summary
       (toc[3], toc[4]); the original reads the entry behind it, which
       CD_TOCMSF does not write */
    if (track + 1 == cd.toc[1])
        end = (UWORD)(bl_mulu_w(cd.toc[3], 60) + cd.toc[4]);
    else
        end = (UWORD)(bl_mulu_w(p[6], 60) + p[7]);
    return end - start;
}

/* CDUpdateInfo (doinfo: CD_QCODEMSF into qcodeinfo): the address of the track position's minutes;
   Fixed (#587): 0 without a device; the original returns the caller's a0 + 5
   (its a0 is loaded only after the test of flag) */
ULONG BLITZ_ROUTINE c_cdtracktime(void)
{
    if (!cd.flag)
        return 0;
    cdio(DOIO, CD_QCODEMSF, cd.qcodeinfo, 0, 0);
    return (ULONG)cd.qcodeinfo + 5;
}

/* CDTrackMins/CDTrackSecs: ASL.b #2,d0 : MOVE.b n(a0,d0.w),d0 */
static ULONG qbyte(ULONG offset, int n)
{
    ULONG index = (offset & 0xffffff00UL) | (UBYTE)(offset << 2);

    return (offset & 0xffffff00UL) | ((const UBYTE *)cd.qcodeinfo)[n + (WORD)index];
}

ULONG BLITZ_ROUTINE c_cdtrackmins(ULONG offset BL_REG(d0))
{
    return qbyte(offset, 5);
}

ULONG BLITZ_ROUTINE c_cdtrackmins2(void)
{
    return qbyte(0, 5);
}

ULONG BLITZ_ROUTINE c_cdtracksecs(ULONG offset BL_REG(d0))
{
    return qbyte(offset, 6);
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

/* ------------------------------------------- error checks (debugger only) */

BLITZ_ERRCHK_MESSAGE(badtrack, "Track number out of range");      /* fixed, #583 */
BLITZ_ERRCHK_MESSAGE(badoffset, "Offset must be 0 or 1");          /* fixed, #586 */
BLITZ_ERRCHK_MESSAGE(isopen, "CD device already open: CloseCD first");  /* fixed, #578 */

/**
 * @brief The error check of CDTrackLength (debugger only).
 *
 * Fixed (#583), not in the original (ricompactdisclib had no error check
 * area): the manual says "The track# should be checked to make sure that
 * it exists on the compact disc"; the release code reads the TOC entries
 * of a track that does not exist (track 0: 65539 bytes behind the TOC).
 * With an open device and a TOC (the SCSI TOC with scsiflag) a track
 * outside first to last track is refused; without a TOC the release code
 * returns 0 (#582) and the check lets it.  Release code unchanged (an
 * invalid argument, D-005).
 *
 * Errors: "Track number out of range".
 * @param track  d0: the track
 */
static void BLITZ_ERRCHK cdtracklengthchk_c(ULONG track BL_REG(d0))
{
    ULONG first, lastt;

    if (!cd.flag)
        return;
    if (scsiflag) {
        if (!scsitoc)
            return;
        first = TOCData[2];
        lastt = TOCData[3];
    } else {
        if (!cd.toc)
            return;
        first = cd.toc[0];
        lastt = cd.toc[1];
    }
    if (track < first || track > lastt)
        BLITZ_RUNERR(badtrack);
}

/**
 * @brief The error check of CDTrackMins offset and CDTrackSecs offset
 *        (debugger only).
 *
 * Fixed (#586), not in the original: the manual says "The optional
 * parameter offset can take the value of 0 or 1"; the release code shifts
 * the low byte of the offset by 2 (ASL.b), indexes the Q-code data with
 * the whole word and keeps the high bytes of d0 in the result, so another
 * offset reads 4 bytes per step behind (or before) the 12 bytes of the
 * Q-code.  Release code unchanged (D-005).
 *
 * Errors: "Offset must be 0 or 1".
 * @param offset  d0.w: the offset
 */
static void BLITZ_ERRCHK offsetchk_c(ULONG offset BL_REG(d0))
{
    if ((UWORD)offset > 1)
        BLITZ_RUNERR(badoffset);
}

/**
 * @brief The error check of OpenCD, all three forms (debugger only).
 *
 * Fixed (#578), not in the original: the manual says "you must call
 * CloseCD between calls to OpenCD (e.g. when changing devices)"; the
 * release code makes a new request and opens the device again, and the
 * open request is lost (it stays open).  Release code unchanged (a call
 * the programmer controls, D-005).
 *
 * Errors: "CD device already open: CloseCD first".
 */
static void BLITZ_ERRCHK opencdchk_c(void)
{
    if (cd.flag)
        BLITZ_RUNERR(isopen);
}

/* the thunks of the check functions */
BLITZ_ERRCHK_THUNK(_cdtracklengthchk, cdtracklengthchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(_offsetchk, offsetchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(_opencdchk, opencdchk_c, "d0-d1/a0-a1");

/* entries: keep d1/a0/a1 (and d0 in init/finit) */
BLITZ_THUNK(ricompactdisclib_libinit, c_libinit, "d0-d1/a0-a1");
BLITZ_THUNK(ricompactdisclib_libfinit, c_libfinit, "d0-d1/a0-a1");
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
