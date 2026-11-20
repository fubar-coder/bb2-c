/*
 * rwe ricompactdisclib - the RWE userlib ricompactdisclib: the C build
 * (libs/userlibs/rwe/ricompactdisclib.c) or the reference build of its
 * source rwelibsrc/CDlib/cdSCSI.bb (ricompactdisclib.orig), one set as
 * LIBSET says, observed as cases (rwecmp.h; the expected
 * data libs/tests/expected/rwe_ricompactdisclib.txt).
 *
 *   ricompactdisclib DIR     (DIR: ricompactdisclib and memlib .obj/.orig)
 *
 * The commands talk to a device of this test, "rwecd.device" (OpenCD
 * "rwecd.device",unit[,scsiflag]): the emulator's uaescsi.device answers
 * the SCSI path but not cd.device's TOC and Q-code commands, and it
 * completes every play at once.  rwecd.device plays a CD with 9, 50 or 20
 * tracks: it answers CD_INFO (the Status word), CD_TOCMSF (summary and
 * track entries, entries behind the last track filled with $EE),
 * CD_QCODEMSF (a changing position), HD_SCSICMD (READ TOC: the header
 * and the track descriptors with their LBA, tracks 2 to ntracks - 2 and
 * the lead-out) and accepts every other command; a request sent with
 * SendIO (CDPlayTrack) is replied at once, or in hold mode (a play in
 * progress) kept until AbortIO.  It logs every request (command, flags,
 * length, offset, data pointer, the SCSICmd and its command bytes), the
 * opens, closes and AbortIOs, the requests begun while the held play was
 * out and the replies left at the library's port (looked at after CDStop,
 * at CloseDevice, then the port is emptied), fails OpenDevice for unit 3
 * and returns an io_Error when the test asks for one (also for one
 * request only, with nothing written).  memlib is loaded with the library
 * (globalloc of init and CDReadTOC).  Every command and form is called
 * (other registers = markers); observed are d0, the requests the device
 * saw, the data the commands return (the Q-code bytes behind
 * CDUpdateInfo's address) and the kept registers.
 * Covered: every command without an open device (0; BUG(orig) (#587, #586):
 * CDUpdateInfo returns a0 + 5, CDTrackMins/Secs/Playing read their data
 * anyway), OpenCD of cd.device (missing in the emulator: the failure
 * path, BUG(orig) (#575): d0 of DeleteIORequest), of unit 3 (OpenDevice fails),
 * the name and unit kept for OpenCD without arguments, OpenCD while open
 * (BUG(orig) (#578): the old request is lost), all CD commands with their
 * arguments and io_Errors, CDReadTOC twice (the old TOC freed, the second
 * read failing), a failing summary read (BUG(orig) (#581)), a play in
 * progress stopped, closed and its request used again (BUG(orig) (#574,
 * #1156)), the TOC functions before any CDReadTOC (BUG(orig) (#582): they
 * read address 0), CDTrackLength of every track 1..9 of the 9 track CD
 * and 40..50 of the 50 track CD (BUG(orig) (#585, #583): from track 44 the start is
 * wrong; the last track reads the entry behind it), CDTrackMins/CDTrackSecs offsets 0, 1, 4, 5, -1..-5 and the
 * high bits of d0 (BUG(orig) (#586)), the SCSI path (scsiflag: CDPlayTrack's
 * fixed blocks 14000-15000 and io_Offset 1 before and after READ TOC,
 * with tracks inside, at the ends of and outside the TOC, CDNumTracks
 * from the SCSI TOC data, CDFirstTrack, CDLastTrack and CDTrackLength
 * still from toc (BUG(orig) (#580, #582)); the flag kept by OpenCD
 * without arguments), CloseCD (twice), finit with a device open and
 * without port, init again, init without a message port
 * (CreateMsgPort fails) and OpenCD without a request (CreateIORequest
 * fails).
 * Not covered: a real CD drive; a device name of more than 40 bytes
 * (overwrites toc ..., BUG(orig) (#576); bug-orig test
 * ricompactdisclib-opencd-name-overflow); CDTrackLength 0 (reads 64 KB
 * behind toc) and of tracks behind the last (memory globalloc did not
 * clear: differs between runs); CDPlayTrack with scsiflag on a 68000
 * (address error: the bug-orig test ricompactdisclib-playtrack-scsi-odd
 * on the a600; this program runs on the a1200); a globalloc failing.
 * The test device leaves the entries behind the last track
 * $EE (a real one would not write them: the last track's length is then
 * garbage, BUG(orig) (#583)).
 */
