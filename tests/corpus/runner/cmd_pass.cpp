/*
 * corpusrun pass - one pass of the corpus check in Amiberry (see corpus.h).
 *
 *   corpusrun pass [--update-expect TAG [--fix PASS:NAME#N]...]
 *                  [--observe TAG --to DIR [--layout-stack]] [--only NAME]... PASSFILE
 *
 * Exit status 0 = no differences, 1 = differences (report), 77 = skipped
 * (no emulator, or the CD's libraries are missing), 2 = error.  With the
 * pass key record=check a bug-orig pass (orig=) compares the original's
 * observation with the recorded one instead, with record=write it writes
 * it to record_to= (corpus.h).  --update-expect TAG writes the expected
 * data of expect_dir= from the configuration TAG (corpus_update in
 * corpus.h): 0 = all written, 1 = a program refused.  --observe TAG
 * writes the observations of TAG to DIR (corpus_observe, the record of the
 * target corpus-expect-record).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
#include <string>
#include <vector>
#include "cli.hpp"
extern "C" {
#include "corpus.h"
}
#include "cmds.h"

struct pass_options {
    std::string update, observe, to, passfile;
    std::vector<std::string> fix, only;
    bool layout_stack = false;
};

/* the pass of the pass file PATH with the options */
static int pass(const char *me, const pass_options &o)
{
    const char *path = o.passfile.c_str();
    struct corpus_pass p;
    char err[700], *text;
    std::vector<cexp_fix> fixes(o.fix.size());
    std::vector<const char *> only;
    FILE *f;
    long n;
    int rc;

    for (size_t i = 0; i < o.fix.size(); i++)
        if (cexp_parse_fix(o.fix[i].c_str(), &fixes[i], err, sizeof err)) {
            fprintf(stderr, "%s: --fix %s\n", me, err);
            return 2;
        }
    for (const std::string &s : o.only)
        only.push_back(s.c_str());
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
    if ((!o.update.empty() && corpus_update(&p, o.update.c_str(), fixes.data(), fixes.size(), err,
                                            sizeof err))
        || (!o.observe.empty() && corpus_observe(&p, o.observe.c_str(), o.to.c_str(),
                                                 o.layout_stack, err, sizeof err))
        || corpus_only(&p, only.data(), only.size(), err, sizeof err)) {
        fprintf(stderr, "%s: %s: %s\n", me, path, err);
        corpus_free(&p);
        return 2;
    }
    /* the configurations of a bug-orig test (orig=; record=: the reference) */
    corpus_select(&p);
    corpus_layout(&p, stdout);
    corpus_skip_missing(&p, stdout);
    rc = corpus_run(&p, stdout);
    corpus_free(&p);
    return rc;
}

void cmd_pass(CLI::App &app, int &status)
{
    auto o = std::make_shared<pass_options>();
    CLI::App *sub = app.add_subcommand("pass", "one pass of the corpus check in Amiberry: compile, run and compare the sources of a pass file");
    sub->add_option("--update-expect", o->update,
                    "write the expected data of the pass's expect_dir= from the configuration TAG "
                    "instead of comparing (port: the executables of the reference and TAG)")
        ->type_name("TAG");
    sub->add_option("--fix", o->fix, "with --update-expect: a program a fix makes differ from the "
                    "original (any number): its changed file becomes the pair NAME.orig.EXT / "
                    "NAME.fixed-N.EXT")
        ->type_size(1)
        ->expected(1)
        ->allow_extra_args(false)
        ->type_name("PASS:NAME#N");
    sub->add_option("--observe", o->observe,
                    "run only the configuration TAG and write its observations to --to DIR "
                    "(NAME.out, NAME.serial, NAME.exe: the SHA-256 of the executable)")
        ->type_name("TAG");
    sub->add_option("--to", o->to, "with --observe: the directory of the observations "
                    "(emptied first)")
        ->type_name("DIR");
    sub->add_flag("--layout-stack", o->layout_stack, "with --observe: a Stack 16384 bytes larger "
                  "(the second run of the record: another memory layout)");
    sub->add_option("--only", o->only, "only this source (NAME without .bb2; any number)")
        ->type_size(1)
        ->expected(1)
        ->allow_extra_args(false)
        ->type_name("NAME");
    sub->add_option("PASSFILE", o->passfile, "the pass file (its keys: tests/corpus/runner/corpus.h)")->required();
    sub->footer("With the pass key record=check a bug-orig pass (orig=) runs only the original and "
                "compares its observation with the recorded one (1 if it differs), with "
                "record=write it writes the observation to record_to= (the target "
                "bug-orig-record). Exit status 0 no differences (--update-expect: "
                "all written; --observe: observed), 1 differences (the report; --update-expect: "
                "a program refused, nothing written for it), 2 error (usage, pass file, a "
                "malformed expect_dir=), 77 skipped (no emulator, or the CD's libraries are "
                "missing). A bug-orig pass gives the worst verdict: 0 all FIXED, 1 REPRODUCED, 3 "
                "BROKEN, 4 DIFFERS.");
    sub->callback([sub, o, &status] {
        const std::string name = cli::name(*sub);

        const char *bad = NULL;

        if (sub->count("--update-expect") && o->update.empty())
            bad = "--update-expect: an empty TAG";
        else if (sub->count("--update-expect") && sub->count("--observe"))
            bad = "--update-expect and --observe exclude each other";
        else if (sub->count("--fix") && !sub->count("--update-expect"))
            bad = "--fix needs --update-expect";
        else if (!sub->count("--observe") != !sub->count("--to")
                 || (sub->count("--observe") && (o->observe.empty() || o->to.empty())))
            bad = "--observe TAG and --to DIR belong together";
        else if (o->layout_stack && !sub->count("--observe"))
            bad = "--layout-stack needs --observe";
        if (bad) {
            fprintf(stderr, "%s: %s\nRun with --help for more information.\n", name.c_str(), bad);
            status = 2;
            return;
        }
        status = pass(name.c_str(), *o);
    });
}
