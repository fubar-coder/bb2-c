/*
 * inject - run a command in this process with faults injected, for the
 * bug-orig tests (tests/bug-orig, pass file keys wrap= for the compiler
 * and runwrap= for the programs): conditions the original reacts to
 * wrongly but that a normal system never shows - no free signal, an
 * allocation that fails, a read error.
 *
 * usage: inject [OPTION...] COMMAND [ARGS]
 *   signals                every free signal bit of the process allocated
 *                          before COMMAND starts (AllocSignal(-1) fails)
 *   allocfail=MIN-MAX[/chip][/COUNT]
 *                          AllocMem of MIN..MAX bytes (with /chip: only
 *                          with MEMF_CHIP) returns 0, the first COUNT times
 *                          (default every time)
 *   readfail=NAME/N        the N-th Read on a file opened under a name that
 *                          ends in NAME (case ignored) returns -1
 *   examinefail=NAME       Examine of a lock on a name that ends in NAME
 *                          returns DOSFALSE
 *   openlibfail=NAME       OpenLibrary/OldOpenLibrary of a library whose
 *                          name ends in NAME (case ignored) returns 0
 *   track                  the process's AllocMem/FreeMem are recorded; what
 *                          is still allocated when COMMAND ends is reported
 * COMMAND is loaded with LoadSeg and run with RunCommand in this process
 * (the signals and the recorded allocations are the process's); the
 * patches are system-wide but act only for this process.  Behind COMMAND
 * it writes (lines "inject: ..."): how often each fault was injected,
 * rc=N of COMMAND and with track the allocations left (sorted by size:
 * the addresses differ between runs).
 */
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>

#define LVO_ALLOCMEM (-198)
#define LVO_FREEMEM  (-210)
#define LVO_OPEN     (-30)
#define LVO_READ     (-42)
#define LVO_LOCK     (-84)
#define LVO_EXAMINE  (-102)
#define LVO_OLDOPENLIBRARY (-408)
#define LVO_OPENLIBRARY    (-552)

static BPTR out;
static struct Task *me;
static volatile int active;             /* COMMAND runs */

static void put(const char *s) { Write(out, (APTR)s, strlen(s)); }

/* -------------------------------------------------------------- options */

static int o_signals, o_track, o_allocfail;
static ULONG af_min, af_max, af_chip, af_count, af_done;
static char rf_name[108], ef_name[108], olf_name[108];
static ULONG rf_n, rf_done, ef_done, olf_done;

/* the patches: asm entry, C decision (only for this process while COMMAND
   runs); the original is called with the registers it got */
APTR old_allocmem, old_freemem, old_open, old_read, old_lock, old_examine;
APTR old_oldopenlib, old_openlib;
void allocmem_patch(void), freemem_patch(void), open_patch(void), read_patch(void);
void lock_patch(void), examine_patch(void), oldopenlib_patch(void), openlib_patch(void);

