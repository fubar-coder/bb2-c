/*
 * results cd-update, cd-reference - the CD result table written from a
 * fresh run, keeping the note rows (lib/cdtable/cdtable.h).
 *
 *   results cd-update    NEW OLD OUT    NEW: the port's run, OLD: the table
 *   results cd-reference NEW OLD OUT    NEW: the original's run
 *
 * OUT may be OLD.  Exit status 0 = written (the report on stdout), 1 =
 * errors (nothing written, the report on stderr), 2 = wrong arguments or
 * a file that cannot be read or written.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cdtable.h"
#include "results.h"

const char cmd_cd_update_usage[] =
    "usage: results cd-update NEW OLD OUT\n"
    "Write the CD result table OUT from the port's fresh run NEW and the table OLD\n"
    "(OUT may be OLD).  Rows whose note starts with \"#NNN:\" and names the CD's\n"
    "result as \"(2.10: HEX)\" (the masked hash) or \"(2.10: error LINE)\" (refused\n"
    "at that line) are note rows: the note stays unless the port's result is the\n"
    "CD's again.\n";
const char cmd_cd_reference_usage[] =
    "usage: results cd-reference NEW OLD OUT\n"
    "Write the CD result table OUT from the original's fresh run NEW and the table\n"
    "OLD (OUT may be OLD).  Rows whose note starts with \"#NNN:\" and names the CD's\n"
    "result as \"(2.10: HEX)\" (the masked hash) or \"(2.10: error LINE)\" (refused\n"
    "at that line) are note rows: the row stays, and its note must name NEW's\n"
    "result.\n";

static char *read_all(const char *path)
{
    FILE *f = fopen(path, "rb");
    char *s = NULL;
    long n = 0;

    if (!f)
        return NULL;
    if (!fseek(f, 0, SEEK_END) && (n = ftell(f)) >= 0 && !fseek(f, 0, SEEK_SET)
        && (s = malloc((size_t)n + 1)) && fread(s, 1, (size_t)n, f) == (size_t)n)
        s[n] = 0;
    else {
        free(s);
        s = NULL;
    }
    fclose(f);
    if (s && strlen(s) != (size_t)n) {  /* a 0 byte */
        free(s);
        return NULL;
    }
    return s;
}

static int run(enum cdtable_mode mode, const char *usage, int argc, char **argv)
{
    const char *me = argv[0]; /* "results cd-...", the prefix of the messages */
    char *fresh, *old, *out, *report;
    int rc;

    if (argc != 4) {
        fputs(usage, stderr);
        return 2;
    }
    fresh = read_all(argv[1]);
    old = read_all(argv[2]);
    if (!fresh || !old) {
        fprintf(stderr, "%s: cannot read %s\n", me, fresh ? argv[2] : argv[1]);
        free(fresh);
        free(old);
        return 2;
    }
    rc = cdtable_merge(mode, fresh, old, &out, &report);
    fputs(report, rc ? stderr : stdout);
    if (!rc) {
        FILE *f = fopen(argv[3], "wb");

        if (!f || fwrite(out, 1, strlen(out), f) != strlen(out) || fclose(f)) {
            fprintf(stderr, "%s: cannot write %s\n", me, argv[3]);
            rc = 2;
        }
    }
    free(fresh);
    free(old);
    free(out);
    free(report);
    return rc;
}

int cmd_cd_update(int argc, char **argv)
{
    return run(CDTABLE_UPDATE, cmd_cd_update_usage, argc, argv);
}

int cmd_cd_reference(int argc, char **argv)
{
    return run(CDTABLE_REFERENCE, cmd_cd_reference_usage, argc, argv);
}
