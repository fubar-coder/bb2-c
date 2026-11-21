/*
 * rwe ridoslib - AddNotify, RemNotify, CheckNotify, WaitNotify,
 * GetDeviceList, NextDevice, FreeDeviceList, NotifyPort, GetFileName,
 * GetPathName, DeviceEntryType: the C build or the CD original (BSS2
 * RIDOSLIB.OBJ), one set as LIBSET says, observed as cases (rwecmp.h;
 * the expected data libs/tests/expected/rwe_ridoslib.txt).
 *
 *   ridoslib DIR
 *
 * Covered: every routine before init (no port, no dos base: 0 / d0
 * unchanged), init, NotifyPort; GetFileName/GetPathName with many paths
 * (separators first/last/doubled, none, empty, an argument of $10006
 * characters) - d0, a3 and the result; notifications on a file in RAM: (ids
 * 5, 9 and 40: CheckNotify until empty after
 * every write, WaitNotify, RemNotify twice and with id -1, a failing
 * StartNotify on a missing device, one request left for finit);
 * GetDeviceList (devices, volumes, assigns) and NextDevice to the end (the
 * entries recorded sorted by name length and type: the list order varies)
 * with DeviceEntryType after each, NextDevice/DeviceEntryType after
 * FreeDeviceList (the type of the last entry, #622); finit (ends the requests left;
 * the failed request was freed by AddNotify, #621).
 * Not covered: ids outside 0..63 (they write outside the tables), file or path
 * parts longer than 255 characters (the original writes over its code, #627).
 */
#include "rwecmp.h"
#include <proto/dos.h>
#include <dos/dosextens.h>

#define LIB "ridoslib"
enum { E_ADD, E_REM, E_CHECK, E_WAIT, E_LOCK, E_NEXT, E_FREE, E_PORT, E_FILE, E_PATH,
       E_TYPE, E_INIT, E_FINIT };

#define TESTFILE "RAM:ridoslib-notify"

static UBYTE out[300];
static ULONG frame[2];
static struct DosLibrary *dos;

/* string argument at out+20 (where the compiler built it), a3 behind it */
#define STRARG(s, len) do {                                                 \
        memset(out, 0xee, sizeof out);                                      \
        strcpy((char *)out + 20, s);                                        \
        R.d[0] = (ULONG)out + 20;                                           \
        frame[0] = (len);                                                   \
        R.a[2] = (ULONG)&frame[1];                                          \
        R.a[3] = (ULONG)out + 20 + strlen(s) + 1;                           \
    } while (0)

#define STRRES() do {                                                       \
        OBS("d0", R.d[0]);                                                  \
        OBS("a3", R.a[3] - (ULONG)out);                                     \
        OBSBYTES("out", out, sizeof out);                                   \
    } while (0)

/* the argument of $10006 characters at big+20, a3 behind it: "abc/de",
   then "a" with a "/" every 100 characters (no part reaches the
   original's 256 byte buffer), then "/xyz" */
#define LONGLEN 0x10006
static UBYTE big[20 + LONGLEN + 1 + 20];

static void longarg(void)
{
    ULONG i;

    memset(big, 0xee, sizeof big);
    memcpy(big + 20, "abc/de", 6);
    for (i = 6; i < LONGLEN - 4; i++)
        big[20 + i] = i % 100 == 99 ? '/' : 'a';
    memcpy(big + 20 + LONGLEN - 4, "/xyz", 5);
    R.d[0] = (ULONG)big + 20;
    frame[0] = LONGLEN;
    R.a[2] = (ULONG)&frame[1];
    R.a[3] = (ULONG)big + 20 + LONGLEN + 1;
}

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
            DO1(ENT(LIB, f ? E_PATH : E_FILE, 0), STRARG(p[i], strlen(p[i])));
            STRRES();
        }
        /* a real argument of $10006 characters (longarg; the low word of
           its length is 6): file part "xyz", path part all but "xyz" */
        strcpy(VEC, "\"abc/de\" length $10006");
        DO1(ENT(LIB, f ? E_PATH : E_FILE, 0), longarg());
        OBS("d0", R.d[0]);
        OBS("a3", R.a[3] - (ULONG)big);
        OBSBYTES("out", big, sizeof big);
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
    ULONG ids[8];
    int n = 0, k;
    ROUT = "CheckNotify";
    strcpy(VEC, what);
    for (k = 0; k < 8; k++) {
        DO1(ENT(LIB, E_CHECK, 0), (void)0);
        if (R.d[0])
            ids[n++] = R.d[0];
        if (!R.d[0])
            break;
    }
    OBS("messages", n);
    for (k = 0; k < n; k++)
        OBS("id", ids[k]);
    return n;
}

