/*
 * test_bugorig.c - the verdict of the bug-orig tests (bugorig.h) with
 * inputs from memory: expectations valid and broken, observations from
 * the raw outputs, the four verdicts (an expectation that equals the
 * buggy output must come out BROKEN), and those of a fixed test or an
 * expect test without a reference (FIXED or REGRESSED only; a wrong
 * expectation and the bug again come out REGRESSED).
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
    CHECK(!strcmp(bugorig_verdict_name(BUGORIG_BROKEN), "BROKEN"));
    /* a fixed test or an expect test (no reference): FIXED or REGRESSED,
       never a verdict that needs the original; c shows the bug (42), d
       the fix (41) */
    CHECK(judge("[run]\n41\nrc=0\n", NULL, &d) == BUGORIG_FIXED);
    CHECK(judge("[run]\n41\nrc=*\n", NULL, &d) == BUGORIG_FIXED);       /* wildcard */
    CHECK(judge("[run]\n41\nrc=0\n", NULL, &c) == BUGORIG_REGRESSED);   /* the bug again */
    CHECK(judge("[session]\ntimeout\n", NULL, &d) == BUGORIG_REGRESSED);
    CHECK(judge("[compile]\nerror 3 Syntax Error\n", NULL, &b) == BUGORIG_REGRESSED); /* hang */
    /* broken on purpose: a wrong expectation fails the fixed port too */
    CHECK(judge("[run]\n40\nrc=0\n", NULL, &d) == BUGORIG_REGRESSED);
    {
        static const char *const ex[] = { "[run]\n41\nrc=0\n", "[run]\n42\nrc=0\n",
                                           "[run]\n4*\nrc=0\n", "[log]\nnothing\n",
                                           "[session]\ntimeout\n", "[blitz2]\n*\n" };
        const struct bugorig_obs *obs[] = { &a, &b, &c, &d };
        size_t i, k;

        for (i = 0; i < sizeof ex / sizeof ex[0]; i++)
            for (k = 0; k < sizeof obs / sizeof obs[0]; k++) {
                enum bugorig_verdict v = judge(ex[i], NULL, obs[k]);

                CHECK(v == BUGORIG_FIXED || v == BUGORIG_REGRESSED);
            }
    }
    CHECK(!strcmp(bugorig_verdict_name(BUGORIG_REGRESSED), "REGRESSED"));

    /* the recorded original (NAME.orig): the observation written as a
       file and read back is the original for the verdicts, no reference
       session needed; a = the original (prints 42), c = a port that
       reproduces it, d = a fixed port (41) */
    {
        struct text f;
        struct bugorig_obs r;
        char msg[200];

        CHECK(bugorig_format(&a, "blitz2 1.95", &f, msg, sizeof msg) == 0);
        CHECK(!strncmp(f.p, "# blitz2 1.95\n[session]\ndone\n[compile]\nok\n[run]\n", 48));
        CHECK(bugorig_parse(f.p, &r, msg, sizeof msg) == 0 && r.n == 5);
        free(f.p);
        CHECK(judge("[run]\n41\nrc=0\n", &r, &c) == BUGORIG_REPRODUCED);
        CHECK(judge("[run]\n41\nrc=0\n", &r, &d) == BUGORIG_FIXED);
        CHECK(judge("[run]\n40\nrc=0\n", &r, &d) == BUGORIG_DIFFERS);
        CHECK(judge("[run]\n42\nrc=0\n", &r, &c) == BUGORIG_BROKEN);    /* the file equals the expectation */
        /* the file read back gives the verdicts of the observation itself */
        {
            static const char *const ex[] = { "[run]\n41\nrc=0\n", "[run]\n4*\nrc=0\n",
                                               "[run]\n40\nrc=0\n", "[compile]\nok\n",
                                               "[log]\n...\nBATCH\tend\n" };
            const struct bugorig_obs *obs[] = { &a, &b, &c, &d };
            size_t i, k;

            for (i = 0; i < sizeof ex / sizeof ex[0]; i++)
                for (k = 0; k < sizeof obs / sizeof obs[0]; k++)
                    CHECK(judge(ex[i], &r, obs[k]) == judge(ex[i], &a, obs[k]));
        }
        bugorig_free(&r);
        /* a line of the file edited to a wildcard (`*` at the end, `...`)
           matches as in an expectation: the port's 42 is the original's
           ("4*"), a 50 is not */
        CHECK(bugorig_parse("# edited\n[session]\ndone\n[run]\n4*\nrc=0\n", &r, msg, sizeof msg) == 0);
        CHECK(judge("[run]\n41\nrc=0\n", &r, &c) == BUGORIG_REPRODUCED);
        CHECK(judge("[run]\n41\nrc=0\n", &r, &d) == BUGORIG_FIXED);
        bugorig_free(&r);
        CHECK(bugorig_parse("[session]\ndone\n[run]\n...\nrc=0\n", &r, msg, sizeof msg) == 0);
        CHECK(judge("[run]\n41\nrc=0\n", &r, &c) == BUGORIG_REPRODUCED);
        bugorig_free(&r);
        CHECK(bugorig_parse("[session]\ndone\n[run]\n5*\nrc=0\n", &r, msg, sizeof msg) == 0);
        CHECK(judge("[run]\n41\nrc=0\n", &r, &c) == BUGORIG_DIFFERS);   /* 42 is not 5* */
        bugorig_free(&r);
        /* the first line is a comment only before the first section */
        CHECK(parses("# a note\n[run]\n# not a note\n", msg, sizeof msg));
        CHECK(!parses("[run]\n1\n[log]\n2\n#\nx\n[run]\n", msg, sizeof msg) && strstr(msg, "twice"));
        /* broken on purpose: an observation that would not read back
           (the program wrote a line that looks like a section header) */
        bugorig_free(&c);
        observe(&c, "done", LOG_OK, "rc=0\n", "[log]\nrc=0\n");
        CHECK(bugorig_format(&c, NULL, &f, msg, sizeof msg) == -1 && strstr(msg, "read back"));
        CHECK(f.p == NULL);
        CHECK(bugorig_format(&a, "two\nlines", &f, msg, sizeof msg) == -1 && strstr(msg, "line feed"));
        bugorig_free(&d);
        observe(&d, "done", LOG_OK, "rc=0\n", "[run]\n");
        CHECK(bugorig_format(&d, NULL, &f, msg, sizeof msg) == -1);
    }
    bugorig_free(&a);
    bugorig_free(&b);
    bugorig_free(&c);
    bugorig_free(&d);
    return failures != 0;
}
