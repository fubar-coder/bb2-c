/*
 * bugorig.h - the verdict of a bug-orig test (README, "Tests"): a program
 * that triggers a bug of the original, compiled and run by the original
 * blitz2 1.95 (the reference) and by the port; the expectation is the
 * *correct* behaviour.
 *
 * Observation: what one configuration showed, as sections
 *   [session]  done | timeout | exited early | failed  (the emulator
 *              session: a crash or a hang of the compiler or the program
 *              ends in timeout)
 *   [compile]  the result of the job: ok | error LINE MESSAGE |
 *              failed REASON | none (no RESULT line: the compiler did not
 *              get that far)
 *   [run]      what the program wrote, with rc=N (empty: not run)
 *   [blitz2]   what the compiler (and a wrapper, `wrap=`) wrote besides
 *              the session log, with rc=N
 *   [log]      the session log of tedstub (RESULT without the size)
 * Expectation (NAME.expect): the sections that matter, same format; a
 * line ending in `*` matches every line that begins with the text before
 * it, a line `...` any number of lines (also none).  [session] not given
 * means `done`.
 *
 * Verdict per configuration (only the sections of the expectation count):
 *   BROKEN      the reference shows the expected behaviour: the original
 *               does not have the bug (or the expectation describes the
 *               buggy behaviour) - not a valid bug-orig test
 *   FIXED       the configuration shows the expected behaviour
 *   REPRODUCED  it shows what the reference shows (the bug, on purpose)
 *   DIFFERS     neither: the port does not reproduce the original
 * A test is valid if the reference does not match the expectation; it
 * passes when every configuration is FIXED.
 */
#ifndef BUGORIG_H
#define BUGORIG_H

#include <stddef.h>
#include "util.h"

enum bugorig_verdict { BUGORIG_FIXED, BUGORIG_REPRODUCED, BUGORIG_DIFFERS, BUGORIG_BROKEN };

struct bugorig_section {
    char *name;
    char *text;                 /* lines, each ending in \n */
};

struct bugorig_obs {
    struct bugorig_section *sec;
    size_t n;
};

/* the raw outputs of one configuration (NULL = file missing) */
struct bugorig_outputs {
    const char *session;        /* amiga_emulation_status_name */
    const char *log;            /* session.log */
    const char *blitz2;         /* blitz2.out */
    const char *run;            /* NAME.run */
    const char *job;            /* the job's id in the log, e.g. j000 */
};

/* parses an expectation (or an observation); 0 = ok, -1 = error (err).
   Section names: session, compile, run, blitz2, log; each at most once,
   at least one */
int bugorig_parse(const char *text, struct bugorig_obs *o, char *err, size_t errlen);
void bugorig_free(struct bugorig_obs *o);

/* the observation of one configuration; 0 = ok, -1 = out of memory */
int bugorig_observe(const struct bugorig_outputs *out, struct bugorig_obs *o);

/* the text of section NAME of o (NULL = not there) */
const char *bugorig_get(const struct bugorig_obs *o, const char *name);

/* does o match the expectation e (wildcards, [session] default done) */
int bugorig_matches(const struct bugorig_obs *e, const struct bugorig_obs *o);

/* the verdict for a configuration; the differing sections go to why
   (may be NULL) */
enum bugorig_verdict bugorig_judge(const struct bugorig_obs *e, const struct bugorig_obs *ref,
                                   const struct bugorig_obs *o, struct text *why);

const char *bugorig_verdict_name(enum bugorig_verdict v);

#endif