#include "rwecmp.h"
#include <exec/devices.h>
#include <exec/io.h>
#include <exec/errors.h>
#include <exec/memory.h>
#include <devices/cd.h>
#include <devices/scsidisk.h>

#define LIB "ricompactdisclib"

enum { OPENCD, CLOSECD, DOOR, PLAY, READTOC, STATUS, STOP, VOLUME, NUMTRACKS, FIRST, LAST,
       TLEN, UPDATE, FLUSH, PAUSE, REWIND, FFWD, NORMAL, SPEED, MINS, SECS, PLAYING, MOTOR,
       FINIT, INIT };

/* ---------------------------------------------------------- the test CD */

static struct Device *dev;
static int ntracks = 9;                 /* the CD in the drive */
static UWORD cdstatus = CDSTSF_CLOSED | CDSTSF_DISK | CDSTSF_TOC;
static UWORD errcmd;                    /* io_Error errval for command errcmd */
static BYTE errval;
static int erronce;                     /* errcmd for one request only, without data */
static int hold;                        /* CD_PLAYTRACK (SendIO) kept until AbortIO */
static struct IOStdReq *held;           /* the play kept */
static struct MsgPort *rport;           /* the reply port of the library's requests */
static ULONG qtick;                    /* CD_QCODEMSF: the position moves */

static struct log {
    ULONG n, opens, closes, failed, aborts;
    ULONG inflight;                     /* requests begun while the held play was out */
    ULONG atclose;                      /* CloseDevice with replies left at the port */
    UWORD cmd, abortcmd;
    UBYTE flags;
    ULONG len, off, unit;
    APTR data;
    /* HD_SCSICMD */
    ULONG slen, sactual;
    UWORD scmdlen, scmdactual, ssenselen, ssenseactual;
    UBYTE sflags, scmd[12], sdata, ssense;
} last;
#define MAXOPEN 8
static struct IORequest *opened[MAXOPEN];

/* the start of track t (1..ntracks+1 = lead-out) in seconds */
static ULONG trackstart(int t)
{
    ULONG s = 2;
    int i;
    for (i = 1; i < t; i++)
        s += 60 + (ULONG)(i * 37) % 300;
    return s;
}

static void msf(UBYTE *p, ULONG secs, UBYTE frame)
{
    p[0] = 0;
    p[1] = (UBYTE)(secs / 60);
    p[2] = (UBYTE)(secs % 60);
    p[3] = frame;
}

/* replies left at the library's reply port (it never takes one off:
   BUG(orig) #574, #1156): 1 if any; the port is emptied, so that the next
   reply or a freed request does not break its list */
static ULONG portreplies(void)
{
    ULONG n = 0;

    if (!rport)
        return 0;
    Forbid();
    if (!IsListEmpty(&rport->mp_MsgList)) {
        struct List *l = &rport->mp_MsgList;
        n = 1;
        l->lh_Head = (struct Node *)&l->lh_Tail;
        l->lh_Tail = NULL;
        l->lh_TailPred = (struct Node *)&l->lh_Head;
    }
    Permit();
    return n;
}

static void dev_open(struct IORequest *ior __asm("a1"), LONG unit __asm("d0"),
                     ULONG flags __asm("d1"), struct Device *d __asm("a6"))
{
    int i;
    (void)flags;
    if (unit == 3) {
        ior->io_Error = IOERR_OPENFAIL;
        last.failed++;
        return;
    }
    ior->io_Error = 0;
    ior->io_Device = d;
    ior->io_Unit = (struct Unit *)unit;
    d->dd_Library.lib_OpenCnt++;
    last.opens++;
    last.unit = unit;
    for (i = 0; i < MAXOPEN; i++)
        if (!opened[i]) {
            opened[i] = ior;
            break;
        }
}

static ULONG dev_close(struct IORequest *ior __asm("a1"), struct Device *d __asm("a6"))
{
    int i;
    d->dd_Library.lib_OpenCnt--;
    last.closes++;
    if (ior == (struct IORequest *)held)
        held = NULL;
    if (ior->io_Message.mn_ReplyPort == rport && portreplies())
        last.atclose = 1;
    last.unit = (ULONG)ior->io_Unit;
    for (i = 0; i < MAXOPEN; i++)
        if (opened[i] == ior)
            opened[i] = NULL;
    return 0;
}

