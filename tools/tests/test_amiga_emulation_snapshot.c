/*
 * test_amiga_emulation_snapshot.c - the snapshot mode with the emulator
 * (A1200), in a slot directory of its own while it holds one of the
 * shared slots: the first session boots cold and saves the state, the
 * second restores it (TEST:, a writable and a read-only mount through the
 * slot's links), a corrupted state is dropped and the session boots cold,
 * `cold` boots without snapshot.  After every session the slot has no
 * links or SNAP: left.  Exit 77 (skipped) without Amiberry or the data.
 */
#define _XOPEN_SOURCE 700       /* mkdtemp, setenv */
#include <dirent.h>
#include <ftw.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include "testutil.h"
#include "amiga_emulation.h"

static char base[] = "/tmp/amiga-emulation-snaptest-XXXXXX";
static char slot[256];          /* base/run/amiga-emulation-slots/slot-0 */

static char *read_text(const char *dir, const char *name)
{
    char path[4096];
    FILE *f;
    char *s = calloc(1, 65536);

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

/* the snap-*.uss in the slot (the last one's name into path) */
static int snapshots(char *path, size_t len)
{
    DIR *d = opendir(slot);
    struct dirent *de;
    int n = 0;

    if (!d)
        return 0;
    while ((de = readdir(d)) != NULL)
        if (!strncmp(de->d_name, "snap-", 5)) {
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

/* one session in base/NAME; the snapshot.log into *log; 1 = the results
   are right, -1 = cannot start */
static int session(const char *name, int cold, char **log, double *secs)
{
    static const char script[] =
        "Type TEST:in.txt >TEST:out.txt\n"
        "Echo >W:w.txt \"written\"\n"
        "Type R:r.txt >TEST:r.txt\n"
        "Version >TEST:version.txt\n";
    char work[256], test[300], w[256], r[256], err[300], *s, *expect;
    struct amiga_emulation_mount mounts[2];
    struct amiga_emulation_params p = { 0 };
    struct amiga_emulation *e;
    int ok;

    snprintf(work, sizeof work, "%s/%s", base, name);
    snprintf(test, sizeof test, "%s/test", work);
    snprintf(w, sizeof w, "%s/%s-w", base, name);
    snprintf(r, sizeof r, "%s/%s-r", base, name);
    CHECK(mkdir(work, 0700) == 0 && mkdir(test, 0700) == 0 && mkdir(w, 0700) == 0
          && mkdir(r, 0700) == 0);
    write_text(test, "in.txt", name);
    write_text(r, "r.txt", "read-only");
    mounts[0] = (struct amiga_emulation_mount){ "W", w, 1 };
    mounts[1] = (struct amiga_emulation_mount){ "R", r, 0 };
    p.mounts = mounts;
    p.nmounts = 2;
    p.script = script;
    p.timeout = 90;
    p.work_dir = work;
    p.cold = cold;
    *secs = now();
    if (!(e = amiga_emulation_start(&p, err, sizeof err))) {
        fprintf(stderr, "test_amiga_emulation_snapshot: %s: %s\n", name, err);
        return -1;
    }
    ok = amiga_emulation_join(e) == AMIGA_EMULATION_DONE;
    *secs = now() - *secs;
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
    ok = ok && s && strstr(s, "Kickstart 47");
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

int main(void)
{
    const char *xdg = getenv("XDG_RUNTIME_DIR");
    char shared[256], runtime[200], path[600], *log = NULL;
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
    r = session("first", 0, &log, &secs);
    if (r < 0)
        return 77;
    CHECK(r == 1);
    CHECK(log && strstr(log, "boot cold, save") && strstr(log, " saved ")
          && strstr(log, "job started") && !strstr(log, "failed"));
    CHECK(snapshots(path, sizeof path) == 1);
    CHECK(slot_clean());
    free(log);
    fprintf(stderr, "test_amiga_emulation_snapshot: first session %.1f s\n", secs);

    /* the second: restored */
    CHECK(session("second", 0, &log, &secs) == 1);
    CHECK(log && strstr(log, "restore ") && strstr(log, "job started") && !strstr(log, "cold")
          && !strstr(log, "failed"));
    CHECK(secs < 10);
    CHECK(snapshots(path, sizeof path) == 1);
    CHECK(slot_clean());
    free(log);
    fprintf(stderr, "test_amiga_emulation_snapshot: restored session %.1f s\n", secs);

    /* broken: a corrupted state is dropped, the session boots cold */
    CHECK(snapshots(path, sizeof path) == 1);
    f = fopen(path, "wb");
    CHECK(f != NULL);
    if (f) {
        fputs("not a savestate", f);
        fclose(f);
    }
    CHECK(session("corrupted", 0, &log, &secs) == 1);
    CHECK(log && strstr(log, "restore failed") && strstr(log, "boot cold"));
    CHECK(snapshots(path, sizeof path) == 0);
    CHECK(slot_clean());
    free(log);

    /* cold: no snapshot at all */
    CHECK(session("cold", 1, &log, &secs) == 1);
    CHECK(log && !*log);
    CHECK(snapshots(path, sizeof path) == 0);
    free(log);

    if (!failures)
        nftw(base, rm_entry, 16, FTW_DEPTH | FTW_PHYS);
    close(held);
    if (failures)
        fprintf(stderr, "test_amiga_emulation_snapshot: %d failures (files in %s)\n", failures, base);
    return failures != 0;
}
