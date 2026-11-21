// test_prevcmp - the library files of two build directories made in a
// temporary directory: the same (passes), one file changed, missing or
// new, intended differences (all, a list), a stale and an unknown entry,
// files that are not compared, and a directory without library files.
#include <sys/stat.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <snitch/snitch.hpp>
extern "C" {
#include "prevcmp.h"
}

static char base[512];

static void put_file(const char *build, const char *rel, const char *text)
{
    char path[1024], dir[1024], *s;
    FILE *f;

    snprintf(path, sizeof path, "%s/%s/%s", base, build, rel);
    snprintf(dir, sizeof dir, "%s", path);
    for (s = dir + strlen(base) + 1; (s = strchr(s, '/')) != NULL; *s++ = '/') {
        *s = 0;
        mkdir(dir, 0777);
    }
    f = fopen(path, "wb");
    CHECK(f != NULL);
    if (f) {
        fputs(text, f);
        fclose(f);
    }
}

/* two builds with the same files */
static void make(void)
{
    static const char *const files[] = {
        "blitz2/acidlibs", "blitz2/deflibs", "libs/acidlibs/2dlib.obj",
        "libs/acidlibs/variants/rwe-2dlib.obj", "variants/rwe-1999/userlibs/rwe/rireqlib.obj",
        "libs/userlibs/rwe/CMakeFiles/x.dir/rireqlib.c.obj", "blitz2/blitz2",
        "libs/acidlibs/2dlib.linked",
    };
    static const char *const builds[] = { "cur", "prev" };
    size_t b, i;

    for (b = 0; b < 2; b++)
        for (i = 0; i < sizeof files / sizeof files[0]; i++)
            put_file(builds[b], files[i], files[i]);
}

static int run(const struct prevcmp_options *o, char *report, size_t n)
{
    char cur[600], prev[600], tmp[600], err[300];
    FILE *f;
    int r;
    size_t got;

    snprintf(cur, sizeof cur, "%s/cur", base);
    snprintf(prev, sizeof prev, "%s/prev", base);
    snprintf(tmp, sizeof tmp, "%s/report", base);
    f = fopen(tmp, "w+");
    REQUIRE(f != NULL);
    r = prevcmp(cur, prev, o, f, err, sizeof err);
    rewind(f);
    got = fread(report, 1, n - 1, f);
    report[got] = 0;
    if (r == 2)
        snprintf(report, n, "%s", err);
    fclose(f);
    return r;
}

static void test_same(void)
{
    struct prevcmp_options o = {};
    char rep[2000];

    /* the objects in CMakeFiles/, a .linked file and the compilers are not
       library files: 5 files compared */
    CHECK(run(&o, rep, sizeof rep) == 0);
    CHECK(strstr(rep, "5 library files, 0 differ, 0 not intended, 0 new"));
}

static void test_changed(void)
{
    struct prevcmp_options o = {};
    const char *list[] = { "libs/acidlibs/2dlib.obj" }, *two[] = { "libs/acidlibs/2dlib.obj",
                                                                   "blitz2/acidlibs" };
    char rep[2000];

    put_file("cur", "libs/acidlibs/2dlib.obj", "a different library");
    CHECK(run(&o, rep, sizeof rep) == 1);
    CHECK(strstr(rep, "DIFFERS libs/acidlibs/2dlib.obj\n"));
    CHECK(strstr(rep, "1 differ, 1 not intended"));
    /* the same size, one byte */
    put_file("cur", "libs/acidlibs/2dlib.obj", "libs/acidlibs/2dlib.obk");
    CHECK(run(&o, rep, sizeof rep) == 1);

    /* intended: all, or the file named */
    o.all = 1;
    CHECK(run(&o, rep, sizeof rep) == 0);
    CHECK(strstr(rep, "DIFFERS libs/acidlibs/2dlib.obj (intended)"));
    o.all = 0;
    o.intended = list;
    o.nintended = 1;
    CHECK(run(&o, rep, sizeof rep) == 0);
    CHECK(strstr(rep, "(intended)") && strstr(rep, "1 differ, 0 not intended"));

    /* an entry for a file that does not differ is stale */
    o.intended = two;
    o.nintended = 2;
    CHECK(run(&o, rep, sizeof rep) == 1);
    CHECK(strstr(rep, "STALE blitz2/acidlibs"));
    /* an entry for no library file at all */
    two[1] = "libs/acidlibs/nolib.obj";
    CHECK(run(&o, rep, sizeof rep) == 1);
    CHECK(strstr(rep, "STALE libs/acidlibs/nolib.obj"));

    put_file("cur", "libs/acidlibs/2dlib.obj", "libs/acidlibs/2dlib.obj");
}

