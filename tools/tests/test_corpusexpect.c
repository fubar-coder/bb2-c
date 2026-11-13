/*
 * test_corpusexpect.c - the expected data of the corpus as cases
 * (lib/corpusexpect, tests/corpus/expect): the file names and the malformed
 * forms, which file applies to which configuration (design D2 of the
 * OpenSpec change corpus-test-model), an executable pair, the record from
 * two runs, the update from the port with and without FIX, the update of
 * the executables, the projection for fix-scope and a plan applied to a
 * directory.  The sets are built in memory.
 */
#define _DEFAULT_SOURCE
#include <dirent.h>
#include <unistd.h>
#include "testutil.h"
#include "corpusexpect.h"

#define H1 "1111111111111111111111111111111111111111111111111111111111111111\n"
#define H2 "2222222222222222222222222222222222222222222222222222222222222222\n"
#define H3 "3333333333333333333333333333333333333333333333333333333333333333\n"

static int add(struct cexp_set *s, const char *name, const char *data)
{
    char err[400];
    int r = cexp_add(s, name, data, strlen(data), err, sizeof err);

    if (r)
        fprintf(stderr, "cexp_add %s: %s\n", name, err);
    return r;
}

static void set_init(struct cexp_set *s, const char *pass)
{
    memset(s, 0, sizeof *s);
    s->pass = strdup(pass);
}

/* the file name is refused by cexp_add with msg in the message */
static int add_refused(const char *name, const char *data, const char *msg)
{
    struct cexp_set s;
    char err[400] = "";
    int r;

    set_init(&s, "corpus");
    r = cexp_add(&s, name, data, strlen(data), err, sizeof err);
    cexp_free(&s);
    if (r != -2 || !strstr(err, msg)) {
        fprintf(stderr, "%s: result %d, message \"%s\", wanted \"%s\"\n", name, r, err, msg);
        return 0;
    }
    return 1;
}

/* the two files make a set that cexp_check refuses with msg */
static int check_refused(const char *n1, const char *d1, const char *n2, const char *d2,
                         int strict, const char *msg)
{
    struct cexp_set s;
    char err[400] = "";
    int r;

    set_init(&s, "corpus");
    r = add(&s, n1, d1);
    if (!r && n2)
        r = cexp_add(&s, n2, d2, strlen(d2), err, sizeof err);
    if (!r)
        r = cexp_check(&s, strict, err, sizeof err);
    cexp_free(&s);
    if (r != -2 || !strstr(err, msg)) {
        fprintf(stderr, "%s, %s: result %d, message \"%s\", wanted \"%s\"\n", n1, n2 ? n2 : "-",
                r, err, msg);
        return 0;
    }
    return 1;
}

