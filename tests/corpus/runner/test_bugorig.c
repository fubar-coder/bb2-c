/*
 * test_bugorig.c - the verdict of the bug-orig tests (bugorig.h) with
 * inputs from memory: expectations valid and broken, observations from
 * the raw outputs, the four verdicts (an expectation that equals the
 * buggy output must come out BROKEN), and those of a port regression test
 * (an expectation the original does not show must come out BROKEN).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bugorig.h"

static int failures;
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "%s:%d: CHECK failed: %s\n", \
                                          __FILE__, __LINE__, #c); failures++; } } while (0)

static int parses(const char *text, char *err, size_t len)
{
    struct bugorig_obs o;
    int r = bugorig_parse(text, &o, err, len);

    if (!r)
        bugorig_free(&o);
    return r == 0;
}

/* the observation of a run; log/blitz2/run as given */
static void observe(struct bugorig_obs *o, const char *session, const char *log,
                    const char *blitz2, const char *run)
{
    struct bugorig_outputs out = { session, log, blitz2, run, "j000" };

    CHECK(bugorig_observe(&out, o) == 0);
}

static enum bugorig_verdict judge(const char *expect, const struct bugorig_obs *ref,
                                  const struct bugorig_obs *o)
{
    struct bugorig_obs e;
    char err[200];
    enum bugorig_verdict v;
    struct text why = { 0 };

    if (bugorig_parse(expect, &e, err, sizeof err)) {
        fprintf(stderr, "expectation: %s\n", err);
        failures++;
        return BUGORIG_BROKEN;
    }
    v = bugorig_judge(&e, ref, o, &why);
    CHECK(v == BUGORIG_FIXED || (why.p && *why.p));     /* every failure says why */
    free(why.p);
    bugorig_free(&e);
    return v;
}

/* the same for a port regression test (bugorig_judge_regression) */
static enum bugorig_verdict judge_regression(const char *expect, const struct bugorig_obs *ref,
                                             const struct bugorig_obs *o)
{
    struct bugorig_obs e;
    char err[200];
    enum bugorig_verdict v;
    struct text why = { 0 };

    if (bugorig_parse(expect, &e, err, sizeof err)) {
        fprintf(stderr, "expectation: %s\n", err);
        failures++;
        return BUGORIG_BROKEN;
    }
    v = bugorig_judge_regression(&e, ref, o, &why);
    CHECK(v == BUGORIG_FIXED || (why.p && *why.p));
    free(why.p);
    bugorig_free(&e);
    return v;
}

#define LOG_OK "BATCH\tstart\nJOB\tj000\tCORPUS:src/t.bb2\nRESULT\tj000\tok\t1234\nBATCH\tend\n"
#define LOG_ERR "JOB\tj000\tCORPUS:src/t.bb2\nRESULT\tj000\terror\t3\tSyntax Error\tx=\n"

