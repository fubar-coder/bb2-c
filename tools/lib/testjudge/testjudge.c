/*
 * testjudge.c - the judge of the Blitz tests of the model (testjudge.h).
 */
#include "testjudge.h"
#include "basictest.h"
#include "outmatch.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void tj_msgs_free(struct tj_msgs *m)
{
    for (size_t i = 0; i < m->n; i++)
        free(m->v[i]);
    free(m->v);
    memset(m, 0, sizeof *m);
}

static int msg(struct tj_msgs *m, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
static int msg(struct tj_msgs *m, const char *fmt, ...)
{
    va_list ap;
    char *s;
    char **v;
    int n;

    va_start(ap, fmt);
    n = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (n < 0 || !(s = malloc((size_t)n + 1)))
        return -1;
    va_start(ap, fmt);
    vsnprintf(s, (size_t)n + 1, fmt, ap);
    va_end(ap);
    if (!(v = realloc(m->v, (m->n + 1) * sizeof *v))) {
        free(s);
        return -1;
    }
    m->v = v;
    v[m->n++] = s;
    return 0;
}

/* ------------------------------------------------------------ the result file */

/* an entry of the result file with its line */
struct ent {
    struct basictest_entry e;
    unsigned line;
    int block;                  /* the block it belongs to: -1 none, else
                                   BASICTEST_BLOCK_* */
    int in_case;                /* it belongs to the judged case */
};

/* what one check did in the run */
struct chk {
    const struct tp_check *c;
    size_t nent;                /* entries */
    const struct ent *first, *first_fail;
    int named;                  /* an assertion names it */
};

struct run {
    uint8_t *buf;               /* the result file, decoded in place */
    struct ent *e;
    size_t n;
    int has_begin, has_end, has_incomplete;
    int case_started, case_ended, case_ended_at_error;
    struct chk *chk;            /* one per check of the table */
    const char **req;           /* the requesters not matched by an announcement */
    size_t nreq;
    int broken;
    int foreign;                /* a block of another case, or the case twice */
};

static void run_free(struct run *r)
{
    free(r->buf);
    free(r->e);
    free(r->chk);
    free(r->req);
}

static void str_text(char *out, size_t size, const struct basictest_str *s)
{
    size_t o = 0;

    for (size_t i = 0; i < s->length && o + 5 < size; i++) {
        uint8_t c = s->data[i];

        if (c >= 0x20 && c <= 0x7E && c != '\\' && c != '"')
            out[o++] = (char)c;
        else
            o += (size_t)snprintf(out + o, size - o, "\\x%02X", c);
    }
    out[o] = 0;
}

/*
 * A string value for the report: "TEXT", and for a value the library cut
 * to fit the line (shorter than the length of the whole string) "START"
 * (cut, length N).
 */
static void strval_text(char *out, size_t size, const struct basictest_str *s, uint32_t length)
{
    char t[150];

    str_text(t, sizeof t, s);
    if (s->length < length)
        snprintf(out, size, "\"%s\" (cut, length %lu)", t, (unsigned long)length);
    else
        snprintf(out, size, "\"%s\"", t);
}

static int same_name(const struct basictest_str *s, const char *name)
{
    return s->length == strlen(name) && !memcmp(s->data, name, s->length);
}

/* the file read and checked; 0, TJ_ERROR (msgs), -1 out of memory */
static int read_run(const struct tj_test *t, const struct tj_obs *obs, struct run *r, struct tj_msgs *msgs)
{
    size_t p = 0, cap = 0;
    unsigned line = 0;
    int block = -1, cur_is_case = 0, seen_case = 0;

    if (!(r->chk = calloc(t->src->nchecks + 1, sizeof *r->chk)))
        return -1;
    for (size_t i = 0; i < t->src->nchecks; i++)
        r->chk[i].c = &t->src->checks[i];
    if (!obs->has_result)
        return 0;
    if (!(r->buf = malloc(obs->result_len + 1)))
        return -1;
    memcpy(r->buf, obs->result, obs->result_len);
    while (p < obs->result_len) {
        uint8_t *nl = memchr(r->buf + p, '\n', obs->result_len - p);
        struct basictest_error be;
        struct ent *en;

        line++;
        if (!nl)
            return msg(msgs, "the result file: line %u has no line end", line) ? -1 : TJ_ERROR;
        if (r->n == cap) {
            struct ent *v = realloc(r->e, (cap = cap ? 2 * cap : 32) * sizeof *v);

            if (!v)
                return -1;
            r->e = v;
        }
        en = &r->e[r->n];
        memset(en, 0, sizeof *en);
        if (basictest_read(r->buf + p, (size_t)(nl - (r->buf + p)), line, &en->e, &be)) {
            char text[200];

            basictest_error_text(&be, text, sizeof text);
            return msg(msgs, "the result file: %s", text) ? -1 : TJ_ERROR;
        }
        en->line = line;
        p = (size_t)(nl - r->buf) + 1;
        if (r->n == 0 && en->e.kind != BASICTEST_BEGIN)
            return msg(msgs, "the result file: line 1 is no begin entry") ? -1 : TJ_ERROR;
        if (r->has_end)
            return msg(msgs, "the result file: line %u follows the end entry", line) ? -1 : TJ_ERROR;
        switch (en->e.kind) {
        case BASICTEST_BEGIN:
            if (r->n)
                return msg(msgs, "the result file: a second begin entry in line %u", line) ? -1 : TJ_ERROR;
            r->has_begin = 1;
            break;
        case BASICTEST_BLOCK:
            block = en->e.block;
            cur_is_case = 0;
            if (block == BASICTEST_BLOCK_CASE) {
                char name[200];

                str_text(name, sizeof name, &en->e.name);
                if (!same_name(&en->e.name, t->case_name)) {
                    r->foreign = 1;
                    if (msg(msgs, "the result file holds the case %s (line %u), which was not selected", name, line))
                        return -1;
                } else if (seen_case) {
                    r->foreign = 1;
                    if (msg(msgs, "the result file holds the case %s twice (line %u)", name, line))
                        return -1;
                } else {
                    cur_is_case = seen_case = 1;
                    r->case_started = 1;
                }
            }
            break;
        case BASICTEST_BLOCKEND:
            /* after broken (block end without a block) the library goes on */
            if (block < 0 && !r->broken)
                return msg(msgs, "the result file: a blockend outside a block in line %u", line) ? -1 : TJ_ERROR;
            if (cur_is_case)
                r->case_ended = 1;
            en->block = block;
            en->in_case = cur_is_case;
            block = -1;
            cur_is_case = 0;
            r->n++;
            continue;
        case BASICTEST_INCOMPLETE:
            r->has_incomplete = 1;
            break;
        case BASICTEST_END:
            r->has_end = 1;
            break;
        case BASICTEST_BROKEN:
            r->broken = 1;
            break;
        default:
            /* after broken (check outside a block) the library goes on */
            if (block < 0 && !r->broken)
                return msg(msgs, "the result file: an entry outside a block in line %u", line) ? -1 : TJ_ERROR;
        }
        en->block = block;
        en->in_case = cur_is_case;
        r->n++;
    }
    return 0;
}

/* ------------------------------------------------------------ values */

static void long_text(char *out, size_t size, uint32_t v)
{
    snprintf(out, size, "%ld ($%08lX)", (long)(int32_t)v, (unsigned long)v);
}

/* a quick (16.16 fixed point) in decimal, exactly, with its bits */
static void quick_text(char *out, size_t size, uint32_t v)
{
    int32_t q = (int32_t)v;
    uint64_t a = q < 0 ? (uint64_t)(-(int64_t)q) : (uint64_t)q;
    uint64_t frac = a & 0xFFFF;
    size_t o = (size_t)snprintf(out, size, "%s%llu", q < 0 ? "-" : "", (unsigned long long)(a >> 16));

    if (frac && o + 1 < size)
        out[o++] = '.';
    while (frac && o + 1 < size) {
        frac *= 10;
        out[o++] = (char)('0' + (frac >> 16));
        frac &= 0xFFFF;
    }
    if (o < size)
        snprintf(out + o, size - o, " ($%08lX)", (unsigned long)v);
}

/* a Motorola FFP float (mantissa 31-8, sign 7, exponent 6-0 excess 64) in
   decimal, with its bits */
static void ffp_text(char *out, size_t size, uint32_t v)
{
    double m = (double)(v >> 8), x = 0;

    if (v >> 8) {
        int e = (int)(v & 0x7F) - 64 - 24;

        x = m;
        for (; e > 0; e--)
            x *= 2;
        for (; e < 0; e++)
            x /= 2;
        if (v & 0x80)
            x = -x;
    }
    snprintf(out, size, "%.7g ($%08lX)", x, (unsigned long)v);
}

/* a value of a check entry of the long kinds, by its kind */
static void value_text(char *out, size_t size, int kind, uint32_t v)
{
    if (kind == BASICTEST_CHECKQ)
        quick_text(out, size, v);
    else if (kind == BASICTEST_CHECKFLT)
        ffp_text(out, size, v);
    else
        long_text(out, size, v);
}

/* the name or the number of a check, for the report */
static void check_id(char *out, size_t size, const struct tp_check *c)
{
    if (c->name)
        snprintf(out, size, "%s %s", c->ns, c->name);
    else
        snprintf(out, size, "%s #%u", c->ns, c->number);
}

/* the report of a failed check (its first failed entry) */
static int report_check(const struct tj_test *t, const struct chk *k, struct tj_msgs *msgs, const char *why)
{
    char id[300], e[200], a[200];
    const struct tp_check *c = k->c;
    const struct basictest_entry *x = &k->first_fail->e;

    check_id(id, sizeof id, c);
    if (x->kind == BASICTEST_CHECKSTR) {
        strval_text(e, sizeof e, &x->expected_str, x->expected_length);
        strval_text(a, sizeof a, &x->actual_str, x->actual_length);
    } else {
        value_text(e, sizeof e, x->kind, x->expected);
        value_text(a, sizeof a, x->kind, x->actual);
    }
    if (x->kind == BASICTEST_CHECKFLT) {
        char d[60];

        ffp_text(d, sizeof d, x->delta);
        return msg(msgs, "%scheck %s (%s:%u) failed: %s: expected %s, actual %s, delta %s%s", why, id, t->file,
                   c->line, c->expr ? c->expr : "", e, a, d, k->nent > 1 ? " (its first failed entry)" : "");
    }
    if (c->kind == TP_CHECK)
        return msg(msgs, "%scheck %s (%s:%u) failed: %s is 0%s", why, id, t->file, c->line, c->expr,
                   k->nent > 1 ? " (in one of its entries)" : "");
    return msg(msgs, "%scheck %s (%s:%u) failed: %s: expected %s, actual %s%s", why, id, t->file, c->line,
               c->expr ? c->expr : "", e, a, k->nent > 1 ? " (its first failed entry)" : "");
}

/* ------------------------------------------------------------ predicates */

static int cmp_int(int op, long long a, long long b)
{
    switch (op) {
    case TC_P_EQ: return a == b;
    case TC_P_NE: return a != b;
    case TC_P_LT: return a < b;
    case TC_P_LE: return a <= b;
    case TC_P_GT: return a > b;
    case TC_P_GE: return a >= b;
    }
    return 0;
}

/* a line of text matches the pattern */
static int text_matches(const char *text, const char *pat)
{
    const char *p = text;

    for (;;) {
        const char *nl = strchr(p, '\n');
        size_t n = nl ? (size_t)(nl - p) : strlen(p);

        if (n && p[n - 1] == '\r')
            n--;
        if (outmatch_line(pat, p, n))
            return 1;
        if (!nl || !nl[1])
            return 0;
        p = nl + 1;
    }
}

/* a text predicate: 1 holds, 0 not */
static int text_pred(const struct tc_pred *p, const char *v)
{
    switch (p->op) {
    case TC_P_EXISTS: return v && *v;
    case TC_P_NOT_EXISTS: return !v || !*v;
    case TC_P_EQ: return v && !strcmp(v, p->str);
    case TC_P_NE: return v && strcmp(v, p->str);
    case TC_P_CONTAINS: return v && strstr(v, p->str);
    case TC_P_NOT_CONTAINS: return v && !strstr(v, p->str);
    case TC_P_MATCHES: return v && text_matches(v, p->str);
    }
    return 0;
}

/* ------------------------------------------------------------ the assertions */

static int is_case(const struct tp_source *src, const char *name)
{
    for (size_t i = 0; i < src->ncases; i++)
        if (!strcmp(src->cases[i].name, name))
            return 1;
    return 0;
}

/* the check a query names: *field 0 verdict, 1 expected, 2 actual */
static struct chk *query_check(const struct tj_test *t, struct run *r, const struct tc_query *q, int *field)
{
    const struct tp_check *c = tp_find(t->src, q->ns, q->rest);
    size_t n = strlen(q->rest);

    *field = 0;
    if (!c && n > 9 && !strcmp(q->rest + n - 9, ".expected")) {
        char *name = strndup(q->rest, n - 9);

        if (name)
            c = tp_find(t->src, q->ns, name);
        free(name);
        *field = 1;
    }
    if (!c && n > 7 && !strcmp(q->rest + n - 7, ".actual")) {
        char *name = strndup(q->rest, n - 7);

        if (name)
            c = tp_find(t->src, q->ns, name);
        free(name);
        *field = 2;
    }
    return c ? &r->chk[c - t->src->checks] : NULL;
}

/* one assertion: 1 holds, 0 fails (msgs), TJ_ERROR, -1 out of memory */
static int eval(const struct tj_test *t, struct run *r, const struct tj_obs *obs, const struct tc_assertion *a,
                struct tj_msgs *msgs)
{
    const struct tc_pred *p = &a->p;
    char got[300];
    int ok = 0;

    got[0] = 0;
    switch (a->q.kind) {
    case TC_Q_RC:
        if (!obs->has_rc) {
            ok = p->op == TC_P_NOT_EXISTS;
            snprintf(got, sizeof got, "no exit code");
        } else {
            ok = p->op == TC_P_EXISTS || (p->op != TC_P_NOT_EXISTS && cmp_int(p->op, obs->rc, p->num));
            snprintf(got, sizeof got, "%lld", obs->rc);
        }
        break;
    case TC_Q_REQUESTER: {
        size_t hits = 0;

        for (size_t i = 0; i < r->nreq; i++)
            if ((p->op == TC_P_CONTAINS || p->op == TC_P_NOT_CONTAINS) ? strstr(r->req[i], p->str) != NULL
                : p->op == TC_P_MATCHES ? text_matches(r->req[i], p->str) : 1)
                hits++;
        switch (p->op) {
        case TC_P_CONTAINS:
        case TC_P_MATCHES:
            ok = hits > 0;
            break;
        case TC_P_NOT_CONTAINS:
            ok = hits == 0;
            break;
        case TC_P_EXISTS:
            ok = r->nreq > 0;
            break;
        case TC_P_NOT_EXISTS:
            ok = r->nreq == 0;
            break;
        case TC_P_COUNT:
            ok = cmp_int(p->cmp, (long long)r->nreq, p->num);
            break;
        }
        if (r->nreq)
            snprintf(got, sizeof got, "%zu, the first \"%.200s\"", r->nreq, r->req[0]);
        else
            snprintf(got, sizeof got, "none");
        break;
    }
    case TC_Q_COMPILE:
    case TC_Q_COMPILE_LOG:
    case TC_Q_OUTPUT:
    case TC_Q_SERIAL:
    case TC_Q_INJECT:
    case TC_Q_SESSION: {
        const char *v = a->q.kind == TC_Q_COMPILE ? obs->compile
                        : a->q.kind == TC_Q_COMPILE_LOG ? obs->compile_log
                        : a->q.kind == TC_Q_OUTPUT ? obs->output
                        : a->q.kind == TC_Q_SERIAL ? obs->serial
                        : a->q.kind == TC_Q_INJECT ? obs->inject : obs->session;

        ok = text_pred(p, v);
        if (!v)
            snprintf(got, sizeof got, "not captured");
        else
            snprintf(got, sizeof got, "\"%.200s\"%s", v, strlen(v) > 200 ? "..." : "");
        break;
    }
    case TC_Q_END_CASE:
    case TC_Q_END_RUN: {
        int present = a->q.kind == TC_Q_END_CASE ? (r->case_ended || r->case_ended_at_error) : r->has_end;

        ok = p->op == TC_P_EXISTS ? present : !present;
        snprintf(got, sizeof got, present ? "present" : "absent");
        break;
    }
    case TC_Q_CHECK: {
        int field;
        struct chk *k = query_check(t, r, &a->q, &field);
        const struct ent *x;

        if (!k)
            return msg(msgs, "%s:%u: %s: the source has no check %s in %s", a->at.file, a->at.line, a->query,
                       a->q.rest, a->q.ns) ? -1 : TJ_ERROR;
        if (field == 0) {
            const char *v = k->nent == 0 ? "not-run" : k->first_fail ? "fail" : "ok";

            if (p->vtype != TC_PV_WORD)
                return msg(msgs, "%s:%u: %s: a verdict is ok, fail or not-run", a->at.file, a->at.line,
                           a->query) ? -1 : TJ_ERROR;
            ok = (p->op == TC_P_EQ) == !strcmp(v, p->str);
            snprintf(got, sizeof got, "%s", v);
            if (!ok && k->first_fail && !strcmp(p->str, "ok")) {
                /* the details of the failure */
                if (report_check(t, k, msgs, ""))
                    return -1;
            }
            break;
        }
        x = k->first_fail ? k->first_fail : k->first;
        if (!x) {
            ok = 0;
            snprintf(got, sizeof got, "not run");
            break;
        }
        if (x->e.kind == BASICTEST_CHECK || x->e.kind == BASICTEST_CHECKQ || x->e.kind == BASICTEST_CHECKFLT) {
            uint32_t v = field == 1 ? x->e.expected : x->e.actual;

            if (p->vtype != TC_PV_INT)
                return msg(msgs, "%s:%u: %s: the value of this check is a long, compare it with an integer",
                           a->at.file, a->at.line, a->query) ? -1 : TJ_ERROR;
            ok = cmp_int(p->op, (int32_t)v, p->num);
            value_text(got, sizeof got, x->e.kind, v);
        } else if (x->e.kind == BASICTEST_CHECKSTR) {
            const struct basictest_str *s = field == 1 ? &x->e.expected_str : &x->e.actual_str;
            uint32_t length = field == 1 ? x->e.expected_length : x->e.actual_length;
            uint32_t crc = field == 1 ? x->e.expected_crc : x->e.actual_crc;

            if (p->vtype != TC_PV_STR)
                return msg(msgs, "%s:%u: %s: the value of this check is a string", a->at.file, a->at.line,
                           a->query) ? -1 : TJ_ERROR;
            if (p->op == TC_P_EQ || p->op == TC_P_NE) {
                /* by the length and the CRC32 of the whole value, which the
                   result file gives also for a value it cut */
                size_t n = strlen(p->str);
                int same = n == length && basictest_crc32((const uint8_t *)p->str, n) == crc;

                ok = (p->op == TC_P_EQ) == same;
            } else {
                char *v;

                if (s->length < length)
                    return msg(msgs, "%s:%u: %s: the value was cut to fit the line (length %lu); compare it "
                               "with == or !=", a->at.file, a->at.line, a->query, (unsigned long)length) ? -1
                                                                                                    : TJ_ERROR;
                if (!(v = strndup((const char *)s->data, s->length)))
                    return -1;
                ok = strlen(v) == s->length && text_pred(p, v);
                free(v);
            }
            strval_text(got, sizeof got, s, length);
        } else {
            return msg(msgs, "%s:%u: %s: this check has no values", a->at.file, a->at.line, a->query) ? -1
                                                                                                    : TJ_ERROR;
        }
        break;
    }
    }
    if (ok)
        return 1;
    if (msg(msgs, "assertion %s %s (%s:%u%s) does not hold: %s", a->query, a->pred, a->at.file, a->at.line,
            a->deviation ? ", a declared deviation" : "", got))
        return -1;
    return 0;
}

/* ------------------------------------------------------------ a cell */

int tj_judge_cell(const struct tj_test *t, size_t cell, int mode, const struct tj_obs *obs, struct tj_msgs *msgs)
{
    struct run r;
    struct tc_assertions as = { 0 };
    char errbuf[600];
    int res = TJ_PASS, broken = 0, rc;
    unsigned char *used = NULL;

    memset(&r, 0, sizeof r);
    rc = read_run(t, obs, &r, msgs);
    if (rc) {
        run_free(&r);
        return rc;
    }
    if (obs->has_result && !r.has_begin) {
        run_free(&r);
        return msg(msgs, "the result file is empty") ? -1 : TJ_ERROR;
    }
    if (!obs->has_result && msg(msgs, "no result file"))
        goto oom;

    /* the entries of the run: the checks, preconditions, announcements */
    if (obs->nreq && !(used = calloc(obs->nreq, 1)))
        goto oom;
    for (size_t i = 0; i < r.n; i++) {
        const struct ent *en = &r.e[i];
        const struct tp_check *c;
        struct chk *k;

        if (en->e.kind == BASICTEST_BROKEN) {
            char reason[200];

            str_text(reason, sizeof reason, &en->e.reason);
            broken = 1;
            if (msg(msgs, "the test library reports its state broken (line %u): %s", en->line, reason))
                goto oom;
            continue;
        }
        if (en->e.kind != BASICTEST_CHECK && en->e.kind != BASICTEST_CHECKSTR && en->e.kind != BASICTEST_CHECKQ
            && en->e.kind != BASICTEST_CHECKFLT && en->e.kind != BASICTEST_REQUIRE
            && en->e.kind != BASICTEST_EXPECT_ERROR)
            continue;
        /* the blocks of the run: the batch init, the judged case, the batch end */
        if (en->block < 0 || (en->block == BASICTEST_BLOCK_CASE && !en->in_case))
            continue;
        if (!(c = tp_by_k(t->src, en->e.k))) {
            res = TJ_ERROR;
            if (msg(msgs, "the result file: line %u names check %u, which the source does not have", en->line,
                    (unsigned)en->e.k))
                goto oom;
            continue;
        }
        {
            static const int want[] = { BASICTEST_CHECK, BASICTEST_CHECK, BASICTEST_CHECKSTR, BASICTEST_REQUIRE,
                                        BASICTEST_EXPECT_ERROR, BASICTEST_CHECKQ, BASICTEST_CHECKFLT };

            if (want[c->kind] != en->e.kind) {
                res = TJ_ERROR;
                if (msg(msgs, "the result file: line %u is no entry of check %u (!%s, %s:%u)", en->line,
                        (unsigned)en->e.k, tp_kind_name(c->kind), t->file, c->line))
                    goto oom;
                continue;
            }
        }
        k = &r.chk[c - t->src->checks];
        if (en->e.kind == BASICTEST_REQUIRE) {
            char id[300];

            check_id(id, sizeof id, c);
            broken = 1;
            if (msg(msgs, "precondition %s (%s:%u) failed: %s", id, t->file, c->line, c->expr))
                goto oom;
            continue;
        }
        if (en->e.kind == BASICTEST_EXPECT_ERROR) {
            size_t j;

            for (j = 0; j < obs->nreq; j++)
                if (!used[j] && obs->req[j].after >= en->line && obs->req[j].text
                    && strstr(obs->req[j].text, c->message))
                    break;
            if (j == obs->nreq) {
                if (msg(msgs, "the announced runtime error \"%s\" (%s:%u) did not come", c->message, t->file,
                        c->line))
                    goto oom;
                if (res == TJ_PASS)
                    res = TJ_FAIL;
                continue;
            }
            used[j] = 1;
            /* the case ends here: its last entry, then incomplete */
            if (en->in_case && (i + 1 == r.n || r.e[i + 1].e.kind == BASICTEST_INCOMPLETE
                                || r.e[i + 1].e.kind == BASICTEST_END))
                r.case_ended_at_error = 1;
            continue;
        }
        if (!k->nent++)
            k->first = en;
        if (!en->e.ok && !k->first_fail)
            k->first_fail = en;
    }
    if (!(r.req = calloc(obs->nreq + 1, sizeof *r.req)))
        goto oom;
    for (size_t j = 0; j < obs->nreq; j++)
        if (!used[j])
            r.req[r.nreq++] = obs->req[j].text ? obs->req[j].text : "";
    if (obs->has_result && !r.case_started && msg(msgs, "the case %s did not start", t->case_name))
        goto oom;

    /* the assertions */
    rc = tc_assertions_for(t->conf, t->m, cell, t->case_name, mode, &as, errbuf, sizeof errbuf);
    if (rc == -2)
        goto oom;
    if (rc) {
        res = TJ_ERROR;
        if (msg(msgs, "%s", errbuf))
            goto oom;
    }
    for (size_t i = 0; i < as.n; i++) {
        const struct tc_assertion *a = &as.a[i];
        int field, e;

        if (a->q.kind == TC_Q_CHECK) {
            struct chk *k;

            if (is_case(t->src, a->q.ns) && strcmp(a->q.ns, t->case_name))
                continue;       /* a check of another case */
            if ((k = query_check(t, &r, &a->q, &field)))
                k->named = 1;
        }
        if (a->q.kind == TC_Q_RC && a->at.depth == (unsigned)-1 && r.case_ended_at_error)
            continue;           /* the default exit code: the case ended at its runtime error */
        e = eval(t, &r, obs, a, msgs);
        if (e < 0)
            goto oom;
        if (e == TJ_ERROR)
            res = TJ_ERROR;
        else if (e == 0 && res == TJ_PASS)
            res = TJ_FAIL;
    }
    /* the default of the checks: every check with entries passed */
    for (size_t i = 0; i < t->src->nchecks; i++) {
        const struct chk *k = &r.chk[i];

        if (k->named || !k->first_fail)
            continue;
        if (report_check(t, k, msgs, ""))
            goto oom;
        if (res == TJ_PASS)
            res = TJ_FAIL;
    }
    if (r.foreign && res == TJ_PASS)
        res = TJ_FAIL;
    if (broken && res != TJ_ERROR)
        res = TJ_BROKEN;
    tc_assertions_free(&as);
    free(used);
    run_free(&r);
    return res;
oom:
    tc_assertions_free(&as);
    free(used);
    run_free(&r);
    return -1;
}

/* ------------------------------------------------------------ the verdict */

int tj_case_has_deviation(const struct tj_test *t)
{
    for (size_t i = 0; i < t->conf->ndeviations; i++) {
        const struct tc_tabkey *d = &t->conf->deviations[i];
        struct tc_query q;
        char m[300];
        int applies = 1;

        if (d->sel) {
            struct tc_selector sel;

            if (tc_selector_parse(d->sel, 1, &sel, m, sizeof m))
                continue;
            for (size_t j = 0; j < sel.n; j++)
                if (!sel.t[j].def && strcmp(sel.t[j].value, t->case_name))
                    applies = 0;
            tc_selector_free(&sel);
        }
        if (applies && tc_query_parse(d->key, &q, m, sizeof m) == 0) {
            if (q.kind == TC_Q_CHECK && is_case(t->src, q.ns) && strcmp(q.ns, t->case_name))
                applies = 0;
            tc_query_free(&q);
        }
        if (applies)
            return 1;
    }
    return 0;
}

static int append(struct tj_msgs *to, struct tj_msgs *from, const char *head)
{
    if (head && msg(to, "%s", head))
        return -1;
    for (size_t i = 0; i < from->n; i++)
        if (msg(to, "  %s", from->v[i]))
            return -1;
    tj_msgs_free(from);
    return 0;
}

int tj_verdict(const struct tj_test *t, const size_t *cells, const struct tj_obs *obs, size_t n,
               struct tj_msgs *msgs)
{
    int dev = tj_case_has_deviation(t);
    /* the test of an extension is judged like that of a fixed original bug */
    int fixed = tc_get(t->conf, "fixed-orig") != NULL || tc_get(t->conf, "extension") != NULL;
    int any_error = 0, any_broken = 0, any_fail = 0;
    char head[400];

    for (size_t i = 0; i < n; i++) {
        size_t cell = cells[i];
        const char *name = t->m->cells[cell].name;
        int faulty = dev && tc_cell_faulty(t->conf, t->m, cell, t->case_name);
        struct tj_msgs m = { 0 };
        int r;

        if (!dev) {
            r = tj_judge_cell(t, cell, TC_PLAIN, &obs[i], &m);
            snprintf(head, sizeof head, "case %s, cell %s:", t->case_name, name);
        } else if (fixed) {
            r = tj_judge_cell(t, cell, faulty ? TC_DEVIATED : TC_PLAIN, &obs[i], &m);
            snprintf(head, sizeof head, "case %s, cell %s%s:", t->case_name, name,
                     faulty ? " (the original's faulty part, its declared deviations)" : "");
        } else {
            r = tj_judge_cell(t, cell, faulty ? TC_DEVIATED : TC_DEVIATED_ANY, &obs[i], &m);
            snprintf(head, sizeof head, "case %s, cell %s (an open bug: its declared deviations):", t->case_name,
                     name);
            if (r == TJ_FAIL && !faulty) {
                struct tj_msgs plain = { 0 };
                int p = tj_judge_cell(t, cell, TC_PLAIN, &obs[i], &plain);

                tj_msgs_free(&plain);
                if (p < 0) {
                    tj_msgs_free(&m);
                    return -1;
                }
                if (p == TJ_PASS) {
                    tj_msgs_free(&m);
                    if (msg(&m, "the case passes without the declared deviations, but the test has no fixed "
                                "issue (fixed-orig, or extension)"))
                        return -1;
                }
            }
        }
        if (r < 0) {
            tj_msgs_free(&m);
            return -1;
        }
        if (r != TJ_PASS && append(msgs, &m, head))
            return -1;
        tj_msgs_free(&m);
        any_error |= r == TJ_ERROR;
        any_broken |= r == TJ_BROKEN;
        any_fail |= r == TJ_FAIL;
    }
    if (any_error)
        return TJ_V_ERROR;
    if (t->require_deviation && !t->conf->ndeviations) {
        if (msg(msgs, "%s: the test declares no deviation of the original, so it does not show its bug "
                      "(a bug-orig test needs [deviations.\"SELECTOR\"] in %s.test.toml)",
                t->file, t->conf->id))
            return -1;
        return TJ_V_BROKEN;
    }
    if (any_broken)
        return TJ_V_BROKEN;
    if (any_fail)
        return TJ_V_FAILED;
    return dev && !fixed ? TJ_V_REPRODUCED : TJ_V_PASSED;
}
