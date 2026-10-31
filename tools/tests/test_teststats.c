/*
 * test_teststats.c - the JUnit reader (lib/teststats/junit.h) and the
 * statistics (teststats.h) with JUnit files and @stat lines in memory,
 * also malformed files, testcases without name or time and broken @stat
 * lines (reported, not counted).
 */
#define _POSIX_C_SOURCE 200809L /* open_memstream */
#include "testutil.h"
#include "junit.h"
#include "teststats.h"

#define HEAD "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" \
             "<testsuite name=\"(empty)\" tests=\"4\" timestamp=\"2026-10-31T10:00:00\">\n"
#define TAIL "</testsuite>\n"
#define STAT(cfg, snap, timed, end, verdict) \
    "@stat v=1 test=bug_a config=" cfg " machine=a1200 snapshot=" snap " session=" timed \
    " wait=1.50 timed=" timed " limit=120 end=" end verdict "\n"

/* run 1: bug_a (ref, port REPRODUCED), hang (every session by timeout),
   unit passed, skip skipped */
static const char run1[] =
    HEAD
    "\t<testcase name=\"bug_a\" classname=\"bug_a\" time=\"20.5\" status=\"run\">\n"
    "\t\t<properties><property name=\"LABELS\" value=\"bug-orig\"/></properties>\n"
    "\t\t<system-out>corpus: ref: emulator session\n"
    STAT("ref", "first-save", "5.00", "done", " verdict=reference")
    STAT("port", "restored", "8.00", "done", " verdict=REPRODUCED")
    "a &lt;tag&gt; &amp; &#65;&#x42;\n"
    "</system-out>\n\t</testcase>\n"
    "\t<testcase name=\"hang\" classname=\"hang\" time=\"250\" status=\"run\">\n"
    "\t\t<system-out><![CDATA["
    "@stat v=1 test=hang config=ref machine=a500 snapshot=cold session=121 wait=0 timed=120.2 "
    "limit=120 end=timeout verdict=reference\n"
    "@stat v=1 test=hang config=port machine=a500 snapshot=restored session=120.5 wait=3 "
    "timed=120.1 limit=120 end=timeout verdict=REPRODUCED\n"
    "]]></system-out>\n\t</testcase>\n"
    "\t<testcase name=\"unit\" classname=\"unit\" time=\"0.5\" status=\"run\">\n"
    "\t\t<system-out>ok</system-out>\n\t</testcase>\n"
    "\t<testcase name=\"skip\" classname=\"skip\" time=\"0.01\" status=\"notrun\">\n"
    "\t\t<skipped message=\"SKIP_RETURN_CODE=77\"/>\n"
    "\t\t<system-out>skipped</system-out>\n\t</testcase>\n"
    TAIL;

/* run 2: bug_a's port FIXED now (flaky), a broken @stat line; hang the
   same; unit failed (flaky); a second pass of bug_a's test name */
static const char run2[] =
    HEAD
    "<!-- a comment -->\n"
    "\t<testcase name=\"bug_a\" classname=\"bug_a\" time=\"22.5\" status=\"run\">\n"
    "\t\t<system-out>"
    STAT("ref", "restored", "4.00", "done", " verdict=reference")
    STAT("port", "fallback", "10.00", "done", " verdict=FIXED")
    "@stat v=1 test=bug_a config=clib\n"
    "@stat v=1 test=other config=x machine=a1200 snapshot=cold session=2 wait=0 timed=2 "
    "limit=60 end=done\n"
    "</system-out>\n\t</testcase>\n"
    "\t<testcase name=\"hang\" classname=\"hang\" time=\"260\" status=\"run\">\n"
    "\t\t<system-out>"
    "@stat v=1 test=hang config=ref machine=a500 snapshot=restored session=121 wait=0 "
    "timed=120.3 limit=120 end=timeout verdict=reference\n"
    "@stat v=1 test=hang config=port machine=a500 snapshot=restored session=121 wait=0 "
    "timed=120.4 limit=120 end=timeout verdict=REPRODUCED\n"
    "</system-out>\n\t</testcase>\n"
    "\t<testcase name=\"unit\" classname=\"unit\" time=\"0.7\" status=\"fail\">\n"
    "\t\t<failure message=\"Failed\"/>\n"
    "\t\t<system-out>bad</system-out>\n\t</testcase>\n"
    TAIL;

