/*
 * refcheck data - data of a C build against the reference build of the
 * assembler original (see checkdata.h).
 *
 *   refcheck data [-v|-q] TABLE LISTING REFERENCE CBUILD [INTENDED]
 *   e.g. refcheck data programs/ted/checkdata.tsv ref/ted.lst ref/ted ted.sym
 *
 * INTENDED: the list of intended differences (fixed original bugs,
 * "CSYM<TAB>#NNN<TAB>NOTE" per line): those checks may differ, and each of
 * them must (a stale entry fails).
 *
 * Prints the failed checks and a summary (-q: only if a check failed, -v:
 * every check); exit status 1 if a check failed or an entry is stale.
 */
#define _XOPEN_SOURCE 700       /* open_memstream */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
#include <string>
#include <vector>
#include "cli.hpp"
extern "C" {
#include "checkdata.h"
}
#include "refcheck.h"

static char *read_all(const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb");
    char *s = NULL;
    long n;

    if (!f)
        return NULL;
    if (!fseek(f, 0, SEEK_END) && (n = ftell(f)) >= 0 && !fseek(f, 0, SEEK_SET)
        && (s = (char *)malloc((size_t)n + 1)) && fread(s, 1, (size_t)n, f) == (size_t)n) {
        s[n] = 0;
        *len = (size_t)n;
    } else {
        free(s);
        s = NULL;
    }
    fclose(f);
    return s;
}

static const char *me; /* "refcheck data", the prefix of the messages */

struct data_options {
    bool verbose = false, quiet = false;
    std::string table, listing, reference, cbuild, intended;
};

static int run(const data_options &o)
{
    struct checkdata_table table;
    struct checkdata_intended intended = { NULL, 0 };
    struct listing listing;
    struct loadfile ref, c;
    char err[200], *text, *report = NULL;
    size_t len, reportlen = 0;
    FILE *out;
    long bad;
    const bool with_intended = !o.intended.empty();

    if (!(text = read_all(o.table.c_str(), &len))) {
        fprintf(stderr, "%s: cannot read %s\n", me, o.table.c_str());
        return 1;
    }
    if (checkdata_table_parse(text, len, &table, err, sizeof err)) {
        fprintf(stderr, "%s: %s: %s\n", me, o.table.c_str(), err);
        return 1;
    }
    free(text);
    if (with_intended) {
        if (!(text = read_all(o.intended.c_str(), &len))) {
            fprintf(stderr, "%s: cannot read %s\n", me, o.intended.c_str());
            return 1;
        }
        if (checkdata_intended_parse(text, len, &intended, err, sizeof err)) {
            fprintf(stderr, "%s: %s: %s\n", me, o.intended.c_str(), err);
            return 1;
        }
        free(text);
    }
    if (listing_read(o.listing.c_str(), &listing, err, sizeof err)) {
        fprintf(stderr, "%s: %s: %s\n", me, o.listing.c_str(), err);
        return 1;
    }
    if (load_read(o.reference.c_str(), &ref, err, sizeof err) || ref.nhunks < 1) {
        fprintf(stderr, "%s: %s: %s\n", me, o.reference.c_str(), ref.nhunks ? err : "no hunk");
        return 1;
    }
    if (load_read(o.cbuild.c_str(), &c, err, sizeof err)) {
        fprintf(stderr, "%s: %s: %s\n", me, o.cbuild.c_str(), err);
        return 1;
    }
    out = o.quiet ? open_memstream(&report, &reportlen) : stdout;
    if (!out)
        return 1;
    bad = checkdata_run(&table, &listing, &ref.hunks[0], &c, with_intended ? &intended : NULL,
                        o.verbose, out);
    if (o.quiet) {
        fclose(out);
        if (bad)
            fwrite(report, 1, reportlen, stdout);
        free(report);
    }
    load_free(&c);
    load_free(&ref);
    listing_free(&listing);
    checkdata_table_free(&table);
    checkdata_intended_free(&intended);
    return bad != 0;
}

void cmd_data(CLI::App &app, int &status)
{
    auto o = std::make_shared<data_options>();
    CLI::App *sub = app.add_subcommand("data", "data of a C build against the reference build of the assembler original");
    CLI::Option *v = sub->add_flag("-v", o->verbose, "every check");
    sub->add_flag("-q", o->quiet, "the report only if a check failed")->excludes(v);
    sub->add_option("TABLE", o->table, "the checks (e.g. programs/ted/checkdata.tsv)")->required();
    sub->add_option("LISTING", o->listing, "the vasm listing of the reference build")->required();
    sub->add_option("REFERENCE", o->reference, "the reference build (a load file)")->required();
    sub->add_option("CBUILD", o->cbuild, "the C build with symbols")->required();
    sub->add_option("INTENDED", o->intended,
                    "the checks that differ on purpose (CSYM<TAB>#ISSUE<TAB>NOTE per line), each of "
                    "them must differ");
    sub->footer("Prints the failed checks and a summary. Exit status 0, 1 a check failed, an "
                "INTENDED entry is stale or an input cannot be read, 2 usage error.");
    sub->callback([sub, o, &status] {
        const std::string name = cli::name(*sub);

        me = name.c_str();
        status = run(*o);
    });
}