static void test_names(void)
{
    struct cexp_set s;
    const struct cexp_prog *p;
    char fn[160], err[400];

    set_init(&s, "corpus_runerr");
    CHECK(add(&s, "t124_bitmaperr.orig.out", "orig\n") == 0);
    CHECK(add(&s, "t124_bitmaperr.fixed-6.out", "fixed\n") == 0);
    CHECK(add(&s, "t01_int.out", "1\n") == 0);
    CHECK(add(&s, "t130_blit.orig.exe", H1) == 0);
    CHECK(add(&s, "t130_blit.fixed-46.exe", H2) == 0);
    CHECK(add(&s, "t130_blit.out", "same\n") == 0);
    CHECK(cexp_check(&s, 1, err, sizeof err) == 0);
    CHECK(s.n == 3 && !strcmp(s.p[0].name, "t01_int") && !strcmp(s.p[2].name, "t130_blit"));
    p = cexp_find(&s, "t124_bitmaperr");
    CHECK(p && p->issue[CEXP_OUT] == 6 && p->f[CEXP_OUT][CEXP_ORIG].len == 5);
    cexp_file_name(p, CEXP_OUT, CEXP_FIXED, fn, sizeof fn);
    CHECK(!strcmp(fn, "t124_bitmaperr.fixed-6.out"));
    cexp_file_name(p, CEXP_OUT, CEXP_ORIG, fn, sizeof fn);
    CHECK(!strcmp(fn, "t124_bitmaperr.orig.out"));
    p = cexp_find(&s, "t130_blit");
    CHECK(p && p->issue[CEXP_EXE] == 46 && cexp_has(p, CEXP_EXE) && !cexp_has(p, CEXP_SERIAL));
    CHECK(!cexp_find(&s, "t130"));
    cexp_free(&s);

    /* malformed file names */
    CHECK(add_refused("README", "x", "not NAME.OBS"));
    CHECK(add_refused("t01_int.txt", "x", "unknown observation .txt"));
    CHECK(add_refused("t01_int.new.out", "x", "unknown qualifier \"new\""));
    CHECK(add_refused("t01_int.fixed-0.out", "x", "unknown qualifier \"fixed-0\""));
    CHECK(add_refused("t01_int.fixed-6a.out", "x", "unknown qualifier \"fixed-6a\""));
    CHECK(add_refused("t01_int.fixed.out", "x", "unknown qualifier \"fixed\""));
    CHECK(add_refused(".out", "x", "the program's name"));
    CHECK(add_refused("t01 int.out", "x", "the program's name"));
    CHECK(add_refused("t01_int.exe", H1, "an executable is only a pair"));
    CHECK(add_refused("t01_int.orig.exe", "123\n", "not a SHA-256"));
    /* a run's set: plain files, NAME.exe the hash */
    {
        struct cexp_set r;

        set_init(&r, "corpus");
        r.run = 1;
        CHECK(add(&r, "t01_int.exe", H1) == 0);
        CHECK(cexp_add(&r, "t01_int.orig.out", "x", 1, err, sizeof err) == -2
              && strstr(err, "a run has no qualified files"));
        cexp_free(&r);
    }
    /* malformed sets */
    CHECK(check_refused("t01_int.fixed-6.out", "a", "t01_int.fixed-7.out", "b", 0,
                        "t01_int.fixed-6.out and t01_int.fixed-7.out: two fixed files"));
    CHECK(check_refused("t01_int.out", "a", "t01_int.orig.out", "b", 0,
                        "corpus/t01_int.out and t01_int.orig.out: an unqualified and a qualified"));
    CHECK(check_refused("t01_int.fixed-6.out", "a", "t01_int.out", "b", 0,
                        "t01_int.out and t01_int.fixed-6.out: an unqualified and a qualified"));
    CHECK(check_refused("t01_int.orig.out", "a", "t01_int.fixed-6.out", "a", 0,
                        "t01_int.orig.out and t01_int.fixed-6.out: a pair of equal files"));
    CHECK(check_refused("t01_int.orig.out", "a", NULL, NULL, 1,
                        "t01_int.orig.out: without its [fixed #N] file"));
    CHECK(check_refused("t01_int.fixed-6.exe", H1, NULL, NULL, 1,
                        "t01_int.fixed-6.exe: without its [orig] file"));
    /* not strict (record, update): a lone file of a pair is fine */
    {
        struct cexp_set t;

        set_init(&t, "corpus");
        CHECK(add(&t, "t01_int.fixed-6.out", "a") == 0);
        CHECK(cexp_check(&t, 0, err, sizeof err) == 0);
        cexp_free(&t);
    }
}

