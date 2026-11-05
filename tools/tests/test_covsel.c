/*
 * test_covsel.c - lib/covsel: the selection of tests for a change, one
 * case per rule of D8 (changed functions, a line outside them, a file
 * without line ranges, a header, the build, an unknown file, labels,
 * names, documentation, new tests, a new file, a stale map), the patterns,
 * the rules of tools/covsel/rules.txt, and broken inputs of each kind.
 */
#include "testutil.h"
#include "covsel.h"

#define ROOT "/src/bb2"

static char err[512];

/* the tracefiles of a coverage run (covmap's lcov/<test>.info): eval.c
   (functions with line ranges; unused executed by no test), stmts.c,
   parse.c (no line ranges: FN without a last line), host functions;
   files outside the root are left out */
#define EVAL(e, p) "SF:" ROOT "/programs/blitz2/src/eval.c\n" \
    "FN:10,50,eval5\nFN:60,70,pokewd\nFN:80,90,unused\n" \
    "FNDA:" e ",eval5\nFNDA:" p ",pokewd\nFNDA:0,unused\nend_of_record\n"
#define PARSE(a, b) "SF:" ROOT "/programs/blitz2/src/parse.c\n" \
    "FN:1,parse\nFN:1,parse2\nFNDA:" a ",parse\nFNDA:" b ",parse2\nend_of_record\n"

static const char *const traces[][2] = {
    { "ab3_cli", "TN:ab3_cli\n" EVAL("4", "1") },
    { "corpus", "TN:corpus\n" EVAL("1", "0")
      "SF:/usr/include/stdio.h\nFN:1,2,printf\nFNDA:1,printf\nend_of_record\n" },
    { "bug_orig_x", "TN:bug_orig_x\n" EVAL("0", "0")
      "SF:" ROOT "/programs/blitz2/src/stmts.c\nFN:5,100,doline\nFNDA:1,doline\n"
      "DA:5,1\nDA:6,0\nLF:2\nLH:1\nend_of_record\n" },
    { "corpus_req", "TN:corpus_req\n" PARSE("1", "0") },
    { "corpus_serial", "TN:corpus_serial\n" PARSE("0", "1") },
    /* no TN, a relative path, a comment, branches */
    { "host/ted_core_tokit", "# host\nSF:programs/ted/src/core/tokit.c\nFN:10,20,tokit\n"
      "FNDA:12345678901234567890,tokit\nBRDA:11,0,0,-\nBRDA:11,0,1,2\nBRF:2\nBRH:1\n"
      "end_of_record\n" },
    { "host/amiga_emulation_run", "TN:host_amiga_emulation_run\n"
      "SF:" ROOT "/tools/lib/amiga_emulation/run.c\nFN:1,9,run\nFNDA:1,run\nend_of_record\n" },
    /* a test that executed nothing measured: not named by the map */
    { "newtest", "TN:newtest\n" EVAL("0", "0") },
};

/* a selection with the map read */
static struct covsel *with_map(void)
{
    struct covsel *s = covsel_new(ROOT);
    size_t i;

    CHECK(s && !covsel_set_commit(s, "abc123\n", err, sizeof err));
    for (i = 0; s && i < sizeof traces / sizeof *traces; i++)
        CHECK(!covsel_read_lcov(s, traces[i][0], traces[i][1], err, sizeof err));
    return s;
}

#define TEST(name, labels) "{\"name\": \"" name "\", \"command\": [\"x\"], \"properties\": " \
    "[{\"name\": \"LABELS\", \"value\": [" labels "]}, {\"name\": \"TIMEOUT\", \"value\": 9}]}"

static const char tests[] =
    "{\"kind\": \"ctestInfo\", \"version\": {\"major\": 1, \"minor\": 0}, \"tests\": ["
    TEST("ab3_cli", "\"emulator\"") ","
    TEST("corpus", "\"emulator\"") ","
    TEST("corpus_req", "\"emulator\"") ","
    TEST("corpus_serial", "\"emulator\"") ","
    TEST("bug_orig_x", "\"emulator\", \"bug-orig\"") ","
    TEST("host", "") ","
    TEST("host_emulator", "\"emulator\"") ","
    "{\"name\": \"newtest\", \"properties\": []}]}";