__asm__(
"   .text\n"
"   .even\n"
/* AllocMem(d0 size, d1 flags): alloc_pre != 0 -> 0, else the original and
   alloc_post(result, size, flags) */
"_allocmem_patch:\n"
"   movem.l d0-d1/a0-a1,-(sp)\n"
"   move.l  d1,-(sp)\n"
"   move.l  d0,-(sp)\n"
"   jsr     _alloc_pre\n"
"   addq.l  #8,sp\n"
"   tst.l   d0\n"
"   movem.l (sp)+,d0-d1/a0-a1\n"     /* movem leaves the flags */
"   bne.s   1f\n"
"   movem.l d0-d1,-(sp)\n"
"   move.l  _old_allocmem,a0\n"
"   jsr     (a0)\n"
"   move.l  d0,-(sp)\n"
"   jsr     _alloc_post\n"
"   move.l  (sp)+,d0\n"
"   addq.l  #8,sp\n"
"   rts\n"
"1: moveq   #0,d0\n"
"   rts\n"
/* FreeMem(a1 memory, d0 size): free_pre(memory, size), the original */
"   .even\n"
"_freemem_patch:\n"
"   movem.l d0-d1/a0-a1,-(sp)\n"
"   move.l  d0,-(sp)\n"
"   move.l  a1,-(sp)\n"
"   jsr     _free_pre\n"
"   addq.l  #8,sp\n"
"   movem.l (sp)+,d0-d1/a0-a1\n"
"   move.l  _old_freemem,-(sp)\n"
"   rts\n"
/* Open(d1 name, d2 mode) / Lock(d1 name, d2 mode): the original, then
   named_post(result, name, which) */
"   .even\n"
"_open_patch:\n"
"   move.l  #0,-(sp)\n"
"   move.l  _old_open,a0\n"
"   bra.s   2f\n"
"   .even\n"
"_lock_patch:\n"
"   move.l  #1,-(sp)\n"
"   move.l  _old_lock,a0\n"
"2: move.l  d1,-(sp)\n"
"   jsr     (a0)\n"
"   move.l  (sp)+,d1\n"
"   movem.l d0-d1/a0-a1,-(sp)\n"
"   move.l  16(sp),-(sp)\n"          /* which */
"   move.l  d1,-(sp)\n"
"   move.l  d0,-(sp)\n"
"   jsr     _named_post\n"
"   lea     12(sp),sp\n"
"   movem.l (sp)+,d0-d1/a0-a1\n"
"   addq.l  #4,sp\n"
"   rts\n"
/* Read(d1 file, d2 buffer, d3 length): read_pre != 0 -> -1 */
"   .even\n"
"_read_patch:\n"
"   movem.l d1/a0-a1,-(sp)\n"
"   move.l  d1,-(sp)\n"
"   jsr     _read_pre\n"
"   addq.l  #4,sp\n"
"   tst.l   d0\n"
"   movem.l (sp)+,d1/a0-a1\n"
"   bne.s   3f\n"
"   move.l  _old_read,-(sp)\n"
"   rts\n"
"3: moveq   #-1,d0\n"
"   rts\n"
/* Examine(d1 lock, d2 fib): examine_pre != 0 -> DOSFALSE */
"   .even\n"
"_examine_patch:\n"
"   movem.l d1/a0-a1,-(sp)\n"
"   move.l  d1,-(sp)\n"
"   jsr     _examine_pre\n"
"   addq.l  #4,sp\n"
"   tst.l   d0\n"
"   movem.l (sp)+,d1/a0-a1\n"
"   bne.s   4f\n"
"   move.l  _old_examine,-(sp)\n"
"   rts\n"
"4: moveq   #0,d0\n"
"   rts\n"
/* OldOpenLibrary(a1 name) / OpenLibrary(a1 name, d0 version):
   openlib_pre != 0 -> 0, else the original */
"   .even\n"
"_oldopenlib_patch:\n"
"   move.l  _old_oldopenlib,-(sp)\n"
"   bra.s   5f\n"
"   .even\n"
"_openlib_patch:\n"
"   move.l  _old_openlib,-(sp)\n"
"5: movem.l d0-d1/a0-a1,-(sp)\n"
"   move.l  a1,-(sp)\n"
"   jsr     _openlib_pre\n"
"   addq.l  #4,sp\n"
"   tst.l   d0\n"
"   movem.l (sp)+,d0-d1/a0-a1\n"
"   bne.s   6f\n"
"   rts\n"                          /* to the original */
"6: addq.l  #4,sp\n"
"   moveq   #0,d0\n"
"   rts\n"
);

static int ours(void) { return active && SysBase->ThisTask == me; }

/* ----------------------------------------------------- allocations */

#define SLOTS 16384                     /* power of 2 */
static struct { ULONG addr, size, flags; } slot[SLOTS];
static ULONG nalloc, tbl_full;
#define TOMB 1UL                        /* removed (odd: no allocation) */

static ULONG hash(ULONG a) { return ((a >> 3) * 2654435761UL) >> 18 & (SLOTS - 1); }

ULONG alloc_pre(ULONG size, ULONG flags)
{
    if (!o_allocfail || !ours() || size < af_min || size > af_max
        || (af_chip && !(flags & MEMF_CHIP)) || (af_count && af_done >= af_count))
        return 0;
    af_done++;
    return 1;
}

void alloc_post(ULONG mem, ULONG size, ULONG flags)
{
    ULONG h, i;

    if (!o_track || !mem || !ours())
        return;
    for (h = hash(mem), i = 0; i < SLOTS; i++, h = (h + 1) & (SLOTS - 1))
        if (slot[h].addr == 0 || slot[h].addr == TOMB) {
            slot[h].addr = mem;
            slot[h].size = size;
            slot[h].flags = flags;
            nalloc++;
            return;
        }
    tbl_full++;
}

void free_pre(ULONG mem, ULONG size)
{
    ULONG h, i;

    (void)size;
    if (!o_track || !mem || !ours())
        return;
    for (h = hash(mem), i = 0; i < SLOTS && slot[h].addr; i++, h = (h + 1) & (SLOTS - 1))
        if (slot[h].addr == mem) {
            slot[h].addr = TOMB;
            nalloc--;
            return;
        }
}

