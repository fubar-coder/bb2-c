/*
 * test_amiga_emulation_run.c - amiga_emulation with the emulator: four
 * runs, in parallel as far as emulator slots are free - a script to its
 * end with control commands, the RTG variant, a second plain run, a
 * script that does not end (time limit, screenshots every 2 seconds);
 * then an emulator whose starting process dies (Linux: it dies too).
 * Exit 77 (skipped) without Amiberry or the emulation data.
 */
#define _XOPEN_SOURCE 700       /* mkdtemp, nftw, readlink, nanosleep */
#include <dirent.h>
#include <errno.h>
#include <ftw.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "testutil.h"
#include "amiga_emulation.h"

static char *read_text(const char *dir, const char *name)
{
    char path[4096];
    FILE *f;
    char *s = calloc(1, 4096);

    snprintf(path, sizeof path, "%s/%s", dir, name);
    if (s && (f = fopen(path, "r")) != NULL) {
        size_t n = fread(s, 1, 4095, f);

        s[n] = 0;
        fclose(f);
    }
    return s;
}

static int file_exists_in(const char *dir, const char *name)
{
    char path[4096];
    FILE *f;

    snprintf(path, sizeof path, "%s/%s", dir, name);
    if (!(f = fopen(path, "r")))
        return 0;
    fclose(f);
    return 1;
}

#ifdef __linux__
static int remove_entry(const char *path, const struct stat *st, int flag, struct FTW *ftw)
{
    (void)st, (void)flag, (void)ftw;
    return remove(path);
}

/* 1 while a process runs in the directory dir (Amiberry chdirs to its
   work directory) */
static int runs_in(const char *dir)
{
    DIR *d = opendir("/proc");
    struct dirent *de;
    char path[300], link[4096];
    int found = 0;
    ssize_t n;

    if (!d)
        return 0;
    while (!found && (de = readdir(d)) != NULL) {
        if (de->d_name[0] < '1' || de->d_name[0] > '9')
            continue;
        snprintf(path, sizeof path, "/proc/%s/cwd", de->d_name);
        n = readlink(path, link, sizeof link - 1);
        if (n > 0) {
            link[n] = 0;
            found = !strcmp(link, dir);
        }
    }
    closedir(d);
    return found;
}

/* a process starts an emulation and ends without amiga_emulation_free:
   the emulator must not outlive it */
static void orphan(void)
{
    struct amiga_emulation_params p = { 0 };
    const char *status[] = { "GET_STATUS", NULL };
    char dir[] = "/tmp/amiga-emulation-orphan-XXXXXX", err[300], reply[256];
    pid_t helper;
    int i, alive;

    if (!mkdtemp(dir)) {
        CHECK(!"mkdtemp");
        return;
    }
    p.script = "Wait 200\n";
    p.timeout = 60;
    p.work_dir = dir;
    helper = fork();
    if (helper == 0) {
        struct amiga_emulation *e = amiga_emulation_start(&p, err, sizeof err);

        /* wait until Amiberry answers, then end with it running */
        for (i = 0; e && i < 100; i++) {
            if (amiga_emulation_command(e, status, reply, sizeof reply) == 0)
                _exit(0);
            nanosleep(&(struct timespec){ 0, 200000000 }, NULL);
        }
        _exit(1);
    }
    CHECK(helper > 0);
    if (helper <= 0)
        return;
    waitpid(helper, &i, 0);
    CHECK(WIFEXITED(i) && WEXITSTATUS(i) == 0);
    for (i = 0; (alive = runs_in(dir)) && i < 50; i++)
        nanosleep(&(struct timespec){ 0, 200000000 }, NULL);
    CHECK(!alive);
    nftw(dir, remove_entry, 16, FTW_DEPTH | FTW_PHYS);
}
#endif

/* the four runs: a plain one (control commands), the RTG variant, a
   second plain one, one that hangs */
enum { PLAIN_A, RTG_B, PLAIN_C, HANG_D, NRUNS };

