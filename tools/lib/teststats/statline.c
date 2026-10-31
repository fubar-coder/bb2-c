/*
 * statline.c - the @stat line, see statline.h.
 */
#include "statline.h"
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

static int value_char(int c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')
           || (c && strchr("._,:/+-", c));
}

void statline_init(struct statline *s)
{
    memset(s, 0, sizeof *s);
    s->runs = -1;
}

void statline_set(char *field, size_t size, const char *value)
{
    if (size)
        snprintf(field, size, "%s", value ? value : "");
}

/* a value as written: the characters outside the set as '_', "" as "-" */
static void put_value(char *dst, size_t size, const char *v)
{
    size_t i;

    if (!*v)
        v = "-";
    for (i = 0; v[i] && i + 1 < size; i++)
        dst[i] = value_char((unsigned char)v[i]) ? v[i] : '_';
    dst[i] = 0;
}

int statline_format(const struct statline *s, char *buf, size_t len)
{
    char test[sizeof s->test], config[sizeof s->config], machine[sizeof s->machine];
    char snapshot[sizeof s->snapshot], end[sizeof s->end], verdict[sizeof s->verdict];
    char runs[24] = "";
    int n;

    put_value(test, sizeof test, s->test);
    put_value(config, sizeof config, s->config);
    put_value(machine, sizeof machine, s->machine);
    put_value(snapshot, sizeof snapshot, s->snapshot);
    put_value(end, sizeof end, s->end);
    put_value(verdict, sizeof verdict, s->verdict);
    if (s->runs >= 0)
        snprintf(runs, sizeof runs, " runs=%d", s->runs);
    n = snprintf(buf, len,
                 STATLINE_TAG " v=%d test=%s config=%s machine=%s snapshot=%s session=%.2f "
                 "wait=%.2f timed=%.2f limit=%u end=%s%s%s%s",
                 STATLINE_VERSION, test, config, machine, snapshot, s->session, s->wait, s->timed,
                 s->limit, end, *s->verdict ? " verdict=" : "", *s->verdict ? verdict : "", runs);
    return n < 0 || (size_t)n >= len ? -1 : n;
}

void statline_write(FILE *f, const struct statline *s)
{
    char buf[512];

    if (statline_format(s, buf, sizeof buf) >= 0)
        fprintf(f, "%s\n", buf);
    fflush(f);
}

static int fail(char *err, size_t errlen, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    if (errlen)
        vsnprintf(err, errlen, fmt, ap);
    va_end(ap);
    return -1;
}

/* seconds: a decimal number >= 0 */
static int seconds(const char *v, size_t n, double *out)
{
    char tmp[32], *end;
    double d;

    if (!n || n >= sizeof tmp || !strchr("0123456789", v[0]))
        return -1;
    memcpy(tmp, v, n);
    tmp[n] = 0;
    errno = 0;
    d = strtod(tmp, &end);
    if (errno || *end || !isfinite(d) || d < 0)
        return -1;
    *out = d;
    return 0;
}

static int whole(const char *v, size_t n, unsigned long max, unsigned long *out)
{
    unsigned long x = 0;
    size_t i;

    if (!n)
        return -1;
    for (i = 0; i < n; i++) {
        if (v[i] < '0' || v[i] > '9' || x > (max - (unsigned long)(v[i] - '0')) / 10)
            return -1;
        x = x * 10 + (unsigned long)(v[i] - '0');
    }
    *out = x;
    return 0;
}

int statline_parse(const char *line, size_t len, struct statline *s, char *err, size_t errlen)
{
    enum { K_V, K_TEST, K_CONFIG, K_MACHINE, K_SNAPSHOT, K_SESSION, K_WAIT, K_TIMED, K_LIMIT,
           K_END, K_VERDICT, K_RUNS, K_N };
    static const char *const keys[K_N] = { "v", "test", "config", "machine", "snapshot",
                                           "session", "wait", "timed", "limit", "end",
                                           "verdict", "runs" };
    static const unsigned mandatory = (1u << K_VERDICT) - 1;     /* all before verdict */
    const size_t tl = strlen(STATLINE_TAG);
    unsigned seen = 0;
    size_t i = tl;

    while (len && (line[len - 1] == '\n' || line[len - 1] == '\r'))
        len--;
    if (len < tl || memcmp(line, STATLINE_TAG, tl) || (len > tl && line[tl] != ' '))
        return 0;
    statline_init(s);
    while (i < len) {
        size_t ks, kl, vs, vl, k;
        unsigned long x;

        if (line[i] != ' ')
            return fail(err, errlen, "no blank before column %zu", i + 1);
        i++;
        ks = i;
        while (i < len && line[i] != '=' && line[i] != ' ')
            i++;
        kl = i - ks;
        if (i == len || line[i] != '=' || !kl)
            return fail(err, errlen, "column %zu: not KEY=VALUE", ks + 1);
        vs = ++i;
        while (i < len && line[i] != ' ')
            i++;
        vl = i - vs;
        if (!vl)
            return fail(err, errlen, "%.*s: empty value", (int)kl, line + ks);
        for (k = vs; k < vs + vl; k++)
            if (!value_char((unsigned char)line[k]))
                return fail(err, errlen, "%.*s: bad character in the value", (int)kl, line + ks);
        for (k = 0; k < K_N && (strlen(keys[k]) != kl || memcmp(keys[k], line + ks, kl)); k++)
            ;
        if (k == K_N)
            continue;           /* a key of a later version */
        if (seen & (1u << k))
            return fail(err, errlen, "%s twice", keys[k]);
        seen |= 1u << k;
#define TEXT(f)                                                                       \
        do {                                                                          \
            if (vl >= sizeof s->f)                                                    \
                return fail(err, errlen, "%s: value too long", keys[k]);              \
            memcpy(s->f, line + vs, vl);                                              \
            s->f[vl] = 0;                                                             \
        } while (0)
        switch (k) {
        case K_V:
            if (whole(line + vs, vl, 1000, &x) || x != STATLINE_VERSION)
                return fail(err, errlen, "unknown version %.*s", (int)vl, line + vs);
            break;
        case K_TEST: TEXT(test); break;
        case K_CONFIG: TEXT(config); break;
        case K_MACHINE: TEXT(machine); break;
        case K_SNAPSHOT: TEXT(snapshot); break;
        case K_END: TEXT(end); break;
        case K_VERDICT: TEXT(verdict); break;
        case K_SESSION:
        case K_WAIT:
        case K_TIMED:
            if (seconds(line + vs, vl, k == K_SESSION ? &s->session : k == K_WAIT ? &s->wait
                                                                         : &s->timed))
                return fail(err, errlen, "%s: not a number of seconds", keys[k]);
            break;
        case K_LIMIT:
            if (whole(line + vs, vl, 1000000000ul, &x))
                return fail(err, errlen, "limit: not a whole number of seconds");
            s->limit = (unsigned)x;
            break;
        case K_RUNS:
            if (whole(line + vs, vl, 1000000000ul, &x))
                return fail(err, errlen, "runs: not a whole number");
            s->runs = (int)x;
            break;
        }
#undef TEXT
    }
    if ((seen & mandatory) != mandatory) {
        for (i = 0; i < K_N && ((seen & (1u << i)) || !(mandatory & (1u << i))); i++)
            ;
        return fail(err, errlen, "no %s", keys[i]);
    }
    return 1;
}
