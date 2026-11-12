/*
 * rwe riregistrylib - the C build of riregistrylib (userlib 3) or the
 * reference build of rwelibsrc/Registry/Registry3.bb
 * (ref/riregistrylib.obj, see libs/userlibs/rwe/refbuild.cmake), one set
 * as LIBSET says, observed as cases (rwecmp.h, LIBS_ONE_SET; the expected
 * data libs/tests/expected/rwe_riregistrylib.txt).
 *
 *   riregistrylib DIR     (DIR with riregistrylib .obj/.orig)
 *
 * registry.library exists nowhere, so the test runs first without it and
 * then with a mock made here (MakeLibrary, in the system list as
 * "registry.library"): each of its 33 functions logs its LVO and
 * registers and returns a value the test chooses (LockRegistry: one of
 * three registry structures, with name, type and value).  Observed: d0,
 * the log (the LVOs, the registers the source loads for each - a0 the
 * structure, the string arguments, a3/a4 for the two calls that take
 * them; pointers into the string area relative to it, into the test
 * program as such), for the string functions a3
 * and the string area, the kept registers; the mock closed again by
 * finit.
 * Covered: without the library: init, IsRegistryOpen, LockRegistry and
 * every command (nothing called, 0; GetLibraryValue$ and
 * GetRegistryStringVariable read 18/24 of address 0, BUG(orig), #780), finit.
 * With the mock: init, IsRegistryOpen, every command and form before a
 * lock (nothing called) and with a structure, the string functions with
 * a string value, another type, no value, no name; GetRegistryInt/
 * BoolVariable with the types 2/3/4; nested locks, a failed lock (0
 * pushed: no defect, it keeps the stack balanced), UnlockRegistry back
 * to the structure below; finit
 * with three locks (unlocked from the top); after finit the stale
 * structure (BUG(orig), #779) and UnlockRegistry below the stack: the libfin's
 * 0, then $0000FFFF unlocked (BUG(orig), #781).
 * Not covered: more than 15 locks (BUG(orig) #781: writes over curstruct and
 * regstruct with the high word of a structure address) and a fourth
 * UnlockRegistry below the stack (the original reads its token table
 * there, DIFF); a structure while registry.library is missing (the call
 * goes to address LVO).
 */
#define LIBS_ONE_SET
#include "rwecmp.h"
#include <exec/memory.h>

#define LIB "riregistrylib"
enum { E_OPEN, E_LOCK, E_UNLOCK, E_SETBASE, E_EXISTS, E_GETLIBVAL, E_GETVAR, E_SETVAR,
       E_CHANGE, E_NEWLIB, E_PARSE, E_NEXT, E_ACCESS, E_DELLIB, E_DELVAR, E_NUMVARS, E_SAVELIB,
       E_LOADLIB, E_GETSTRVAR, E_SETBOOL, E_ITEMNAME, E_ITEMVALUE, E_GETBOOL, E_SETINT,
       E_GETINT, E_DELLIBVARS, E_SETLIBVAL, E_SETINDEX, E_RENVAR, E_SAVELIBVARS, E_RENLIB,
       E_INSERT, E_GETSTRUCT, E_FREESTRUCT, E_LOADLIBVARS };

/* ---------------------------------------------- the mock registry.library */

#define NVEC 33                         /* up to LoadLibraryVariables (-198) */
static struct Library *fake;
static UBYTE rs[3][32];                 /* the registry structures, shared */
static ULONG fk_ret, fk_lock;

/* the registers of a call: d0 d1 d2 a0 a1 a2 a3 a4 */
enum { RD0, RD1, RD2, RA0, RA1, RA2, RA3, RA4 };
#define M(r) (1 << (r))
struct rec {
    LONG lvo;
    ULONG r[8];
};
#define NLOG 8
static struct rec log_[NLOG];
static int nlog;
static UBYTE strbuf[256];               /* the string area */
static ULONG stk[8];                    /* the compiler's a2 stack */

/* the registers each call gets from the source (CallLibrary: a0) */
static ULONG lvomask(LONG lvo)
{
    switch (lvo) {
    case -30: return M(RD0);
    case -36: case -96: case -102: return M(RA0);
    case -42: case -84: case -108: case -114: case -120: case -138: return M(RA0) | M(RA1);
    case -48: case -54: case -60: return M(RA0) | M(RA1) | M(RD0);
    case -66: case -72: return M(RA0) | M(RA1) | M(RA2) | M(RD0) | M(RD1);
    case -78: return M(RA0) | M(RA1) | M(RA2) | M(RA3);
    case -150: return M(RA0) | M(RA1) | M(RA2) | M(RD0) | M(RD1) | M(RD2);
    case -180: return M(RA0) | M(RA1) | M(RA2) | M(RA3) | M(RA4);
    default: return M(RA0) | M(RA1) | M(RA2);
    }
}