/* run 3: testcases without name and without time, a time that is no
   number; bug_a with a timeout of its port */
static const char run3[] =
    HEAD
    "\t<testcase classname=\"x\" time=\"1\" status=\"run\"/>\n"
    "\t<testcase name=\"notime\" status=\"run\"/>\n"
    "\t<testcase name=\"badtime\" time=\"soon\" status=\"run\"/>\n"
    "\t<testcase name=\"bug_a\" time=\"130\" status=\"fail\"><failure message=\"Failed\"/>\n"
    "<system-out>"
    STAT("ref", "restored", "6.00", "done", " verdict=reference")
    STAT("port", "restored", "120.00", "timeout", "")
    "</system-out></testcase>\n"
    TAIL;

static char *report(struct teststats *t, double factor, double floor, int tsv)
{
    struct teststats_options o = { factor, floor, tsv };
    char *p = NULL;
    size_t n = 0;
    FILE *f = open_memstream(&p, &n);

    CHECK(f && teststats_report(t, &o, f) == 0);
    if (f)
        fclose(f);
    return p;
}

static int add(struct teststats *t, const char *name, const char *text, char **diag)
{
    size_t n = 0;
    FILE *f;
    int r;

    *diag = NULL;
    f = open_memstream(diag, &n);
    r = teststats_add(t, name, text, strlen(text), f);
    fclose(f);
    return r;
}

/* the line of the report that starts with `start` and contains `has`;
   blanks in a row of the line count as one (the column widths do not
   matter) */
static int has_line(const char *text, const char *start, const char *has)
{
    const char *l = text;

    while (l && *l) {
        const char *e = strchr(l, '\n');
        size_t n = e ? (size_t)(e - l) : strlen(l), i, k = 0;
        char *line = malloc(n + 1);
        int ok;

        for (i = 0; line && i < n; i++)
            if (l[i] != ' ' || !k || line[k - 1] != ' ')
                line[k++] = l[i];
        if (line)
            line[k] = 0;
        ok = line && !strncmp(line, start, strlen(start)) && strstr(line, has);
        free(line);
        if (ok)
            return 1;
        l = e ? e + 1 : NULL;
    }
    return 0;
}

