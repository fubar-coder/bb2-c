/*
 * rwe ricompactdisclib - the C build of the RWE userlib ricompactdisclib
 * (libs/userlibs/rwe/ricompactdisclib.c) against the reference build of
 * its source rwelibsrc/CDlib/cdSCSI.bb (ricompactdisclib.orig), method of
 * rwecmp.h.
 *
 *   ricompactdisclib DIR     (DIR: ricompactdisclib and memlib .obj/.orig)
 *
 * There is no CD drive in the emulator: the commands talk to a device of
 * this test, "rwecd.device" (OpenCD "rwecd.device",unit[,scsiflag]), that
 * plays a CD with 9 or 50 tracks: it answers CD_INFO (the Status word),
 * CD_TOCMSF (summary and track entries, entries behind the last track
 * filled with $EE), CD_QCODEMSF (a changing position), HD_SCSICMD (READ
 * TOC: the header of the TOC data) and accepts every other command; it
 * logs every request (command, flags, length, offset, data pointer, the
 * SCSICmd and its command bytes), the opens, closes and AbortIOs, fails
 * OpenDevice for unit 3 and returns an io_Error when the test asks for
 * one.  Each set has its own memlib (globalloc of init and CDReadTOC).
 * Every command and form is called on both sets with the same inputs
 * (other registers = markers); compared are d0, the requests the device
 * saw, the data the commands return (the Q-code bytes behind
 * CDUpdateInfo's address) and every register the original keeps.
 * Covered: every command without an open device (0; BUG(orig) (#587, #586):
 * CDUpdateInfo returns a0 + 5, CDTrackMins/Secs/Playing read their data
 * anyway), OpenCD of cd.device (missing in the emulator: the failure
 * path, BUG(orig) (#575): d0 of DeleteIORequest), of unit 3 (OpenDevice fails),
 * the name and unit kept for OpenCD without arguments, OpenCD while open
 * (BUG(orig) (#578): the old request is lost), all CD commands with their
 * arguments and io_Errors, CDReadTOC twice (the old TOC freed, the second
 * read failing), CDTrackLength of every track 1..9 of the 9 track CD
 * and 40..50 of the 50 track CD (BUG(orig) (#585, #583): from track 44 the start is
 * wrong; the last track reads the entry behind it), CDTrackMins/CDTrackSecs offsets 0, 1, 4, 5, -1..-5 and the
 * high bits of d0 (BUG(orig) (#586)), the SCSI path (scsiflag: CDPlayTrack's
 * fixed blocks 14000-15000 and io_Offset 1, READ TOC, CDNumTracks from
 * the SCSI TOC data, CDFirstTrack still from toc; the flag kept by OpenCD
 * without arguments), CloseCD (twice), finit with a device open and
 * without port, init again, init without a message port
 * (CreateMsgPort fails) and OpenCD without a request (CreateIORequest
 * fails).
 * Not covered: a real CD drive (none in the emulator); a device name of
 * more than 40 bytes (overwrites toc ..., BUG(orig) (#576)); CDTrackLength 0
 * (reads 64 KB behind toc) and of tracks behind the last (memory
 * globalloc did not clear: differs between the sets); the TOC functions
 * before any CDReadTOC (they read address 0); CDPlayTrack with scsiflag
 * on a 68000 (address error; the emulator has a 68020); a globalloc
 * failing.  The test device leaves the entries behind the last track
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
static ULONG qtick[2];                  /* CD_QCODEMSF: the position moves */

static struct log {
    ULONG n, opens, closes, failed, aborts;
    UWORD cmd, abortcmd;
    UBYTE flags;
    ULONG len, off, unit;
    APTR data;
    /* HD_SCSICMD */
    ULONG slen, sactual;
    UWORD scmdlen, scmdactual, ssenselen, ssenseactual;
    UBYTE sflags, scmd[12], sdata, ssense;
} last[2];
#define MAXOPEN 8
static struct IORequest *opened[2][MAXOPEN];

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

