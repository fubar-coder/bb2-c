/*
 * basictest_read.c - the reader of the result file's entries
 * (basictest.h): one line into a struct basictest_entry, strictly - every
 * value in the one form the writer gives it, so that writer and reader
 * cannot drift apart unnoticed.  Freestanding like the writer.
 */
#include "basictest.h"

#define MAXFIELDS 5

struct field {
    uint8_t *data;
    size_t length;
};

/* the entry names, their kinds and their numbers of fields (with the
   name); block: 2 for init and final, 3 for a case (checked below) */
static const struct {
    const char *name;
    int kind;
    unsigned fields;
} kinds[] = {
    { "begin", BASICTEST_BEGIN, 2 },
    { "block", BASICTEST_BLOCK, 0 },
    { "check", BASICTEST_CHECK, 5 },
    { "checkstr", BASICTEST_CHECKSTR, 5 },
    { "require", BASICTEST_REQUIRE, 3 },
    { "expect-error", BASICTEST_EXPECT_ERROR, 2 },
    { "blockend", BASICTEST_BLOCKEND, 1 },
    { "broken", BASICTEST_BROKEN, 2 },
    { "incomplete", BASICTEST_INCOMPLETE, 1 },
    { "end", BASICTEST_END, 1 },
};

static int is(const struct field *f, const char *s)
{
    size_t i;

    for (i = 0; i < f->length; i++)
        if (s[i] != (char)f->data[i])
            return 0;
    return s[i] == 0;
}