static const char host_tests[] =
    "{\"kind\": \"ctestInfo\", \"tests\": ["
    TEST("ted_core_tokit", "\"ted\"") ","
    TEST("amiga_emulation_run", "\"emulator\"") "]}";

static const char rules[] =
    "# comment\n"
    "docs/**                 none\n"
    "**/CMakeLists.txt       all   # trailing comment\n"
    "programs/blitz2/src/glue*.c all\n"
    "programs/blitz2/src/*.c coverage\n"
    "programs/blitz2/src/*.h header\n"
    "programs/ted/src/**     coverage name:ted_ab*\n"
    "tests/corpus/**         name:corpus*\n"
    "libs/**                 label:emulator\n"
    "tools/tests/**          name:host/*\n";

static const char deps[] =
    "programs/blitz2/CMakeFiles/blitz2_core.dir/src/eval.c.obj: #deps 2, deps mtime 1 (VALID)\n"
    "    " ROOT "/programs/blitz2/src/eval.c\n"
    "    " ROOT "/programs/blitz2/src/blitz2.h\n"
    "\n"
    "programs/blitz2/CMakeFiles/blitz2_core.dir/src/stmts.c.obj: #deps 3, deps mtime 1 (VALID)\n"
    "    " ROOT "/programs/blitz2/src/stmts.c\n"
    "    " ROOT "/programs/blitz2/src/blitz2.h\n"
    "    " ROOT "/programs/blitz2/src/stmts.h\n"
    "\n"
    "programs/blitz2/CMakeFiles/blitz2_core.dir/src/parse.c.obj: #deps 2, deps mtime 1 (VALID)\n"
    "    " ROOT "/programs/blitz2/src/parse.c\n"
    "    " ROOT "/programs/blitz2/src/parse.h\n"
    "\n"
    "programs/blitz2/CMakeFiles/blitz2_core.dir/src/glue.c.obj: #deps 2, deps mtime 1 (VALID)\n"
    "    " ROOT "/programs/blitz2/src/glue.c\n"
    "    " ROOT "/programs/blitz2/src/glue.h\n"
    "\n"
    "tools/xtra/CMakeFiles/xtra.dir/main.c.o: #deps 2, deps mtime 1 (VALID)\n"
    "    " ROOT "/tools/xtra/main.c\n"
    "    " ROOT "/programs/blitz2/src/parse.h\n";

/* the selection for a diff (and new files, "stale"), as "a b c" */
static char *select_for(const char *rules_text, const char *diff, const char *newfile, int stale)
{
    static char out[4096];
    struct covsel *s = with_map();
    size_t i;

    out[0] = 0;
    if (!s)
        return out;
    CHECK(!covsel_read_rules(s, rules_text, err, sizeof err));
    CHECK(!covsel_read_tests(s, tests, 0, err, sizeof err));
    CHECK(!covsel_read_tests(s, host_tests, 1, err, sizeof err));
    CHECK(!covsel_read_deps(s, deps, err, sizeof err));
    CHECK(!covsel_read_diff(s, diff, err, sizeof err));
    if (newfile)
        CHECK(!covsel_add_new_file(s, newfile));
    if (stale)
        covsel_set_stale(s);
    CHECK(!covsel_run(s, NULL));
    for (i = 0; i < covsel_count(s); i++) {
        if (i)
            strcat(out, " ");
        strcat(out, covsel_name(s, i));
    }
    covsel_free(s);
    return out;
}

/* a diff of one file with one hunk header */
static char *diff1(const char *path, const char *hunk, const char *body)
{
    static char d[2048];

    snprintf(d, sizeof d,
             "diff --git a/%s b/%s\n"
             "index 1111111..2222222 100644\n"
             "--- a/%s\n"
             "+++ b/%s\n"
             "%s\n%s",
             path, path, path, path, hunk, body);
    return d;
}

