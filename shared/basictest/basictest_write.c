/*
 * basictest_write.c - the writer of the result file's entries
 * (basictest.h).  Compiled by the test library with the Blitz library
 * conventions and by the host tools: no libc, no libgcc - decimal by
 * subtracting powers of ten, hex by shifts - and no writable static data.
 */
#include "basictest.h"

/* the output: buf up to end, at the next byte; full once a byte did not fit */
struct out {
    uint8_t *at, *end;
    int full;
};

static void put(struct out *o, uint8_t c)
{
    if (o->at == o->end)
        o->full = 1;
    else
        *o->at++ = c;
}

static void puts_(struct out *o, const char *s)
{
    while (*s)
        put(o, (uint8_t)*s++);
}

static const char hexdigit[] = "0123456789ABCDEF";

size_t basictest_decimal(uint32_t v, uint8_t *buf)
{
    static const uint32_t pow10[10] = { 1000000000, 100000000, 10000000, 1000000, 100000,
                                        10000, 1000, 100, 10, 1 };
    size_t n = 0;
    int i;

    for (i = 0; i < 10; i++) {
        uint8_t d = '0';

        while (v >= pow10[i]) {
            v -= pow10[i];
            d++;
        }
        if (d != '0' || n || i == 9)
            buf[n++] = d;
    }
    return n;
}

static void putdecimal(struct out *o, uint32_t v)
{
    uint8_t digits[10];
    size_t n = basictest_decimal(v, digits), i;

    for (i = 0; i < n; i++)
        put(o, digits[i]);
}

static void putlong(struct out *o, uint32_t v)
{
    int shift;

    put(o, '$');
    for (shift = 28; shift >= 0; shift -= 4)
        put(o, (uint8_t)hexdigit[(v >> shift) & 15]);
}

static void putstr(struct out *o, const struct basictest_str *s)
{
    size_t i;

    for (i = 0; i < s->length; i++) {
        uint8_t c = s->data[i];

        if (c >= 0x20 && c <= 0x7E && c != '\\') {
            put(o, c);
        } else {
            put(o, '\\');
            put(o, 'x');
            put(o, (uint8_t)hexdigit[c >> 4]);
            put(o, (uint8_t)hexdigit[c & 15]);
        }
    }
}

static void tab(struct out *o)
{
    put(o, '\t');
}

size_t basictest_write(const struct basictest_entry *e, uint8_t *buf, size_t size)
{
    struct out o;

    o.at = buf;
    o.end = buf + size;
    o.full = 0;
    switch (e->kind) {
    case BASICTEST_BEGIN:
        puts_(&o, "begin\t");
        putdecimal(&o, e->version);
        break;
    case BASICTEST_BLOCK:
        puts_(&o, "block\t");
        if (e->block == BASICTEST_BLOCK_INIT) {
            puts_(&o, "init");
        } else if (e->block == BASICTEST_BLOCK_FINAL) {
            puts_(&o, "final");
        } else if (e->block == BASICTEST_BLOCK_CASE && e->name.length) {
            puts_(&o, "case ");
            putstr(&o, &e->name);
        } else {
            return 0;
        }
        break;
    case BASICTEST_CHECK:
    case BASICTEST_CHECKSTR:
        puts_(&o, e->kind == BASICTEST_CHECK ? "check\t" : "checkstr\t");
        putdecimal(&o, e->k);
        puts_(&o, e->ok ? "\tok\t" : "\tfail\t");
        if (e->kind == BASICTEST_CHECK) {
            putlong(&o, e->expected);
            tab(&o);
            putlong(&o, e->actual);
        } else {
            putstr(&o, &e->expected_str);
            tab(&o);
            putstr(&o, &e->actual_str);
        }
        break;
    case BASICTEST_REQUIRE:
        puts_(&o, "require\t");
        putdecimal(&o, e->k);
        puts_(&o, "\tbroken");
        break;
    case BASICTEST_EXPECT_ERROR:
        puts_(&o, "expect-error\t");
        putdecimal(&o, e->k);
        break;
    case BASICTEST_BLOCKEND:
        puts_(&o, "blockend");
        break;
    case BASICTEST_BROKEN:
        puts_(&o, "broken\t");
        putstr(&o, &e->reason);
        break;
    case BASICTEST_INCOMPLETE:
        puts_(&o, "incomplete");
        break;
    case BASICTEST_END:
        puts_(&o, "end");
        break;
    default:
        return 0;
    }
    put(&o, '\n');
    if (o.full)
        return 0;
    return (size_t)(o.at - buf);
}
