/*
 * testinfo map - the coverage of each test as lcov tracefiles
 * (lib/covmap/covmap.h; cmake/coverage-map.cmake runs it after a coverage
 * test run).
 *
 *   testinfo map --commit SHA [--root DIR] [--drop PREFIX]... [INPUT]... -o DIR
 *
 * Writes DIR/lcov/<test>.info per test, DIR/lcov/all.info with all tests
 * and DIR/commit with SHA.  Exit status 0 if they are written, 1 if an
 * input is broken (the message on stderr), 2 on wrong arguments or a file
 * that cannot be read or written.
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <memory>
#include <string>
#include <vector>
#include "cli.hpp"
extern "C" {
#include "covmap.h"
}
#include "testinfo.h"

static const char *me; /* "testinfo map", the prefix of the messages */

enum kind { K_DEPS, K_LINKMAP, K_OBJSYMS, K_LINES, K_GCOV, K_COV, K_COUNT };

static const struct {
    const char *name;
    int nargs;
} kinds[K_COUNT] = {
    { "deps", 1 }, { "linkmap", 2 }, { "objsyms", 2 }, { "lines", 2 }, { "gcov", 2 }, { "cov", 3 },
};

struct input {
    enum kind kind;
    char *arg[3];
};

static struct input *inputs;
static size_t ninputs;

static int add_input(enum kind k, char **args)
{
    struct input *v = (struct input *)realloc(inputs, (ninputs + 1) * sizeof *v);
    int i;

    if (!v)
        return 0;
    inputs = v;
    v[ninputs].kind = k;
    for (i = 0; i < 3; i++)
        v[ninputs].arg[i] = i < kinds[k].nargs ? strdup(args[i]) : NULL;
    ninputs++;
    return 1;
}

static int find_kind(const char *name)
{
    int k;

    for (k = 0; k < K_COUNT; k++)
        if (!strcmp(name, kinds[k].name))
            return k;
    return -1;
}

/* a whole file, 0-terminated */
static char *read_file(const char *path, size_t *len)
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

/* the lines of an --inputs file */
static int read_inputs(const char *path)
{
    size_t len, pos = 0, n, lineno = 0;
    char *text = read_file(path, &len), line[8192];
    const char *l;

    if (!text) {
        perror(path);
        return 2;
    }
    while ((l = cov_next_line(text, len, &pos, &n))) {
        char *args[5] = { 0 }, *s;
        int na = 0, k;

        lineno++;
        if (!n || n >= sizeof line)
            continue;
        memcpy(line, l, n);
        line[n] = 0;
        for (s = line; na < 5;) {
            args[na++] = s;
            if (!(s = strchr(s, '\t')))
                break;
            *s++ = 0;
        }
        if ((k = find_kind(args[0])) < 0 || na - 1 != kinds[k].nargs) {
            fprintf(stderr, "%s: %s:%zu: not an input: %s\n", me, path, lineno, line);
            free(text);
            return 2;
        }
        if (!add_input((enum kind)k, args + 1)) {
            free(text);
            return 2;
        }
    }
    free(text);
    return 0;
}

/* path with its missing parent directories (path itself is a file) */
static int make_parents(char *path)
{
    char *s;

    for (s = path + 1; (s = strchr(s, '/')); s++) {
        *s = 0;
        if (mkdir(path, 0777) && errno != EEXIST) {
            perror(path);
            *s = '/';
            return 0;
        }
        *s = '/';
    }
    return 1;
}

/* write a file of dir: one tracefile (test i), all.info (i -1) or the
   commit file (commit); 0, or 2 */
static int write_one(struct covmap *m, const char *dir, const char *name, long i,
                     const char *commit)
{
    size_t n = strlen(dir) + strlen(name) + 16;
    char *path = (char *)malloc(n);
    FILE *out = NULL;
    int r = 2;

    if (!path)
        return 2;
    if (commit)
        snprintf(path, n, "%s/%s", dir, name);
    else
        snprintf(path, n, "%s/lcov/%s.info", dir, name);
    if (make_parents(path) && (out = fopen(path, "w"))) {
        if (commit)
            r = fprintf(out, "%s\n", commit) < 0;
        else
            r = i < 0 ? covmap_write_all(m, out) : covmap_write_test(m, (size_t)i, out);
        r = (r | fclose(out)) ? 2 : 0;
    }
    if (r)
        fprintf(stderr, "%s: %s: cannot write\n", me, path);
    free(path);
    return r;
}

static int write_output(struct covmap *m, const char *commit, const char *dir)
{
    long n = covmap_tests(m), i;
    int r = 0;

    if (n < 0) {
        fprintf(stderr, "%s: out of memory\n", me);
        return 2;
    }
    for (i = 0; i < n && !r; i++)
        r = write_one(m, dir, covmap_test(m, (size_t)i), i, NULL);
    if (!r)
        r = write_one(m, dir, "all", -1, NULL);
    if (!r)
        r = write_one(m, dir, "commit", 0, commit);
    return r;
}

