/*
 * test_bugorigcmp.c - the bug-orig tests in the terms of the shared test
 * model (tools/lib/bugorigcmp) with files from memory: the observation
 * files and their wildcard lines with a reason (broken ones too), the
 * matcher, the comparison with the recorded original, the projection of a
 * test onto cases (open, fixed, a long section, TED, no ID.orig, broken
 * headers) and fix-scope on the projections.
 */
#define _DEFAULT_SOURCE
#include "testutil.h"
#include "bugorigcmp.h"

static char err[700];

static int parse(const char *text, enum bugorigcmp_kind kind, struct bugorigcmp_obs *o)
{
    return bugorigcmp_parse(text, "t.orig", kind, o, err, sizeof err);
}

static void test_parse(void)
{
    struct bugorigcmp_obs o;

    CHECK(parse("# the original\n[session]\ndone\n[run]\nrc=0\n", BUGORIGCMP_ORIG, &o) == 0);
    CHECK(o.n == 2 && o.note && !strcmp(o.note, "the original"));
    CHECK(!strcmp(bugorigcmp_get(&o, "run")->text, "rc=0\n") && !bugorigcmp_get(&o, "run")->wild);
    bugorigcmp_free(&o);
    /* a last line without LF, an empty section */
    CHECK(parse("[run]\n[log]\nx", BUGORIGCMP_RUN, &o) == 0);
    CHECK(!strcmp(bugorigcmp_get(&o, "run")->text, "") && !strcmp(bugorigcmp_get(&o, "log")->text, "x\n"));
    bugorigcmp_free(&o);
    /* a wildcard line with its reason: the reason is no part of the text */
    CHECK(parse("[run]\na\n# varies: the address of a buffer\nptr=$*\nb\n", BUGORIGCMP_ORIG, &o) == 0);
    CHECK(!strcmp(bugorigcmp_get(&o, "run")->text, "a\nptr=$*\nb\n"));
    CHECK(!strcmp(bugorigcmp_get(&o, "run")->raw, "a\n# varies: the address of a buffer\nptr=$*\nb\n"));
    CHECK(bugorigcmp_get(&o, "run")->wild);
    bugorigcmp_free(&o);
    /* in a run or an expectation the lines are literal */
    CHECK(parse("[run]\n# varies: x\nptr=$*\n", BUGORIGCMP_RUN, &o) == 0);
    CHECK(!strcmp(bugorigcmp_get(&o, "run")->text, "# varies: x\nptr=$*\n"));
    bugorigcmp_free(&o);
    /* broken: a wildcard line without a reason (also ...), a reason
       without a wildcard line behind it, an empty reason, text before the
       first section, a section twice, no section */
    CHECK(parse("[run]\na\nptr=$*\n", BUGORIGCMP_ORIG, &o) == -2);
    CHECK(strstr(err, "t.orig:3: a wildcard line") != NULL);
    CHECK(parse("[run]\n...\n", BUGORIGCMP_ORIG, &o) == -2 && strstr(err, "t.orig:2:"));
    CHECK(parse("[run]\n# varies: x\na\n", BUGORIGCMP_ORIG, &o) == -2);
    CHECK(strstr(err, "t.orig:2: the \"# varies:\" line is not followed") != NULL);
    CHECK(parse("[run]\n# varies: x\n[log]\n", BUGORIGCMP_ORIG, &o) == -2);
    CHECK(parse("[run]\n# varies:  \nx*\n", BUGORIGCMP_ORIG, &o) == -2 && strstr(err, "without a reason"));
    CHECK(parse("x\n[run]\n", BUGORIGCMP_RUN, &o) == -2 && strstr(err, "t.orig:1: text before"));
    CHECK(parse("[run]\n[run]\n", BUGORIGCMP_RUN, &o) == -2 && strstr(err, "t.orig:2: section twice"));
    CHECK(parse("# only a note\n", BUGORIGCMP_RUN, &o) == -2 && strstr(err, "no section"));
    CHECK(o.n == 0 && !o.sec);
}

