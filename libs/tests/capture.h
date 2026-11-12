/*
 * capture.h - the output of a library test program as cases (capture.c;
 * openspec change libs-tests-capture, design D4, D6, D7).
 *
 * A program observes one library set and writes every observation as one
 * case, a line KEY<TAB>VALUE, to DIR/NAME.txt:
 *
 *   # capture NAME 1
 *   ROUTINE|CHECK|VECTOR|OCC<TAB>VALUE
 *
 * The key is the current routine (cap_routine), the check text, the vector
 * (the input of the case; NULL: empty) and the occurrence number of that
 * routine, check and vector, counted from 1.  In every key part a TAB is
 * written as \t, a | as \|, a \ as \\ and any other byte outside $20-$7E
 * as \xHH (two upper-case hex digits).
 *
 *   cap_num(check, vec, v)          $XXXXXXXX
 *   cap_bytes(check, vec, p, n)     bytes N HEX (N <= 64), else bytes N fnv XXXXXXXX
 *   cap_ptrbytes(check, vec, p, n)  the same, a long at any offset that points
 *                                   into the test program written as "PROG"
 *   cap_str(check, vec, s)          "TEXT" (\" \\ and \xHH escapes); NULL: null
 *   cap_ptr(check, vec, v, name, base)
 *                                   0: $00000000; into the test program: prog;
 *                                   else NAME+$OFF / NAME-$OFF (hex, no zero
 *                                   padding); name NULL: $XXXXXXXX
 *   cap_regs(check, vec, lost)      kept, or lost d3 a1 (bit 0-7 d0-d7, 8-14 a0-a6)
 *   cap_not_captured(check, vec, reason)
 *                                   not captured: REASON (a part that cannot be
 *                                   reproduced; the judge skips it)
 *
 * cap_blocks(1) starts the block mode of the two very large programs (D6):
 * the observations are not written; every CAP_BLOCK of them become one
 * case BLOCK:N<TAB>COUNT XXXXXXXX first=KEY, N counted from 1, block N
 * holding the observations (N-1)*CAP_BLOCK+1 ... ; XXXXXXXX is the FNV-1a
 * hash of the block's observation lines as they would be written
 * (KEY<TAB>VALUE<LF>), whose occurrence number is the observation's number
 * in the program (from 1), and KEY in first= is the key of the block's
 * first observation.  cap_blocks(0) and cap_end() close a partial block.
 * cap_not_captured() is written as a plain case in block mode too.
 *
 * The shell variable LIBS_CAP_DUMP names a case by its full key (with the
 * occurrence number) or a block (BLOCK:N); the run then writes to
 * DIR/NAME.dump the bytes of that bytes case (KEY<TAB>at $OFFS HEX, 32
 * bytes a line) or the observation lines of that block.
 *
 * cap_begin(dir, name) / cap_end() around one test program; cap_end()
 * returns the number of faults (a file not written, a key too long, too
 * many distinct keys), each also reported through bl_test_check().
 */
#ifndef CAPTURE_H
#define CAPTURE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CAP_BLOCK 4096                  /* observations of a block case */
#define CAP_BYTES 64                    /* bytes written as hex, longer: fnv */

void cap_begin(const char *dir, const char *name);
int cap_end(void);
void cap_routine(const char *routine);
void cap_blocks(int on);

void cap_num(const char *check, const char *vec, uint32_t v);
void cap_bytes(const char *check, const char *vec, const void *p, uint32_t n);
void cap_ptrbytes(const char *check, const char *vec, const void *p, uint32_t n);
void cap_str(const char *check, const char *vec, const char *s);
void cap_ptr(const char *check, const char *vec, uint32_t v, const char *name, uint32_t base);
void cap_regs(const char *check, const char *vec, uint32_t lost);
void cap_not_captured(const char *check, const char *vec, const char *reason);

#ifdef __cplusplus
}
#endif

#endif