static void dev_open(struct IORequest *ior __asm("a1"), LONG unit __asm("d0"),
                     ULONG flags __asm("d1"), struct Device *d __asm("a6"))
{
    int i;
    (void)flags;
    if (unit == 3) {
        ior->io_Error = IOERR_OPENFAIL;
        last[cur].failed++;
        return;
    }
    ior->io_Error = 0;
    ior->io_Device = d;
    ior->io_Unit = (struct Unit *)unit;
    d->dd_Library.lib_OpenCnt++;
    last[cur].opens++;
    last[cur].unit = unit;
    for (i = 0; i < MAXOPEN; i++)
        if (!opened[cur][i]) {
            opened[cur][i] = ior;
            break;
        }
}

static ULONG dev_close(struct IORequest *ior __asm("a1"), struct Device *d __asm("a6"))
{
    int i;
    d->dd_Library.lib_OpenCnt--;
    last[cur].closes++;
    last[cur].unit = (ULONG)ior->io_Unit;
    for (i = 0; i < MAXOPEN; i++)
        if (opened[cur][i] == ior)
            opened[cur][i] = NULL;
    return 0;
}

static ULONG dev_null(void)
{
    return 0;
}

static ULONG dev_abortio(struct IORequest *ior __asm("a1"))
{
    last[cur].aborts++;
    last[cur].abortcmd = ior->io_Command;
    return 0;
}

static void scsicmd(struct IOStdReq *ior)
{
    struct log *l = &last[cur];
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
        /* READ TOC: the header (data length, first and last track) */
        data[0] = 0;
        data[1] = (UBYTE)(2 + 8 * (ntracks - 1));
        data[2] = 2;
        data[3] = (UBYTE)(ntracks - 2);
        c->scsi_Actual = 4;
    }
}

static void dev_beginio(struct IOStdReq *ior __asm("a1"))
{
    struct log *l = &last[cur];
    UBYTE *p = ior->io_Data;
    ULONG i;

    l->n++;
    l->cmd = ior->io_Command;
    l->flags = ior->io_Flags;
    l->len = ior->io_Length;
    l->off = ior->io_Offset;
    l->data = ior->io_Data;
    l->unit = (ULONG)ior->io_Unit;
    ior->io_Error = 0;
    ior->io_Actual = 0;
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
        qtick[cur] += 7;
        p[0] = 0x11;
        p[1] = (UBYTE)(1 + qtick[cur] % ntracks);
        p[2] = 1;
        p[3] = 0;
        msf(p + 4, qtick[cur] % 250, (UBYTE)(qtick[cur] % 75));
        msf(p + 8, 2 + qtick[cur] * 3, (UBYTE)(qtick[cur] % 70));
        break;
    case HD_SCSICMD:
        scsicmd(ior);
        break;
    default:
        break;
    }
    if (ior->io_Command == errcmd)
        ior->io_Error = errval;
    /* SendIO (CDPlayTrack): done at once, but not replied - nobody
       waits for it, and a reply would stay in the library's port (the
       next one would link the same message again) */
    if (!(ior->io_Flags & IOF_QUICK))
        ior->io_Message.mn_Node.ln_Type = NT_REPLYMSG;
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
    struct lib *l = getlib(&sets[cur], LIB);
    ULONG a = (ULONG)p;
    if (!a)
        return 0;
    if (a >= (ULONG)l->code && a < (ULONG)l->code + l->size)
        return 1;
    return 2;
}

