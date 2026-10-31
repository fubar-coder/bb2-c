/*
 * junit.c - reading ctest's JUnit XML files, see junit.h.
 */
#define _POSIX_C_SOURCE 200809L /* strdup */
#include "junit.h"
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXDEPTH 64
#define MAXATTRS 32

/* ------------------------------------------------------------- helpers */

struct sb {
    char *p;
    size_t n, cap;
    int bad;
};

static void sb_add(struct sb *b, const char *s, size_t n)
{
    if (b->bad)
        return;
    if (b->n + n + 1 > b->cap) {
        size_t cap = b->cap ? b->cap : 64;
        char *p;

        while (cap < b->n + n + 1)
            cap *= 2;
        if (!(p = realloc(b->p, cap))) {
            b->bad = 1;
            return;
        }
        b->p = p;
        b->cap = cap;
    }
    memcpy(b->p + b->n, s, n);
    b->n += n;
    b->p[b->n] = 0;
}

/* the text, "" if empty; NULL if memory ran out (b freed) */
static char *sb_take(struct sb *b)
{
    char *p = b->bad ? NULL : b->p ? b->p : strdup("");

    if (b->bad)
        free(b->p);
    memset(b, 0, sizeof *b);
    return p;
}

struct attr {
    char name[64];
    char *value;
};

struct parser {
    const char *s;
    size_t len, pos;
    size_t line_pos;            /* line counting: up to line_pos, line_no lines */
    unsigned line_no;
    char *err;
    size_t errlen;
    struct junit_file *f;
    /* the testcase being read */
    int in_case;
    unsigned case_line, case_depth;
    char *c_name, *c_time, *c_status;
    int c_failed, c_skipped;
    struct sb out;
    unsigned out_depth;         /* depth of <system-out>, 0 = not in one */
    int nomem;
};

static unsigned line_at(struct parser *P, size_t pos)
{
    if (pos < P->line_pos) {
        P->line_pos = 0;
        P->line_no = 1;
    }
    for (; P->line_pos < pos && P->line_pos < P->len; P->line_pos++)
        if (P->s[P->line_pos] == '\n')
            P->line_no++;
    return P->line_no;
}

static int fail_at(struct parser *P, size_t pos, const char *fmt, ...)
{
    char msg[200];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(msg, sizeof msg, fmt, ap);
    va_end(ap);
    if (P->errlen)
        snprintf(P->err, P->errlen, "line %u: %s", line_at(P, pos), msg);
    return -1;
}

static int problem(struct parser *P, const char *fmt, ...)
{
    char msg[300], **v;
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(msg, sizeof msg, fmt, ap);
    va_end(ap);
    if (!(v = realloc(P->f->problems, (P->f->nproblems + 1) * sizeof *v))
        || !(v[P->f->nproblems] = strdup(msg))) {
        if (v)
            P->f->problems = v;
        P->nomem = 1;
        return -1;
    }
    P->f->problems = v;
    P->f->nproblems++;
    return 0;
}

static int starts(const struct parser *P, const char *lit)
{
    size_t n = strlen(lit);

    return P->len - P->pos >= n && !memcmp(P->s + P->pos, lit, n);
}

/* moves past the next `lit`; -1 if there is none */
static int skip_past(struct parser *P, const char *lit)
{
    size_t n = strlen(lit);

    for (; P->pos + n <= P->len; P->pos++)
        if (!memcmp(P->s + P->pos, lit, n)) {
            P->pos += n;
            return 0;
        }
    return -1;
}

