/*
 * test_amiga_emulation_snapshot - the snapshot mode with the emulator
 * (A1200), in a slot directory of its own while it holds one of the
 * shared slots: the first session boots cold and saves the state, the
 * second, in a new process, restores it (TEST:, a writable and a
 * read-only mount through the slot's links); a copy of the hardfile, then
 * the same copy with another modification time, each boot and save anew
 * and remove the stale snapshot; a state whose restore gets no start is
 * kept the first time (the session boots cold) and removed the second, a
 * file that is no savestate is removed and the session saves anew, `cold`
 * boots without snapshot.  After every session
 * the slot has no links or SNAP: left.  Exit 77 (skipped) without
 * Amiberry or the data.  The second session runs this program again with
 * the hidden test case "restored session" (the directory in
 * $AMIGA_EMULATION_SNAPTEST_BASE).
 */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <ftw.h>
#include <snitch/snitch.hpp>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
extern "C" {
#include "amiga_emulation.h"
}

#define BASE_ENV "AMIGA_EMULATION_SNAPTEST_BASE"
#define RESTORED_CASE "amiga_emulation_snapshot: restored session"

static char base[] = "/tmp/amiga-emulation-snaptest-XXXXXX";
static char slot[256];          /* base/run/amiga-emulation-slots/slot-0 */

static char *read_text(const char *dir, const char *name)
{
    char path[4096];
    FILE *f;
    char *s = static_cast<char *>(calloc(1, 65536));

    snprintf(path, sizeof path, "%s/%s", dir, name);
    if (s && (f = fopen(path, "r")) != NULL) {
        size_t n = fread(s, 1, 65535, f);

        s[n] = 0;
        fclose(f);
    }
    return s;
}

static void write_text(const char *dir, const char *name, const char *text)
{
    char path[4096];
    FILE *f;

    snprintf(path, sizeof path, "%s/%s", dir, name);
    f = fopen(path, "w");
    CHECK(f != NULL);
    if (f) {
        fputs(text, f);
        fclose(f);
    }
}

static int rm_entry(const char *path, const struct stat *st, int flag, struct FTW *ftw)
{
    (void)st; (void)flag; (void)ftw;
    remove(path);
    return 0;
}