/* design D2: which file applies to which configuration */
static void test_sides(void)
{
    struct cexp_set s;
    const struct cexp_prog *plain, *pair, *exe;
    enum cexp_qual q[2], m;

    CHECK(cexp_side_of_tag("clib") == CEXP_SIDE_PORT);
    CHECK(cexp_side_of_tag("prev") == CEXP_SIDE_PORT);
    CHECK(cexp_side_of_tag("ref") == CEXP_SIDE_ORIG);
    CHECK(cexp_side_of_tag("port") == CEXP_SIDE_EITHER);
    CHECK(cexp_side_of_tag("orig-clib") == CEXP_SIDE_EITHER);
    set_init(&s, "corpus");
    CHECK(add(&s, "a.out", "same\n") == 0);
    CHECK(add(&s, "b.orig.out", "orig\n") == 0);
    CHECK(add(&s, "b.fixed-6.out", "fixed\n") == 0);
    CHECK(add(&s, "c.orig.exe", H1) == 0);
    CHECK(add(&s, "c.fixed-46.exe", H2) == 0);
    CHECK(add(&s, "c.out", "c\n") == 0);
    plain = cexp_find(&s, "a");
    pair = cexp_find(&s, "b");
    exe = cexp_find(&s, "c");
    /* unqualified: every configuration */
    CHECK(cexp_applies(plain, CEXP_OUT, CEXP_SIDE_PORT, q) == 1 && q[0] == CEXP_PLAIN);
    CHECK(cexp_applies(plain, CEXP_OUT, CEXP_SIDE_ORIG, q) == 1 && q[0] == CEXP_PLAIN);
    CHECK(cexp_applies(plain, CEXP_OUT, CEXP_SIDE_EITHER, q) == 1 && q[0] == CEXP_PLAIN);
    CHECK(cexp_judge(plain, CEXP_OUT, CEXP_SIDE_ORIG, (const uint8_t *)"same\n", 5, &m) == 0
          && m == CEXP_PLAIN);
    CHECK(cexp_judge(plain, CEXP_OUT, CEXP_SIDE_PORT, (const uint8_t *)"Same\n", 5, &m) == 1);
    CHECK(cexp_judge(plain, CEXP_SERIAL, CEXP_SIDE_PORT, (const uint8_t *)"x", 1, &m) == -1);
    /* clib, prev: [fixed #N] only */
    CHECK(cexp_applies(pair, CEXP_OUT, CEXP_SIDE_PORT, q) == 1 && q[0] == CEXP_FIXED);
    CHECK(cexp_judge(pair, CEXP_OUT, CEXP_SIDE_PORT, (const uint8_t *)"fixed\n", 6, &m) == 0
          && m == CEXP_FIXED);
    CHECK(cexp_judge(pair, CEXP_OUT, CEXP_SIDE_PORT, (const uint8_t *)"orig\n", 5, &m) == 1);
    /* ref: [orig] only */
    CHECK(cexp_applies(pair, CEXP_OUT, CEXP_SIDE_ORIG, q) == 1 && q[0] == CEXP_ORIG);
    CHECK(cexp_judge(pair, CEXP_OUT, CEXP_SIDE_ORIG, (const uint8_t *)"orig\n", 5, &m) == 0
          && m == CEXP_ORIG);
    CHECK(cexp_judge(pair, CEXP_OUT, CEXP_SIDE_ORIG, (const uint8_t *)"fixed\n", 6, &m) == 1);
    /* port, orig-clib: either, the report says which */
    CHECK(cexp_applies(pair, CEXP_OUT, CEXP_SIDE_EITHER, q) == 2 && q[0] == CEXP_FIXED
          && q[1] == CEXP_ORIG);
    CHECK(cexp_judge(pair, CEXP_OUT, CEXP_SIDE_EITHER, (const uint8_t *)"orig\n", 5, &m) == 0
          && m == CEXP_ORIG);
    CHECK(cexp_judge(pair, CEXP_OUT, CEXP_SIDE_EITHER, (const uint8_t *)"fixed\n", 6, &m) == 0
          && m == CEXP_FIXED);
    CHECK(cexp_judge(pair, CEXP_OUT, CEXP_SIDE_EITHER, (const uint8_t *)"other\n", 6, &m) == 1);
    CHECK(cexp_judge(pair, CEXP_OUT, CEXP_SIDE_EITHER, NULL, 0, &m) == 1);
    /* an executable pair: ref against [orig], port against [fixed #N] */
    CHECK(cexp_judge(exe, CEXP_EXE, CEXP_SIDE_ORIG, (const uint8_t *)H1, CEXP_HASHLEN, &m) == 0);
    CHECK(cexp_judge(exe, CEXP_EXE, CEXP_SIDE_PORT, (const uint8_t *)H2, CEXP_HASHLEN, &m) == 0);
    CHECK(cexp_judge(exe, CEXP_EXE, CEXP_SIDE_PORT, (const uint8_t *)H1, CEXP_HASHLEN, &m) == 1);
    CHECK(cexp_judge(plain, CEXP_EXE, CEXP_SIDE_PORT, (const uint8_t *)H1, CEXP_HASHLEN, &m) == -1);
    cexp_free(&s);
}