/* waits for run i, checks what it left, frees it */
static void finish(int i, struct amiga_emulation *e)
{
    static const enum amiga_emulation_status want[NRUNS] = {
        AMIGA_EMULATION_DONE, AMIGA_EMULATION_DONE, AMIGA_EMULATION_DONE, AMIGA_EMULATION_TIMEOUT
    };
    char *s;

    CHECK(amiga_emulation_join(e) == want[i]);
    switch (i) {
    case PLAIN_A:
        s = read_text(amiga_emulation_test_dir(e), "hallo.txt");
        CHECK(s && !strcmp(s, "hallo\n"));
        free(s);
        CHECK(!file_exists_in(amiga_emulation_work_dir(e), "shots/0001.png"));
        break;
    case RTG_B:
        s = read_text(amiga_emulation_test_dir(e), "version.txt");
        CHECK(s && strstr(s, "Kickstart 47"));
        free(s);
        break;
    case PLAIN_C:
        CHECK(file_exists_in(amiga_emulation_test_dir(e), "hallo.txt"));
        break;
    default:
        /* the hanging run: its output so far and the screenshot */
        CHECK(file_exists_in(amiga_emulation_test_dir(e), "before.txt"));
        CHECK(!file_exists_in(amiga_emulation_test_dir(e), "run.done"));
        CHECK(file_exists_in(amiga_emulation_work_dir(e), "timeout.png"));
        CHECK(file_exists_in(amiga_emulation_work_dir(e), "shots/0001.png"));
        /* the state at the limit: Amiberry answered, saved the machine */
        s = read_text(amiga_emulation_work_dir(e), "timeout.txt");
        CHECK(s && strstr(s, "screenshot: answered\n") && strstr(s, "state: timeout.uss\n"));
        /* the CPU of a script that waits: running, not halted */
        CHECK(s && strstr(s, "cpu: D0=") && strstr(s, " SR=") && strstr(s, "cpu state: running\n"));
#ifdef __linux__
        CHECK(s && strstr(s, "threads (tid name state ticks/s wchan):\n  "));
#endif
        free(s);
        CHECK(file_exists_in(amiga_emulation_work_dir(e), "timeout.uss"));
        CHECK(file_exists_in(amiga_emulation_work_dir(e), "timeout.uae"));
        break;
    }
    amiga_emulation_free(e);
}

int main(void)
{
    struct amiga_emulation_params plain = { 0 }, rtg = { 0 }, hang = { 0 };
    const struct amiga_emulation_params *p[NRUNS] = { &plain, &rtg, &plain, &hang };
    struct amiga_emulation *e[NRUNS] = { NULL };
    char err[300], reply[256];
    const char *status[] = { "GET_STATUS", NULL };
    int i, together = 0;

    plain.script = "Version >TEST:version.txt\nEcho \"hallo\" >TEST:hallo.txt\n";
    plain.timeout = 60;
    rtg = plain;
    rtg.changes = amiga_emulation_rtg;
    rtg.nchanges = amiga_emulation_rtg_count;
    hang.script = "Echo \"before\" >TEST:before.txt\nWait 200\n";
    hang.timeout = 25;

    /* in parallel as far as there are free slots: without one, the start
       is tried again (the slot limit is hard) and the runs that have
       ended meanwhile are finished */
    for (i = 0; i < NRUNS; i++) {
        int running = 0;

        /* screenshots every 2 seconds and the state at the limit, only
           for the hanging run */
        if (i == HANG_D) {
            setenv("AMIGA_EMULATION_SHOTS", "2", 1);
            setenv("AMIGA_EMULATION_TIMEOUT_STATE", "1", 1);
        }
        while (!(e[i] = amiga_emulation_start(p[i], err, sizeof err)) && amiga_emulation_no_slot()
               && i) {
            int ended;

            CHECK(errno == EAGAIN);
            if ((ended = amiga_emulation_wait_any(e, (size_t)i, AMIGA_EMULATION_SLOT_RETRY_MS)) >= 0) {
                finish(ended, e[ended]);
                e[ended] = NULL;
            }
        }
        unsetenv("AMIGA_EMULATION_SHOTS");
        unsetenv("AMIGA_EMULATION_TIMEOUT_STATE");
        if (!e[i] && i == 0) {
            fprintf(stderr, "skipped: %s\n", err);
            return 77;
        }
        CHECK(e[i] != NULL);
        if (!e[i]) {
            fprintf(stderr, "test_amiga_emulation_run: %s\n", err);
            return 1;
        }
        for (int k = 0; k <= i; k++)
            running += e[k] != NULL;
        if (running > together)
            together = running;
        /* the control socket while it runs */
        if (i == PLAIN_A)
            CHECK(amiga_emulation_command(e[i], status, reply, sizeof reply) == 0
                  && !strncmp(reply, "OK", 2));
    }
    printf("test_amiga_emulation_run: up to %d runs at the same time\n", together);
    for (i = 0; i < NRUNS; i++)
        if (e[i])
            finish(i, e[i]);
#ifdef __linux__
    orphan();
#endif
    if (failures)
        fprintf(stderr, "test_amiga_emulation_run: %d failures\n", failures);
    return failures != 0;
}
