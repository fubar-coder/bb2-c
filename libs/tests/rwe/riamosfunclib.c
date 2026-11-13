/*
 * rwe riamosfunclib - riamosfunclib (userlib 75): the C build or AmiBlitz3's
 * SVN RIAmosFuncLib.obj, one set as LIBSET says, observed as cases
 * (rwecmp.h; the expected data
 * libs/tests/expected/rwe_riamosfunclib.txt).
 *
 *   riamosfunclib DIR   (DIR: riamosfunclib.obj / .orig, memlib.obj / .orig)
 *
 * memlib is loaded with the library (the bank tables are a globalloc
 * block); the files and directories are in RAM:rwamos0.  Observed: d0
 * (pointers into the test program as prog), a3 and the string area of
 * DeviceName$, the contents and lengths of the banks, the memory written
 * by the copy and fill functions, the files written by BSave and
 * SaveExec, the current directory after ChangeDir, and the kept
 * registers.
 *
 * Covered: Min/Max (signed edges), both inline forms of GTTagValue
 * (copied with an RTS like the compiler does), CopyByte/Word/Long
 * (counts 1..n, a negative count: nothing), KeyCode, Timer (range, it
 * runs), Lisa (AGA: the id, the high word of d0 kept), BlitterDone,
 * WaitBlitter, MemFree, CachesOff/CachesOn (masks 0 and the instruction
 * cache, the state restored before every call); init; the banks: Start,
 * Length (free, used, >= the limit), Reserve in all three forms (a bank
 * in use erased first, chip memory with MEMF_CLEAR, an allocation that
 * fails - AllocMem replaced by a failing stub, also for a bank in use -,
 * a negative length), NextBank (both forms, a used start, the
 * limit, -1), FillMem (bank, address, both forms, a free bank), SwapBank
 * (used and free), Erase (used, free), BankLimit (get, set: the banks
 * freed, a bank above the old limit, back to 50); the files on RAM:
 * MakeDir (new, existing, missing parent; whether the directory's lock
 * was given back), FileSize (file, directory, missing), BSave (bank,
 * address with and without a length, a free bank, a bad path), BLoad in
 * all five forms (into a new bank, an address, a used bank, a length, an
 * offset, chip memory; empty and missing files, BUG(orig) (#487): the bank of a
 * failed open stays), its error check (an RTS), Rename (done, missing),
 * ChangeDir (absolute, relative, missing; the current directory after
 * it), FindVolume, DeviceName$ (with and without ":", volume and device
 * names, unknown, empty, d0 pointing elsewhere), SaveExec (a list with
 * two code hunks - relocations, memory flags, a length not a multiple
 * of 4 - and a symbol hunk; no list; a file that cannot be made; an
 * empty list, BUG(orig) (#501): the file stays open); EraseAll, finit.
 * Not covered: Reboot; counts of 0 (2^32 units) and bank numbers that
 * index outside the tables (negative, Reserve above the limit, Reserve
 * with every bank in use) - they write outside the test's memory;
 * BankLimit 0 or above 65536; a ChangeDir before BankLimit/finit while
 * its lock is current (BUG(orig) #491: unlocked while in use - the test
 * restores the directory first); CurrentDir returning 0.
 */
#include "rwecmp.h"
#include "amosfunc.h"
#include <proto/dos.h>
#include <dos/dosextens.h>
#include <dos/doshunks.h>

#define LIB "riamosfunclib"
enum { E_CACHESON, E_REBOOT, E_LISA, E_MAKEDIR, E_RENAME, E_MEMFREE, E_GTTAG, E_TIMER,
       E_MIN, E_COPYB, E_COPYW, E_COPYL, E_MAX, E_FILESIZE, E_BLOAD, E_BSAVE, E_START,
       E_LENGTH, E_ERASE, E_ERASEALL, E_RESERVE, E_NEXTBANK, E_CACHESOFF, E_N23, E_FILLMEM,
       E_KEYCODE, E_N26, E_N27, E_BLITDONE, E_BLITWAIT, E_FINDVOL, E_N31, E_SWAPBANK,
       E_DEVNAME, E_CHANGEDIR, E_BANKLIMIT, E_SAVEEXEC, E_FINIT, E_INIT };

static UBYTE str[300];                  /* string area (a3) */
static ULONG frame[2];                  /* the compiler's argument stack (a2) */
static char path[80];                   /* a file name */
static UBYTE mem[1024];                 /* memory for copies, fills, loads */
static UBYTE pattern[600];

