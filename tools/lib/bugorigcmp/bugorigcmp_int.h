/*
 * bugorigcmp_int.h - the parts of tools/lib/bugorigcmp shared by its
 * source files (bugorigcmp.h is the interface).
 */
#ifndef BUGORIGCMP_INT_H
#define BUGORIGCMP_INT_H

#include <stddef.h>
#include <stdio.h>

/* a growing string, always 0-terminated once something was added */
struct bstr {
    char *p;
    size_t n, cap;
    int bad;                    /* out of memory: nothing more is added */
};

/* 0 = ok, -1 = out of memory (s->bad) */
int bstr_add(struct bstr *s, const char *p, size_t n);
/* the string (malloc'd, "" if empty; NULL if out of memory), s emptied */
char *bstr_take(struct bstr *s);

/* the line p (n bytes) for messages into buf: bytes outside 32..126 and
   '"' as \xNN, cut with ...; end: "(the end)" */
void bugorigcmp_show(const char *p, size_t n, char *buf, size_t len, int end);

/* the case value of a section's text (bugorigcmp.h, "the cases") */
void bugorigcmp_put_value(FILE *out, const char *s);

#endif
