/*
 * tlcore.h - the logic of the Blitz test library (testlib.c): blocks,
 * checks, preconditions and the entries of the result file (spec
 * testing/basic-test-interface, testing/basic-testlib).
 *
 * The library's routines call these functions with the one state of the
 * library; the host tests (tests/host) call them with states of their
 * own.  Plain C99 on the freestanding headers, like the codec of
 * shared/basictest whose writer makes the lines: no libc, no libgcc, no
 * allocation.  The entries leave through two functions the user defines:
 *
 *   tl_os_write(line, n)  writes n bytes to the result file (one call per
 *                         write; on the Amiga one dos Write)
 *   tl_os_can_write()     1 if an entry may be written now, 0 if it must
 *                         be kept (the program is in Blitz mode, or has
 *                         multitasking or interrupts off)
 *
 * What the functions write (the entries of basictest.h):
 *
 *   tl_begin          begin 1 (the state is cleared first)
 *   tl_block          block init | block case NAME | block final; for a
 *                     case after a batch init that failed a check: the
 *                     first failed check of the init again (the case
 *                     fails); after a batch init whose precondition
 *                     failed: the precondition again and blockend (the
 *                     case is broken and does not run); for the batch
 *                     end after such an init nothing (it does not run)
 *   tl_blockend       blockend
 *   tl_check(str)     check / checkstr K ok|fail EXP ACT
 *   tl_require        require K broken and blockend, if the condition is 0
 *   tl_expect_error   expect-error K
 *   tl_finit          incomplete (a block is open), end
 *
 * A call out of order (a block inside a block, a check outside one, the
 * batch init after a case, a case after the batch end, a block end
 * without a block, a second tl_begin) writes `broken REASON` before its
 * own entry.  A damaged state (its guard words overwritten) writes
 * `broken state damaged` once; then only tl_finit writes, its `end`.
 *
 * Entries that cannot be written (tl_os_can_write() 0) are kept in the
 * state, TL_PENDING bytes, and written before the next entry that can
 * be, or by tl_finit, which always writes.  Entries that do not fit are
 * lost; `broken entries lost while kept` follows the kept ones then.  A
 * string check whose line does not fit into TL_LINE bytes has its two
 * values cut until it fits (the verdict is that of the whole strings).
 */
#ifndef TLCORE_H
#define TLCORE_H

#include <stddef.h>
#include <stdint.h>
#include "basictest.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TL_LINE    512          /* the longest entry */
#define TL_PENDING 1024         /* entries kept while they cannot be written */

#define TL_GUARD_LO 0x544C7374UL        /* "TLst" */
#define TL_GUARD_HI 0x7473544CUL        /* "tsTL" */

/** @brief The open block (enum basictest_block + 1; TL_NONE: none). */
enum { TL_NONE = 0, TL_INIT = BASICTEST_BLOCK_INIT + 1, TL_CASE = BASICTEST_BLOCK_CASE + 1,
       TL_FINAL = BASICTEST_BLOCK_FINAL + 1 };

/** @brief The state of the library; all of it between two guard words. */
struct tl_state {
    uint32_t guard_lo;
    uint8_t begun;              /* tl_begin ran, tl_finit not yet */
    uint8_t dead;               /* the state was damaged: write nothing more */
    uint8_t open;               /* the open block (TL_NONE ...) */
    uint8_t phase;              /* the furthest block opened: TL_NONE ... TL_FINAL */
    uint8_t init_failed;        /* the batch init failed a check: init_line */
    uint8_t init_broken;        /* the batch init failed a precondition */
    uint8_t lost;               /* kept entries were lost */
    uint8_t damaged;            /* the note of a damaged state is to be written */
    uint32_t init_broken_k;     /* that precondition */
    size_t init_length;
    size_t pending_length;
    uint8_t line[TL_LINE];      /* the entry being written */
    uint8_t init_line[TL_LINE]; /* the first failed check of the batch init */
    uint8_t pending[TL_PENDING];
    uint32_t guard_hi;
};

/* defined by the user of the functions (testlib.c, the host tests) */
void tl_os_write(const uint8_t *data, size_t length);
int tl_os_can_write(void);

/** @brief Clears the state and writes `begin`. */
void tl_begin(struct tl_state *st);

/**
 * @brief Opens a block (block: enum basictest_block; name and length:
 *   the case name, ignored for the batch init and end).
 * @return 1 if the block runs, 0 if it does not (no tl_begin, or after a
 *   batch init whose precondition failed).
 */
int tl_block(struct tl_state *st, int block, const uint8_t *name, size_t length);

/** @brief Closes the open block. */
void tl_blockend(struct tl_state *st);

/** @brief A check of two longs: ok if they are equal. */
void tl_check(struct tl_state *st, uint32_t k, uint32_t expected, uint32_t actual);

/** @brief A check of two strings (bytes): ok if they are equal. */
void tl_checkstr(struct tl_state *st, uint32_t k, const uint8_t *expected, size_t expected_length,
                 const uint8_t *actual, size_t actual_length);

/**
 * @brief A precondition.
 * @return 1 if it failed inside a block: the caller leaves the block (the
 *   block's call returns a second time, with 0); 0 otherwise.
 */
int tl_require(struct tl_state *st, uint32_t k, uint32_t condition);

/** @brief The announcement of an expected runtime error. */
void tl_expect_error(struct tl_state *st, uint32_t k);

/** @brief The end of the run: kept entries, `incomplete` if a block is open, `end`. */
void tl_finit(struct tl_state *st);

#ifdef __cplusplus
}
#endif

#endif