/* path = RAM:rwamos0/name */
static const char *P(const char *name)
{
    sprintf(path, "RAM:rwamos0%s%s", *name ? "/" : "", name);
    return path;
}

/* a string argument at str+20 (where the compiler built it), a3 behind
   its 0, the length below a2 */
#define STRARG(s) do {                                                      \
        memset(str, 0xee, sizeof str);                                      \
        strcpy((char *)str + 20, s);                                        \
        R.d[0] = (ULONG)str + 20;                                           \
        frame[0] = strlen(s);                                               \
        R.a[2] = (ULONG)&frame[1];                                          \
        R.a[3] = (ULONG)str + 20 + strlen(s) + 1;                           \
    } while (0)

/* a file name argument in d0 (path), a3 on a scratch area */
#define NAMEARG(n) do {                                                     \
        R.d[0] = (ULONG)P(n);                                               \
        R.a[3] = (ULONG)str + 200;                                          \
    } while (0)

/* call entry e form f with d0..d3 (other registers markers) */
static void call4(int e, int f, ULONG d0, ULONG d1, ULONG d2, ULONG d3)
{
    DO1(ENT(LIB, e, f), R.d[0] = d0; R.d[1] = d1; R.d[2] = d2;
        R.d[3] = d3);
}

static void start(int bank)
{
    ROUT = "Start";
    sprintf(VEC, "%d", bank);
    DO1(ENT(LIB, E_START, 0), R.d[0] = (ULONG)bank);
    OBS("0 or an address", R.d[0] == 0);
}

/* the start of bank n (no check) */
static ULONG bankstart;
static void getstart(int bank)
{
    start(bank);
    bankstart = R.d[0];
}

static void length(int bank)
{
    ROUT = "Length";
    sprintf(VEC, "%d", bank);
    DO1(ENT(LIB, E_LENGTH, 0), R.d[0] = (ULONG)bank);
    OBS("d0", R.d[0]);
}

/* bank n: its length and contents (contents only if `contents`: memory
   AllocMem did not clear may differ) */
static void samebank2(const char *what, int bank, int contents)
{
    ULONG len;
    length(bank);
    len = R.d[0];
    getstart(bank);
    ROUT = what;
    sprintf(VEC, "bank %d", bank);
    OBS("used", bankstart != 0);
    if (contents && bankstart)
        OBSBYTES("contents", (const UBYTE *)bankstart, len);
}
#define samebank(what, bank) samebank2(what, bank, 1)

/* ------------------------------------------------- pure functions */

static void minmax(void)
{
    static const LONG v[][2] = {
        { 0, 0 }, { 1, 2 }, { 2, 1 }, { -1, 1 }, { 1, -1 }, { -5, -7 },
        { 0x7fffffff, (LONG)0x80000000 }, { (LONG)0x80000000, 0x7fffffff }, { 100, 100 },
    };
    unsigned i;
    int e;
    for (e = 0; e < 2; e++) {
        ROUT = e ? "Max" : "Min";
        for (i = 0; i < sizeof v / sizeof v[0]; i++) {
            sprintf(VEC, "%ld,%ld", (long)v[i][0], (long)v[i][1]);
            call4(e ? E_MAX : E_MIN, 0, (ULONG)v[i][0], (ULONG)v[i][1], 0, 0);
            OBS("d0", R.d[0]);
        }
    }
}

/* an inline entry ($A000,len): the code copied with an RTS, as the
   compiler copies it into the program */
static UWORD tramp[16];
static void gttag(int form, ULONG d0)
{
    UBYTE *code;
    UWORD len;
    ROUT = "GTTagValue";
    sprintf(VEC, "form %d d0 $%lX", form, d0);
    code = ENT(LIB, E_GTTAG, form);
    len = W(code + 2);
    if (W(code) != 0xa000 || len == 0 || len >= 24) {
        check("GTTagValue: inline entry", FALSE);
        return;
    }
    memcpy(tramp, code + 4, len);
    tramp[len / 2] = 0x4e75;
    CacheClearU();
    DO1((APTR)tramp, R.d[0] = d0);
    OBS("d0", R.d[0]);
}