static ULONG dev_null(void)
{
    return 0;
}

static ULONG dev_abortio(struct IORequest *ior __asm("a1"))
{
    last.aborts++;
    last.abortcmd = ior->io_Command;
    if (ior == (struct IORequest *)held) {
        held = NULL;
        ior->io_Error = IOERR_ABORTED;
        ReplyMsg(&ior->io_Message);
    }
    return 0;
}

/* the start of track t in frames (SCSI READ TOC: its LBA) */
static ULONG tracklba(int t)
{
    return (trackstart(t) - 2) * 75 + (ULONG)t * 3;
}

static void putlong(UBYTE *p, ULONG v)
{
    p[0] = (UBYTE)(v >> 24);
    p[1] = (UBYTE)(v >> 16);
    p[2] = (UBYTE)(v >> 8);
    p[3] = (UBYTE)v;
}

static void scsicmd(struct IOStdReq *ior)
{
    struct log *l = &last;
    struct SCSICmd *c = ior->io_Data;
    UBYTE *data = (UBYTE *)c->scsi_Data;
    int i;

    l->slen = c->scsi_Length;
    l->sactual = c->scsi_Actual;
    l->scmdlen = c->scsi_CmdLength;
    l->scmdactual = c->scsi_CmdActual;
    l->ssenselen = c->scsi_SenseLength;
    l->ssenseactual = c->scsi_SenseActual;
    l->sflags = c->scsi_Flags;
    l->sdata = data != NULL;
    l->ssense = c->scsi_SenseData != NULL;
    memset(l->scmd, 0, sizeof l->scmd);
    for (i = 0; i < c->scsi_CmdLength && i < 12; i++)
        l->scmd[i] = c->scsi_Command[i];
    c->scsi_Status = 0;
    c->scsi_CmdActual = c->scsi_CmdLength;
    if (c->scsi_Command[0] == 0x43 && data && c->scsi_Length >= 4) {
        /* READ TOC: the header (data length, first track 2, last track
           ntracks - 2), then a descriptor of 8 bytes (track, LBA) per
           track and the lead-out ($AA), as far as scsi_Length takes them */
        int first = 2, lastt = ntracks - 2, t;
        ULONG len = 4;

        data[0] = 0;
        data[1] = (UBYTE)(2 + 8 * (lastt - first + 2));
        data[2] = (UBYTE)first;
        data[3] = (UBYTE)lastt;
        for (t = first; t <= lastt + 1 && len + 8 <= c->scsi_Length; t++, len += 8) {
            UBYTE *d = data + len;
            d[0] = 0;
            d[1] = 0x10;
            d[2] = (UBYTE)(t > lastt ? 0xaa : t);
            d[3] = 0;
            putlong(d + 4, tracklba(t));
        }
        c->scsi_Actual = len;
    }
}