static void junit_reader(void)
{
    struct junit_file f;
    char err[200];

    CHECK(junit_parse(run1, strlen(run1), &f, err, sizeof err) == 0);
    CHECK(f.ncases == 4 && f.nproblems == 0 && !strcmp(f.timestamp, "2026-10-31T10:00:00"));
    if (f.ncases == 4) {
        CHECK(!strcmp(f.cases[0].name, "bug_a") && f.cases[0].time == 20.5
              && f.cases[0].status == JUNIT_PASSED);
        CHECK(strstr(f.cases[0].out, "a <tag> & AB\n"));
        CHECK(strstr(f.cases[1].out, "end=timeout verdict=REPRODUCED"));
        CHECK(f.cases[3].status == JUNIT_SKIPPED && !strcmp(f.cases[3].out, "skipped"));
    }
    junit_free(&f);
    CHECK(junit_parse(run3, strlen(run3), &f, err, sizeof err) == 0);
    CHECK(f.ncases == 1 && f.nproblems == 3 && f.cases[0].status == JUNIT_FAILED);
    CHECK(f.nproblems == 3 && strstr(f.problems[0], "line 3: a testcase without name")
          && strstr(f.problems[1], "notime: no time")
          && strstr(f.problems[2], "badtime: time is not a number"));
    junit_free(&f);

    /* malformed: refused, with the line */
    static const struct { const char *text, *msg; } bad[] = {
        { "", "no element" },
        { "just text", "text outside" },
        { "<testsuite>", "<testsuite> not closed at the end" },
        { "<testsuite><testcase name=\"a\" time=\"1\"></testsuite>", "does not close" },
        { "<testsuite></testcase>", "does not close" },
        { "</testsuite>", "without <testsuite>" },
        { "<testsuite/><testsuite/>", "a second root" },
        { "<testsuite name=\"a></testsuite>", "value of name not closed" },
        { "<testsuite name=a></testsuite>", "not quoted" },
        { "<testsuite name></testsuite>", "without value" },
        { "<testsuite a=\"1\" a=\"2\"></testsuite>", "twice" },
        { "<testsuite a=\"1\"b=\"2\"></testsuite>", "no blank" },
        { "<testsuite a=\"&nbsp;\"></testsuite>", "unknown entity" },
        { "<testsuite>&#xZZ;</testsuite>", "bad character reference" },
        { "<testsuite>&#0;</testsuite>", "out of range" },
        { "<testsuite>&#x110000;</testsuite>", "out of range" },
        { "<testsuite>a & b</testsuite>", "without entity" },
        { "<testsuite a=\"<\"></testsuite>", "'<'" },
        { "<testsuite><!-- open", "comment not closed" },
        { "<testsuite><![CDATA[ open", "CDATA not closed" },
        { "<?xml version=\"1.0\"", "<? not closed" },
        { "<!DOCTYPE x [ <!ENTITY a \"b\"> ]><x/>", "internal subset" },
        { "<testsuite", "not closed" },
        { "<>", "without a name" },
        { "<testsuite>\n\n</x>", "line 3:" },
        { "<testsuite/>trailing", "text outside" },
    };
    for (size_t i = 0; i < sizeof bad / sizeof bad[0]; i++) {
        err[0] = 0;
        int r = junit_parse(bad[i].text, strlen(bad[i].text), &f, err, sizeof err);

        if (r != -1 || !strstr(err, bad[i].msg)) {
            fprintf(stderr, "test_teststats: malformed %zu (%s): %d, %s\n", i, bad[i].text, r,
                    err);
            failures++;
        }
        CHECK(f.ncases == 0 && f.cases == NULL);
    }
    /* too deep */
    {
        char deep[1000] = "";

        for (int i = 0; i < 70; i++)
            strcat(deep, "<a>");
        CHECK(junit_parse(deep, strlen(deep), &f, err, sizeof err) == -1
              && strstr(err, "too deep"));
    }
    /* a BOM, a DOCTYPE, a <testsuites> root */
    {
        static const char ok[] = "\xEF\xBB\xBF<!DOCTYPE testsuites><testsuites><testsuite>"
                                 "<testcase name=\"x\" time=\"1e1\"/></testsuite></testsuites>";

        CHECK(junit_parse(ok, strlen(ok), &f, err, sizeof err) == 0 && f.ncases == 1
              && f.cases[0].time == 10 && f.cases[0].status == JUNIT_PASSED
              && !strcmp(f.cases[0].out, ""));
        junit_free(&f);
    }
}

