/*
 * test_amiga_emulation_run.c - amiga_emulation with the emulator: four
 * runs in parallel - a script to its end with control commands, the RTG
 * variant, a second plain run, a script that does not end (time limit).
 * Exit 77 (skipped) without Amiberry or the emulation data.
 */
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
    if (failures)
        fprintf(stderr, "test_amiga_emulation_run: %d failures\n", failures);
    return failures != 0;
}
