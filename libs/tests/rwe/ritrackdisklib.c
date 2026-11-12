/*
 * rwe ritrackdisklib - the C build of the RWE userlib ritrackdisklib
 * (libs/userlibs/rwe/ritrackdisklib.c) or the CD binary BSS2
 * RITRACKDISKLIB.OBJ (ritrackdisklib.orig), one set as LIBSET says,
 * observed as cases (sysload.h, LIBS_ONE_SET; the expected data
 * libs/tests/expected/rwe_ritrackdisklib.txt).
 *
 *   ritrackdisklib DIR     (DIR: ritrackdisklib.obj and .orig)
 *
 * The commands talk to a device of this test, "rwetest.device"
 * (OpenDisk unit,"rwetest.device"): a RAM disk of 32 sectors that logs
 * every request (command, flags, length, offset, buffer, unit, a
 * checksum of the data written) and fails OpenDevice for the units 2 and
 * -2.  Every command and form is called with its inputs (other registers
 * = markers); observed are d0, the request the device saw, the buffers
 * read and written (BootProg's checksum), the device's open/close counts
 * and the kept registers.  Covered: all commands before OpenDisk (0), OpenDisk
 * of units 5, 0-3, -1, -2 (BUG(orig) #861: the request of -1 is lost), the
 * name kept for later OpenDisks, a second OpenDisk of an open unit,
 * sector commands with and without count, errors of the device
 * (positive and negative io_Error), FormatTrack's track * 11 of the low
 * word, WriteBoot unit (the default boot block, observed: its 8 bytes)
 * and WriteBoot unit,bootdata (buffer = the caller's d2, BUG(orig), #864),
 * BootProg's checksum, DiskStatus/DiskIO, CloseDisk, finit (closes units
 * 0-2, not 3: BUG(orig), #863) and init again (the stale entry of unit 3).
 * Then a short real trackdisk.device run on the empty DF0: (OpenDisk,
 * MotorOn, DiskMotor, MotorOff, DiskMotor, ReadSector = "no disk",
 * CloseDisk).
 * Not covered: trackdisk.device with a disk in the drive (no floppy in
 * the emulator; the hardfiles are read-only), a device name longer than
 * the 58 byte buffer (overwrites the request table, BUG(orig), #860).
 */
#define LIBS_ONE_SET
#include "sysload.h"
#include <exec/devices.h>
#include <exec/io.h>
#include <exec/errors.h>
#include <devices/trackdisk.h>

static const char *names[] = { "ritrackdisklib", NULL };

enum { OPENDISK, MOTORON, CLOSEDISK, READSECTOR, MOTOROFF, WRITESECTOR, FORMATTRACK,
       WRITEBOOT, DISKSTATUS, DISKIO, BOOTPROG, DISKMOTOR, FINIT, INIT };

/* the registers of the call (R after it, IN before it); ROUT and VEC
   (the key of the cases) are sysload.h's */
static struct regs R, IN;

static struct lib *thelib(void)
{
    return getlib(&loaded, names[0]);
}

/* code of entry idx, form `form` (bit 0 = Amiga mode only cleared) */
static APTR ent(int idx, int form)
{
    return (APTR)((ULONG)entry_form(thelib(), idx, form) & ~1UL);
}

/* call entry idx, form `form`: markers in all registers, `setup` sets R;
   then the kept case */
#define DO1(idx, form, setup) do {                                          \
        markregs(&R);                                                       \
        setup;                                                              \
        IN = R;                                                             \
        callregs(ent(idx, form), &R, 0, 0);                                 \
        obs_kept(&IN, &R);                                                  \
    } while (0)

/* ------------------------------------------------------- the test device */

#define NSECT 32
#define DISKSIZE (NSECT * 512)
static UBYTE *disk;
static struct Device *dev;

/* the last request */
static struct req {
    ULONG n, opens, closes, failed;
    UWORD cmd;
    UBYTE flags;
    ULONG len, off, unit, sum, actual;
    APTR data;
    UBYTE head[8];
    struct MsgPort *reply;
} last;
static ULONG motor[4];
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
        last.failed++;
        return;
    }
    ior->io_Error = 0;
    ior->io_Device = d;
    ior->io_Unit = (struct Unit *)unit;
    d->dd_Library.lib_OpenCnt++;
    last.opens++;
    last.unit = unit;
}