static void test_varies(void)
{
    char t[] = "# note\n# varies: no section yet\n[run]\na\n# varies: why\nb*\n[log]\nc\n";

    CHECK(bugorigcmp_check_varies(t, "x.orig", err, sizeof err) == 0);
    CHECK(bugorigcmp_check_varies("[run]\nb*\n", "x.orig", err, sizeof err) == -2);
    CHECK(strstr(err, "x.orig:2: a wildcard line") != NULL);
    CHECK(bugorigcmp_check_varies("[run]\n# varies: why\n", "x.orig", err, sizeof err) == -2);
    bugorigcmp_strip_varies(t);
    CHECK(!strcmp(t, "# note\n# varies: no section yet\n[run]\na\nb*\n[log]\nc\n"));
}

static void test_match(void)
{
    char w[120], g[120];

    CHECK(bugorigcmp_matches("a\nb*\n", "a\nbcd\n"));
    CHECK(!bugorigcmp_matches("a\nb*\n", "a\nx\n"));
    CHECK(bugorigcmp_matches("a\n...\nz\n", "a\nb\nc\nz\n"));
    CHECK(bugorigcmp_matches("...\n", ""));
    CHECK(!bugorigcmp_matches("a\n", "a\nb\n"));
    CHECK(bugorigcmp_first_diff("a\nb\nc\n", "a\nb\nc\n", w, sizeof w, g, sizeof g) == 0);
    CHECK(bugorigcmp_first_diff("a\nb\nc\n", "a\nx\"\x01\nc\n", w, sizeof w, g, sizeof g) == 2);
    CHECK(!strcmp(w, "b") && !strcmp(g, "x\\x22\\x01"));
    CHECK(bugorigcmp_first_diff("a\nb\n", "a\n", w, sizeof w, g, sizeof g) == 2 && !strcmp(g, "(the end)"));
    CHECK(bugorigcmp_first_diff("a\n", "a\nb\n", w, sizeof w, g, sizeof g) == 2 && !strcmp(w, "(the end)"));
}

static void test_compare(void)
{
    struct bugorigcmp_obs o, r;
    char msg[300];

    CHECK(parse("[session]\ndone\n[run]\nx\n# varies: a time\nt=*\n", BUGORIGCMP_ORIG, &o) == 0);
    CHECK(parse("[session]\ndone\n[run]\nx\nt=12:00\n", BUGORIGCMP_RUN, &r) == 0);
    CHECK(bugorigcmp_compare(&o, &r, msg, sizeof msg) == 0);
    bugorigcmp_free(&r);
    CHECK(parse("[session]\ndone\n[run]\ny\nt=12:00\n", BUGORIGCMP_RUN, &r) == 0);
    CHECK(bugorigcmp_compare(&o, &r, msg, sizeof msg) == 1);
    CHECK(!strcmp(msg, "[run] line 1: recorded \"x\", observed \"y\""));
    bugorigcmp_free(&r);
    CHECK(parse("[session]\ndone\n[run]\nx\nt=1\n[log]\n", BUGORIGCMP_RUN, &r) == 0);
    CHECK(bugorigcmp_compare(&o, &r, msg, sizeof msg) == 1);
    CHECK(!strcmp(msg, "[log]: observed, not in the recorded original"));
    bugorigcmp_free(&r);
    CHECK(parse("[run]\nx\nt=1\n", BUGORIGCMP_RUN, &r) == 0);
    CHECK(bugorigcmp_compare(&o, &r, msg, sizeof msg) == 1 && !strncmp(msg, "[session] line 1:", 17));
    bugorigcmp_free(&r);
    bugorigcmp_free(&o);
}

/* the case lines of one test (after the header) */
static char *project(const char *id, const char *src, int ted, const char *expect, const char *orig,
                     int *r)
{
    char *text = NULL;
    size_t len = 0;
    FILE *f = open_memstream(&text, &len);

    *r = bugorigcmp_project_test(f, id, src, ted, expect, orig, "area/x", err, sizeof err);
    fclose(f);
    return text;
}

