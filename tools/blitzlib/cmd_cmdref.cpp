/*
 * blitzlib cmdref - the command table of a Blitz Basic 2 library file as JSON, the
 * data of the website's command reference (format: lib/cmdref/cmdref.h).
 *
 *   blitzlib cmdref [-o OUT] --group GROUP --name NAME LIB.obj
 *
 * Without -o the JSON goes to stdout.  Exit status 1 and no OUT if the
 * file is not a readable library file.
 */
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <memory>
#include <string>
#include "cli.hpp"
extern "C" {
#include "cmdref.h"
}
#include "cmds.h"

static const char *me; /* "blitzlib cmdref", the prefix of the messages */

/* the JSON of the library file PATH to OUT (NULL: stdout) */
static int cmdref(const char *out, const char *group, const char *name, const char *path)
{
    const char *opens;
    struct loadfile lf;
    struct blitzlib lib;
    char err[300];
    int rc = 1;
    FILE *f;

    if (out)
        remove(out);                    /* no stale file if this run fails */
    if (load_read(path, &lf, err, sizeof err)) {
        fprintf(stderr, "%s: %s: %s\n", me, path, err);
        return 1;
    }
    if (lf.nhunks != 1 || blitzlib_read(&lf.hunks[0], &lib, err, sizeof err)) {
        fprintf(stderr, "%s: %s: %s\n", me, path,
                lf.nhunks != 1 ? "not one hunk (not a library file)" : err);
        load_free(&lf);
        return 1;
    }
    if (cmdref_opens(&lib, &opens, err, sizeof err)) {
        fprintf(stderr, "%s: %s: %s\n", me, path, err);
        goto done;
    }
    f = out ? fopen(out, "w") : stdout;
    if (!f) {
        fprintf(stderr, "%s: %s: %s\n", me, out, strerror(errno));
        goto done;
    }
    if (cmdref_write(f, &lib, group, name, opens) || (out ? fclose(f) : fflush(f))) {
        fprintf(stderr, "%s: %s: %s\n", me, out ? out : "stdout", strerror(errno));
        if (out)
            remove(out);
        goto done;
    }
    rc = 0;
done:
    blitzlib_free(&lib);
    load_free(&lf);
    return rc;
}

void cmd_cmdref(CLI::App &app, int &status)
{
    struct options {
        std::string out, group, name, lib;
    };
    auto o = std::make_shared<options>();
    CLI::App *sub = app.add_subcommand("cmdref", "the command table of a Blitz Basic 2 library file as JSON");
    sub->add_option("-o", o->out, "the JSON written (default: stdout); removed first")->type_name("OUT");
    sub->add_option("--group", o->group, "the group of the library (acidlibs, deflibs, ...)")
        ->type_name("GROUP")
        ->required();
    sub->add_option("--name", o->name, "the name of the library")->type_name("NAME")->required();
    sub->add_option("LIB.obj", o->lib, "the library file")->required();
    sub->footer("The data of the website's command reference, site/data/libraries/GROUP/NAME.json. "
                "Exit status 0, 1 LIB.obj is no readable library file or OUT cannot be written "
                "(no OUT is left), 2 usage error.");
    sub->callback([sub, o, &status] {
        const std::string name = cli::name(*sub);

        me = name.c_str();
        status = cmdref(sub->count("-o") ? o->out.c_str() : NULL, o->group.c_str(), o->name.c_str(),
                        o->lib.c_str());
    });
}