static ULONG dev_beginio(struct IOStdReq *ior __asm("a1"))
{
    struct log *l = &last;
    UBYTE *p = ior->io_Data;
    ULONG i;

    l->n++;
    l->cmd = ior->io_Command;
    l->flags = ior->io_Flags;
    l->len = ior->io_Length;
    l->off = ior->io_Offset;
    l->data = ior->io_Data;
    l->unit = (ULONG)ior->io_Unit;
    rport = ior->io_Message.mn_ReplyPort;
    if (held && ior == held) {
        /* the held play's request used again while it is out (#1156) */
        l->inflight++;
        held = NULL;
    }
    ior->io_Error = 0;
    ior->io_Actual = 0;
    if (ior->io_Command == errcmd && erronce) {
        /* one failing request, nothing written */
        ior->io_Error = errval;
        errcmd = 0;
        erronce = 0;
        goto done;
    }
    switch (ior->io_Command) {
    case CD_INFO:
        for (i = 0; i < ior->io_Length && i < 26; i++)
            p[i] = (UBYTE)(i * 7 + 1);
        if (ior->io_Length >= 26) {
            p[24] = (UBYTE)(cdstatus >> 8);
            p[25] = (UBYTE)cdstatus;
        }
        ior->io_Actual = ior->io_Length;
        break;
    case CD_TOCMSF:
        for (i = 0; i < ior->io_Length; i++) {
            UBYTE *e = p + 6 * i;
            ULONG t = ior->io_Offset + i;
            if (t == 0) {
                e[0] = 1;
                e[1] = (UBYTE)ntracks;
                msf(e + 2, trackstart(ntracks + 1), 33);
            } else if (t <= (ULONG)ntracks) {
                e[0] = 0x10;
                e[1] = (UBYTE)t;
                msf(e + 2, trackstart((int)t), (UBYTE)(t * 3));
            } else {
                memset(e, 0xee, 6);
            }
        }
        ior->io_Actual = ior->io_Length;
        break;
    case CD_QCODEMSF:
        qtick += 7;
        p[0] = 0x11;
        p[1] = (UBYTE)(1 + qtick % ntracks);
        p[2] = 1;
        p[3] = 0;
        msf(p + 4, qtick % 250, (UBYTE)(qtick % 75));
        msf(p + 8, 2 + qtick * 3, (UBYTE)(qtick % 70));
        break;
    case HD_SCSICMD:
        scsicmd(ior);
        break;
    default:
        break;
    }
    if (ior->io_Command == errcmd)
        ior->io_Error = errval;
done:
    /* SendIO (CDPlayTrack): kept until AbortIO in hold mode (a play in
       progress), else done at once and replied (a reply the library does
       not take stays at its port: portreplies) */
    if (!(ior->io_Flags & IOF_QUICK)) {
        ior->io_Message.mn_Node.ln_Type = NT_MESSAGE;
        if (hold && ior->io_Command == CD_PLAYTRACK) {
            if (held) {
                held->io_Error = IOERR_ABORTED;
                ReplyMsg(&held->io_Message);
            }
            held = ior;
            return 0;
        }
        ReplyMsg(&ior->io_Message);
    }
    /* d0 0: the original's CDPlayTrack returns the d0 SendIO leaves
       (BUG(orig) (#1157)), the same in both builds only if it is defined */
    return 0;
}

static const APTR devfuncs[] = { (APTR)dev_open, (APTR)dev_close, (APTR)dev_null,
                                 (APTR)dev_null, (APTR)dev_beginio, (APTR)dev_abortio,
                                 (APTR)-1 };

static BOOL add_device(void)
{
    dev = (struct Device *)MakeLibrary((CONST_APTR)devfuncs, NULL, NULL, sizeof(struct Device),
                                       0);
    if (!dev)
        return FALSE;
    dev->dd_Library.lib_Node.ln_Type = NT_DEVICE;
    dev->dd_Library.lib_Node.ln_Name = (char *)"rwecd.device";
    AddDevice(dev);
    return TRUE;
}

static void rem_device(void)
{
    Forbid();
    Remove(&dev->dd_Library.lib_Node);
    Permit();
    FreeMem((UBYTE *)dev - dev->dd_Library.lib_NegSize,
            dev->dd_Library.lib_NegSize + dev->dd_Library.lib_PosSize);
}

/* ------------------------------------------------------------ the checks */

/* where a data pointer points: 0, into the library, elsewhere */
static ULONG where(APTR p)
{
    struct lib *l = getlib(&loaded, LIB);
    ULONG a = (ULONG)p;
    if (!a)
        return 0;
    if (a >= (ULONG)l->code && a < (ULONG)l->code + l->size)
        return 1;
    return 2;
}

static void same_device(void)
{
    OBS("requests", last.n);
    OBS("opens", last.opens);
    OBS("closes", last.closes);
    OBS("failed opens", last.failed);
    OBS("aborts", last.aborts);
    OBS("abort command", last.abortcmd);
    OBS("command", last.cmd);
    OBS("flags", last.flags);
    OBS("length", last.len);
    OBS("offset", last.off);
    OBS("unit", last.unit);
    OBS("data", where(last.data));
    OBS("scsi_Length", last.slen);
    OBS("scsi_Actual", last.sactual);
    OBS("scsi_CmdLength", last.scmdlen);
    OBS("scsi_CmdActual", last.scmdactual);
    OBS("scsi_SenseLength", last.ssenselen);
    OBS("scsi_SenseActual", last.ssenseactual);
    OBS("scsi_Flags", last.sflags);
    OBS("scsi_Data", last.sdata);
    OBS("scsi_SenseData", last.ssense);
    OBSBYTES("scsi_Command", last.scmd, 12);
}

