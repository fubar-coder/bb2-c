/*
 * tape.h - the recorded results of the original libraries (tape.c):
 * every comparison of the original (sets[0]) with the C build (sets[1])
 * passes its observations through these, so that libs_emulator can
 * compare the C build with a recording when the original is not there.
 *
 *   tape_obs(what, o, c)        the original's value to compare with c (a
 *                               value inside the test program's hunks or
 *                               stack is hashed as "PROG": the layout moves)
 *   tape_val(what, o, c)        the same for a plain number (libtest)
 *   tape_same(what, o, c)       o == c, through tape_obs
 *   tape_bytes(what, o, c, n)   memcmp of n bytes (0: the same)
 *   tape_ptrbytes(what, o, c, n)  the same for records holding pointers: a
 *                               long at any offset that points into the
 *                               test program is hashed as "PROG"
 *   tape_strsame(what, o, c)    the two strings the same
 *   tape_fixed(what, o, c)      the original's value of a check with an
 *                               exemption (expect_fixed), recorded as it is
 *   tape_kept(what, o, c)       the registers the original keeps (mask o)
 *                               for a call where the C build keeps c
 *   tape_pause(on)              from here not recorded, compared live only (a
 *                               part whose results differ from run to run)
 *   tape_orig()                 sets[0] is the original (not in replay)
 *
 * tape_begin(dir, name) / tape_end() around one test program
 * (libs_tests.cpp).
 */
#ifndef TAPE_H
#define TAPE_H

#include <exec/types.h>

#ifdef __cplusplus
extern "C" {
#endif

enum { TAPE_LIVE, TAPE_RECORD, TAPE_REPLAY };
extern int tape_mode;

void tape_begin(const char *dir, const char *name);
void tape_end(void);
ULONG tape_obs(const char *what, ULONG o, ULONG c);
ULONG tape_val(const char *what, ULONG o, ULONG c);
int tape_same(const char *what, ULONG o, ULONG c);
int tape_bytes(const char *what, const void *o, const void *c, ULONG n);
int tape_ptrbytes(const char *what, const void *o, const void *c, ULONG n);
int tape_strsame(const char *what, const char *o, const char *c);
ULONG tape_fixed(const char *what, ULONG o, ULONG c);
ULONG tape_kept(const char *what, ULONG o, ULONG c);
void tape_pause(int on);
int tape_orig(void);

#ifdef __cplusplus
}
#endif

#endif
