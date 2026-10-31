/*
 * test_statline.c - the @stat line (lib/teststats/statline.h): written,
 * read back, and broken lines refused.
 */
#include "testutil.h"
#include "statline.h"

static int parse(const char *line, struct statline *s, char *err, size_t errlen)
{
    return statline_parse(line, strlen(line), s, err, errlen);
}

int main(void)
{
    struct statline s, r;
    char buf[512], err[200];
    int n;

    /* written: the keys in order, two decimals, no verdict/runs if unset */
    statline_init(&s);
    statline_set(s.test, sizeof s.test, "bug_orig_ted_t01");
    statline_set(s.config, sizeof s.config, "C5");
    statline_set(s.machine, sizeof s.machine, "a1200");
    statline_set(s.snapshot, sizeof s.snapshot, "first-save");
    statline_set(s.end, sizeof s.end, "done");
    s.session = 12.345;
    s.wait = 0;
    s.timed = 3.5;
    s.limit = 120;
    n = statline_format(&s, buf, sizeof buf);
    CHECK(n > 0 && (size_t)n == strlen(buf));
    CHECK(!strcmp(buf, "@stat v=1 test=bug_orig_ted_t01 config=C5 machine=a1200 "
                       "snapshot=first-save session=12.35 wait=0.00 timed=3.50 limit=120 "
                       "end=done"));
    CHECK(parse(buf, &r, err, sizeof err) == 1);
    CHECK(!strcmp(r.test, "bug_orig_ted_t01") && !strcmp(r.config, "C5")
          && !strcmp(r.machine, "a1200") && !strcmp(r.snapshot, "first-save")
          && !strcmp(r.end, "done") && !*r.verdict && r.runs == -1 && r.limit == 120);
    CHECK(r.session > 12.34 && r.session < 12.36 && r.wait == 0 && r.timed == 3.5);

    /* verdict and runs; characters outside the set and empty values */
    statline_set(s.verdict, sizeof s.verdict, "REPRODUCED");
    statline_set(s.config, sizeof s.config, "a b=c");
    statline_set(s.machine, sizeof s.machine, "");
    s.runs = 4;
    CHECK(statline_format(&s, buf, sizeof buf) > 0);
    CHECK(strstr(buf, " config=a_b_c ") && strstr(buf, " machine=- ")
          && strstr(buf, " end=done verdict=REPRODUCED runs=4") && !strchr(buf, '\n'));
    CHECK(parse(buf, &r, err, sizeof err) == 1);
    CHECK(!strcmp(r.verdict, "REPRODUCED") && r.runs == 4 && !strcmp(r.config, "a_b_c"));
    CHECK(statline_format(&s, buf, 20) == -1);

    /* the reader: any order, unknown keys ignored, a newline at the end */
    CHECK(parse("@stat end=timeout limit=60 timed=60.1 wait=2 session=61 snapshot=cold "
                "machine=a500 config=ref test=corpus v=1 future=x\n", &r, err, sizeof err) == 1);
    CHECK(!strcmp(r.end, "timeout") && r.limit == 60 && r.wait == 2 && !strcmp(r.test, "corpus"));

    /* not a @stat line */
    CHECK(parse("corpus: ref: emulator session", &r, err, sizeof err) == 0);
    CHECK(parse("@status v=1", &r, err, sizeof err) == 0);
    CHECK(parse("", &r, err, sizeof err) == 0);
    CHECK(parse(" @stat v=1", &r, err, sizeof err) == 0);

    /* broken ones */
#define BAD(line, what)                                                        \
    do {                                                                       \
        err[0] = 0;                                                            \
        CHECK(parse(line, &r, err, sizeof err) == -1 && strstr(err, what));    \
    } while (0)
#define REST " test=t config=c machine=m snapshot=cold session=1 wait=0 timed=1 limit=9 end=done"
    BAD("@stat", "no v");
    BAD("@stat v=1 test=t", "no config");
    BAD("@stat v=2" REST, "unknown version");
    BAD("@stat v=1" REST " end=timeout", "end twice");
    BAD("@stat v=1" REST " verdict=", "empty value");
    BAD("@stat v=1" REST " verdict", "not KEY=VALUE");
    BAD("@stat v=1" REST " =x", "not KEY=VALUE");
    BAD("@stat v=1" REST "  runs=1", "not KEY=VALUE");
    BAD("@stat v=1" REST " ", "not KEY=VALUE");
    BAD("@stat v=1 test=t config=c machine=m snapshot=cold session=-1 wait=0 timed=1 limit=9 "
        "end=done", "session: not a number");
    BAD("@stat v=1 test=t config=c machine=m snapshot=cold session=1x wait=0 timed=1 limit=9 "
        "end=done", "session: not a number");
    BAD("@stat v=1 test=t config=c machine=m snapshot=cold session=1 wait=nan timed=1 limit=9 "
        "end=done", "wait: not a number");
    BAD("@stat v=1 test=t config=c machine=m snapshot=cold session=1 wait=0 timed=1 limit=9.5 "
        "end=done", "limit: not a whole");
    BAD("@stat v=1" REST " runs=99999999999", "runs: not a whole");
    BAD("@stat v=1 test=t\"x config=c", "bad character");
    BAD("@stat v=1 test=t\tx config=c", "bad character");
    {
        char longline[400];

        snprintf(longline, sizeof longline, "@stat v=1" REST " verdict=%040d", 0);
        BAD(longline, "verdict: value too long");
    }

    if (failures)
        fprintf(stderr, "test_statline: %d failures\n", failures);
    return failures != 0;
}
