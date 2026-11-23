/*
 * test_amiga_emulation_supervisor - a session runs without the caller:
 * the supervisor of every session drives it (snapshot save, restore,
 * fallback, time limit, end), whenever the caller joins it.  No emulator:
 * this program is its own fake Amiberry (FAKE_AMIBERRY in the
 * environment), in a temporary XDG_RUNTIME_DIR with slots of its own.
 *
 * The fake reads amiberry.uae for TEST: and SNAP:, answers the control
 * socket (SAVESTATE writes a fake state, QUIT ends it), "boots" in 0.3 s
 * and then plays the hardfile's user-startup: a cold boot with the
 * snapshot prologue writes SNAP:ready and waits for SNAP:job.script, a
 * restore (--statefile) waits for it at once (a state that is not a fake
 * one ends it, as a failed restore; the stuck state loads and never runs
 * its job, a restore without a start; SAVESTATE writes it while the file
 * $FAKE_BAD_SAVES counts down); a job writes SNAP:started, a script
 * writes TEST:run.done unless it contains HANG.  A script with SLOWTERM
 * writes run.done, and on SIGTERM takes 2.5 s to end, writing TEST:late as
 * its last act (Amiberry closing down: the session must wait for it).
 * The fake is a constructor that runs before main (snitch would reject
 * the arguments of Amiberry) and ends the process.
 */
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <ftw.h>
#include <poll.h>
#include <snitch/snitch.hpp>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
extern "C" {
#include "amiga_emulation.h"
}

#define FAKE_STATE "ASF fake state\n"  /* begins as a savestate */
#define FAKE_NO_START "ASF stuck state\n"      /* loads, never runs the job */

