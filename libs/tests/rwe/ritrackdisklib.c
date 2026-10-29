/*
 * rwe ritrackdisklib - the C build of the RWE userlib ritrackdisklib
 * (libs/userlibs/rwe/ritrackdisklib.c) against the CD binary BSS2
 * RITRACKDISKLIB.OBJ (ritrackdisklib.orig).
 *
 *   ritrackdisklib DIR     (DIR: ritrackdisklib.obj and .orig)
 *
 * The commands talk to a device of this test, "rwetest.device"
 * (OpenDisk unit,"rwetest.device"): a RAM disk of 32 sectors per set
 * that logs every request (command, flags, length, offset, buffer,
 * unit, a checksum of the data written) and fails OpenDevice for the
 * units 2 and -2.  Every command and form is called on both sets with
 * the same inputs (other registers = markers); compared are d0, the
 * request the device saw, the buffers read and written (BootProg's
 * checksum), the device's open/close counts and every register the
 * original keeps.  Covered: all commands before OpenDisk (0), OpenDisk
 * of units 5, 0-3, -1, -2 (BUG(orig): the request of -1 is lost), the
 * name kept for later OpenDisks, a second OpenDisk of an open unit,
 * sector commands with and without count, errors of the device
 * (positive and negative io_Error), FormatTrack's track * 11 of the low
 * word, WriteBoot unit (the default boot block, compared: its 8 bytes)
 * and WriteBoot unit,bootdata (buffer = the caller's d2, BUG(orig)),
 * BootProg's checksum, DiskStatus/DiskIO, CloseDisk, finit (closes units
 * 0-2, not 3: BUG(orig)) and init again (the stale entry of unit 3).
 * Then a short real trackdisk.device run on the empty DF0: (OpenDisk,
 * MotorOn, DiskMotor, MotorOff, DiskMotor, ReadSector = "no disk",
 * CloseDisk).
 * Not covered: trackdisk.device with a disk in the drive (no floppy in
 * the emulator; the hardfiles are read-only), a device name longer than
 * the 58 byte buffer (overwrites the request table, BUG(orig)).
 */
#include "sysload.h"
#include <exec/devices.h>
#include <exec/io.h>
#include <exec/errors.h>
#include <devices/trackdisk.h>

static const char *names[] = { "ritrackdisklib", NULL };

enum { OPENDISK, MOTORON, CLOSEDISK, READSECTOR, MOTOROFF, WRITESECTOR, FORMATTRACK,
       WRITEBOOT, DISKSTATUS, DISKIO, BOOTPROG, DISKMOTOR, FINIT, INIT };

static int cur;
static struct regs R[2], IN[2];
static const char *ROUT;
static char VEC[100];
static int prints;

static void fail(const char *what, ULONG o, ULONG c)
{
    char buf[200];
    if (prints++ < 60)
        printf("FAIL %s %s %s: orig $%08lX, C $%08lX\n", ROUT, VEC, what, o, c);
    snprintf(buf, sizeof buf, "%s %s %s", ROUT, VEC, what);
    check(buf, FALSE);
}

#define SAME(what, expr) do {                                               \
        ULONG v_[2];                                                        \
        for (cur = 0; cur < 2; cur++) v_[cur] = (ULONG)(expr);              \
        if (v_[0] != v_[1]) fail(what, v_[0], v_[1]); else checks++;        \
    } while (0)

static void preserve(void)
{
    struct call c[2];
    char buf[140];
    int s;
    for (s = 0; s < 2; s++) {
        c[s].in = IN[s];
        c[s].out = R[s];
    }
    sprintf(buf, "%s %s", ROUT, VEC);
    cmp_preserve(buf, &c[0], &c[1]);
}

static struct lib *thelib(void)
{
    return getlib(&sets[cur], names[0]);
}

/* code of entry idx, form `form` (bit 0 = Amiga mode only cleared) */
static APTR ent(int idx, int form)
{
    return (APTR)((ULONG)entry_form(thelib(), idx, form) & ~1UL);
}

#define DO2(idx, form, setup) do {                                          \
        for (cur = 0; cur < 2; cur++) {                                     \
            markregs(&R[cur]);                                              \
            setup;                                                          \
            IN[cur] = R[cur];                                               \
            callregs(ent(idx, form), &R[cur], 0, 0);                        \
        }                                                                   \
        preserve();                                                         \
    } while (0)

/* ------------------------------------------------------- the test device */

#define NSECT 32
#define DISKSIZE (NSECT * 512)
static UBYTE *disk[2];
static struct Device *dev;