/* One entry of the device list as NextDevice and DeviceEntryType report it:
   the name length (d0), a3, the type and the buffer with the name bytes
   masked.  The order of the list depends on what the session has mounted
   and when, so devlist() records the entries sorted. */
#define MAXENT 64
struct dent {
    ULONG len, a3, type;
    UBYTE out[sizeof out];
};

static void dent_get(struct dent *e)
{
    ULONG i;
    e->len = R.d[0];
    e->a3 = R.a[3] - (ULONG)out;
    memcpy(e->out, out, sizeof out);
    for (i = 0; i < e->len && 4 + i < sizeof e->out; i++)
        e->out[4 + i] = 'x';
}

/* by name length, then type */
static int dent_less(const struct dent *a, const struct dent *b)
{
    return a->len != b->len ? a->len < b->len : a->type < b->type;
}

static void dent_sort(struct dent *e, int n)
{
    int i, j;
    struct dent t;
    for (i = 1; i < n; i++) {
        t = e[i];
        for (j = i; j > 0 && dent_less(&t, &e[j - 1]); j--)
            e[j] = e[j - 1];
        e[j] = t;
    }
}

static int dent_same(const struct dent *a, const struct dent *b)
{
    return a->len == b->len && a->a3 == b->a3 && a->type == b->type
           && !memcmp(a->out, b->out, sizeof a->out);
}

static struct dent scan[MAXENT], seen[MAXENT];

/* the entries of the list without a case (lock, NextDevice and
   DeviceEntryType to the end, free), sorted; the terminating entry
   (name length 0) is not among them */
static int devscan(ULONG flags)
{
    int n = 0;
    markregs(&R);
    R.d[0] = flags;
    R.a[6] = (ULONG)dos;
    callstk(ENT(LIB, E_LOCK, 0), &R, ARGS, 0, &POP);
    for (;;) {
        markregs(&R);
        memset(out, 0xee, sizeof out);
        R.a[3] = (ULONG)out + 4;
        R.a[6] = (ULONG)dos;
        callstk(ENT(LIB, E_NEXT, 0), &R, ARGS, 0, &POP);
        if (!R.d[0] || n == MAXENT)
            break;
        dent_get(&scan[n]);
        markregs(&R);
        callstk(ENT(LIB, E_TYPE, 0), &R, ARGS, 0, &POP);
        scan[n].type = R.d[0];
        n++;
    }
    markregs(&R);
    R.a[6] = (ULONG)dos;
    callstk(ENT(LIB, E_FREE, 0), &R, ARGS, 0, &POP);
    dent_sort(scan, n);
    return n;
}

/* The order of the list depends on when the volumes and assigns were
   mounted in the session.  So the entries are collected first (devscan),
   sorted by name length, then type, and only then recorded as entry 0 ..
   N: NextDevice (d0 = the name length, a3, the buffer with the name
   masked) and DeviceEntryType.  The walk itself (the kept and popped
   cases, FreeDeviceList, the calls after it) is as it was; a walk that
   stops at `stop` records the first stop+1 sorted entries, and every
   entry it saw must be one of the collected ones. */