#define ALL "ab3_cli bug_orig_x corpus corpus_req corpus_serial host host_emulator newtest"

static void test_select(void)
{
    char *r;

    /* always: newtest (not in the map) */
    CHECK(!strcmp(select_for(rules, "", NULL, 0), "newtest"));
    /* the body of one function: its tests only (both programs) */
    r = select_for(rules, diff1("programs/blitz2/src/eval.c", "@@ -61,2 +61,2 @@ x", "-a\n-b\n+c\n+d\n"), NULL, 0);
    CHECK(!strcmp(r, "ab3_cli newtest"));
    r = select_for(rules, diff1("programs/blitz2/src/eval.c", "@@ -12 +12 @@", "-a\n+b\n"), NULL, 0);
    CHECK(!strcmp(r, "ab3_cli corpus newtest"));
    /* a function no test executes: no test */
    r = select_for(rules, diff1("programs/blitz2/src/eval.c", "@@ -85 +85 @@", "-a\n+b\n"), NULL, 0);
    CHECK(!strcmp(r, "newtest"));
    /* a line outside the functions, an insertion at the end of one: all
       tests of the file */
    r = select_for(rules, diff1("programs/blitz2/src/eval.c", "@@ -55 +55 @@", "-a\n+b\n"), NULL, 0);
    CHECK(!strcmp(r, "ab3_cli corpus newtest"));
    r = select_for(rules, diff1("programs/blitz2/src/eval.c", "@@ -70,0 +71,2 @@", "+a\n+b\n"), NULL, 0);
    CHECK(!strcmp(r, "ab3_cli corpus newtest"));
    /* an insertion inside one: that function */
    r = select_for(rules, diff1("programs/blitz2/src/eval.c", "@@ -62,0 +63 @@", "+a\n"), NULL, 0);
    CHECK(!strcmp(r, "ab3_cli newtest"));
    /* a file without line ranges: all tests of its functions */
    r = select_for(rules, diff1("programs/blitz2/src/parse.c", "@@ -5 +5 @@", "-a\n+b\n"), NULL, 0);
    CHECK(!strcmp(r, "corpus_req corpus_serial newtest"));
    /* a deleted file, a binary change: the whole file */
    r = select_for(rules, "diff --git a/programs/blitz2/src/stmts.c b/programs/blitz2/src/stmts.c\n"
                          "deleted file mode 100644\n"
                          "--- a/programs/blitz2/src/stmts.c\n"
                          "+++ /dev/null\n"
                          "@@ -1,2 +0,0 @@\n-a\n-b\n", NULL, 0);
    CHECK(!strcmp(r, "bug_orig_x newtest"));
    r = select_for(rules, "diff --git a/programs/blitz2/src/stmts.c b/programs/blitz2/src/stmts.c\n"
                          "index 1..2 100644\n"
                          "Binary files a/programs/blitz2/src/stmts.c and b/programs/blitz2/src/stmts.c differ\n",
                   NULL, 0);
    CHECK(!strcmp(r, "bug_orig_x newtest"));
    /* a header: the tests of the objects that include it */
    r = select_for(rules, diff1("programs/blitz2/src/blitz2.h", "@@ -3 +3 @@", "-a\n+b\n"), NULL, 0);
    CHECK(!strcmp(r, "ab3_cli bug_orig_x corpus newtest"));
    r = select_for(rules, diff1("programs/blitz2/src/stmts.h", "@@ -3 +3 @@", "-a\n+b\n"), NULL, 0);
    CHECK(!strcmp(r, "bug_orig_x newtest"));
    /* ... a source whose rule selects all (not measured, no rule) includes
       it, none includes it: all */
    r = select_for(rules, diff1("programs/blitz2/src/parse.h", "@@ -3 +3 @@", "-a\n+b\n"), NULL, 0);
    CHECK(!strcmp(r, ALL));
    r = select_for(rules, diff1("programs/blitz2/src/glue.h", "@@ -3 +3 @@", "-a\n+b\n"), NULL, 0);
    CHECK(!strcmp(r, ALL));
    r = select_for(rules, diff1("programs/blitz2/src/unused.h", "@@ -3 +3 @@", "-a\n+b\n"), NULL, 0);
    CHECK(!strcmp(r, ALL));
    /* the build, a file no rule knows: all */
    r = select_for(rules, diff1("libs/acidlibs/CMakeLists.txt", "@@ -3 +3 @@", "-a\n+b\n"), NULL, 0);
    CHECK(!strcmp(r, ALL));
    r = select_for(rules, diff1("CMakeLists.txt", "@@ -3 +3 @@", "-a\n+b\n"), NULL, 0);
    CHECK(!strcmp(r, ALL));
    r = select_for(rules, diff1("tests/data/ted-tokens.txt", "@@ -3 +3 @@", "-a\n+b\n"), NULL, 0);
    CHECK(!strcmp(r, ALL));
    /* documentation: none */
    r = select_for(rules, diff1("docs/ab3/README.md", "@@ -3 +3 @@", "-a\n+b\n"), NULL, 0);
    CHECK(!strcmp(r, "newtest"));
    /* names, labels; host tests by their runner */
    r = select_for(rules, diff1("tests/corpus/x.bas", "@@ -3 +3 @@", "-a\n+b\n"), NULL, 0);
    CHECK(!strcmp(r, "corpus corpus_req corpus_serial newtest"));
    r = select_for(rules, diff1("libs/acidlibs/x.c", "@@ -3 +3 @@", "-a\n+b\n"), NULL, 0);
    CHECK(!strcmp(r, "ab3_cli bug_orig_x corpus corpus_req corpus_serial host_emulator newtest"));
    r = select_for(rules, diff1("tools/tests/test_x.c", "@@ -3 +3 @@", "-a\n+b\n"), NULL, 0);
    CHECK(!strcmp(r, "host host_emulator newtest"));
    /* a host function: its host test's runner; coverage and a name */
    r = select_for(rules, diff1("programs/ted/src/core/tokit.c", "@@ -15 +15 @@", "-a\n+b\n"), NULL, 0);
    CHECK(!strcmp(r, "host newtest"));
    /* a new file (untracked, or new in the diff) the map does not know: all */
    r = select_for(rules, "", "programs/blitz2/src/new.c", 0);
    CHECK(!strcmp(r, ALL));
    r = select_for(rules, "diff --git a/programs/blitz2/src/new.c b/programs/blitz2/src/new.c\n"
                          "new file mode 100644\n"
                          "--- /dev/null\n"
                          "+++ b/programs/blitz2/src/new.c\n"
                          "@@ -0,0 +1 @@\n+a\n", NULL, 0);
    CHECK(!strcmp(r, ALL));
    /* a rename: both paths, the old one whole */
    r = select_for(rules, "diff --git a/programs/blitz2/src/stmts.c b/docs/stmts.c\n"
                          "similarity index 100%\n"
                          "rename from programs/blitz2/src/stmts.c\n"
                          "rename to docs/stmts.c\n", NULL, 0);
    CHECK(!strcmp(r, "bug_orig_x newtest"));
    /* several files */
    r = select_for(rules, "diff --git a/programs/blitz2/src/parse.c b/programs/blitz2/src/parse.c\n"
                          "--- a/programs/blitz2/src/parse.c\n+++ b/programs/blitz2/src/parse.c\n"
                          "@@ -1 +1 @@\n--- a line that looks like a header\n+b\n"
                          "\\ No newline at end of file\n"
                          "diff --git a/programs/blitz2/src/stmts.c b/programs/blitz2/src/stmts.c\n"
                          "--- a/programs/blitz2/src/stmts.c\n+++ b/programs/blitz2/src/stmts.c\n"
                          "@@ -7 +7,2 @@\n-a\n+b\n+c\n", NULL, 0);
    CHECK(!strcmp(r, "bug_orig_x corpus_req corpus_serial newtest"));
    /* a stale map: all */
    CHECK(!strcmp(select_for(rules, "", NULL, 1), ALL));
}

