/*
 * amiga_run - run an AmigaDOS script in Amiberry (tools/lib/amiga_emulation).
 *
 *   amiga_run [--machine NAME] [--rtg] [--visible] [--log] [--timeout SEC]
 *             [--set KEY=VALUE]... [--unset KEY]... [--mount VOL=DIR]...
 *             [--mount-ro VOL=DIR]... [--dir WORKDIR] [--cold] SCRIPT|-
 *
 * --machine: a1200 (the default), a500 (68000, Kickstart 1.3),
 * a500-68010 or a600 (68000, Kickstart 3.1) (amiga_emulation.h).
 * The script runs with TEST: = WORKDIR/test.  Without --dir the work
 * directory is temporary: the files the script wrote to TEST: are printed
 * and the directory is removed; with --dir everything stays.  Exit status
 * 0 if the script ran to its end, 1 otherwise, 77 if the emulator cannot
 * start (Amiberry or its data missing: skipped), 2 on usage errors.
 * --log: Amiberry's own log into WORKDIR/amiberry.log (with --dir).
 * --cold: boot, do not start from a snapshot (amiga_emulation.h).
 *
 *   amiga_run --purge-snapshots [OWNER]
 *
 * removes the snapshots of OWNER (a build directory, as in
 * $AMIGA_EMULATION_OWNER; without it $AMIGA_EMULATION_OWNER, unset: the
 * snapshots without owner, those of amiga_run by hand) from all slots and
 * nothing else (amiga_emulation_snapshot_purge); exit status 0, also
 * without a slot directory, 2 on usage errors.  For manual use: the tests
 * keep their snapshots across runs, and a session discards one whose
 * inputs have changed by itself (tools/lib/amiga_emulation/README.md, "Snapshot
 * mode").
 */
#define _XOPEN_SOURCE 700       /* scandir */
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include "cli.hpp"
extern "C" {
#include "amiga_emulation.h"
}

static char *read_all(FILE *f)
{
    size_t n = 0, cap = 4096;
    char *s = (char *)malloc(cap);

    while (s) {
        size_t r = fread(s + n, 1, cap - n - 1, f);

        n += r;
        if (r == 0)
            break;
        if (n + 1 == cap) {
            char *t = (char *)realloc(s, cap *= 2);

            if (!t) {
                free(s);
                return NULL;
            }
            s = t;
        }
    }
    if (s)
        s[n] = 0;
    return s;
}

static int skip_entry(const struct dirent *d)
{
    return strcmp(d->d_name, ".") && strcmp(d->d_name, "..") && strcmp(d->d_name, "run.script")
           && strcmp(d->d_name, "run.done");
}

static void print_results(const char *dir)
{
    struct dirent **list;
    int i, n = scandir(dir, &list, skip_entry, alphasort);

    for (i = 0; i < n; i++) {
        char path[4096];
        FILE *f;

        snprintf(path, sizeof path, "%s/%s", dir, list[i]->d_name);
        if ((f = fopen(path, "rb")) != NULL) {
            char buf[4096];
            size_t r;

            printf("==> %s <==\n", list[i]->d_name);
            fflush(stdout);
            while ((r = fread(buf, 1, sizeof buf, f)) > 0)
                fwrite(buf, 1, r, stdout);
            fclose(f);
        }
        free(list[i]);
    }
    if (n >= 0)
        free(list);
}

/* --purge-snapshots [OWNER]: no slot directory yet is not an error */
static int purge(const char *owner)
{
    int n;

    if (owner && !*owner)
        owner = NULL;
    n = amiga_emulation_snapshot_purge(owner);
    if (n < 0)
        printf("amiga_run: no snapshot directory, nothing to purge\n");
    else
        printf("amiga_run: %d snapshot%s of %s removed\n", n, n == 1 ? "" : "s",
               owner ? owner : "no owner");
    return 0;
}

/* the script SCRIPT_PATH (- stdin) run with P */
static int run(struct amiga_emulation_params *p, const char *script_path)
{
    struct amiga_emulation *e;
    enum amiga_emulation_status st;
    char *script, err[300];
    FILE *f;

    f = strcmp(script_path, "-") ? fopen(script_path, "r") : stdin;
    if (!f) {
        perror(script_path);
        return 2;
    }
    script = read_all(f);
    if (f != stdin)
        fclose(f);
    if (!script)
        return 2;
    p->script = script;

    if (!(e = amiga_emulation_start(p, err, sizeof err))) {
        fprintf(stderr, "amiga_run: %s\n", err);
        free(script);
        return 77;                      /* no emulator (or its data): skipped */
    }
    st = amiga_emulation_join(e);
    if (p->work_dir)
        printf("work directory: %s\n", amiga_emulation_work_dir(e));
    else
        print_results(amiga_emulation_test_dir(e));
    if (st != AMIGA_EMULATION_DONE)
        fprintf(stderr, "amiga_run: %s%s\n", amiga_emulation_status_name(st),
                (st == AMIGA_EMULATION_TIMEOUT || st == AMIGA_EMULATION_HALT) && p->work_dir
                    ? " (screenshot: timeout.png)" : "");
    amiga_emulation_free(e);
    free(script);
    return st == AMIGA_EMULATION_DONE ? 0 : 1;
}

/* a usage error: the message and the hint, status 2 */
static int usage_error(const std::string &msg)
{
    fprintf(stderr, "amiga_run: %s\nRun with --help for more information.\n", msg.c_str());
    return 2;
}