static void copies(void)
{
    static const LONG n[] = { 1, 2, 7, 100, -1, (LONG)0x80000000 };
    static const char *nm[] = { "CopyByte", "CopyWord", "CopyLong" };
    unsigned i;
    int e;
    for (e = 0; e < 3; e++) {
        ROUT = nm[e];
        for (i = 0; i < sizeof n / sizeof n[0]; i++) {
            sprintf(VEC, "%ld", (long)n[i]);
            DO1(ENT(LIB, E_COPYB + e, 0), memset(mem, 0x33, sizeof mem);
                R.d[0] = (ULONG)pattern; R.d[1] = (ULONG)mem + 8;
                R.d[2] = (ULONG)n[i]);
            OBSBYTES("dest", mem, sizeof mem);
            OBSPTR("d0", R.d[0], NULL, 0);      /* into the test program */
        }
    }
}

static ULONG freemem;

static void hardware(void)
{
    ULONG t;

    ROUT = "KeyCode";
    VEC[0] = 0;
    DO1(ENT(LIB, E_KEYCODE, 0), (void)0);
    BOTH("d0 a byte", R.d[0] < 256);
    ROUT = "Timer";
    DO1(ENT(LIB, E_TIMER, 0), (void)0);
    BOTH("d0 24 bit", R.d[0] < 0x1000000);
    t = R.d[0];
    Delay(5);
    DO1(ENT(LIB, E_TIMER, 0), (void)0);
    BOTH("runs", R.d[0] != t);
    ROUT = "Lisa";
    DO1(ENT(LIB, E_LISA, 0), (void)0);
    OBS("d0", R.d[0]);
    BOTH("the high word kept", (R.d[0] >> 16) == (IN.d[0] >> 16));
    printf("riamosfunclib: Lisa $%04lX\n", R.d[0] & 0xffff);
    ROUT = "WaitBlitter";
    DO1(ENT(LIB, E_BLITWAIT, 0), (void)0);
    ROUT = "BlitterDone";
    DO1(ENT(LIB, E_BLITDONE, 0), (void)0);
    OBS("d0", R.d[0]);
    ROUT = "MemFree";
    /* the free memory differs from run to run (the memory layout, e.g.
       LIBS_PAD): the difference to AvailMem(MEMF_PUBLIC) of the moment */
    DO1(ENT(LIB, E_MEMFREE, 0), freemem = AvailMem(MEMF_PUBLIC));
    OBS("d0 - AvailMem(MEMF_PUBLIC)", R.d[0] - freemem);
}

static void caches(void)
{
    static const ULONG m[] = { 0, CACRF_EnableI, CACRF_ClearI, CACRF_EnableI | CACRF_ClearI };
    ULONG orig = CacheControl(0, 0);
    unsigned i;
    int e;
    for (e = 0; e < 2; e++) {
        ROUT = e ? "CachesOn" : "CachesOff";
        for (i = 0; i < sizeof m / sizeof m[0]; i++) {
            sprintf(VEC, "$%lX", m[i]);
            DO1(ENT(LIB, e ? E_CACHESON : E_CACHESOFF, 0), CacheControl(orig, ~0UL);
                R.d[0] = m[i]);
            OBS("d0", R.d[0]);
        }
    }
    CacheControl(orig, ~0UL);
}

/* ------------------------------------------------------------- banks */

static void reserve(int form, ULONG a, ULONG b, ULONG c)
{
    ROUT = "Reserve";
    sprintf(VEC, "form %d %ld,%ld,$%lX", form, (long)a, (long)b, c);
    call4(E_RESERVE, form, a, b, c, 0);
    OBS("d0", R.d[0]);
}

/* Reserve with AllocMem failing (SetFunction under Forbid; a real
   failure of a huge request runs the low-memory handlers, whose
   expunges crashed the system later in the emulator) */
static void reserve_nomem(int form, ULONG a, ULONG b, ULONG c)
{
    APTR old;

    Forbid();
    old = SetFunction((struct Library *)SysBase, -198, (APTR)rwe_fail0);
    CacheClearU();
    reserve(form, a, b, c);
    SetFunction((struct Library *)SysBase, -198, old);
    CacheClearU();
    Permit();
}

static void nextbank(int form, LONG from)
{
    ROUT = "NextBank";
    sprintf(VEC, "form %d %ld", form, (long)from);
    call4(E_NEXTBANK, form, (ULONG)from, 0, 0, 0);
    OBS("d0", R.d[0]);
}

