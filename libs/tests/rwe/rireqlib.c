/*
 * rwe rireqlib - ReqFileRequest, ReqFileLoc, ReqOutput, ReqFlags,
 * ReqColour: the C build against the CD original (OL RIREQLIB.OBJ),
 * method of rwecmp.h.
 *
 *   rireqlib DIR
 *
 * The requesters would need a user, so the test puts a fake req.library
 * in the system library list: its FileRequester and ColorRequester record
 * the structure they get and return a chosen value.  Covered: init
 * (OpenLibrary fails; AllocMem fails - the library is closed again; ok;
 * a second init does nothing), ReqFileLoc, ReqFlags, ReqOutput (a0 = a
 * window object), all three forms of ReqFileRequest (ok and cancel; the
 * copied path, d0, a3, the FrStruct the requester gets), ReqColour (the
 * structure, d0), finit (library closed, memory back).  Not covered: the
 * real req.library (not in the system, and its requesters need a user);
 * ReqFileRequest/ReqColour without the library (they call through a6 =
 * 0, BUG(orig), #788).
 */
#include "rwecmp.h"

#define LIB "rireqlib"
enum { E_FILEREQ, E_FILELOC, E_OUTPUT, E_FLAGS, E_COLOUR, E_FINIT, E_INIT };

/* ------------------------------------------------- the fake req.library */

#define NVEC 32                         /* up to ColorRequester (-192) */
static struct Library *fake;
static ULONG fk_ret, fk_colret, fk_calls[2];
static UBYTE rec_fr[2][162], rec_cr[2][30];

static ULONG fk_filereq(const UBYTE *fr __asm("a0"))
{
    memcpy(rec_fr[cur], fr, sizeof rec_fr[0]);
    fk_calls[cur]++;
    return fk_ret;
}

static ULONG fk_colorreq(const UBYTE *cr __asm("a0"))
{
    memcpy(rec_cr[cur], cr, sizeof rec_cr[0]);
    fk_calls[cur]++;
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
    DO2(ENT(LIB, E_INIT, 0), (void)0);
    SetFunction((struct Library *)SysBase, lvo, old);
    CacheClearU();
    Permit();
    SAME("d0", R[cur].d[0]);
    SAME("library closed", fake ? fake->lib_OpenCnt : 0);
    ROUT = "ReqFileLoc (no init)";
    DO2(ENT(LIB, E_FILELOC, 0), (void)0);
    SAME("d0", R[cur].d[0]);
    SAME("a0", R[cur].a[0]);
    ROUT = "finit (no init)";
    DO2(ENT(LIB, E_FINIT, 0), (void)0);
}

/* ------------------------------------------------------------- the test */

static UBYTE out[2][200];
static ULONG lenframe[2][2], winobj[2][2], loc[2];

