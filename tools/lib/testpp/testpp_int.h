/*
 * testpp_int.h - the scanner of the Blitz source text that the files of
 * lib_testpp share (not part of its interface).
 */
#ifndef TESTPP_INT_H
#define TESTPP_INT_H

#include <stddef.h>
#include "testpp.h"

/* a statement of a line: [start, end) without the blanks around it */
struct tp_stmt {
    size_t start, end;
};

/*
 * The statements of a line (l[0..len)): split at ':' outside strings and
 * outside ( { nesting; a ';' outside strings starts the comment, which
 * ends the last statement.  *comment = the offset of the ';' or len.
 * Returns the number of statements written to st (at most max; empty
 * statements are left out).
 */
size_t tp_statements(const char *l, size_t len, struct tp_stmt *st, size_t max, size_t *comment);

/* whether the statement l[s..e) starts with the word w (case ignored, a
   word ends at a character that is no letter, digit or '_'); *after the
   offset behind it */
int tp_word(const char *l, size_t s, size_t e, const char *w, size_t *after);

/* skip blanks and tabs from p up to e */
size_t tp_blanks(const char *l, size_t p, size_t e);

/*
 * The arguments of a macro call: l[open] is the '{'.  The compiler's
 * findit: an argument ends at ',' or '}' outside strings and outside
 * ( { nesting; a byte with bit 7 set takes the next byte with it.
 * Writes up to max [start, end) pairs (raw, not trimmed) to args;
 * *close = the offset of the '}'.  Returns the number of arguments, -1 if
 * the line ends before the '}'.
 */
int tp_args(const char *l, size_t len, size_t open, struct tp_stmt *args, int max, size_t *close);

/* the block macros */
enum tp_block {
    TP_B_NONE,
    TP_B_INIT,                  /* !TestBatchInit */
    TP_B_END_INIT,              /* !EndTestBatchInit */
    TP_B_CASE,                  /* !TestCase{NAME} */
    TP_B_END_CASE,              /* !EndTestCase */
    TP_B_FINAL,                 /* !TestBatchEnd */
    TP_B_END_FINAL              /* !EndTestBatchEnd */
};

/* the block macro the statement l[s..e) starts with (TP_B_NONE: none);
   for TP_B_CASE *name/[nlen] the argument trimmed (nlen 0: none) */
int tp_block_macro(const char *l, size_t s, size_t e, size_t len, const char **name, size_t *nlen);

/* adds "FILE:LINE: TEXT" to the messages; -1 on out of memory */
int tp_error(struct tp_errors *e, const char *file, unsigned line, const char *fmt, ...)
    __attribute__((format(printf, 4, 5)));

char *tp_strndup(const char *s, size_t n);

#endif
