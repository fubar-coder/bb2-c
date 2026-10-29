/*
 * rwe ridoslib - AddNotify, RemNotify, CheckNotify, WaitNotify,
 * GetDeviceList, NextDevice, FreeDeviceList, NotifyPort, GetFileName,
 * GetPathName, DeviceEntryType: the C build against the CD original
 * (BSS2 RIDOSLIB.OBJ), method of rwecmp.h.
 *
 *   ridoslib DIR
 *
 * Covered: every routine before init (no port, no dos base: 0 / d0
 * unchanged), init, NotifyPort; GetFileName/GetPathName with many paths
 * (separators first/last/doubled, none, empty, a length whose high word
 * is set) - d0, a3 and the result; notifications on a file in RAM: (ids
 * 5, 9 and 40, each set with its own port: CheckNotify until empty after
 * every write, WaitNotify, RemNotify twice and with id -1, a failing
 * StartNotify on a missing device, one request left for finit);
 * GetDeviceList (devices, volumes, assigns) and NextDevice to the end
 * with DeviceEntryType after each, NextDevice/DeviceEntryType after
 * FreeDeviceList (stale entry, BUG(orig)); finit (ends the requests left;
 * the failed request stays allocated, BUG(orig)).  Each set its own state,
 * the system the same: the results must be equal.
 * Not covered: ids outside 0..63 (they write outside the tables), names
 * longer than tbuf.
 */
#include "rwecmp.h"
#include <proto/dos.h>
#include <dos/dosextens.h>

#define LIB "ridoslib"
enum { E_ADD, E_REM, E_CHECK, E_WAIT, E_LOCK, E_NEXT, E_FREE, E_PORT, E_FILE, E_PATH,
       E_TYPE, E_INIT, E_FINIT };

#define TESTFILE "RAM:ridoslib-notify"

static UBYTE out[2][300];
static ULONG frame[2][2];
static struct DosLibrary *dos;

/* string argument at out+20 (where the compiler built it), a3 behind it */
#define STRARG(s, len) do {                                                 \
        memset(out[cur], 0xee, sizeof out[0]);                              \
        strcpy((char *)out[cur] + 20, s);                                   \
        R[cur].d[0] = (ULONG)out[cur] + 20;                                 \
        frame[cur][0] = (len);                                              \
        R[cur].a[2] = (ULONG)&frame[cur][1];                                \
        R[cur].a[3] = (ULONG)out[cur] + 20 + strlen(s) + 1;                 \
    } while (0)

#define STRRES() do {                                                       \
        SAME("d0", R[cur].d[0]);                                            \
        SAME("a3", R[cur].a[3] - (ULONG)out[cur]);                          \
        SAME("out", memcmp(out[0], out[1], sizeof out[0]));                 \
    } while (0)

static void paths(void)
{
    static const char *p[] = {
        "", "a", "/", ":", "abc", "/abc", ":x", "a/", "a:", "a/b", "dh0:", "dh0:x",
        "DH0:dir/file.txt", "a//b", "ram:t/", "x:y:z", "Work:a/b/c/d.info", "//",
        "name with spaces/and more",
    };
    unsigned i;
    int f;
    for (f = 0; f < 2; f++) {
        ROUT = f ? "GetPathName" : "GetFileName";
        for (i = 0; i < sizeof p / sizeof p[0]; i++) {
            sprintf(VEC, "\"%s\"", p[i]);
            DO2(ENT(LIB, f ? E_PATH : E_FILE, 0), STRARG(p[i], strlen(p[i])));
            STRRES();
        }
        strcpy(VEC, "\"abc/de\" length $10006");
        DO2(ENT(LIB, f ? E_PATH : E_FILE, 0), STRARG("abc/de", 0x10006));
        STRRES();
    }
}

static void touch(void)
{
    BPTR fh = Open((STRPTR)TESTFILE, MODE_NEWFILE);
    if (fh) {
        Write(fh, (APTR)"x", 1);
        Close(fh);
    }
    Delay(10);
}