/* a pointer into the string area as an offset (| $53000000) */
static ULONG norm(ULONG p)
{
    if (p >= (ULONG)strbuf && p < (ULONG)strbuf + sizeof strbuf)
        return 0x53000000 | (p - (ULONG)strbuf);
    return p;
}

static ULONG fkcall(LONG lvo, ULONG d0, ULONG d1, ULONG d2, ULONG a0, ULONG a1, ULONG a2,
                    ULONG a3, ULONG a4)
{
    struct rec *r = &log_[nlog < NLOG ? nlog++ : NLOG - 1];
    ULONG v[8] = { d0, d1, d2, a0, a1, a2, a3, a4 };
    int i;

    r->lvo = lvo;
    for (i = 0; i < 8; i++)
        r->r[i] = norm(v[i]);
    return lvo == -30 ? fk_lock : fk_ret;
}

#define FK(n)                                                               \
    static ULONG fk##n(ULONG d0 __asm("d0"), ULONG d1 __asm("d1"), ULONG d2 __asm("d2"), \
                       ULONG a0 __asm("a0"), ULONG a1 __asm("a1"), ULONG a2 __asm("a2"), \
                       ULONG a3 __asm("a3"), ULONG a4 __asm("a4"))          \
    {                                                                       \
        return fkcall(-6 * ((n) + 1), d0, d1, d2, a0, a1, a2, a3, a4);      \
    }
FK(4) FK(5) FK(6) FK(7) FK(8) FK(9) FK(10) FK(11) FK(12) FK(13) FK(14) FK(15) FK(16) FK(17)
FK(18) FK(19) FK(20) FK(21) FK(22) FK(23) FK(24) FK(25) FK(26) FK(27) FK(28) FK(29) FK(30)
FK(31) FK(32)

static void fake_add(void)
{
    static APTR vec[NVEC + 1];
    static const APTR fks[] = { (APTR)fk4, (APTR)fk5, (APTR)fk6, (APTR)fk7, (APTR)fk8,
        (APTR)fk9, (APTR)fk10, (APTR)fk11, (APTR)fk12, (APTR)fk13, (APTR)fk14, (APTR)fk15,
        (APTR)fk16, (APTR)fk17, (APTR)fk18, (APTR)fk19, (APTR)fk20, (APTR)fk21, (APTR)fk22,
        (APTR)fk23, (APTR)fk24, (APTR)fk25, (APTR)fk26, (APTR)fk27, (APTR)fk28, (APTR)fk29,
        (APTR)fk30, (APTR)fk31, (APTR)fk32 };
    int i;

    for (i = 4; i < NVEC; i++)
        vec[i] = fks[i - 4];
    fake = fake_library("registry.library", 0, vec, NVEC);
}

/* the log of the calls: the LVOs, the registers each gets (pointers into
   the test program as such) */
static void samelog(void)
{
    char w[160];
    int i, k;

    OBS("calls", nlog);
    for (i = 0; i < nlog; i++) {
        ULONG m = lvomask(log_[i].lvo);
        snprintf(w, sizeof w, "call %d LVO", i);
        OBS(w, log_[i].lvo);
        for (k = 0; k < 8; k++)
            if (m & M(k)) {
                snprintf(w, sizeof w, "call %d %c%d", i, k < 3 ? 'd' : 'a', k < 3 ? k : k - 3);
                OBSPTR(w, log_[i].r[k], NULL, 0);
            }
    }
    nlog = 0;
}

static void setrs(int i, const char *name, UWORD type, ULONG data)
{
    memset(rs[i], 0x6b, sizeof rs[i]);
    *(const char **)(rs[i] + 14) = name;
    *(UWORD *)(rs[i] + 18) = type;
    *(ULONG *)(rs[i] + 24) = data;
}

/* ------------------------------------------------------------- calls */

