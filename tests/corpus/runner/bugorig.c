/*
 * bugorig.c - the verdict of a bug-orig test, see bugorig.h.
 */
#define _XOPEN_SOURCE 700       /* strdup, strndup */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bugorig.h"

static const char *const names[] = { "session", "compile", "run", "blitz2", "log" };
#define NNAMES (sizeof names / sizeof names[0])

void bugorig_free(struct bugorig_obs *o)
{
    size_t i;

    for (i = 0; i < o->n; i++) {
        free(o->sec[i].name);
        free(o->sec[i].text);
    }
    free(o->sec);
    o->sec = NULL;
    o->n = 0;
}

const char *bugorig_get(const struct bugorig_obs *o, const char *name)
{
    size_t i;

    for (i = 0; i < o->n; i++)
        if (!strcmp(o->sec[i].name, name))
            return o->sec[i].text;
    return NULL;
}

static int add(struct bugorig_obs *o, const char *name, size_t nlen, const char *text, size_t tlen)
{
    struct bugorig_section *s = realloc(o->sec, (o->n + 1) * sizeof *s);

    if (!s)
        return -1;
    o->sec = s;
    s += o->n;
    s->name = strndup(name, nlen);
    s->text = strndup(text, tlen);
    if (!s->name || !s->text) {
        free(s->name);
        free(s->text);
        return -1;
    }
    o->n++;
    return 0;
}

static int fail(struct bugorig_obs *o, char *err, size_t errlen, unsigned line, const char *msg)
{
    snprintf(err, errlen, "line %u: %s", line, msg);
    bugorig_free(o);
    return -1;
}

/* [name], name of a-z and 0-9: a section header (known or not) */
static int header(const char *p, size_t n)
{
    size_t i;

    if (n < 3 || p[0] != '[' || p[n - 1] != ']')
        return 0;
    for (i = 1; i + 1 < n; i++)
        if ((p[i] < 'a' || p[i] > 'z') && (p[i] < '0' || p[i] > '9'))
            return 0;
    return 1;
}

int bugorig_parse(const char *text, struct bugorig_obs *o, char *err, size_t errlen)
{
    struct text cur = { 0 };
    const char *p = text, *name = NULL;
    size_t nlen = 0, k;
    unsigned no = 0;

    memset(o, 0, sizeof *o);
    for (;;) {
        const char *eol = strchr(p, '\n');
        size_t n = eol ? (size_t)(eol - p) : strlen(p);

        if (!*p || header(p, n)) {
            if (name) {
                if (add(o, name, nlen, cur.p ? cur.p : "", cur.n)) {
                    free(cur.p);
                    return fail(o, err, errlen, no, "out of memory");
                }
                free(cur.p);
                memset(&cur, 0, sizeof cur);
            }
            if (!*p)
                break;
            name = p + 1;
            nlen = n - 2;
            no++;
            for (k = 0; k < NNAMES; k++)
                if (strlen(names[k]) == nlen && !strncmp(names[k], name, nlen))
                    break;
            if (k == NNAMES) {
                free(cur.p);
                return fail(o, err, errlen, no, "unknown section (session, compile, run, blitz2, log)");
            }
            for (k = 0; k < o->n; k++)
                if (strlen(o->sec[k].name) == nlen && !strncmp(o->sec[k].name, name, nlen)) {
                    free(cur.p);
                    return fail(o, err, errlen, no, "section twice");
                }
        } else {
            no++;
            if (!name) {
                if (n) {
                    free(cur.p);
                    return fail(o, err, errlen, no, "text before the first [section]");
                }
            } else
                text_add(&cur, "%.*s\n", (int)n, p);
        }
        p = eol ? eol + 1 : p + n;
    }
    free(cur.p);
    if (cur.bad)
        return fail(o, err, errlen, no, "out of memory");
    if (!o->n)
        return fail(o, err, errlen, no, "no section");
    return 0;
}

/* ---------------------------------------------------------- observing */

/* does line (n chars) occur as a whole line of text */
static int has_line(const char *text, const char *line, size_t n)
{
    const char *p = text;

    while (p && *p) {
        const char *eol = strchr(p, '\n');
        size_t m = eol ? (size_t)(eol - p) : strlen(p);

        if (m && p[m - 1] == '\r')
            m--;
        if (m == n && !memcmp(p, line, n))
            return 1;
        p = eol ? eol + 1 : NULL;
    }
    return 0;
}

