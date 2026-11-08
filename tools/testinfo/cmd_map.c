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
#include "covmap.h"
#include "testinfo.h"

const char cmd_map_usage[] =
    "usage: testinfo map --commit SHA [--root DIR] [--drop PREFIX]... [INPUT]... -o DIR\n"
    "Write the coverage of each test as lcov tracefile DIR/lcov/<test>.info, all\n"
    "of them as DIR/lcov/all.info, and SHA as DIR/commit.\n"
    "INPUT:\n"
    "  --deps FILE              ninja -t deps of a build (the source of each object)\n"
    "  --linkmap PROGRAM FILE   the link map of an Amiga program\n"
    "  --objsyms PROGRAM FILE   nm -A of its objects (static functions too)\n"
    "  --lines PROGRAM FILE     objdump --dwarf=decodedline of its objects (line ranges)\n"
    "  --gcov TEST FILE         gcov --json-format --stdout of a host test\n"
    "  --cov TEST PROGRAM FILE  the coverage file a test left for PROGRAM\n"
    "  --inputs FILE            inputs, one per line: the option without \"--\" and its\n"
    "                           arguments, separated by tabs\n"
    "--root: the repository (default: the current directory); files outside it\n"
    "and below a --drop prefix are left out.\n";

static const char *me; /* "testinfo map", the prefix of the messages */

static void usage(FILE *f)
{
    fputs(cmd_map_usage, f);
}

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
    struct input *v = realloc(inputs, (ninputs + 1) * sizeof *v);
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
        && (s = malloc((size_t)n + 1)) && fread(s, 1, (size_t)n, f) == (size_t)n) {
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
    char *path = malloc(n);
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

int cmd_map(int argc, char **argv)
{
    const char *commit = NULL, *output = NULL;
    char *root = NULL, err[512];
    struct covmap m;
    size_t i;
    int a, k, r = 0;

    me = argv[0];
    for (a = 1; a < argc; a++) {
        if (!strcmp(argv[a], "--commit") && a + 1 < argc)
            commit = argv[++a];
        else if (!strcmp(argv[a], "--root") && a + 1 < argc)
            root = argv[++a];
        else if (!strcmp(argv[a], "-o") && a + 1 < argc)
            output = argv[++a];
        else if (!strcmp(argv[a], "--drop") && a + 1 < argc)
            a++;                /* below, once the root is known */
        else if (!strcmp(argv[a], "--inputs") && a + 1 < argc) {
            if ((r = read_inputs(argv[++a])))
                return r;
        } else if (!strncmp(argv[a], "--", 2) && (k = find_kind(argv[a] + 2)) >= 0
                   && a + kinds[k].nargs < argc) {
            if (!add_input((enum kind)k, argv + a + 1))
                return 2;
            a += kinds[k].nargs;
        } else {
            usage(stderr);
            return 2;
        }
    }
    if (!commit || !output) {
        usage(stderr);
        return 2;
    }
    if (!(root = realpath(root ? root : ".", NULL))) {
        fprintf(stderr, "%s: --root: %s\n", me, strerror(errno));
        return 2;
    }
    covmap_init(&m, root);
    free(root);
    for (a = 1; a < argc; a++)
        if (!strcmp(argv[a], "--drop") && a + 1 < argc && covmap_drop(&m, argv[++a]))
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