static int hexval(uint8_t c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

/* K and the version: decimal, no sign, no leading zero, 32 bits */
static int decimal(const uint8_t *s, size_t n, uint32_t *v)
{
    static const char max[] = "4294967295";
    size_t i;
    uint32_t r = 0;

    if (n == 0 || n > 10 || (s[0] == '0' && n > 1))
        return 0;
    for (i = 0; i < n; i++)
        if (s[i] < '0' || s[i] > '9')
            return 0;
    if (n == 10) {
        for (i = 0; i < 10 && s[i] == (uint8_t)max[i]; i++)
            ;
        if (i < 10 && s[i] > (uint8_t)max[i])
            return 0;
    }
    for (i = 0; i < n; i++)
        r = (r << 3) + (r << 1) + (uint32_t)(s[i] - '0');
    *v = r;
    return 1;
}

/* a long value: $ and 8 upper-case hex digits */
static int longval(const struct field *f, uint32_t *v)
{
    size_t i;
    uint32_t r = 0;

    if (f->length != 9 || f->data[0] != '$')
        return 0;
    for (i = 1; i < 9; i++) {
        int d = hexval(f->data[i]);

        if (d < 0)
            return 0;
        r = (r << 4) | (uint32_t)d;
    }
    *v = r;
    return 1;
}

/* a string value, decoded in place: a byte $20-$7E other than \ as it is,
   every other byte \xHH; nothing else (no other escape, no byte escaped
   that stands as it is) */
static int strval(uint8_t *s, size_t n, struct basictest_str *v)
{
    size_t i = 0, o = 0;

    while (i < n) {
        uint8_t c = s[i];

        if (c == '\\') {
            int hi, lo;

            if (n - i < 4 || s[i + 1] != 'x')
                return 0;
            hi = hexval(s[i + 2]);
            lo = hexval(s[i + 3]);
            if (hi < 0 || lo < 0)
                return 0;
            c = (uint8_t)((hi << 4) | lo);
            if (c >= 0x20 && c <= 0x7E && c != '\\')
                return 0;
            i += 4;
        } else if (c < 0x20 || c > 0x7E) {
            return 0;
        } else {
            i++;
        }
        s[o++] = c;
    }
    v->data = s;
    v->length = o;
    return 1;
}

static int fail(struct basictest_error *err, int code, unsigned field)
{
    if (err) {
        err->code = code;
        err->field = field;
    }
    return code;
}

int basictest_read(uint8_t *line, size_t length, uint32_t line_number,
                   struct basictest_entry *e, struct basictest_error *err)
{
    struct field f[MAXFIELDS];
    unsigned nf = 0, ki;
    size_t at = 0, start = 0;

    if (err) {
        err->line = line_number;
        err->code = BASICTEST_OK;
        err->field = 0;
        err->kind = line;
        err->kind_length = 0;
    }
    for (at = 0; at <= length; at++) {
        if (at == length || line[at] == '\t') {
            if (nf < MAXFIELDS) {
                f[nf].data = line + start;
                f[nf].length = at - start;
            }
            nf++;
            start = at + 1;
        }
    }
    if (err)
        err->kind_length = f[0].length;
    for (ki = 0; ki < sizeof kinds / sizeof kinds[0]; ki++)
        if (is(&f[0], kinds[ki].name))
            break;
    if (ki == sizeof kinds / sizeof kinds[0])
        return fail(err, BASICTEST_E_KIND, 1);
    if (kinds[ki].fields ? nf != kinds[ki].fields : nf != 2 && nf != 3)
        return fail(err, BASICTEST_E_FIELDS, 0);

    e->kind = kinds[ki].kind;
    e->version = 0;
    e->block = 0;
    e->name.data = line;
    e->name.length = 0;
    e->k = 0;
    e->ok = 0;
    e->expected = e->actual = 0;
    e->expected_str = e->actual_str = e->reason = e->name;

    switch (e->kind) {
    case BASICTEST_BEGIN:
        if (!decimal(f[1].data, f[1].length, &e->version))
            return fail(err, BASICTEST_E_VALUE, 2);
        if (e->version != BASICTEST_VERSION)
            return fail(err, BASICTEST_E_VERSION, 2);
        break;
    case BASICTEST_BLOCK:
        if (is(&f[1], "init"))
            e->block = BASICTEST_BLOCK_INIT;
        else if (is(&f[1], "final"))
            e->block = BASICTEST_BLOCK_FINAL;
        else if (is(&f[1], "case"))
            e->block = BASICTEST_BLOCK_CASE;
        else
            return fail(err, BASICTEST_E_VALUE, 2);
        /* a case has a name, init and final have none */
        if (nf != (e->block == BASICTEST_BLOCK_CASE ? 3u : 2u))
            return fail(err, BASICTEST_E_FIELDS, 0);
        if (e->block == BASICTEST_BLOCK_CASE
            && (f[2].length == 0 || !strval(f[2].data, f[2].length, &e->name)))
            return fail(err, BASICTEST_E_VALUE, 3);
        break;
    case BASICTEST_CHECK:
    case BASICTEST_CHECKSTR:
        if (!decimal(f[1].data, f[1].length, &e->k))
            return fail(err, BASICTEST_E_VALUE, 2);
        if (is(&f[2], "ok"))
            e->ok = 1;
        else if (!is(&f[2], "fail"))
            return fail(err, BASICTEST_E_VALUE, 3);
        if (e->kind == BASICTEST_CHECK) {
            if (!longval(&f[3], &e->expected))
                return fail(err, BASICTEST_E_VALUE, 4);
            if (!longval(&f[4], &e->actual))
                return fail(err, BASICTEST_E_VALUE, 5);
        } else {
            if (!strval(f[3].data, f[3].length, &e->expected_str))
                return fail(err, BASICTEST_E_VALUE, 4);
            if (!strval(f[4].data, f[4].length, &e->actual_str))
                return fail(err, BASICTEST_E_VALUE, 5);
        }
        break;
    case BASICTEST_REQUIRE:
        if (!decimal(f[1].data, f[1].length, &e->k))
            return fail(err, BASICTEST_E_VALUE, 2);
        if (!is(&f[2], "broken"))
            return fail(err, BASICTEST_E_VALUE, 3);
        break;
    case BASICTEST_EXPECT_ERROR:
        if (!decimal(f[1].data, f[1].length, &e->k))
            return fail(err, BASICTEST_E_VALUE, 2);
        break;
    case BASICTEST_BROKEN:
        if (!strval(f[1].data, f[1].length, &e->reason))
            return fail(err, BASICTEST_E_VALUE, 2);
        break;
    default:
        break;
    }
    return BASICTEST_OK;
}

/* the error text: a bounded writer into a 0-terminated buffer */
struct text {
    char *at, *end;             /* end: the place of the 0 */
};

static void tput(struct text *t, char c)
{
    if (t->at < t->end)
        *t->at++ = c;
}

static void tputs(struct text *t, const char *s)
{
    while (*s)
        tput(t, *s++);
}

static void tdecimal(struct text *t, uint32_t v)
{
    uint8_t d[10];
    size_t n = basictest_decimal(v, d), i;

    for (i = 0; i < n; i++)
        tput(t, (char)d[i]);
}

size_t basictest_error_text(const struct basictest_error *err, char *buf, size_t size)
{
    static const char hex[] = "0123456789ABCDEF";
    struct text t;
    size_t i;

    t.at = buf;
    t.end = buf + size - 1;
    tputs(&t, "line ");
    tdecimal(&t, err->line);
    tputs(&t, ": ");
    if (err->kind && err->kind_length) {
        for (i = 0; i < err->kind_length; i++) {
            uint8_t c = err->kind[i];

            if (c >= 0x20 && c <= 0x7E && c != '\\') {
                tput(&t, (char)c);
            } else {
                tputs(&t, "\\x");
                tput(&t, hex[c >> 4]);
                tput(&t, hex[c & 15]);
            }
        }
        tputs(&t, ": ");
    }
    switch (err->code) {
    case BASICTEST_OK:
        tputs(&t, "no error");
        break;
    case BASICTEST_E_KIND:
        tputs(&t, "unknown entry");
        break;
    case BASICTEST_E_FIELDS:
        tputs(&t, "wrong number of fields");
        break;
    case BASICTEST_E_VALUE:
        tputs(&t, "malformed value in field ");
        tdecimal(&t, err->field);
        break;
    case BASICTEST_E_VERSION:
        tputs(&t, "unknown format version");
        break;
    default:
        tputs(&t, "unknown error");
        break;
    }
    *t.at = 0;
    return (size_t)(t.at - buf);
}