/* a command or function without a string result: d0-d4 */
static void cmd(int idx, int form, const char *name, ULONG d0, ULONG d1, ULONG d2, ULONG d3,
                ULONG d4)
{
    ROUT = name;
    fk_ret += 0x10001;
    sprintf(VEC, "form %d ret $%08lX", form, fk_ret);
    /* the original leaves the "insert before" name in a4 (the DIFF of
       libs/userlibs/rwe/riregistrylib.c): exempt */
    DONTCARE = idx == E_INSERT ? 1UL << 12 : 0;
    DO1(ENT(LIB, idx, form), R.d[0] = d0; R.d[1] = d1; R.d[2] = d2;
        R.d[3] = d3; R.d[4] = d4);
    DONTCARE = 0;
    /* the statements leave the library's d0 (the C build keeps d0) */
    if (idx != E_ACCESS && idx != E_DELLIBVARS && idx != E_FREESTRUCT)
        OBS("d0", R.d[0]);
    samelog();
}

static const char s1[] = "Library", s2[] = "Value", s3[] = "File", s4[] = "Before";
#define S1 (ULONG)s1
#define S2 (ULONG)s2
#define S3 (ULONG)s3
#define S4 (ULONG)s4

/* every command and form but the string functions and lock/unlock */
static void all(void)
{
    cmd(E_SETBASE, 0, "SetRegistryBaseLibrary", S1, MARK(1), MARK(2), 0, 0);
    cmd(E_EXISTS, 0, "ExistsRegistryLibrary", S1, MARK(1), MARK(2), 0, 0);
    cmd(E_GETVAR, 0, "GetRegistryVariable", S1, MARK(1), MARK(2), 0, 0);
    cmd(E_SETVAR, 0, "SetRegistryVariable", S1, S2, MARK(2), 0, 0);   /* size: d2 */
    cmd(E_SETVAR, 1, "SetRegistryVariable", S1, S2, 77, 0, 0);
    cmd(E_CHANGE, 0, "ChangeRegistryVariable", S1, S2, 3, 4, 0);
    cmd(E_NEWLIB, 0, "NewRegistryLibrary", S1, S2, MARK(2), 0, 0);
    cmd(E_NEWLIB, 1, "NewRegistryLibrary", S1, S2, S3, 0, 0);
    cmd(E_PARSE, 0, "ParseRegistryLibrary", S1, MARK(1), MARK(2), 0, 0);
    cmd(E_NEXT, 0, "NextParseItem", MARK(0), MARK(1), MARK(2), 0, 0);
    cmd(E_ACCESS, 0, "ChangeRegistryAccess", MARK(0), MARK(1), MARK(2), 0, 0);
    cmd(E_DELLIB, 0, "DeleteRegistryLibrary", S1, MARK(1), MARK(2), 0, 0);
    cmd(E_DELVAR, 0, "DeleteRegistryVariable", S1, MARK(1), MARK(2), 0, 0);
    cmd(E_NUMVARS, 0, "NumRegistryLibraryVariables", S1, MARK(1), MARK(2), 0, 0);
    cmd(E_SAVELIB, 0, "SaveRegistryLibrary", S1, S3, MARK(2), 0, 0);
    cmd(E_LOADLIB, 0, "LoadRegistryLibrary", S1, S3, MARK(2), 0, 0);
    cmd(E_SETBOOL, 0, "SetRegistryBoolVariable", S1, (ULONG)-1, MARK(2), 0, 0);
    cmd(E_SETINT, 0, "SetRegistryIntVariable", S1, 12345, 4, 0, 0);
    cmd(E_DELLIBVARS, 0, "DeleteRegistryLibraryVariables", S1, MARK(1), MARK(2), 0, 0);
    cmd(E_SETLIBVAL, 0, "SetLibraryValue", S1, S2, MARK(2), 0, 0);
    cmd(E_SETINDEX, 0, "SetRegistryVariableIndex", S1, S2, 1, 6, 9);
    cmd(E_RENVAR, 0, "RenameRegistryVariable", S1, S2, MARK(2), 0, 0);
    cmd(E_SAVELIBVARS, 0, "SaveLibraryVariables", S1, S3, MARK(2), 0, 0);
    cmd(E_RENLIB, 0, "RenameRegistryLibrary", S1, S2, MARK(2), 0, 0);
    cmd(E_INSERT, 0, "InsertNewRegistryLibrary", S1, S2, S4, MARK(3), 0);
    cmd(E_INSERT, 1, "InsertNewRegistryLibrary", S1, S2, S4, S3, 0);
    cmd(E_GETSTRUCT, 0, "GetStructureVariables", S1, S2, MARK(2), 0, 0);
    cmd(E_FREESTRUCT, 0, "FreeStructureVariables", S1, S2, MARK(2), 0, 0);
    cmd(E_LOADLIBVARS, 0, "LoadLibraryVariables", S1, S3, MARK(2), 0, 0);
}

