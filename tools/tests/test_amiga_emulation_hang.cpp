/*
 * test_amiga_emulation_hang - the hang check (params.hang_after): a
 * program that loops for ever and writes nothing ends its session as a
 * timeout after hang_after seconds of silence, long before the time limit
 * (and says hung); so does a program that raises a dead-end alert (the machine
 * stays in the alert for about 10 s, then reboots and runs the job again, and
 * again: the original's compiler crashes); a script that keeps writing for longer than hang_after
 * is not a hang and runs to its end; without hang_after the same loop runs
 * to the time limit.  The program is a hunk file from memory, put into
 * TEST: before the start.  The check needs the snapshot mode (a cold boot
 * is silent): exit 77 (skipped) without Amiberry or the emulation data, or
 * where the session has no slot (AMIGA_EMULATION_MAX=0).  hang_arm: the check
 * counts from the second a file is there (a compile that is silent for long is
 * no hang).
 */
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ftw.h>
#include <snitch/snitch.hpp>
#include <sys/stat.h>
#include <unistd.h>
extern "C" {
#include "amiga_emulation.h"
}

/* one code hunk: bra.s * (the loop that never ends); nop */
static const uint32_t loop_prog[] = {
    0x000003F3, 0, 1, 0, 0, 1,          /* HUNK_HEADER: one hunk of 1 long */
    0x000003E9, 1, 0x60FE4E71,          /* HUNK_CODE */
    0x000003F2,                         /* HUNK_END */
};
/* the same with Alert($80000005) (a dead-end alert): movea.l 4.w,a6;
   move.l #$80000005,d7; jsr -108(a6); rts */
static const uint32_t alert_prog[] = {
    0x000003F3, 0, 1, 0, 0, 4,
    0x000003E9, 4, 0x2C780004, 0x2E3C8000, 0x00054EAE, 0xFF944E75,
    0x000003F2,
};
struct prog {
    const uint32_t *l;
    size_t n;
};
static const struct prog loop = { loop_prog, sizeof loop_prog / sizeof loop_prog[0] };
static const struct prog alert = { alert_prog, sizeof alert_prog / sizeof alert_prog[0] };

