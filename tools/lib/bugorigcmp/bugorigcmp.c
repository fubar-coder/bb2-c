/*
 * bugorigcmp.c - the observation files of the bug-orig tests: parsing,
 * the wildcard lines with a reason, matching (bugorigcmp.h).
 */
#define _DEFAULT_SOURCE
#include <stdlib.h>
#include <string.h>
#include "bugorigcmp.h"
#include "bugorigcmp_int.h"

/* --------------------------------------------------------------- strings */

int bstr_add(struct bstr *s, const char *p, size_t n)
{
    if (s->bad)
        return -1;
    if (s->n + n + 1 > s->cap) {
        size_t cap = s->cap ? s->cap : 64;
        char *q;

        while (cap < s->n + n + 1)
            cap *= 2;
        if (!(q = realloc(s->p, cap))) {
            s->bad = 1;
            return -1;
        }
        s->p = q;
        s->cap = cap;
    }
    memcpy(s->p + s->n, p, n);
    s->n += n;
    s->p[s->n] = 0;
    return 0;
}

char *bstr_take(struct bstr *s)
{
    char *p = s->p;

    if (s->bad) {
        free(p);
        p = NULL;
    } else if (!p)
        p = calloc(1, 1);
    memset(s, 0, sizeof *s);
    return p;
}

/* ----------------------------------------------------------------- lines */

static const char varies[] = "# varies:";

/* the line at p: its length without the LF */
static size_t line_len(const char *p)
{
    const char *e = strchr(p, '\n');

    return e ? (size_t)(e - p) : strlen(p);
}

/* the start of the next line behind the line at p of n bytes */
static const char *next_line(const char *p, size_t n)
{
    return p[n] ? p + n + 1 : p + n;
}

int bugorigcmp_is_wild(const char *line, size_t n)
{
    return (n && line[n - 1] == '*') || (n == 3 && !memcmp(line, "...", 3));
}

/* "# varies:" at the start: 1 = with a reason, -1 = without, 0 = not
   such a line */
static int varies_line(const char *p, size_t n)
{
    size_t k = sizeof varies - 1;

    if (n < k || memcmp(p, varies, k))
        return 0;
    while (k < n && (p[k] == ' ' || p[k] == '\t' || p[k] == '\r'))
        k++;
    return k < n ? 1 : -1;
}

/* [name], name of a-z and 0-9 */
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

/* ID.orig: the rule of the wildcard lines for the line at p (number no),
   *pending the line of a "# varies:" line before it (0: none); 1 = it is
   a "# varies:" line, 0 = another line, -2 = broken (err) */
static int varies_rule(const char *p, size_t n, unsigned no, unsigned *pending, const char *path,
                       char *err, size_t errlen)
{
    int v = varies_line(p, n);

    if (v < 0) {
        snprintf(err, errlen, "%s:%u: a \"# varies:\" line without a reason", path, no);
        return -2;
    }
    if (v > 0) {
        if (*pending) {
            snprintf(err, errlen, "%s:%u: the \"# varies:\" line is not followed by a "
                     "wildcard line", path, *pending);
            return -2;
        }
        *pending = no;
        return 1;
    }
    if (bugorigcmp_is_wild(p, n)) {
        if (!*pending) {
            snprintf(err, errlen, "%s:%u: a wildcard line (`*` at the end, `...`) without a "
                     "\"# varies: REASON\" line before it", path, no);
            return -2;
        }
    } else if (*pending) {
        snprintf(err, errlen, "%s:%u: the \"# varies:\" line is not followed by a wildcard line",
                 path, *pending);
        return -2;
    }
    *pending = 0;
    return 0;
}

int bugorigcmp_check_varies(const char *text, const char *path, char *err, size_t errlen)
{
    const char *p = text;
    unsigned no = 0, pending = 0;
    int in = 0;

    while (*p) {
        size_t n = line_len(p);

        no++;
        if (header(p, n)) {
            if (pending) {
                snprintf(err, errlen, "%s:%u: the \"# varies:\" line is not followed by a "
                         "wildcard line", path, pending);
                return -2;
            }
            in = 1;
        } else if (in && varies_rule(p, n, no, &pending, path, err, errlen) < 0)
            return -2;
        p = next_line(p, n);
    }
    if (pending) {
        snprintf(err, errlen, "%s:%u: the \"# varies:\" line is not followed by a wildcard line",
                 path, pending);
        return -2;
    }
    return 0;
}