/* the fields of a RESULT line behind the id (tab separated) */
static void compile_of(const char *log, const char *job, struct text *t)
{
    char key[40];
    const char *s, *e, *f[4] = { "" };
    size_t len[4] = { 0 }, n = 0;

    snprintf(key, sizeof key, "RESULT\t%s\t", job);
    for (s = log; s && (s = strstr(s, key)); s++)
        if (s == log || s[-1] == '\n')
            break;
    if (!s) {
        text_add(t, "none\n");
        return;
    }
    s += strlen(key);
    e = s + strcspn(s, "\r\n");
    while (n < 4 && s <= e) {
        const char *tab = memchr(s, '\t', (size_t)(e - s));

        f[n] = s;
        len[n] = tab ? (size_t)(tab - s) : (size_t)(e - s);
        n++;
        if (!tab)
            break;
        s = tab + 1;
    }
    if (len[0] == 2 && !memcmp(f[0], "ok", 2))
        text_add(t, "ok\n");                    /* the size depends on the libraries */
    else if (len[0] == 5 && !memcmp(f[0], "error", 5) && n >= 3)
        text_add(t, "error %.*s %.*s\n", (int)len[1], f[1], (int)len[2], f[2]);
    else
        text_add(t, "%.*s%s%.*s\n", (int)len[0], f[0], n > 1 ? " " : "",
                 n > 1 ? (int)len[1] : 0, n > 1 ? f[1] : "");
}

/* text as lines: \r removed, a last line without \n completed; lines
   that occur in except (may be NULL) left out */
static void lines_of(const char *text, struct text *t, const char *except)
{
    const char *p = text;

    while (p && *p) {
        const char *eol = strchr(p, '\n');
        size_t m = eol ? (size_t)(eol - p) : strlen(p);

        if (m && p[m - 1] == '\r')
            m--;
        if (!except || !has_line(except, p, m))
            text_add(t, "%.*s\n", (int)m, p);
        p = eol ? eol + 1 : NULL;
    }
}

/* the log with the size of a successful RESULT removed */
/* a line of the version requester ("Blitz Basic 2 Version 1.95"): added
   with the version number as N.NN, so that the reference (1.95) and the
   port (2.10) show the same line; 0 = not such a line */
static int version_line(struct text *t, const char *p, size_t m)
{
    static const char tag[] = "\"Blitz Basic 2 Version ";
    const char *v = NULL, *q, *end = p + m;

    for (q = p; (size_t)(end - q) >= sizeof tag - 1; q++)
        if (!memcmp(q, tag, sizeof tag - 1)) {
            v = q + sizeof tag - 1;
            break;
        }
    if (!v)
        return 0;
    for (q = v; q < end && *q != '"'; q++)
        ;
    if (q == end)
        return 0;
    text_add(t, "%.*sN.NN%.*s\n", (int)(v - p), p, (int)(end - q), q);
    return 1;
}

static void log_of(const char *log, struct text *t)
{
    const char *p = log;

    while (p && *p) {
        const char *eol = strchr(p, '\n');
        size_t m = eol ? (size_t)(eol - p) : strlen(p);
        const char *ok;

        if (m && p[m - 1] == '\r')
            m--;
        if (m > 7 && !memcmp(p, "RESULT\t", 7) && (ok = memchr(p + 7, '\t', m - 7))
            && (size_t)(p + m - ok) > 3 && !memcmp(ok, "\tok\t", 4))
            m = (size_t)(ok + 3 - p);
        if (!version_line(t, p, m))
            text_add(t, "%.*s\n", (int)m, p);
        p = eol ? eol + 1 : NULL;
    }
}

int bugorig_observe(const struct bugorig_outputs *out, struct bugorig_obs *o)
{
    struct text t[NNAMES];
    size_t k;
    int r = 0;

    memset(o, 0, sizeof *o);
    memset(t, 0, sizeof t);
    text_add(&t[0], "%s\n", out->session ? out->session : "failed");
    compile_of(out->log, out->job ? out->job : "j000", &t[1]);
    lines_of(out->run, &t[2], NULL);
    lines_of(out->blitz2, &t[3], out->log);
    log_of(out->log, &t[4]);
    for (k = 0; k < NNAMES; k++) {
        if (t[k].bad || add(o, names[k], strlen(names[k]), t[k].p ? t[k].p : "", t[k].n))
            r = -1;
        free(t[k].p);
    }
    if (r)
        bugorig_free(o);
    return r;
}

/* ------------------------------------------------------------- judging */

/* expected lines against observed lines: a line ending in '*' matches
   every line that begins with the text before it, a line "..." any
   number of lines (also none) */
