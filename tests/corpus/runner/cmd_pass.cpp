/*
 * corpusrun pass - one pass of the corpus check in Amiberry (see corpus.h).
 *
 *   corpusrun pass [--update-expect TAG] PASSFILE
 *
 * Exit status 0 = no differences, 1 = differences (report), 77 = skipped
 * (no emulator, or the CD's libraries are missing), 2 = error.  With the
 * pass key record=check a bug-orig pass (orig=) compares the original's
 * observation with the recorded one instead, with record=write it writes
 * it to record_to= (corpus.h).  --update-expect TAG runs the
 * configuration TAG only and writes its outputs to expect_dir=
 * (corpus_update in corpus.h): 0 = all written, 1 = some not.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
#include <string>
#include "cli.hpp"
extern "C" {
#include "corpus.h"
}
#include "cmds.h"

/* the pass of the pass file PATH; UPDATE: the TAG of --update-expect */
static int pass(const char *me, const char *path, const char *update)
{
    struct corpus_pass p;
    char err[300], *text;
    FILE *f;
    long n;
    int rc;

    if (!(f = fopen(path, "rb")) || fseek(f, 0, SEEK_END) || (n = ftell(f)) < 0
        || fseek(f, 0, SEEK_SET) || !(text = (char *)malloc((size_t)n + 1))
        || fread(text, 1, (size_t)n, f) != (size_t)n) {
        fprintf(stderr, "%s: cannot read %s\n", me, path);
        return 2;
    }
    fclose(f);
    text[n] = 0;
    if (corpus_parse(text, &p, err, sizeof err)) {
        fprintf(stderr, "%s: %s: %s\n", me, path, err);
        return 2;
    }
    free(text);
    if (update && corpus_update(&p, update, err, sizeof err)) {
        fprintf(stderr, "%s: %s: %s\n", me, path, err);
        corpus_free(&p);
        return 2;
    }
    /* the configurations of a bug-orig test (orig=; record=: the reference) */
    corpus_select(&p);
    if (corpus_layout(&p, stdout)) {
        fprintf(stderr, "%s: out of memory\n", me);
        corpus_free(&p);
        return 2;
    }
    corpus_skip_missing(&p, stdout);
    rc = corpus_run(&p, stdout);
    corpus_free(&p);
    return rc;
}

void cmd_pass(CLI::App &app, int &status)
{
    struct options {
        std::string update, passfile;
    };
    auto o = std::make_shared<options>();
    CLI::App *sub = app.add_subcommand("pass", "one pass of the corpus check in Amiberry: compile, run and compare the sources of a pass file");
    sub->add_option("--update-expect", o->update,
                    "run only the configuration TAG and write its outputs to the pass's expect_dir= "
                    "(NAME.out, with serial=on NAME.serial) instead of comparing")
        ->type_name("TAG");
    sub->add_option("PASSFILE", o->passfile, "the pass file (its keys: tests/corpus/runner/corpus.h)")->required();
    sub->footer("With the pass key record=check a bug-orig pass (orig=) runs only the original and "
                "compares its observation with the recorded one (1 if it differs), with "
                "record=write it writes the observation to record_to= (the target "
                "bug-orig-record). Exit status 0 no differences (--update-expect: "
                "all written), 1 differences (the report; --update-expect: some not written), 2 "
                "error (usage, pass file), 77 skipped (no emulator, or the CD's libraries are "
                "missing). A bug-orig pass gives the worst verdict: 0 all FIXED, 1 REPRODUCED, 3 "
                "BROKEN, 4 DIFFERS.");
    sub->callback([sub, o, &status] {
        const std::string name = cli::name(*sub);

        if (sub->count("--update-expect") && o->update.empty()) {
            fprintf(stderr, "%s: --update-expect: an empty TAG\nRun with --help for more information.\n",
                    name.c_str());
            status = 2;
            return;
        }
        status = pass(name.c_str(), o->passfile.c_str(), sub->count("--update-expect") ? o->update.c_str() : NULL);
    });
}