int main(void)
{
    struct bugorig_obs a, b, c, d;
    char err[200];

    /* expectations */
    CHECK(parses("[run]\n42\nrc=0\n[compile]\nok\n", err, sizeof err));
    CHECK(parses("\n[session]\ntimeout\n", err, sizeof err));       /* empty lines before */
    CHECK(parses("[run]\n[X1]\n[a b]\n", err, sizeof err));          /* not headers: output */
    CHECK(!parses("", err, sizeof err) && strstr(err, "no section"));
    CHECK(!parses("42\n[run]\n", err, sizeof err) && strstr(err, "before"));
    CHECK(!parses("[output]\n42\n", err, sizeof err) && strstr(err, "unknown"));
    CHECK(!parses("[run]\n42\n[x1]\n", err, sizeof err) && strstr(err, "unknown"));
    CHECK(!parses("[run]\n1\n[run]\n2\n", err, sizeof err) && strstr(err, "twice"));
    {
        struct bugorig_obs e;

        CHECK(bugorig_parse("[run]\n42\nrc=0", &e, err, sizeof err) == 0);  /* no last LF */
        CHECK(e.n == 1 && !strcmp(bugorig_get(&e, "run"), "42\nrc=0\n"));
        bugorig_free(&e);
    }

    /* observations */
    observe(&a, "done", LOG_OK, "BATCH\tstart\nrc=0\n", "42\r\nrc=0\n");
    CHECK(!strcmp(bugorig_get(&a, "session"), "done\n"));
    CHECK(!strcmp(bugorig_get(&a, "compile"), "ok\n"));
    CHECK(!strcmp(bugorig_get(&a, "run"), "42\nrc=0\n"));
    CHECK(!strcmp(bugorig_get(&a, "blitz2"), "rc=0\n"));          /* the log lines left out */
    CHECK(strstr(bugorig_get(&a, "log"), "RESULT\tj000\tok\nBATCH"));  /* size removed */
    observe(&c, "done", "openreq \"Blitz Basic 2 Version 1.95\"\n"
            "getinput \"Blitz Basic 2 Version 2.10\" -> gadget 1 \" OKEE DOKEE \" (version requester)\n"
            "openreq \"Blitz Basic 2 Version x\n", NULL, NULL);  /* the version as N.NN */
    CHECK(!strcmp(bugorig_get(&c, "log"), "openreq \"Blitz Basic 2 Version N.NN\"\n"
                  "getinput \"Blitz Basic 2 Version N.NN\" -> gadget 1 \" OKEE DOKEE \" (version requester)\n"
                  "openreq \"Blitz Basic 2 Version x\n"));      /* no closing quote: unchanged */
    bugorig_free(&c);
    observe(&b, "timeout", LOG_ERR, NULL, NULL);
    CHECK(!strcmp(bugorig_get(&b, "compile"), "error 3 Syntax Error\n"));
    CHECK(!strcmp(bugorig_get(&b, "run"), "") && !strcmp(bugorig_get(&b, "blitz2"), ""));
    observe(&c, "done", "JOB\tj000\tx\n", NULL, NULL);
    CHECK(!strcmp(bugorig_get(&c, "compile"), "none\n"));
    bugorig_free(&c);
    observe(&c, "done", "RESULT\tj000\tfailed\tabort: extalloc failed\n", NULL, NULL);
    CHECK(!strcmp(bugorig_get(&c, "compile"), "failed abort: extalloc failed\n"));
    bugorig_free(&c);
    observe(&c, "done", "RESULT\tj0001\tok\t5\n", NULL, NULL);        /* another job */
    CHECK(!strcmp(bugorig_get(&c, "compile"), "none\n"));
    bugorig_free(&c);

    /* verdicts: a = the original (the bug: prints 42), c = a port that
       reproduces it, d = a fixed port (prints 41) */
    observe(&c, "done", LOG_OK, "rc=0\n", "42\nrc=0\n");
    observe(&d, "done", LOG_OK, "rc=0\n", "41\nrc=0\n");
    CHECK(judge("[run]\n41\nrc=0\n", &a, &c) == BUGORIG_REPRODUCED);
    CHECK(judge("[run]\n41\nrc=0\n", &a, &d) == BUGORIG_FIXED);
    CHECK(judge("[run]\n41\nrc=*\n", &a, &d) == BUGORIG_FIXED);      /* wildcard */
    CHECK(judge("[run]\n4*\nrc=0\n", &a, &c) == BUGORIG_BROKEN);     /* matches the bug too */
    /* broken on purpose: the expectation is the buggy output */
    CHECK(judge("[run]\n42\nrc=0\n", &a, &c) == BUGORIG_BROKEN);
    CHECK(judge("[run]\n42\nrc=0\n", &a, &d) == BUGORIG_BROKEN);
    /* the port neither correct nor as the original */
    CHECK(judge("[run]\n40\nrc=0\n", &a, &d) == BUGORIG_DIFFERS);
    /* only the sections of the expectation count: log differs, run the same */
    bugorig_free(&c);
    observe(&c, "done", LOG_OK "extra\n", "rc=0\n", "42\nrc=0\n");
    CHECK(judge("[run]\n41\nrc=0\n", &a, &c) == BUGORIG_REPRODUCED);
    CHECK(judge("[log]\nnothing\n", &a, &c) == BUGORIG_DIFFERS);
    /* [session] not given means done: a hang of the original is the bug */
    CHECK(judge("[compile]\nerror 3 Syntax Error\n", &b, &b) == BUGORIG_REPRODUCED);
    CHECK(judge("[session]\ntimeout\n[compile]\nerror 3 Syntax Error\n", &b, &b) == BUGORIG_BROKEN);
    CHECK(judge("[compile]\nerror 3 *\n", &b, &a) == BUGORIG_DIFFERS);
    /* a wildcard line needs a line: nothing observed does not match */
    CHECK(judge("[blitz2]\n*\n", &b, &a) == BUGORIG_FIXED);
    CHECK(judge("[blitz2]\nrc=0\n*\n", &b, &a) == BUGORIG_DIFFERS);
    /* "..." : any number of lines */
    CHECK(judge("[log]\n...\nRESULT\tj000\tok\n...\n", &b, &a) == BUGORIG_FIXED);
    CHECK(judge("[log]\n...\nBATCH\tend\n", &b, &a) == BUGORIG_FIXED);     /* the last line */
    CHECK(judge("[log]\nBATCH\tstart\n...\n", &b, &a) == BUGORIG_FIXED);   /* the first line */
    CHECK(judge("[log]\n...\nJOB*\n...\nBATCH\tstart\n...\n", &b, &a) == BUGORIG_DIFFERS);
    CHECK(judge("[session]\ntimeout\n[log]\n...\nRESULT\tj000\terror*\n...\n", &a, &b) == BUGORIG_FIXED);
    CHECK(judge("[session]\ntimeout\n[log]\n...\nRESULT\tj000\terror*\n...\n", &b, &a) == BUGORIG_BROKEN);
    /* a halt (the emulated CPU halted) is an observation of its own: a
       port that halts as the original reproduces it, one that hangs
       until the limit instead does not */
    bugorig_free(&c);
    bugorig_free(&d);
    observe(&c, "halt", LOG_ERR, NULL, NULL);
    observe(&d, "halt", LOG_ERR, NULL, NULL);
    CHECK(!strcmp(bugorig_get(&c, "session"), "halt\n"));
    CHECK(judge("[compile]\nerror 3 Syntax Error\n", &c, &d) == BUGORIG_REPRODUCED);
    CHECK(judge("[compile]\nerror 3 Syntax Error\n", &c, &b) == BUGORIG_DIFFERS);
    CHECK(judge("[session]\nhalt\n", &c, &d) == BUGORIG_BROKEN);
    CHECK(judge("[session]\nhalt\n", &b, &c) == BUGORIG_FIXED);
    CHECK(judge("[session]\nhalt\n", &b, &a) == BUGORIG_DIFFERS);
    bugorig_free(&c);
    bugorig_free(&d);
    observe(&c, "done", LOG_OK, "rc=0\n", "42\nrc=0\n");
    observe(&d, "done", LOG_OK, "rc=0\n", "41\nrc=0\n");
    /* port regression: the expectation is what the original does (a
       prints 42); c does the same, d not */
    CHECK(judge_regression("[run]\n42\nrc=0\n", &a, &c) == BUGORIG_FIXED);
    CHECK(judge_regression("[run]\n4*\nrc=0\n", &a, &d) == BUGORIG_FIXED);  /* wildcard */
    CHECK(judge_regression("[run]\n42\nrc=0\n", &a, &d) == BUGORIG_DIFFERS);
    /* broken on purpose: the expectation is not what the original does */
    CHECK(judge_regression("[run]\n41\nrc=0\n", &a, &d) == BUGORIG_BROKEN);
    CHECK(judge_regression("[session]\ntimeout\n", &a, &a) == BUGORIG_BROKEN);
    CHECK(!strcmp(bugorig_verdict_name(BUGORIG_BROKEN), "BROKEN"));
    bugorig_free(&a);
    bugorig_free(&b);
    bugorig_free(&c);
    bugorig_free(&d);
    return failures != 0;
}