void bugorigcmp_strip_varies(char *text)
{
    char *p = text, *d = text;
    int in = 0;

    while (*p) {
        size_t n = line_len(p), m = p[n] ? n + 1 : n;

        if (header(p, n))
            in = 1;
        if (!in || !varies_line(p, n)) {
            memmove(d, p, m);
            d += m;
        }
        p += m;
    }
    *d = 0;
}

/* -------------------------------------------------------------- parsing */

void bugorigcmp_free(struct bugorigcmp_obs *o)
{
    size_t i;

    for (i = 0; i < o->n; i++) {
        free(o->sec[i].name);
        free(o->sec[i].text);
        free(o->sec[i].raw);
    }
    free(o->sec);
    free(o->note);
    memset(o, 0, sizeof *o);
}

const struct bugorigcmp_sec *bugorigcmp_get(const struct bugorigcmp_obs *o, const char *name)
{
    size_t i;

    for (i = 0; i < o->n; i++)
        if (!strcmp(o->sec[i].name, name))
            return &o->sec[i];
    return NULL;
}

static int fail(struct bugorigcmp_obs *o, int r, char *err, size_t errlen, const char *path,
                unsigned no, const char *msg)
{
    if (r == -1)
        snprintf(err, errlen, "out of memory");
    else
        snprintf(err, errlen, "%s:%u: %s", path, no, msg);
    bugorigcmp_free(o);
    return r;
}

int bugorigcmp_parse(const char *text, const char *path, enum bugorigcmp_kind kind,
                     struct bugorigcmp_obs *o, char *err, size_t errlen)
{
    const char *p = text;
    struct bstr t = { 0 }, raw = { 0 };
    struct bugorigcmp_sec *cur = NULL;
    unsigned no = 0, pending = 0;
    int r;

    memset(o, 0, sizeof *o);
    for (;;) {
        size_t n = *p ? line_len(p) : 0;

        if (!*p || header(p, n)) {
            if (cur) {
                if (pending) {
                    free(t.p);
                    free(raw.p);
                    snprintf(err, errlen, "%s:%u: the \"# varies:\" line is not followed by a "
                             "wildcard line", path, pending);
                    bugorigcmp_free(o);
                    return -2;
                }
                cur->text = bstr_take(&t);
                cur->raw = bstr_take(&raw);
                if (!cur->text || !cur->raw)
                    return fail(o, -1, err, errlen, path, no, NULL);
            }
            if (!*p)
                break;
            no++;
            {
                struct bugorigcmp_sec *s;
                size_t k;

                for (k = 0; k < o->n; k++)
                    if (strlen(o->sec[k].name) == n - 2 && !memcmp(o->sec[k].name, p + 1, n - 2))
                        return fail(o, -2, err, errlen, path, no, "section twice");
                if (!(s = realloc(o->sec, (o->n + 1) * sizeof *s)))
                    return fail(o, -1, err, errlen, path, no, NULL);
                o->sec = s;
                cur = &s[o->n++];
                memset(cur, 0, sizeof *cur);
                cur->line = no;
                if (!(cur->name = strndup(p + 1, n - 2)))
                    return fail(o, -1, err, errlen, path, no, NULL);
            }
        } else {
            no++;
            if (!cur) {
                if (n && *p == '#') {
                    if (!o->note) {
                        const char *q = p + 1;
                        size_t m = n - 1;

                        if (m && *q == ' ')
                            q++, m--;
                        if (!(o->note = strndup(q, m)))
                            return fail(o, -1, err, errlen, path, no, NULL);
                    }
                } else if (n)
                    return fail(o, -2, err, errlen, path, no, "text before the first [section]");
            } else {
                int v = 0;

                if (kind == BUGORIGCMP_ORIG
                    && (v = varies_rule(p, n, no, &pending, path, err, errlen)) < 0) {
                    free(t.p);
                    free(raw.p);
                    bugorigcmp_free(o);
                    return -2;
                }
                if (kind == BUGORIGCMP_ORIG && !v && bugorigcmp_is_wild(p, n))
                    cur->wild = 1;
                r = bstr_add(&raw, p, n) | bstr_add(&raw, "\n", 1);
                if (!v)
                    r |= bstr_add(&t, p, n) | bstr_add(&t, "\n", 1);
                if (r) {
                    free(t.p);
                    free(raw.p);
                    return fail(o, -1, err, errlen, path, no, NULL);
                }
            }
        }
        p = next_line(p, n);
    }
    if (!o->n)
        return fail(o, -2, err, errlen, path, no ? no : 1, "no section");
    return 0;
}

