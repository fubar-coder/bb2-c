/*
 * results bug-orig-accept - the recorded original of a bug-orig test
 * (ID.orig) from two recording runs of the original
 * (lib/bugorigcmp/bugorigcmp.h, OpenSpec change bug-orig-test-model,
 * design D4).
 *
 *   results bug-orig-accept RUN1 RUN2 OUT [--id ID]
 *
 * Exit status 0 = OUT written or unchanged, 1 = refused (the runs differ,
 * a wildcard of OUT does not match them, a line reads as a wildcard;
 * nothing written), 2 = wrong arguments, a malformed, unreadable or
 * unwritable file.
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
#include <string>
#include "cli.hpp"
extern "C" {
#include "bugorigcmp.h"
}
#include "results.h"

/* the file (malloc'd, 0-terminated); NULL: missing (*missing) or unreadable */
static char *read_text(const char *path, int *missing)
{
    FILE *f = fopen(path, "rb");
    std::string s;
    char buf[8192];
    size_t n;

    *missing = !f && errno == ENOENT;
    if (!f)
        return NULL;
    while ((n = fread(buf, 1, sizeof buf, f)) > 0)
        s.append(buf, n);
    int bad = ferror(f);
    fclose(f);
    if (bad || memchr(s.data(), 0, s.size()))
        return NULL;
    char *r = (char *)malloc(s.size() + 1);
    if (r)
        memcpy(r, s.c_str(), s.size() + 1);
    return r;
}

/* the ID of OUT: its name without directory and .orig */
static std::string id_of(const std::string &out)
{
    std::string n = out.substr(out.find_last_of('/') == std::string::npos ? 0 : out.find_last_of('/') + 1);

    if (n.size() > 5 && n.compare(n.size() - 5, 5, ".orig") == 0)
        n.resize(n.size() - 5);
    return n;
}

static int accept(const std::string &run1, const std::string &run2, const std::string &out,
                  const std::string &id, FILE *err)
{
    int miss;
    char *a = read_text(run1.c_str(), &miss), *b = NULL, *old = NULL, *text = NULL, msg[700];
    int r = 2;

    if (!a) {
        fprintf(err, "%s: cannot read it\n", run1.c_str());
        return 2;
    }
    if (!(b = read_text(run2.c_str(), &miss))) {
        fprintf(err, "%s: cannot read it\n", run2.c_str());
        goto done;
    }
    if (!(old = read_text(out.c_str(), &miss)) && !miss) {
        fprintf(err, "%s: cannot read it\n", out.c_str());
        goto done;
    }
    r = bugorigcmp_accept(id.c_str(), a, run1.c_str(), b, run2.c_str(), old, out.c_str(), &text,
                          stdout, msg, sizeof msg);
    if (r < 0) {
        fprintf(err, "%s\n", msg);
        r = 2;
    } else if (r == 0) {
        if (old && !strcmp(old, text))
            printf("%s: %s unchanged\n", id.c_str(), out.c_str());
        else {
            FILE *f = fopen(out.c_str(), "wb");
            size_t n = strlen(text);

            if (!f || fwrite(text, 1, n, f) != n || fclose(f)) {
                fprintf(err, "%s: cannot write it\n", out.c_str());
                r = 2;
            } else
                printf("%s: recorded %s\n", id.c_str(), out.c_str());
        }
    }
done:
    free(a);
    free(b);
    free(old);
    free(text);
    return r;
}

void cmd_bug_orig_accept(CLI::App &app, int &status)
{
    struct options {
        std::string run1, run2, out, id;
    };
    auto o = std::make_shared<options>();
    CLI::App *sub = app.add_subcommand("bug-orig-accept", "the recorded original of a bug-orig test from two recording runs of the original");
    sub->add_option("RUN1", o->run1, "the observation of the first run")->required();
    sub->add_option("RUN2", o->run2, "the observation of the second run, another memory layout")->required();
    sub->add_option("OUT", o->out, "the recorded original written (ID.orig)")->required();
    sub->add_option("--id", o->id, "the test's ID in the messages (default: the name of OUT)")
        ->type_name("ID");
    sub->footer("The runs must agree, compared as cases (\"test not deterministic: ID\" and the "
                "differing sections otherwise); a section of OUT with wildcard lines (behind "
                "\"# varies: REASON\") is kept if both runs match it. Exit status 0 OUT written or "
                "unchanged, 1 refused (nothing written), 2 usage error or a malformed, unreadable "
                "or unwritable file.");
    sub->callback([sub, o, &status] {
        const std::string name = cli::name(*sub);
        const std::string id = o->id.empty() ? id_of(o->out) : o->id;

        status = prefixed(name, [&](FILE *err) { return accept(o->run1, o->run2, o->out, id, err); });
    });
}