/* the run with the inputs of the command line (inputs); 0, 1 or 2 */
static int run(const char *commit, const char *root_dir, const char *output,
               const std::vector<std::string> &drops)
{
    char *root, err[512];
    struct covmap m;
    size_t i;
    int a, k, r = 0;

    if (!(root = realpath(root_dir ? root_dir : ".", NULL))) {
        fprintf(stderr, "%s: --root: %s\n", me, strerror(errno));
        return 2;
    }
    covmap_init(&m, root);
    free(root);
    for (const std::string &d : drops)
        if (covmap_drop(&m, d.c_str()))
            return 2;
    /* the build's objects and the symbols before the coverage */
    for (k = 0; k < K_COUNT && !r; k++)
        for (i = 0; i < ninputs && !r; i++) {
            struct input *in = &inputs[i];
            const char *path = in->arg[kinds[k].nargs - 1];
            size_t len;
            char *data;

            if (in->kind != (enum kind)k)
                continue;
            if (!(data = read_file(path, &len))) {
                perror(path);
                r = 2;
                break;
            }
            switch (in->kind) {
            case K_DEPS: r = covmap_add_deps(&m, data, err, sizeof err); break;
            case K_LINKMAP: r = covmap_add_linkmap(&m, in->arg[0], data, err, sizeof err); break;
            case K_OBJSYMS: r = covmap_add_objsyms(&m, in->arg[0], data, err, sizeof err); break;
            case K_LINES: r = covmap_add_lines(&m, in->arg[0], data, err, sizeof err); break;
            case K_GCOV: r = covmap_add_gcov(&m, in->arg[0], data, len, err, sizeof err); break;
            case K_COV:
                r = covmap_add_cov(&m, in->arg[0], in->arg[1], (const unsigned char *)data, len,
                                   err, sizeof err);
                break;
            default: break;
            }
            free(data);
            if (r) {
                fprintf(stderr, "%s: %s: %s\n", me, path, err);
                r = 1;
            }
        }
    if (!r)
        r = write_output(&m, commit, output);
    covmap_free(&m);
    for (i = 0; i < ninputs; i++)
        for (a = 0; a < 3; a++)
            free(inputs[i].arg[a]);
    free(inputs);
    return r;
}

void cmd_map(CLI::App &app, int &status)
{
    struct options {
        std::string commit, root, output;
        std::vector<std::string> drop, inputs, values[K_COUNT];
    };
    static const char *const help[K_COUNT] = {
        "ninja -t deps of a build (the source of each object)",
        "the link map of an Amiga program",
        "nm -A of its objects (static functions too)",
        "objdump --dwarf=decodedline of its objects (line ranges)",
        "gcov --json-format --stdout of a host test",
        "the coverage file a test left for PROGRAM",
    };
    static const char *const types[K_COUNT] = {
        "FILE", "PROGRAM FILE", "PROGRAM FILE", "PROGRAM FILE", "TEST FILE", "TEST PROGRAM FILE",
    };
    auto o = std::make_shared<options>();
    CLI::App *sub = app.add_subcommand("map", "the coverage of each test as lcov tracefiles");
    sub->add_option("--commit", o->commit, "the commit of the coverage run, written to DIR/commit")
        ->type_name("SHA")
        ->required();
    sub->add_option("--root", o->root, "the repository (default: the current directory)")->type_name("DIR");
    cli::add_repeated(*sub, "--drop", o->drop, 1, "leave out the files below PREFIX")->type_name("PREFIX");
    for (int k = 0; k < K_COUNT; k++)
        cli::add_repeated(*sub, std::string("--") + kinds[k].name, o->values[k], kinds[k].nargs, help[k])
            ->type_name(types[k]);
    cli::add_repeated(*sub, "--inputs", o->inputs, 1,
                      "inputs, one per line: the option without \"--\" and its arguments, "
                      "separated by tabs")
        ->type_name("FILE");
    sub->add_option("-o", o->output, "the output directory")->type_name("DIR")->required();
    sub->footer("Writes the coverage of each test as lcov tracefile DIR/lcov/<test>.info, all of "
                "them as DIR/lcov/all.info, and SHA as DIR/commit; files outside the root and below "
                "a --drop prefix are left out. Exit status 0 if they are written, 1 if an input is "
                "broken (the message on stderr), 2 usage error or a file that cannot be read or "
                "written.");
    sub->callback([sub, o, &status] {
        const std::string name = cli::name(*sub);

        me = name.c_str();
        /* the inputs in the order given, those of --inputs files where they are named */
        for (const cli::occurrence &oc : cli::occurrences(*sub)) {
            const std::string opt = oc.option->get_name();
            std::vector<char *> args;

            for (const std::string &v : oc.values)
                args.push_back(const_cast<char *>(v.c_str()));
            if (opt == "--inputs") {
                if ((status = read_inputs(args[0])))
                    return;
            } else if (opt.size() > 2 && find_kind(opt.c_str() + 2) >= 0) {
                if (!add_input((enum kind)find_kind(opt.c_str() + 2), args.data())) {
                    status = 2;
                    return;
                }
            }
        }
        status = run(o->commit.c_str(), o->root.empty() ? NULL : o->root.c_str(), o->output.c_str(), o->drop);
    });
}