static double now(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

/* the snap-*.uss in the slot (the last one's name into path), not their
   marks snap-*.uss.nostart */
static int snapshots(char *path, size_t len)
{
    DIR *d = opendir(slot);
    struct dirent *de;
    int n = 0;

    if (!d)
        return 0;
    while ((de = readdir(d)) != NULL)
        if (!strncmp(de->d_name, "snap-", 5) && strlen(de->d_name) > 9
            && !strcmp(de->d_name + strlen(de->d_name) - 4, ".uss")) {
            snprintf(path, len, "%s/%s", slot, de->d_name);
            n++;
        }
    closedir(d);
    return n;
}

/* the slot holds nothing but its snapshots */
static int slot_clean(void)
{
    DIR *d = opendir(slot);
    struct dirent *de;
    int clean = 1;

    if (!d)
        return 0;
    while ((de = readdir(d)) != NULL)
        if (strcmp(de->d_name, ".") && strcmp(de->d_name, "..") && strncmp(de->d_name, "snap-", 5)) {
            fprintf(stderr, "test_amiga_emulation_snapshot: left in the slot: %s\n", de->d_name);
            clean = 0;
        }
    closedir(d);
    return clean;
}

/* one session in base/NAME (emu: the emulation directory, NULL: the
   default); the snapshot.log into *log; 1 = the results are right
   (amiga_emulation_get_stats too: snapshot use `use`), -1 = cannot start */
static int session(const char *name, const char *emu, int cold, enum amiga_emulation_snapshot_use use,
                   char **log, double *secs)
{
    static const char script[] =
        "Type TEST:in.txt >TEST:out.txt\n"
        "Echo >W:w.txt \"written\"\n"
        "Type R:r.txt >TEST:r.txt\n"
        "Version >TEST:version.txt\n";
    char work[256], test[300], w[256], r[256], err[300], *s, *expect;
    struct amiga_emulation_mount mounts[2];
    struct amiga_emulation_params p = {};
    struct amiga_emulation *e;
    struct amiga_emulation_stats st;
    int ok;

    snprintf(work, sizeof work, "%s/%s", base, name);
    snprintf(test, sizeof test, "%s/test", work);
    snprintf(w, sizeof w, "%s/%s-w", base, name);
    snprintf(r, sizeof r, "%s/%s-r", base, name);
    CHECK(mkdir(work, 0700) == 0 && mkdir(test, 0700) == 0 && mkdir(w, 0700) == 0
          && mkdir(r, 0700) == 0);
    write_text(test, "in.txt", name);
    write_text(r, "r.txt", "read-only");
    mounts[0] = amiga_emulation_mount{ "W", w, 1 };
    mounts[1] = amiga_emulation_mount{ "R", r, 0 };
    p.mounts = mounts;
    p.nmounts = 2;
    p.script = script;
    p.timeout = 90;
    p.work_dir = work;
    p.cold = cold;
    p.emulation_dir = emu;
    *secs = now();
    if (!(e = amiga_emulation_start(&p, err, sizeof err))) {
        fprintf(stderr, "test_amiga_emulation_snapshot: %s: %s\n", name, err);
        return -1;
    }
    ok = amiga_emulation_join(e) == AMIGA_EMULATION_DONE;
    *secs = now() - *secs;
    amiga_emulation_get_stats(e, &st);
    if (st.snapshot != use || st.status != AMIGA_EMULATION_DONE || st.timeout != 90
        || strcmp(st.machine, "a1200") || st.wait < 0 || st.session <= 0
        || st.session > *secs + 0.01 || st.timed <= 0 || st.timed > st.session + 0.01) {
        fprintf(stderr, "test_amiga_emulation_snapshot: %s: stats %s %s wait %.2f session %.2f "
                "timed %.2f (whole %.2f)\n", name, amiga_emulation_snapshot_use_name(st.snapshot),
                amiga_emulation_status_name(st.status), st.wait, st.session, st.timed, *secs);
        ok = 0;
    }
    s = read_text(test, "out.txt");
    ok = ok && s && !strncmp(s, name, strlen(name));
    free(s);
    s = read_text(w, "w.txt");
    ok = ok && s && !strncmp(s, "written", 7);
    free(s);
    s = read_text(test, "r.txt");
    ok = ok && s && !strncmp(s, "read-only", 9);
    free(s);
    s = read_text(test, "version.txt");
    ok = ok && s && strstr(s, "Kickstart 40");
    free(s);
    /* TEST:run.script is the test's script, as without snapshot */
    s = read_text(test, "run.script");
    expect = amiga_emulation_script(script);
    ok = ok && s && expect && !strcmp(s, expect);
    free(s);
    free(expect);
    *log = read_text(work, "snapshot.log");
    if (!ok)
        fprintf(stderr, "test_amiga_emulation_snapshot: %s: wrong results; snapshot.log:\n%s\n", name,
                *log ? *log : "");
    amiga_emulation_free(e);
    return ok;
}

/* the name of the slot's only snapshot ("" if there is not exactly one) */
static void only_snapshot(char *name, size_t len)
{
    char path[600];

    if (snapshots(path, sizeof path) == 1)
        snprintf(name, len, "%s", strrchr(path, '/') + 1);
    else
        snprintf(name, len, "%s", "");
}

/* the second session, restored: run in a new process (this program again
   with the hidden test case, BASE in the environment), as the first session
   of a later ctest run */
TEST_CASE(RESTORED_CASE, "[.restored_child]")
{
    const char *dir = getenv(BASE_ENV);
    char *log = nullptr;
    double secs;

    if (!dir || strlen(dir) != strlen(base))
        FAIL("hidden test case, run by amiga_emulation_snapshot: " BASE_ENV " missing");
    memcpy(base, dir, sizeof base);
    snprintf(slot, sizeof slot, "%s/run/amiga-emulation-slots/slot-0", base);
    CHECK(session("second", NULL, 0, AMIGA_EMULATION_SNAP_RESTORED, &log, &secs) == 1);
    CHECK(log && strstr(log, "restore ") && strstr(log, "job started") && !strstr(log, "cold")
          && !strstr(log, "failed"));
    CHECK(secs < 10);
    free(log);
    fprintf(stderr, "test_amiga_emulation_snapshot: restored session (new process) %.1f s\n", secs);
}

static int run_restored_child(void)
{
    pid_t pid = fork();
    int st;

    if (pid == 0) {
        setenv(BASE_ENV, base, 1);
        execl("/proc/self/exe", "/proc/self/exe", RESTORED_CASE, (char *)nullptr);
        _exit(127);
    }
    return pid > 0 && waitpid(pid, &st, 0) == pid && WIFEXITED(st) ? WEXITSTATUS(st) : -1;
}

/* copies src to dst (another file: its own inode and time) */
static int copy_file(const char *src, const char *dst)
{
    FILE *in = fopen(src, "rb"), *out = in ? fopen(dst, "wb") : NULL;
    static char buf[65536];
    size_t n;
    int bad = !in || !out;

    while (!bad && (n = fread(buf, 1, sizeof buf, in)) > 0)
        bad = fwrite(buf, 1, n, out) != n;
    if (in)
        fclose(in);
    if (out && fclose(out))
        bad = 1;
    return bad ? -1 : 0;
}

/* base/emu: the default emulation directory's ROM and rom.key as links,
   a copy of the hardfile (another identity, the same content) */
static int changed_emulation_dir(char *emu, size_t len)
{
    static const char *const links[] = { "amiga-os-310-a1200.rom", "rom.key" };
    const char *src = getenv("AMIGA_EMULATION_DIR");
    char from[600], to[600];
    size_t i;

    if (!src || !*src)
        src = TEST_EMULATION_DIR;
    snprintf(emu, len, "%s/emu", base);
    if (mkdir(emu, 0700))
        return -1;
    for (i = 0; i < 2; i++) {
        snprintf(from, sizeof from, "%s/%s", src, links[i]);
        snprintf(to, sizeof to, "%s/%s", emu, links[i]);
        if (!access(from, F_OK) && symlink(from, to))
            return -1;
    }
    snprintf(from, sizeof from, "%s/workbench-311.hdf", src);
    snprintf(to, sizeof to, "%s/workbench-311.hdf", emu);
    return copy_file(from, to);
}

TEST_CASE("amiga_emulation_snapshot: save, restore, stale and broken states", "[amiga_emulation_snapshot]")
{
    const char *xdg = getenv("XDG_RUNTIME_DIR");
    char shared[256], runtime[200], path[600], emu[300], name[128], before[128], *log = nullptr;
    struct timespec old[2] = { { 1000000000, 0 }, { 1000000000, 0 } };
    double secs;
    int held, r;
    FILE *f;

    /* one of the shared slots for the whole test, then a slot directory
       of its own with one slot */
    if (xdg && *xdg)
        snprintf(shared, sizeof shared, "%s/amiga-emulation-slots", xdg);
    else
        snprintf(shared, sizeof shared, "/tmp/amiga-emulation-slots-%u", (unsigned)getuid());
    held = amiga_emulation_slot(shared, AMIGA_EMULATION_SLOTS_AUTO, 1);
    CHECK(held >= 0);
    CHECK(mkdtemp(base) != NULL);
    snprintf(runtime, sizeof runtime, "%s/run", base);
    snprintf(slot, sizeof slot, "%s/amiga-emulation-slots/slot-0", runtime);
    CHECK(mkdir(runtime, 0700) == 0);
    setenv("XDG_RUNTIME_DIR", runtime, 1);
    setenv("AMIGA_EMULATION_MAX", "1", 1);
    unsetenv("AMIGA_EMULATION_SNAPSHOT");

    /* the first session: boots cold, saves, restores */
    r = session("first", NULL, 0, AMIGA_EMULATION_SNAP_FIRST_SAVE, &log, &secs);
    if (r < 0)
        std::exit(77);          // skipped (the exit status of ctest SKIP_RETURN_CODE)
    CHECK(r == 1);
    CHECK(log && strstr(log, "boot cold, save") && strstr(log, " saved ")
          && strstr(log, "job started") && !strstr(log, "failed") && !strstr(log, "stale"));
    CHECK(snapshots(path, sizeof path) == 1);
    CHECK(slot_clean());
    free(log);
    fprintf(stderr, "test_amiga_emulation_snapshot: first session %.1f s\n", secs);

    /* the second, in a new process: restored, the snapshot stays */
    only_snapshot(before, sizeof before);
    CHECK(run_restored_child() == 0);
    only_snapshot(name, sizeof name);
    CHECK(*name && !strcmp(name, before));
    CHECK(slot_clean());

    /* a changed input (a copy of the hardfile): a new snapshot, the
       stale one of the same configuration removed when it is saved */
    CHECK(changed_emulation_dir(emu, sizeof emu) == 0);
    CHECK(session("copy", emu, 0, AMIGA_EMULATION_SNAP_FIRST_SAVE, &log, &secs) == 1);
    CHECK(log && strstr(log, "boot cold, save") && strstr(log, "removed 1 stale snapshot")
          && !strstr(log, "failed"));
    only_snapshot(name, sizeof name);
    CHECK(*name && strcmp(name, before));
    CHECK(slot_clean());
    free(log);
    /* the copy's modification time changed: again */
    snprintf(path, sizeof path, "%s/workbench-311.hdf", emu);
    CHECK(utimensat(AT_FDCWD, path, old, 0) == 0);
    snprintf(before, sizeof before, "%s", name);
    CHECK(session("mtime", emu, 0, AMIGA_EMULATION_SNAP_FIRST_SAVE, &log, &secs) == 1);
    CHECK(log && strstr(log, "boot cold, save") && strstr(log, "removed 1 stale snapshot")
          && !strstr(log, "failed"));
    only_snapshot(name, sizeof name);
    CHECK(*name && strcmp(name, before));
    CHECK(slot_clean());
    free(log);

    /* broken: a restored state whose TEST: takes no file (here: the
       check writes to a volume that does not exist) gets no start, the
       session boots cold; the state file stays the first time (no start
       in time can also be a busy host), with a mark beside it; the second
       time in a row it is removed, the next session saves anew.  The
       restores that are to fail get 5 s, not 15. */
    CHECK(snapshots(path, sizeof path) == 1);
    setenv("AMIGA_EMULATION_SNAP_CHECK_VOLUME", "NOSUCHVOLUME", 1);
    setenv("AMIGA_EMULATION_SNAP_RESTORE_GRACE", "5", 1);
    CHECK(session("notest", emu, 0, AMIGA_EMULATION_SNAP_FALLBACK, &log, &secs) == 1);
    CHECK(log && strstr(log, "restore failed (no start)") && strstr(log, "kept, boot cold"));
    CHECK(snapshots(path, sizeof path) == 1);
    CHECK(slot_clean());
    free(log);
    CHECK(session("notest2", emu, 0, AMIGA_EMULATION_SNAP_FALLBACK, &log, &secs) == 1);
    unsetenv("AMIGA_EMULATION_SNAP_CHECK_VOLUME");
    unsetenv("AMIGA_EMULATION_SNAP_RESTORE_GRACE");
    CHECK(log && strstr(log, "restore failed (no start)") && strstr(log, "removed (no start twice), boot cold"));
    CHECK(snapshots(path, sizeof path) == 0);
    CHECK(slot_clean());
    free(log);
    CHECK(session("again", emu, 0, AMIGA_EMULATION_SNAP_FIRST_SAVE, &log, &secs) == 1);
    free(log);
    CHECK(slot_clean());

    /* broken: a file that is no savestate is removed, the session boots
       cold and saves anew */
    CHECK(snapshots(path, sizeof path) == 1);
    f = fopen(path, "wb");
    CHECK(f != NULL);
    if (f) {
        fputs("not a savestate", f);
        fclose(f);
    }
    CHECK(session("corrupted", emu, 0, AMIGA_EMULATION_SNAP_FIRST_SAVE, &log, &secs) == 1);
    CHECK(log && strstr(log, "not a savestate, removed") && strstr(log, "boot cold, save"));
    CHECK(snapshots(path, sizeof path) == 1);
    CHECK(slot_clean());
    free(log);

    /* cold: no snapshot at all */
    CHECK(session("cold", NULL, 1, AMIGA_EMULATION_SNAP_COLD, &log, &secs) == 1);
    CHECK(log && !*log);
    CHECK(snapshots(path, sizeof path) == 1);       /* the one of "corrupted", untouched */
    free(log);

    if (!snitch::impl::get_current_test().failures)
        nftw(base, rm_entry, 16, FTW_DEPTH | FTW_PHYS);
    close(held);
    if (snitch::impl::get_current_test().failures)
        fprintf(stderr, "test_amiga_emulation_snapshot: files in %s\n", base);
}
