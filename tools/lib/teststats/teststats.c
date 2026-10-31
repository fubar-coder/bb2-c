/*
 * teststats.c - statistics of test runs, see teststats.h.
 */
#define _POSIX_C_SOURCE 200809L /* strdup */
#include "teststats.h"
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "junit.h"
#include "statline.h"

struct run {
    double time;
    enum junit_status status;
    size_t file;
};

struct sess {
    struct statline st;
    char *group;                /* config, or TEST:config */
    size_t file;
};

struct test {
    char *name;
    struct run *runs;
    size_t nruns;
    struct sess *sess;
    size_t nsess;
    int multi;                  /* lines of more than one test= */
};

struct teststats {
    struct test *tests;
    size_t ntests;
    size_t nfiles;
};

struct teststats *teststats_new(void)
{
    return calloc(1, sizeof(struct teststats));
}

void teststats_free(struct teststats *t)
{
    size_t i, k;

    if (!t)
        return;
    for (i = 0; i < t->ntests; i++) {
        for (k = 0; k < t->tests[i].nsess; k++)
            free(t->tests[i].sess[k].group);
        free(t->tests[i].sess);
        free(t->tests[i].runs);
        free(t->tests[i].name);
    }
    free(t->tests);
    free(t);
}

size_t teststats_runs(const struct teststats *t)
{
    return t->nfiles;
}

static struct test *find_test(struct teststats *t, const char *name)
{
    struct test *v;
    size_t i;

    for (i = 0; i < t->ntests; i++)
        if (!strcmp(t->tests[i].name, name))
            return &t->tests[i];
    if (!(v = realloc(t->tests, (t->ntests + 1) * sizeof *v)))
        return NULL;
    t->tests = v;
    memset(&v[t->ntests], 0, sizeof *v);
    if (!(v[t->ntests].name = strdup(name)))
        return NULL;
    return &v[t->ntests++];
}

static char *str_printf(const char *fmt, ...)
{
    va_list ap;
    char *s;
    int n;

    va_start(ap, fmt);
    n = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (n < 0 || !(s = malloc((size_t)n + 1)))
        return NULL;
    va_start(ap, fmt);
    vsnprintf(s, (size_t)n + 1, fmt, ap);
    va_end(ap);
    return s;
}

int teststats_add(struct teststats *t, const char *source, const char *text, size_t len,
                  FILE *diag)
{
    struct junit_file f;
    char err[300];
    size_t i;
    int problems = 0;

    if (junit_parse(text, len, &f, err, sizeof err)) {
        fprintf(diag, "%s: %s - the file is left out\n", source, err);
        return -1;
    }
    for (i = 0; i < f.nproblems; i++, problems++)
        fprintf(diag, "%s: %s\n", source, f.problems[i]);
    for (i = 0; i < f.ncases; i++) {
        const struct junit_case *c = &f.cases[i];
        struct test *te = find_test(t, c->name);
        struct run *rv;
        const char *l = c->out;
        unsigned lineno = 0;
        size_t first, k;

        if (!te || !(rv = realloc(te->runs, (te->nruns + 1) * sizeof *rv)))
            goto nomem;
        te->runs = rv;
        rv[te->nruns++] = (struct run){ c->time, c->status, t->nfiles };
        first = te->nsess;
        while (*l) {
            size_t n = strcspn(l, "\n");
            struct statline st;
            int r = statline_parse(l, n, &st, err, sizeof err);

            lineno++;
            if (r < 0) {
                fprintf(diag, "%s: testcase %s, output line %u: bad @stat line (%s): %.*s\n",
                        source, c->name, lineno, err, (int)(n > 120 ? 120 : n), l);
                problems++;
            } else if (r > 0) {
                struct sess *sv = realloc(te->sess, (te->nsess + 1) * sizeof *sv);
                char *g;

                if (!sv)
                    goto nomem;
                te->sess = sv;
                if (!(g = strdup(st.config)))
                    goto nomem;
                sv[te->nsess++] = (struct sess){ st, g, t->nfiles };
            }
            l += n + (l[n] == '\n');
        }
        /* lines of more than one pass (in any run): PASS:config, for all
           of the test's sessions */
        for (k = 1; !te->multi && k < te->nsess; k++)
            if (strcmp(te->sess[k].st.test, te->sess[0].st.test)) {
                te->multi = 1;
                first = 0;
            }
        for (; te->multi && first < te->nsess; first++) {
            char *g = str_printf("%s:%s", te->sess[first].st.test, te->sess[first].st.config);

            if (!g)
                goto nomem;
            free(te->sess[first].group);
            te->sess[first].group = g;
        }
    }
    t->nfiles++;
    junit_free(&f);
    return problems;
nomem:
    fprintf(diag, "%s: out of memory\n", source);
    t->nfiles++;
    junit_free(&f);
    return -1;
}