/* ---------------------------------------------------- dos.library */

#define NAMED 8
static ULONG rf_fh[NAMED], rf_reads[NAMED], ef_lock[NAMED];

static int lower(int c) { return c >= 'A' && c <= 'Z' ? c + 32 : c; }

/* does name end in suffix (case ignored) */
static int ends(const char *name, const char *suffix)
{
    ULONG n = strlen(name), k = strlen(suffix), i;

    if (!k || k > n)
        return 0;
    for (i = 0; i < k; i++)
        if (lower(name[n - k + i]) != lower(suffix[i]))
            return 0;
    return 1;
}

void named_post(ULONG result, const char *name, ULONG which)
{
    ULONG *list = which ? ef_lock : rf_fh;
    const char *want = which ? ef_name : rf_name;
    int i;

    if (!result || !name || !*want || !ours() || !ends(name, want))
        return;
    for (i = 0; i < NAMED; i++)
        if (!list[i]) {
            list[i] = result;
            if (!which)
                rf_reads[i] = 0;
            return;
        }
}

ULONG read_pre(ULONG fh)
{
    int i;

    if (!rf_name[0] || !ours() || !fh)
        return 0;
    for (i = 0; i < NAMED; i++)
        if (rf_fh[i] == fh) {
            if (++rf_reads[i] != rf_n)
                return 0;
            rf_fh[i] = 0;               /* once per open */
            rf_done++;
            return 1;
        }
    return 0;
}

ULONG examine_pre(ULONG lock)
{
    int i;

    if (!ef_name[0] || !ours() || !lock)
        return 0;
    for (i = 0; i < NAMED; i++)
        if (ef_lock[i] == lock) {
            ef_lock[i] = 0;             /* once per lock */
            ef_done++;
            return 1;
        }
    return 0;
}

/* ---------------------------------------------------- exec libraries */

ULONG openlib_pre(const char *name)
{
    if (!olf_name[0] || !ours() || !name || !ends(name, olf_name))
        return 0;
    olf_done++;
    return 1;
}

/* ------------------------------------------------------------ main */

static int number(const char **s, ULONG *v)
{
    const char *p = *s;

    *v = 0;
    if (*p < '0' || *p > '9')
        return 0;
    while (*p >= '0' && *p <= '9')
        *v = *v * 10 + (ULONG)(*p++ - '0');
    *s = p;
    return 1;
}

/* 1 = an option, 0 = the command, -1 = a bad option */
static int option(const char *a)
{
    const char *s;

    if (!strcmp(a, "signals"))
        return o_signals = 1;
    if (!strcmp(a, "track"))
        return o_track = 1;
    if (!strncmp(a, "allocfail=", 10)) {
        s = a + 10;
        if (!number(&s, &af_min) || *s++ != '-' || !number(&s, &af_max) || af_min > af_max)
            return -1;
        if (!strncmp(s, "/chip", 5)) {
            af_chip = 1;
            s += 5;
        }
        if (*s == '/' && (s++, !number(&s, &af_count) || !af_count))
            return -1;
        return *s ? -1 : (o_allocfail = 1);
    }
    if (!strncmp(a, "readfail=", 9)) {
        const char *slash = strrchr(a, '/');

        if (!slash || slash == a + 9 || (ULONG)(slash - a - 9) >= sizeof rf_name)
            return -1;
        s = slash + 1;
        if (!number(&s, &rf_n) || *s || !rf_n)
            return -1;
        memcpy(rf_name, a + 9, (ULONG)(slash - a - 9));
        return 1;
    }
    if (!strncmp(a, "examinefail=", 12)) {
        if (!a[12] || strlen(a + 12) >= sizeof ef_name)
            return -1;
        strcpy(ef_name, a + 12);
        return 1;
    }
    if (!strncmp(a, "openlibfail=", 12)) {
        if (!a[12] || strlen(a + 12) >= sizeof olf_name)
            return -1;
        strcpy(olf_name, a + 12);
        return 1;
    }
    return 0;
}