static int space(int c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static int name_start(int c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == ':' || c >= 0x80;
}

static int name_char(int c)
{
    return name_start(c) || (c >= '0' && c <= '9') || c == '-' || c == '.';
}

/* a name at P->pos into buf (cut to size); its length, 0 if none */
static size_t read_name(struct parser *P, char *buf, size_t size)
{
    size_t start = P->pos, n;

    if (P->pos >= P->len || !name_start((unsigned char)P->s[P->pos]))
        return 0;
    while (P->pos < P->len && name_char((unsigned char)P->s[P->pos]))
        P->pos++;
    n = P->pos - start;
    snprintf(buf, size, "%.*s", (int)(n < size ? n : size - 1), P->s + start);
    return n;
}

static void put_utf8(struct sb *b, unsigned long c)
{
    char u[4];

    if (c < 0x80) {
        u[0] = (char)c;
        sb_add(b, u, 1);
    } else if (c < 0x800) {
        u[0] = (char)(0xC0 | (c >> 6));
        u[1] = (char)(0x80 | (c & 0x3F));
        sb_add(b, u, 2);
    } else if (c < 0x10000) {
        u[0] = (char)(0xE0 | (c >> 12));
        u[1] = (char)(0x80 | ((c >> 6) & 0x3F));
        u[2] = (char)(0x80 | (c & 0x3F));
        sb_add(b, u, 3);
    } else {
        u[0] = (char)(0xF0 | (c >> 18));
        u[1] = (char)(0x80 | ((c >> 12) & 0x3F));
        u[2] = (char)(0x80 | ((c >> 6) & 0x3F));
        u[3] = (char)(0x80 | (c & 0x3F));
        sb_add(b, u, 4);
    }
}

/* the characters s[from..to) with their entities resolved, appended to
   out; -1 on a bad entity or a '<' (in_attr: also the quote) */
static int unescape(struct parser *P, size_t from, size_t to, struct sb *out)
{
    static const struct { const char *name; char c; } named[] = {
        { "lt", '<' }, { "gt", '>' }, { "amp", '&' }, { "quot", '"' }, { "apos", '\'' } };
    size_t i = from, run = from;

    while (i < to) {
        if (P->s[i] == '<')
            return fail_at(P, i, "'<' in text or a value");
        if (P->s[i] != '&') {
            i++;
            continue;
        }
        sb_add(out, P->s + run, i - run);
        {
            size_t e = i + 1, k;

            while (e < to && e - i < 12 && P->s[e] != ';')
                e++;
            if (e >= to || P->s[e] != ';')
                return fail_at(P, i, "'&' without entity");
            if (P->s[i + 1] == '#') {
                int hex = e > i + 2 && (P->s[i + 2] == 'x' || P->s[i + 2] == 'X');
                unsigned long c = 0;

                k = i + 2 + (size_t)hex;
                if (k == e)
                    return fail_at(P, i, "empty character reference");
                for (; k < e; k++) {
                    int ch = (unsigned char)P->s[k], d;

                    d = ch >= '0' && ch <= '9' ? ch - '0'
                        : hex && ch >= 'a' && ch <= 'f' ? ch - 'a' + 10
                        : hex && ch >= 'A' && ch <= 'F' ? ch - 'A' + 10 : -1;
                    if (d < 0)
                        return fail_at(P, i, "bad character reference");
                    c = c * (hex ? 16 : 10) + (unsigned long)d;
                    if (c > 0x10FFFF)
                        return fail_at(P, i, "character reference out of range");
                }
                if (!c || (c >= 0xD800 && c <= 0xDFFF))
                    return fail_at(P, i, "character reference out of range");
                put_utf8(out, c);
            } else {
                for (k = 0; k < sizeof named / sizeof named[0]; k++)
                    if (strlen(named[k].name) == e - i - 1
                        && !memcmp(P->s + i + 1, named[k].name, e - i - 1))
                        break;
                if (k == sizeof named / sizeof named[0])
                    return fail_at(P, i, "unknown entity &%.*s;", (int)(e - i - 1), P->s + i + 1);
                sb_add(out, &named[k].c, 1);
            }
            i = run = e + 1;
        }
    }
    sb_add(out, P->s + run, to - run);
    if (out->bad) {
        P->nomem = 1;
        return fail_at(P, from, "out of memory");
    }
    return 0;
}

/* ---------------------------------------------------------- the model */

static const char *attr_value(const struct attr *a, size_t n, const char *name)
{
    size_t i;

    for (i = 0; i < n; i++)
        if (!strcmp(a[i].name, name))
            return a[i].value;
    return NULL;
}

static void case_reset(struct parser *P)
{
    free(P->c_name);
    free(P->c_time);
    free(P->c_status);
    free(P->out.p);
    P->c_name = P->c_time = P->c_status = NULL;
    memset(&P->out, 0, sizeof P->out);
    P->c_failed = P->c_skipped = 0;
    P->in_case = 0;
    P->out_depth = 0;
}

static char *dup_or_null(const char *s)
{
    return s ? strdup(s) : NULL;
}

static int on_start(struct parser *P, const char *name, const struct attr *a, size_t na,
                    unsigned depth, size_t pos)
{
    if (!strcmp(name, "testsuite") && !P->in_case) {
        const char *ts = attr_value(a, na, "timestamp");

        if (ts && !*P->f->timestamp) {
            char *t = strdup(ts);

            if (!t)
                return P->nomem = 1, fail_at(P, pos, "out of memory");
            free(P->f->timestamp);
            P->f->timestamp = t;
        }
    } else if (!strcmp(name, "testcase") && !P->in_case) {
        const char *n = attr_value(a, na, "name"), *t = attr_value(a, na, "time");
        const char *s = attr_value(a, na, "status");

        P->in_case = 1;
        P->case_line = line_at(P, pos);
        P->case_depth = depth;
        if ((n && !(P->c_name = strdup(n))) || (t && !(P->c_time = strdup(t)))
            || (s && !(P->c_status = dup_or_null(s))))
            return P->nomem = 1, fail_at(P, pos, "out of memory");
    } else if (P->in_case && !P->out_depth) {
        if (!strcmp(name, "failure") || !strcmp(name, "error"))
            P->c_failed = 1;
        else if (!strcmp(name, "skipped"))
            P->c_skipped = 1;
        else if (!strcmp(name, "system-out"))
            P->out_depth = depth;
    }
    return 0;
}

static int on_end(struct parser *P, const char *name, unsigned depth, size_t pos)
{
    struct junit_file *f = P->f;

    if (P->out_depth == depth && !strcmp(name, "system-out"))
        P->out_depth = 0;
    if (!P->in_case || depth != P->case_depth)
        return 0;
    {
        char *end = NULL;
        double t = 0;
        struct junit_case *v;
        char *out;

        if (!P->c_name || !*P->c_name) {
            problem(P, "line %u: a testcase without name - left out", P->case_line);
            case_reset(P);
            return P->nomem ? fail_at(P, pos, "out of memory") : 0;
        }
        if (P->c_time) {
            errno = 0;
            t = strtod(P->c_time, &end);
        }
        if (!P->c_time || !*P->c_time || errno || *end || !isfinite(t) || t < 0) {
            problem(P, "line %u: testcase %s: %s - left out", P->case_line, P->c_name,
                    P->c_time ? "time is not a number of seconds" : "no time");
            case_reset(P);
            return P->nomem ? fail_at(P, pos, "out of memory") : 0;
        }
        if (!(v = realloc(f->cases, (f->ncases + 1) * sizeof *v)))
            return P->nomem = 1, fail_at(P, pos, "out of memory");
        f->cases = v;
        if (!(out = sb_take(&P->out)))
            return P->nomem = 1, fail_at(P, pos, "out of memory");
        v[f->ncases].name = P->c_name;
        v[f->ncases].time = t;
        v[f->ncases].out = out;
        v[f->ncases].status =
            P->c_failed ? JUNIT_FAILED
            : P->c_skipped || (P->c_status && (!strcmp(P->c_status, "notrun")
                                               || !strcmp(P->c_status, "disabled")))
              ? JUNIT_SKIPPED
            : !P->c_status || !strcmp(P->c_status, "run") ? JUNIT_PASSED : JUNIT_FAILED;
        f->ncases++;
        P->c_name = NULL;
        case_reset(P);
    }
    return 0;
}

/* ---------------------------------------------------------- the reader */

static int text(struct parser *P, size_t from, size_t to, unsigned depth, int cdata)
{
    size_t i;

    if (!depth) {
        for (i = from; i < to; i++)
            if (!space((unsigned char)P->s[i]))
                return fail_at(P, i, "text outside the root element");
        return 0;
    }
    if (!P->out_depth) {
        /* not kept, but checked */
        struct sb tmp = { 0 };
        int r = cdata ? 0 : unescape(P, from, to, &tmp);

        free(tmp.p);
        return r;
    }
    if (cdata) {
        sb_add(&P->out, P->s + from, to - from);
        return P->out.bad ? (P->nomem = 1, fail_at(P, from, "out of memory")) : 0;
    }
    return unescape(P, from, to, &P->out);
}

/* a start tag at P->pos ('<' + name) */
static int start_tag(struct parser *P, char stack[][64], unsigned *depth, int *root_seen)
{
    size_t tag = P->pos, na = 0, i;
    struct attr a[MAXATTRS];
    char name[64];
    int empty = 0, rc = -1;

    P->pos++;
    if (!read_name(P, name, sizeof name))
        return fail_at(P, tag, "'<' without a name");
    if (!*depth && *root_seen)
        return fail_at(P, tag, "a second root element <%s>", name);
    for (;;) {
        size_t sp = P->pos, vs;
        char q;
        struct sb v = { 0 };

        while (P->pos < P->len && space((unsigned char)P->s[P->pos]))
            P->pos++;
        if (P->pos >= P->len) {
            fail_at(P, tag, "<%s not closed", name);
            goto out;
        }
        if (starts(P, "/>")) {
            P->pos += 2;
            empty = 1;
            break;
        }
        if (P->s[P->pos] == '>') {
            P->pos++;
            break;
        }
        if (sp == P->pos) {
            fail_at(P, P->pos, "<%s: no blank before an attribute", name);
            goto out;
        }
        if (na == MAXATTRS) {
            fail_at(P, P->pos, "<%s: too many attributes", name);
            goto out;
        }
        if (!read_name(P, a[na].name, sizeof a[na].name)) {
            fail_at(P, P->pos, "<%s: bad attribute", name);
            goto out;
        }
        while (P->pos < P->len && space((unsigned char)P->s[P->pos]))
            P->pos++;
        if (P->pos >= P->len || P->s[P->pos] != '=') {
            fail_at(P, P->pos, "<%s: attribute %s without value", name, a[na].name);
            goto out;
        }
        P->pos++;
        while (P->pos < P->len && space((unsigned char)P->s[P->pos]))
            P->pos++;
        if (P->pos >= P->len || (P->s[P->pos] != '"' && P->s[P->pos] != '\'')) {
            fail_at(P, P->pos, "<%s: value of %s not quoted", name, a[na].name);
            goto out;
        }
        q = P->s[P->pos++];
        vs = P->pos;
        while (P->pos < P->len && P->s[P->pos] != q)
            P->pos++;
        if (P->pos >= P->len) {
            fail_at(P, vs, "<%s: value of %s not closed", name, a[na].name);
            goto out;
        }
        if (unescape(P, vs, P->pos, &v) || !(a[na].value = sb_take(&v))) {
            free(v.p);
            if (!P->nomem && !*P->err)
                fail_at(P, vs, "out of memory");
            goto out;
        }
        P->pos++;
        for (i = 0; i < na && strcmp(a[i].name, a[na].name); i++)
            ;
        na++;
        if (i < na - 1) {
            fail_at(P, tag, "<%s: attribute %s twice", name, a[na - 1].name);
            goto out;
        }
    }
    if (!*depth)
        *root_seen = 1;
    if (*depth == MAXDEPTH) {
        fail_at(P, tag, "elements nested too deep");
        goto out;
    }
    if (on_start(P, name, a, na, *depth + 1, tag))
        goto out;
    if (empty)
        rc = on_end(P, name, *depth + 1, tag);
    else {
        snprintf(stack[(*depth)++], 64, "%s", name);
        rc = 0;
    }
out:
    for (i = 0; i < na; i++)
        free(a[i].value);
    return rc;
}

int junit_parse(const char *s, size_t len, struct junit_file *f, char *err, size_t errlen)
{
    struct parser P = { 0 };
    char stack[MAXDEPTH][64];
    unsigned depth = 0;
    int root_seen = 0, rc = -1;

    memset(f, 0, sizeof *f);
    if (errlen)
        *err = 0;
    P.s = s;
    P.len = len;
    P.line_no = 1;
    P.err = errlen ? err : (char[1]){ 0 };
    P.errlen = errlen;
    P.f = f;
    if (!(f->timestamp = strdup("")))
        goto nomem;
    if (len >= 3 && !memcmp(s, "\xEF\xBB\xBF", 3))
        P.pos = 3;
    while (P.pos < len) {
        size_t at = P.pos;

        if (s[P.pos] != '<') {
            const char *lt = memchr(s + P.pos, '<', len - P.pos);
            size_t to = lt ? (size_t)(lt - s) : len;

            if (text(&P, P.pos, to, depth, 0))
                goto out;
            P.pos = to;
        } else if (starts(&P, "<?")) {
            if (skip_past(&P, "?>")) {
                fail_at(&P, at, "<? not closed");
                goto out;
            }
        } else if (starts(&P, "<!--")) {
            if (skip_past(&P, "-->")) {
                fail_at(&P, at, "comment not closed");
                goto out;
            }
        } else if (starts(&P, "<![CDATA[")) {
            size_t from = P.pos + 9;

            P.pos = from;
            if (skip_past(&P, "]]>")) {
                fail_at(&P, at, "CDATA not closed");
                goto out;
            }
            if (!depth) {
                fail_at(&P, at, "CDATA outside the root element");
                goto out;
            }
            if (text(&P, from, P.pos - 3, depth, 1))
                goto out;
        } else if (starts(&P, "<!")) {
            if (depth || root_seen) {
                fail_at(&P, at, "<! inside the document");
                goto out;
            }
            while (P.pos < len && s[P.pos] != '>' && s[P.pos] != '[')
                P.pos++;
            if (P.pos >= len || s[P.pos] == '[') {
                fail_at(&P, at, P.pos < len ? "DOCTYPE with internal subset" : "<! not closed");
                goto out;
            }
            P.pos++;
        } else if (starts(&P, "</")) {
            char name[64];

            P.pos += 2;
            if (!read_name(&P, name, sizeof name)) {
                fail_at(&P, at, "'</' without a name");
                goto out;
            }
            while (P.pos < len && space((unsigned char)s[P.pos]))
                P.pos++;
            if (P.pos >= len || s[P.pos] != '>') {
                fail_at(&P, at, "</%s not closed", name);
                goto out;
            }
            P.pos++;
            if (!depth || strcmp(stack[depth - 1], name)) {
                if (depth)
                    fail_at(&P, at, "</%s> does not close <%s>", name, stack[depth - 1]);
                else
                    fail_at(&P, at, "</%s> without <%s>", name, name);
                goto out;
            }
            if (on_end(&P, name, depth, at))
                goto out;
            depth--;
        } else if (start_tag(&P, stack, &depth, &root_seen)) {
            goto out;
        }
    }
    if (depth) {
        fail_at(&P, len, "<%s> not closed at the end", stack[depth - 1]);
        goto out;
    }
    if (!root_seen) {
        fail_at(&P, len, "no element");
        goto out;
    }
    if (P.nomem)
        goto nomem;
    rc = 0;
    goto out;
nomem:
    if (errlen)
        snprintf(err, errlen, "out of memory");
out:
    case_reset(&P);
    if (rc)
        junit_free(f);
    return rc;
}

void junit_free(struct junit_file *f)
{
    size_t i;

    for (i = 0; i < f->ncases; i++) {
        free(f->cases[i].name);
        free(f->cases[i].out);
    }
    for (i = 0; i < f->nproblems; i++)
        free(f->problems[i]);
    free(f->cases);
    free(f->problems);
    free(f->timestamp);
    memset(f, 0, sizeof *f);
}