static void filereq(int form, const char *title, ULONG flags, ULONG ret, const char *path)
{
    int k;
    for (cur = 0; cur < 2; cur++) {
        memset(out[cur], 0xee, sizeof out[0]);
        strcpy((char *)loc[cur], path);
        if (title)                      /* the compiler built it at a3 */
            strcpy((char *)out[cur] + 20, title);
        fk_calls[cur] = 0;
    }
    fk_ret = ret;
    sprintf(VEC, "form %d title %s flags $%lX ret %lu path \"%s\"", form, title ? title : "-",
            flags, ret, path);
    ROUT = "ReqFileRequest";
    DO2(ENT(LIB, E_FILEREQ, form), R[cur].d[0] = (ULONG)title; R[cur].d[1] = flags;
        lenframe[cur][0] = title ? strlen(title) : 0; R[cur].a[2] = (ULONG)&lenframe[cur][1];
        R[cur].a[3] = (ULONG)out[cur] + 20 + (title ? strlen(title) + 1 : 0));
    SAME("d0", R[cur].d[0]);
    SAME("a3", R[cur].a[3] - (ULONG)out[cur]);
    SAMEBYTES("out", out, sizeof out[0]);
    SAME("called", fk_calls[cur]);
    for (k = 0; k < (int)sizeof rec_fr[0]; k += 2) {
        char what[40];
        sprintf(what, "FrStruct+%d", k);
        SAME(what, *(UWORD *)&rec_fr[cur][k]);
    }
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
    if (!load_sets(argv[1], names))
        return 20;
    strcpy(VEC, "");

    init_failing(-552, "init (OpenLibrary fails)");
    fake_add();
    avail = AvailMem(MEMF_ANY);
    rwe_mem_start();
    init_failing(-198, "init (AllocMem fails)");

    ROUT = "init";
    DO2(ENT(LIB, E_INIT, 0), (void)0);
    for (cur = 0; cur < 2; cur++)
        loc[cur] = R[cur].d[0];
    SAME("library open", fake->lib_OpenCnt);
    ROUT = "init (again)";
    DO2(ENT(LIB, E_INIT, 0), (void)0);
    SAME("d0", R[cur].d[0]);
    SAME("library open", fake->lib_OpenCnt);

    ROUT = "ReqFileLoc";
    DO2(ENT(LIB, E_FILELOC, 0), (void)0);
    SAME("d0 = init's", R[cur].d[0] == loc[cur]);
    SAME("a0 = d0", R[cur].a[0] == R[cur].d[0]);
    SAME("buffers cleared", ({ int z = 1, i; for (i = 0; i < 940; i++)
                                   z &= ((UBYTE *)loc[cur])[i - 620] == 0; z; }));

    filereq(0, NULL, 0, 1, "Work:dir/file");
    ROUT = "ReqFlags";
    strcpy(VEC, "$12345678");
    DO2(ENT(LIB, E_FLAGS, 0), R[cur].d[0] = 0x12345678);
    filereq(0, NULL, 0, 1, "");
    ROUT = "ReqOutput";
    strcpy(VEC, "");
    for (cur = 0; cur < 2; cur++)
        winobj[cur][0] = 0xcafe0042UL;
    DO2(ENT(LIB, E_OUTPUT, 0), R[cur].a[0] = (ULONG)winobj[cur]);
    filereq(0, NULL, 0, 1, "RAM:x");
    filereq(1, "Choose a file", 0, 1, "DH0:a/b/c.info");
    filereq(1, "", 0, 5, "x");
    filereq(1, "Cancel", 0, 0, "never");
    filereq(2, "With flags", 0x80, 1, "S:Startup-Sequence");
    filereq(2, "Cancel again", 0x41, 0, "never");
    filereq(0, NULL, 0, 1, "title and flags stay");

    ROUT = "ReqColour";
    for (k = 0; k < 2; k++) {
        sprintf(VEC, "colour %d", 3 + k);
        fk_colret = 7 + k;
        DO2(ENT(LIB, E_COLOUR, 0), R[cur].d[0] = 3 + k; fk_calls[cur] = 0);
        SAME("d0", R[cur].d[0]);
        SAME("called", fk_calls[cur]);
        SAME("ecr", memcmp(rec_cr[cur], rec_cr[!cur], sizeof rec_cr[0]));
        SAME("DefColour", *(UWORD *)rec_cr[cur]);
        SAME("WinPtr", *(ULONG *)(rec_cr[cur] + 2));
    }

    ROUT = "finit";
    strcpy(VEC, "");
    DO2(ENT(LIB, E_FINIT, 0), (void)0);
    SAME("library closed", fake->lib_OpenCnt);
    check("rireqlib: library closed", fake->lib_OpenCnt == 0);
    rwe_mem_check(LIB, avail, TRUE);
    ROUT = "finit (again)";
    DO2(ENT(LIB, E_FINIT, 0), (void)0);
    fake_remove(fake);
    return rwe_done(LIB);
}