static void test_match(void)
{
    CHECK(covsel_match("docs/**", "docs/a/b.md"));
    CHECK(!covsel_match("docs/**", "docsx/a"));
    CHECK(covsel_match("**/CMakeLists.txt", "CMakeLists.txt"));
    CHECK(covsel_match("**/CMakeLists.txt", "libs/acidlibs/CMakeLists.txt"));
    CHECK(!covsel_match("**/CMakeLists.txt", "libs/xCMakeLists.txt"));
    CHECK(covsel_match("programs/blitz2/src/*.c", "programs/blitz2/src/eval.c"));
    CHECK(!covsel_match("programs/blitz2/src/*.c", "programs/blitz2/src/sub/eval.c"));
    CHECK(covsel_match("*.md", "README.md") && !covsel_match("*.md", "docs/README.md"));
    CHECK(covsel_match("tests/*/runner/**", "tests/corpus/runner/main.c"));
    CHECK(covsel_match("glue?.c", "glue1.c") && !covsel_match("glue?.c", "glue.c"));
    CHECK(covsel_match("**.cmake", "a/b/c.cmake") && covsel_match("**/*.cmake", "c.cmake"));
}

/* tools/covsel/rules.txt: it parses, and chooses as intended */
static const char real_rules[] = {
#embed COVSEL_RULES
    , 0
};

