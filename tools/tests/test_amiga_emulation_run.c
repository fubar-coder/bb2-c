/*
 * test_amiga_emulation_run.c - amiga_emulation with the emulator: four
 * runs in parallel - a script to its end with control commands, the RTG
 * variant, a second plain run, a script that does not end (time limit);
 * then an emulator whose starting process dies (Linux: it dies too).
 * Exit 77 (skipped) without Amiberry or the emulation data.
 */
#define _XOPEN_SOURCE 700       /* mkdtemp, nftw, readlink, nanosleep */
#include <dirent.h>
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

int main(void)
{
    struct amiga_emulation_params plain = { 0 }, rtg = { 0 }, hang = { 0 };
    struct amiga_emulation *a, *b, *c, *d;
    char err[300], reply[256], *s;
    const char *status[] = { "GET_STATUS", NULL };

    plain.script = "Version >TEST:version.txt\nEcho \"hallo\" >TEST:hallo.txt\n";
    plain.timeout = 60;
    rtg = plain;
    rtg.changes = amiga_emulation_rtg;
    rtg.nchanges = amiga_emulation_rtg_count;
    hang.script = "Echo \"before\" >TEST:before.txt\nWait 200\n";
    hang.timeout = 25;

    a = amiga_emulation_start(&plain, err, sizeof err);
    if (!a) {
        fprintf(stderr, "skipped: %s\n", err);
        return 77;
    }
    b = amiga_emulation_start(&rtg, err, sizeof err);
    c = amiga_emulation_start(&plain, err, sizeof err);
    d = amiga_emulation_start(&hang, err, sizeof err);
    CHECK(b && c && d);
    if (!b || !c || !d)
        return 1;

    /* the control socket while it runs */
    CHECK(amiga_emulation_command(a, status, reply, sizeof reply) == 0 && !strncmp(reply, "OK", 2));

    CHECK(amiga_emulation_join(a) == AMIGA_EMULATION_DONE);
    CHECK(amiga_emulation_join(b) == AMIGA_EMULATION_DONE);
    CHECK(amiga_emulation_join(c) == AMIGA_EMULATION_DONE);
    CHECK(amiga_emulation_join(d) == AMIGA_EMULATION_TIMEOUT);

    s = read_text(amiga_emulation_test_dir(a), "hallo.txt");
    CHECK(s && !strcmp(s, "hallo\n"));
    free(s);
    s = read_text(amiga_emulation_test_dir(b), "version.txt");
    CHECK(s && strstr(s, "Kickstart 47"));
    free(s);
    CHECK(file_exists_in(amiga_emulation_test_dir(c), "hallo.txt"));
    /* the hanging run: its output so far and the screenshot */
    CHECK(file_exists_in(amiga_emulation_test_dir(d), "before.txt"));
    CHECK(!file_exists_in(amiga_emulation_test_dir(d), "run.done"));
    CHECK(file_exists_in(amiga_emulation_work_dir(d), "timeout.png"));

    amiga_emulation_free(a);
    amiga_emulation_free(b);
    amiga_emulation_free(c);
    amiga_emulation_free(d);
#ifdef __linux__
    orphan();
#endif
    if (failures)
        fprintf(stderr, "test_amiga_emulation_run: %d failures\n", failures);
    return failures != 0;
}
