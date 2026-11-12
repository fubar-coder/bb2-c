/*
 * rwe rinonvolatilelib - IsNonvolatile, StoreNVData, GetCopyNV,
 * FreeNVData, GetNVList: the C build or the CD original (BSS2
 * RINONVOLATILELIB.OBJ), one set as LIBSET says, observed as cases
 * (rwecmp.h; the expected data
 * libs/tests/expected/rwe_rinonvolatilelib.txt).
 *
 *   rinonvolatilelib DIR
 *
 * Three rounds, each init ... finit:
 *   - without the library (OpenLibrary patched to fail for the inits):
 *     every routine returns 0;
 *   - with the system's nonvolatile.library: on a machine without NVRAM
 *     it stores in the place named by SYS:prefs/env-archive/sys/
 *     nv_location, which the read-only system disk does not have, so
 *     every StoreNV fails (NVERR_FAIL) and nothing is read - only the
 *     answers;
 *   - with a fake nonvolatile.library (before the real one in the
 *     library list) that logs every call with its registers and returns
 *     chosen values: the system calls each routine makes (DeleteNV, then
 *     StoreNV with the length in tens; killRequesters), lengths 0, 1, 10,
 *     25, 65535, 655359 and 655365 (BUG(orig) #764: DIVU overflow), the
 *     results (d0; StoreNVData only the low word, the command is #word).
 * Kept registers on every call.
 */
#include "rwecmp.h"
#include <proto/nonvolatile.h>

#define LIB "rinonvolatilelib"
enum { E_IS, E_STORE, E_GETCOPY, E_FREE, E_LIST, E_FINIT, E_INIT };

static const char app[] = "RWETest", item1[] = "Item1", item2[] = "Item2";
static UBYTE data[64];

/* ------------------------------------------- the fake nonvolatile.library */

#define NVEC 10                         /* up to GetNVList (-60) */
#define MAXLOG 8
struct logent { ULONG lvo, a0, a1, a2, d0, d1; };
static struct logent fklog[MAXLOG];
static int fkn;
static ULONG fk_ret;

static void logcall(ULONG lvo, ULONG a0, ULONG a1, ULONG a2, ULONG d0, ULONG d1)
{
    if (fkn < MAXLOG) {
        struct logent *e = &fklog[fkn];
        e->lvo = lvo;
        e->a0 = a0;
        e->a1 = a1;
        e->a2 = a2;
        e->d0 = d0;
        e->d1 = d1;
    }
    fkn++;
}

static ULONG fk_getcopy(ULONG a0 __asm("a0"), ULONG a1 __asm("a1"), ULONG d1 __asm("d1"))
{
    logcall(30, a0, a1, 0, 0, d1);
    return fk_ret;
}

static ULONG fk_free(ULONG a0 __asm("a0"))
{
    logcall(36, a0, 0, 0, 0, 0);
    return fk_ret;
}

static ULONG fk_store(ULONG a0 __asm("a0"), ULONG a1 __asm("a1"), ULONG a2 __asm("a2"),
                      ULONG d0 __asm("d0"), ULONG d1 __asm("d1"))
{
    logcall(42, a0, a1, a2, d0, d1);
    return fk_ret;
}

static ULONG fk_delete(ULONG a0 __asm("a0"), ULONG a1 __asm("a1"), ULONG d1 __asm("d1"))
{
    logcall(48, a0, a1, 0, 0, d1);
    return 1;
}

static ULONG fk_list(ULONG a0 __asm("a0"), ULONG d1 __asm("d1"))
{
    logcall(60, a0, 0, 0, 0, d1);
    return fk_ret;
}

static struct Library *fake_add(void)
{
    static APTR vec[NVEC + 1];
    vec[4] = (APTR)fk_getcopy;
    vec[5] = (APTR)fk_free;
    vec[6] = (APTR)fk_store;
    vec[7] = (APTR)fk_delete;
    vec[9] = (APTR)fk_list;
    return fake_library("nonvolatile.library", 127, vec, NVEC);
}

static void cmplog(void)
{
    int k;
    OBS("calls", fkn);
    for (k = 0; k < fkn && k < MAXLOG; k++) {
        char what[40];
        sprintf(what, "call %d lvo", k);
        OBS(what, fklog[k].lvo);
        sprintf(what, "call %d a0", k);
        OBSPTR(what, fklog[k].a0, NULL, 0);    /* a string or data of the test: prog */
        sprintf(what, "call %d a1", k);
        OBSPTR(what, fklog[k].a1, NULL, 0);    /* a string or data of the test: prog */
        sprintf(what, "call %d a2", k);
        OBSPTR(what, fklog[k].a2, NULL, 0);    /* a string or data of the test: prog */
        sprintf(what, "call %d d0", k);
        OBS(what, fklog[k].d0);
        sprintf(what, "call %d d1", k);
        OBS(what, fklog[k].d1);
    }
}

#define LOGGED(setup) do { fkn = 0; setup; } while (0)

/* ------------------------------------------------------------- the test */