/* CheckNotify until 0: the ids in order */
static int drain(const char *what)
{
    ULONG ids[2][8];
    int n[2], k;
    for (cur = 0; cur < 2; cur++)
        n[cur] = 0;
    ROUT = "CheckNotify";
    strcpy(VEC, what);
    for (k = 0; k < 8; k++) {
        DO2(ENT(LIB, E_CHECK, 0), (void)0);
        for (cur = 0; cur < 2; cur++)
            if (R[cur].d[0])
                ids[cur][n[cur]++] = R[cur].d[0];
        if (!R[0].d[0] && !R[1].d[0])
            break;
    }
    SAME("messages", n[cur]);
    for (k = 0; k < n[0] && k < n[1]; k++)
        SAME("id", ids[cur][k]);
    return n[0];
}

static void devlist(ULONG flags, int stop)
{
    int k;
    ULONG len, names = 0;
    ROUT = "GetDeviceList";
    sprintf(VEC, "flags $%lX", flags);
    DO2(ENT(LIB, E_LOCK, 0), R[cur].d[0] = flags; R[cur].a[6] = (ULONG)dos);
    SAME("d0", R[cur].d[0]);
    for (k = 0; k < 200; k++) {
        ROUT = "NextDevice";
        sprintf(VEC, "flags $%lX entry %d", flags, k);
        DO2(ENT(LIB, E_NEXT, 0), memset(out[cur], 0xee, sizeof out[0]);
            R[cur].a[3] = (ULONG)out[cur] + 4; R[cur].a[6] = (ULONG)dos);
        STRRES();
        len = R[0].d[0];
        if (len)
            names++;
        ROUT = "DeviceEntryType";
        DO2(ENT(LIB, E_TYPE, 0), (void)0);
        SAME("d0", R[cur].d[0]);
        if (!len || k == stop)
            break;
    }
    check("ridoslib: NextDevice found entries", names > 0);
    printf("ridoslib: flags $%lX: %lu entries\n", flags, names);
    ROUT = "FreeDeviceList";
    DO2(ENT(LIB, E_FREE, 0), R[cur].a[6] = (ULONG)dos);
    SAME("d0", R[cur].d[0]);
    ROUT = "DeviceEntryType (unlocked)";
    DO2(ENT(LIB, E_TYPE, 0), (void)0);
    SAME("d0", R[cur].d[0]);
    ROUT = "NextDevice (unlocked)";
    DO2(ENT(LIB, E_NEXT, 0), R[cur].a[3] = (ULONG)out[cur] + 4; R[cur].a[6] = (ULONG)dos);
    SAME("d0 unchanged", R[cur].d[0] == IN[cur].d[0]);
    SAME("a3", R[cur].a[3] - (ULONG)out[cur]);
}

static void before_init(void)
{
    strcpy(VEC, "before init");
    ROUT = "CheckNotify";
    DO2(ENT(LIB, E_CHECK, 0), (void)0);
    SAME("d0", R[cur].d[0]);
    ROUT = "WaitNotify";
    DO2(ENT(LIB, E_WAIT, 0), (void)0);
    SAME("d0", R[cur].d[0]);
    ROUT = "NotifyPort";
    DO2(ENT(LIB, E_PORT, 0), (void)0);
    SAME("d0", R[cur].d[0]);
    ROUT = "AddNotify";
    DO2(ENT(LIB, E_ADD, 0), R[cur].d[0] = 3; R[cur].d[1] = (ULONG)TESTFILE;
        frame[cur][0] = strlen(TESTFILE); R[cur].a[2] = (ULONG)&frame[cur][1]);
    SAME("d0", R[cur].d[0]);
    ROUT = "RemNotify";
    DO2(ENT(LIB, E_REM, 0), R[cur].d[0] = 3);
    SAME("d0", R[cur].d[0]);
    ROUT = "FreeDeviceList";
    DO2(ENT(LIB, E_FREE, 0), R[cur].a[6] = (ULONG)dos);
    SAME("d0", R[cur].d[0]);
    ROUT = "NextDevice";
    DO2(ENT(LIB, E_NEXT, 0), R[cur].a[3] = (ULONG)out[cur]; R[cur].a[6] = (ULONG)dos);
    SAME("d0", R[cur].d[0]);
    SAME("a3", R[cur].a[3] - (ULONG)out[cur]);
    ROUT = "DeviceEntryType";
    DO2(ENT(LIB, E_TYPE, 0), (void)0);
    SAME("d0", R[cur].d[0]);
}