/* ------------------------------------------------------------- tables */

struct table {
    const char *section, *title;
    size_t ncols;
    char **cells;               /* row-major, the header first */
    size_t nrows;
    int bad;
};

static void row(struct table *tb, ...)
{
    char **v;
    va_list ap;
    size_t i;

    if (tb->bad)
        return;
    if (!(v = realloc(tb->cells, (tb->nrows + 1) * tb->ncols * sizeof *v))) {
        tb->bad = 1;
        return;
    }
    tb->cells = v;
    va_start(ap, tb);
    for (i = 0; i < tb->ncols; i++) {
        const char *s = va_arg(ap, const char *);

        if (!(v[tb->nrows * tb->ncols + i] = strdup(s ? s : "")))
            tb->bad = 1;
    }
    va_end(ap);
    tb->nrows++;
}

static int print_table(struct table *tb, int tsv, FILE *out)
{
    size_t r, c, *w = calloc(tb->ncols, sizeof *w);
    int bad = tb->bad || !w;

    if (!bad) {
        if (tsv) {
            for (r = 0; r < tb->nrows; r++) {
                fprintf(out, "%s%s", r ? "" : "#", tb->section);
                for (c = 0; c < tb->ncols; c++)
                    fprintf(out, "\t%s", tb->cells[r * tb->ncols + c]);
                fputc('\n', out);
            }
        } else {
            for (r = 0; r < tb->nrows; r++)
                for (c = 0; c < tb->ncols; c++) {
                    size_t n = strlen(tb->cells[r * tb->ncols + c]);

                    if (n > w[c])
                        w[c] = n;
                }
            fprintf(out, "== %s\n", tb->title);
            if (tb->nrows == 1)
                fprintf(out, "(none)\n");
            else
                for (r = 0; r < tb->nrows; r++) {
                    size_t last = tb->ncols;

                    /* no blanks at the end of the line */
                    while (last > 1 && !*tb->cells[r * tb->ncols + last - 1])
                        last--;
                    for (c = 0; c < last; c++) {
                        const char *s = tb->cells[r * tb->ncols + c];

                        if (c + 1 == last)
                            fprintf(out, "%s", s);
                        else
                            fprintf(out, "%-*s  ", (int)w[c], s);
                    }
                    fputc('\n', out);
                }
            fputc('\n', out);
        }
    }
    for (r = 0; r < tb->nrows * tb->ncols; r++)
        free(tb->cells[r]);
    free(tb->cells);
    free(w);
    tb->cells = NULL;
    tb->nrows = 0;
    return bad ? -1 : 0;
}

/* ---------------------------------------------------------- the report */

static int by_double(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;

    return x < y ? -1 : x > y;
}

static double median(double *v, size_t n)
{
    qsort(v, n, sizeof *v, by_double);
    return n % 2 ? v[n / 2] : (v[n / 2 - 1] + v[n / 2]) / 2;
}

/* the groups (configs) of a test, in the order of their first session */
static size_t groups(const struct test *te, const char ***out)
{
    const char **g = calloc(te->nsess + 1, sizeof *g);
    size_t n = 0, i, k;

    if (!g) {
        *out = NULL;
        return (size_t)-1;
    }
    for (i = 0; i < te->nsess; i++) {
        for (k = 0; k < n && strcmp(g[k], te->sess[i].group); k++)
            ;
        if (k == n)
            g[n++] = te->sess[i].group;
    }
    *out = g;
    return n;
}

/* a list "A x2, B" of the distinct strings in v (n of them) */
static char *counted(const char **v, size_t n)
{
    char *s = strdup(""), *t;
    size_t i, k, c;

    for (i = 0; s && i < n; i++) {
        for (k = 0; k < i && strcmp(v[k], v[i]); k++)
            ;
        if (k < i)
            continue;
        for (c = 0, k = i; k < n; k++)
            c += !strcmp(v[k], v[i]);
        t = c > 1 ? str_printf("%s%s%s x%zu", s, *s ? ", " : "", v[i], c)
                  : str_printf("%s%s%s", s, *s ? ", " : "", v[i]);
        free(s);
        s = t;
    }
    return s;
}

static char *fmt(const char *f, ...)
{
    va_list ap;
    char buf[64];

    va_start(ap, f);
    vsnprintf(buf, sizeof buf, f, ap);
    va_end(ap);
    return strdup(buf);
}

