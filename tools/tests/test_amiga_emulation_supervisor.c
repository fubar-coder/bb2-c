/*
 * test_amiga_emulation_supervisor.c - a session runs without the caller:
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
 * one ends it, as a failed restore); a job writes SNAP:started, a script
 * writes TEST:run.done unless it contains HANG.
 */
#define _XOPEN_SOURCE 700       /* mkdtemp, setenv */
#define _DEFAULT_SOURCE         /* readlink */
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <ftw.h>
#include <poll.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>
#include "testutil.h"
#include "amiga_emulation.h"

#define FAKE_STATE "ASF fake state\n"  /* begins as a savestate */

static double now(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

static void nap(double s)
{
    struct timespec ts = { (time_t)s, (long)((s - (time_t)s) * 1e9) };

    nanosleep(&ts, NULL);
}

static char *read_text(const char *path)
{
    FILE *f = fopen(path, "rb");
    char *s;
    size_t n;

    if (!f)
        return NULL;
    s = calloc(1, 65536);
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
static enum { FAKE_CPU_NONE, FAKE_CPU_HALTED, FAKE_CPU_MASK7 } fake_cpu;
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
            write_text(tab, FAKE_STATE);
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
    const char *statefile = NULL, *rt = getenv("XDG_RUNTIME_DIR");
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
        int good = s && !strcmp(s, FAKE_STATE);

        free(s);
        if (!good)
            return 3;           /* a broken state: Amiberry ends */
        boot = start;
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
            int c = accept(srv, NULL, NULL);

            if (c >= 0 && fake_command(c))
                return 0;
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
                snprintf(path, sizeof path, "%s/run.done", test);
                write_text(path, "done\n");
            }
            free(job);
        }
    }
    return 0;
}

/* ------------------------------------------------------------------ tests */

static char self[4096];
static char base[] = "/tmp/amiga-emulation-suptest-XXXXXX";
static char slots[256];

static struct amiga_emulation *start(const char *script, unsigned timeout)
{
    struct amiga_emulation_params p = { 0 };
    struct amiga_emulation *e;
    char err[512];

    p.changes = (const struct amiga_emulation_option[]){ { "kickstart_rom_file", NULL },
                                                         { "hardfile2", NULL } };
    p.nchanges = 2;
    p.amiberry = self;
    p.script = script;
    p.timeout = timeout;
    e = amiga_emulation_start(&p, err, sizeof err);
    if (!e)
        fprintf(stderr, "test_amiga_emulation_supervisor: start: %s\n", err);
    return e;
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

    amiga_emulation_stats(e, &st);
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
    while ((de = readdir(d)) != NULL)
        if (strcmp(de->d_name, ".") && strcmp(de->d_name, "..") && strncmp(de->d_name, "snap-", 5)) {
            fprintf(stderr, "test_amiga_emulation_supervisor: left in slot-%d: %s\n", k, de->d_name);
            clean = 0;
        }
    closedir(d);
    return clean;
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
    amiga_emulation_stats(a, &st);
    CHECK(st.snapshot == AMIGA_EMULATION_SNAP_FIRST_SAVE && st.timed >= 3 && st.timed < 4.5);
    CHECK(amiga_emulation_join(b) == AMIGA_EMULATION_DONE);
    amiga_emulation_stats(b, &st);
    CHECK(st.snapshot == AMIGA_EMULATION_SNAP_FIRST_SAVE && st.status == AMIGA_EMULATION_DONE);
    /* b ended long before the caller joined it */
    CHECK(st.session > 0 && st.session < joined_a - 1);
    CHECK(st.timed > 0 && st.timed < 3 && st.timed < st.session);
    log = snap_log(b);
    CHECK(log && strstr(log, " saved ") && strstr(log, "job started") && strstr(log, "done"));
    free(log);
    if (failures) {
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
        if ((d = opendir(dir)) != NULL) {
            while ((de = readdir(d)) != NULL)
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
        amiga_emulation_stats(e, &st);
        CHECK(st.snapshot == AMIGA_EMULATION_SNAP_RESTORED);
    }
    amiga_emulation_free(e);
    break_snapshots("not a state\n");
    e = start("Echo ok", 5);
    CHECK(e && amiga_emulation_join(e) == AMIGA_EMULATION_DONE);
    if (e) {
        char *log = snap_log(e);

        amiga_emulation_stats(e, &st);
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

        amiga_emulation_stats(e, &st);
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
    struct amiga_emulation *a = start("HANG", 30), *b = start("Echo ok", 30), *c = NULL;
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
    if (failures) {
        show("a", a);
        show("b", b);
    }
out:
    amiga_emulation_free(c);
    amiga_emulation_free(a);
    amiga_emulation_free(b);
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
        amiga_emulation_stats(b, &st);
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
    amiga_emulation_stats(a, &st);
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
    amiga_emulation_stats(b, &st);
    CHECK(st.timed >= 6);
    snprintf(path, sizeof path, "%s/timeout.txt", amiga_emulation_work_dir(b));
    s = read_text(path);
    CHECK(s && !strncmp(s, "end: timeout\n", 13) && strstr(s, " SR=2708 ")
          && strstr(s, "cpu state: running"));
    free(s);
    if (failures) {
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
        amiga_emulation_stats(a, &st);
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

int main(int argc, char **argv)
{
    char runtime[200];
    ssize_t n;

    if (getenv("FAKE_AMIBERRY"))
        return fake_amiberry(argc, argv);
    n = readlink("/proc/self/exe", self, sizeof self - 1);
    if (n <= 0) {
        if (!realpath(argv[0], self))
            return 1;
    } else {
        self[n] = 0;
    }
    CHECK(mkdtemp(base) != NULL);
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

    join_order();
    restore_and_fallback();
    free_running();
    slot_free_at_end();
    cold();
    halt();

    if (!failures)
        nftw(base, rm_entry, 16, FTW_DEPTH | FTW_PHYS);
    if (failures)
        fprintf(stderr, "test_amiga_emulation_supervisor: %d failures (files in %s)\n", failures,
                base);
    return failures != 0;
}