static void fillmem(int form, int bank, ULONG len, ULONG v)
{
    ROUT = "FillMem";
    sprintf(VEC, "form %d %d,%lu,%lu", form, bank, len, v);
    if (bank >= 0) {
        call4(E_FILLMEM, form, (ULONG)bank, len, v, 0);
        samebank("FillMem", bank);
    } else {
        DO1(ENT(LIB, E_FILLMEM, form), memset(mem, 0x44, sizeof mem);
            R.d[0] = (ULONG)mem + 3; R.d[1] = len; R.d[2] = v);
        OBSBYTES("memory", mem, sizeof mem);
    }
}

static void banks(void)
{
    int b;

    ROUT = "BankLimit";
    VEC[0] = 0;
    DO1(ENT(LIB, E_BANKLIMIT, 1), (void)0);
    OBS("d0", R.d[0]);
    for (b = 0; b < 3; b++) {
        static const int fb[] = { 0, 49, 50 };
        start(fb[b]);
        OBS("d0 free", R.d[0]);
        length(fb[b]);
    }
    start(100000);
    OBS("d0", R.d[0]);
    nextbank(1, 0);
    nextbank(0, 5);                                     /* d0 as it is */
    reserve(0, 100, 0, 0);                              /* bank 0 */
    samebank2("Reserve", 0, 0);
    reserve(1, 3, 256, 0);
    samebank2("Reserve", 3, 0);
    reserve(2, 7, 64, MEMF_CHIP | MEMF_CLEAR);
    samebank("Reserve", 7);
    getstart(7);
    BOTH("chip", TypeOfMem((APTR)bankstart) & MEMF_CHIP);
    reserve(1, 3, 32, 0);                               /* in use: erased first */
    length(3);
    fillmem(1, 3, 32, 0x99);
    reserve(0, (ULONG)-5, 0, 0);                        /* negative length */
    reserve_nomem(0, 64, 0, 0);                         /* AllocMem fails: -1 */
    reserve_nomem(1, 4, 64, 0);                         /* 0 */
    reserve_nomem(2, 7, 16, MEMF_CHIP);                 /* in use: erased, then 0 */
    length(7);
    length(4);
    nextbank(1, 0);
    nextbank(1, 3);
    nextbank(1, 7);
    nextbank(1, 49);
    nextbank(1, 50);
    nextbank(1, -1);                                    /* reads the length of 49 */
    fillmem(1, 0, 100, 0x5a);
    fillmem(0, 3, 32, 0);
    fillmem(1, 3, 7, 0x1ff);
    fillmem(1, 5, 10, 0x77);                            /* free: nothing */
    fillmem(1, -1, 10, 7);                              /* an address */
    fillmem(0, -1, 300, 0);
    ROUT = "SwapBank";
    strcpy(VEC, "0,3");
    call4(E_SWAPBANK, 0, 0, 3, 0, 0);
    OBS("d0", R.d[0]);
    samebank("SwapBank", 0);
    samebank("SwapBank", 3);
    strcpy(VEC, "0,5");
    call4(E_SWAPBANK, 0, 0, 5, 0, 0);
    OBS("d0", R.d[0]);
    strcpy(VEC, "5,0");
    call4(E_SWAPBANK, 0, 5, 0, 0, 0);
    OBS("d0", R.d[0]);
    ROUT = "Erase";
    strcpy(VEC, "3");
    call4(E_ERASE, 0, 3, 0, 0, 0);
    length(3);
    start(3);
    OBS("d0", R.d[0]);
    ROUT = "Erase";
    strcpy(VEC, "5 (free)");
    call4(E_ERASE, 0, 5, 0, 0, 0);

    /* BankLimit: all banks freed, new tables */
    ROUT = "BankLimit";
    strcpy(VEC, "60");
    call4(E_BANKLIMIT, 0, 60, 0, 0, 0);
    BOTH("d0 = the start table", R.d[0] != 0 && R.d[0] != 60);
    DO1(ENT(LIB, E_BANKLIMIT, 1), (void)0);
    OBS("d0", R.d[0]);
    length(0);
    length(7);
    reserve(1, 55, 10, 0);
    samebank2("Reserve", 55, 0);
    nextbank(1, 0);
    nextbank(1, 55);
    ROUT = "BankLimit";
    strcpy(VEC, "50");
    call4(E_BANKLIMIT, 0, 50, 0, 0, 0);
    DO1(ENT(LIB, E_BANKLIMIT, 1), (void)0);
    OBS("d0", R.d[0]);
    length(55);                                         /* above the limit: 0 */
    reserve(0, 40, 0, 0);                               /* bank 0 */
    fillmem(1, 0, 40, 0x11);
    reserve(0, 24, 0, 0);                               /* bank 1 */
    fillmem(1, 1, 24, 0x22);
}

