/*
 * statline.h - the @stat line: one machine-readable line per emulator
 * session, written by the test runners (tests/corpus/runner: corpusrun,
 * cdrun; tests/ted-ab/runner: tedab, also in its bug-orig mode) to their
 * stdout.  ctest keeps the output of the test (the presets keep it whole,
 * also for passing tests), so the lines end up in the JUnit file of the
 * run (<system-out>); tools/teststats reads them from there.
 *
 * Format: one line, the fields separated by one blank, no other blanks:
 *
 *   @stat v=1 test=T config=C machine=M snapshot=S session=SEC wait=SEC
 *         timed=SEC limit=SEC end=E [verdict=V] [runs=N]
 *
 *   v         the version of the format: 1
 *   test      the runner's pass: the name of its work directory (the
 *             ctest name for the corpus and the bug-orig passes)
 *   config    what the session ran: corpusrun the configuration (ref,
 *             port, clib); cdrun the worker (w0, w1, ...); tedab the
 *             combination (R5, C5, Cc, Rc) in the bug-orig mode (a run per
 *             session), else the session (s001, s002, ...)
 *   machine   the emulated machine (a1200, a500, a500-68010)
 *   snapshot  what the snapshot mode did: cold (booted without one),
 *             restored, first-save (booted, saved, restored), fallback
 *             (the snapshot did not work: booted after all)
 *   session   seconds from the emulator slot taken to the end of the
 *             session (Amiberry ended by the session's supervisor, not
 *             when the runner joined it; the wait for the slot not
 *             included)
 *   wait      seconds the session waited for an emulator slot
 *   timed     the part of `session` counted against the time limit (from
 *             the last start of Amiberry, the job in place: a snapshot
 *             save or a fallback restarts it; amiga_emulation.h)
 *   limit     the time limit of the session (seconds)
 *   end       done (the script ran to its end), timeout (killed at the
 *             limit), exited (Amiberry ended before the script), failed
 *   verdict   optional: the result for this config - bug-orig FIXED,
 *             REPRODUCED, DIFFERS, BROKEN (tedab also NOTRUN), the
 *             reference "reference"; corpus same / differs; tedab outside
 *             bug-orig ok / fail (gates and every run of the session OK);
 *             missing where the session decides nothing (cdrun, a
 *             session whose run is repeated)
 *   runs      optional: the runs in the session (tedab)
 *
 * Seconds with two decimals.  Values only of the characters
 * [A-Za-z0-9._,:/+-]: the writer replaces any other one by '_' and an
 * empty value by '-'.  The writer puts the keys in the order above; the
 * reader takes them in any order, ignores keys it does not know (later
 * versions may add some), and refuses a line without one of the
 * mandatory keys, with a key twice, with a bad number or character.
 */
#ifndef STATLINE_H
#define STATLINE_H

#include <stddef.h>
#include <stdio.h>

#define STATLINE_TAG     "@stat"
#define STATLINE_VERSION 1

/** @brief The fields of a @stat line (see the top). */
struct statline {
    char test[128];
    char config[128];
    char machine[32];
    char snapshot[16];
    char end[16];
    char verdict[32];           /* "" = none */
    double session, wait, timed;
    unsigned limit;
    int runs;                   /* -1 = none */
};

/** @brief An empty line: no verdict, no runs. */
void statline_init(struct statline *s);

/**
 * @brief Set a text field (cut to its size).
 * @param field  the field
 * @param size   its size
 * @param value  the text (NULL: "")
 */
void statline_set(char *field, size_t size, const char *value);

/**
 * @brief Format the line (without newline).
 * @param s    the fields
 * @param buf  out: the line
 * @param len  the size of buf
 * @return the length, or -1 if it does not fit
 */
int statline_format(const struct statline *s, char *buf, size_t len);

/** @brief Write the line and a newline to f and flush it. */
void statline_write(FILE *f, const struct statline *s);

/**
 * @brief Read one line.
 * @param line    the line (no newline needed)
 * @param len     its length in characters
 * @param s       out: the fields
 * @param err     the message for a broken line
 * @param errlen  the size of err
 * @return 1 = a @stat line in *s, 0 = not one (it does not start with
 *         "@stat "), -1 = a broken one
 */
int statline_parse(const char *line, size_t len, struct statline *s, char *err, size_t errlen);

#endif
