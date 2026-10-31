/*
 * test_amiga_emulation_a500.c - the 68000 machines a500, a500-68010 and
 * a600 with the emulator, one after the other: the Kickstart (1.3, 3.2 on
 * the a600) boots from the hardfile, its startup runs TEST:run.script
 * (AmigaDOS 1.3: the redirection right behind the command name), the run
 * ends with run.done; Amiberry's log names the CPU.  Exit 77 (skipped)
 * without Amiberry or the A500 data; the a600 is skipped alone without
 * its ROM (amiga-emulation/README.md, "A500", "A600").
 */
#include "testutil.h"
#include "amiga_emulation.h"

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

/* 1 = passed, 0 = failed, -1 = cannot start (skipped) */
static int run(const char *machine, const char *kickstart, const char *cpu)
{
    struct amiga_emulation_params p = { 0 };
    struct amiga_emulation *e;
    char err[300], *version, *log;
    int ok;

    p.machine = machine;
    p.script = "Version >TEST:version.txt\n";
    p.timeout = 90;
    p.log = 1;
    if (!(e = amiga_emulation_start(&p, err, sizeof err))) {
        fprintf(stderr, "test_amiga_emulation_a500: %s: %s\n", machine, err);
        return -1;
    }
    ok = amiga_emulation_join(e) == AMIGA_EMULATION_DONE;
    version = read_text(amiga_emulation_test_dir(e), "version.txt");
    log = read_text(amiga_emulation_work_dir(e), "amiberry.log");
    ok = ok && version && strstr(version, kickstart);
    ok = ok && log && strstr(log, cpu);
    if (!ok)
        fprintf(stderr, "test_amiga_emulation_a500: %s: version \"%s\", %s %s in the log\n",
                machine, version ? version : "", cpu, log && strstr(log, cpu) ? "" : "not");
    free(version);
    free(log);
    amiga_emulation_free(e);
    return ok;
}

int main(void)
{
    int r = run("a500", "Kickstart version 34.5", "CPU=68000,");

    if (r < 0)
        return 77;
    CHECK(r == 1);
    CHECK(run("a500-68010", "Kickstart version 34.5", "CPU=68010,") == 1);
    r = run("a600", "Kickstart 47.115", "CPU=68000,");
    if (r < 0)
        fprintf(stderr, "test_amiga_emulation_a500: a600 skipped (no ROM)\n");
    else
        CHECK(r == 1);
    if (failures)
        fprintf(stderr, "test_amiga_emulation_a500: %d failures\n", failures);
    return failures != 0;
}