/* ------------------------------------------------------------- files */

static void writefile(const char *name, const UBYTE *data, ULONG len)
{
    BPTR f = Open((STRPTR)P(name), MODE_NEWFILE);
    if (f) {
        Write(f, (APTR)data, (LONG)len);
        Close(f);
    }
}

/* the file `name`: its size and contents */
static UBYTE fbuf[2048];
static LONG flen;
static void samefile(const char *what, const char *name)
{
    BPTR f;
    memset(fbuf, 0, sizeof fbuf);
    flen = -1;
    f = Open((STRPTR)P(name), MODE_OLDFILE);
    if (f) {
        flen = Read(f, fbuf, sizeof fbuf);
        Close(f);
    }
    ROUT = what;
    sprintf(VEC, "file %s", name);
    OBS("length", flen);
    OBSBYTES("contents", fbuf, sizeof fbuf);
}

static void filesize(const char *name)
{
    ROUT = "FileSize";
    sprintf(VEC, "%s", name);
    DO1(ENT(LIB, E_FILESIZE, 0), NAMEARG(name));
    OBS("d0", R.d[0]);
}

static void makedir(const char *name)
{
    BPTR l;
    ROUT = "MakeDir";
    sprintf(VEC, "%s", name);
    DO1(ENT(LIB, E_MAKEDIR, 0), NAMEARG(name));
    OBS("d0", R.d[0]);
    /* the new directory's lock given back: an exclusive lock works */
    l = Lock((STRPTR)P(name), EXCLUSIVE_LOCK);
    frame[0] = l != 0;
    if (l)
        UnLock(l);
    OBS("lock free", frame[0]);
}

static void bsave(int form, const char *name, LONG bank, ULONG len)
{
    ROUT = "BSave";
    sprintf(VEC, "form %d %s %ld %lu", form, name, (long)bank, len);
    DeleteFile((STRPTR)P(name));
    DO1(ENT(LIB, E_BSAVE, form), NAMEARG(name);
        R.d[1] = bank < 0 ? (ULONG)pattern : (ULONG)bank; R.d[2] = len);
    OBS("d0", R.d[0]);
    if (R.d[0])
        samefile("BSave", name);
}

/* BLoad: bank < 0 = the address mem+5 */
static void bload(int form, const char *name, LONG bank, ULONG len, ULONG off, ULONG type)
{
    ROUT = "BLoad";
    sprintf(VEC, "form %d %s %ld,%lu,%lu,$%lX", form, name, (long)bank, len, off, type);
    DO1(ENT(LIB, E_BLOAD, form), memset(mem, 0x66, sizeof mem); NAMEARG(name);
        R.d[1] = bank < 0 ? (ULONG)mem + 5 : (ULONG)bank;
        R.d[2] = len; R.d[3] = off; R.d[4] = type);
    OBS("d0", R.d[0]);
    OBSBYTES("memory", mem, sizeof mem);
    if (form == 0 && R.d[0] < 50)
        samebank("BLoad", (int)R.d[0]);
    else if (form && bank >= 0)
        samebank2("BLoad", (int)bank, R.d[0] != 0);
}