static const char blitz_src[] = "; a test\n; @pass timeout=60\nPrint 1\n";
static const char fixed_src[] = "; a test\n; @fixed #7\nPrint 1\n";
static const char expect_txt[] = "[run]\n1\nrc=0\n";
static const char orig_txt[] = "# the original\n[session]\ndone\n[run]\n0\nrc=0\n";

static void test_project(void)
{
    char *t, big[400];
    int r;

    /* open test: the expectation and the original unqualified */
    t = project("x", blitz_src, 0, expect_txt, orig_txt, &r);
    CHECK(r == 0);
    CHECK(t && !strcmp(t, "x|run||1\t\"1\\x0Arc=0\\x0A\"\n"
                          "x|session|orig|1\t\"done\\x0A\"\n"
                          "x|run|orig|1\t\"0\\x0Arc=0\\x0A\"\n"));
    free(t);
    /* fixed test: the expectation [fixed #N], the original as a pair */
    t = project("x", fixed_src, 0, expect_txt, orig_txt, &r);
    CHECK(r == 0);
    CHECK(t && !strcmp(t, "x|run||1 [fixed #7]\t\"1\\x0Arc=0\\x0A\"\n"
                          "x|session|orig|1 [orig]\t\"done\\x0A\"\n"
                          "x|session|orig|1 [fixed #7]\tjudged by ID.expect\n"
                          "x|run|orig|1 [orig]\t\"0\\x0Arc=0\\x0A\"\n"
                          "x|run|orig|1 [fixed #7]\tjudged by ID.expect\n"));
    free(t);
    /* no ID.orig (a port regression test, or not recorded yet) */
    t = project("x", blitz_src, 0, expect_txt, NULL, &r);
    CHECK(r == 0 && t && !strcmp(t, "x|run||1\t\"1\\x0Arc=0\\x0A\"\n"));
    free(t);
    /* a long section: its length and hash; a backslash escaped */
    memset(big, 'a', sizeof big);
    memcpy(big, "[run]\n\\", 7);
    big[sizeof big - 2] = '\n';
    big[sizeof big - 1] = 0;
    t = project("x", blitz_src, 0, big, NULL, &r);
    CHECK(r == 0 && t && !strncmp(t, "x|run||1\tbytes 393 fnv ", 23) && strlen(t) == 23 + 8 + 1);
    free(t);
    t = project("x", blitz_src, 0, "[run]\n\\\n", NULL, &r);
    CHECK(r == 0 && t && !strcmp(t, "x|run||1\t\"\\x5C\\x0A\"\n"));
    free(t);
    /* TED: the script's @expect lines, the sections of its original */
    t = project("ted-x", "; a script\n; @fixed #12\n; @source none\n; @expect stdout \"rc=0\"\nwait exit\n",
                1, NULL, "# TED 1.1\n[state]\nOK\n[problems]\nstdout: rc=5\n", &r);
    CHECK(r == 0);
    CHECK(t && !strcmp(t, "ted-x|expect||1 [fixed #12]\t\"; @expect stdout \\x22rc=0\\x22\\x0A\"\n"
                          "ted-x|state|orig|1 [orig]\t\"OK\\x0A\"\n"
                          "ted-x|state|orig|1 [fixed #12]\tjudged by ID.expect\n"
                          "ted-x|problems|orig|1 [orig]\t\"stdout: rc=5\\x0A\"\n"
                          "ted-x|problems|orig|1 [fixed #12]\tjudged by ID.expect\n"));
    free(t);
    /* broken: no ID.expect, a bad @fixed, @fixed twice, a broken
       expectation, a wildcard of ID.orig without a reason */
    free(project("x", blitz_src, 0, NULL, NULL, &r));
    CHECK(r == -2 && strstr(err, "area/x.bas has no x.expect"));
    free(project("x", "; @fixed 7\n", 0, expect_txt, NULL, &r));
    CHECK(r == -2 && strstr(err, "area/x.bas:1: not \"; @fixed #N\""));
    free(project("x", "; @fixed #7\n; @fixed #8\n", 0, expect_txt, NULL, &r));
    CHECK(r == -2 && strstr(err, "area/x.bas:2: @fixed twice"));
    free(project("x", blitz_src, 0, "junk\n", NULL, &r));
    CHECK(r == -2 && strstr(err, "area/x.expect:1: text before"));
    free(project("x", blitz_src, 0, expect_txt, "[run]\nx*\n", &r));
    CHECK(r == -2 && strstr(err, "area/x.orig:2: a wildcard line"));
}