static void test_hash_diff(void)
{
    char h[CEXP_HASHLEN + 1], what[700];

    cexp_exe_hash((const uint8_t *)"abc", 3, h);
    CHECK(!strcmp(h, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad\n"));
    CHECK(cexp_diff((const uint8_t *)"a\nb\n", 4, (const uint8_t *)"a\nb\n", 4, what,
                    sizeof what) == 0);
    CHECK(cexp_diff((const uint8_t *)"a\nb\n", 4, (const uint8_t *)"a\nc\n", 4, what,
                    sizeof what) == 1 && !strcmp(what, "line 2: expected \"b\", got \"c\""));
    CHECK(cexp_diff((const uint8_t *)"a\n", 2, (const uint8_t *)"a\nx\n", 4, what,
                    sizeof what) == 1 && !strcmp(what, "line 2: expected the end, got \"x\""));
    CHECK(cexp_diff((const uint8_t *)"a\n", 2, NULL, 0, what, sizeof what) == 1
          && !strcmp(what, "no output"));
}

/* the plan has a write of name with data (NULL: a removal) */
static int planned(const struct cexp_plan *pl, const char *name, const char *data)
{
    size_t i;

    for (i = 0; i < pl->n; i++)
        if (!strcmp(pl->op[i].name, name)) {
            if (!data)
                return !pl->op[i].data;
            return pl->op[i].data && pl->op[i].len == strlen(data)
                   && !memcmp(pl->op[i].data, data, pl->op[i].len);
        }
    return 0;
}

/* a report into a buffer */
struct rep {
    char *p;
    size_t n;
    FILE *f;
};

static FILE *rep_open(struct rep *r)
{
    memset(r, 0, sizeof *r);
    return r->f = open_memstream(&r->p, &r->n);
}

static char *rep_text(struct rep *r)
{
    fclose(r->f);
    return r->p;
}

static void test_accept(void)
{
    struct cexp_set r1, r2, old;
    struct cexp_plan pl = { 0 };
    struct rep rep;
    char *t;
    const char *only[] = { "c" };

    set_init(&r1, "corpus");
    set_init(&r2, "corpus");
    r1.run = r2.run = 1;
    set_init(&old, "corpus");
    /* a: unqualified, the original changed; b: a pair, [orig] rewritten,
       [fixed #6] kept; c: new; d: exe pair; e: no exe pair */
    CHECK(add(&old, "a.out", "old\n") == 0);
    CHECK(add(&old, "b.orig.out", "orig\n") == 0);
    CHECK(add(&old, "b.fixed-6.out", "fixed\n") == 0);
    CHECK(add(&old, "d.out", "d\n") == 0);
    CHECK(add(&old, "d.fixed-46.exe", H2) == 0);
    CHECK(add(&old, "e.out", "e\n") == 0);
    {
        const char *progs[] = { "a", "b", "c", "d", "e" };
        const char *outs[] = { "new\n", "orig2\n", "c\n", "d\n", "e\n" };
        size_t i;

        for (i = 0; i < 5; i++) {
            char n[32];

            snprintf(n, sizeof n, "%s.out", progs[i]);
            CHECK(add(&r1, n, outs[i]) == 0 && add(&r2, n, outs[i]) == 0);
        }
    }
    /* the runs' executables (corpusrun pass --observe writes NAME.exe) */
    CHECK(add(&r1, "d.exe", H1) == 0 && add(&r2, "d.exe", H1) == 0);
    CHECK(add(&r1, "e.exe", H3) == 0 && add(&r2, "e.exe", H3) == 0);
    rep_open(&rep);
    CHECK(cexp_accept(&r1, &r2, &old, NULL, 0, &pl, rep.f) == 0);
    t = rep_text(&rep);
    CHECK(!*t);
    free(t);
    CHECK(planned(&pl, "a.out", "new\n"));
    CHECK(planned(&pl, "b.orig.out", "orig2\n") && !planned(&pl, "b.fixed-6.out", "fixed\n"));
    CHECK(planned(&pl, "c.out", "c\n"));
    CHECK(planned(&pl, "d.orig.exe", H1) && planned(&pl, "d.out", "d\n"));
    CHECK(!planned(&pl, "e.orig.exe", H3) && !planned(&pl, "e.exe", H3));
    CHECK(pl.n == 6);
    cexp_plan_free(&pl);
    /* only: just c */
    rep_open(&rep);
    CHECK(cexp_accept(&r1, &r2, &old, only, 1, &pl, rep.f) == 0);
    free(rep_text(&rep));
    CHECK(pl.n == 1 && planned(&pl, "c.out", "c\n"));
    cexp_plan_free(&pl);
    /* one differing line: refused, nothing planned for it, the others go on */
    {
        struct cexp_prog *pa = (struct cexp_prog *)cexp_find(&r2, "a");

        free(pa->f[CEXP_OUT][CEXP_PLAIN].data);
        pa->f[CEXP_OUT][CEXP_PLAIN].data = (uint8_t *)strdup("neu\n");
        pa->f[CEXP_OUT][CEXP_PLAIN].len = 4;
    }
    rep_open(&rep);
    CHECK(cexp_accept(&r1, &r2, &old, NULL, 0, &pl, rep.f) == 1);
    t = rep_text(&rep);
    CHECK(strstr(t, "test not deterministic: corpus:a\n")
          && strstr(t, "corpus:a: a.out line 1: expected \"new\", got \"neu\""));
    free(t);
    CHECK(!planned(&pl, "a.out", "new\n") && planned(&pl, "c.out", "c\n") && pl.n == 5);
    cexp_plan_free(&pl);
    /* the original gives the [fixed #N] value: the pair would be equal */
    {
        struct cexp_prog *pb = (struct cexp_prog *)cexp_find(&r1, "b");
        struct cexp_prog *pb2 = (struct cexp_prog *)cexp_find(&r2, "b");

        free(pb->f[CEXP_OUT][CEXP_PLAIN].data);
        free(pb2->f[CEXP_OUT][CEXP_PLAIN].data);
        pb->f[CEXP_OUT][CEXP_PLAIN].data = (uint8_t *)strdup("fixed\n");
        pb2->f[CEXP_OUT][CEXP_PLAIN].data = (uint8_t *)strdup("fixed\n");
        pb->f[CEXP_OUT][CEXP_PLAIN].len = pb2->f[CEXP_OUT][CEXP_PLAIN].len = 6;
    }
    rep_open(&rep);
    CHECK(cexp_accept(&r1, &r2, &old, only, 0, &pl, rep.f) == 1);
    t = rep_text(&rep);
    CHECK(strstr(t, "corpus:b: the original gives b.fixed-6.out (the pair would be equal"));
    free(t);
    CHECK(!planned(&pl, "b.orig.out", "fixed\n"));
    cexp_plan_free(&pl);
    /* a program only one run has */
    CHECK(add(&r2, "z.out", "z\n") == 0);
    rep_open(&rep);
    CHECK(cexp_accept(&r1, &r2, &old, NULL, 0, &pl, rep.f) == 1);
    t = rep_text(&rep);
    CHECK(strstr(t, "corpus:z: not in run 1"));
    free(t);
    cexp_plan_free(&pl);
    cexp_free(&r1);
    cexp_free(&r2);
    cexp_free(&old);
}

static void test_update(void)
{
    struct cexp_set run, old;
    struct cexp_plan pl = { 0 };
    struct cexp_fix fix[2];
    struct rep rep;
    char err[300], *t;

    CHECK(cexp_parse_fix("corpus_runerr:t124_bitmaperr#6", &fix[0], err, sizeof err) == 0
          && !strcmp(fix[0].pass, "corpus_runerr") && !strcmp(fix[0].name, "t124_bitmaperr")
          && fix[0].issue == 6);
    CHECK(cexp_parse_fix("corpus:t01_int", &fix[1], err, sizeof err) == -1
          && strstr(err, "not PASS:NAME#N"));
    CHECK(cexp_parse_fix("t01_int#6", &fix[1], err, sizeof err) == -1);
    CHECK(cexp_parse_fix("corpus:t01_int#0", &fix[1], err, sizeof err) == -1);
    CHECK(cexp_parse_fix("corpus:t01_int#6x", &fix[1], err, sizeof err) == -1);

    set_init(&run, "corpus_runerr");
    set_init(&old, "corpus_runerr");
    CHECK(add(&old, "a.out", "a\n") == 0);
    CHECK(add(&old, "t124_bitmaperr.out", "old\n") == 0);
    CHECK(add(&old, "p.orig.out", "orig\n") == 0);
    CHECK(add(&old, "p.fixed-7.out", "fixed\n") == 0);
    CHECK(add(&run, "a.out", "a\n") == 0);
    CHECK(add(&run, "t124_bitmaperr.out", "new\n") == 0);
    CHECK(add(&run, "p.out", "fixed2\n") == 0);
    CHECK(add(&run, "n.out", "n\n") == 0);
    /* without FIX: the changed unqualified file is refused, the rest done */
    rep_open(&rep);
    CHECK(cexp_update(&run, &old, 0, NULL, 0, &pl, rep.f) == 1);
    t = rep_text(&rep);
    CHECK(strstr(t, "corpus_runerr:t124_bitmaperr: t124_bitmaperr.out changed, line 1: expected "
                    "\"old\", got \"new\": name the fix with FIX=corpus_runerr:t124_bitmaperr#N"));
    free(t);
    CHECK(!planned(&pl, "t124_bitmaperr.out", "new\n") && !planned(&pl, "a.out", "a\n"));
    CHECK(planned(&pl, "p.fixed-7.out", "fixed2\n") && planned(&pl, "n.out", "n\n") && pl.n == 2);
    cexp_plan_free(&pl);
    /* with FIX: the pair */
    rep_open(&rep);
    CHECK(cexp_update(&run, &old, 0, fix, 1, &pl, rep.f) == 0);
    free(rep_text(&rep));
    CHECK(planned(&pl, "t124_bitmaperr.orig.out", "old\n")
          && planned(&pl, "t124_bitmaperr.fixed-6.out", "new\n")
          && planned(&pl, "t124_bitmaperr.out", NULL));
    cexp_plan_free(&pl);
    /* a FIX of another issue than the program's pair */
    CHECK(cexp_parse_fix("corpus_runerr:p#8", &fix[1], err, sizeof err) == 0);
    rep_open(&rep);
    CHECK(cexp_update(&run, &old, 0, fix + 1, 1, &pl, rep.f) == 1);
    t = rep_text(&rep);
    CHECK(strstr(t, "corpus_runerr:p: FIX names #8, but the program has p.fixed-7.out"));
    free(t);
    cexp_plan_free(&pl);
    /* the port gives the [orig] value again */
    {
        struct cexp_prog *pp = (struct cexp_prog *)cexp_find(&run, "p");

        free(pp->f[CEXP_OUT][CEXP_PLAIN].data);
        pp->f[CEXP_OUT][CEXP_PLAIN].data = (uint8_t *)strdup("orig\n");
        pp->f[CEXP_OUT][CEXP_PLAIN].len = 5;
    }
    rep_open(&rep);
    CHECK(cexp_update(&run, &old, 0, fix, 1, &pl, rep.f) == 1);
    t = rep_text(&rep);
    CHECK(strstr(t, "corpus_runerr:p: the port gives p.orig.out again"));
    free(t);
    CHECK(!planned(&pl, "p.fixed-7.out", "orig\n"));
    cexp_plan_free(&pl);
    /* a FIX for a program the run does not have; a FIX of another pass is not this one's */
    CHECK(cexp_parse_fix("corpus_runerr:gone#9", &fix[1], err, sizeof err) == 0);
    rep_open(&rep);
    CHECK(cexp_update(&run, &old, 0, fix + 1, 1, &pl, rep.f) == 1);
    t = rep_text(&rep);
    CHECK(strstr(t, "corpus_runerr:gone: FIX names a program the run does not have"));
    free(t);
    cexp_plan_free(&pl);
    /* the serial data of a serial pass; a program without output */
    CHECK(add(&run, "s.out", "s\n") == 0);
    rep_open(&rep);
    CHECK(cexp_update(&run, &old, 1, NULL, 0, &pl, rep.f) == 1);
    t = rep_text(&rep);
    CHECK(strstr(t, "corpus_runerr:s: no serial data"));
    free(t);
    cexp_plan_free(&pl);
    cexp_free(&run);
    cexp_free(&old);
}

static void test_update_exe(void)
{
    struct cexp_set old;
    struct cexp_plan pl = { 0 };
    struct cexp_fix fix;
    struct rep rep;
    char err[300], *t;

    set_init(&old, "corpus");
    CHECK(add(&old, "t130_blit.out", "x\n") == 0);
    CHECK(add(&old, "q.orig.exe", H1) == 0);
    CHECK(add(&old, "q.fixed-46.exe", H2) == 0);
    CHECK(cexp_parse_fix("corpus:t130_blit#46", &fix, err, sizeof err) == 0);
    /* a fix whose executables differ: the pair */
    rep_open(&rep);
    CHECK(cexp_update_exe(&old, "t130_blit", H1, H2, &fix, 1, &pl, rep.f) == 0);
    free(rep_text(&rep));
    CHECK(planned(&pl, "t130_blit.orig.exe", H1) && planned(&pl, "t130_blit.fixed-46.exe", H2));
    cexp_plan_free(&pl);
    /* the same without FIX: nothing (the judge compares them live) */
    rep_open(&rep);
    CHECK(cexp_update_exe(&old, "t130_blit", H1, H2, NULL, 0, &pl, rep.f) == 0 && !pl.n);
    free(rep_text(&rep));
    /* a fix whose executables agree: only noted */
    rep_open(&rep);
    CHECK(cexp_update_exe(&old, "t130_blit", H1, H1, &fix, 1, &pl, rep.f) == 0 && !pl.n);
    t = rep_text(&rep);
    CHECK(strstr(t, "the executables of ref and port agree"));
    free(t);
    /* an existing pair: the port's hash rewritten; the original's differs: reported */
    rep_open(&rep);
    CHECK(cexp_update_exe(&old, "q", H3, H3, NULL, 0, &pl, rep.f) == 0);
    t = rep_text(&rep);
    CHECK(strstr(t, "corpus:q: the original's executable is not q.orig.exe"));
    free(t);
    CHECK(planned(&pl, "q.fixed-46.exe", H3) && pl.n == 1);
    cexp_plan_free(&pl);
    /* the port's is the original's again: refused */
    rep_open(&rep);
    CHECK(cexp_update_exe(&old, "q", H1, H1, NULL, 0, &pl, rep.f) == 1 && !pl.n);
    t = rep_text(&rep);
    CHECK(strstr(t, "corpus:q: the port's executable is q.orig.exe again"));
    free(t);
    cexp_free(&old);
}

static void test_project(void)
{
    struct cexp_set s;
    struct rep rep;
    char *t;

    set_init(&s, "corpus_runerr");
    CHECK(add(&s, "t124_bitmaperr.orig.out", "abc") == 0);
    CHECK(add(&s, "t124_bitmaperr.fixed-6.out", "abd") == 0);
    CHECK(add(&s, "t01_int.out", "") == 0);
    CHECK(add(&s, "t130_blit.orig.exe", H1) == 0);
    rep_open(&rep);
    cexp_project(rep.f, &s);
    t = rep_text(&rep);
    CHECK(!strcmp(t, "corpus_runerr|t01_int|out|1\tbytes 0 fnv 811C9DC5\n"
                     "corpus_runerr|t124_bitmaperr|out|1 [orig]\tbytes 3 fnv 1A47E90B\n"
                     "corpus_runerr|t124_bitmaperr|out|1 [fixed #6]\tbytes 3 fnv 1F47F0EA\n"
                     "corpus_runerr|t130_blit|exe|1 [orig]\tsha256 "
                     "1111111111111111111111111111111111111111111111111111111111111111\n"));
    if (strncmp(t, "corpus_runerr|t01_int|out|1\tbytes 0", 36))
        fprintf(stderr, "projection:\n%s", t);
    free(t);
    cexp_free(&s);
}

/* a plan applied to a directory and the directory loaded back */
static void test_apply(void)
{
    char tmpl[] = "/tmp/cexp-XXXXXX", dir[64], err[400];
    struct cexp_set s, back;
    struct cexp_plan pl = { 0 };
    struct rep rep;
    char *t;
    struct dirent *e;
    DIR *d;

    if (!mkdtemp(tmpl)) {
        CHECK(!"mkdtemp");
        return;
    }
    snprintf(dir, sizeof dir, "%s/corpus", tmpl);
    set_init(&s, "corpus");
    CHECK(add(&s, "a.out", "a\n") == 0);
    {
        struct cexp_set run;
        struct cexp_fix fix;

        set_init(&run, "corpus");
        CHECK(add(&run, "a.out", "a2\n") == 0);
        CHECK(cexp_parse_fix("corpus:a#5", &fix, err, sizeof err) == 0);
        /* the first state: a.out */
        CHECK(cexp_update(&run, NULL, 0, NULL, 0, &pl, stderr) == 0);
        CHECK(cexp_plan_apply(&pl, dir, NULL, err, sizeof err) == 0);
        cexp_plan_free(&pl);
        CHECK(cexp_load(&back, dir, CEXP_STRICT, err, sizeof err) == 0 && back.n == 1
              && !strcmp(back.pass, "corpus"));
        /* the pair from it, then the executables */
        rep_open(&rep);
        CHECK(cexp_update(&run, &s, 0, &fix, 1, &pl, rep.f) == 0);
        CHECK(cexp_update_exe(&back, "a", H1, H2, &fix, 1, &pl, rep.f) == 0);
        free(rep_text(&rep));
        cexp_free(&back);
        rep_open(&rep);
        CHECK(cexp_plan_apply(&pl, dir, rep.f, err, sizeof err) == 0);
        t = rep_text(&rep);
        CHECK(strstr(t, "wrote ") && strstr(t, "removed "));
        free(t);
        cexp_plan_free(&pl);
        cexp_free(&run);
    }
    /* read back by the judge: strict, the pair applies */
    CHECK(cexp_load(&back, dir, CEXP_STRICT, err, sizeof err) == 0);
    {
        const struct cexp_prog *p = cexp_find(&back, "a");
        enum cexp_qual m;

        CHECK(p && cexp_judge(p, CEXP_OUT, CEXP_SIDE_PORT, (const uint8_t *)"a2\n", 3, &m) == 0
              && cexp_judge(p, CEXP_OUT, CEXP_SIDE_ORIG, (const uint8_t *)"a\n", 2, &m) == 0
              && cexp_judge(p, CEXP_EXE, CEXP_SIDE_PORT, (const uint8_t *)H2, CEXP_HASHLEN, &m) == 0
              && cexp_judge(p, CEXP_EXE, CEXP_SIDE_ORIG, (const uint8_t *)H1, CEXP_HASHLEN, &m) == 0);
    }
    cexp_free(&back);
    /* a malformed directory names the file with its path */
    {
        char path[128];
        FILE *f;

        snprintf(path, sizeof path, "%s/a.out", dir);
        f = fopen(path, "wb");
        CHECK(f && fputs("a\n", f) >= 0 && !fclose(f));
        CHECK(cexp_load(&back, dir, CEXP_STRICT, err, sizeof err) == -2
              && strstr(err, "/corpus/a.out and a.orig.out: an unqualified and a qualified"));
        cexp_free(&back);
    }
    /* a directory that does not exist */
    {
        char missing[96];

        snprintf(missing, sizeof missing, "%s/none", tmpl);
        CHECK(cexp_load(&back, missing, CEXP_STRICT | CEXP_MISSING_OK, err, sizeof err) == 0 && !back.n);
        cexp_free(&back);
        CHECK(cexp_load(&back, missing, CEXP_STRICT, err, sizeof err) == -3);
        cexp_free(&back);
    }
    cexp_free(&s);
    if ((d = opendir(dir)) != NULL) {
        while ((e = readdir(d)) != NULL)
            if (e->d_name[0] != '.') {
                char path[400];

                snprintf(path, sizeof path, "%s/%s", dir, e->d_name);
                unlink(path);
            }
        closedir(d);
    }
    rmdir(dir);
    rmdir(tmpl);
}

int main(void)
{
    test_names();
    test_sides();
    test_hash_diff();
    test_accept();
    test_update();
    test_update_exe();
    test_project();
    test_apply();
    return failures ? 1 : 0;
}