/* the last request per set */
static struct req {
    ULONG n, opens, closes, failed;
    UWORD cmd;
    UBYTE flags;
    ULONG len, off, unit, sum, actual;
    APTR data;
    UBYTE head[8];
    struct MsgPort *reply;
} last[2];
static ULONG motor[2][4];
static ULONG relbuf(APTR p);

static LONG unitof(struct IORequest *ior)
{
    return (LONG)ior->io_Unit;
}

static void dev_open(struct IORequest *ior __asm("a1"), LONG unit __asm("d0"),
                     ULONG flags __asm("d1"), struct Device *d __asm("a6"))
{
    (void)flags;
    if (unit == 2 || unit == -2) {
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
}

static ULONG dev_close(struct IORequest *ior __asm("a1"), struct Device *d __asm("a6"))
{
    d->dd_Library.lib_OpenCnt--;
    last[cur].closes++;
    last[cur].unit = unitof(ior);
    return 0;
}

static ULONG dev_null(void)
{
    return 0;
}

static void dev_beginio(struct IOStdReq *ior __asm("a1"))
{
    struct req *l = &last[cur];
    ULONG i;

    l->n++;
    l->cmd = ior->io_Command;
    l->flags = ior->io_Flags;
    l->len = ior->io_Length;
    l->off = ior->io_Offset;
    l->data = ior->io_Data;
    l->unit = unitof((struct IORequest *)ior);
    l->reply = ior->io_Message.mn_ReplyPort;
    l->sum = 0;
    memset(l->head, 0, sizeof l->head);
    ior->io_Error = 0;
    ior->io_Actual = 0;
    switch (ior->io_Command) {
    case CMD_READ:
    case CMD_WRITE:
    case TD_FORMAT:
        if (!ior->io_Length) {
            ior->io_Error = IOERR_BADLENGTH;
            break;
        }
        if (ior->io_Offset + ior->io_Length > DISKSIZE) {
            ior->io_Error = TDERR_SeekError;
            break;
        }
        if (ior->io_Command == CMD_READ) {
            memcpy(ior->io_Data, disk[cur] + ior->io_Offset, ior->io_Length);
        } else {
            for (i = 0; i < ior->io_Length; i++)
                l->sum = l->sum * 31 + ((UBYTE *)ior->io_Data)[i];
            memcpy(l->head, ior->io_Data, 8);
            /* the default boot block of WriteBoot unit is 8 bytes and
               then whatever follows it in the library: only those 8 */
            memcpy(disk[cur] + ior->io_Offset, ior->io_Data,
                   relbuf(ior->io_Data) == 0x40000 ? 8 : ior->io_Length);
        }
        ior->io_Actual = ior->io_Length;
        break;
    case TD_MOTOR:
        if (l->unit >= 0 && l->unit < 4) {
            ior->io_Actual = motor[cur][l->unit];
            motor[cur][l->unit] = ior->io_Length;
        }
        break;
    default:
        ior->io_Error = IOERR_NOCMD;
        break;
    }
    l->actual = ior->io_Actual;
    if (!(ior->io_Flags & IOF_QUICK))
        ReplyMsg(&ior->io_Message);
}

static const APTR devfuncs[] = { (APTR)dev_open, (APTR)dev_close, (APTR)dev_null,
                                 (APTR)dev_null, (APTR)dev_beginio, (APTR)dev_null, (APTR)-1 };

static BOOL add_device(void)
{
    dev = (struct Device *)MakeLibrary((CONST_APTR)devfuncs, NULL, NULL, sizeof(struct Device),
                                       0);
    if (!dev)
        return FALSE;
    dev->dd_Library.lib_Node.ln_Type = NT_DEVICE;
    dev->dd_Library.lib_Node.ln_Name = (char *)"rwetest.device";
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

static UBYTE *buf[2], *rbuf[2], *boot[2];
static ULONG reqaddr[2][4];
#define BUFSIZE 4096

/* a buffer address of the set as a number that is the same in both */
static ULONG relbuf(APTR p)
{
    ULONG a = (ULONG)p;
    struct lib *l = thelib();
    if (!a)
        return 0;
    if (a >= (ULONG)buf[cur] && a < (ULONG)buf[cur] + BUFSIZE)
        return 0x10000 + a - (ULONG)buf[cur];
    if (a >= (ULONG)rbuf[cur] && a < (ULONG)rbuf[cur] + BUFSIZE)
        return 0x20000 + a - (ULONG)rbuf[cur];
    if (a >= (ULONG)boot[cur] && a < (ULONG)boot[cur] + 1024)
        return 0x30000 + a - (ULONG)boot[cur];
    if (a >= (ULONG)l->code && a < (ULONG)l->code + l->size)
        return 0x40000;                 /* inside the library */
    return 0xdead0000;
}

static void same_device(int data)
{
    SAME("requests", last[cur].n);
    SAME("opens", last[cur].opens);
    SAME("closes", last[cur].closes);
    SAME("failed opens", last[cur].failed);
    SAME("command", last[cur].cmd);
    SAME("flags", last[cur].flags);
    SAME("length", last[cur].len);
    SAME("offset", last[cur].off);
    SAME("unit", last[cur].unit);
    SAME("actual", last[cur].actual);
    SAME("buffer", relbuf(last[cur].data));
    SAME("head", memcmp(last[0].head, last[1].head, 8) == 0);
    if (data)
        SAME("data written", last[cur].sum);
    SAME("disk", memcmp(disk[0], disk[1], DISKSIZE) == 0);
    SAME("read buffer", memcmp(rbuf[0], rbuf[1], BUFSIZE) == 0);
}

static void fill(UBYTE **b, ULONG size, ULONG seed)
{
    ULONG i;
    for (cur = 0; cur < 2; cur++)
        for (i = 0; i < size; i++)
            b[cur][i] = (UBYTE)((i * 13 + seed) ^ (i >> 8));
}

/* d0 of a command; a request address as 1 */
static ULONG result(void)
{
    ULONG d0 = R[cur].d[0];
    int u;
    for (u = 0; u < 4; u++)
        if (d0 && d0 == reqaddr[cur][u])
            return 0x100 + u;
    return d0;
}

static void opendisk(LONG unit, const char *name)
{
    ROUT = "OpenDisk";
    sprintf(VEC, "%ld,%s", unit, name ? name : "-");
    if (name)
        DO2(OPENDISK, 1, R[cur].d[0] = unit; R[cur].d[1] = (ULONG)name);
    else
        DO2(OPENDISK, 0, R[cur].d[0] = unit);
    SAME("d0", R[cur].d[0]);
    same_device(0);
}

static void unitcmd(int idx, const char *name, LONG unit)
{
    ROUT = name;
    sprintf(VEC, "%ld", unit);
    DO2(idx, 0, R[cur].d[0] = unit);
    SAME("d0", result());
    same_device(0);
}

/* DiskIO unit: remembers the request of the set */
static void diskio(LONG unit)
{
    ROUT = "DiskIO";
    sprintf(VEC, "%ld", unit);
    DO2(DISKIO, 0, R[cur].d[0] = unit);
    SAME("d0 != 0", R[cur].d[0] != 0);
    if (unit >= 0 && unit < 4)
        for (cur = 0; cur < 2; cur++)
            reqaddr[cur][unit] = R[cur].d[0];
    if (R[0].d[0] && R[1].d[0] && unit >= 0 && unit < 4) {
        SAME("io_Device", ((struct IORequest *)R[cur].d[0])->io_Device == dev);
        SAME("io_Unit", (ULONG)((struct IORequest *)R[cur].d[0])->io_Unit);
        SAME("io_Error", ((struct IORequest *)R[cur].d[0])->io_Error);
        SAME("reply port = port of init", ((struct Message *)R[cur].d[0])->mn_ReplyPort != NULL);
    }
}

/* sector command: form 0 = without count */
static void sector(int idx, const char *name, LONG unit, ULONG sec, UBYTE **b, ULONG off,
                   int form, ULONG num)
{
    ROUT = name;
    sprintf(VEC, "%ld,%lu,+%lu,%lu (form %d)", unit, sec, off, num, form);
    DO2(idx, form, R[cur].d[0] = unit; R[cur].d[1] = sec; R[cur].d[2] = (ULONG)b[cur] + off;
        R[cur].d[3] = num);
    SAME("d0", R[cur].d[0]);
    same_device(idx != READSECTOR);
}

int main(int argc, char **argv)
{
    static const char rwe[] = "rwetest.device", td[] = "trackdisk.device";
    int i;

    if (argc < 2) {
        printf("usage: ritrackdisklib DIR\n");
        return 20;
    }
    if (!load_sets(argv[1], names))
        return 20;
    if (!add_device()) {
        check("MakeLibrary rwetest.device", FALSE);
        return 20;
    }
    for (cur = 0; cur < 2; cur++) {
        disk[cur] = AllocMem(DISKSIZE, MEMF_PUBLIC | MEMF_CLEAR);
        buf[cur] = AllocMem(BUFSIZE, MEMF_PUBLIC);
        rbuf[cur] = AllocMem(BUFSIZE, MEMF_PUBLIC | MEMF_CLEAR);
        boot[cur] = AllocMem(1024, MEMF_PUBLIC);
    }
    fill(disk, DISKSIZE, 5);
    fill(buf, BUFSIZE, 77);

    ROUT = "init";
    strcpy(VEC, "");
    DO2(INIT, 0, (void)0);

    /* nothing open */
    unitcmd(CLOSEDISK, "CloseDisk", 0);
    unitcmd(MOTORON, "MotorOn", 0);
    unitcmd(MOTOROFF, "MotorOff", 0);
    unitcmd(DISKSTATUS, "DiskStatus", 0);
    diskio(0);
    sector(READSECTOR, "ReadSector", 0, 1, rbuf, 0, 0, 0);
    sector(WRITESECTOR, "WriteSector", 0, 1, buf, 0, 1, 1);
    opendisk(5, NULL);
    opendisk(4, rwe);

    /* unit 0 on the test device */
    opendisk(0, rwe);
    diskio(0);
    unitcmd(DISKSTATUS, "DiskStatus", 0);
    sector(WRITESECTOR, "WriteSector", 0, 3, buf, 0, 0, 0);
    sector(WRITESECTOR, "WriteSector", 0, 4, buf, 512, 1, 3);
    sector(READSECTOR, "ReadSector", 0, 3, rbuf, 0, 0, 0);
    sector(READSECTOR, "ReadSector", 0, 2, rbuf, 100, 1, 5);
    sector(READSECTOR, "ReadSector", 0, 100, rbuf, 0, 0, 0);       /* error 28 */
    sector(READSECTOR, "ReadSector", 0, 1, rbuf, 0, 1, 0);         /* error -4 */
    sector(READSECTOR, "ReadSector", 0, 0x00800003UL, rbuf, 0, 0, 0);
    unitcmd(DISKSTATUS, "DiskStatus", 0);
    sector(FORMATTRACK, "FormatTrack", 0, 1, buf, 1024, 0, 0);
    sector(FORMATTRACK, "FormatTrack", 0, 0x10001UL, buf, 0, 1, 2);
    sector(FORMATTRACK, "FormatTrack", 0, 2, buf, 0, 1, 1);
    sector(READSECTOR, "ReadSector", 0, 0, rbuf, 0, 1, 8);
    unitcmd(MOTORON, "MotorOn", 0);
    unitcmd(MOTORON, "MotorOn", 0);
    unitcmd(MOTOROFF, "MotorOff", 0);

    /* WriteBoot unit: the default boot block */
    ROUT = "WriteBoot";
    strcpy(VEC, "0");
    DO2(WRITEBOOT, 0, R[cur].d[0] = 0);
    SAME("d0", R[cur].d[0]);
    same_device(0);
    SAME("default boot block", memcmp(last[cur].head, "DOS\0\377\377\377\377", 8) == 0);
    /* WriteBoot unit,bootdata: the buffer is d2 */
    strcpy(VEC, "0,bootdata (d2 = buf+512)");
    DO2(WRITEBOOT, 1, R[cur].d[0] = 0; R[cur].d[1] = (ULONG)boot[cur];
        R[cur].d[2] = (ULONG)buf[cur] + 512);
    SAME("d0", R[cur].d[0]);
    same_device(1);

    /* BootProg */
    for (i = 0; i < 2; i++) {
        fill(boot, 1024, 3 + i * 100);
        ROUT = "BootProg";
        sprintf(VEC, "0,boot %d", i);
        DO2(BOOTPROG, 0, R[cur].d[0] = 0; R[cur].d[1] = (ULONG)boot[cur]);
        SAME("d0", R[cur].d[0]);
        SAME("boot block", memcmp(boot[0], boot[1], 1024) == 0);
        SAME("checksum", ((ULONG *)boot[cur])[1]);
        same_device(1);
    }
    sector(READSECTOR, "ReadSector", 0, 0, rbuf, 0, 1, 2);

    /* units 1-3 (the name of the last OpenDisk stays), 2 fails */
    opendisk(1, NULL);
    diskio(1);
    opendisk(2, NULL);
    diskio(2);
    opendisk(3, NULL);
    diskio(3);
    sector(WRITESECTOR, "WriteSector", 3, 7, buf, 2048, 1, 2);
    sector(READSECTOR, "ReadSector", 1, 7, rbuf, 0, 1, 2);
    unitcmd(MOTORON, "MotorOn", 3);
    /* a second OpenDisk of unit 1: the old request is lost (BUG(orig)) */
    opendisk(1, rwe);
    diskio(1);
    /* negative units: OpenDevice ok (request lost) / failed */
    opendisk(-1, NULL);
    opendisk(-2, NULL);
    diskio(-1);
    unitcmd(MOTORON, "MotorOn", -1);
    sector(READSECTOR, "ReadSector", -1, 0, rbuf, 0, 0, 0);
    ROUT = "BootProg (no request)";
    strcpy(VEC, "7,boot");
    DO2(BOOTPROG, 0, R[cur].d[0] = 7; R[cur].d[1] = (ULONG)boot[cur]);
    SAME("d0", R[cur].d[0]);
    SAME("boot block", memcmp(boot[0], boot[1], 1024) == 0);
    same_device(0);
    ROUT = "DiskMotor";
    strcpy(VEC, "");
    DO2(DISKMOTOR, 0, (void)0);
    SAME("d0", R[cur].d[0]);

    printf("rwetest.device: %lu requests, %lu opens, %lu failed\n", last[0].n, last[0].opens,
           last[0].failed);
    check("rwetest.device used", last[0].n >= 20 && last[0].opens >= 5 && last[0].failed >= 2
          && last[1].n == last[0].n);
    unitcmd(CLOSEDISK, "CloseDisk", 1);
    unitcmd(CLOSEDISK, "CloseDisk", 1);
    unitcmd(CLOSEDISK, "CloseDisk", 9);
    unitcmd(CLOSEDISK, "CloseDisk", -1);
    diskio(3);

    /* finit closes 0-2, unit 3 stays open */
    ROUT = "finit";
    strcpy(VEC, "");
    DO2(FINIT, 0, (void)0);
    same_device(0);
    unitcmd(DISKIO, "DiskIO (after finit)", 3);
    sector(READSECTOR, "ReadSector (after finit)", 0, 0, rbuf, 0, 0, 0);
    ROUT = "init";
    DO2(INIT, 0, (void)0);
    unitcmd(DISKIO, "DiskIO (init again: unit 3 stale)", 3);
    ROUT = "finit";
    DO2(FINIT, 0, (void)0);
    for (cur = 0; cur < 2; cur++) {
        CloseDevice((struct IORequest *)reqaddr[cur][3]);
        DeleteIORequest((struct IORequest *)reqaddr[cur][3]);
    }

    /* the real trackdisk.device on the empty DF0: */
    ROUT = "init";
    DO2(INIT, 0, (void)0);
    opendisk(0, td);
    printf("trackdisk.device unit 0: OpenDisk %ld\n", (LONG)R[0].d[0]);
    diskio(0);
    unitcmd(MOTORON, "MotorOn (trackdisk)", 0);
    ROUT = "DiskMotor (motor on)";
    DO2(DISKMOTOR, 0, (void)0);
    SAME("d0", R[cur].d[0]);
    unitcmd(MOTOROFF, "MotorOff (trackdisk)", 0);
    ROUT = "DiskMotor (motor off)";
    DO2(DISKMOTOR, 0, (void)0);
    SAME("d0", R[cur].d[0]);
    ROUT = "ReadSector (trackdisk, no disk)";
    strcpy(VEC, "0,0");
    DO2(READSECTOR, 0, R[cur].d[0] = 0; R[cur].d[1] = 0; R[cur].d[2] = (ULONG)rbuf[cur]);
    SAME("d0", R[cur].d[0]);
    printf("trackdisk.device unit 0: ReadSector %ld\n", (LONG)R[0].d[0]);
    unitcmd(DISKSTATUS, "DiskStatus (trackdisk)", 0);
    unitcmd(CLOSEDISK, "CloseDisk (trackdisk)", 0);
    ROUT = "finit";
    DO2(FINIT, 0, (void)0);

    rem_device();
    for (cur = 0; cur < 2; cur++) {
        FreeMem(disk[cur], DISKSIZE);
        FreeMem(buf[cur], BUFSIZE);
        FreeMem(rbuf[cur], BUFSIZE);
        FreeMem(boot[cur], 1024);
    }
    printf("%d checks, %d failures\n", checks, failures);
    return failures ? 5 : 0;
}