static double now(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

static void nap(double s)
{
    struct timespec ts = { (time_t)s, (long)((s - (time_t)s) * 1e9) };

    nanosleep(&ts, nullptr);
}

static char *read_text(const char *path)
{
    FILE *f = fopen(path, "rb");
    char *s;
    size_t n;

    if (!f)
        return nullptr;
    s = static_cast<char *>(calloc(1, 65536));
    n = s ? fread(s, 1, 65535, f) : 0;
    if (s)
        s[n] = 0;
    fclose(f);
    return s;
}

static void write_text(const char *path, const char *text)
{
    FILE *f = fopen(path, "wb");

    if (f) {
        fputs(text, f);
        fclose(f);
    }
}

static int exists(const char *path)
{
    struct stat st;

    return !stat(path, &st);
}

/* ---------------------------------------------------------- fake Amiberry */

static char late_path[4300];

static void fake_term(int sig)
{
    (void)sig;
    nap(2.5);
    write_text(late_path, "late\n");
    _exit(0);
}

/* the host directory of volume vol in the configuration ("...:VOL:DIR,0") */
static int volume_dir(const char *cfg, const char *vol, char *dir, size_t len)
{
    char key[32];
    const char *p, *c;

    snprintf(key, sizeof key, ":%s:", vol);
    if (!(p = strstr(cfg, key)))
        return -1;
    p += strlen(key);
    c = strchr(p, ',');
    if (!c || (size_t)(c - p) >= len)
        return -1;
    memcpy(dir, p, (size_t)(c - p));
    dir[c - p] = 0;
    return 0;
}

/* the CPU of the job (GET_CPU_REGS): FAKE_CPU_NONE answers OK only (no
   registers); a job with HALT halts the CPU (the registers never change,
   SR with interrupt mask 7 in supervisor mode, as Amiberry's cpu_halt
   leaves a double fault); one with MASK7 runs at interrupt mask 7, its
   registers change (not halted) */
enum fake_cpu_mode { FAKE_CPU_NONE, FAKE_CPU_HALTED, FAKE_CPU_MASK7 };
static fake_cpu_mode fake_cpu;
static unsigned fake_ticks;

static const char *fake_regs(void)
{
    static char buf[512];

    if (fake_cpu == FAKE_CPU_NONE)
        return "OK\n";
    if (fake_cpu == FAKE_CPU_MASK7)
        fake_ticks++;
    snprintf(buf, sizeof buf,
             "OK\tD0=00000001\tD1=%08X\tA7=40140318\tPC=%08X\tSR=2708\tT=0\tS=1\tX=0\tN=1\tZ=0\tV=0"
             "\tC=0\tUSP=4014024C\tISP=40002338\n",
             fake_ticks, fake_cpu == FAKE_CPU_MASK7 ? 0x00F80000u + (fake_ticks & 0xfe) : 0xFFFFFFACu);
    return buf;
}

/* whether this save writes a state that loads but never starts its job:
   while the file $FAKE_BAD_SAVES holds a number > 0 (counted down) */
static int bad_save(void)
{
    const char *path = getenv("FAKE_BAD_SAVES");
    char *s = path ? read_text(path) : nullptr;
    int n = s ? atoi(s) : 0;
    char text[16];

    free(s);
    if (n <= 0)
        return 0;
    snprintf(text, sizeof text, "%d\n", n - 1);
    write_text(path, text);
    return 1;
}

/* one command line of the control socket; 1 = QUIT */
static int fake_command(int fd)
{
    char line[4096], *tab;
    ssize_t n = read(fd, line, sizeof line - 1);
    const char *answer = "OK\n";
    int quit = 0;

    if (n > 0) {
        line[n] = 0;
        line[strcspn(line, "\r\n")] = 0;
        tab = strchr(line, '\t');
        if (tab)
            *tab++ = 0;
        if (!strcmp(line, "SAVESTATE") && tab) {
            char *end = strchr(tab, '\t');

            if (end)
                *end = 0;
            write_text(tab, bad_save() ? FAKE_NO_START : FAKE_STATE);
        }
        quit = !strcmp(line, "QUIT");
        if (!strcmp(line, "GET_CPU_REGS"))
            answer = fake_regs();
    }
    if (write(fd, answer, strlen(answer)) != (ssize_t)strlen(answer))
        quit = 1;               /* the library has gone */
    close(fd);
    return quit;
}

static int fake_amiberry(int argc, char **argv)
{
    char *cfg = read_text("amiberry.uae"), test[4096], snap[4096], path[4200];
    const char *statefile = nullptr, *rt = getenv("XDG_RUNTIME_DIR");
    struct sockaddr_un sa;
    double start = now(), boot = start + 0.3;
    int srv, i, ran = 0, readied = 0;

    for (i = 1; i < argc - 1; i++)
        if (!strcmp(argv[i], "--statefile"))
            statefile = argv[i + 1];
    if (!cfg || !rt || volume_dir(cfg, "TEST", test, sizeof test))
        return 2;
    if (volume_dir(cfg, "SNAP", snap, sizeof snap))
        snap[0] = 0;
    if (statefile) {
        char *s = read_text(statefile);
        int good = s && !strcmp(s, FAKE_STATE), stuck = s && !strcmp(s, FAKE_NO_START);

        free(s);
        if (!good && !stuck)
            return 3;           /* a broken state: Amiberry ends */
        boot = start;
        ran = stuck;            /* loaded, but the job never starts */
    }
    srv = socket(AF_UNIX, SOCK_STREAM, 0);
    memset(&sa, 0, sizeof sa);
    sa.sun_family = AF_UNIX;
    snprintf(sa.sun_path, sizeof sa.sun_path, "%s/amiberry.sock", rt);
    unlink(sa.sun_path);
    if (srv < 0 || bind(srv, (struct sockaddr *)&sa, sizeof sa) || listen(srv, 4))
        return 4;
    /* at most two minutes (the library ends it much earlier) */
    while (now() - start < 120) {
        struct pollfd pfd = { srv, POLLIN, 0 };

        if (poll(&pfd, 1, 20) > 0) {
            int c = accept(srv, nullptr, nullptr);

            if (c >= 0 && fake_command(c)) {
                if (late_path[0])
                    fake_term(0);   /* QUIT: closes down as slowly */
                return 0;
            }
        }
        if (ran || now() < boot)
            continue;
        snprintf(path, sizeof path, "%s/run.script", test);
        if (snap[0] && !statefile && !readied) {
            /* the user-startup: TEST:run.script, the prologue or (a cold
               boot without snapshot) the test's script */
            char *rs = read_text(path);
            int prologue = rs && strstr(rs, "SNAP:waitjob");

            free(rs);
            if (prologue) {
                snprintf(path, sizeof path, "%s/ready", snap);
                write_text(path, "ready\n");
                readied = 1;
            } else {
                snap[0] = 0;
            }
        }
        if (snap[0]) {
            snprintf(path, sizeof path, "%s/job.script", snap);
            if (!exists(path))
                continue;
        }
        {
            char *job = read_text(path);

            ran = 1;
            if (snap[0]) {
                char started[4200];

                snprintf(started, sizeof started, "%s/started", snap);
                write_text(started, "started\n");
            }
            if (job && strstr(job, "HALT"))
                fake_cpu = FAKE_CPU_HALTED;
            else if (job && strstr(job, "MASK7"))
                fake_cpu = FAKE_CPU_MASK7;
            else if (job && !strstr(job, "HANG")) {
                if (strstr(job, "SLOWTERM")) {
                    snprintf(late_path, sizeof late_path, "%s/late", test);
                    signal(SIGTERM, fake_term);
                }
                snprintf(path, sizeof path, "%s/run.done", test);
                write_text(path, "done\n");
            }
            free(job);
        }
    }
    return 0;
}

/* the fake runs instead of the tests when the library starts this program
   as Amiberry (FAKE_AMIBERRY, set by the test case for its sessions) */
__attribute__((constructor)) static void fake_amiberry_hook(int argc, char **argv, char **)
{
    if (getenv("FAKE_AMIBERRY")) {
        int rc = fake_amiberry(argc, argv);

        fflush(nullptr);
        _exit(rc);
    }
}

/* ------------------------------------------------------------------ tests */

static char self[4096];
static char base[] = "/tmp/amiga-emulation-suptest-XXXXXX";
static char slots[256];

/* a session; other 1: another configuration (one change more), hang_after
   the hang check's seconds */
static struct amiga_emulation *start_with(const char *script, unsigned timeout, int other,
                                          unsigned hang_after)
{
    static const struct amiga_emulation_option changes[] = { { "kickstart_rom_file", nullptr },
                                                             { "hardfile2", nullptr },
                                                             { "cpu_speed", "max" } };
    struct amiga_emulation_params p = {};
    struct amiga_emulation *e;
    char err[512];

    p.changes = changes;
    p.nchanges = other ? 3 : 2;
    p.amiberry = self;
    p.script = script;
    p.timeout = timeout;
    p.hang_after = hang_after;
    e = amiga_emulation_start(&p, err, sizeof err);
    if (!e)
        fprintf(stderr, "test_amiga_emulation_supervisor: start: %s\n", err);
    return e;
}

static struct amiga_emulation *start(const char *script, unsigned timeout)
{
    return start_with(script, timeout, 0, 0);
}

static char *snap_log(struct amiga_emulation *e)
{
    char path[4200];

    snprintf(path, sizeof path, "%s/snapshot.log", amiga_emulation_work_dir(e));
    return read_text(path);
}

static void show(const char *name, struct amiga_emulation *e)
{
    struct amiga_emulation_stats st;
    char *log = snap_log(e);

    amiga_emulation_get_stats(e, &st);
    fprintf(stderr, "test_amiga_emulation_supervisor: %s: %s %s session %.2f timed %.2f\n%s", name,
            amiga_emulation_snapshot_use_name(st.snapshot), amiga_emulation_status_name(st.status),
            st.session, st.timed, log ? log : "");
    free(log);
}

/* the slot directory holds nothing but snapshots and lock files */
static int slot_clean(int k)
{
    char dir[300];
    DIR *d;
    struct dirent *de;
    int clean = 1;

    snprintf(dir, sizeof dir, "%s/slot-%d", slots, k);
    if (!(d = opendir(dir)))
        return 1;
    while ((de = readdir(d)) != nullptr)
        if (strcmp(de->d_name, ".") && strcmp(de->d_name, "..") && strncmp(de->d_name, "snap-", 5)) {
            fprintf(stderr, "test_amiga_emulation_supervisor: left in slot-%d: %s\n", k, de->d_name);
            clean = 0;
        }
    closedir(d);
    return clean;
}

static void break_snapshots(const char *text);

/* the files of slot k whose names begin with "snap-" and end with suffix */
static int slot_files(int k, const char *suffix)
{
    char dir[300];
    DIR *d;
    struct dirent *de;
    int n = 0;

    snprintf(dir, sizeof dir, "%s/slot-%d", slots, k);
    if (!(d = opendir(dir)))
        return 0;
    while ((de = readdir(d)) != nullptr) {
        size_t len = strlen(de->d_name), sl = strlen(suffix);

        if (!strncmp(de->d_name, "snap-", 5) && len > sl && !strcmp(de->d_name + len - sl, suffix))
            n++;
    }
    closedir(d);
    return n;
}

/* removes the snapshots (and marks) of both slots */
static void drop_snapshots(void)
{
    char path[700];
    DIR *d;
    struct dirent *de;

    for (int k = 0; k < 2; k++) {
        char dir[300];

        snprintf(dir, sizeof dir, "%s/slot-%d", slots, k);
        if ((d = opendir(dir)) != nullptr) {
            while ((de = readdir(d)) != nullptr)
                if (!strncmp(de->d_name, "snap-", 5)) {
                    snprintf(path, sizeof path, "%s/%s", dir, de->d_name);
                    unlink(path);
                }
            closedir(d);
        }
    }
}

/* one session of the default configuration alone (slot 0): its result,
   the snapshot use, its snapshot.log must have the texts of want (NULL
   ends), and the slot is clean after it */
static void one_session(const char *name, enum amiga_emulation_snapshot_use use,
                        const char *const *want)
{
    struct amiga_emulation *e = start("Echo ok", 10);
    struct amiga_emulation_stats st;
    char *log;
    int ok = 1;

    CHECK(e && amiga_emulation_join(e) == AMIGA_EMULATION_DONE);
    if (!e)
        return;
    amiga_emulation_get_stats(e, &st);
    log = snap_log(e);
    ok = st.snapshot == use && log;
    for (; ok && *want; want++)
        ok = strstr(log, *want) != nullptr;
    CHECK(ok);
    if (!ok)
        show(name, e);
    free(log);
    amiga_emulation_free(e);
    CHECK(slot_clean(0));
}

/* the first session of a configuration that no session has used lately:
   no snapshot, the job runs in the booted machine (use single), the hang
   check works there; the next session of it saves one */
static void single_use(void)
{
    static const char *const single[] = { "no snapshot", "job started", "done", nullptr };
    static const char *const saved[] = { "boot cold, save", " saved ", "job started", nullptr };
    struct amiga_emulation *e;
    struct amiga_emulation_stats st;

    one_session("single", AMIGA_EMULATION_SNAP_SINGLE, single);
    CHECK(slot_files(0, "") == 0);
    one_session("second", AMIGA_EMULATION_SNAP_FIRST_SAVE, saved);
    CHECK(slot_files(0, ".uss") == 1);
    /* another configuration, used once, whose job hangs: the hang check ends it */
    e = start_with("HANG", 30, 1, 2);
    CHECK(e && amiga_emulation_join(e) == AMIGA_EMULATION_TIMEOUT);
    if (e) {
        amiga_emulation_get_stats(e, &st);
        CHECK(st.snapshot == AMIGA_EMULATION_SNAP_SINGLE && st.hung && st.timed < 10);
        if (snitch::impl::get_current_test().failures)
            show("single hang", e);
    }
    amiga_emulation_free(e);
    CHECK(slot_files(0, ".uss") == 1 && slot_clean(0));
    drop_snapshots();
}

/* states that load but never start their job (no SNAP:started): one this
   session has just saved is removed and saved again, at most twice, then
   the session boots cold; an older one is kept with a mark the first
   time, removed the second; a start in between removes the mark */
static void restore_repair(void)
{
    static const char *const once[] = { " saved ", "restore failed (no start): removed, save again",
                                         "job started", nullptr };
    static const char *const twice[] = { "restore failed (no start): removed, boot cold", nullptr };
    static const char *const fresh[] = { " saved ", "job started", nullptr };
    static const char *const kept[] = { "restore failed (no start): kept, boot cold", nullptr };
    static const char *const gone[] = { "restore failed (no start): removed (no start twice), boot cold",
                                        nullptr };
    static const char *const restored[] = { "restore ", "job started", nullptr };
    char bad[300];

    snprintf(bad, sizeof bad, "%s/bad-saves", base);
    setenv("FAKE_BAD_SAVES", bad, 1);
    setenv("AMIGA_EMULATION_SNAP_RESTORE_GRACE", "3", 1);
    /* the first save does not start, the second does */
    drop_snapshots();
    write_text(bad, "1\n");
    one_session("bad once", AMIGA_EMULATION_SNAP_FIRST_SAVE, once);
    CHECK(slot_files(0, ".uss") == 1 && slot_files(0, ".nostart") == 0);
    /* neither does: no snapshot left */
    drop_snapshots();
    write_text(bad, "2\n");
    one_session("bad twice", AMIGA_EMULATION_SNAP_FALLBACK, twice);
    CHECK(slot_files(0, ".uss") == 0);
    /* a kept state that stops starting: a mark, then removed */
    one_session("fresh", AMIGA_EMULATION_SNAP_FIRST_SAVE, fresh);
    break_snapshots(FAKE_NO_START);
    one_session("kept", AMIGA_EMULATION_SNAP_FALLBACK, kept);
    CHECK(slot_files(0, ".uss") == 1 && slot_files(0, ".nostart") == 1);
    one_session("gone", AMIGA_EMULATION_SNAP_FALLBACK, gone);
    CHECK(slot_files(0, ".uss") == 0 && slot_files(0, ".nostart") == 0);
    /* a start between two failures removes the mark: kept again */
    one_session("fresh again", AMIGA_EMULATION_SNAP_FIRST_SAVE, fresh);
    break_snapshots(FAKE_NO_START);
    one_session("kept again", AMIGA_EMULATION_SNAP_FALLBACK, kept);
    break_snapshots(FAKE_STATE);
    one_session("restored", AMIGA_EMULATION_SNAP_RESTORED, restored);
    CHECK(slot_files(0, ".uss") == 1 && slot_files(0, ".nostart") == 0);
    break_snapshots(FAKE_NO_START);
    one_session("kept once more", AMIGA_EMULATION_SNAP_FALLBACK, kept);
    CHECK(slot_files(0, ".uss") == 1 && slot_files(0, ".nostart") == 1);
    drop_snapshots();
    unsetenv("AMIGA_EMULATION_SNAP_RESTORE_GRACE");
    unsetenv("FAKE_BAD_SAVES");
}

/* a session that is done is not killed while Amiberry is still closing
   down (TEST:late is its last write): in snapshot mode and cold */
static void slow_close(void)
{
    int cold;

    for (cold = 0; cold < 2; cold++) {
        struct amiga_emulation *a;
        char path[4300];
        double t0;

        if (cold)
            setenv("AMIGA_EMULATION_SNAPSHOT", "0", 1);
        a = start("SLOWTERM", 30);
        CHECK(a != nullptr);
        if (!a)
            continue;
        t0 = now();
        CHECK(amiga_emulation_join(a) == AMIGA_EMULATION_DONE);
        snprintf(path, sizeof path, "%s/late", amiga_emulation_test_dir(a));
        CHECK(exists(path));
        CHECK(now() - t0 < 12);
        amiga_emulation_free(a);
    }
    unsetenv("AMIGA_EMULATION_SNAPSHOT");
}

/* the first session hangs until its limit while the caller joins it; the
   second, started after it, is a first-save session (fresh slot): it
   saves, restores and runs its job meanwhile.  Its session and time limit
   end with its job, not with the join. */
static void join_order(void)
{
    struct amiga_emulation *a = start("HANG", 3), *b = start("Echo ok", 3);
    struct amiga_emulation_stats st;
    double t0 = now(), joined_a;
    char *log;

    CHECK(a && b);
    if (!a || !b)
        goto out;
    CHECK(amiga_emulation_join(a) == AMIGA_EMULATION_TIMEOUT);
    joined_a = now() - t0;
    CHECK(joined_a > 3);
    amiga_emulation_get_stats(a, &st);
    CHECK(st.snapshot == AMIGA_EMULATION_SNAP_FIRST_SAVE && st.timed >= 3 && st.timed < 4.5);
    CHECK(amiga_emulation_join(b) == AMIGA_EMULATION_DONE);
    amiga_emulation_get_stats(b, &st);
    CHECK(st.snapshot == AMIGA_EMULATION_SNAP_FIRST_SAVE && st.status == AMIGA_EMULATION_DONE);
    /* b ended long before the caller joined it */
    CHECK(st.session > 0 && st.session < joined_a - 1);
    CHECK(st.timed > 0 && st.timed < 3 && st.timed < st.session);
    log = snap_log(b);
    CHECK(log && strstr(log, " saved ") && strstr(log, "job started") && strstr(log, "done"));
    free(log);
    if (snitch::impl::get_current_test().failures) {
        show("a", a);
        show("b", b);
    }
out:
    amiga_emulation_free(a);
    amiga_emulation_free(b);
    CHECK(slot_clean(0) && slot_clean(1));
}

/* the snapshots of both slots get text */
static void break_snapshots(const char *text)
{
    char path[700];
    DIR *d;
    struct dirent *de;

    for (int k = 0; k < 2; k++) {
        char dir[300];

        snprintf(dir, sizeof dir, "%s/slot-%d", slots, k);
        if ((d = opendir(dir)) != nullptr) {
            while ((de = readdir(d)) != nullptr)
                if (!strncmp(de->d_name, "snap-", 5)) {
                    snprintf(path, sizeof path, "%s/%s", dir, de->d_name);
                    write_text(path, text);
                }
            closedir(d);
        }
    }
}

/* a later session restores the slot's snapshot; a broken one that
   Amiberry cannot load is dropped and the session boots cold; a file that
   is no savestate at all is removed before, the session saves anew */
static void restore_and_fallback(void)
{
    struct amiga_emulation *e = start("Echo ok", 5);
    struct amiga_emulation_stats st;

    CHECK(e && amiga_emulation_join(e) == AMIGA_EMULATION_DONE);
    if (e) {
        amiga_emulation_get_stats(e, &st);
        CHECK(st.snapshot == AMIGA_EMULATION_SNAP_RESTORED);
    }
    amiga_emulation_free(e);
    break_snapshots("not a state\n");
    e = start("Echo ok", 5);
    CHECK(e && amiga_emulation_join(e) == AMIGA_EMULATION_DONE);
    if (e) {
        char *log = snap_log(e);

        amiga_emulation_get_stats(e, &st);
        CHECK(st.snapshot == AMIGA_EMULATION_SNAP_FIRST_SAVE);
        CHECK(log && strstr(log, "not a savestate, removed") && strstr(log, "job started"));
        free(log);
    }
    amiga_emulation_free(e);
    /* a savestate the fake cannot load: it ends */
    break_snapshots("ASF broken state\n");
    e = start("Echo ok", 5);
    CHECK(e && amiga_emulation_join(e) == AMIGA_EMULATION_DONE);
    if (e) {
        char *log = snap_log(e);

        amiga_emulation_get_stats(e, &st);
        CHECK(st.snapshot == AMIGA_EMULATION_SNAP_FALLBACK);
        CHECK(log && strstr(log, "restore failed (Amiberry ended)")
              && strstr(log, "dropped, boot cold"));
        free(log);
    }
    amiga_emulation_free(e);
    CHECK(slot_clean(0) && slot_clean(1));
}

/* freed without join: the supervisor stops the session at once; the
   slot is free again also while another session (whose supervisor was
   started while the first one's slot was held) still runs */
static void free_running(void)
{
    struct amiga_emulation *a = start("HANG", 30), *b = start("HANG", 30);
    double t;
    int x;

    CHECK(a && b);
    nap(0.5);
    t = now();
    amiga_emulation_free(a);
    CHECK(now() - t < 4);
    x = amiga_emulation_slot(slots, AMIGA_EMULATION_SLOTS_AUTO, 0);
    CHECK(x >= 0);
    if (x >= 0)
        close(x);
    t = now();
    amiga_emulation_free(b);
    CHECK(now() - t < 4);
    CHECK(slot_clean(0) && slot_clean(1));
}

/* only the supervisor holds the slot: a session that has ended frees its
   slot at once, also while the caller still waits for an earlier session
   (joins in start order); a third start of the same process gets it */
static void slot_free_at_end(void)
{
    struct amiga_emulation *a = start("HANG", 30), *b = start("Echo ok", 30), *c = nullptr;
    double until = now() + 15;
    char done[4200];
    int x = -1;

    CHECK(a && b);
    if (!a || !b)
        goto out;
    snprintf(done, sizeof done, "%s/run.done", amiga_emulation_test_dir(b));
    /* bounded: b ends within seconds, a only at its limit (30 s) */
    while (now() < until && (x = amiga_emulation_slot(slots, AMIGA_EMULATION_SLOTS_AUTO, 0)) < 0)
        nap(0.1);
    CHECK(x >= 0);
    CHECK(exists(done));
    if (x >= 0)
        close(x);
    c = start("Echo ok", 30);
    CHECK(c &&amiga_emulation_join(c) == AMIGA_EMULATION_DONE);
    CHECK(amiga_emulation_join(b) == AMIGA_EMULATION_DONE);
    if (snitch::impl::get_current_test().failures) {
        show("a", a);
        show("b", b);
    }
out:
    amiga_emulation_free(c);
    amiga_emulation_free(a);
    amiga_emulation_free(b);
    CHECK(slot_clean(0) && slot_clean(1));
}

/* a runner's start that finds no free slot while sessions of its own run:
   tried again every AMIGA_EMULATION_SLOT_RETRY_MS, the own sessions that
   end meanwhile collected (the runners' loop) - c gets a slot; the time
   until then */
static struct amiga_emulation *start_retry(const char *script, unsigned timeout,
                                           struct amiga_emulation **own, size_t n, int *collected)
{
    static const struct amiga_emulation_option changes[] = { { "kickstart_rom_file", nullptr },
                                                             { "hardfile2", nullptr } };
    struct amiga_emulation_params p = {};
    struct amiga_emulation *e;
    char err[512];
    int ended;

    p.changes = changes;
    p.nchanges = 2;
    p.amiberry = self;
    p.script = script;
    p.timeout = timeout;
    while (!(e = amiga_emulation_start(&p, err, sizeof err)) && amiga_emulation_no_slot()) {
        CHECK(errno == EAGAIN);
        if ((ended = amiga_emulation_wait_any(own, n, AMIGA_EMULATION_SLOT_RETRY_MS)) >= 0) {
            CHECK(amiga_emulation_finished(own[ended]));
            amiga_emulation_join(own[ended]);
            amiga_emulation_free(own[ended]);
            own[ended] = nullptr;
            (*collected)++;
        }
    }
    if (!e)
        fprintf(stderr, "test_amiga_emulation_supervisor: start: %s\n", err);
    return e;
}

/* both slots are this process's: a runs until its limit (30 s), b ends at
   its limit of 2 s.  The third start waits for b, which is collected; it
   does not wait for a. */
static void retry_own_ended(void)
{
    struct amiga_emulation *none[2] = { nullptr, nullptr };
    double t0 = now(), t;
    int collected = 0;

    /* no sessions: nothing ends, the wait takes its time */
    CHECK(amiga_emulation_finished(nullptr));
    CHECK(amiga_emulation_wait_any(nullptr, 0, 0) == -1);
    CHECK(amiga_emulation_wait_any(none, 2, 200) == -1 && now() - t0 >= 0.19);

    struct amiga_emulation *own[2] = { start("HANG", 30), start("HANG", 2) }, *c = nullptr;

    t0 = now();

    CHECK(own[0] && own[1]);
    if (!own[0] || !own[1])
        goto out;
    CHECK(!amiga_emulation_finished(own[0]) && !amiga_emulation_finished(own[1]));
    /* nothing ends within a short wait */
    t = now();
    CHECK(amiga_emulation_wait_any(own, 2, 300) == -1);
    CHECK(now() - t >= 0.29 && now() - t < 2);
    c = start_retry("Echo ok", 30, own, 2, &collected);
    t = now() - t0;
    CHECK(c != nullptr);
    CHECK(collected == 1 && own[0] && !own[1]);
    CHECK(t > 1.5 && t < 8);
    CHECK(own[0] && !amiga_emulation_finished(own[0]));
    CHECK(c && amiga_emulation_join(c) == AMIGA_EMULATION_DONE);
    if (snitch::impl::get_current_test().failures)
        fprintf(stderr, "test_amiga_emulation_supervisor: retry_own_ended: c after %.2f s\n", t);
out:
    amiga_emulation_free(c);
    amiga_emulation_free(own[0]);
    amiga_emulation_free(own[1]);
    CHECK(slot_clean(0) && slot_clean(1));
}

/* the other slot belongs to another process (another runner) and is freed
   after 1.5 s while this process's session runs on (30 s): the next start
   takes it within a retry or two */
static void retry_foreign_freed(void)
{
    struct amiga_emulation *own[1] = { nullptr }, *c = nullptr;
    int fds[2], collected = 0;
    char got = 0;
    double t;
    pid_t pid;

    CHECK(pipe(fds) == 0);
    pid = fork();
    if (pid == 0) {
        int x = amiga_emulation_slot(slots, AMIGA_EMULATION_SLOTS_AUTO, 1);

        (void)!write(fds[1], x >= 0 ? "y" : "n", 1);
        nap(1.5);
        _exit(0);
    }
    CHECK(pid > 0);
    close(fds[1]);
    CHECK(read(fds[0], &got, 1) == 1 && got == 'y');
    close(fds[0]);
    own[0] = start("HANG", 30);
    CHECK(own[0] != nullptr);
    t = now();
    c = start_retry("Echo ok", 30, own, 1, &collected);
    t = now() - t;
    CHECK(c != nullptr && collected == 0 && own[0] && !amiga_emulation_finished(own[0]));
    CHECK(t > 0.5 && t < 4);
    CHECK(c && amiga_emulation_join(c) == AMIGA_EMULATION_DONE);
    if (snitch::impl::get_current_test().failures)
        fprintf(stderr, "test_amiga_emulation_supervisor: retry_foreign_freed: c after %.2f s\n", t);
    if (pid > 0)
        waitpid(pid, nullptr, 0);
    amiga_emulation_free(c);
    amiga_emulation_free(own[0]);
    CHECK(slot_clean(0) && slot_clean(1));
}

/* two runners (this process and a child), each with up to two sessions at
   once and four in all, on two slots: both get through, none waits for
   ever (every session ends at its limit of 2 s) */
static int run_four(void)
{
    struct amiga_emulation *own[2] = { nullptr, nullptr };
    int collected = 0, started = 0;
    size_t bad = snitch::impl::get_current_test().failures;

    while (started < 4) {
        size_t k = own[0] ? 1 : 0;

        if (own[k]) {
            /* both running: the next that ends */
            int ended = amiga_emulation_wait_any(own, 2, AMIGA_EMULATION_SLOT_RETRY_MS);

            if (ended >= 0) {
                CHECK(amiga_emulation_join(own[ended]) == AMIGA_EMULATION_TIMEOUT);
                amiga_emulation_free(own[ended]);
                own[ended] = nullptr;
            }
            continue;
        }
        own[k] = start_retry("HANG", 2, own, 2, &collected);
        CHECK(own[k] != nullptr);
        if (!own[k])
            break;
        started++;
    }
    for (size_t k = 0; k < 2; k++) {
        if (own[k])
            CHECK(amiga_emulation_join(own[k]) == AMIGA_EMULATION_TIMEOUT);
        amiga_emulation_free(own[k]);
    }
    return snitch::impl::get_current_test().failures != bad;
}

static void retry_two_runners(void)
{
    double t = now();
    int status = -1;
    pid_t pid = fork();

    if (pid == 0)
        _exit(run_four());
    CHECK(pid > 0);
    CHECK(run_four() == 0);
    CHECK(pid > 0 && waitpid(pid, &status, 0) == pid && WIFEXITED(status)
          && WEXITSTATUS(status) == 0);
    t = now() - t;
    /* 8 sessions of about 2 s on 2 slots: 4 rounds */
    CHECK(t < 30);
    if (snitch::impl::get_current_test().failures)
        fprintf(stderr, "test_amiga_emulation_supervisor: retry_two_runners: %.2f s\n", t);
    CHECK(slot_clean(0) && slot_clean(1));
}

/* without snapshot: the session ends with its job too, not at the join */
static void cold(void)
{
    struct amiga_emulation *a, *b;
    struct amiga_emulation_stats st;
    double t0 = now(), joined_a;

    setenv("AMIGA_EMULATION_SNAPSHOT", "0", 1);
    a = start("HANG", 2);
    b = start("Echo ok", 2);
    CHECK(a && b);
    if (a && b) {
        CHECK(amiga_emulation_join(a) == AMIGA_EMULATION_TIMEOUT);
        joined_a = now() - t0;
        CHECK(amiga_emulation_join(b) == AMIGA_EMULATION_DONE);
        amiga_emulation_get_stats(b, &st);
        CHECK(st.snapshot == AMIGA_EMULATION_SNAP_COLD);
        CHECK(st.session > 0 && st.session < joined_a - 0.5 && st.timed <= st.session);
    }
    amiga_emulation_free(a);
    amiga_emulation_free(b);
    unsetenv("AMIGA_EMULATION_SNAPSHOT");
}

/* a job whose CPU halts ends as halt long before its limit, with the
   report of AMIGA_EMULATION_TIMEOUT_STATE; one at interrupt mask 7 whose
   registers change is no halt (it runs to its limit); without snapshot
   the halt ends the session too */
static void halt(void)
{
    struct amiga_emulation *a, *b;
    struct amiga_emulation_stats st;
    char path[4200], *s;

    setenv("AMIGA_EMULATION_TIMEOUT_STATE", "1", 1);
    a = start("HALT", 30);
    b = start("MASK7", 6);
    unsetenv("AMIGA_EMULATION_TIMEOUT_STATE");
    CHECK(a && b);
    if (!a || !b)
        goto out;
    CHECK(amiga_emulation_join(a) == AMIGA_EMULATION_HALT);
    amiga_emulation_get_stats(a, &st);
    CHECK(st.status == AMIGA_EMULATION_HALT && st.timed > 1.5 && st.timed < 10);
    snprintf(path, sizeof path, "%s/timeout.txt", amiga_emulation_work_dir(a));
    s = read_text(path);
    CHECK(s && !strncmp(s, "end: halt\n", 10) && strstr(s, " SR=2708 ")
          && strstr(s, "cpu state: halted"));
    free(s);
    s = snap_log(a);
    CHECK(s && strstr(s, "job started") && strstr(s, "CPU halted"));
    free(s);
    CHECK(amiga_emulation_join(b) == AMIGA_EMULATION_TIMEOUT);
    amiga_emulation_get_stats(b, &st);
    CHECK(st.timed >= 6);
    snprintf(path, sizeof path, "%s/timeout.txt", amiga_emulation_work_dir(b));
    s = read_text(path);
    CHECK(s && !strncmp(s, "end: timeout\n", 13) && strstr(s, " SR=2708 ")
          && strstr(s, "cpu state: running"));
    free(s);
    if (snitch::impl::get_current_test().failures) {
        show("a", a);
        show("b", b);
    }
out:
    amiga_emulation_free(a);
    amiga_emulation_free(b);
    CHECK(slot_clean(0) && slot_clean(1));
    setenv("AMIGA_EMULATION_SNAPSHOT", "0", 1);
    a = start("HALT", 30);
    CHECK(a && amiga_emulation_join(a) == AMIGA_EMULATION_HALT);
    if (a) {
        amiga_emulation_get_stats(a, &st);
        CHECK(st.snapshot == AMIGA_EMULATION_SNAP_COLD && st.timed < 10);
    }
    amiga_emulation_free(a);
    unsetenv("AMIGA_EMULATION_SNAPSHOT");
}

static int rm_entry(const char *path, const struct stat *st, int flag, struct FTW *ftw)
{
    (void)st; (void)flag; (void)ftw;
    remove(path);
    return 0;
}

TEST_CASE("amiga_emulation_supervisor: sessions without the caller (fake Amiberry)",
          "[amiga_emulation_supervisor]")
{
    char runtime[200];
    ssize_t n;

    n = readlink("/proc/self/exe", self, sizeof self - 1);
    if (n <= 0)
        FAIL("readlink /proc/self/exe failed");
    self[n] = 0;
    CHECK(mkdtemp(base) != nullptr);
    snprintf(runtime, sizeof runtime, "%s/run", base);
    snprintf(slots, sizeof slots, "%s/amiga-emulation-slots", runtime);
    CHECK(mkdir(runtime, 0700) == 0);
    setenv("XDG_RUNTIME_DIR", runtime, 1);
    setenv("AMIGA_EMULATION_MAX", "2", 1);
    setenv("FAKE_AMIBERRY", "1", 1);
    unsetenv("AMIGA_EMULATION_SNAPSHOT");
    unsetenv("AMIGA_EMULATION_SHOTS");
    /* the timings below have no room for the report at the time limit
       (set by the emulator test presets) */
    unsetenv("AMIGA_EMULATION_TIMEOUT_STATE");

    single_use();
    join_order();
    restore_and_fallback();
    restore_repair();
    free_running();
    slot_free_at_end();
    retry_own_ended();
    retry_foreign_freed();
    retry_two_runners();
    cold();
    halt();
    slow_close();

    if (!snitch::impl::get_current_test().failures)
        nftw(base, rm_entry, 16, FTW_DEPTH | FTW_PHYS);
    if (snitch::impl::get_current_test().failures)
        fprintf(stderr, "test_amiga_emulation_supervisor: files in %s\n", base);
}
