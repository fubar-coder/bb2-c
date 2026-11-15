/*
 * perfbudget.h - the benchmark of the libraries (tests/perf): the cycles per
 * call from the programs' output and its budgets.
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

/*
 * The budget file (tests/perf/budgets.txt), '#' comments, one line per
 * command:
 *
 *   PROGRAM.TEST BUDGET TARGET TEXT
 *
 *   BUDGET  the highest allowed cycles per call (per frame), a whole number;
 *           "B~T" marks a loop whose repeated runs differ by more than a
 *           cycle per call (it waits for the blitter or depends on the
 *           frame): B is the highest value of the runs, T the tolerance (a
 *           positive whole number, twice the spread of the runs), and
 *           B + T the highest allowed value
 *   TARGET  the original's value (a decimal number, or "-": none), only
 *           shown
 *   TEXT    what the loop does (the rest of the line, not empty)
 */
struct perf_budget {
    char *name;                         /* PROGRAM.TEST */
    long budget;                        /* B, or the plain budget */
    long tolerance;                     /* T of "B~T", else 0: the CPU alone */
    double target;
    int has_target;
    char *text;
};

struct perf_budgets {
    struct perf_budget *b;
    size_t n;
};

/**
 * @brief Read a budget file.
 * @return 0 = read (replaces *b), -1 = malformed or out of memory (message
 *         in err with the line; *b unchanged)
 */
int perf_read_budgets(struct perf_budgets *b, const char *text, char *err, size_t errlen);
void perf_budgets_free(struct perf_budgets *b);

/**
 * @brief Judge the measurements against the budgets: one line to f per
 *        command over its budget ("NAME: measured X cycles per call, budget
 *        B", for a marked loop "budget B + tolerance T"), per measured command without a budget and per budget without
 *        a measurement.  The empty loops have no budget.
 * @return the number of those lines (0 = every command within its budget)
 */
int perf_judge(const struct perf_run *r, const struct perf_budgets *b, FILE *f);

/**
 * @brief The table of the measurements: per command its cycles per call,
 *        the empty loops as the programs' baseline; with budgets (b not
 *        NULL) also the budget, the target and the factor to it, and the
 *        text.
 */
void perf_table(const struct perf_run *r, const struct perf_budgets *b, FILE *f);

#ifdef __cplusplus
}
#endif

#endif