int main(int argc, char **argv)
{
    CLI::App app{"Run an AmigaDOS script (a file or - for stdin) in Amiberry.", "amiga_run"};
    std::string machine, timeout = "60", dir, script;
    std::vector<std::string> set, unset, mount, mount_ro;
    bool visible = false, log = false, cold = false;
    int status = 0;

    app.add_option("--machine", machine,
                   "a1200 (default), a500 (68000, Kickstart 1.3), a500-68010, a600 (68000, Kickstart 3.1)")
        ->type_name("NAME")
        ->multi_option_policy(CLI::MultiOptionPolicy::TakeLast);
    app.add_flag("--rtg", "the RTG board and its options (amiga_emulation_rtg)");
    app.add_flag("--visible", visible, "show Amiberry's window");
    app.add_flag("--log", log, "Amiberry's own log into WORKDIR/amiberry.log (with --dir)");
    app.add_option("--timeout", timeout, "seconds for the run, boot or restore included (default 60)")
        ->type_name("SEC")
        ->multi_option_policy(CLI::MultiOptionPolicy::TakeLast);
    cli::add_repeated(app, "--set", set, 1, "a change of the machine's configuration")->type_name("KEY=VALUE");
    cli::add_repeated(app, "--unset", unset, 1, "a key removed from the configuration")->type_name("KEY");
    cli::add_repeated(app, "--mount", mount, 1, "a host directory as an AmigaDOS volume")->type_name("VOL=DIR");
    cli::add_repeated(app, "--mount-ro", mount_ro, 1, "the same, read-only")->type_name("VOL=DIR");
    app.add_option("--dir", dir, "the work directory, kept (TEST: = WORKDIR/test); default: temporary")
        ->type_name("WORKDIR")
        ->multi_option_policy(CLI::MultiOptionPolicy::TakeLast);
    app.add_flag("--cold", cold, "boot, do not start from a snapshot");
    CLI::Option *script_opt =
        app.add_option("SCRIPT", script, "the AmigaDOS script, - for stdin; with --purge-snapshots: OWNER");
    CLI::Option *purge_opt =
        app.add_flag("--purge-snapshots",
                     "instead of a run: remove the snapshots of OWNER (a build directory; default "
                     "$AMIGA_EMULATION_OWNER, unset: those without owner) from all slots; no other "
                     "option");
    app.footer("The script runs with TEST: = WORKDIR/test. Without --dir the files the script "
               "wrote to TEST: are printed and the work directory is removed. Exit status 0 the "
               "script ran to its end (--purge-snapshots: also without a slot directory), 1 it did "
               "not, 2 usage error, 77 the emulator cannot start (Amiberry or its data missing: "
               "skipped).");
    app.callback([&] {
        struct amiga_emulation_params p = {};
        std::vector<struct amiga_emulation_option> changes;
        std::vector<struct amiga_emulation_mount> mounts;
        std::vector<std::string> keys;   // the KEY of every --set, the VOL of every --mount

        if (purge_opt->count()) {
            const char *o = script_opt->count() ? script.c_str() : getenv("AMIGA_EMULATION_OWNER");

            if (script_opt->count() && script.empty()) {
                status = usage_error("--purge-snapshots: an empty OWNER");
                return;
            }
            for (const CLI::Option *opt : app.get_options())
                if (opt != purge_opt && opt != script_opt && opt->count()) {
                    status = usage_error("--purge-snapshots excludes " + opt->get_name());
                    return;
                }
            status = purge(o);
            return;
        }
        if (!script_opt->count()) {
            status = usage_error("SCRIPT is required");
            return;
        }
        if (app.count("--machine") && !amiga_emulation_machine_known(machine.c_str())) {
            status = usage_error("unknown machine \"" + machine + "\"");
            return;
        }
        // the changes and mounts in the order given (the later wins)
        std::vector<cli::occurrence> occ = cli::occurrences(app);
        keys.reserve(occ.size());
        for (const cli::occurrence &oc : occ) {
            const std::string name = oc.option->get_name();
            const std::string v = oc.values.empty() ? "" : oc.values[0];
            std::size_t eq = v.find('=');

            if (name == "--rtg")
                changes.insert(changes.end(), amiga_emulation_rtg, amiga_emulation_rtg + amiga_emulation_rtg_count);
            else if (name == "--unset") {
                keys.push_back(v);
                changes.push_back({ keys.back().c_str(), NULL });
            } else if (name == "--set" || name == "--mount" || name == "--mount-ro") {
                if (eq == std::string::npos) {
                    status = usage_error(name + ": " + (name == "--set" ? "KEY=VALUE" : "VOL=DIR") +
                                         " expected, not \"" + v + "\"");
                    return;
                }
                keys.push_back(v);
                keys.back()[eq] = 0;
                const char *k = keys.back().c_str();
                if (name == "--set")
                    changes.push_back({ k, k + eq + 1 });
                else
                    mounts.push_back({ k, k + eq + 1, name == "--mount" });
            }
        }
        p.machine = app.count("--machine") ? machine.c_str() : NULL;
        p.changes = changes.data();
        p.nchanges = changes.size();
        p.mounts = mounts.data();
        p.nmounts = mounts.size();
        p.timeout = (unsigned)strtoul(timeout.c_str(), NULL, 10);
        p.visible = visible;
        p.log = log;
        p.cold = cold;
        p.work_dir = app.count("--dir") ? dir.c_str() : NULL;
        status = run(&p, script.c_str());
    });
    return cli::run(app, argc, argv, status);
}