static void devlist(ULONG flags, int stop)
{
    int k, last, j, m = 0, total, ok;
    int used[MAXENT] = { 0 };
    struct dent cur, end;
    ULONG names = 0;

    total = devscan(flags);
    memset(&end, 0, sizeof end);
    ROUT = "GetDeviceList";
    sprintf(VEC, "flags $%lX", flags);
    DO1(ENT(LIB, E_LOCK, 0), R.d[0] = flags; R.a[6] = (ULONG)dos);
    OBS("d0", R.d[0]);
    for (k = 0; k < 200; k++) {
        ROUT = "NextDevice";
        sprintf(VEC, "flags $%lX entry %d", flags, k);
        DO1(ENT(LIB, E_NEXT, 0), memset(out, 0xee, sizeof out);
            R.a[3] = (ULONG)out + 4; R.a[6] = (ULONG)dos);
        dent_get(&cur);
        if (cur.len)
            names++;
        ROUT = "DeviceEntryType";
        DO1(ENT(LIB, E_TYPE, 0), (void)0);
        cur.type = R.d[0];
        if (!cur.len)
            end = cur;
        else if (m < MAXENT)
            seen[m++] = cur;
        if (!cur.len || k == stop)
            break;
    }
    last = k;
    check("ridoslib: NextDevice found entries", names > 0);
    printf("ridoslib: flags $%lX: %lu entries\n", flags, names);
    check("ridoslib: the walk and the scan agree on the entries",
          stop < 0 ? names == (ULONG)total : names <= (ULONG)total);
    for (k = 0; k < m; k++) {
        ok = 0;
        for (j = 0; j < total && !ok; j++)
            if (!used[j] && dent_same(&scan[j], &seen[k]))
                used[j] = ok = 1;
        check("ridoslib: an entry of the walk is one of the scan", ok);
    }
    for (k = 0; k <= last; k++) {
        const struct dent *e = k < total ? &scan[k] : &end;
        sprintf(VEC, "flags $%lX entry %d", flags, k);
        ROUT = "NextDevice";
        OBS("d0", e->len);
        OBS("a3", e->a3);
        OBSBYTES("out", e->out, sizeof e->out);
        ROUT = "DeviceEntryType";
        OBS("d0", e->type);
    }
    sprintf(VEC, "flags $%lX entry %d", flags, last);
    ROUT = "FreeDeviceList";
    DO1(ENT(LIB, E_FREE, 0), R.a[6] = (ULONG)dos);
    OBS("d0", R.d[0]);
    ROUT = "DeviceEntryType (unlocked)";
    DO1(ENT(LIB, E_TYPE, 0), (void)0);
    OBS("d0", R.d[0]);
    ROUT = "NextDevice (unlocked)";
    DO1(ENT(LIB, E_NEXT, 0), R.a[3] = (ULONG)out + 4; R.a[6] = (ULONG)dos);
    OBS("d0 unchanged", R.d[0] == IN.d[0]);
    OBS("a3", R.a[3] - (ULONG)out);
}

static void before_init(void)
{
    strcpy(VEC, "before init");
    ROUT = "CheckNotify";
    DO1(ENT(LIB, E_CHECK, 0), (void)0);
    OBS("d0", R.d[0]);
    ROUT = "WaitNotify";
    DO1(ENT(LIB, E_WAIT, 0), (void)0);
    OBS("d0", R.d[0]);
    ROUT = "NotifyPort";
    DO1(ENT(LIB, E_PORT, 0), (void)0);
    OBS("d0", R.d[0]);
    ROUT = "AddNotify";
    DO1(ENT(LIB, E_ADD, 0), R.d[0] = 3; R.d[1] = (ULONG)TESTFILE;
        frame[0] = strlen(TESTFILE); R.a[2] = (ULONG)&frame[1]);
    OBS("d0", R.d[0]);
    ROUT = "RemNotify";
    DO1(ENT(LIB, E_REM, 0), R.d[0] = 3);
    OBS("d0", R.d[0]);
    ROUT = "FreeDeviceList";
    DO1(ENT(LIB, E_FREE, 0), R.a[6] = (ULONG)dos);
    OBS("d0", R.d[0]);
    ROUT = "NextDevice";
    DO1(ENT(LIB, E_NEXT, 0), R.a[3] = (ULONG)out; R.a[6] = (ULONG)dos);
    OBS("d0", R.d[0]);
    OBS("a3", R.a[3] - (ULONG)out);
    ROUT = "DeviceEntryType";
    DO1(ENT(LIB, E_TYPE, 0), (void)0);
    OBS("d0", R.d[0]);
}