static ULONG dev_close(struct IORequest *ior __asm("a1"), struct Device *d __asm("a6"))
{
    d->dd_Library.lib_OpenCnt--;
    last.closes++;
    last.unit = unitof(ior);
    return 0;
}

static ULONG dev_null(void)
{
    return 0;
}

static void dev_beginio(struct IOStdReq *ior __asm("a1"))
{
    struct req *l = &last;
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
            memcpy(ior->io_Data, disk + ior->io_Offset, ior->io_Length);
        } else {
            for (i = 0; i < ior->io_Length; i++)
                l->sum = l->sum * 31 + ((UBYTE *)ior->io_Data)[i];
            memcpy(l->head, ior->io_Data, 8);
            /* the default boot block of WriteBoot unit is 8 bytes and
               then whatever follows it in the library: only those 8 */
            memcpy(disk + ior->io_Offset, ior->io_Data,
                   relbuf(ior->io_Data) == 0x40000 ? 8 : ior->io_Length);
        }
        ior->io_Actual = ior->io_Length;
        break;
    case TD_MOTOR:
        if (l->unit >= 0 && l->unit < 4) {
            ior->io_Actual = motor[l->unit];
            motor[l->unit] = ior->io_Length;
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

static UBYTE *buf, *rbuf, *boot;
static ULONG reqaddr[4];
#define BUFSIZE 4096

/* a buffer address as a number that does not depend on the layout */
static ULONG relbuf(APTR p)
{
    ULONG a = (ULONG)p;
    struct lib *l = thelib();
    if (!a)
        return 0;
    if (a >= (ULONG)buf && a < (ULONG)buf + BUFSIZE)
        return 0x10000 + a - (ULONG)buf;
    if (a >= (ULONG)rbuf && a < (ULONG)rbuf + BUFSIZE)
        return 0x20000 + a - (ULONG)rbuf;
    if (a >= (ULONG)boot && a < (ULONG)boot + 1024)
        return 0x30000 + a - (ULONG)boot;
    if (a >= (ULONG)l->code && a < (ULONG)l->code + l->size)
        return 0x40000;                 /* inside the library */
    return 0xdead0000;
}

static void same_device(int data)
{
    OBS("requests", last.n);
    OBS("opens", last.opens);
    OBS("closes", last.closes);
    OBS("failed opens", last.failed);
    OBS("command", last.cmd);
    OBS("flags", last.flags);
    OBS("length", last.len);
    OBS("offset", last.off);
    OBS("unit", last.unit);
    OBS("actual", last.actual);
    OBS("buffer", relbuf(last.data));
    OBSBYTES("head", last.head, 8);
    if (data)
        OBS("data written", last.sum);
    OBSBYTES("disk", disk, DISKSIZE);
    OBSBYTES("read buffer", rbuf, BUFSIZE);
}

static void fill(UBYTE *b, ULONG size, ULONG seed)
{
    ULONG i;
    for (i = 0; i < size; i++)
        b[i] = (UBYTE)((i * 13 + seed) ^ (i >> 8));
}

/* d0 of a command; a request address as 1 */
static ULONG result(void)
{
    ULONG d0 = R.d[0];
    int u;
    for (u = 0; u < 4; u++)
        if (d0 && d0 == reqaddr[u])
            return 0x100 + u;
    return d0;
}

static void opendisk(LONG unit, const char *name)
{
    ROUT = "OpenDisk";
    sprintf(VEC, "%ld,%s", unit, name ? name : "-");
    if (name)
        DO1(OPENDISK, 1, R.d[0] = unit; R.d[1] = (ULONG)name);
    else
        DO1(OPENDISK, 0, R.d[0] = unit);
    OBS("d0", R.d[0]);
    same_device(0);
}

static void unitcmd(int idx, const char *name, LONG unit)
{
    ROUT = name;
    sprintf(VEC, "%ld", unit);
    DO1(idx, 0, R.d[0] = unit);
    OBS("d0", result());
    same_device(0);
}

/* DiskIO unit: remembers the request */
static void diskio(LONG unit)
{
    ROUT = "DiskIO";
    sprintf(VEC, "%ld", unit);
    DO1(DISKIO, 0, R.d[0] = unit);
    OBS("d0 != 0", R.d[0] != 0);
    if (unit >= 0 && unit < 4)
        reqaddr[unit] = R.d[0];
    if (R.d[0] && unit >= 0 && unit < 4) {
        OBS("io_Device", ((struct IORequest *)R.d[0])->io_Device == dev);
        OBS("io_Unit", (ULONG)((struct IORequest *)R.d[0])->io_Unit);
        OBS("io_Error", ((struct IORequest *)R.d[0])->io_Error);
        OBS("reply port = port of init", ((struct Message *)R.d[0])->mn_ReplyPort != NULL);
    }
}

/* sector command: form 0 = without count */
static void sector(int idx, const char *name, LONG unit, ULONG sec, UBYTE *b, ULONG off,
                   int form, ULONG num)
{
    ROUT = name;
    sprintf(VEC, "%ld,%lu,+%lu,%lu (form %d)", unit, sec, off, num, form);
    DO1(idx, form, R.d[0] = unit; R.d[1] = sec; R.d[2] = (ULONG)b + off;
        R.d[3] = num);
    OBS("d0", R.d[0]);
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
    if (!load_set(argv[1], names))
        return 20;
    KEEPS("init", d2-d7|a2|a3);
    KEEPS("CloseDisk", d2-d6|a2|a3);
    KEEPS("MotorOn", d2-d6|a2|a3);
    KEEPS("MotorOff", d0|d2-d6|a2|a3);
    KEEPS("DiskStatus", d1-d7|a2|a3);
    KEEPS("DiskIO", d1-d7|a1-a3);
    KEEPS("ReadSector", d2|d6|a2|a3);
    KEEPS("WriteSector", d2|d6|a2|a3);
    KEEPS("OpenDisk", d2-d5|a2|a3);
    KEEPS("FormatTrack", d0|d2|d5|d6|a2|a3);
    KEEPS("WriteBoot", d0|d5|d6|a2|a3);
    KEEPS("BootProg", d0|d6|a2|a3);
    KEEPS("BootProg (no request)", d6|d7|a2|a3);
    KEEPS("DiskMotor", d1-d7|a0-a3|a6);
    KEEPS("finit", d2-d6|a2|a3);
    KEEPS("DiskIO (after finit)", d1-d7|a0-a3);
    KEEPS("ReadSector (after finit)", d0-d2|d6|d7|a0-a3);
    KEEPS("DiskIO (init again: unit 3 stale)", d1-d7|a1-a3);
    KEEPS("MotorOn (trackdisk)", d0|d2-d6|a2|a3);
    KEEPS("DiskMotor (motor on)", d1-d7|a0-a3|a6);
    KEEPS("MotorOff (trackdisk)", d0|d2-d6|a2|a3);
    KEEPS("DiskMotor (motor off)", d1-d7|a0-a3|a6);
    KEEPS("ReadSector (trackdisk, no disk)", d1|d2|d6|a2|a3);
    KEEPS("DiskStatus (trackdisk)", d1-d7|a2|a3);
    KEEPS("CloseDisk (trackdisk)", d2-d6|a2|a3);
    if (!add_device()) {
        check("MakeLibrary rwetest.device", FALSE);
        return 20;
    }
    disk = AllocMem(DISKSIZE, MEMF_PUBLIC | MEMF_CLEAR);
    buf = AllocMem(BUFSIZE, MEMF_PUBLIC);
    rbuf = AllocMem(BUFSIZE, MEMF_PUBLIC | MEMF_CLEAR);
    boot = AllocMem(1024, MEMF_PUBLIC);
    fill(disk, DISKSIZE, 5);
    fill(buf, BUFSIZE, 77);

    ROUT = "init";
    strcpy(VEC, "");
    DO1(INIT, 0, (void)0);

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
    DO1(WRITEBOOT, 0, R.d[0] = 0);
    OBS("d0", R.d[0]);
    same_device(0);
    OBS("default boot block", memcmp(last.head, "DOS\0\377\377\377\377", 8) == 0);
    /* WriteBoot unit,bootdata: the buffer is d2 */
    strcpy(VEC, "0,bootdata (d2 = buf+512)");
    DO1(WRITEBOOT, 1, R.d[0] = 0; R.d[1] = (ULONG)boot;
        R.d[2] = (ULONG)buf + 512);
    OBS("d0", R.d[0]);
    same_device(1);

    /* BootProg */
    for (i = 0; i < 2; i++) {
        fill(boot, 1024, 3 + i * 100);
        ROUT = "BootProg";
        sprintf(VEC, "0,boot %d", i);
        DO1(BOOTPROG, 0, R.d[0] = 0; R.d[1] = (ULONG)boot);
        OBS("d0", R.d[0]);
        OBSBYTES("boot block", boot, 1024);
        OBS("checksum", ((ULONG *)boot)[1]);
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
    /* a second OpenDisk of unit 1: the old request is lost (BUG(orig), #862) */
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
    DO1(BOOTPROG, 0, R.d[0] = 7; R.d[1] = (ULONG)boot);
    OBS("d0", R.d[0]);
    OBSBYTES("boot block", boot, 1024);
    same_device(0);
    ROUT = "DiskMotor";
    strcpy(VEC, "");
    DO1(DISKMOTOR, 0, (void)0);
    OBS("d0", R.d[0]);

    printf("rwetest.device: %lu requests, %lu opens, %lu failed\n", last.n, last.opens,
           last.failed);
    check("rwetest.device used", last.n >= 20 && last.opens >= 5 && last.failed >= 2);
    unitcmd(CLOSEDISK, "CloseDisk", 1);
    unitcmd(CLOSEDISK, "CloseDisk", 1);
    unitcmd(CLOSEDISK, "CloseDisk", 9);
    unitcmd(CLOSEDISK, "CloseDisk", -1);
    diskio(3);

    /* finit closes 0-2, unit 3 stays open */
    ROUT = "finit";
    strcpy(VEC, "");
    DO1(FINIT, 0, (void)0);
    same_device(0);
    unitcmd(DISKIO, "DiskIO (after finit)", 3);
    sector(READSECTOR, "ReadSector (after finit)", 0, 0, rbuf, 0, 0, 0);
    ROUT = "init";
    DO1(INIT, 0, (void)0);
    unitcmd(DISKIO, "DiskIO (init again: unit 3 stale)", 3);
    ROUT = "finit";
    DO1(FINIT, 0, (void)0);
    CloseDevice((struct IORequest *)reqaddr[3]);
    DeleteIORequest((struct IORequest *)reqaddr[3]);

    /* the real trackdisk.device on the empty DF0: */
    ROUT = "init";
    DO1(INIT, 0, (void)0);
    opendisk(0, td);
    printf("trackdisk.device unit 0: OpenDisk %ld\n", (LONG)R.d[0]);
    diskio(0);
    unitcmd(MOTORON, "MotorOn (trackdisk)", 0);
    ROUT = "DiskMotor (motor on)";
    DO1(DISKMOTOR, 0, (void)0);
    OBS("d0", R.d[0]);
    unitcmd(MOTOROFF, "MotorOff (trackdisk)", 0);
    ROUT = "DiskMotor (motor off)";
    DO1(DISKMOTOR, 0, (void)0);
    OBS("d0", R.d[0]);
    ROUT = "ReadSector (trackdisk, no disk)";
    strcpy(VEC, "0,0");
    DO1(READSECTOR, 0, R.d[0] = 0; R.d[1] = 0; R.d[2] = (ULONG)rbuf);
    OBS("d0", R.d[0]);
    printf("trackdisk.device unit 0: ReadSector %ld\n", (LONG)R.d[0]);
    unitcmd(DISKSTATUS, "DiskStatus (trackdisk)", 0);
    unitcmd(CLOSEDISK, "CloseDisk (trackdisk)", 0);
    ROUT = "finit";
    DO1(FINIT, 0, (void)0);

    rem_device();
    FreeMem(disk, DISKSIZE);
    FreeMem(buf, BUFSIZE);
    FreeMem(rbuf, BUFSIZE);
    FreeMem(boot, 1024);
    printf("%d checks, %d failures\n", checks, failures);
    kept_summary();
    return failures ? 5 : 0;
}