static void all_calls(int fake, const char *round)
{
    static const ULONG lens[] = { 0, 1, 10, 25, 65535, 655359, 655365 };
    static const ULONG rets[] = { 0, 3, 0x12340001UL };
    unsigned i, r;
    ULONG got;

    ROUT = "IsNonvolatile";
    sprintf(VEC, "%s", round);
    DO1(ENT(LIB, E_IS, 0), (void)0);
    OBS("d0", R.d[0]);

    for (r = 0; r < (fake ? sizeof rets / sizeof rets[0] : 1); r++) {
        fk_ret = rets[r];
        ROUT = "StoreNVData";
        for (i = 0; i < sizeof lens / sizeof lens[0]; i++) {
            const char *it = i & 1 ? item2 : item1;
            if (!fake && lens[i] > sizeof data)
                continue;
            sprintf(VEC, "%s %s length %lu ret $%lX", round, it, lens[i], fk_ret);
            DO1(ENT(LIB, E_STORE, 0), LOGGED(R.d[0] = (ULONG)app; R.d[1] = (ULONG)it;
                R.d[2] = (ULONG)data; R.d[3] = lens[i]));
            OBS("d0.w", (UWORD)R.d[0]);
            cmplog();
        }
        sprintf(VEC, "%s ret $%lX", round, fk_ret);
        ROUT = "GetCopyNV";
        DO1(ENT(LIB, E_GETCOPY, 0), LOGGED(R.d[0] = (ULONG)app; R.d[1] = (ULONG)item1));
        OBS("d0", fake ? R.d[0] : R.d[0] != 0);   /* system: an allocation */
        cmplog();
        got = R.d[0];
        if (!fake && got) {
            ROUT = "FreeNVData";
            DO1(ENT(LIB, E_FREE, 0), R.d[0] = got);
        }
        ROUT = "GetNVList";
        DO1(ENT(LIB, E_LIST, 0), LOGGED(R.d[0] = (ULONG)app));
        OBS("d0", fake ? R.d[0] : R.d[0] != 0);
        cmplog();
        got = R.d[0];
        if (!fake && got) {
            ROUT = "FreeNVData";
            DO1(ENT(LIB, E_FREE, 0), R.d[0] = got);
        }
        if (fake) {
            ROUT = "FreeNVData";
            DO1(ENT(LIB, E_FREE, 0), LOGGED(R.d[0] = (ULONG)data));
            cmplog();
        }
    }
    if (!fake) {
        /* nothing can be stored (see above); if it was, remove it */
        struct Library *NVBase = OpenLibrary((STRPTR)"nonvolatile.library", 0);
        if (NVBase) {
            DeleteNV((STRPTR)app, (STRPTR)item1, TRUE);
            DeleteNV((STRPTR)app, (STRPTR)item2, TRUE);
            CloseLibrary(NVBase);
        }
    }
}

int main(int argc, char **argv)
{
    static const char *names[] = { LIB, NULL };
    struct Library *fake;
    APTR oldopen;
    unsigned i;

    if (argc < 2) {
        printf("usage: rinonvolatilelib DIR\n");
        return 20;
    }
    if (!load_set(argv[1], names))
        return 20;
    KEEPS("IsNonvolatile", d1-d7|a0-a3|a6);
    KEEPS("StoreNVData", d2|d4-d7|a3);
    KEEPS("GetCopyNV", d2-d7|a2|a3);
    KEEPS("FreeNVData", d2-d7|a2|a3);
    KEEPS("GetNVList", d2-d7|a2|a3);
    KEEPS("init (fails)", d1-d7|a0|a2|a3);
    KEEPS("finit (closed)", d1-d7|a0-a3|a6);
    KEEPS("init (system)", d2-d7|a2|a3);
    KEEPS("finit (system)", d2-d7|a2|a3);
    KEEPS("init (fake)", d2-d7|a2|a3);
    KEEPS("finit (fake)", d2-d7|a2|a3);
    for (i = 0; i < sizeof data; i++)
        data[i] = (UBYTE)(i * 13 + 1);
    strcpy(VEC, "");

    /* without the library: both inits get 0 from OpenLibrary */
    Forbid();
    oldopen = SetFunction((struct Library *)SysBase, -552, (APTR)rwe_fail0);
    CacheClearU();
    ROUT = "init (fails)";
    DO1(ENT(LIB, E_INIT, 0), (void)0);
    SetFunction((struct Library *)SysBase, -552, oldopen);
    CacheClearU();
    Permit();
    OBS("d0", R.d[0]);
    all_calls(1, "closed");
    OBS("no call", fkn);
    ROUT = "finit (closed)";
    strcpy(VEC, "");
    DO1(ENT(LIB, E_FINIT, 0), (void)0);

    /* the system's library, if there is one */
    ROUT = "init (system)";
    DO1(ENT(LIB, E_INIT, 0), (void)0);
    OBS("d0 != 0", R.d[0] != 0);
    if (R.d[0]) {
        all_calls(0, "system");
        ROUT = "finit (system)";
        strcpy(VEC, "");
        DO1(ENT(LIB, E_FINIT, 0), (void)0);
    } else
        printf("rinonvolatilelib: no nonvolatile.library in the system\n");

    /* the fake */
    fake = fake_add();
    ROUT = "init (fake)";
    strcpy(VEC, "");
    DO1(ENT(LIB, E_INIT, 0), (void)0);
    OBS("d0 = fake", R.d[0] == (ULONG)fake);
    OBS("opened", fake->lib_OpenCnt);
    all_calls(1, "fake");
    ROUT = "finit (fake)";
    strcpy(VEC, "");
    DO1(ENT(LIB, E_FINIT, 0), (void)0);
    check("rinonvolatilelib: fake closed", fake->lib_OpenCnt == 0);
    fake_remove(fake);
    return rwe_done(LIB);
}