static void same_device(void)
{
    SAME("requests", last[cur].n);
    SAME("opens", last[cur].opens);
    SAME("closes", last[cur].closes);
    SAME("failed opens", last[cur].failed);
    SAME("aborts", last[cur].aborts);
    SAME("abort command", last[cur].abortcmd);
    SAME("command", last[cur].cmd);
    SAME("flags", last[cur].flags);
    SAME("length", last[cur].len);
    SAME("offset", last[cur].off);
    SAME("unit", last[cur].unit);
    SAME("data", where(last[cur].data));
    SAME("scsi_Length", last[cur].slen);
    SAME("scsi_Actual", last[cur].sactual);
    SAME("scsi_CmdLength", last[cur].scmdlen);
    SAME("scsi_CmdActual", last[cur].scmdactual);
    SAME("scsi_SenseLength", last[cur].ssenselen);
    SAME("scsi_SenseActual", last[cur].ssenseactual);
    SAME("scsi_Flags", last[cur].sflags);
    SAME("scsi_Data", last[cur].sdata);
    SAME("scsi_SenseData", last[cur].ssense);
    {
        const UBYTE *c[2] = { last[0].scmd, last[1].scmd };
        SAMEBYTES("scsi_Command", c, 12);
    }
}

/* a call of entry idx, form `form`, d0/d1/d2 = arguments; d0 compared */
static void call(int idx, int form, const char *name, ULONG d0, ULONG d1, ULONG d2)
{
    ROUT = name;
    DO2(ENT(LIB, idx, form), R[cur].d[0] = d0; R[cur].d[1] = d1; R[cur].d[2] = d2);
    SAME("d0", R[cur].d[0]);
    same_device();
}

#define CALL0(idx, name) call(idx, 0, name, MARK(0), MARK(1), MARK(2))
#define CALL1(idx, name, a) call(idx, 0, name, a, MARK(1), MARK(2))
#define CALL2(idx, name, a, b) call(idx, 0, name, a, b, MARK(2))

static void opencd(const char *name, ULONG unit, int form, ULONG flag)
{
    sprintf(VEC, "%s,%lu (form %d, scsiflag %lu)", name ? name : "-", unit, form, flag);
    call(OPENCD, form, "OpenCD", (ULONG)name, unit, flag);
}