/* CloseCD, finit: replies left at the port when the device was closed */
static void closereplies(void)
{
    OBS("replies at the port at CloseDevice", last.atclose);
    last.atclose = 0;
}

/* finit: the port is gone (no reply port to look at until the next
   request) */
static void finit(const char *what)
{
    ROUT = "finit";
    strcpy(VEC, what);
    DO1(ENT(LIB, FINIT, 0), (void)0);
    same_device();
    closereplies();
    rport = NULL;
}

/* a call of entry idx, form `form`, d0/d1/d2 = arguments; d0 compared */
static void call(int idx, int form, const char *name, ULONG d0, ULONG d1, ULONG d2)
{
    ROUT = name;
    DO1(ENT(LIB, idx, form), R.d[0] = d0; R.d[1] = d1; R.d[2] = d2);
    OBS("d0", R.d[0]);
    same_device();
    if (idx == STOP)
        OBS("replies at the port", portreplies());
    if (idx == CLOSECD)
        closereplies();
}

#define CALL0(idx, name) call(idx, 0, name, MARK(0), MARK(1), MARK(2))
#define CALL1(idx, name, a) call(idx, 0, name, a, MARK(1), MARK(2))
#define CALL2(idx, name, a, b) call(idx, 0, name, a, b, MARK(2))

static void opencd(const char *name, ULONG unit, int form, ULONG flag)
{
    sprintf(VEC, "%s,%lu (form %d, scsiflag %lu)", name ? name : "-", unit, form, flag);
    call(OPENCD, form, "OpenCD", (ULONG)name, unit, flag);
}

/* the play of hold mode: still out, and the requests begun while it was */
static void playstate(void)
{
    OBS("play held", held != NULL);
    OBS("requests begun while the play was out", last.inflight);
}

/* CDUpdateInfo: d0 = the Q-code + 5 in the library (else a0 + 5) */
static void updateinfo(void)
{
    ROUT = "CDUpdateInfo";
    DO1(ENT(LIB, UPDATE, 0), (void)0);
    OBS("d0 in the library", where((APTR)R.d[0]));
    if (where((APTR)R.d[0]) == 1) {
        OBSBYTES("qcodeinfo", (const UBYTE *)R.d[0] - 5, 12);
    } else {
        OBS("d0", R.d[0]);
    }
    same_device();
}

static void qcode(void)
{
    static const ULONG offs[] = { 0, 1, 4, 5, 0xffff, 0xfffe, 0xfffd, 0xfffc, 0xfffb,
                                  0x12340040, 0x5a00ffff, 0xabcd0001 };
    unsigned i;
    strcpy(VEC, "");
    call(MINS, 0, "CDTrackMins", MARK(0), MARK(1), MARK(2));
    call(SECS, 0, "CDTrackSecs", MARK(0), MARK(1), MARK(2));
    call(PLAYING, 0, "CDTrackPlaying", MARK(0), MARK(1), MARK(2));
    for (i = 0; i < sizeof offs / sizeof offs[0]; i++) {
        sprintf(VEC, "$%08lX", offs[i]);
        call(MINS, 1, "CDTrackMins", offs[i], MARK(1), MARK(2));
        call(SECS, 1, "CDTrackSecs", offs[i], MARK(1), MARK(2));
    }
}

/* the TOC functions and CDTrackLength from..to */
static void tocfuncs(int from, int to)
{
    int t;
    strcpy(VEC, "");
    CALL0(NUMTRACKS, "CDNumTracks");
    CALL0(FIRST, "CDFirstTrack");
    CALL0(LAST, "CDLastTrack");
    for (t = from; t <= to; t++) {
        sprintf(VEC, "%d", t);
        CALL1(TLEN, "CDTrackLength", (ULONG)t);
    }
}

/* CDVolume volume,fade; with a device open the request is checked for the
   BUG(orig) (#982): CDVolume puts the volume into io_Length and the fade time
   into io_Offset, the reverse of cd.device's CD_ATTENUATE (io_Offset = volume,
   io_Length = fade in frames); both builds send the swapped fields */
static void volume(ULONG vol, ULONG fade)
{
    ULONG n0 = last.n;

    CALL2(VOLUME, "CDVolume", vol, fade);
    if (last.n > n0) {
        check("CDVolume: CD_ATTENUATE", last.cmd == 44);
        check("CDVolume: io_Length is the volume (BUG(orig), #982)", last.len == vol);
        check("CDVolume: io_Offset is the fade (BUG(orig), #982)", last.off == fade);
    }
}