static void test_real_rules(void)
{
    char *r;

    r = select_for(real_rules, diff1("programs/blitz2/src/eval.c", "@@ -61 +61 @@", "-a\n+b\n"), NULL, 0);
    CHECK(!strcmp(r, "ab3_cli newtest"));
    r = select_for(real_rules, diff1("programs/blitz2/src/glue.c", "@@ -61 +61 @@", "-a\n+b\n"), NULL, 0);
    CHECK(!strcmp(r, ALL));
    r = select_for(real_rules, diff1("programs/blitz2/src/stmts.h", "@@ -3 +3 @@", "-a\n+b\n"), NULL, 0);
    CHECK(!strcmp(r, "bug_orig_x newtest"));
    r = select_for(real_rules, diff1("tests/bug-orig/CMakeLists.txt", "@@ -3 +3 @@", "-a\n+b\n"), NULL, 0);
    CHECK(!strcmp(r, ALL));
    r = select_for(real_rules, diff1("tests/bug-orig/x/expect.txt", "@@ -3 +3 @@", "-a\n+b\n"), NULL, 0);
    CHECK(!strcmp(r, "bug_orig_x newtest"));
    r = select_for(real_rules, diff1("README.md", "@@ -3 +3 @@", "-a\n+b\n"), NULL, 0);
    CHECK(!strcmp(r, "newtest"));
    r = select_for(real_rules, diff1("programs/ted/src/core/tokit.c", "@@ -15 +15 @@", "-a\n+b\n"), NULL, 0);
    CHECK(!strcmp(r, ALL));
    r = select_for(real_rules, diff1("shared/xtra/xtra.c", "@@ -15 +15 @@", "-a\n+b\n"), NULL, 0);
    CHECK(!strcmp(r, ALL));
}

