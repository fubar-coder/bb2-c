/*
 * amiga_run - run an AmigaDOS script in Amiberry (tools/lib/amiga_emulation).
 *
 *   amiga_run [--machine NAME] [--rtg] [--visible] [--log] [--timeout SEC]
 *             [--set KEY=VALUE]... [--unset KEY]... [--mount VOL=DIR]...
 *             [--mount-ro VOL=DIR]... [--dir WORKDIR] [--snapshot] SCRIPT|-
 *
 * --machine: a1200 (the default), a500 (68000, Kickstart 1.3) or
 * a500-68010 (amiga_emulation.h).
 * The script runs with TEST: = WORKDIR/test.  Without --dir the work
 * directory is temporary: the files the script wrote to TEST: are printed
 * and the directory is removed; with --dir everything stays.  Exit status
 * 0 if the script ran to its end, 1 otherwise, 77 if the emulator cannot
 * start (Amiberry or its data missing: skipped), 2 on usage errors.
 * --log: Amiberry's own log into WORKDIR/amiberry.log (with --dir).
 * --snapshot: EXPERIMENT, start from a savestate (amiga_emulation.h).
 */
#define _XOPEN_SOURCE 700       /* scandir */
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "amiga_emulation.h"

static char *read_all(FILE *f)
{
    size_t n = 0, cap = 4096;
    char *s = malloc(cap);

    while (s) {
        size_t r = fread(s + n, 1, cap - n - 1, f);

        n += r;
        if (r == 0)
            break;
        if (n + 1 == cap) {
            char *t = realloc(s, cap *= 2);

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

/* to stdout for -h/--help (0), else to stderr (2) */
static int usage(FILE *out)
{
    fprintf(out, "usage: amiga_run [--machine NAME] [--rtg] [--visible] [--log] [--timeout SEC]\n"
                 "                 [--set KEY=VALUE]... [--unset KEY]... [--mount VOL=DIR]...\n"
                 "                 [--mount-ro VOL=DIR]... [--dir WORKDIR] [--snapshot] SCRIPT|-\n"
                 "  run an AmigaDOS script (a file or - for stdin) in Amiberry\n"
                 "  --machine: a1200 (default), a500 (68000, Kickstart 1.3), a500-68010\n");
    return out == stdout ? 0 : 2;
}

int main(int argc, char **argv)
{
    struct amiga_emulation_params p = { 0 };
    struct amiga_emulation_option *changes = calloc((size_t)argc + amiga_emulation_rtg_count,
                                                    sizeof *changes);
    struct amiga_emulation_mount *mounts = calloc((size_t)argc, sizeof *mounts);
    struct amiga_emulation *e;
    enum amiga_emulation_status st;
    const char *script_path = NULL;
    char *script, err[300];
    FILE *f;
    int i;

    if (argc == 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
        free(changes);
        free(mounts);
        return usage(stdout);
    }
    if (!changes || !mounts)
        return 2;
    p.changes = changes;
    p.mounts = mounts;
    p.timeout = 60;
    for (i = 1; i < argc; i++) {
        const char *a = argv[i];
        char *eq;

        if (!strcmp(a, "--machine") && i + 1 < argc) {
            p.machine = argv[++i];
            if (!amiga_emulation_machine_known(p.machine)) {
                fprintf(stderr, "amiga_run: unknown machine \"%s\"\n", p.machine);
                return usage(stderr);
            }
        } else if (!strcmp(a, "--rtg")) {
            memcpy(changes + p.nchanges, amiga_emulation_rtg,
                   amiga_emulation_rtg_count * sizeof *changes);
            p.nchanges += amiga_emulation_rtg_count;
        } else if (!strcmp(a, "--visible")) {
            p.visible = 1;
        } else if (!strcmp(a, "--log")) {
            p.log = 1;
        } else if (!strcmp(a, "--snapshot")) {
            p.snapshot = 1;
        } else if (!strcmp(a, "--timeout") && i + 1 < argc) {
            p.timeout = (unsigned)strtoul(argv[++i], NULL, 10);
        } else if (!strcmp(a, "--set") && i + 1 < argc && (eq = strchr(argv[i + 1], '='))) {
            *eq = 0;
            changes[p.nchanges].key = argv[++i];
            changes[p.nchanges++].value = eq + 1;
        } else if (!strcmp(a, "--unset") && i + 1 < argc) {
            changes[p.nchanges++].key = argv[++i];
        } else if ((!strcmp(a, "--mount") || !strcmp(a, "--mount-ro")) && i + 1 < argc
                   && (eq = strchr(argv[i + 1], '='))) {
            *eq = 0;
            mounts[p.nmounts].volume = argv[++i];
            mounts[p.nmounts].host_dir = eq + 1;
            mounts[p.nmounts++].writable = !strcmp(a, "--mount");
        } else if (!strcmp(a, "--dir") && i + 1 < argc) {
            p.work_dir = argv[++i];
        } else if (a[0] == '-' && a[1]) {
            return usage(stderr);
        } else if (!script_path) {
            script_path = a;
        } else {
            return usage(stderr);
        }
    }
    if (!script_path)
        return usage(stderr);
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
    p.script = script;

    if (!(e = amiga_emulation_start(&p, err, sizeof err))) {
        fprintf(stderr, "amiga_run: %s\n", err);
        return 77;                      /* no emulator (or its data): skipped */
    }
    st = amiga_emulation_join(e);
    if (p.work_dir)
        printf("work directory: %s\n", amiga_emulation_work_dir(e));
    else
        print_results(amiga_emulation_test_dir(e));
    if (st != AMIGA_EMULATION_DONE)
        fprintf(stderr, "amiga_run: %s%s\n", amiga_emulation_status_name(st),
                st == AMIGA_EMULATION_TIMEOUT && p.work_dir ? " (screenshot: timeout.png)" : "");
    amiga_emulation_free(e);
    free(script);
    free(changes);
    free(mounts);
    return st == AMIGA_EMULATION_DONE ? 0 : 1;
}