/* a string function: the argument (NULL: none) at the start of the
   string area, a3 behind it, its length on the a2 stack */
static void strfn(int idx, const char *name, const char *arg, ULONG ret)
{
    ROUT = name;
    sprintf(VEC, "\"%s\" ret %lu", arg ? arg : "", ret);
    fk_ret = ret;
    DO1(ENT(LIB, idx, 0),
        memset(strbuf, 0x77, sizeof strbuf);
        if (arg) strcpy((char *)strbuf, arg);
        stk[7] = arg ? strlen(arg) : 0x11111111;
        R.d[0] = (ULONG)strbuf;
        R.a[2] = (ULONG)&stk[8];
        R.a[3] = (ULONG)strbuf + (arg ? strlen(arg) + 1 : 4));
    OBS("d0", R.d[0]);
    OBS("a3", R.a[3] - (ULONG)strbuf);
    OBSBYTES("string area", strbuf, sizeof strbuf);
    samelog();
}

/* every string function with the structure as it is */
static void strings(void)
{
    strfn(E_GETLIBVAL, "GetLibraryValue$", "LibName", 0x1111);
    strfn(E_GETSTRVAR, "GetRegistryStringVariable", "Var", 1);
    strfn(E_ITEMNAME, "ParseItemName$", NULL, 0);
    strfn(E_ITEMVALUE, "ParseItemValue$", NULL, 0);
    strfn(E_GETINT, "GetRegistryIntVariable", "Int", 4);
    strfn(E_GETINT, "GetRegistryIntVariable", "Int", 0x50003);
    strfn(E_GETBOOL, "GetRegistryBoolVariable", "Bool", 3);
    strfn(E_GETBOOL, "GetRegistryBoolVariable", "Bool", 2);
}

static void lock(ULONG mode, ULONG to)
{
    fk_lock = to;
    ROUT = "LockRegistry";
    sprintf(VEC, "mode %lu", mode);
    DO1(ENT(LIB, E_LOCK, 0), R.d[0] = mode);
    OBSPTR("d0", R.d[0], NULL, 0);    /* the structure */
    samelog();
}

static void unlock(const char *what)
{
    ROUT = "UnlockRegistry";
    strcpy(VEC, what);
    DO1(ENT(LIB, E_UNLOCK, 0), (void)0);
    samelog();
}

/* NextParseItem: shows the current structure in a0 (or no call) */
static void current(const char *what)
{
    ROUT = "NextParseItem";
    snprintf(VEC, sizeof VEC, "current: %s", what);
    DO1(ENT(LIB, E_NEXT, 0), (void)0);
    OBSPTR("d0", R.d[0], NULL, 0);    /* the structure */
    samelog();
}

static void isopen(void)
{
    ROUT = "IsRegistryOpen";
    strcpy(VEC, "");
    DO1(ENT(LIB, E_OPEN, 0), (void)0);
    /* the mock's base as 1: its address differs from run to run */
    OBS("d0", fake && R.d[0] == (ULONG)fake ? 1 : R.d[0]);
}

/* the code of a library's init (header +22) or finit (+28) nullsub */
static APTR nullsub(int off)
{
    struct lib *l = getlib(&loaded, LIB);
    UBYTE *p = (UBYTE *)L(l->code + off);
    p += 6;
    while (W(p))
        p += 4;
    return (APTR)L(p + 2 + 4);
}

static void initfinit(int off)
{
    ROUT = off == 22 ? "init" : "finit";
    strcpy(VEC, "");
    DO1(nullsub(off), (void)0);
    samelog();
}