static void files(void)
{
    int f;

    makedir("");                                        /* RAM:rwamosN */
    makedir("");                                        /* exists: 0 */
    makedir("nodir/sub");                               /* no parent: 0 */
    makedir("sub");

    getstart(0);                                        /* the 100 bytes of FillMem */
    bsave(0, "a.bin", 0, 0);
    filesize("a.bin");
    bsave(1, "b.bin", -1, 37);                          /* an address */
    filesize("b.bin");
    bsave(0, "c.bin", -1, 0);                           /* address, no length: 0 */
    bsave(0, "c.bin", 5, 0);                            /* free bank: 0 */
    bsave(1, "c.bin", 0, 10);
    bsave(1, "nodir/x.bin", 0, 10);                     /* cannot open: 0 */
    filesize("");                                       /* the directory */
    filesize("missing.bin");
    writefile("p.bin", pattern, 300);
    writefile("empty.bin", pattern, 0);

    /* BLoad, all forms */
    bload(0, "p.bin", 0, 0, 0, 0);                      /* a new bank */
    bload(0, "missing.bin", 0, 0, 0, 0);                /* -1 */
    bload(0, "empty.bin", 0, 0, 0, 0);                  /* -1 */
    bload(1, "b.bin", -1, 0, 0, 0);                     /* an address */
    bload(1, "b.bin", 8, 0, 0, 0);                      /* bank 8 */
    bload(1, "p.bin", 8, 0, 0, 0);                      /* bank 8 in use */
    bload(2, "p.bin", -1, 5, 0, 0);
    bload(2, "p.bin", 9, 20, 0, 0);
    bload(3, "p.bin", -1, 5, 3, 0);
    bload(3, "p.bin", -1, 0, 250, 0);                   /* the file's length from 250 */
    bload(4, "p.bin", 10, 0, 2, MEMF_CHIP | MEMF_CLEAR);
    getstart(10);
    BOTH("chip", TypeOfMem((APTR)bankstart) & MEMF_CHIP);
    bload(4, "p.bin", -1, 16, 1, MEMF_CHIP);            /* an address: no memtype */
    bload(1, "missing.bin", 11, 0, 0, 0);               /* no length: 0 */
    bload(2, "missing.bin", 11, 16, 0, 0);              /* BUG(orig) (#487): bank 11 stays */
    length(11);
    bload(1, "empty.bin", -1, 0, 0, 0);
    for (f = 0; f < 5; f++) {
        UBYTE *chk;
        ROUT = "BLoad error check";
        sprintf(VEC, "form %d", f);
        chk = ERR(LIB, E_BLOAD, f);
        BOTH("present", chk != NULL);
        if (chk) {
            DO1(ERR(LIB, E_BLOAD, f), NAMEARG("missing.bin"));
            OBS("d0 kept", R.d[0] == IN.d[0]);
        }
    }

    /* Rename */
    ROUT = "Rename";
    strcpy(VEC, "c.bin d.bin");
    DO1(ENT(LIB, E_RENAME, 0), NAMEARG("c.bin"); strcpy((char *)str, path);
        R.d[0] = (ULONG)str; R.d[1] = (ULONG)P("d.bin"));
    OBS("d0", R.d[0]);
    filesize("d.bin");
    filesize("c.bin");
    ROUT = "Rename";
    strcpy(VEC, "missing");
    DO1(ENT(LIB, E_RENAME, 0), NAMEARG("missing.bin"); strcpy((char *)str, path);
        R.d[0] = (ULONG)str; R.d[1] = (ULONG)P("e.bin"));
    OBS("d0", R.d[0]);
}

/* ChangeDir: the current directory checked right after the call */
static BPTR mybase, mysub;
static void changedir(const char *name, int relative)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    ULONG same;
    APTR fn;
    ROUT = "ChangeDir";
    sprintf(VEC, "%s%s", name, relative ? " (relative)" : "");
    markregs(&R);
    if (relative) {
        CurrentDir(mybase);
        strcpy(path, name);
        R.d[0] = (ULONG)path;
    } else
        R.d[0] = (ULONG)P(name);
    R.a[3] = (ULONG)str + 200;
    IN = R;
    fn = ENT(LIB, E_CHANGEDIR, 0);
    CCR = callstk(fn, &R, NULL, 0, &POP);
    same = !R.d[0] ? 3
           : (ULONG)SameLock(me->pr_CurrentDir, mysub) == LOCK_SAME ? 1
           : (ULONG)SameLock(me->pr_CurrentDir, mybase) == LOCK_SAME ? 2 : 0;
    preserve();
    OBS("d0", R.d[0]);
    OBS("current directory", same);
}

#define LDF_FINDVOLUME_T (LDF_READ | LDF_DEVICES | LDF_VOLUMES | LDF_ASSIGNS | LDF_ENTRY)