int main(void)
{
    struct teststats *t = teststats_new();
    char *diag, *text;

    junit_reader();

    CHECK(t != NULL);
    CHECK(add(t, "run1.xml", run1, &diag) == 0 && diag && !*diag);
    free(diag);
    CHECK(add(t, "run2.xml", run2, &diag) == 1);
    CHECK(diag && strstr(diag, "run2.xml: testcase bug_a, output line 3: bad @stat line (no "
                         "machine): @stat v=1 test=bug_a config=clib"));
    free(diag);
    CHECK(add(t, "run3.xml", run3, &diag) == 3);
    CHECK(diag && strstr(diag, "run3.xml: line 3: a testcase without name - left out"));
    free(diag);
    CHECK(add(t, "bad.xml", "<testsuite><testcase>", &diag) == -1);
    CHECK(diag && strstr(diag, "bad.xml: line 1: <testcase> not closed at the end - the file "
                         "is left out"));
    free(diag);
    CHECK(teststats_runs(t) == 3);

    text = report(t, 4, 10, 0);
    CHECK(text != NULL);
    if (!text)
        return 1;
    /* tests: runs, results, durations */
    CHECK(has_line(text, "bug_a ", " 3 2 1 0 20.5 / 22.5 / 130.0"));
    CHECK(has_line(text, "unit ", " 2 1 1 0 0.5 / 0.6 / 0.7"));
    CHECK(has_line(text, "skip ", " 1 0 0 1 -"));
    /* sessions; a line of another pass (run 2): every config of the test
       with its pass */
    CHECK(has_line(text, "bug_a ", " bug_a:port 3 restored x2, fallback 120.0 120.0 1.5 1 120 "
                   "REPRODUCED, FIXED"));
    CHECK(has_line(text, "bug_a ", " bug_a:ref 3 first-save, restored x2 6.0 6.0 1.5 0 120 "
                   "reference x3"));
    CHECK(has_line(text, "bug_a ", " other:x 1 cold 2.0 2.0 0.0 0 60 -"));
    CHECK(has_line(text, "hang ", " ref 2 cold, restored 121.0 120.3 0.0 2 120 reference x2"));
    /* flaky */
    CHECK(has_line(text, "bug_a ", " bug_a:port verdict REPRODUCED, FIXED"));
    CHECK(has_line(text, "bug_a ", " bug_a:port end timeout in some of 3 runs"));
    CHECK(has_line(text, "bug_a ", " ctest result passed 2, failed 1"));
    CHECK(has_line(text, "unit ", " ctest result passed 1, failed 1"));
    CHECK(!has_line(text, "hang ", "verdict"));
    /* limits: max timed of the sessions that ended normally x 4, >= 10 */
    CHECK(has_line(text, "bug_a ", " bug_a:ref 6.0 120 24"));
    CHECK(has_line(text, "bug_a ", " bug_a:port 10.0 120 40 1 of 3 sessions ended by timeout"));
    CHECK(has_line(text, "bug_a ", " other:x 2.0 60 10"));
    CHECK(has_line(text, "bug_a ", " (test) 120 40 the largest of its configs"));
    CHECK(has_line(text, "hang ", " ref - 120 - hangs: every session ended by the time limit "
                   "(sessions 2, runs 2); no data on the last output - no suggestion"));
    CHECK(has_line(text, "hang ", " (test) 120 - every config hangs"));
    /* COST: the median, rounded up */
    CHECK(has_line(text, "bug_a ", " 3 22.5 23"));
    CHECK(has_line(text, "hang ", " 2 255.0 255"));
    CHECK(has_line(text, "unit ", " 2 0.6 1"));
    if (failures)
        fprintf(stderr, "%s", text);
    free(text);

    /* the floor */
    text = report(t, 1, 30, 0);
    CHECK(text && has_line(text, "bug_a ", " bug_a:ref 6.0 120 30"));
    free(text);

    /* TSV */
    text = report(t, 3, 30, 1);
    CHECK(text && has_line(text, "#tests\ttest\truns\tpassed", ""));
    CHECK(text && has_line(text, "limits\tbug_a\tbug_a:port\t10.0\t120\t30\t1 of 3", ""));
    CHECK(text && has_line(text, "cost\tunit\t2\t0.6\t1", ""));
    CHECK(text && !strstr(text, "=="));
    free(text);

    teststats_free(t);

    /* nothing added: empty tables */
    t = teststats_new();
    text = report(t, 3, 30, 0);
    CHECK(text && strstr(text, "0 runs (JUnit files), 0 tests") && strstr(text, "(none)"));
    free(text);
    teststats_free(t);

    if (failures)
        fprintf(stderr, "test_teststats: %d failures\n", failures);
    return failures != 0;
}