int main(int argc, char **argv)
{
    static const char *names[] = { LIB, NULL };
    ULONG avail;

    if (argc < 2) {
        printf("usage: riregistrylib DIR\n");
        return 20;
    }
    if (!load_set(argv[1], names))
        return 20;
    KEEPS("init", d2-d7|a2|a3);
    KEEPS("IsRegistryOpen", d1-d7|a0-a3|a6);
    KEEPS("LockRegistry", d2-d7|a2|a3);
    KEEPS("SetRegistryBaseLibrary", d2-d6|a2|a3);
    KEEPS("ExistsRegistryLibrary", d2-d6|a2|a3);
    KEEPS("GetRegistryVariable", d2-d6|a2|a3);
    KEEPS("SetRegistryVariable", d2-d6|a3);
    KEEPS("ChangeRegistryVariable", d2-d6|a3);
    KEEPS("NewRegistryLibrary", d3-d6);
    KEEPS("ParseRegistryLibrary", d2-d6|a2|a3);
    KEEPS("NextParseItem", d2-d6|a2|a3);
    KEEPS("ChangeRegistryAccess", d2-d6|a2|a3);
    KEEPS("DeleteRegistryLibrary", d2-d6|a2|a3);
    KEEPS("DeleteRegistryVariable", d2-d6|a2|a3);
    KEEPS("NumRegistryLibraryVariables", d2-d6|a2|a3);
    KEEPS("SaveRegistryLibrary", d2-d6|a3);
    KEEPS("LoadRegistryLibrary", d2-d6|a3);
    KEEPS("SetRegistryBoolVariable", d2-d6|a3);
    KEEPS("SetRegistryIntVariable", d2-d6|a3);
    KEEPS("DeleteRegistryLibraryVariables", d2-d6|a2|a3);
    KEEPS("SetLibraryValue", d2-d6|a3);
    KEEPS("SetRegistryVariableIndex", d3-d6|a3);
    KEEPS("RenameRegistryVariable", d2-d6|a3);
    KEEPS("SaveLibraryVariables", d2-d6|a3);
    KEEPS("RenameRegistryLibrary", d2-d6|a3);
    KEEPS("InsertNewRegistryLibrary", d2|d4-d6);
    KEEPS("GetStructureVariables", d2-d6|a3);
    KEEPS("FreeStructureVariables", d2-d6|a3);
    KEEPS("LoadLibraryVariables", d2-d6|a3);
    KEEPS("GetLibraryValue$", d2-d6);
    KEEPS("GetRegistryStringVariable", d2-d6);
    KEEPS("ParseItemName$", d1-d7|a1|a2|a6);
    KEEPS("ParseItemValue$", d1-d7|a1|a2|a6);
    KEEPS("GetRegistryIntVariable", d2-d6|a2|a3);
    KEEPS("GetRegistryBoolVariable", d2-d6|a2|a3);
    KEEPS("finit", d2-d7|a2|a3);
    KEEPS("UnlockRegistry", d2-d6|a2|a3);
    avail = AvailMem(MEMF_ANY);
    rwe_mem_start();

    /* no registry.library */
    initfinit(22);
    isopen();
    BOTH("not open", R.d[0] == 0);
    lock(1, (ULONG)rs[0]);
    all();
    strings();
    initfinit(28);

    /* the mock */
    fake_add();
    initfinit(22);
    isopen();
    BOTH("open", R.d[0] == (ULONG)fake);
    all();                              /* no structure yet: nothing called */
    setrs(0, "Item", 1, (ULONG)"String value");
    lock(2, (ULONG)rs[0]);
    all();
    strings();
    setrs(0, "Item2", 4, 4711);
    strings();
    setrs(0, NULL, 1, 0);
    strings();
    setrs(1, "Second", 1, (ULONG)"2nd");
    lock(1, (ULONG)rs[1]);
    current("rs 1");
    strfn(E_ITEMVALUE, "ParseItemValue$", NULL, 0);
    lock(0, 0);                         /* failed: 0 pushed */
    current("none");
    unlock("of the failed lock");       /* nothing called */
    current("rs 1 again");
    unlock("rs 1");
    current("rs 0 again");
    setrs(2, "Third", 3, 1);
    lock(1, (ULONG)rs[1]);
    lock(1, (ULONG)rs[2]);
    initfinit(28);                      /* unlocks rs 2, rs 1, rs 0 */
    BOTH("closed", fake->lib_OpenCnt == 0);

    /* after finit: regstruct stays (BUG(orig), #779); below the stack */
    initfinit(22);
    current("the stale rs 2");
    unlock("rs 2, to the libfin's 0");
    current("none (0)");
    unlock("nothing, to $0000FFFF");
    current("$0000FFFF");
    unlock("$0000FFFF");
    initfinit(28);
    BOTH("closed again", fake->lib_OpenCnt == 0);

    fake_remove(fake);
    rwe_mem_check(LIB, avail, TRUE);
    return rwe_done(LIB);
}
