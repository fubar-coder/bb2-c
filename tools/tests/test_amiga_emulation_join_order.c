/*
 * test_amiga_emulation_join_order.c - with the emulator (A1200): two
 * sessions of one process, joined in start order as the runners do.  The
 * first hangs until its time limit; the second is a first-save session (a
 * slot directory of its own, so no snapshot exists yet).  While the
 * caller waits for the first, the second must boot, save, restore and run
 * its job on its own (its supervisor), not sit at SNAP:ready until its
 * own limit has run out.  It holds two of the shared slots meanwhile.
 * Exit 77 (skipped) without Amiberry or the emulation data.
 */
#define _XOPEN_SOURCE 700       /* mkdtemp, setenv */
#include <ftw.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include "testutil.h"
#include "amiga_emulation.h"

#define LIMIT 25                /* seconds: a cold boot (14 s) and the save fit */

static double now(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

static char *read_text(const char *dir, const char *name)
{
    char path[4096];
    FILE *f;
    char *s;
    size_t n;

    snprintf(path, sizeof path, "%s/%s", dir, name);
    if (!(f = fopen(path, "r")))
        return NULL;
    s = calloc(1, 65536);
    n = s ? fread(s, 1, 65535, f) : 0;
    if (s)
        s[n] = 0;
    fclose(f);
    return s;
}

static int rm_entry(const char *path, const struct stat *st, int flag, struct FTW *ftw)
{
    (void)st; (void)flag; (void)ftw;
    remove(path);
    return 0;
}

/* amiberry in PATH */
static int have_amiberry(void)
{
    const char *path = getenv("PATH");
    char buf[4096];

    while (path && *path) {
        size_t n = strcspn(path, ":");

        snprintf(buf, sizeof buf, "%.*s/amiberry", (int)n, path);
        if (!access(buf, X_OK))
            return 1;
        path += n + (path[n] == ':');
    }
    return 0;
}

static void show(const char *name, struct amiga_emulation *e)
{
    struct amiga_emulation_stats st;
    char *log = read_text(amiga_emulation_work_dir(e), "snapshot.log");

    amiga_emulation_stats(e, &st);
    fprintf(stderr, "test_amiga_emulation_join_order: %s: %s %s session %.2f timed %.2f\n%s", name,
            amiga_emulation_snapshot_use_name(st.snapshot), amiga_emulation_status_name(st.status),
            st.session, st.timed, log ? log : "");
    free(log);
}

int main(void)
{
    static char base[] = "/tmp/amiga-emulation-jointest-XXXXXX";
    const char *xdg = getenv("XDG_RUNTIME_DIR");
    char shared[256], runtime[200], err[512], *log, *v;
    struct amiga_emulation_params pa = { 0 }, pb = { 0 };
    struct amiga_emulation *a, *b;
    struct amiga_emulation_stats st;
    double t0, joined_a;
    int held[2];

    if (!have_amiberry()) {
        fprintf(stderr, "skipped: no amiberry in PATH\n");
        return 77;
    }
    /* two of the shared slots for the whole test, then a slot directory
       of its own with two slots (no snapshots in it: first-save sessions) */
    if (xdg && *xdg)
        snprintf(shared, sizeof shared, "%s/amiga-emulation-slots", xdg);
    else
        snprintf(shared, sizeof shared, "/tmp/amiga-emulation-slots-%u", (unsigned)getuid());
    held[0] = amiga_emulation_slot(shared, AMIGA_EMULATION_SLOTS_AUTO, 1);
    held[1] = amiga_emulation_slot(shared, AMIGA_EMULATION_SLOTS_AUTO, 1);
    CHECK(held[0] >= 0 && held[1] >= 0);
    CHECK(mkdtemp(base) != NULL);
    snprintf(runtime, sizeof runtime, "%s/run", base);
    CHECK(mkdir(runtime, 0700) == 0);
    setenv("XDG_RUNTIME_DIR", runtime, 1);
    setenv("AMIGA_EMULATION_MAX", "2", 1);
    unsetenv("AMIGA_EMULATION_SNAPSHOT");

    pa.script = "Wait 300\n";
    pa.timeout = LIMIT;
    pb.script = "Version >TEST:version.txt\n";
    pb.timeout = LIMIT;
    t0 = now();
    if (!(a = amiga_emulation_start(&pa, err, sizeof err))) {
        fprintf(stderr, "skipped: %s\n", err);
        return 77;
    }
    b = amiga_emulation_start(&pb, err, sizeof err);
    CHECK(b != NULL);
    if (!b) {
        fprintf(stderr, "test_amiga_emulation_join_order: %s\n", err);
        amiga_emulation_free(a);
        return 1;
    }

    /* the first: its job hangs, the limit ends it */
    CHECK(amiga_emulation_join(a) == AMIGA_EMULATION_TIMEOUT);
    joined_a = now() - t0;
    amiga_emulation_stats(a, &st);
    CHECK(st.snapshot == AMIGA_EMULATION_SNAP_FIRST_SAVE);
    CHECK(st.timed >= LIMIT && st.timed < LIMIT + 5);

    /* the second has run its job meanwhile, within its limit */
    CHECK(amiga_emulation_join(b) == AMIGA_EMULATION_DONE);
    amiga_emulation_stats(b, &st);
    CHECK(st.snapshot == AMIGA_EMULATION_SNAP_FIRST_SAVE && st.status == AMIGA_EMULATION_DONE);
    CHECK(st.timed > 0 && st.timed < LIMIT);
    CHECK(st.session > 0 && st.session < joined_a - 5);
    log = read_text(amiga_emulation_work_dir(b), "snapshot.log");
    CHECK(log && strstr(log, " saved ") && strstr(log, "job started") && strstr(log, "done"));
    free(log);
    v = read_text(amiga_emulation_test_dir(b), "version.txt");
    CHECK(v && strstr(v, "Kickstart 40"));
    free(v);
    fprintf(stderr, "test_amiga_emulation_join_order: first joined after %.1f s, the second's "
            "session %.1f s\n", joined_a, st.session);
    if (failures) {
        show("first", a);
        show("second", b);
    }
    amiga_emulation_free(a);
    amiga_emulation_free(b);

    if (!failures)
        nftw(base, rm_entry, 16, FTW_DEPTH | FTW_PHYS);
    close(held[0]);
    close(held[1]);
    if (failures)
        fprintf(stderr, "test_amiga_emulation_join_order: %d failures (files in %s)\n", failures,
                base);
    return failures != 0;
}
