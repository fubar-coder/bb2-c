/*
 * results ted-oracle - the oracle data of the TED tests written from a
 * fresh run of the original TED, keeping the note rows
 * (lib/oracletable/oracletable.h).
 *
 *   results ted-oracle NEW OLD OUT    NEW: the original's run, OLD: the file
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
#include "oracletable.h"
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

void cmd_ted_oracle(CLI::App &app, int &status)
{
    auto paths = std::make_shared<std::array<std::string, 3>>();
    CLI::App *sub = app.add_subcommand("ted-oracle", "the oracle data of the TED tests from the original TED's fresh run");
    sub->add_option("NEW", (*paths)[0], "the original TED's fresh run")->required();
    sub->add_option("OLD", (*paths)[1], "the oracle data so far (tests/data/ted-oracle.sha256)")->required();
    sub->add_option("OUT", (*paths)[2], "the file written (may be OLD)")->required();
    sub->footer("Rows with a note \"#NNN: TEXT (TED 1.1: HASH)\" behind a tab are note rows (the "
                "port's hash after a fix): the row stays, and its note must name NEW's hash. Exit "
                "status 0 OUT written, 1 the files do not fit (the report on stderr), 2 usage "
                "error or a file that cannot be read or written.");
    sub->callback([sub, paths, &status] {
        const std::string name = cli::name(*sub);
        const char *me = name.c_str();
        const char *fresh_path = (*paths)[0].c_str(), *old_path = (*paths)[1].c_str(),
                   *out_path = (*paths)[2].c_str();
        char *fresh, *old, *out, *report;
        int rc;

        fresh = read_all(fresh_path);
        old = read_all(old_path);
        if (!fresh || !old) {
            fprintf(stderr, "%s: %s: cannot read it\n", me, fresh ? old_path : fresh_path);
            free(fresh);
            free(old);
            status = 2;
            return;
        }
        rc = oracletable_merge(fresh, old, &out, &report);
        fputs(report, rc ? stderr : stdout);
        if (!rc) {
            FILE *f = fopen(out_path, "wb");

            if (!f || fwrite(out, 1, strlen(out), f) != strlen(out) || fclose(f)) {
                fprintf(stderr, "%s: %s: cannot write it\n", me, out_path);
                rc = 2;
            }
        }
        free(fresh);
        free(old);
        free(out);
        free(report);
        status = rc;
    });
}
