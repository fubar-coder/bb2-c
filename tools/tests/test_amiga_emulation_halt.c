/*
 * test_amiga_emulation_halt.c - a program that halts the emulated CPU (a
 * jump to an address without memory: Amiberry's cpu_halt, "CPU halted:
 * reason = 3" in its log) ends its session with AMIGA_EMULATION_HALT
 * within seconds, long before the time limit.  The program is a hunk file
 * from memory, put into TEST: before the start.  Exit 77 (skipped) without
 * Amiberry or the emulation data.
 */
#define _XOPEN_SOURCE 700       /* mkdtemp, nftw */
#include <ftw.h>
#include <stdint.h>
#include <sys/stat.h>
#include <unistd.h>
#include "testutil.h"
#include "amiga_emulation.h"

/* one code hunk: jmp $FFFFFFAC.l; nop */
static const uint32_t halt_prog[] = {
    0x000003F3, 0, 1, 0, 0, 2,          /* HUNK_HEADER: one hunk of 2 longs */
    0x000003E9, 2, 0x4EF9FFFF, 0xFFAC4E71,  /* HUNK_CODE */
    0x000003F2,                         /* HUNK_END */
};

static int write_prog(const char *path)
{
    FILE *f = fopen(path, "wb");
    size_t i;
    int ok = f != NULL;

    for (i = 0; ok && i < sizeof halt_prog / sizeof halt_prog[0]; i++) {
        unsigned char b[4] = { (unsigned char)(halt_prog[i] >> 24),
                               (unsigned char)(halt_prog[i] >> 16),
                               (unsigned char)(halt_prog[i] >> 8), (unsigned char)halt_prog[i] };

        ok = fwrite(b, 1, 4, f) == 4;
    }
    if (f && fclose(f))
        ok = 0;
    return ok;
}

static int file_exists_in(const char *dir, const char *name)
{
    char path[4096];
    struct stat st;

    snprintf(path, sizeof path, "%s/%s", dir, name);
    return !stat(path, &st);
}

static char *read_text(const char *dir, const char *name)
{
    char path[4096];
    FILE *f;
    char *s = calloc(1, 1 << 20);

    snprintf(path, sizeof path, "%s/%s", dir, name);
    if (s && (f = fopen(path, "r")) != NULL) {
        size_t n = fread(s, 1, (1 << 20) - 1, f);

        s[n] = 0;
        fclose(f);
    }
    return s;
}

static int rm_entry(const char *path, const struct stat *st, int flag, struct FTW *ftw)
{
    (void)st; (void)flag; (void)ftw;
    remove(path);
    return 0;
}

int main(void)
{
    struct amiga_emulation_params p = { 0 };
    struct amiga_emulation_stats st;
    struct amiga_emulation *e;
    const char *tmp = getenv("TMPDIR");
    char work[4096], path[4200], err[300], *log, *txt;

    snprintf(work, sizeof work, "%s/amiga-emulation-halt-XXXXXX", tmp && *tmp ? tmp : "/tmp");
    if (!mkdtemp(work)) {
        perror("mkdtemp");
        return 1;
    }
    snprintf(path, sizeof path, "%s/test", work);
    CHECK(mkdir(path, 0700) == 0);
    snprintf(path, sizeof path, "%s/test/halt", work);
    CHECK(write_prog(path));
    p.script = "Echo \"before\" >TEST:before.txt\nTEST:halt\nEcho \"after\" >TEST:after.txt\n";
    p.timeout = 90;
    p.log = 1;
    p.work_dir = work;
    setenv("AMIGA_EMULATION_TIMEOUT_STATE", "1", 1);
    e = amiga_emulation_start(&p, err, sizeof err);
    if (!e) {
        fprintf(stderr, "skipped: %s\n", err);
        nftw(work, rm_entry, 16, FTW_DEPTH | FTW_PHYS);
        return 77;
    }
    CHECK(amiga_emulation_join(e) == AMIGA_EMULATION_HALT);
    amiga_emulation_stats(e, &st);
    /* the halt about a second after the job's start, three answers */
    CHECK(st.status == AMIGA_EMULATION_HALT && st.timed < 30);
    CHECK(file_exists_in(amiga_emulation_test_dir(e), "before.txt"));
    CHECK(!file_exists_in(amiga_emulation_test_dir(e), "after.txt"));
    CHECK(file_exists_in(amiga_emulation_work_dir(e), "timeout.png"));
    /* Amiberry's own view, and the report */
    log = read_text(amiga_emulation_work_dir(e), "amiberry.log");
    CHECK(log && strstr(log, "CPU halted: reason = 3"));
    txt = read_text(amiga_emulation_work_dir(e), "timeout.txt");
    CHECK(txt && !strncmp(txt, "end: halt\n", 10) && strstr(txt, "PC=FFFFFFAC")
          && strstr(txt, "cpu state: halted"));
    if (failures)
        fprintf(stderr, "test_amiga_emulation_halt: %s after %.2f s (limit %u), files in %s\n%s",
                amiga_emulation_status_name(st.status), st.timed, st.timeout, work,
                txt ? txt : "");
    free(log);
    free(txt);
    amiga_emulation_free(e);
    if (!failures)
        nftw(work, rm_entry, 16, FTW_DEPTH | FTW_PHYS);
    return failures != 0;
}
