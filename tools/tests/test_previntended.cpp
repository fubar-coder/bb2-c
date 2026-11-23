// test_previntended - the PREV_INTENDED entries for the corpus and ted_ab
// (lib/previntended): listed and different, listed and the same (stale),
// not listed and different, ON, a pair, partial runs and malformed entries.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <snitch/snitch.hpp>
extern "C" {
#include "previntended.h"
}

static int add(struct prevint *pi, const char *e)
{
    char err[300];

    return prevint_add(pi, e, err, sizeof err);
}

static int add_err(const char *e, const char *msg)
{
    struct prevint pi = {};
    char err[300] = "";
    int r = prevint_add(&pi, e, err, sizeof err);

    prevint_free(&pi);
    return r == -1 && strstr(err, msg) && strstr(err, e);
}

TEST_CASE("previntended: judge", "[previntended]")
{
    struct prevint pi = {};
    size_t c = 0;

    CHECK(add(&pi, "corpus:alpha") == 0);
    CHECK(add(&pi, "corpus:beta") == 0);
    CHECK(add(&pi, "corpus_runerr:alpha") == 0);
    /* listed and different: intended */
    CHECK(prevint_judge(&pi, "corpus", "alpha", NULL, 1) == PREVINT_INTENDED);
    /* not listed and different: an error; not listed and the same: fine */
    CHECK(prevint_judge(&pi, "corpus", "gamma", NULL, 1) == PREVINT_DIFFERS);
    CHECK(prevint_judge(&pi, "corpus", "gamma", NULL, 0) == PREVINT_SAME);
    /* listed for another test only */
    CHECK(prevint_judge(&pi, "corpus_req", "alpha", NULL, 1) == PREVINT_DIFFERS);
    /* listed and the same: judged SAME, but stale */
    CHECK(prevint_judge(&pi, "corpus", "beta", NULL, 0) == PREVINT_SAME);
    CHECK(!strcmp(prevint_stale(&pi, "corpus", 0, &c), "corpus:beta"));
    CHECK(prevint_stale(&pi, "corpus", 0, &c) == NULL);
    /* corpus_runerr:alpha was never judged: stale (of its test only) */
    c = 0;
    CHECK(!strcmp(prevint_stale(&pi, "corpus_runerr", 0, &c), "corpus_runerr:alpha"));
    /* ... unless the run was partial */
    c = 0;
    CHECK(prevint_stale(&pi, "corpus_runerr", 1, &c) == NULL);
    prevint_free(&pi);
}

TEST_CASE("previntended: on", "[previntended]")
{
    struct prevint pi = {};
    size_t c = 0;

    CHECK(add(&pi, "ON") == 0);
    CHECK(prevint_judge(&pi, "corpus", "anything", NULL, 1) == PREVINT_INTENDED);
    CHECK(prevint_judge(&pi, "ted_ab", "x", "C5-P5", 1) == PREVINT_INTENDED);
    CHECK(prevint_judge(&pi, "corpus", "anything", NULL, 0) == PREVINT_SAME);
    CHECK(prevint_stale(&pi, "corpus", 0, &c) == NULL);
    prevint_free(&pi);
}

TEST_CASE("previntended: pair", "[previntended]")
{
    struct prevint pi = {};
    size_t c = 0;

    CHECK(add(&pi, "ted_ab:open") == 0);
    CHECK(add(&pi, "ted_ab:save:Cc-Pc") == 0);
    /* a case without pair: either pair may differ, one is enough */
    CHECK(prevint_judge(&pi, "ted_ab", "open", "C5-P5", 1) == PREVINT_INTENDED);
    CHECK(prevint_judge(&pi, "ted_ab", "open", "Cc-Pc", 0) == PREVINT_SAME);
    /* a case with a pair: only that one */
    CHECK(prevint_judge(&pi, "ted_ab", "save", "C5-P5", 1) == PREVINT_DIFFERS);
    CHECK(prevint_judge(&pi, "ted_ab", "save", "Cc-Pc", 0) == PREVINT_SAME);
    CHECK(!strcmp(prevint_stale(&pi, "ted_ab", 0, &c), "ted_ab:save:Cc-Pc"));
    CHECK(prevint_stale(&pi, "ted_ab", 0, &c) == NULL);
    prevint_free(&pi);
}

