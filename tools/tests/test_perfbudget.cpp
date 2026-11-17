// test_perfbudget - the benchmark's measurements (lib/perfbudget): the
// cycles per call with the empty loop subtracted, a frame test, and the
// malformed outputs that are refused.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <snitch/snitch.hpp>
extern "C" {
#include "perfbudget.h"
}

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

TEST_CASE("perfbudget: values", "[perfbudget]")
{
    struct perf_run r = {};
    const struct perf_meas *m;
    char err[300];

    /* empty: 2520 units for 20000 iterations = 126 cycles per iteration;
       peekb: 686 units for 2000 = 343, minus 126 = 217; nextitem: 51 calls
       per iteration; the lines as NPrint and the runner write them */
    CHECK(perf_read_out(&r, "bmath", "empty 2520 20000 1\r\npeekb 686 2000 1\n"
                        "nextitem 7000 500 51\nrc=0\n", err, sizeof err) == 0);
    CHECK(r.n == 3);
    m = find(&r, "bmath.empty");
    CHECK(m && m->empty && near(m->value, 126.0));
    m = find(&r, "bmath.peekb");
    CHECK(m && !m->empty && !m->frame && near(m->value, 217.0));
    m = find(&r, "bmath.nextitem");
    CHECK(m && near(m->value, (14000.0 - 126.0) / 51));
    /* a frame test: the loop of 126 cycles takes 128 while the handler
       runs, so the handler takes 2/128 of each frame */
    CHECK(perf_read_out(&r, "bmod", "empty 2520 20000 1\nvblmod 19200 150000 frame\nrc=0\n",
                        err, sizeof err) == 0);
    CHECK(r.n == 5);
    m = find(&r, "bmod.vblmod");
    CHECK(m && m->frame
          && near(m->value, PERF_FRAME_CYCLES * (2.0 / 128.0)));
    /* the same test name in two programs: two commands */
    CHECK(perf_read_out(&r, "bstr", "empty 2520 20000 1\npeekb 686 2000 2\nrc=0\n", err,
                        sizeof err) == 0);
    m = find(&r, "bstr.peekb");
    CHECK(m && near(m->value, 108.5));
    perf_run_free(&r);
    CHECK(r.n == 0 && !r.m);
}

/* the output is refused with a message naming the program and msg; nothing
   is added */
static int refused(const char *text, const char *msg)
{
    struct perf_run r = {};
    char err[300] = "";
    int ok = perf_read_out(&r, "prog", text, err, sizeof err) == -1 && r.n == 0
        && strstr(err, "prog") && strstr(err, msg);

    if (!ok)
        fprintf(stderr, "refused(%s): %s\n", msg, err);
    perf_run_free(&r);
    return ok;
}

TEST_CASE("perfbudget: malformed", "[perfbudget]")
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

/* the judge's lines for run against budgets, in buf; the number of failures */
static int judge(const struct perf_run *r, const struct perf_budgets *b, char *buf, size_t n)
{
    FILE *f = fmemopen(buf, n, "w");
    int k;

    REQUIRE(f != nullptr);
    buf[0] = 0;
    k = perf_judge(r, b, f);
    fclose(f);
    return k;
}

TEST_CASE("perfbudget: budgets", "[perfbudget]")
{
    struct perf_run r = {};
    struct perf_budgets b = {};
    char err[300], out[2000], table[4000];
    FILE *f;

    CHECK(perf_read_out(&r, "bmath", "empty 2520 20000 1\npeekb 686 2000 1\n"
                        "pokeb 650 2000 1\nrc=0\n", err, sizeof err) == 0);
    CHECK(perf_read_out(&r, "bmod", "empty 2520 20000 1\nvblmod 19200 150000 frame\nrc=0\n",
                        err, sizeof err) == 0);
    /* within the budgets (217.0 against 223, the frame test 2220.3 against
       2300); comments, blank lines, a target without decimals, none (-),
       the text with blanks and a trailing CR */
    CHECK(perf_read_budgets(&b, "# the budgets\n\n"
                            "bmath.peekb   223   129.0  c=Peek.b(p)\r\n"
                            "bmath.pokeb   204   111    Poke.b p,1\n"
                            "bmod.vblmod  2300   -      PlayModule playing\n", err,
                            sizeof err) == 0);
    CHECK(b.n == 3 && b.b[0].budget == 223 && b.b[0].has_target && b.b[0].target == 129.0
          && !strcmp(b.b[0].text, "c=Peek.b(p)") && !strcmp(b.b[1].text, "Poke.b p,1")
          && b.b[1].target == 111.0 && !b.b[2].has_target);
    CHECK(judge(&r, &b, out, sizeof out) == 0 && !out[0]);
    /* the table: the budget, the target and the factor, the text */
    REQUIRE((f = fmemopen(table, sizeof table, "w")) != NULL);
    perf_table(&r, &b, f);
    fclose(f);
    CHECK(strstr(table, "bmath.peekb") && strstr(table, "217.0") && strstr(table, "223")
          && strstr(table, "129.0") && strstr(table, "1.68") && strstr(table, "c=Peek.b(p)"));
    CHECK(strstr(table, "bmath.empty") && strstr(table, "(empty loop, per iteration)"));

    /* the broken budget of the spec: Peek.b measured 217 against 130 fails
       with the command and both values; the others pass */
    CHECK(perf_read_budgets(&b, "bmath.peekb 130 129.0 c=Peek.b(p)\n"
                            "bmath.pokeb 204 111.0 Poke.b p,1\n"
                            "bmod.vblmod 2300 - PlayModule playing\n", err, sizeof err) == 0);
    CHECK(judge(&r, &b, out, sizeof out) == 1
          && !strcmp(out, "bmath.peekb: measured 217.0 cycles per call, budget 130\n"));
    /* exactly the budget passes, a frame test over its budget names the frame */
    CHECK(perf_read_budgets(&b, "bmath.peekb 217 - x\nbmath.pokeb 204 - x\n"
                            "bmod.vblmod 2200 - x\n", err, sizeof err) == 0);
    CHECK(judge(&r, &b, out, sizeof out) == 1
          && !strcmp(out, "bmod.vblmod: measured 2220.3 cycles per frame, budget 2200\n"));
    /* a measured command without a budget, a budget without a measurement */
    CHECK(perf_read_budgets(&b, "bmath.peekb 223 - x\nbmod.vblmod 2300 - x\n"
                            "bmath.peekw 230 - x\n", err, sizeof err) == 0);
    CHECK(judge(&r, &b, out, sizeof out) == 2
          && strstr(out, "bmath.pokeb: measured 199.0 cycles per call, no budget\n")
          && strstr(out, "bmath.peekw: not measured (budget 230)\n"));
    perf_budgets_free(&b);
    perf_run_free(&r);
}