static void patch(int on)
{
    struct Library *dos = (struct Library *)DOSBase, *ex = (struct Library *)SysBase;

    Forbid();
    if (on) {
        old_allocmem = SetFunction(ex, LVO_ALLOCMEM, (APTR)allocmem_patch);
        old_freemem = SetFunction(ex, LVO_FREEMEM, (APTR)freemem_patch);
        old_open = SetFunction(dos, LVO_OPEN, (APTR)open_patch);
        old_read = SetFunction(dos, LVO_READ, (APTR)read_patch);
        old_lock = SetFunction(dos, LVO_LOCK, (APTR)lock_patch);
        old_examine = SetFunction(dos, LVO_EXAMINE, (APTR)examine_patch);
        old_oldopenlib = SetFunction(ex, LVO_OLDOPENLIBRARY, (APTR)oldopenlib_patch);
        old_openlib = SetFunction(ex, LVO_OPENLIBRARY, (APTR)openlib_patch);
    } else {
        SetFunction(ex, LVO_ALLOCMEM, old_allocmem);
        SetFunction(ex, LVO_FREEMEM, old_freemem);
        SetFunction(dos, LVO_OPEN, old_open);
        SetFunction(dos, LVO_READ, old_read);
        SetFunction(dos, LVO_LOCK, old_lock);
        SetFunction(dos, LVO_EXAMINE, old_examine);
        SetFunction(ex, LVO_OLDOPENLIBRARY, old_oldopenlib);
        SetFunction(ex, LVO_OPENLIBRARY, old_openlib);
    }
    CacheClearU();
    Permit();
}

/* what is left with track, sorted by size and flags */
static void report_track(void)
{
    static ULONG sz[64], fl[64];
    static char line[100];
    ULONG bytes = 0, n = 0, i, j, t;

    for (i = 0; i < SLOTS; i++)
        if (slot[i].addr > TOMB) {
            bytes += slot[i].size;
            if (n < 64) {
                sz[n] = slot[i].size;
                fl[n++] = slot[i].flags;
            }
        }
    for (i = 1; i < n; i++)
        for (j = i; j > 0 && (sz[j - 1] > sz[j] || (sz[j - 1] == sz[j] && fl[j - 1] > fl[j])); j--) {
            t = sz[j], sz[j] = sz[j - 1], sz[j - 1] = t;
            t = fl[j], fl[j] = fl[j - 1], fl[j - 1] = t;
        }
    sprintf(line, "inject: track: %lu allocations left, %lu bytes%s\n", nalloc, bytes,
            tbl_full ? " (table full: more not recorded)" : "");
    put(line);
    for (i = 0; i < n; i++) {
        sprintf(line, "inject:   %lu bytes, flags $%lx\n", sz[i], fl[i]);
        put(line);
    }
}

int main(int argc, char **argv)
{
    static char args[512], line[100];
    struct CommandLineInterface *cli = Cli();
    ULONG stack = cli ? cli->cli_DefaultStack * 4 : 4096, sigs = 0;
    BPTR seg;
    LONG rc;
    int i, k, r = 0;

    out = Output();
    me = FindTask(NULL);
    for (i = 1; i < argc && (r = option(argv[i])) != 0; i++)
        if (r < 0) {
            put("inject: bad option ");
            put(argv[i]);
            put("\n");
            return 20;
        }
    if (i >= argc) {
        put("usage: inject [signals] [allocfail=MIN-MAX[/chip][/COUNT]] [readfail=NAME/N]\n"
            "              [examinefail=NAME] [openlibfail=NAME] [track] COMMAND [ARGS]\n");
        return 20;
    }
    for (k = i + 1; k < argc; k++) {
        if (strlen(args) + strlen(argv[k]) + 3 >= sizeof args)
            return 20;
        if (k > i + 1)
            strcat(args, " ");
        strcat(args, argv[k]);
    }
    strcat(args, "\n");
    if (!(seg = LoadSeg(argv[i]))) {
        put("inject: cannot load ");
        put(argv[i]);
        put("\n");
        return 20;
    }
    if (o_signals)
        for (;;) {
            BYTE s = AllocSignal(-1);

            if (s < 0)
                break;
            sigs |= 1UL << s;
        }
    patch(1);
    active = 1;
    rc = RunCommand(seg, stack, args, strlen(args));
    active = 0;
    patch(0);
    for (k = 0; k < 32; k++)
        if (sigs & (1UL << k))
            FreeSignal(k);
    UnLoadSeg(seg);
    if (o_allocfail) {
        sprintf(line, "inject: allocfail: %lu allocations failed\n", af_done);
        put(line);
    }
    if (rf_name[0]) {
        sprintf(line, "inject: readfail: %lu reads failed\n", rf_done);
        put(line);
    }
    if (ef_name[0]) {
        sprintf(line, "inject: examinefail: %lu examines failed\n", ef_done);
        put(line);
    }
    if (olf_name[0]) {
        sprintf(line, "inject: openlibfail: %lu opens failed\n", olf_done);
        put(line);
    }
    sprintf(line, "inject: rc=%ld\n", rc);
    put(line);
    if (o_track)
        report_track();
    return 0;
}