static void addnotify(ULONG id, const char *file)
{
    ROUT = "AddNotify";
    sprintf(VEC, "id %ld %s", (long)id, file);
    DO2(ENT(LIB, E_ADD, 0), R[cur].d[0] = id; R[cur].d[1] = (ULONG)file;
        frame[cur][0] = strlen(file); R[cur].a[2] = (ULONG)&frame[cur][1]);
    SAME("d0", R[cur].d[0]);
}

static void remnotify(ULONG id)
{
    ROUT = "RemNotify";
    sprintf(VEC, "id %ld", (long)id);
    DO2(ENT(LIB, E_REM, 0), R[cur].d[0] = id);
}

int main(int argc, char **argv)
{
    static const char *names[] = { LIB, NULL };
    struct Process *me = (struct Process *)FindTask(NULL);
    APTR win;
    ULONG avail, port[2];

    if (argc < 2) {
        printf("usage: ridoslib DIR\n");
        return 20;
    }
    if (!load_sets(argv[1], names))
        return 20;
    dos = (struct DosLibrary *)OpenLibrary((STRPTR)"dos.library", 36);
    DeleteFile((STRPTR)TESTFILE);

    before_init();
    paths();

    avail = AvailMem(MEMF_ANY);
    ROUT = "init";
    strcpy(VEC, "");
    DO2(ENT(LIB, E_INIT, 0), (void)0);
    SAME("d0 != 0", R[cur].d[0] != 0);
    for (cur = 0; cur < 2; cur++)
        port[cur] = R[cur].d[0];
    ROUT = "NotifyPort";
    DO2(ENT(LIB, E_PORT, 0), (void)0);
    SAME("d0 = init's port", R[cur].d[0] == port[cur]);

    /* notifications */
    drain("nothing yet");
    addnotify(5, TESTFILE);
    addnotify(9, TESTFILE);
    drain("nothing yet");
    touch();
    check("ridoslib: messages after a write", drain("after a write") > 0);
    touch();
    ROUT = "WaitNotify";
    strcpy(VEC, "after a write");
    DO2(ENT(LIB, E_WAIT, 0), (void)0);
    SAME("d0", R[cur].d[0]);
    drain("the rest");
    remnotify(5);
    remnotify(5);
    SAME("d0 = id*4", R[cur].d[0]);
    remnotify((ULONG)-1);
    SAME("d0 = id*4", R[cur].d[0]);
    touch();
    check("ridoslib: messages for 9", drain("only 9 left") > 0);
    /* a request StartNotify refuses (no requester for the missing volume) */
    win = me->pr_WindowPtr;
    me->pr_WindowPtr = (APTR)-1;
    addnotify(2, "RWENOSUCHDEVICE:x");
    me->pr_WindowPtr = win;
    addnotify(40, TESTFILE);
    remnotify(9);
    touch();
    check("ridoslib: messages for 40", drain("only 40 left") > 0);

    /* the device list */
    devlist(LDF_DEVICES | LDF_VOLUMES | LDF_ASSIGNS, -1);
    devlist(LDF_VOLUMES, -1);
    devlist(LDF_DEVICES, 1);

    /* finit ends request 40; the failed request 2 stays allocated
       (BUG(orig)) */
    ROUT = "finit";
    strcpy(VEC, "");
    DO2(ENT(LIB, E_FINIT, 0), (void)0);
    touch();
    DeleteFile((STRPTR)TESTFILE);
    printf("ridoslib: %ld bytes less memory than before init\n",
           (long)(avail - AvailMem(MEMF_ANY)));
    CloseLibrary((struct Library *)dos);
    return rwe_done(LIB);
}
