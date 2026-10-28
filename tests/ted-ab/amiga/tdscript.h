/*
 * tdscript.h - the script syntax of teddrive (teddrive.c): words, "..."
 * strings with escapes, numbers, gadget label matching.  Plain C without
 * AmigaOS headers: the runner (tests/ted-ab/runner, tedab) reads the
 * scripts with it too; host test: tests/ted-ab/host/test_tdscript.cpp.
 */
#ifndef TDSCRIPT_H
#define TDSCRIPT_H

#include <string.h>

static int td_lower(int c) { return c >= 'A' && c <= 'Z' ? c + 32 : c; }
static int td_ishex(int c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

/* decimal (or base), "$" / "0x" = hex, optional "-"; stops at the first
   character that is no digit */
static long td_number(const char *s, int base)
{
    long v = 0;
    int neg = 0;
    if (*s == '-') neg = 1, s++;
    if (*s == '$') base = 16, s++;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) base = 16, s += 2;
    for (; *s; s++) {
        int d;
        if (*s >= '0' && *s <= '9') d = *s - '0';
        else if (*s >= 'a' && *s <= 'f') d = *s - 'a' + 10;
        else if (*s >= 'A' && *s <= 'F') d = *s - 'A' + 10;
        else break;
        if (d >= base) break;
        v = v * base + d;
    }
    return neg ? -v : v;
}

/* split a line into words in place: blanks separate, ';' outside a word
   ends the line, "..." with \n \t \xHH (two hex digits; \ + anything else
   = that character).
   Returns the count (at most max). */
static int td_split(char *s, char **av, int max)
{
    int n = 0;
    for (;;) {
        char *d;
        while (*s == ' ' || *s == '\t' || *s == '\r') s++;
        if (!*s || *s == ';' || n >= max) break;
        if (*s == '"') {
            av[n++] = d = ++s;
            while (*s && *s != '"') {
                if (*s == '\\' && s[1]) {
                    s++;
                    if (*s == 'n') *d++ = '\n', s++;
                    else if (*s == 't') *d++ = '\t', s++;
                    else if (*s == 'x' && td_ishex(s[1]) && td_ishex(s[2])) {
                        char h[3];
                        h[0] = s[1];
                        h[1] = s[2];
                        h[2] = 0;
                        *d++ = (char)td_number(h, 16);
                        s += 3;
                    } else
                        *d++ = *s++;
                } else
                    *d++ = *s++;
            }
            if (*s) s++;            /* the closing quote */
            *d = 0;                 /* d <= the closing quote */
        } else {
            av[n++] = s;
            while (*s && *s != ' ' && *s != '\t' && *s != '\r') s++;
            if (*s) *s++ = 0;
        }
    }
    return n;
}

/* gadget label l = want?  case-insensitive, blanks around both ignored */
static int td_labelmatch(const unsigned char *l, const char *want)
{
    int n;
    if (!l) return 0;
    while (*l == ' ') l++;
    while (*want == ' ') want++;
    n = strlen(want);
    while (n && want[n - 1] == ' ') n--;
    while (n--)
        if (td_lower(*l++) != td_lower((unsigned char)*want++)) return 0;
    while (*l == ' ') l++;
    return *l == 0;
}

#endif