static int text_matches(const char *want, const char *got)
{
    while (*want) {
        const char *we = strchr(want, '\n'), *ge;
        size_t wn = we ? (size_t)(we - want) : strlen(want), gn;

        if (wn == 3 && !memcmp(want, "...", 3)) {
            const char *rest = want + wn + (we != NULL);

            for (;;) {
                if (text_matches(rest, got))
                    return 1;
                if (!*got)
                    return 0;
                ge = strchr(got, '\n');
                got = ge ? ge + 1 : got + strlen(got);
            }
        }
        if (!*got)
            return 0;
        ge = strchr(got, '\n');
        gn = ge ? (size_t)(ge - got) : strlen(got);
        if (wn && want[wn - 1] == '*') {
            if (gn < wn - 1 || memcmp(want, got, wn - 1))
                return 0;
        } else if (wn != gn || memcmp(want, got, wn))
            return 0;
        want += wn + (we != NULL);
        got += gn + (ge != NULL);
    }
    return !*got;
}

static const char *expected(const struct bugorig_obs *e, const char *name)
{
    const char *s = bugorig_get(e, name);

    return s ? s : !strcmp(name, "session") ? "done\n" : NULL;
}

int bugorig_matches(const struct bugorig_obs *e, const struct bugorig_obs *o)
{
    size_t k;

    for (k = 0; k < NNAMES; k++) {
        const char *w = expected(e, names[k]), *g = bugorig_get(o, names[k]);

        if (w && !text_matches(w, g ? g : ""))
            return 0;
    }
    return 1;
}

/* o and ref the same in the sections of the expectation */
static int same_as(const struct bugorig_obs *e, const struct bugorig_obs *ref,
                   const struct bugorig_obs *o)
{
    size_t k;

    for (k = 0; k < NNAMES; k++) {
        const char *a = bugorig_get(ref, names[k]), *b = bugorig_get(o, names[k]);

        if (expected(e, names[k]) && strcmp(a ? a : "", b ? b : ""))
            return 0;
    }
    return 1;
}

static void indent(struct text *why, const char *label, const char *s)
{
    const char *p = s && *s ? s : "(empty)\n";

    text_add(why, "    %s:\n", label);
    while (*p) {
        size_t m = strcspn(p, "\n");

        text_add(why, "      %.*s\n", (int)m, p);
        p += m + (p[m] == '\n');
    }
}

/* the sections of e in which o does not match, indented */
static void explain(const struct bugorig_obs *e, const struct bugorig_obs *o, const char *tag,
                    struct text *why)
{
    size_t k;

    for (k = 0; k < NNAMES; k++) {
        const char *w = expected(e, names[k]), *g = bugorig_get(o, names[k]);

        if (!w || text_matches(w, g ? g : ""))
            continue;
        text_add(why, "  [%s]\n", names[k]);
        indent(why, "expected (correct)", w);
        indent(why, tag, g);
    }
}

enum bugorig_verdict bugorig_judge(const struct bugorig_obs *e, const struct bugorig_obs *ref,
                                   const struct bugorig_obs *o, struct text *why)
{
    /* a fixed test: only the expectation counts */
    if (!ref) {
        if (bugorig_matches(e, o))
            return BUGORIG_FIXED;
        if (why)
            explain(e, o, "observed", why);
        return BUGORIG_REGRESSED;
    }
    if (bugorig_matches(e, ref)) {
        if (why)
            text_add(why, "  the original shows the expected (correct) behaviour: either it does "
                     "not have the bug or the expectation describes the bug\n");
        return BUGORIG_BROKEN;
    }
    if (bugorig_matches(e, o))
        return BUGORIG_FIXED;
    if (same_as(e, ref, o)) {
        if (why)
            explain(e, o, "observed (as the original)", why);
        return BUGORIG_REPRODUCED;
    }
    if (why) {
        explain(e, o, "observed", why);
        explain(e, ref, "the original", why);
    }
    return BUGORIG_DIFFERS;
}

enum bugorig_verdict bugorig_judge_regression(const struct bugorig_obs *e,
                                              const struct bugorig_obs *ref,
                                              const struct bugorig_obs *o, struct text *why)
{
    if (!bugorig_matches(e, ref)) {
        if (why) {
            text_add(why, "  the original does not show the expected behaviour: the expectation "
                     "is not what the original does\n");
            explain(e, ref, "the original", why);
        }
        return BUGORIG_BROKEN;
    }
    if (bugorig_matches(e, o))
        return BUGORIG_FIXED;
    if (why)
        explain(e, o, "observed", why);
    return BUGORIG_DIFFERS;
}

const char *bugorig_verdict_name(enum bugorig_verdict v)
{
    switch (v) {
    case BUGORIG_FIXED: return "FIXED";
    case BUGORIG_REPRODUCED: return "REPRODUCED";
    case BUGORIG_DIFFERS: return "DIFFERS";
    case BUGORIG_REGRESSED: return "REGRESSED";
    default: return "BROKEN";
    }
}
