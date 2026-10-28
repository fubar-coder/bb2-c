/*
 * tdout.h - unbuffered line output for the TED A/B test tools (tedprobe,
 * teddrivetest): a line is collected with o_str/o_dec/o_hex and written
 * with o_line() to stdout and, if open, to a file (Write, no stdio - the
 * output must survive a hang of the emulator session).
 */
#ifndef TDOUT_H
#define TDOUT_H

#include <string.h>
#include <proto/dos.h>

static char o_buf[400];
static BPTR o_file;             /* optional copy of every line */

static void o_str(const char *s)
{
    strncat(o_buf, s ? s : "(null)", sizeof o_buf - strlen(o_buf) - 2);
}

static void o_dec(long v)
{
    char b[12], *p = b + 11;
    unsigned long u = v < 0 ? -(unsigned long)v : (unsigned long)v;
    *p = 0;
    do { *--p = '0' + u % 10; u /= 10; } while (u);
    if (v < 0) *--p = '-';
    o_str(p);
}

static void o_hex(unsigned long v, int digits)
{
    char b[10];
    int i;
    for (i = digits - 1; i >= 0; i--, v >>= 4) b[i] = "0123456789ABCDEF"[v & 15];
    b[digits] = 0;
    o_str("$");
    o_str(b);
}

static void o_line(void)
{
    strcat(o_buf, "\n");
    Write(Output(), o_buf, strlen(o_buf));
    if (o_file) Write(o_file, o_buf, strlen(o_buf));
    o_buf[0] = 0;
}

/* "key value" lines */
static inline void o_kv(const char *k, long v) { o_str(k); o_str(" "); o_dec(v); o_line(); }
static inline void o_kx(const char *k, unsigned long v) { o_str(k); o_str(" "); o_hex(v, 8); o_line(); }
static inline void o_ks(const char *k, const char *s) { o_str(k); o_str(" "); o_str(s); o_line(); }

#endif