static int write_prog(const char *path, const struct prog *pr)
{
    FILE *f = fopen(path, "wb");
    size_t i;
    int ok = f != NULL;

    for (i = 0; ok && i < pr->n; i++) {
        unsigned char b[4] = { (unsigned char)(pr->l[i] >> 24), (unsigned char)(pr->l[i] >> 16),
                               (unsigned char)(pr->l[i] >> 8), (unsigned char)pr->l[i] };

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
    char *s = static_cast<char *>(calloc(1, 1 << 20));

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

/* one session in its own work directory; 77: it cannot run */
static int run(const char *script, unsigned hang_after, const char *arm, unsigned timeout,
               const struct prog *pr,
               enum amiga_emulation_status want, int want_hung, double max_timed, double min_timed,
               const char *what)
{
    struct amiga_emulation_params p = {};
    struct amiga_emulation_stats st;
    struct amiga_emulation *e;
    const char *tmp = getenv("TMPDIR");
    char work[4096], path[4200], armpath[4200], err[300], *txt;
    size_t before = snitch::impl::get_current_test().failures;

    snprintf(work, sizeof work, "%s/amiga-emulation-hang-XXXXXX", tmp && *tmp ? tmp : "/tmp");
    if (!mkdtemp(work)) {
        FAIL("mkdtemp failed");
    }
    snprintf(path, sizeof path, "%s/test", work);
    CHECK(mkdir(path, 0700) == 0);
    snprintf(path, sizeof path, "%s/test/hang", work);
    if (pr)
        CHECK(write_prog(path, pr));
    p.script = script;
    p.timeout = timeout;
    p.hang_after = hang_after;
    snprintf(armpath, sizeof armpath, "%s/test/%s", work, arm ? arm : "");
    p.hang_arm = arm ? armpath : nullptr;
    p.work_dir = work;
    setenv("AMIGA_EMULATION_TIMEOUT_STATE", "1", 1);
    e = amiga_emulation_start(&p, err, sizeof err);
    if (!e) {
        fprintf(stderr, "skipped: %s\n", err);
        nftw(work, rm_entry, 16, FTW_DEPTH | FTW_PHYS);
        return 77;
    }
    CHECK(amiga_emulation_join(e) == want);
    amiga_emulation_get_stats(e, &st);
    if (st.snapshot == AMIGA_EMULATION_SNAP_COLD || st.snapshot == AMIGA_EMULATION_SNAP_FALLBACK) {
        fprintf(stderr, "skipped: no snapshot mode, the hang check does not apply\n");
        amiga_emulation_free(e);
        nftw(work, rm_entry, 16, FTW_DEPTH | FTW_PHYS);
        return 77;
    }
    CHECK(st.status == want);
    CHECK(st.hung == want_hung);
    CHECK(st.timed < max_timed && st.timed >= min_timed);
    txt = read_text(amiga_emulation_work_dir(e), "timeout.txt");
    if (want == AMIGA_EMULATION_TIMEOUT) {
        CHECK(file_exists_in(amiga_emulation_work_dir(e), "timeout.png"));
        CHECK(txt && !strncmp(txt, "end: timeout\n", 13));
        CHECK((want_hung != 0) == (txt && strstr(txt, "\nhang: ") != NULL));
    }
    if (snitch::impl::get_current_test().failures != before)
        fprintf(stderr, "test_amiga_emulation_hang: %s: %s after %.2f s (limit %u, hang_after %u,"
                " hung %d), files in %s\n%s", what, amiga_emulation_status_name(st.status),
                st.timed, st.timeout, hang_after, st.hung, work, txt ? txt : "");
    free(txt);
    amiga_emulation_free(e);
    if (snitch::impl::get_current_test().failures == before)
        nftw(work, rm_entry, 16, FTW_DEPTH | FTW_PHYS);
    return 0;
}

TEST_CASE("amiga_emulation_hang: the hang check", "[amiga_emulation_hang]")
{
    /* the program loops at once: the files are those before it */
    static const char hang_script[] = "Echo \"before\" >TEST:before.txt\nTEST:hang\n"
                                      "Echo \"after\" >TEST:after.txt\n";
    /* a script that writes a line every 4 s for about 20 s: more than hang_after */
    static const char tick_script[] =
        "Echo \"1\" >TEST:tick.txt\nWait 4\nEcho \"2\" >>TEST:tick.txt\nWait 4\n"
        "Echo \"3\" >>TEST:tick.txt\nWait 4\nEcho \"4\" >>TEST:tick.txt\nWait 4\n"
        "Echo \"5\" >>TEST:tick.txt\nWait 4\nEcho \"6\" >>TEST:tick.txt\n";
    /* silent for 15 s, then arm.txt, then the loop */
    static const char arm_script[] = "Wait 15\nEcho \"armed\" >TEST:arm.txt\nTEST:hang\n";
    int rc;

    /* silent for 6 s: ended at once as a timeout, hung, far before the limit of 90 s */
    rc = run(hang_script, 6, NULL, 90, &loop, AMIGA_EMULATION_TIMEOUT, 1, 45, 0, "hang_after 6");
    if (rc == 77)
        std::exit(77);          // skipped (the exit status of ctest SKIP_RETURN_CODE)
    /* the same loop without hang_after: the whole limit (not hung) */
    run(hang_script, 0, NULL, 14, &loop, AMIGA_EMULATION_TIMEOUT, 0, 40, 13, "no hang_after");
    /* a dead-end alert: silent in the alert, before the reboot */
    run(hang_script, 6, NULL, 90, &alert, AMIGA_EMULATION_TIMEOUT, 1, 45, 0, "dead-end alert");
    /* broken on purpose: a job that is silent less than hang_after at a time
       is no hang, though it runs longer than hang_after */
    run(tick_script, 12, NULL, 120, NULL, AMIGA_EMULATION_DONE, 0, 90, 0, "writing job");
    /* hang_arm: the silence counts from the second the file is there.  A job
       silent for 15 s (a compile on a loaded host) and then a loop: without
       hang_arm the 15 s count (hung within 15 s), with it the check starts at
       arm.txt (hung 6 s after it, not before 20 s); a file that never appears
       gives no hang check at all (the limit) */
    run(arm_script, 6, NULL, 90, &loop, AMIGA_EMULATION_TIMEOUT, 1, 15, 0, "silent start, no arm");
    run(arm_script, 6, "arm.txt", 90, &loop, AMIGA_EMULATION_TIMEOUT, 1, 60, 20, "armed");
    run(hang_script, 6, "never.txt", 14, &loop, AMIGA_EMULATION_TIMEOUT, 0, 40, 13, "never armed");
}