/* the facts of one group of a test */
struct gstat {
    size_t nsess, ntimeout, nrunsseen;
    double max_session, max_timed, max_wait, max_timed_ok;
    unsigned limit;
    int all_timeout;            /* every session of every run */
    char *snapshots, *verdicts; /* counted lists */
    char *run_verdicts;         /* the last verdict of each run, counted */
    size_t ndistinct_verdicts;  /* among the runs' last verdicts */
    int end_differs;            /* a run with timeouts and one without */
};

static void gstat_free(struct gstat *g)
{
    free(g->snapshots);
    free(g->verdicts);
    free(g->run_verdicts);
}

static int gstat_of(const struct test *te, const char *group, size_t nfiles, struct gstat *g)
{
    const char **snap = calloc(te->nsess + 1, sizeof *snap);
    const char **verd = calloc(te->nsess + 1, sizeof *verd);
    const char **rv = calloc(nfiles + 1, sizeof *rv);
    size_t nverd = 0, nrv = 0, i, f, k;
    int any_to = 0, any_clean = 0;

    memset(g, 0, sizeof *g);
    if (!snap || !verd || !rv) {
        free(snap);
        free(verd);
        free(rv);
        return -1;
    }
    for (i = 0; i < te->nsess; i++) {
        const struct statline *s = &te->sess[i].st;

        if (strcmp(te->sess[i].group, group))
            continue;
        snap[g->nsess++] = s->snapshot;
        if (s->session > g->max_session)
            g->max_session = s->session;
        if (s->timed > g->max_timed)
            g->max_timed = s->timed;
        if (s->wait > g->max_wait)
            g->max_wait = s->wait;
        if (s->limit > g->limit)
            g->limit = s->limit;
        if (!strcmp(s->end, "timeout"))
            g->ntimeout++;
        else if (s->timed > g->max_timed_ok)
            g->max_timed_ok = s->timed;
        if (*s->verdict)
            verd[nverd++] = s->verdict;
    }
    /* per run: the last verdict, timeouts or not */
    for (f = 0; f < nfiles; f++) {
        const char *last = NULL;
        int seen = 0, to = 0;

        for (i = 0; i < te->nsess; i++)
            if (te->sess[i].file == f && !strcmp(te->sess[i].group, group)) {
                seen = 1;
                to |= !strcmp(te->sess[i].st.end, "timeout");
                if (*te->sess[i].st.verdict)
                    last = te->sess[i].st.verdict;
            }
        if (!seen)
            continue;
        g->nrunsseen++;
        if (to)
            any_to = 1;
        else
            any_clean = 1;
        if (last) {
            for (k = 0; k < nrv && strcmp(rv[k], last); k++)
                ;
            g->ndistinct_verdicts += k == nrv;
            rv[nrv++] = last;
        }
    }
    g->end_differs = any_to && any_clean;
    g->all_timeout = g->nsess && g->ntimeout == g->nsess;
    g->snapshots = counted(snap, g->nsess);
    g->verdicts = counted(verd, nverd);
    g->run_verdicts = counted(rv, nrv);
    free(snap);
    free(verd);
    free(rv);
    if (!g->snapshots || !g->verdicts || !g->run_verdicts) {
        gstat_free(g);
        return -1;
    }
    return 0;
}

static unsigned suggest(double timed, const struct teststats_options *o)
{
    double s = ceil(timed * o->factor);

    return (unsigned)(s < o->floor ? ceil(o->floor) : s);
}

