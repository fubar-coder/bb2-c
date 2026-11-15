/*
 * perfbudget.h - the benchmark of the libraries (tests/perf): the cycles per
 * call from the programs' output.
 *
 * A benchmark program (tests/perf/bench/NAME.bas) runs one For loop per
 * command and prints one line per loop:
 *
 *   TEST T N PER
 *
 *   T    the time of the loop in units of the CIA-B cascade (timer A divides
 *        the E clock by 100, timer B counts its underflows: one unit is
 *        1000 CPU cycles), 0 to 65535
 *   N    the iterations of the loop
 *   PER  the calls per iteration (a positive number), or "frame": the loop
 *        is empty and runs while an interrupt handler works (PlayModule);
 *        the value is the handler's cycles per PAL frame
 *
 * Every program has the empty loop "empty"; its cycles per iteration E are
 * subtracted: a command costs (T * 1000 / N - E) / PER cycles per call, a
 * frame test PERF_FRAME_CYCLES * (1 - E / (T * 1000 / N)) cycles per frame
 * (the share of the frame the handler takes from the loop).  The output
 * ends with the runner's line "rc=N", which must be rc=0.  The command's
 * name is PROGRAM.TEST, PROGRAM the name of the program.
 */
#ifndef PERFBUDGET_H
#define PERFBUDGET_H

#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* the 68000 cycles of a PAL frame: 313 lines of 454 cycles */
#define PERF_FRAME_CYCLES 142102

struct perf_meas {
    char *name;                         /* PROGRAM.TEST */
    double value;                       /* cycles per call (per frame, the
                                           empty loop: per iteration) */
    int frame;                          /* a frame test */
    int empty;                          /* the empty loop (not a command) */
};

struct perf_run {
    struct perf_meas *m;
    size_t n;
};

/**
 * @brief Add the measurements of one program from its output.
 * @param program  the program's name (the file name without .out)
 * @param text     its output (the observation NAME.out)
 * @return 0 = added, -1 = malformed output, out of memory (message in err,
 *         with the program's name; nothing added)
 */
int perf_read_out(struct perf_run *r, const char *program, const char *text, char *err,
                  size_t errlen);
void perf_run_free(struct perf_run *r);

/**
 * @brief The table of the measurements: per command its cycles per call,
 *        the empty loops as the programs' baseline.
 */
void perf_table(const struct perf_run *r, FILE *f);

#ifdef __cplusplus
}
#endif

#endif