TEST_CASE("previntended: malformed", "[previntended]")
{
    struct prevint pi = {};

    /* a library path is not an entry of this kind */
    CHECK(add(&pi, "libs/acidlibs/2dlib.obj") == 1 && pi.n == 0);
    CHECK(add_err("corpuss:alpha", "unknown test"));
    CHECK(add_err(":alpha", "unknown test"));
    CHECK(add_err("corpus:", "name is empty"));
    CHECK(add_err("corpus:a b", "name is empty"));
    CHECK(add_err("corpus:a/b", "name is empty"));
    CHECK(add_err("compiler_prev:a/b", "name is empty"));
    CHECK(add_err("corpus_cd_prev:a b", "name is empty"));
    CHECK(add_err("corpus_cd_prev:A/B:x", "only ted_ab"));
    CHECK(add_err("prev:alpha", "unknown test"));
    CHECK(add_err("_prev:alpha", "unknown test"));
    CHECK(add_err("bug_orig_x_prev:x/a b", "name is empty"));
    CHECK(add_err("ab3_cli_prev:x/case", "name is empty"));
    /* the data source of a test of the model: ID/FILE (basic-test-local-data) */
    CHECK(add(&pi, "blitz2_x_prev:x/child.bas") == 0);
    CHECK(add(&pi, "bug_orig_x_prev:x/child.bas-runerrs") == 0);
    CHECK(add(&pi, "regression_x_prev:x/sub/b.bas") == 0);
    CHECK(prevint_judge(&pi, "blitz2_x_prev", "x/child.bas", NULL, 1) == PREVINT_INTENDED);
    prevint_free(&pi);
    CHECK(add(&pi, "corpus_cd_prev:BB2/DIR/X.BB2") == 0);
    CHECK(add(&pi, "corpus_cd_ab3_prev:BB2/DIR/X.BB2") == 0);
    CHECK(add(&pi, "compiler_prev:compiler-x") == 0);
    CHECK(add(&pi, "bug_orig_compiler_x_prev:compiler-x") == 0);
    CHECK(add(&pi, "site_examples_run:ex1") == 0);
    CHECK(add(&pi, "site_examples_compile_rwe_1999:ex2") == 0);
    CHECK(add(&pi, "ab3_cli_prev:case1") == 0);
    CHECK(add(&pi, "corpus_exprmatrix_dl_A_agree_01:dl-A-agree-01") == 0);
    CHECK(add(&pi, "corpus_exprmatrix_runerr_i1_I_c_agree_01:i1-I-c-agree-01") == 0);
    CHECK(add_err("corpus_exprmatrix:x", "unknown test"));
    CHECK(prevint_judge(&pi, "corpus_cd_prev", "BB2/DIR/X.BB2", NULL, 1) == PREVINT_INTENDED);
    CHECK(prevint_judge(&pi, "corpus_cd_prev", "BB2/DIR/Y.BB2", NULL, 1) == PREVINT_DIFFERS);
    CHECK(prevint_judge(&pi, "corpus_exprmatrix_dl_A_agree_01", "dl-A-agree-01", NULL, 1) == PREVINT_INTENDED);
    CHECK(pi.n == 9);
    prevint_free(&pi);
    CHECK(add_err("corpus:alpha:C5-P5", "only ted_ab"));
    CHECK(add_err("ted_ab:x:C5-C5", "the pair is"));
    CHECK(add_err("ted_ab:x:", "the pair is"));
    CHECK(add_err("ted_ab::C5-P5", "name is empty"));
    CHECK(add(&pi, "corpus:alpha") == 0);
    CHECK(add(&pi, "corpus:alpha") == -1);
    CHECK(pi.n == 1);
    prevint_free(&pi);
}

