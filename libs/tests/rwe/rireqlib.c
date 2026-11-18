/*
 * rwe rireqlib - ReqFileRequest, ReqFileLoc, ReqOutput, ReqFlags,
 * ReqColour: the C build or the CD original (OL RIREQLIB.OBJ), one set
 * as LIBSET says, observed as cases (rwecmp.h; the expected data
 * libs/tests/expected/rwe_rireqlib.txt).
 *
 *   rireqlib DIR
 *
 * The requesters would need a user, so the test puts a fake req.library
 * in the system library list: its FileRequester and ColorRequester record
 * the structure they get and return a chosen value; FileRequester writes
 * "FR:" and the path into the structure's path buffer when it has one and
 * the requester is confirmed, as req.library does.  Covered: init
 * (OpenLibrary fails; AllocMem fails - the library is closed again; ok;
 * a second init does nothing), ReqFileLoc, ReqFlags, ReqOutput (a0 = a
 * window object), all three forms of ReqFileRequest (ok and cancel; the
 * copied path, d0, a3, the fields of the FrStruct the requester gets and
 * the title text it sees), ReqColour (the structure, d0), the
 * requester's own fields behind offset 162 (the fake writes them once,
 * then ReqFileLoc and ReqColour show whether the library's data is
 * intact; the test writes the old bytes back before the next call),
 * finit (library closed, memory back).  Not covered: the real
 * req.library (not in the system, and its requesters need a user);
 * ReqFileRequest/ReqColour without the library (the original calls
 * through a6 = 0, #788: the bug-orig test rireqlib-no-base-check).
 */
#include "rwecmp.h"

#define LIB "rireqlib"
enum { E_FILEREQ, E_FILELOC, E_OUTPUT, E_FLAGS, E_COLOUR, E_FINIT, E_INIT };

/* ------------------------------------------------- the fake req.library */

#define NVEC 32                         /* up to ColorRequester (-192) */
static struct Library *fake;
static ULONG fk_ret, fk_colret, fk_calls;
static UBYTE rec_fr[162] __attribute__((aligned(4)));
static UBYTE rec_cr[30] __attribute__((aligned(4)));
static char fk_path[60], rec_title[41];
static const char *fk_title;            /* the title pointer it got */

/* req.library's own fields from offset 152 on (frq_Show, the cursor
   positions, frq_Memory, frq_Memory2; frq_SIZEOF 356): with fk_overrun
   the fake writes 152-207 once, as the requester keeps its state there,
   and saves the old bytes for fk_restore.  Not further: frq_Lock (208)
   is where both builds of the 162-byte structure have the library base. */
#define OV_FROM 152
#define OV_TO 208
static int fk_overrun;
static UBYTE *ov_at, ov_saved[OV_TO - OV_FROM];

static ULONG fk_filereq(UBYTE *fr __asm("a0"))
{
    char *path;
    int i;

    memcpy(rec_fr, fr, sizeof rec_fr);
    fk_title = *(const char **)(fr + 2);
    memset(rec_title, 0, sizeof rec_title);
    if (fk_title)
        for (i = 0; i < 40 && fk_title[i]; i++)
            rec_title[i] = fk_title[i];
    path = *(char **)(fr + 14);
    if (path && fk_ret) {
        strcpy(path, "FR:");
        strcat(path, fk_path);
    }
    if (fk_overrun) {
        fk_overrun = 0;
        ov_at = fr + OV_FROM;
        memcpy(ov_saved, ov_at, sizeof ov_saved);
        memset(ov_at, 0, sizeof ov_saved);
        strcpy((char *)ov_at, "#?.bas");              /* frq_Show */
        for (i = 0; i < 8; i++)                         /* cursor positions */
            ((UWORD *)(fr + 184))[i] = 0x101 * (i + 1);
        ((ULONG *)(fr + 200))[0] = 0x00a5a5a4UL;        /* frq_Memory */
        ((ULONG *)(fr + 200))[1] = 0x00a5a5a8UL;        /* frq_Memory2 */
    }
    fk_calls++;
    return fk_ret;
}

static void fk_restore(void)
{
    memcpy(ov_at, ov_saved, sizeof ov_saved);
}

static ULONG fk_colorreq(const UBYTE *cr __asm("a0"))
{
    memcpy(rec_cr, cr, sizeof rec_cr);
    fk_calls++;
    return fk_colret;
}