/* FindVolume / DeviceName$ */
static void volumes(void)
{
    static const char *v[] = { "RAM", "Ram Disk", "TEST", "nosuchvolume", "" };
    static const char *d[] = { "RAM", "RAM:", "Ram Disk", "Ram Disk:", "TEST", "TEST:",
                               "nosuchvolume", "nosuch:", "", ":" };
    unsigned i;
    ROUT = "FindVolume";
    for (i = 0; i < sizeof v / sizeof v[0]; i++) {
        sprintf(VEC, "\"%s\"", v[i]);
        DO1(ENT(LIB, E_FINDVOL, 0), R.d[0] = (ULONG)v[i]);
        {
            /* the entry dos.library itself finds; the node's address
               depends on the system, so the case holds only whether d0
               is 0 and whether it is that entry */
            ULONG found = R.d[0];
            struct DosList *dl = LockDosList(LDF_FINDVOLUME_T);

            if (dl)
                dl = FindDosEntry(dl, v[i], LDF_FINDVOLUME_T);
            UnLockDosList(LDF_FINDVOLUME_T);
            OBS("d0 is not 0", found != 0);
            OBS("d0 is the DosList entry of the name", found == (ULONG)dl);
        }
    }
    ROUT = "DeviceName$";
    for (i = 0; i < sizeof d / sizeof d[0]; i++) {
        sprintf(VEC, "\"%s\"", d[i]);
        DO1(ENT(LIB, E_DEVNAME, 0), STRARG(d[i]));
        OBS("d0", R.d[0]);
        OBS("a3", R.a[3] - (ULONG)str);
        OBSBYTES("string area", str, sizeof str);
        if (i == 0)
            printf("riamosfunclib: DeviceName$(\"RAM\") = \"%s\"\n", (char *)str + 20);
    }
    strcpy(VEC, "\"RAM\", d0 elsewhere");
    DO1(ENT(LIB, E_DEVNAME, 0), STRARG("RAM"); R.d[0] = (ULONG)"nosuchvolume");
    OBS("d0", R.d[0]);
    OBS("a3", R.a[3] - (ULONG)str);
    OBSBYTES("string area", str, sizeof str);
}

/* SaveExec: a Blitz list - the array pointer with "lIsT" at -4 and the
   first node at -28; nodes next.l, prev.l, then NEWTYPE .hunk
   (hunktype.w, hunkloc.l, hunklen.l, reloc.l, reloclen.l, flags.l) */
struct node {
    struct node *next, *prev;
    UWORD hunktype;
    ULONG hunkloc, hunklen, reloc, reloclen, flags;
} __attribute__((packed, aligned(2)));

static struct node nodes[4];
static ULONG listarr[8];
static const ULONG relocs[] = { 2, 10, 14 };

static void saveexec(const char *what, const char *name, ULONG magic, struct node *first)
{
    listarr[1] = (ULONG)first;                          /* -28 from listarr+8 */
    listarr[7] = magic;                                 /* -4 */
    ROUT = "SaveExec";
    sprintf(VEC, "%s", what);
    DO1(ENT(LIB, E_SAVEEXEC, 0), NAMEARG(name); R.d[1] = (ULONG)(listarr + 8));
    OBS("d0 (1: the name)", R.d[0] == IN.d[0] ? 1 : R.d[0]);
}

static void saveexecs(void)
{
    nodes[0] = (struct node){ &nodes[1], 0, HUNK_CODE, (ULONG)pattern, 22, 0, 0, 0 };
    nodes[1] = (struct node){ &nodes[2], &nodes[0], HUNK_SYMBOL, (ULONG)pattern, 8, 0, 0, 0 };
    nodes[2] = (struct node){ &nodes[3], &nodes[1], HUNK_CODE, (ULONG)pattern + 40, 24,
                              (ULONG)relocs, sizeof relocs, HUNKF_CHIP };
    nodes[3] = (struct node){ 0, &nodes[2], HUNK_DATA, (ULONG)pattern, 4, 0, 0, 0 };
    saveexec("list", "x.exe", 0x6c497354, &nodes[0]);
    samefile("SaveExec", "x.exe");
    nodes[1].next = &nodes[3];                          /* code and symbol: one hunk */
    saveexec("list 2", "y.exe", 0x6c497354, &nodes[0]);
    samefile("SaveExec", "y.exe");
    saveexec("no list", "z.exe", 0x6c497355, &nodes[0]);
    saveexec("no file", "nodir/z.exe", 0x6c497354, &nodes[0]);
    saveexec("empty (the file stays open)", "open.exe", 0x6c497354, NULL);
}

static void cleanup(void)
{
    static const char *f[] = { "a.bin", "b.bin", "c.bin", "d.bin", "p.bin", "empty.bin",
                               "x.exe", "y.exe", "sub", "" };
    unsigned i;
    for (i = 0; i < sizeof f / sizeof f[0]; i++)
        DeleteFile((STRPTR)P(f[i]));
}