static void addnotify(ULONG id, const char *file)
{
    ROUT = "AddNotify";
    sprintf(VEC, "id %ld %s", (long)id, file);
    DO1(ENT(LIB, E_ADD, 0), R.d[0] = id; R.d[1] = (ULONG)file;
        frame[0] = strlen(file); R.a[2] = (ULONG)&frame[1]);
    OBS("d0", R.d[0]);
}

static void remnotify(ULONG id)
{
    ROUT = "RemNotify";
    sprintf(VEC, "id %ld", (long)id);
    DO1(ENT(LIB, E_REM, 0), R.d[0] = id);
}

int main(int argc, char **argv)
{
    static const char *names[] = { LIB, NULL };
    struct Process *me = (struct Process *)FindTask(NULL);
    APTR win;
    ULONG avail, port;

    if (argc < 2) {
        printf("usage: ridoslib DIR\n");
        return 20;
    }
    if (!load_set(argv[1], names))
        return 20;
    KEEPS("CheckNotify", d2-d7|a2|a3);
    KEEPS("WaitNotify", d2-d7|a2|a3);
    KEEPS("NotifyPort", d1-d7|a0-a3|a6);
    KEEPS("AddNotify", d2-d7|a3);
    KEEPS("RemNotify", d2|d3|d5-d7|a2|a3);
    KEEPS("FreeDeviceList", d2-d7|a1-a3);
    KEEPS("NextDevice", d3-d7|a1|a2);
    KEEPS("DeviceEntryType", d1-d7|a1-a3|a6);
    KEEPS("GetFileName", d1-d3|d6|d7|a6);
    KEEPS("GetPathName", d2|d3|d6|d7);
    KEEPS("init", d2-d7|a2|a3);
    KEEPS("GetDeviceList", d2-d7|a2|a3|a6);
    KEEPS("DeviceEntryType (unlocked)", d1-d7|a1-a3|a6);
    KEEPS("NextDevice (unlocked)", d1-d7|a0-a3|a6);   /* d0: the length (fixed #625: 0) */
    KEEPS("finit", d2|d3|d5-d7|a2|a3);
    dos = (struct DosLibrary *)OpenLibrary((STRPTR)"dos.library", 36);
    DeleteFile((STRPTR)TESTFILE);

    before_init();
    paths();

    avail = AvailMem(MEMF_ANY);
    ROUT = "init";
    strcpy(VEC, "");
    DO1(ENT(LIB, E_INIT, 0), (void)0);
    OBS("d0 != 0", R.d[0] != 0);
    port = R.d[0];
    ROUT = "NotifyPort";
    DO1(ENT(LIB, E_PORT, 0), (void)0);
    OBS("d0 = init's port", R.d[0] == port);

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
    DO1(ENT(LIB, E_WAIT, 0), (void)0);
    OBS("d0", R.d[0]);
    drain("the rest");
    remnotify(5);
    remnotify(5);
    OBS("d0 = id*4", R.d[0]);
    remnotify((ULONG)-1);
    OBS("d0 = id*4", R.d[0]);
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

    /* finit ends request 40; the failed request 2 was freed by AddNotify
       (#621) */
    ROUT = "finit";
    strcpy(VEC, "");
    DO1(ENT(LIB, E_FINIT, 0), (void)0);
    touch();
    DeleteFile((STRPTR)TESTFILE);
    printf("ridoslib: %ld bytes less memory than before init\n",
           (long)(avail - AvailMem(MEMF_ANY)));
    CloseLibrary((struct Library *)dos);
    return rwe_done(LIB);
}
