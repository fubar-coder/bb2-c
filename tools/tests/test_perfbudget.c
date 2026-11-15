/*
 * test_perfbudget.c - the benchmark's measurements (lib/perfbudget): the
 * cycles per call with the empty loop subtracted, a frame test, and the
 * malformed outputs that are refused.
 */
#include <math.h>
#include "testutil.h"
#include "perfbudget.h"

static const struct perf_meas *find(const struct perf_run *r, const char *name)
{
    size_t i;

    for (i = 0; i < r->n; i++)
        if (!strcmp(r->m[i].name, name))
            return &r->m[i];
    return NULL;
}

static int near(double a, double b)
{
    return fabs(a - b) < 0.05;
}

static void test_values(void)
{
    struct perf_run r = { 0 };
    const struct perf_meas *m;
    char err[300];

    /* empty: 2520 units for 20000 iterations = 126 cycles per iteration;
       peekb: 686 units for 2000 = 343, minus 126 = 217; nextitem: 51 calls
       per iteration; the lines as NPrint and the runner write them */
    CHECK(perf_read_out(&r, "bmath", "empty 2520 20000 1\r\npeekb 686 2000 1\n"
                        "nextitem 7000 500 51\nrc=0\n", err, sizeof err) == 0);
    CHECK(r.n == 3);
    CHECK((m = find(&r, "bmath.empty")) && m->empty && near(m->value, 126.0));
    CHECK((m = find(&r, "bmath.peekb")) && !m->empty && !m->frame && near(m->value, 217.0));
    CHECK((m = find(&r, "bmath.nextitem")) && near(m->value, (14000.0 - 126.0) / 51));
    /* a frame test: the loop of 126 cycles takes 128 while the handler
       runs, so the handler takes 2/128 of each frame */
    CHECK(perf_read_out(&r, "bmod", "empty 2520 20000 1\nvblmod 19200 150000 frame\nrc=0\n",
                        err, sizeof err) == 0);
    CHECK(r.n == 5);
    CHECK((m = find(&r, "bmod.vblmod")) && m->frame
          && near(m->value, PERF_FRAME_CYCLES * (2.0 / 128.0)));
    /* the same test name in two programs: two commands */
    CHECK(perf_read_out(&r, "bstr", "empty 2520 20000 1\npeekb 686 2000 2\nrc=0\n", err,
                        sizeof err) == 0);
    CHECK((m = find(&r, "bstr.peekb")) && near(m->value, 108.5));
    perf_run_free(&r);
    CHECK(r.n == 0 && !r.m);
}

/* the output is refused with a message naming the program and msg; nothing
   is added */
static int refused(const char *text, const char *msg)
{
    struct perf_run r = { 0 };
    char err[300] = "";
    int ok = perf_read_out(&r, "prog", text, err, sizeof err) == -1 && r.n == 0
        && strstr(err, "prog") && strstr(err, msg);

    if (!ok)
        fprintf(stderr, "refused(%s): %s\n", msg, err);
    perf_run_free(&r);
    return ok;
}

static void test_malformed(void)
{
    CHECK(refused("empty 2520 20000 1\n", "no exit status"));
    CHECK(refused("empty 2520 20000 1\nrc=5\n", "rc=5"));
    CHECK(refused("empty 2520 20000 1\nrc=x\n", "malformed exit status"));
    CHECK(refused("peekb 686 2000 1\nrc=0\n", "no empty loop"));
    CHECK(refused("empty 2520 20000 1\npeekb 686 2000\nrc=0\n", "not \"TEST T N PER\""));
    CHECK(refused("empty 2520 20000 1\npeekb 686 2000 1 1\nrc=0\n", "not \"TEST T N PER\""));
    CHECK(refused("empty 2520 20000 1\n** Error\nrc=0\n", "line 2"));
    CHECK(refused("empty 2520 20000 1\npeekb 65536 2000 1\nrc=0\n", "0 to 65535"));
    CHECK(refused("empty 2520 20000 1\npeekb -1 2000 1\nrc=0\n", "0 to 65535"));
    CHECK(refused("empty 2520 20000 1\npeekb 686 0 1\nrc=0\n", "iterations"));
    CHECK(refused("empty 2520 20000 1\npeekb 686 2000 0\nrc=0\n", "calls per iteration"));
    CHECK(refused("empty 2520 20000 1\npeekb 686 2000 frames\nrc=0\n", "calls per iteration"));
    CHECK(refused("empty 2520 20000 frame\nrc=0\n", "one call per iteration"));
    CHECK(refused("empty 2520 20000 1\npeekb 686 2000 1\npeekb 686 2000 1\nrc=0\n", "twice"));
    CHECK(refused("empty 2520 20000 1\nrc=0\npeekb 686 2000 1\n", "after the exit status"));
}

int main(void)
{
    test_values();
    test_malformed();
    if (failures)
        fprintf(stderr, "%d failures\n", failures);
    return failures != 0;
}