int main(int argc, char **argv)
{
    static const char *names[] = { "memlib", LIB, NULL };
    struct Process *me = (struct Process *)FindTask(NULL);
    BPTR home;
    ULONG avail;
    unsigned i;
    int b;

    if (argc < 2) {
        printf("usage: riamosfunclib DIR\n");
        return 20;
    }
    if (!load_set(argv[1], names))
        return 20;
    KEEPS("Min", d1-d7|a0-a3|a6);
    KEEPS("Max", d1-d7|a0-a3|a6);
    KEEPS("GTTagValue", d1-d7|a0-a3|a6);
    KEEPS("CopyByte", d1|d3-d7|a2|a3|a6);
    KEEPS("CopyWord", d1|d3-d7|a2|a3|a6);
    KEEPS("CopyLong", d1|d3-d7|a2|a3|a6);
    KEEPS("KeyCode", d1-d7|a0-a3|a6);
    KEEPS("Timer", d1-d7|a0-a3|a6);
    KEEPS("Lisa", d1-d7|a0-a3|a6);
    KEEPS("WaitBlitter", d0-d7|a0-a3|a6);
    KEEPS("BlitterDone", d1-d7|a0-a3|a6);
    KEEPS("MemFree", d2-d7|a0|a2|a3);
    KEEPS("CachesOff", d2-d7|a0-a3);
    KEEPS("CachesOn", d2-d7|a0-a3);
    KEEPS("init", d2-d7|a2|a3);
    KEEPS("BankLimit", d2|d3|d5-d7|a2|a3);
    KEEPS("Start", d1-d7|a1-a3|a6);
    KEEPS("Length", d1-d7|a1-a3|a6);
    KEEPS("NextBank", d1-d7|a1-a3);
    KEEPS("Reserve", d3-d7|a2|a3);
    KEEPS("FillMem", d2-d7|a1-a3);
    KEEPS("SwapBank", d2|d3|a2|a3|a6);
    KEEPS("Erase", d2-d7|a2|a3|a6);
    KEEPS("MakeDir", d2-d5|d7|a2|a3);
    KEEPS("BSave", d4-d7|a2|a3);
    KEEPS("FileSize", d3-d5|a2|a3);
    KEEPS("BLoad", d4|a2|a3);
    KEEPS("BLoad error check", d0-d7|a0-a3|a6);
    KEEPS("Rename", d3-d7|a2|a3);
    KEEPS("FindVolume", d4-d6|a2|a3);
    KEEPS("DeviceName$", d1-d7|a0|a6);
    KEEPS("SaveExec", d4|d5|a3);
    KEEPS("ChangeDir", d3-d7|a2|a3);
    KEEPS("EraseAll", d3|d5-d7|a2|a3|a6);
    KEEPS("finit", d2|d3|d5-d7|a2|a3);
    for (i = 0; i < sizeof pattern; i++)
        pattern[i] = (UBYTE)(i * 7 + (i >> 3));
    cleanup();

    minmax();
    gttag(0, 0x12345678);
    gttag(1, 0);
    gttag(1, 0x21);
    gttag(1, 0xffffffff);
    copies();
    hardware();
    caches();

    avail = AvailMem(MEMF_ANY);
    ROUT = "memlib init";
    VEC[0] = 0;
    markregs(&R);
    callregs(entry_form(getlib(&loaded, "memlib"), 7, 0), &R, 0, 0);
    ROUT = "init";
    DO1(ENT(LIB, E_INIT, 0), (void)0);
    banks();
    files();
    volumes();
    saveexecs();

    /* ChangeDir last: its lock stays current until the test restores the
       directory (finit unlocks it, BUG(orig), #491) */
    home = me->pr_CurrentDir;
    mybase = Lock((STRPTR)P(""), ACCESS_READ);
    mysub = Lock((STRPTR)P("sub"), ACCESS_READ);
    changedir("", 0);
    changedir("sub", 0);
    changedir("missing", 0);
    changedir("sub", 1);
    changedir("", 0);
    CurrentDir(home);

    ROUT = "EraseAll";
    VEC[0] = 0;
    DO1(ENT(LIB, E_ERASEALL, 0), (void)0);
    for (b = 0; b < 12; b++)
        length(b);
    ROUT = "finit";
    VEC[0] = 0;
    DO1(ENT(LIB, E_FINIT, 0), (void)0);
    ROUT = "memlib finit";
    markregs(&R);
    callregs(entry_form(getlib(&loaded, "memlib"), 8, 0), &R, 0, 0);
    UnLock(mybase);
    UnLock(mysub);
    cleanup();
    printf("riamosfunclib: %ld bytes less memory than before init\n",
           (long)(avail - AvailMem(MEMF_ANY)));
    return rwe_done(LIB);
}
