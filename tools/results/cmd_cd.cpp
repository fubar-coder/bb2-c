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
#include <array>
#include <memory>
#include <string>
#include <vector>
#include "cli.hpp"
extern "C" {
#include "cdtable.h"
}
#include "results.h"

static char *read_all(const char *path)
{
    FILE *f = fopen(path, "rb");
    char *s = NULL;
    long n = 0;

    if (!f)
        return NULL;
    if (!fseek(f, 0, SEEK_END) && (n = ftell(f)) >= 0 && !fseek(f, 0, SEEK_SET)
        && (s = (char *)malloc((size_t)n + 1)) && fread(s, 1, (size_t)n, f) == (size_t)n)
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

static int run(enum cdtable_mode mode, const char *me, const char *fresh_path, const char *old_path,
               const char *out_path)
{
    char *fresh, *old, *out, *report;
    int rc;

    fresh = read_all(fresh_path);
    old = read_all(old_path);
    if (!fresh || !old) {
        fprintf(stderr, "%s: cannot read %s\n", me, fresh ? old_path : fresh_path);
        free(fresh);
        free(old);
        return 2;
    }
    rc = cdtable_merge(mode, fresh, old, &out, &report);
    fputs(report, rc ? stderr : stdout);
    if (!rc) {
        FILE *f = fopen(out_path, "wb");

        if (!f || fwrite(out, 1, strlen(out), f) != strlen(out) || fclose(f)) {
            fprintf(stderr, "%s: cannot write %s\n", me, out_path);
            rc = 2;
        }
    }
    free(fresh);
    free(old);
    free(out);
    free(report);
    return rc;
}

/* cd-update or cd-reference */
static void add(CLI::App &app, int &status, enum cdtable_mode mode, const char *name,
                const char *description, const char *fresh, const char *footer)
{
    auto paths = std::make_shared<std::array<std::string, 3>>();
    CLI::App *sub = app.add_subcommand(name, description);
    sub->add_option("NEW", (*paths)[0], fresh)->required();
    sub->add_option("OLD", (*paths)[1], "the table so far (tests/data/blitz2-cd.tsv)")->required();
    sub->add_option("OUT", (*paths)[2], "the table written (may be OLD)")->required();
    sub->footer(footer);
    sub->callback([sub, paths, mode, &status] {
        const std::string me = cli::name(*sub);

        status = run(mode, me.c_str(), (*paths)[0].c_str(), (*paths)[1].c_str(), (*paths)[2].c_str());
    });
}

void cmd_cd_update(CLI::App &app, int &status)
{
    add(app, status, CDTABLE_UPDATE, "cd-update", "the CD result table from the port's fresh run",
        "the port's fresh run",
        "Rows whose note starts with \"#NNN:\" and names the CD's result as \"(2.10: HEX)\" (the "
        "masked hash) or \"(2.10: error LINE)\" (refused at that line) are note rows: the note "
        "stays unless the port's result is the CD's again. Exit status 0 OUT written, 1 the "
        "tables do not fit (the report on stderr), 2 usage error or a file that cannot be read "
        "or written.");
}

void cmd_cd_reference(CLI::App &app, int &status)
{
    add(app, status, CDTABLE_REFERENCE, "cd-reference", "the CD result table from the original's fresh run",
        "the original's fresh run",
        "Rows whose note starts with \"#NNN:\" and names the CD's result as \"(2.10: HEX)\" (the "
        "masked hash) or \"(2.10: error LINE)\" (refused at that line) are note rows: the row "
        "stays, and its note must name NEW's result. Exit status 0 OUT written, 1 the tables do "
        "not fit (the report on stderr), 2 usage error or a file that cannot be read or "
        "written.");
}