static void fake_add(void)
{
    static APTR vec[NVEC + 1];
    vec[13] = (APTR)fk_filereq;         /* -$54 */
    vec[31] = (APTR)fk_colorreq;        /* -$c0 */
    fake = fake_library("req.library", 0, vec, NVEC);
}

static void init_failing(WORD lvo, const char *what)
{
    APTR old;
    ROUT = what;
    Forbid();
    old = SetFunction((struct Library *)SysBase, lvo, (APTR)rwe_fail0);
    CacheClearU();
    DO1(ENT(LIB, E_INIT, 0), (void)0);
    SetFunction((struct Library *)SysBase, lvo, old);
    CacheClearU();
    Permit();
    OBS("d0", R.d[0]);
    OBS("library closed", fake ? fake->lib_OpenCnt : 0);
    ROUT = "ReqFileLoc (no init)";
    DO1(ENT(LIB, E_FILELOC, 0), (void)0);
    OBS("d0", R.d[0]);
    OBS("a0", R.a[0]);
    ROUT = "finit (no init)";
    DO1(ENT(LIB, E_FINIT, 0), (void)0);
}

/* ------------------------------------------------------------- the test */

static UBYTE out[200];
static ULONG lenframe[2], winobj[2], loc;

static void filereq(int form, const char *title, ULONG flags, ULONG ret, const char *path)
{
    char res[61];
    ULONG n;

    memset(out, 0xee, sizeof out);
    strcpy((char *)loc, path);
    strcpy(fk_path, path);
    if (title)                          /* the compiler built it at a3 */
        strcpy((char *)out + 20, title);
    fk_calls = 0;
    fk_ret = ret;
    sprintf(VEC, "form %d title %s flags $%lX ret %lu path \"%s\"", form, title ? title : "-",
            flags, ret, path);
    ROUT = "ReqFileRequest";
    DO1(ENT(LIB, E_FILEREQ, form), R.d[0] = (ULONG)title; R.d[1] = flags;
        lenframe[0] = title ? strlen(title) : 0; R.a[2] = (ULONG)&lenframe[1];
        R.a[3] = (ULONG)out + 20 + (title ? strlen(title) + 1 : 0));
    OBS("d0", R.d[0]);
    OBS("a3", R.a[3] - (ULONG)out);
    OBSBYTES("out", out, sizeof out);
    n = R.d[0] < 60 ? R.d[0] : 60;      /* the result, at the start */
    memcpy(res, out + 20, n);
    res[n] = 0;
    OBSSTR("result", res);
    OBS("called", fk_calls);
    /* the FrStruct the requester got: the buffers relative to init's
       path buffer, a pointer into the test program as prog */
    OBS("version", *(UWORD *)rec_fr);
    OBSPTR("title", *(ULONG *)(rec_fr + 2), "path", loc);
    OBSPTR("dir", *(ULONG *)(rec_fr + 6), "path", loc);
    OBSPTR("file", *(ULONG *)(rec_fr + 10), "path", loc);
    OBSPTR("pathname", *(ULONG *)(rec_fr + 14), "path", loc);
    OBS("window", *(ULONG *)(rec_fr + 18));
    OBSBYTES("maxextsel lines columns", rec_fr + 22, 8);
    OBS("flags", *(ULONG *)(rec_fr + 30));
    OBSBYTES("rest", rec_fr + 34, sizeof rec_fr - 34);
    OBSSTR("title text", fk_title ? rec_title : NULL);
}