/* every command; with a device open the CD commands, else 0 */
static void commands(const char *what)
{
    static const ULONG onoff[] = { 0, 1, 2, 3, 0xffffffffUL };
    unsigned i;

    sprintf(VEC, "%s", what);
    CALL0(STATUS, "CDStatus");
    CALL0(FLUSH, "CDFlush");
    CALL0(STOP, "CDStop");
    CALL0(REWIND, "CDRewind");
    CALL0(FFWD, "CDFastForward");
    CALL0(NORMAL, "CDNormalSpeed");
    for (i = 0; i < sizeof onoff / sizeof onoff[0]; i++) {
        sprintf(VEC, "%s %ld", what, (LONG)onoff[i]);
        CALL1(DOOR, "CDDoor", onoff[i]);
        CALL1(PAUSE, "CDPause", onoff[i]);
        CALL1(MOTOR, "CDMotor", onoff[i]);
        CALL1(SPEED, "CDSpeed", onoff[i]);
    }
    sprintf(VEC, "%s 3,2", what);
    CALL2(PLAY, "CDPlayTrack", 3, 2);
    CALL0(STOP, "CDStop");
    sprintf(VEC, "%s 200,75", what);
    volume(200, 75);
    sprintf(VEC, "%s 0x7fff,0", what);
    volume(0x7fff, 0);
    updateinfo();
    qcode();
}