int teststats_report(const struct teststats *t, const struct teststats_options *opt, FILE *out)
{
    struct teststats_options o = opt ? *opt : (struct teststats_options){ 0 };
    struct table tests = { "tests", "Tests: runs and durations (seconds)", 6, 0, 0, 0 };
    struct table sess = { "sessions", "Emulator sessions per test and config (seconds; "
                          "session without the slot wait, timed: counted against the limit)",
                          10, 0, 0, 0 };
    struct table flaky = { "flaky", "Flaky: results that differ between runs", 4, 0, 0, 0 };
    struct table lim = { "limits", NULL, 6, 0, 0, 0 };
    struct table cost = { "cost", "Suggested COST (ctest, seconds: the median duration)", 4,
                          0, 0, 0 };
    char limtitle[200];
    size_t i, k;
    int bad = 0;

    if (!(o.factor > 0))
        o.factor = TESTSTATS_FACTOR;
    if (!(o.floor >= 0))
        o.floor = TESTSTATS_FLOOR;
    snprintf(limtitle, sizeof limtitle,
             "Suggested time limits per session (max timed x %g, at least %g s)", o.factor,
             o.floor);
    lim.title = limtitle;
    row(&tests, "test", "runs", "passed", "failed", "skipped", "min/median/max");
    row(&sess, "test", "config", "sessions", "snapshot", "max session", "max timed",
        "max wait", "timeouts", "limit", "verdicts");
    row(&flaky, "test", "config", "what", "seen");
    row(&lim, "test", "config", "max timed", "limit now", "suggested", "note");
    row(&cost, "test", "runs", "median", "COST");
    for (i = 0; i < t->ntests && !bad; i++) {
        const struct test *te = &t->tests[i];
        double *d = calloc(te->nruns + 1, sizeof *d);
        size_t nd = 0, np = 0, nf = 0, ns = 0, ng;
        const char **g = NULL;
        unsigned best = 0, now = 0;
        int any_normal = 0;

        if (!d) {
            bad = 1;
            break;
        }
        for (k = 0; k < te->nruns; k++) {
            np += te->runs[k].status == JUNIT_PASSED;
            nf += te->runs[k].status == JUNIT_FAILED;
            ns += te->runs[k].status == JUNIT_SKIPPED;
            if (te->runs[k].status != JUNIT_SKIPPED)
                d[nd++] = te->runs[k].time;
        }
        {
            char *a = fmt("%zu", te->nruns), *b = fmt("%zu", np), *c = fmt("%zu", nf);
            char *e = fmt("%zu", ns), *m = NULL, *md = NULL, *co = NULL;

            if (nd) {
                double med = median(d, nd);
                unsigned cs = (unsigned)ceil(med);

                m = fmt("%.1f / %.1f / %.1f", d[0], med, d[nd - 1]);
                md = fmt("%.1f", med);
                co = fmt("%u", cs < 1 ? 1 : cs);
                row(&cost, te->name, a, md, co);
            }
            row(&tests, te->name, a, b, c, e, m ? m : "-");
            if (np && nf) {
                char *s = str_printf("passed %zu, failed %zu", np, nf);

                row(&flaky, te->name, "", "ctest result", s);
                free(s);
            }
            free(a), free(b), free(c), free(e), free(m), free(md), free(co);
        }
        free(d);
        if ((ng = groups(te, &g)) == (size_t)-1) {
            bad = 1;
            break;
        }
        for (k = 0; k < ng; k++) {
            struct gstat s;
            char *n, *ms, *mt, *mw, *to, *li, *mto, *sg = NULL, *note = NULL;

            if (gstat_of(te, g[k], t->nfiles, &s)) {
                bad = 1;
                break;
            }
            n = fmt("%zu", s.nsess);
            ms = fmt("%.1f", s.max_session);
            mt = fmt("%.1f", s.max_timed);
            mw = fmt("%.1f", s.max_wait);
            to = fmt("%zu", s.ntimeout);
            li = fmt("%u", s.limit);
            if (s.limit > now)
                now = s.limit;
            row(&sess, te->name, g[k], n, s.snapshots, ms, mt, mw, to, li,
                *s.verdicts ? s.verdicts : "-");
            if (s.ndistinct_verdicts > 1)
                row(&flaky, te->name, g[k], "verdict", s.run_verdicts);
            if (s.end_differs) {
                char *w = str_printf("timeout in some of %zu runs", s.nrunsseen);

                row(&flaky, te->name, g[k], "end", w);
                free(w);
            }
            if (s.all_timeout) {
                mto = strdup("-");
                note = str_printf("hangs: every session ended by the time limit (sessions %zu, "
                                  "runs %zu); no data on the last output - no suggestion",
                                  s.nsess, s.nrunsseen);
            } else {
                unsigned sug = suggest(s.max_timed_ok, &o);

                any_normal = 1;
                if (sug > best)
                    best = sug;
                mto = fmt("%.1f", s.max_timed_ok);
                sg = fmt("%u", sug);
                if (s.ntimeout)
                    note = str_printf("%zu of %zu sessions ended by timeout (not counted)",
                                      s.ntimeout, s.nsess);
            }
            row(&lim, te->name, g[k], mto, li, sg ? sg : "-", note ? note : "");
            free(n), free(ms), free(mt), free(mw), free(to), free(li), free(mto), free(sg);
            free(note);
            gstat_free(&s);
        }
        if (ng > 1 || (ng == 1 && !any_normal)) {
            char *li = fmt("%u", now), *sg = any_normal ? fmt("%u", best) : NULL;

            row(&lim, te->name, "(test)", "", li, sg ? sg : "-",
                any_normal ? "the largest of its configs" : "every config hangs");
            free(li);
            free(sg);
        }
        free(g);
    }
    if (!o.tsv)
        fprintf(out, "%zu runs (JUnit files), %zu tests\n\n", t->nfiles, t->ntests);
    bad |= print_table(&tests, o.tsv, out) | print_table(&sess, o.tsv, out)
           | print_table(&flaky, o.tsv, out) | print_table(&lim, o.tsv, out)
           | print_table(&cost, o.tsv, out);
    return bad ? -1 : 0;
}
