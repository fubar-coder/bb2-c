/*
 * testinfo stats - statistics of test runs from ctest's JUnit files and the
 * runners' @stat lines in them (lib/teststats/teststats.h): durations,
 * emulator sessions, flaky results, suggested time limits and COST
 * values.
 *
 *   testinfo stats [--tsv] [--factor X] [--floor SEC] JUNIT.xml...
 *
 * Every file is one run (ctest --preset P writes build/amiga/junit-P.xml;
 * keep runs apart with --output-junit NAME, tests/README.md).  Problems (a
 * malformed file, a testcase without name or time, a broken @stat line)
 * go to stderr; the rest is reported.  Exit status 0, 1 = problems
 * (reported), 2 = usage or a file that cannot be read.
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "teststats.h"
#include "testinfo.h"

/* the defaults the usage names */
static_assert(TESTSTATS_FACTOR == 3.0 && TESTSTATS_FLOOR == 30.0, "the defaults in cmd_stats_usage");

const char cmd_stats_usage[] =
    "usage: testinfo stats [--tsv] [--factor X] [--floor SEC] JUNIT.xml...\n"
    "Statistics of ctest runs (one JUnit file per run) and the @stat lines of\n"
    "the emulator sessions in them: durations, sessions, flaky results,\n"
    "suggested time limits (max timed x factor, at least the floor) and COST.\n"
    "  --tsv         tab-separated values instead of text tables\n"
    "  --factor X    the factor of the time limits (default 3)\n"
    "  --floor SEC   the smallest suggested limit (default 30)\n";

static const char *me; /* "testinfo stats", the prefix of the messages */

static void usage(FILE *f)
{
    fputs(cmd_stats_usage, f);
}

static char *read_all(const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb");
    char *s = NULL;
    long n;

    if (!f)
        return NULL;
    if (!fseek(f, 0, SEEK_END) && (n = ftell(f)) >= 0 && !fseek(f, 0, SEEK_SET)
        && (s = malloc((size_t)n + 1)) && fread(s, 1, (size_t)n, f) == (size_t)n) {
        *len = (size_t)n;
        s[n] = 0;
    } else {
        free(s);
        s = NULL;
    }
    fclose(f);
    return s;
}

static int number(const char *s, double *out)
{
    char *end;

    errno = 0;
    *out = strtod(s, &end);
    return !*s || *end || errno ? -1 : 0;
}

int cmd_stats(int argc, char **argv)
{
    struct teststats_options o = { TESTSTATS_FACTOR, TESTSTATS_FLOOR, 0 };
    struct teststats *t;
    int i, nfiles = 0, problems = 0, rc;

    me = argv[0];
    for (i = 1; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (!strcmp(argv[i], "--")) {
            i++;
            break;
        } else if (!strcmp(argv[i], "--tsv")) {
            o.tsv = 1;
        } else if (!strcmp(argv[i], "--factor") && i + 1 < argc) {
            if (number(argv[++i], &o.factor) || !(o.factor > 0)) {
                fprintf(stderr, "%s: --factor %s: not a number > 0\n", me, argv[i]);
                return 2;
            }
        } else if (!strcmp(argv[i], "--floor") && i + 1 < argc) {
            if (number(argv[++i], &o.floor) || !(o.floor >= 0)) {
                fprintf(stderr, "%s: --floor %s: not a number >= 0\n", me, argv[i]);
                return 2;
            }
        } else {
            usage(stderr);
            return 2;
        }
    }
    if (i == argc) {
        usage(stderr);
        return 2;
    }
    if (!(t = teststats_new())) {
        fprintf(stderr, "%s: out of memory\n", me);
        return 2;
    }
    for (; i < argc; i++) {
        size_t len;
        char *text = read_all(argv[i], &len);
        int r;

        if (!text) {
            fprintf(stderr, "%s: cannot read %s\n", me, argv[i]);
            teststats_free(t);
            return 2;
        }
        r = teststats_add(t, argv[i], text, len, stderr);
        free(text);
        problems += r ? 1 : 0;
        nfiles++;
    }
    rc = teststats_report(t, &o, stdout);
    teststats_free(t);
    if (rc) {
        fprintf(stderr, "%s: out of memory\n", me);
        return 2;
    }
    if (problems)
        fprintf(stderr, "%s: problems in %d of %d files\n", me, problems, nfiles);
    return problems ? 1 : 0;
}