/* a marked loop (BUDGET "B~T"): B + T is the highest allowed value; an
   unmarked loop keeps its plain budget */
TEST_CASE("perfbudget: tolerance", "[perfbudget]")
{
    struct perf_run r = {};
    struct perf_budgets b = {};
    char err[300], out[2000], table[4000];
    FILE *f;

    CHECK(perf_read_out(&r, "bmath", "empty 2520 20000 1\npeekb 686 2000 1\n"
                        "pokeb 650 2000 1\nrc=0\n", err, sizeof err) == 0);
    /* peekb 217.0: within 200 + 18, over 200 + 16; the unmarked pokeb 199.0
       is over a plain 198 */
    CHECK(perf_read_budgets(&b, "bmath.peekb 200~18 - x\nbmath.pokeb 199 - y\n", err,
                            sizeof err) == 0);
    CHECK(b.n == 2 && b.b[0].budget == 200 && b.b[0].tolerance == 18 && b.b[1].tolerance == 0);
    CHECK(judge(&r, &b, out, sizeof out) == 0 && !out[0]);
    REQUIRE((f = fmemopen(table, sizeof table, "w")) != NULL);
    perf_table(&r, &b, f);
    fclose(f);
    CHECK(strstr(table, " 218 ") && strstr(table, "(varies, tolerance 18)"));
    CHECK(perf_read_budgets(&b, "bmath.peekb 200~16 - x\nbmath.pokeb 198 - y\n", err,
                            sizeof err) == 0);
    CHECK(judge(&r, &b, out, sizeof out) == 2
          && strstr(out, "bmath.peekb: measured 217.0 cycles per call, budget 200 + tolerance "
                    "16\n")
          && strstr(out, "bmath.pokeb: measured 199.0 cycles per call, budget 198\n"));
    perf_budgets_free(&b);
    perf_run_free(&r);
}

/* the budget file is refused with msg (and the line); b stays as it was */
static int budgets_refused(const char *text, const char *msg)
{
    struct perf_budgets b = {};
    char err[300] = "";
    int ok;

    REQUIRE(perf_read_budgets(&b, "x.y 1 - kept\n", err, sizeof err) == 0);
    ok = perf_read_budgets(&b, text, err, sizeof err) == -1 && strstr(err, msg)
        && b.n == 1 && !strcmp(b.b[0].name, "x.y");
    if (!ok)
        fprintf(stderr, "budgets_refused(%s): %s\n", msg, err);
    perf_budgets_free(&b);
    return ok;
}

TEST_CASE("perfbudget: budgets malformed", "[perfbudget]")
{
    CHECK(budgets_refused("bmath.peekb 223 129.0\n", "line 1: not \"COMMAND BUDGET TARGET TEXT\""));
    CHECK(budgets_refused("# c\npeekb 223 129.0 x\n", "line 2: the command \"peekb\" is not "
                          "PROGRAM.TEST"));
    CHECK(budgets_refused("bmath. 223 129.0 x\n", "not PROGRAM.TEST"));
    CHECK(budgets_refused(".peekb 223 129.0 x\n", "not PROGRAM.TEST"));
    CHECK(budgets_refused("bmath.empty 130 126.0 x\n", "the empty loop"));
    CHECK(budgets_refused("bmath.peekb 22.5 129.0 x\n", "the budget \"22.5\""));
    CHECK(budgets_refused("bmath.peekb -1 129.0 x\n", "the budget \"-1\""));
    CHECK(budgets_refused("bmath.peekb 223~ 129.0 x\n", "the tolerance in \"223~\""));
    CHECK(budgets_refused("bmath.peekb 223~0 129.0 x\n", "the tolerance in \"223~0\""));
    CHECK(budgets_refused("bmath.peekb 223~-5 129.0 x\n", "the tolerance in \"223~-5\""));
    CHECK(budgets_refused("bmath.peekb 223~5~6 129.0 x\n", "the tolerance in \"223~5~6\""));
    CHECK(budgets_refused("bmath.peekb ~5 129.0 x\n", "the budget \"~5\""));
    CHECK(budgets_refused("bmath.peekb 223 12x x\n", "the target \"12x\""));
    CHECK(budgets_refused("bmath.peekb 223 12. x\n", "the target \"12.\""));
    CHECK(budgets_refused("bmath.peekb 223 .5 x\n", "the target \".5\""));
    CHECK(budgets_refused("bmath.peekb 223 1.2.3 x\n", "the target \"1.2.3\""));
    CHECK(budgets_refused("bmath.peekb 223 - x\nbmath.peekb 224 - y\n",
                          "line 2: the command bmath.peekb twice"));
}
