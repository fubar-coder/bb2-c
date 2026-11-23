/*
 * testdriver.h - the driver of a Blitz test of the model (change
 * basic-test-runner, design D6; the order: spec
 * testing/basic-test-interface, "The order of the driver").
 *
 * The runner writes one driver per run: a program that
 *
 *   1. holds the macro text of the test library (shared/basictest/
 *      macros.bas, as the build gives it: TESTLIB_MACROS),
 *   2. starts the library with the name of the result file
 *      (`TLBegin "NAME"`; user: "In den Treiber"),
 *   3. jumps over the text (`Goto tc__skip`),
 *   4. holds the preprocessed text of the test source (tp_process),
 *      copied line by line, so a compile error keeps its line,
 *   5. behind the label `.tc__skip` calls with `Gosub` the batch init
 *      (only if the source has one and a case is selected), each selected
 *      case (`tc_NAME`) and the batch cleanup (only if the source has one and
 *      a case is selected),
 *   6. and ends (`End`).
 *
 * A selected case the source does not have is called all the same (after
 * the cases it has, in the order given): compiling the driver fails with
 * "Label not Found" on its call, which tp_driver_where names (a renamed
 * case, or one behind CNIF).  The cases the source has are called in
 * source order.
 */
#ifndef TESTDRIVER_H
#define TESTDRIVER_H

#include <stddef.h>
#include "testpp.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief The parameters of a driver. */
struct tp_driver_params {
    const char *macros;         /* the macro text (lines ended by LF) */
    size_t macros_len;
    const char *result;         /* the result file's Amiga name: printable
                                   ASCII without `"`, at most 100 bytes */
    const char *const *cases;   /* the selected cases */
    size_t ncases;
};

/** @brief A driver and the map of its lines. */
struct tp_driver {
    char *text;                 /* the driver, lines ended by LF */
    size_t len;
    unsigned nlines;
    unsigned first;             /* the driver line of the source's line 1 */
    unsigned nsource;           /* the source's lines in the driver */
    unsigned calls;             /* the driver line of the first Gosub, 0: none */
    char **called;              /* what the Gosub lines from `calls` on call:
                                   "init", "final" or the case's name */
    size_t ncalled;
};

/**
 * @brief Write the driver of a preprocessed source (tp_process without
 *        errors).
 * @param err  the reason of a refusal (a result name that cannot stand in
 *             a string literal)
 * @return 0 = ok, -1 = refused (err), -2 = out of memory
 */
int tp_driver_write(const struct tp_source *src, const struct tp_driver_params *dp, struct tp_driver *d,
                    char *err, size_t errlen);

/** @brief Free a driver (zeroed). */
void tp_driver_free(struct tp_driver *d);

/** @brief Where a driver line is. */
enum tp_where {
    TP_IN_MACROS,               /* the macro text or the start */
    TP_IN_SOURCE,               /* the source: *line its line */
    TP_IN_CALL,                 /* a Gosub: *name what it calls */
    TP_IN_END                   /* the end of the driver */
};

/**
 * @brief Where line driver_line (from 1) of the driver is: in the source
 *        (*line its line), in a call (*name "init", "final" or the case),
 *        or in the driver's own text.
 */
int tp_driver_where(const struct tp_driver *d, unsigned driver_line, unsigned *line, const char **name);

#ifdef __cplusplus
}
#endif

#endif