/* ------------------------------------------------------------- matching */

int bugorigcmp_matches(const char *want, const char *got)
{
    while (*want) {
        size_t wn = line_len(want), gn;

        if (wn == 3 && !memcmp(want, "...", 3)) {
            const char *rest = next_line(want, wn);

            for (;;) {
                if (bugorigcmp_matches(rest, got))
                    return 1;
                if (!*got)
                    return 0;
                got = next_line(got, line_len(got));
            }
        }
        if (!*got)
            return 0;
        gn = line_len(got);
        if (wn && want[wn - 1] == '*') {
            if (gn < wn - 1 || memcmp(want, got, wn - 1))
                return 0;
        } else if (wn != gn || memcmp(want, got, wn))
            return 0;
        want = next_line(want, wn);
        got = next_line(got, gn);
    }
    return !*got;
}

/* a line for messages: bytes outside 32..126 as \xNN, cut with ... */
void bugorigcmp_show(const char *p, size_t n, char *buf, size_t len, int end)
{
    size_t i, k = 0;

    if (!len)
        return;
    if (end) {
        snprintf(buf, len, "(the end)");
        return;
    }
    for (i = 0; i < n && k + 8 < len && k < 100; i++) {
        unsigned char c = (unsigned char)p[i];

        if (c < 32 || c > 126 || c == '"')
            k += (size_t)snprintf(buf + k, len - k, "\\x%02X", c);
        else
            buf[k++] = (char)c;
    }
    if (i < n && k + 4 < len) {
        memcpy(buf + k, "...", 3);
        k += 3;
    }
    buf[k < len ? k : len - 1] = 0;
}

unsigned bugorigcmp_first_diff(const char *want, const char *got, char *w, size_t wlen, char *g,
                               size_t glen)
{
    unsigned no = 1;

    if (bugorigcmp_matches(want, got))
        return 0;
    while (*want) {
        size_t wn = line_len(want), gn = *got ? line_len(got) : 0;
        int ok;

        if (wn == 3 && !memcmp(want, "...", 3))
            break;              /* behind it: no line to name, the place of ... */
        if (!*got)
            ok = 0;
        else if (wn && want[wn - 1] == '*')
            ok = gn >= wn - 1 && !memcmp(want, got, wn - 1);
        else
            ok = wn == gn && !memcmp(want, got, wn);
        if (!ok) {
            bugorigcmp_show(want, wn, w, wlen, 0);
            bugorigcmp_show(got, gn, g, glen, !*got);
            return no;
        }
        want = next_line(want, wn);
        got = next_line(got, gn);
        no++;
    }
    bugorigcmp_show(want, line_len(want), w, wlen, !*want);
    bugorigcmp_show(got, *got ? line_len(got) : 0, g, glen, !*got);
    return no;
}

int bugorigcmp_compare(const struct bugorigcmp_obs *orig, const struct bugorigcmp_obs *got,
                       char *msg, size_t msglen)
{
    char w[120], g[120];
    size_t i;

    for (i = 0; i < orig->n; i++) {
        const struct bugorigcmp_sec *s = &orig->sec[i], *o = bugorigcmp_get(got, s->name);
        unsigned no = bugorigcmp_first_diff(s->text, o ? o->text : "", w, sizeof w, g, sizeof g);

        if (no) {
            snprintf(msg, msglen, "[%s] line %u: recorded \"%s\", observed \"%s\"", s->name, no,
                     w, g);
            return 1;
        }
    }
    for (i = 0; i < got->n; i++)
        if (!bugorigcmp_get(orig, got->sec[i].name)) {
            snprintf(msg, msglen, "[%s]: observed, not in the recorded original",
                     got->sec[i].name);
            return 1;
        }
    return 0;
}