/* fix-scope on the projections of a base and a branch test */
static size_t scope(const char *base_src, const char *base_exp, const char *base_orig,
                    const char *br_src, const char *br_exp, const char *br_orig, uint32_t issue,
                    char **report)
{
    struct libscmp_file b, r;
    struct libscmp_issues is = { 0 };
    char *tb, *tr, *rep = NULL;
    size_t lb, lr, nrep = 99, nch, lrep;
    FILE *f;
    int x;

    memset(&b, 0, sizeof b);
    memset(&r, 0, sizeof r);
    f = open_memstream(&tb, &lb);
    fprintf(f, "# capture bug-orig 1\n");
    CHECK(bugorigcmp_project_test(f, "x", base_src, 0, base_exp, base_orig, "a/x", err, sizeof err) == 0);
    fclose(f);
    f = open_memstream(&tr, &lr);
    fprintf(f, "# capture bug-orig 1\n");
    CHECK(bugorigcmp_project_test(f, "x", br_src, 0, br_exp, br_orig, "a/x", err, sizeof err) == 0);
    fclose(f);
    x = libscmp_parse(&b, tb, lb, "base", LIBSCMP_EXPECTED, err, sizeof err);
    CHECK(x == 0);
    x |= libscmp_parse(&r, tr, lr, "branch", LIBSCMP_EXPECTED, err, sizeof err);
    CHECK(x == 0);
    if (issue)
        libscmp_issues_add(&is, issue);
    f = open_memstream(&rep, &lrep);
    if (!x)
        nrep = libscmp_fix_scope(&b, &r, &is, f, &nch);
    fclose(f);
    *report = rep;
    libscmp_free(&b);
    libscmp_free(&r);
    libscmp_issues_free(&is);
    free(tb);
    free(tr);
    return nrep;
}

static void test_scope(void)
{
    static const char exp2[] = "[run]\n1\nrc=5\n";
    char *rep;

    /* the fix: the header added, ID.expect changed: issue 7's cases */
    CHECK(scope(blitz_src, expect_txt, orig_txt, fixed_src, exp2, orig_txt, 7, &rep) == 0);
    free(rep);
    /* the same without the issue named: every case reported */
    CHECK(scope(blitz_src, expect_txt, orig_txt, fixed_src, exp2, orig_txt, 0, &rep) > 0);
    CHECK(strstr(rep, "bug-orig: x|run||1 [fixed #7]: added") != NULL);
    free(rep);
    /* a test fixed by another issue changed */
    CHECK(scope(fixed_src, expect_txt, orig_txt, fixed_src, exp2, orig_txt, 8, &rep) == 1);
    CHECK(strstr(rep, "x|run||1 [fixed #7]: changed") != NULL);
    free(rep);
    /* an open test's expectation changed */
    CHECK(scope(blitz_src, expect_txt, orig_txt, blitz_src, exp2, orig_txt, 7, &rep) == 1);
    CHECK(strstr(rep, "x|run||1: changed") != NULL);
    free(rep);
    /* a fixed test's original recorded again: the [orig] case of issue 7 */
    CHECK(scope(fixed_src, expect_txt, orig_txt, fixed_src, expect_txt,
                "[session]\ndone\n[run]\n2\nrc=0\n", 7, &rep) == 0);
    free(rep);
}

int main(void)
{
    test_parse();
    test_varies();
    test_match();
    test_compare();
    test_project();
    test_scope();
    if (failures)
        fprintf(stderr, "%d failures\n", failures);
    return failures != 0;
}