/* CDUpdateInfo: d0 = the Q-code + 5 in the library (else a0 + 5) */
static void updateinfo(void)
{
    const UBYTE *q[2];
    ROUT = "CDUpdateInfo";
    DO2(ENT(LIB, UPDATE, 0), (void)0);
    SAME("d0 in the library", where((APTR)R[cur].d[0]));
    if (where((APTR)R[0].d[0]) == 1 && where((APTR)R[1].d[0]) == 1) {
        for (cur = 0; cur < 2; cur++)
            q[cur] = (const UBYTE *)R[cur].d[0] - 5;
        SAMEBYTES("qcodeinfo", q, 12);
    } else {
        SAME("d0", R[cur].d[0]);
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
    ULONG n0[2] = { last[0].n, last[1].n };

    CALL2(VOLUME, "CDVolume", vol, fade);
    for (cur = 0; cur < 2; cur++)
        if (last[cur].n > n0[cur]) {
            check("CDVolume: CD_ATTENUATE", last[cur].cmd == 44);
            check("CDVolume: io_Length is the volume (BUG(orig), #982)", last[cur].len == vol);
            check("CDVolume: io_Offset is the fade (BUG(orig), #982)", last[cur].off == fade);
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
    if (!load_sets(argv[1], names))
        return 20;
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
    for (cur = 0; cur < 2; cur++) {
        markregs(&R[cur]);
        callstk(entry_form(getlib(&sets[cur], "memlib"), 7, 0), &R[cur], NULL, 0, &POP[cur]);
    }

    ROUT = "init";
    DO2(ENT(LIB, INIT, 0), (void)0);

    /* nothing open */
    CALL0(CLOSECD, "CloseCD");
    CALL0(READTOC, "CDReadTOC");
    commands("not open");

    /* cd.device: not in the emulator (or no drive) */
    opencd(NULL, 0, 0, 0);
    printf("ricompactdisclib: OpenCD (cd.device) = %ld\n", (LONG)R[0].d[0]);
    if (R[0].d[0] == (ULONG)-1)
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

    /* OpenCD without arguments: rwecd.device unit 1 kept; the open
       request is lost (BUG(orig) (#578)) */
    ntracks = 9;
    opencd(NULL, 0, 0, 0);
    CALL0(READTOC, "CDReadTOC");
    tocfuncs(1, 3);
    CALL0(CLOSECD, "CloseCD");
    CALL0(CLOSECD, "CloseCD (again)");
    commands("closed");

    /* scsiflag: the SCSI path */
    opencd(rwecd, 2, 2, 1);
    commands("scsi");
    strcpy(VEC, "scsi");
    CALL0(READTOC, "CDReadTOC");
    CALL0(NUMTRACKS, "CDNumTracks");
    CALL0(FIRST, "CDFirstTrack");
    CALL0(LAST, "CDLastTrack");
    sprintf(VEC, "scsi 5,1");
    CALL2(PLAY, "CDPlayTrack", 5, 1);
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
    strcpy(VEC, "open");
    ROUT = "finit";
    DO2(ENT(LIB, FINIT, 0), (void)0);
    same_device();
    strcpy(VEC, "again");
    DO2(ENT(LIB, FINIT, 0), (void)0);
    same_device();
    ROUT = "init";
    strcpy(VEC, "again");
    DO2(ENT(LIB, INIT, 0), (void)0);
    ntracks = 9;
    opencd(NULL, 0, 0, 0);              /* the name is cd.device again */
    opencd(rwecd, 0, 1, 0);
    CALL0(READTOC, "CDReadTOC");
    tocfuncs(1, 2);
    CALL2(PLAY, "CDPlayTrack", 2, 1);   /* scsiflag 0 again */
    ROUT = "finit";
    DO2(ENT(LIB, FINIT, 0), (void)0);
    same_device();

    /* no request: CreateIORequest fails */
    ROUT = "init";
    DO2(ENT(LIB, INIT, 0), (void)0);
    Forbid();
    old = SetFunction((struct Library *)SysBase, -654, (APTR)rwe_fail0);
    CacheClearU();
    opencd(rwecd, 0, 1, 0);
    SetFunction((struct Library *)SysBase, -654, old);
    CacheClearU();
    Permit();
    commands("no request");
    ROUT = "finit";
    DO2(ENT(LIB, FINIT, 0), (void)0);

    /* no message port: CreateMsgPort fails in init */
    Forbid();
    old = SetFunction((struct Library *)SysBase, -666, (APTR)rwe_fail0);
    CacheClearU();
    ROUT = "init";
    strcpy(VEC, "no port");
    DO2(ENT(LIB, INIT, 0), (void)0);
    SetFunction((struct Library *)SysBase, -666, old);
    CacheClearU();
    Permit();
    opencd(rwecd, 0, 1, 0);
    commands("no port");
    ROUT = "finit";
    strcpy(VEC, "no port");
    DO2(ENT(LIB, FINIT, 0), (void)0);

    printf("rwecd.device: %lu requests, %lu opens, %lu failed, %lu aborts\n", last[0].n,
           last[0].opens, last[0].failed, last[0].aborts);
    check("rwecd.device used", last[0].n >= 50 && last[0].opens >= 5 && last[0].failed >= 1
          && last[1].n == last[0].n);

    /* the requests the library lost (OpenCD while open) */
    for (cur = 0; cur < 2; cur++)
        for (i = 0; i < MAXOPEN; i++)
            if (opened[cur][i]) {
                struct IORequest *ior = opened[cur][i];
                CloseDevice(ior);
                DeleteIORequest(ior);
            }
    ROUT = "memlib finit";
    for (cur = 0; cur < 2; cur++) {
        markregs(&R[cur]);
        callstk(entry_form(getlib(&sets[cur], "memlib"), 8, 0), &R[cur], NULL, 0, &POP[cur]);
    }
    rwe_mem_check(LIB, avail, TRUE);
    rem_device();
    return rwe_done(LIB);
}