static void test_missing_new(void)
{
    struct prevcmp_options o = {};
    char rep[2000], path[1024];

    /* a new library file is counted, not judged */
    put_file("cur", "variants/rwe-1999/userlibs/rwe/newlib.obj", "new");
    CHECK(run(&o, rep, sizeof rep) == 0);
    CHECK(strstr(rep, "0 not intended, 1 new"));
    snprintf(path, sizeof path, "%s/cur/variants/rwe-1999/userlibs/rwe/newlib.obj", base);
    unlink(path);

    /* a file of the previous build the current build lacks */
    snprintf(path, sizeof path, "%s/cur/blitz2/deflibs", base);
    CHECK(unlink(path) == 0);
    CHECK(run(&o, rep, sizeof rep) == 1);
    CHECK(strstr(rep, "MISSING blitz2/deflibs\n"));
    o.all = 1;
    CHECK(run(&o, rep, sizeof rep) == 0);
    CHECK(strstr(rep, "MISSING blitz2/deflibs (intended)"));
    put_file("cur", "blitz2/deflibs", "blitz2/deflibs");
}

static void test_no_library_files(void)
{
    struct prevcmp_options o = {};
    char rep[2000], dir[600];

    snprintf(dir, sizeof dir, "%s/prev", base);
    /* a prev directory with files that are no library files: an error, not a pass */
    put_file("empty", "blitz2/blitz2", "x");
    snprintf(dir, sizeof dir, "%s/empty", base);
    {
        char cur[600], err[300];
        FILE *f = tmpfile();
        int r;

        REQUIRE(f != NULL);
        snprintf(cur, sizeof cur, "%s/cur", base);
        r = prevcmp(cur, dir, &o, f, err, sizeof err);
        CHECK(r == 2 && strstr(err, "no library files"));
        fclose(f);
    }
    (void)rep;
}

TEST_CASE("prevcmp: build directories in a temporary directory", "[prevcmp]")
{
    const char *tmp = getenv("TMPDIR");

    snprintf(base, sizeof base, "%s/test_prevcmp-XXXXXX", tmp && *tmp ? tmp : "/tmp");
    if (!mkdtemp(base)) {
        perror("mkdtemp");
        FAIL("cannot create the temporary directory");
    }
    make();
    test_same();
    test_changed();
    test_missing_new();
    test_no_library_files();
    // the directory is kept when a check failed (for the diagnosis)
    if (!snitch::impl::get_current_test().failures) {
        char cmd[700];

        snprintf(cmd, sizeof cmd, "rm -rf '%s'", base);
        if (system(cmd))
            fprintf(stderr, "cannot remove %s\n", base);
    }
}

// the executables of two compiles of the same source: the parameter blocks of the traps hold
// addresses of the compiler's memory and are masked (prevcmp_mask_traps), anything else that
// differs, or another length, is a difference
TEST_CASE("prevcmp: executables, the trap blocks masked", "[prevcmp]")
{
    // code, trap #4 with its block, code, trap #1 with its block, code
    uint8_t a[] = { 0x60, 0x00, 0x4E, 0x44, 1, 2, 3, 4, 5, 6, 7, 8, 0x4E, 0x71,
                    0x4E, 0x41, 9, 9, 9, 9, 9, 9, 9, 9, 0x4E, 0x75 };
    uint8_t b[sizeof a];
    size_t first = 99;

    memcpy(b, a, sizeof a);
    CHECK(prevcmp_exe_same(a, sizeof a, b, sizeof b, &first));
    // another block behind the traps only: the same
    b[5] = 0xAA;
    b[17] = 0xBB;
    CHECK(prevcmp_exe_same(a, sizeof a, b, sizeof b, &first));
    // another code byte: the offset of the first difference
    b[13] = 0x70;
    CHECK(!prevcmp_exe_same(a, sizeof a, b, sizeof b, &first));
    CHECK(first == 13);
    // another length
    memcpy(b, a, sizeof a);
    CHECK(!prevcmp_exe_same(a, sizeof a, b, sizeof b - 1, &first));
    CHECK(first == sizeof a - 1);
    // the trap word itself is no block: a changed 4E41 differs
    b[sizeof b - 1] = a[sizeof a - 1];
    b[15] = 0x42;
    CHECK(!prevcmp_exe_same(a, sizeof a, b, sizeof b, &first));
    CHECK(first == 15);
    // masking in place: all blocks zero, the search goes on behind a zeroed block
    prevcmp_mask_traps(a, sizeof a);
    CHECK(a[4] == 0 && a[11] == 0 && a[16] == 0 && a[23] == 0 && a[24] == 0x4E);
    // empty executables are the same
    CHECK(prevcmp_exe_same(a, 0, b, 0, NULL));
}