int main(int argc, char **argv)
{
    static const char *names[] = { LIB, NULL };
    ULONG avail;
    int k;

    if (argc < 2) {
        printf("usage: rireqlib DIR\n");
        return 20;
    }
    if (!load_set(argv[1], names))
        return 20;
    KEEPS("init (OpenLibrary fails)", d1-d7|a0|a2|a3);
    KEEPS("ReqFileLoc (no init)", d1-d7|a1-a3|a6);
    KEEPS("finit (no init)", d0-d7|a0-a3|a6);
    KEEPS("init (AllocMem fails)", d2-d7|a2|a3);
    KEEPS("init", d2-d7|a2|a3);
    KEEPS("init (again)", d0-d7|a0-a3|a6);
    KEEPS("ReqFileLoc", d1-d7|a1-a3|a6);
    KEEPS("ReqFileRequest", d2|d4-d7);
    KEEPS("ReqFlags", d0-d7|a1-a3|a6);
    KEEPS("ReqOutput", d1-d7|a1-a3|a6);
    KEEPS("ReqColour", d2-d7|a2|a3);
    KEEPS("finit", d2-d7|a2|a3);
    KEEPS("finit (again)", d0-d7|a0-a3|a6);
    strcpy(VEC, "");

    init_failing(-552, "init (OpenLibrary fails)");
    fake_add();
    avail = AvailMem(MEMF_ANY);
    rwe_mem_start();
    init_failing(-198, "init (AllocMem fails)");

    ROUT = "init";
    DO1(ENT(LIB, E_INIT, 0), (void)0);
    loc = R.d[0];
    OBS("library open", fake->lib_OpenCnt);
    ROUT = "init (again)";
    DO1(ENT(LIB, E_INIT, 0), (void)0);
    OBS("d0", R.d[0]);
    OBS("library open", fake->lib_OpenCnt);

    ROUT = "ReqFileLoc";
    DO1(ENT(LIB, E_FILELOC, 0), (void)0);
    OBS("d0 = init's", R.d[0] == loc);
    OBS("a0 = d0", R.a[0] == R.d[0]);
    OBS("buffers cleared", ({ int z = 1, i; for (i = 0; i < 940; i++)
                                   z &= ((UBYTE *)loc)[i - 620] == 0; z; }));

    filereq(0, NULL, 0, 1, "Work:dir/file");
    ROUT = "ReqFlags";
    strcpy(VEC, "$12345678");
    DO1(ENT(LIB, E_FLAGS, 0), R.d[0] = 0x12345678);
    filereq(0, NULL, 0, 1, "");
    ROUT = "ReqOutput";
    strcpy(VEC, "");
    winobj[0] = 0xcafe0042UL;
    DO1(ENT(LIB, E_OUTPUT, 0), R.a[0] = (ULONG)winobj);
    filereq(0, NULL, 0, 1, "RAM:x");
    filereq(1, "Choose a file", 0, 1, "DH0:a/b/c.info");
    filereq(1, "", 0, 5, "x");
    filereq(1, "Cancel", 0, 0, "never");
    filereq(2, "With flags", 0x80, 1, "S:Startup-Sequence");
    filereq(2, "Cancel again", 0x41, 0, "never");
    filereq(0, NULL, 0, 1, "title and flags stay");

    /* the requester writes its own fields behind offset 162: with a
       structure of 162 bytes that is the library's data behind it (the
       buffer pointers and the colour structure in the original, the
       buffer pointers, the window and the colour structure in the C
       build), so the path buffer and the colour structure's RFU longs are
       observed - what both layouts lose alike */
    fk_overrun = 1;
    filereq(0, NULL, 0, 1, "overrun");
    strcpy(VEC, "after overrun");
    ROUT = "ReqFileLoc";
    DO1(ENT(LIB, E_FILELOC, 0), (void)0);
    OBS("d0 = init's", R.d[0] == loc);
    ROUT = "ReqColour";
    fk_colret = 9;
    DO1(ENT(LIB, E_COLOUR, 0), R.d[0] = 2; fk_calls = 0);
    OBS("d0", R.d[0]);
    OBS("called", fk_calls);
    OBS("RFU 0", ({ int z = 1, i; for (i = 6; i < 30; i++) z &= rec_cr[i] == 0; z; }));
    fk_restore();

    ROUT = "ReqColour";
    for (k = 0; k < 2; k++) {
        sprintf(VEC, "colour %d", 3 + k);
        fk_colret = 7 + k;
        DO1(ENT(LIB, E_COLOUR, 0), R.d[0] = 3 + k; fk_calls = 0);
        OBS("d0", R.d[0]);
        OBS("called", fk_calls);
        OBSBYTES("ecr", rec_cr, sizeof rec_cr);
        OBS("DefColour", *(UWORD *)rec_cr);
        OBS("WinPtr", *(ULONG *)(rec_cr + 2));
    }

    ROUT = "finit";
    strcpy(VEC, "");
    DO1(ENT(LIB, E_FINIT, 0), (void)0);
    OBS("library closed", fake->lib_OpenCnt);
    check("rireqlib: library closed", fake->lib_OpenCnt == 0);
    rwe_mem_check(LIB, avail, TRUE);
    ROUT = "finit (again)";
    DO1(ENT(LIB, E_FINIT, 0), (void)0);
    fake_remove(fake);
    return rwe_done(LIB);
}