static void test_broken(void)
{
    struct covsel *s = covsel_new(ROOT);
    static const char *const bad_commits[] = { "", "\n", "abc 123\n", "HEAD\n", "abc123\nx\n" };
    static const char *const bad_traces[][2] = {
        { "FN:1,f\n", "line 1: \"FN:1,f\" outside a section" },
        { "SF:x.c\nend_of_record\nDA:1,1\n", "line 3: \"DA:1,1\" outside a section" },
        { "SF:x.c\nTN:t\nend_of_record\n", "TN: inside a section" },
        { "TN:other\nSF:x.c\nend_of_record\n", "TN:other is not this test" },
        { "SF:x.c\nSF:y.c\nend_of_record\n", "SF: inside a section" },
        { "SF:\nend_of_record\n", "SF: without a file" },
        { "SF:x.c\nFN:1,f\n", "no end_of_record" },
        { "SF:x.c\nFN:0,f\nend_of_record\n", "bad FN:" },
        { "SF:x.c\nFN:x,f\nend_of_record\n", "bad FN:" },
        { "SF:x.c\nFN:5,2,f\nend_of_record\n", "ends before it starts" },
        { "SF:x.c\nFN:5,\nend_of_record\n", "without a name" },
        { "SF:x.c\nFN:5,6,\nend_of_record\n", "without a name" },
        { "SF:x.c\nFN:1,f\nFNDA:1,g\nend_of_record\n", "line 3: FNDA: of a function without FN:" },
        { "SF:x.c\nFN:1,f\nFNDA:-1,f\nend_of_record\n", "bad FNDA:" },
        { "SF:x.c\nFN:1,f\nFNDA:1\nend_of_record\n", "bad FNDA:" },
        { "SF:x.c\nFN:1,f\nFNDA:1,\nend_of_record\n", "bad FNDA:" },
        { "SF:x.c\nXX:1\nend_of_record\n", "not an lcov record: XX:1" },
        { "SF:x.c\nend_of_records\n", "not an lcov record" },
        { "this is no tracefile\n", "outside a section" },
    };
    static const char *const bad_rules[][2] = {
        { "docs/** nothing\n", "unknown selector \"nothing\"" },
        { "docs/**\n", "without a selector" },
        { "docs/** none all\n", "\"none\" with other" },
        { "a all\nb label:\n", "rules line 2" },
    };
    static const char *const bad_diffs[][2] = {
        { "garbage\n", "not git diff output" },
        { "diff --git a/x b/x\n@@ -a +1 @@\n", "bad hunk header" },
        { "diff --git a/x b/x\n@@ -1 +1\n", "bad hunk header" },
        { "diff --git a/x b/x\n@@ -1,2 +1 @@\n-a\n context\n", "line 4: the hunk ends early" },
        { "diff --git a/x b/x\n@@ -1,2 +1 @@\n-a\n", "last hunk ends early" },
    };
    size_t i;

    CHECK(s);
    for (i = 0; i < sizeof bad_commits / sizeof *bad_commits; i++)
        CHECK(covsel_set_commit(s, bad_commits[i], err, sizeof err) && strstr(err, "no commit"));
    for (i = 0; i < sizeof bad_traces / sizeof *bad_traces; i++)
        CHECK(covsel_read_lcov(s, "t", bad_traces[i][0], err, sizeof err)
              && strstr(err, bad_traces[i][1]) && !strncmp(err, "t: ", 3));
    for (i = 0; i < sizeof bad_rules / sizeof *bad_rules; i++)
        CHECK(covsel_read_rules(s, bad_rules[i][0], err, sizeof err) && strstr(err, bad_rules[i][1]));
    for (i = 0; i < sizeof bad_diffs / sizeof *bad_diffs; i++)
        CHECK(covsel_read_diff(s, bad_diffs[i][0], err, sizeof err) && strstr(err, bad_diffs[i][1]));
    CHECK(covsel_read_tests(s, "{\"tests\": [", 0, err, sizeof err) && strstr(err, "ctest JSON"));
    CHECK(covsel_read_tests(s, "", 0, err, sizeof err) && strstr(err, "empty"));
    CHECK(covsel_read_tests(s, "{\"kind\": 1}", 0, err, sizeof err) && strstr(err, "no \"tests\""));
    CHECK(covsel_read_tests(s, "{\"tests\": [{}]}", 0, err, sizeof err) && strstr(err, "without \"name\""));
    CHECK(covsel_read_deps(s, "not deps\n", err, sizeof err));
    covsel_free(s);

    /* the map's commit; a test's name in TN: as it is and as lcov has it */
    s = with_map();
    CHECK(s && !strcmp(covsel_commit(s), "abc123"));
    CHECK(s && !covsel_read_lcov(s, "a-b/c.d", "TN:a_b_c_d\n", err, sizeof err));
    CHECK(s && !covsel_read_lcov(s, "a-b/c.d", "TN:a-b/c.d\n", err, sizeof err));
    CHECK(s && covsel_read_lcov(s, "a-b/c.d", "TN:a_b/c.d\n", err, sizeof err));
    covsel_free(s);
}