int main(int argc, char **argv)
{
    static const char *names[] = { LIB, "memlib", NULL };
    static const char rwecd[] = "rwecd.device";
    ULONG avail;
    APTR old;
    int i;

    if (argc < 2) {
        printf("usage: ricompactdisclib DIR\n");
        return 20;
    }
    if (!load_set(argv[1], names))
        return 20;
    KEEPS("init", d2-d7|a2|a3);
    KEEPS("CloseCD", d2-d7|a2|a3);
    KEEPS("CDReadTOC", d5|d6|a2|a3);
    KEEPS("CDStatus", d2-d7|a2|a3);
    KEEPS("CDFlush", d1-d7|a0|a2|a3);
    KEEPS("CDStop", d2-d7|a2|a3);
    KEEPS("CDRewind", d1-d7|a0|a2|a3);
    KEEPS("CDFastForward", d1-d7|a0|a2|a3);
    KEEPS("CDNormalSpeed", d1-d7|a0|a2|a3);
    KEEPS("CDDoor", d1-d7|a0|a2|a3);
    KEEPS("CDPause", d1-d7|a0|a2|a3);
    KEEPS("CDMotor", d1-d7|a0|a2|a3);
    KEEPS("CDSpeed", d1-d7|a0|a2|a3);
    KEEPS("CDPlayTrack", d5-d7|a2|a3);
    KEEPS("CDVolume", d1-d7|a0|a2|a3);
    KEEPS("CDUpdateInfo", d2-d7|a2|a3);
    KEEPS("CDTrackMins", d1-d7|a1-a3|a6);
    KEEPS("CDTrackSecs", d1-d7|a1-a3|a6);
    KEEPS("CDTrackPlaying", d1-d7|a1-a3|a6);
    KEEPS("OpenCD", d2-d7|a2|a3);
    KEEPS("CloseCD (failed)", d1-d7|a0-a3|a6);
    KEEPS("CDNumTracks", d2-d7|a1-a3|a6);
    KEEPS("CDFirstTrack", d1-d7|a1-a3|a6);
    KEEPS("CDLastTrack", d1-d7|a1-a3|a6);
    KEEPS("CDTrackLength", d3-d7|a1-a3|a6);
    KEEPS("CloseCD (again)", d1-d7|a0-a3|a6);
    KEEPS("finit", d2-d7|a2|a3);
    if (!add_device()) {
        check("MakeLibrary rwecd.device", FALSE);
        return 20;
    }
    {   /* cd.device once before the memory count: what OpenDevice keeps
           of a device that cannot be loaded (ramlib, DOS) */
        struct MsgPort *mp = CreateMsgPort();
        struct IORequest *ior = CreateIORequest(mp, 200);
        if (ior && !OpenDevice((CONST_STRPTR)"cd.device", 0, ior, 0))
            CloseDevice(ior);
        DeleteIORequest(ior);
        DeleteMsgPort(mp);
    }
    avail = AvailMem(MEMF_ANY);
    rwe_mem_start();
    ROUT = "memlib init";
    strcpy(VEC, "");
    markregs(&R);
    callstk(entry_form(getlib(&loaded, "memlib"), 7, 0), &R, NULL, 0, &POP);

    ROUT = "init";
    DO1(ENT(LIB, INIT, 0), (void)0);

    /* nothing open */
    CALL0(CLOSECD, "CloseCD");
    CALL0(READTOC, "CDReadTOC");
    commands("not open");

    /* cd.device: not in the emulator (or no drive) */
    opencd(NULL, 0, 0, 0);
    printf("ricompactdisclib: OpenCD (cd.device) = %ld\n", (LONG)R.d[0]);
    if (R.d[0] == (ULONG)-1)
        CALL0(CLOSECD, "CloseCD (cd.device)");

    /* unit 3 fails */
    opencd(rwecd, 3, 1, 0);
    CALL0(CLOSECD, "CloseCD (failed)");

    /* the test CD, unit 1 */
    opencd(rwecd, 1, 1, 0);
    commands("open");
    errcmd = CD_EJECT;
    errval = CDERR_ABORTED;
    sprintf(VEC, "io_Error %d", errval);
    CALL1(DOOR, "CDDoor", 1);
    errcmd = CD_INFO;
    errval = 40;
    sprintf(VEC, "io_Error %d", errval);
    CALL0(STATUS, "CDStatus");
    errcmd = 0;
    strcpy(VEC, "9 tracks");
    CALL0(READTOC, "CDReadTOC");
    tocfuncs(1, 9);
    updateinfo();
    qcode();
    updateinfo();
    qcode();

    /* a CD with 50 tracks: the old TOC freed; from track 44 BUG(orig) (#585) */
    ntracks = 50;
    strcpy(VEC, "50 tracks");
    CALL0(READTOC, "CDReadTOC");
    tocfuncs(40, 50);
    tocfuncs(1, 3);
    /* the second read fails */
    errcmd = CD_TOCMSF;
    errval = CDERR_NotSpecified;
    cdstatus = CDSTSF_CLOSED;
    strcpy(VEC, "error");
    CALL0(READTOC, "CDReadTOC");
    errcmd = 0;
    CALL0(STATUS, "CDStatus");
    /* only the summary read fails (nothing written): the entries are read
       with the last track of the summary before (BUG(orig) (#581)) */
    ntracks = 9;
    cdstatus = CDSTSF_CLOSED | CDSTSF_DISK | CDSTSF_TOC;
    errcmd = CD_TOCMSF;
    errval = CDERR_NotSpecified;
    erronce = 1;
    strcpy(VEC, "summary error");
    CALL0(READTOC, "CDReadTOC");
    tocfuncs(1, 3);

    /* OpenCD without arguments: rwecd.device unit 1 kept; the open
       request is lost (BUG(orig) (#578)) */
    ntracks = 9;
    opencd(NULL, 0, 0, 0);
    CALL0(READTOC, "CDReadTOC");
    tocfuncs(1, 3);
    CALL0(CLOSECD, "CloseCD");
    CALL0(CLOSECD, "CloseCD (again)");
    commands("closed");

    /* a play in progress (kept until AbortIO): CDStop and CloseCD abort it
       without WaitIO, the reply stays at the port (BUG(orig) (#574));
       other commands and the next play use its request while it is out
       (BUG(orig) (#1156)) */
    hold = 1;
    opencd(rwecd, 1, 1, 0);
    strcpy(VEC, "held 1,1");
    CALL2(PLAY, "CDPlayTrack", 1, 1);
    playstate();
    CALL0(STOP, "CDStop");
    playstate();
    strcpy(VEC, "held 2,1");
    CALL2(PLAY, "CDPlayTrack", 2, 1);
    CALL0(STATUS, "CDStatus");
    playstate();
    CALL0(STOP, "CDStop");
    playstate();
    strcpy(VEC, "held 3,1 4,1");
    CALL2(PLAY, "CDPlayTrack", 3, 1);
    CALL2(PLAY, "CDPlayTrack", 4, 1);
    playstate();
    CALL0(STOP, "CDStop");
    playstate();
    strcpy(VEC, "held 5,1 closed");
    CALL2(PLAY, "CDPlayTrack", 5, 1);
    CALL0(CLOSECD, "CloseCD");
    playstate();
    hold = 0;

    /* scsiflag: the SCSI path; CDPlayTrack before CDReadTOC (in
       commands), then after it: the first, the last, more tracks than
       there are, track 0 (behind the first) and behind the last */
    opencd(rwecd, 2, 2, 1);
    commands("scsi");
    strcpy(VEC, "scsi");
    CALL0(READTOC, "CDReadTOC");
    tocfuncs(2, 7);
    sprintf(VEC, "scsi 5,1");
    CALL2(PLAY, "CDPlayTrack", 5, 1);
    sprintf(VEC, "scsi 2,1");
    CALL2(PLAY, "CDPlayTrack", 2, 1);
    sprintf(VEC, "scsi 7,1");
    CALL2(PLAY, "CDPlayTrack", 7, 1);
    sprintf(VEC, "scsi 6,5");
    CALL2(PLAY, "CDPlayTrack", 6, 5);
    sprintf(VEC, "scsi 0,1");
    CALL2(PLAY, "CDPlayTrack", 0, 1);
    sprintf(VEC, "scsi 8,1");
    CALL2(PLAY, "CDPlayTrack", 8, 1);
    sprintf(VEC, "scsi 3,0");
    CALL2(PLAY, "CDPlayTrack", 3, 0);
    CALL0(CLOSECD, "CloseCD");
    /* OpenCD without arguments: still the SCSI path */
    opencd(NULL, 0, 0, 0);
    ntracks = 20;
    strcpy(VEC, "scsi kept");
    CALL0(READTOC, "CDReadTOC");
    CALL0(NUMTRACKS, "CDNumTracks");
    CALL2(PLAY, "CDPlayTrack", 1, 1);
    /* OpenCD name,unit: scsiflag stays too */
    opencd(rwecd, 0, 1, 0);
    CALL0(READTOC, "CDReadTOC");
    CALL0(NUMTRACKS, "CDNumTracks");

    /* finit with the device open, again without port, init again */
    finit("open");
    finit("again");
    ROUT = "init";
    strcpy(VEC, "again");
    DO1(ENT(LIB, INIT, 0), (void)0);
    ntracks = 9;
    opencd(NULL, 0, 0, 0);              /* the name is cd.device again */
    opencd(rwecd, 0, 1, 0);
    /* the TOC functions before any CDReadTOC (BUG(orig) (#582): toc is
       0, they read from address 0) */
    tocfuncs(1, 2);
    strcpy(VEC, "again");
    CALL0(READTOC, "CDReadTOC");
    tocfuncs(1, 2);
    CALL2(PLAY, "CDPlayTrack", 2, 1);   /* scsiflag 0 again */
    finit("played");

    /* no request: CreateIORequest fails */
    ROUT = "init";
    DO1(ENT(LIB, INIT, 0), (void)0);
    Forbid();
    old = SetFunction((struct Library *)SysBase, -654, (APTR)rwe_fail0);
    CacheClearU();
    opencd(rwecd, 0, 1, 0);
    SetFunction((struct Library *)SysBase, -654, old);
    CacheClearU();
    Permit();
    commands("no request");
    finit("no request");

    /* no message port: CreateMsgPort fails in init */
    Forbid();
    old = SetFunction((struct Library *)SysBase, -666, (APTR)rwe_fail0);
    CacheClearU();
    ROUT = "init";
    strcpy(VEC, "no port");
    DO1(ENT(LIB, INIT, 0), (void)0);
    SetFunction((struct Library *)SysBase, -666, old);
    CacheClearU();
    Permit();
    opencd(rwecd, 0, 1, 0);
    commands("no port");
    finit("no port");

    printf("rwecd.device: %lu requests, %lu opens, %lu failed, %lu aborts\n", last.n,
           last.opens, last.failed, last.aborts);
    check("rwecd.device used", last.n >= 50 && last.opens >= 5 && last.failed >= 1);

    /* the requests the library lost (OpenCD while open) */
    for (i = 0; i < MAXOPEN; i++)
        if (opened[i]) {
            struct IORequest *ior = opened[i];
            CloseDevice(ior);
            DeleteIORequest(ior);
        }
    ROUT = "memlib finit";
    markregs(&R);
    callstk(entry_form(getlib(&loaded, "memlib"), 8, 0), &R, NULL, 0, &POP);
    rwe_mem_check(LIB, avail, TRUE);
    rem_device();
    return rwe_done(LIB);
}