/* two tests whose names have the same lcov form (TN:x_a): the name comes
   from the tracefile's path, so a change selects only the test that ran it */
static void test_same_tn(void)
{
    static const char two[] =
        "{\"kind\": \"ctestInfo\", \"tests\": [" TEST("x-a", "") "," TEST("x_a", "") "]}";
    const char *diff = diff1("programs/blitz2/src/eval.c", "@@ -65 +65 @@", "-a\n+b\n");
    struct covsel *s = covsel_new(ROOT);

    CHECK(s && !covsel_set_commit(s, "abc123\n", err, sizeof err));
    CHECK(s && !covsel_read_lcov(s, "x-a", "TN:x_a\n" EVAL("1", "0"), err, sizeof err));
    CHECK(s && !covsel_read_lcov(s, "x_a", "TN:x_a\n" EVAL("0", "1"), err, sizeof err));
    CHECK(s && !covsel_read_rules(s, "** coverage\n", err, sizeof err));
    CHECK(s && !covsel_read_tests(s, two, 0, err, sizeof err));
    CHECK(s && !covsel_read_diff(s, diff, err, sizeof err));
    CHECK(s && !covsel_run(s, NULL));
    CHECK(s && covsel_count(s) == 1 && !strcmp(covsel_name(s, 0), "x_a"));
    covsel_free(s);
}

/* two tests whose names have the same lcov form (TN:x_a): their tracefiles'
   paths keep them apart, a change selects the one that ran the function */
static void test_tn_collision(void)
{
    static const char ctest[] = "{\"kind\": \"ctestInfo\", \"tests\": ["
                                TEST("x-a", "") "," TEST("x_a", "") "]}";
    struct covsel *s = covsel_new(ROOT);

    CHECK(s && !covsel_set_commit(s, "abc123\n", err, sizeof err));
    CHECK(s && !covsel_read_lcov(s, "x-a", "TN:x_a\n" EVAL("1", "0"), err, sizeof err));
    CHECK(s && !covsel_read_lcov(s, "x_a", "TN:x_a\n" EVAL("0", "1"), err, sizeof err));
    CHECK(s && !covsel_read_rules(s, "** coverage\n", err, sizeof err));
    CHECK(s && !covsel_read_tests(s, ctest, 0, err, sizeof err));
    CHECK(s && !covsel_read_diff(s, diff1("programs/blitz2/src/eval.c", "@@ -65 +65 @@", "-a\n+b\n"),
                                 err, sizeof err));
    CHECK(s && !covsel_run(s, NULL));
    CHECK(s && covsel_count(s) == 1 && !strcmp(covsel_name(s, 0), "x_a"));
    covsel_free(s);

    s = covsel_new(ROOT);
    CHECK(s && !covsel_set_commit(s, "abc123\n", err, sizeof err));
    CHECK(s && !covsel_read_lcov(s, "x-a", "TN:x_a\n" EVAL("1", "0"), err, sizeof err));
    CHECK(s && !covsel_read_lcov(s, "x_a", "TN:x_a\n" EVAL("0", "1"), err, sizeof err));
    CHECK(s && !covsel_read_rules(s, "** coverage\n", err, sizeof err));
    CHECK(s && !covsel_read_tests(s, ctest, 0, err, sizeof err));
    CHECK(s && !covsel_read_diff(s, diff1("programs/blitz2/src/eval.c", "@@ -12 +12 @@", "-a\n+b\n"),
                                 err, sizeof err));
    CHECK(s && !covsel_run(s, NULL));
    CHECK(s && covsel_count(s) == 1 && !strcmp(covsel_name(s, 0), "x-a"));
    covsel_free(s);
}

int main(void)
{
    test_match();
    test_select();
    test_real_rules();
    test_broken();
    test_tn_collision();
    test_same_tn();
    if (failures)
        fprintf(stderr, "%d failure(s)\n", failures);
    return failures != 0;
}
